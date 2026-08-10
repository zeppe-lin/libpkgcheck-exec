// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <libpkgbuild/libpkgbuild.h>
#include <libpkgcatalog/libpkgcatalog.h>
#include <libpkgcheck-exec/libpkgcheck-exec.h>
#include <libpkgresolve/libpkgresolve.h>
#include <libpkgsource/libpkgsource.h>
#include <libpkgstate/libpkgstate.h>
#include <libpkgtransaction/libpkgtransaction.h>

namespace {

std::string hex(char digit)
{
  return std::string(64, digit);
}

pkgsource::declaration_provenance at(const char* path)
{
  return pkgsource::declaration_provenance("recipe.yml", path, 1, 1);
}

pkgsource::source_snapshot source(
    const pkgsource::profile_catalog& profiles,
    const char* name,
    bool checked)
{
  std::vector<pkgsource::requirement_declaration> requirements;
  if (checked) {
    requirements.emplace_back(
        pkgsource::requirement_scope::check(),
        pkgsource::requirement_subject(pkgsource::package_reference("tester")),
        at("requirements.check[0]"));
  }
  std::optional<pkgsource::program> check;
  if (checked)
    check.emplace(pkgsource::program_language::posix_shell, "true\n");
  pkgsource::recipe_declaration recipe(
      pkgsource::package_release(pkgsource::package_reference(name), "1.0.0", 1),
      pkgsource::package_metadata(name, std::nullopt, std::nullopt,
                                  {"GPL-3.0-or-later"}),
      {}, pkgsource::program(pkgsource::program_language::posix_shell, "true\n"),
      std::move(requirements), {},
      pkgsource::architecture_requirements(
          {pkgsource::architecture_reference("x86_64")},
          {pkgsource::architecture_reference("x86_64")}),
      at("recipe"), std::move(check));
  return pkgsource::seal_source(
      pkgsource::source_origin(std::string(name) + "/recipe.yml"),
      std::move(recipe), profiles);
}

template<typename Identity>
Identity state_identity(std::uint8_t seed)
{
  pkgstate::sha256_digest_bytes bytes{};
  for (std::size_t index = 0; index < bytes.size(); ++index)
    bytes[index] = static_cast<std::uint8_t>(seed + index);
  return Identity::from_sha256(bytes);
}

pkgstate::state_target_binding target()
{
  return pkgstate::state_target_binding::make(
      state_identity<pkgstate::managed_target_identity>(1),
      state_identity<pkgstate::state_store_identity>(2),
      state_identity<pkgstate::root_view_identity>(3),
      state_identity<pkgstate::state_backend_identity>(4),
      state_identity<pkgstate::publication_domain_identity>(5));
}

const pkgtransaction::transaction_node& node(
    const pkgtransaction::transaction_program& program,
    pkgtransaction::transaction_action_kind action)
{
  const auto found = std::find_if(
      program.nodes().begin(), program.nodes().end(),
      [action](const auto& value) {
        return value.action() == action && value.package().name() == "checked";
      });
  if (found == program.nodes().end())
    throw std::runtime_error("installed consumer lacks transaction node");
  return *found;
}

std::vector<pkgexec::execution_guarantee> all_guarantees()
{
  return {
      pkgexec::execution_guarantee::exact_interpreter,
      pkgexec::execution_guarantee::closed_environment,
      pkgexec::execution_guarantee::root_view,
      pkgexec::execution_guarantee::read_only_resources,
      pkgexec::execution_guarantee::writable_resources,
      pkgexec::execution_guarantee::fixed_credentials,
      pkgexec::execution_guarantee::network_denied,
      pkgexec::execution_guarantee::loopback_isolated,
      pkgexec::execution_guarantee::resource_limits,
      pkgexec::execution_guarantee::cancellation,
      pkgexec::execution_guarantee::complete_stdout_capture,
      pkgexec::execution_guarantee::complete_stderr_capture,
      pkgexec::execution_guarantee::cleanup_verified,
      pkgexec::execution_guarantee::cpu_time_limit,
      pkgexec::execution_guarantee::address_space_limit,
      pkgexec::execution_guarantee::file_size_limit,
      pkgexec::execution_guarantee::open_files_limit,
      pkgexec::execution_guarantee::process_count_limit,
  };
}

class backend final : public pkgexec::execution_backend {
public:
  pkgexec::backend_capability_profile capabilities() const override
  {
    return pkgexec::backend_capability_profile::seal(
        pkgexec::backend_identity::from_sha256(hex('9')),
        all_guarantees());
  }

