// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fixtures/session.h"
#include "../support/test.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
namespace fs = std::filesystem;
using execution_fixture::admit;
using execution_fixture::hex;
using execution_fixture::single_input_fixture;
using pkgcheck_exec_test::expect_error;

void multi_input_sets_are_canonical()
{
  auto scenario = check_fixture::make_multi_input_scenario();
  auto build = check_fixture::successful_multi_input_build(scenario);
  auto request = pkgcheck::check_request::seal(
      scenario.transaction,
      check_fixture::node(
          scenario.transaction,
          pkgtransaction::transaction_action_kind::check).identity(),
      build);

  TEST_CHECK(request.inputs().inputs().size() == 2U);
  std::vector<pkgcheck_exec::package_input_resource> supplied;
  char seed = '4';
  for (const auto& input : request.inputs().inputs()) {
    supplied.push_back({
        input.identity(),
        pkgexec::resource_identity::from_sha256(hex(seed++)),
        "/trees/input/" + input.package().name(),
    });
  }
  std::reverse(supplied.begin(), supplied.end());

  auto admitted = execution_fixture::multi_input_session(
      request, scenario, build, supplied);
  TEST_CHECK(admitted.inputs().size() == request.inputs().inputs().size());
  for (std::size_t index = 0; index < admitted.inputs().size(); ++index)
    TEST_CHECK(admitted.inputs()[index].input ==
               request.inputs().inputs()[index].identity());

  auto canonical = supplied;
  std::reverse(canonical.begin(), canonical.end());
  const auto canonical_session = execution_fixture::multi_input_session(
      request, scenario, build, canonical);
  TEST_CHECK(pkgcheck_exec::seal_execution_request(admitted).identity() ==
             pkgcheck_exec::seal_execution_request(canonical_session).identity());

  auto missing = supplied;
  missing.pop_back();
  expect_error(pkgcheck_exec::error_code::missing_input, [&] {
    (void)execution_fixture::multi_input_session(
        request, scenario, build, missing);
  });

  auto duplicate = supplied;
  duplicate[1] = duplicate[0];
  duplicate[1].path = "/trees/input/duplicate";
  duplicate[1].resource = pkgexec::resource_identity::from_sha256(hex('8'));
  expect_error(pkgcheck_exec::error_code::duplicate_input, [&] {
    (void)execution_fixture::multi_input_session(
        request, scenario, build, duplicate);
  });

  auto unrelated = supplied;
  unrelated[0].input = pkgbuild::build_input_identity::from_sha256(hex('0'));
  expect_error(pkgcheck_exec::error_code::missing_input, [&] {
    (void)execution_fixture::multi_input_session(
        request, scenario, build, unrelated);
  });
}

void credentials_are_canonical()
{
  auto first = single_input_fixture();
  first.identity.supplementary_groups = {3000, 2000};
  auto second = single_input_fixture();
  second.identity.supplementary_groups = {2000, 3000};

  const auto admitted_first = admit(std::move(first));
  const auto admitted_second = admit(std::move(second));
  TEST_CHECK(admitted_first.identity().supplementary_groups ==
             std::vector<std::uint64_t>({2000, 3000}));
  TEST_CHECK(pkgcheck_exec::seal_execution_request(admitted_first).identity() ==
             pkgcheck_exec::seal_execution_request(admitted_second).identity());

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

void paths_are_normalized_and_disjoint()
{
  auto normalized = single_input_fixture();
  normalized.source.path = "/trees/source/../source";
  normalized.paths.root_view_path = "/root/../";
  const auto normalized_session = admit(std::move(normalized));
  TEST_CHECK(normalized_session.source().path == fs::path("/trees/source"));
  TEST_CHECK(normalized_session.paths().root_view_path == fs::path("/"));

  auto relative = single_input_fixture();
  relative.source.path = "relative/source";
  expect_error(pkgcheck_exec::error_code::invalid_path, [&] {
    (void)admit(std::move(relative));
  });

  auto relative_root = single_input_fixture();
  relative_root.paths.root_view_path = "root";
  expect_error(pkgcheck_exec::error_code::invalid_path, [&] {
    (void)admit(std::move(relative_root));
  });

  auto root_source = single_input_fixture();
  root_source.source.path = "/";
  expect_error(pkgcheck_exec::error_code::invalid_path, [&] {
    (void)admit(std::move(root_source));
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
}

void semantic_authorities_are_exact()
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

void concrete_resource_identities_do_not_alias()
{
  auto source_package = single_input_fixture();
  source_package.package.tree = source_package.source.tree;
  expect_error(pkgcheck_exec::error_code::invalid_session, [&] {
    (void)admit(std::move(source_package));
  });

  auto source_input = single_input_fixture();
  source_input.inputs[0].resource = source_input.source.tree;
  expect_error(pkgcheck_exec::error_code::invalid_session, [&] {
    (void)admit(std::move(source_input));
  });

  auto package_input = single_input_fixture();
  package_input.inputs[0].resource = package_input.package.tree;
  expect_error(pkgcheck_exec::error_code::invalid_session, [&] {
    (void)admit(std::move(package_input));
  });
}

void resource_limits_are_retained()
{
  auto fixture = single_input_fixture();
  fixture.limits = pkgexec::resource_limits::make(
      1250, 512U * 1024U * 1024U, 4096, 64, 32);
  const auto session = admit(std::move(fixture));
  TEST_CHECK(session.limits().cpu_time_milliseconds() == 1250U);
  TEST_CHECK(session.limits().address_space_bytes() == 512U * 1024U * 1024U);
  TEST_CHECK(session.limits().file_size_bytes() == 4096U);
  TEST_CHECK(session.limits().open_files() == 64U);
  TEST_CHECK(session.limits().process_count() == 32U);
}

} // namespace

int main()
{
  try {
    multi_input_sets_are_canonical();
    credentials_are_canonical();
    paths_are_normalized_and_disjoint();
    semantic_authorities_are_exact();
    concrete_resource_identities_do_not_alias();
    resource_limits_are_retained();
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
