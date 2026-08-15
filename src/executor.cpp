// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgcheck-exec/executor.h>

#include <libpkgcheck-exec/error.h>

#include "result_identity.h"

#include <openssl/evp.h>

#include <array>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pkgcheck_exec {
namespace {

constexpr std::string_view source_path = "/check/source";
constexpr std::string_view package_path = "/check/inputs/package";
constexpr std::string_view input_path_prefix = "/check/inputs/";
constexpr std::string_view temporary_path = "/tmp";
constexpr std::string_view home_path = "/tmp/home";

std::string sha256_hex(std::string_view material)
{
  using context_pointer =
      std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

  context_pointer context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  if (!context ||
      EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(context.get(), material.data(), material.size()) != 1)
    throw error(error_code::identity_failed,
                "cannot hash execution evidence");

  std::array<unsigned char, 32> digest{};
  unsigned int digest_size = 0;
  if (EVP_DigestFinal_ex(context.get(), digest.data(), &digest_size) != 1 ||
      digest_size != digest.size())
    throw error(error_code::identity_failed,
                "cannot finalize execution evidence");

  static constexpr char digits[] = "0123456789abcdef";
  std::string result(digest.size() * 2, '0');
  for (std::size_t index = 0; index < digest.size(); ++index) {
    result[index * 2] = digits[digest[index] >> 4];
    result[index * 2 + 1] = digits[digest[index] & 0x0f];
  }
  return result;
}

std::string identity_material(std::string_view domain,
                              std::string_view authority)
{
  std::string material;
  material.reserve(domain.size() + authority.size());
  material.append(domain);
  material.append(authority);
  return material;
}

pkgexec::resource_identity temporary_resource_identity(
    const pkgcheck::check_request& request)
{
  const auto material = identity_material(
      "pkgcheck-exec/temp/v1", request.identity().hex());
  return pkgexec::resource_identity::from_sha256(sha256_hex(material));
}

std::vector<pkgexec::environment_variable> environment_variables(
    const admitted_check_session& session)
{
  std::vector<pkgexec::environment_variable> variables;
  variables.emplace_back("PKG_SOURCE_ROOT", std::string(source_path));
  variables.emplace_back("PKG_PACKAGE_ROOT", std::string(package_path));
  return variables;
}

pkgexec::environment_policy environment_for(
    const admitted_check_session& session)
{
  using namespace pkgexec;

  return environment_policy::hermetic(
      {logical_path::parse("/usr/bin"), logical_path::parse("/bin")},
      logical_path::parse(home_path),
      logical_path::parse(temporary_path),
      1,
      0022,
      std::nullopt,
      network_policy::denied,
      stdin_policy::closed,
      stream_policy::capture_complete,
      stream_policy::capture_complete,
      environment_variables(session));
}

pkgexec::credential_policy credentials_for(
    const admitted_check_session& session)
{
  const auto& identity = session.identity();
  return pkgexec::credential_policy::fixed(
      identity.user_id,
      identity.group_id,
      identity.supplementary_groups,
      true);
}

pkgexec::backend_capability_profile backend_capabilities(
    pkgexec::execution_backend& backend)
{
  try {
    return backend.capabilities();
  } catch (const std::exception& exception) {
    throw error(
        error_code::backend_contract_violation,
        std::string("execution backend could not report capabilities: ") +
            exception.what());
  } catch (...) {
    throw error(error_code::backend_contract_violation,
                "execution backend threw non-standard capability evidence");
  }
}

pkgexec::execution_result invoke_backend(
    pkgexec::execution_backend& backend,
    const prepared_execution& prepared)
{
  try {
    return backend.execute(prepared.request, prepared.resources);
  } catch (const std::exception& exception) {
    throw error(
        error_code::backend_contract_violation,
        std::string("execution backend threw instead of returning evidence: ") +
            exception.what());
  } catch (...) {
    throw error(error_code::backend_contract_violation,
                "execution backend threw non-standard execution evidence");
  }
}

void require_temporary_resource_unique(
    const admitted_check_session& session,
    const pkgexec::resource_identity& temporary)
{
  if (temporary == session.source().tree ||
      temporary == session.package().tree)
    throw error(error_code::invalid_session,
                "temporary resource aliases an admitted read-only resource");
  for (const auto& input : session.inputs()) {
    if (temporary == input.resource)
      throw error(error_code::invalid_session,
                  "temporary resource aliases an admitted check input");
  }
}

} // namespace

pkgexec::execution_request seal_execution_request(
    const admitted_check_session& session)
{
  using namespace pkgexec;

  const auto source_slot = resource_slot::named(
      resource_role::source_tree, "checked-source");
  const auto package_slot = resource_slot::named(
      resource_role::build_input_tree, "checked-package");
  const auto temporary_slot = resource_slot::singleton(
      resource_role::private_temporary_root);

  const auto temporary_resource =
      temporary_resource_identity(session.request());
  require_temporary_resource_unique(session, temporary_resource);

  std::vector<resource_binding> bindings;
  bindings.emplace_back(
      source_slot,
      session.source().tree,
      resource_access::read_only,
      logical_path::parse(source_path));
  bindings.emplace_back(
      package_slot,
      session.package().tree,
      resource_access::read_only,
      logical_path::parse(package_path));
  bindings.emplace_back(
      temporary_slot,
      temporary_resource,
      resource_access::writable,
      logical_path::parse(temporary_path));

  for (const auto& input : session.inputs()) {
    bindings.emplace_back(
        resource_slot::named(resource_role::check_input_tree,
                             input.input.hex()),
        input.resource,
        resource_access::read_only,
        logical_path::parse(
            std::string(input_path_prefix) + input.input.hex()));
  }

  auto layout = resource_layout::seal(std::move(bindings), package_slot);
  return execution_request::seal(
      session.request().program(),
      execution_purpose::check(),
      session.identity().interpreter,
      session.paths().root_view,
      std::move(layout),
      environment_for(session),
      credentials_for(session),
      session.limits(),
      cancellation_policy::disabled());
}

prepared_execution prepare(const admitted_check_session& session)
{
  using namespace pkgexec;

  const auto source_slot = resource_slot::named(
      resource_role::source_tree, "checked-source");
  const auto package_slot = resource_slot::named(
      resource_role::build_input_tree, "checked-package");
  const auto temporary_slot = resource_slot::singleton(
      resource_role::private_temporary_root);
  auto request = seal_execution_request(session);

  std::vector<resource_materialization> materializations;
  materializations.emplace_back(session.source().tree,
                                session.source().path);
  materializations.emplace_back(session.package().tree,
                                session.package().path);
  materializations.emplace_back(
      request.resources().binding(temporary_slot).resource(),
      session.paths().temporary_root);
  for (const auto& input : session.inputs())
    materializations.emplace_back(input.resource, input.path);

  auto resources = execution_resources::admit(
      request,
      session.paths().root_view,
      session.paths().root_view_path,
      std::move(materializations));

  return {std::move(request), std::move(resources)};
}

check_execution_result execute(const admitted_check_session& session,
                               pkgexec::execution_backend& backend)
{
  auto prepared = prepare(session);
  const auto advertised_backend = backend_capabilities(backend);
  auto execution = invoke_backend(backend, prepared);

  if (execution.request() != prepared.request)
    throw error(error_code::backend_contract_violation,
                "execution backend returned evidence for another request");
  if (execution.backend() != advertised_backend)
    throw error(error_code::backend_contract_violation,
                "execution backend returned evidence for another backend profile");

  auto check = detail::expected_check_result(
      execution, session.request());
  return check_execution_result(std::move(execution), std::move(check));
}

} // namespace pkgcheck_exec
