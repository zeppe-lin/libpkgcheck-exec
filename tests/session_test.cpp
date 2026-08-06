// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgcheck-exec/libpkgcheck-exec.h>

#include "session_fixture.h"
#include "test.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

namespace {
namespace fs = std::filesystem;
using execution_fixture::admit;
using execution_fixture::hex;
using execution_fixture::single_input_fixture;

template<typename Function>
void expect_error(pkgcheck_exec::error_code expected, Function&& function)
{
  bool caught = false;
  try {
    function();
  } catch (const pkgcheck_exec::error& exception) {
    caught = true;
    TEST_CHECK(exception.code() == expected);
  }
  TEST_CHECK(caught);
}

pkgcheck_exec::admitted_check_session multi_input_session(
    pkgcheck::check_request& request,
    std::vector<pkgcheck_exec::package_input_resource>& supplied,
    check_fixture::multi_input_scenario& scenario,
    pkgbuild::build_result& build)
{
  return pkgcheck_exec::admitted_check_session::admit(
      request,
      {
          scenario.checked.identity(),
          pkgexec::resource_identity::from_sha256(hex('a')),
          "/trees/source",
      },
      {
          build.artifact()->identity(),
          pkgexec::resource_identity::from_sha256(hex('b')),
          "/trees/package",
      },
      supplied,
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

void prove_multi_input_set_admission()
{
  auto scenario = check_fixture::make_multi_input_scenario();
  auto build = check_fixture::successful_multi_input_build(scenario);
  auto request = pkgcheck::check_request::seal(
      scenario.transaction,
      check_fixture::node(
          scenario.transaction,
          pkgtransaction::transaction_action_kind::check).identity(),
      build);

  TEST_CHECK(request.inputs().inputs().size() == 2);
  std::vector<std::string> package_names;
  for (const auto& input : request.inputs().inputs())
    package_names.push_back(input.package().name());
  std::sort(package_names.begin(), package_names.end());
  TEST_CHECK(package_names ==
             std::vector<std::string>({"tester-a", "tester-b"}));

  std::vector<pkgcheck_exec::package_input_resource> supplied;
  char resource_seed = '4';
  for (const auto& input : request.inputs().inputs()) {
    supplied.push_back({
        input.identity(),
        pkgexec::resource_identity::from_sha256(hex(resource_seed++)),
        "/trees/input/" + input.package().name(),
    });
  }
  std::reverse(supplied.begin(), supplied.end());

  auto admitted = multi_input_session(request, supplied, scenario, build);
  const auto sealed_request = pkgcheck_exec::seal_execution_request(admitted);
  const auto prepared = pkgcheck_exec::prepare(admitted);
  TEST_CHECK(prepared.request == sealed_request);
  for (std::size_t index = 0; index < admitted.inputs().size(); ++index) {
    const auto& expected = request.inputs().inputs()[index];
    const auto& concrete = admitted.inputs()[index];
    TEST_CHECK(concrete.input == expected.identity());

    const auto slot = pkgexec::resource_slot::named(
        pkgexec::resource_role::check_input_tree,
        concrete.input.hex());
    const auto& binding = prepared.request.resources().binding(slot);
    TEST_CHECK(binding.resource() == concrete.resource);
    TEST_CHECK(binding.access() == pkgexec::resource_access::read_only);
    TEST_CHECK(binding.mount_point().string() ==
               "/check/inputs/" + concrete.input.hex());
    TEST_CHECK(prepared.resources.materialization(concrete.resource)
                   .host_path() == concrete.path);
  }

  auto canonical = supplied;
  std::reverse(canonical.begin(), canonical.end());
  const auto canonical_session =
      multi_input_session(request, canonical, scenario, build);
  TEST_CHECK(pkgcheck_exec::prepare(admitted).request.identity() ==
             pkgcheck_exec::prepare(canonical_session).request.identity());

  auto missing = supplied;
  missing.pop_back();
  expect_error(pkgcheck_exec::error_code::missing_input, [&] {
    (void)multi_input_session(request, missing, scenario, build);
  });

  auto duplicate = supplied;
  duplicate[1] = duplicate[0];
  duplicate[1].path = "/trees/input/duplicate";
  duplicate[1].resource =
      pkgexec::resource_identity::from_sha256(hex('8'));
  expect_error(pkgcheck_exec::error_code::duplicate_input, [&] {
    (void)multi_input_session(request, duplicate, scenario, build);
  });

  auto unrelated = supplied;
  unrelated[0].input =
      pkgbuild::build_input_identity::from_sha256(hex('0'));
  expect_error(pkgcheck_exec::error_code::missing_input, [&] {
    (void)multi_input_session(request, unrelated, scenario, build);
  });
}

void prove_canonical_credentials()
{
  auto first = single_input_fixture();
  first.identity.supplementary_groups = {3000, 2000};
  auto second = single_input_fixture();
  second.identity.supplementary_groups = {2000, 3000};

  const auto admitted_first = admit(std::move(first));
  const auto admitted_second = admit(std::move(second));
  TEST_CHECK(admitted_first.identity().supplementary_groups ==
             std::vector<std::uint64_t>({2000, 3000}));
  TEST_CHECK(pkgcheck_exec::prepare(admitted_first).request.identity() ==
             pkgcheck_exec::prepare(admitted_second).request.identity());

  auto duplicate = single_input_fixture();
  duplicate.identity.supplementary_groups = {2000, 2000};
  expect_error(pkgcheck_exec::error_code::invalid_session, [&] {
    (void)admit(std::move(duplicate));
  });

  auto repeated_primary = single_input_fixture();
  repeated_primary.identity.supplementary_groups = {1000};
  expect_error(pkgcheck_exec::error_code::invalid_session, [&] {
    (void)admit(std::move(repeated_primary));
  });
}

void prove_concrete_resource_isolation()
{
  auto normalized = single_input_fixture();
  normalized.source.path = "/trees/source/../source";
  const auto normalized_session = admit(std::move(normalized));
  TEST_CHECK(normalized_session.source().path == fs::path("/trees/source"));

  auto relative = single_input_fixture();
  relative.source.path = "relative/source";
  expect_error(pkgcheck_exec::error_code::invalid_path, [&] {
    (void)admit(std::move(relative));
  });

  auto root = single_input_fixture();
  root.package.path = "/";
  expect_error(pkgcheck_exec::error_code::invalid_path, [&] {
    (void)admit(std::move(root));
  });

  auto equal_paths = single_input_fixture();
  equal_paths.package.path = equal_paths.source.path;
  expect_error(pkgcheck_exec::error_code::invalid_path, [&] {
    (void)admit(std::move(equal_paths));
  });

  auto nested_temporary = single_input_fixture();
  nested_temporary.paths.temporary_root = "/trees/source/tmp";
  expect_error(pkgcheck_exec::error_code::invalid_path, [&] {
    (void)admit(std::move(nested_temporary));
  });

  auto nested_input = single_input_fixture();
  nested_input.inputs[0].path = "/trees/package/input";
  expect_error(pkgcheck_exec::error_code::invalid_path, [&] {
    (void)admit(std::move(nested_input));
  });

  auto alias = single_input_fixture();
  alias.package.tree = alias.source.tree;
  expect_error(pkgcheck_exec::error_code::invalid_session, [&] {
    (void)admit(std::move(alias));
  });

  auto input_alias = single_input_fixture();
  input_alias.inputs[0].resource = input_alias.source.tree;
  expect_error(pkgcheck_exec::error_code::invalid_session, [&] {
    (void)admit(std::move(input_alias));
  });
}

void prove_exact_source_and_artifact_authority()
{
  auto stale_source = single_input_fixture();
  stale_source.source.source =
      pkgsource::source_snapshot_identity::from_sha256(hex('0'));
  expect_error(pkgcheck_exec::error_code::inconsistent_authority, [&] {
    (void)admit(std::move(stale_source));
  });

  auto stale_artifact = single_input_fixture();
  stale_artifact.package.artifact =
      pkgbuild::artifact_identity::from_sha256(hex('0'));
  expect_error(pkgcheck_exec::error_code::inconsistent_authority, [&] {
    (void)admit(std::move(stale_artifact));
  });
}

} // namespace

int main()
{
  try {
    prove_multi_input_set_admission();
    prove_canonical_credentials();
    prove_concrete_resource_isolation();
    prove_exact_source_and_artifact_authority();
    return 0;
  } catch (const std::exception& exception) {
    std::cerr << "unexpected exception: " << exception.what() << '\n';
    return 1;
  }
}