  pkgexec::execution_result execute(
      const pkgexec::execution_request& request,
      const pkgexec::execution_resources&) override
  {
    return pkgexec::execution_result::succeeded(
        request, capabilities(), request.interpreter(),
        pkgexec::stream_capture::retained("ok\n"),
        pkgexec::stream_capture::retained(""),
        request.required_guarantees());
  }
};

pkgcheck_exec::admitted_check_session session(bool invalid_source_path = false)
{
  const auto profiles = pkgsource::profile_catalog::seal({});
  auto checked = source(profiles, "checked", true);
  auto tester = source(profiles, "tester", false);
  auto checked_identity = checked.identity();
  pkgcatalog::collection_declaration collection(
      pkgcatalog::collection_reference("core"),
      pkgcatalog::collection_provenance(
          "/collections/core", std::nullopt,
          pkgsource::declaration_provenance("catalog", "collections[0]", 1, 1)),
      {checked, tester});
  std::vector<pkgcatalog::catalog_collection> collections;
  collections.emplace_back(0, pkgcatalog::seal_collection(std::move(collection)));
  auto catalog = pkgcatalog::catalog_snapshot::seal(profiles, std::move(collections));
  auto state = pkgstate::snapshot::make(target());

  auto resolution_request = pkgresolve::resolution_request::seal(
      std::move(catalog), std::move(state),
      pkgresolve::architecture_context(
          pkgsource::architecture_reference("x86_64"),
          pkgsource::architecture_reference("x86_64")),
      {pkgresolve::resolution_goal(
          pkgsource::requirement_scope::check(),
          pkgsource::requirement_subject(pkgsource::package_reference("checked")),
          "installed-consumer")});
  auto resolution = pkgresolve::resolve(std::move(resolution_request));
  auto transaction = pkgtransaction::compose(
      pkgtransaction::transaction_request::seal(std::move(resolution)));

  const auto& build_node = node(
      transaction, pkgtransaction::transaction_action_kind::build);
  const auto& check_node = node(
      transaction, pkgtransaction::transaction_action_kind::check);
  if (!build_node.selection())
    throw std::runtime_error("installed consumer lacks build selection");

  auto build_request = pkgbuild::build_request::seal(
      transaction.request().resolution(), build_node.selection()->identity(),
      pkgbuild::build_policy::make(
          pkgbuild::environment_policy::hermetic(1, 0022, 1700000000)));
  auto build = pkgbuild::build_result::succeeded(
      std::move(build_request), pkgbuild::payload_manifest::seal({}),
      pkgbuild::sealed_artifact::make(
          pkgbuild::artifact_encoding::package_tar,
          pkgbuild::artifact_compression::none, 1,
          pkgbuild::sha256_digest(hex('a'))),
      pkgbuild::execution_evidence_identity::from_sha256(hex('b')));
  const auto artifact = build.artifact()->identity();
  auto check = pkgcheck::check_request::seal(
      transaction, check_node.identity(), std::move(build));

  std::vector<pkgcheck_exec::package_input_resource> inputs;
  char seed = 'd';
  for (const auto& input : check.inputs().inputs()) {
    inputs.push_back({
        input.identity(),
        pkgexec::resource_identity::from_sha256(hex(seed++)),
        "/trees/input/tester",
    });
  }

  return pkgcheck_exec::admitted_check_session::admit(
      std::move(check),
      {
          checked_identity,
          pkgexec::resource_identity::from_sha256(hex('a')),
          invalid_source_path ? "relative/source" : "/trees/source",
      },
      {
          artifact,
          pkgexec::resource_identity::from_sha256(hex('b')),
          "/trees/package",
      },
      std::move(inputs),
      {
          pkgexec::root_view_identity::from_sha256(hex('c')),
          "/",
          "/tmp/check-session",
      },
      {
          pkgexec::interpreter_identity::from_sha256(hex('e')),
          1000,
          1000,
          {},
      });
}

} // namespace

int main()
{
  auto admitted = session();
  backend executor;
  auto result = pkgcheck_exec::execute(admitted, executor);
  if (result.check().outcome() != pkgcheck::check_outcome::passed)
    return 1;
  if (result.execution().request() != pkgcheck_exec::seal_execution_request(admitted))
    return 2;

  const auto encoding = pkgcheck_exec::encode_check_execution_result(result);
  auto decoded = pkgcheck_exec::decode_check_execution_result(
      encoding, admitted.request(), result.execution().request(),
      result.execution().backend());
  if (decoded.check().identity() != result.check().identity())
    return 3;
  if (pkgcheck_exec::encode_check_execution_result(decoded) != encoding)
    return 4;

  try {
    (void)session(true);
    return 5;
  } catch (const pkgcheck_exec::error& value) {
    if (value.code() != pkgcheck_exec::error_code::invalid_path)
      return 6;
  }
  return 0;
}
