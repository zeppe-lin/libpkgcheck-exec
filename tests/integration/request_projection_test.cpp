// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fixtures/session.h"
#include "../support/test.h"

#include <algorithm>
#include <iostream>
#include <utility>

namespace {

using execution_fixture::admit;
using execution_fixture::single_input_fixture;

bool has_guarantee(const pkgexec::execution_request& request,
                   pkgexec::execution_guarantee value)
{
  const auto& guarantees = request.required_guarantees();
  return std::find(guarantees.begin(), guarantees.end(), value) !=
         guarantees.end();
}

void prove_exact_projection()
{
  auto fixture = single_input_fixture();
  fixture.identity.supplementary_groups = {3000, 2000};
  fixture.limits =
      pkgexec::resource_limits::make(1000, 1024U * 1024U, 4096, 64, 8);
  const auto session = admit(std::move(fixture));
  const auto request = pkgcheck_exec::seal_execution_request(session);

  TEST_CHECK(request.purpose().kind() ==
             pkgexec::execution_purpose_kind::check);
  TEST_CHECK(request.program() == session.request().program());
  TEST_CHECK(request.interpreter() == session.identity().interpreter);
  TEST_CHECK(request.root_view() == session.paths().root_view);
  TEST_CHECK(request.resources().working_directory() ==
             pkgexec::resource_slot::named(
                 pkgexec::resource_role::build_input_tree,
                 "checked-package"));

  const auto& environment = request.environment();
  TEST_CHECK(environment.network() == pkgexec::network_policy::denied);
  TEST_CHECK(environment.standard_input() == pkgexec::stdin_policy::closed);
  TEST_CHECK(environment.standard_output() ==
             pkgexec::stream_policy::capture_complete);
  TEST_CHECK(environment.standard_error() ==
             pkgexec::stream_policy::capture_complete);
  TEST_CHECK(environment.home_directory().string() == "/tmp/home");
  TEST_CHECK(environment.temporary_directory().string() == "/tmp");
  TEST_CHECK(environment.parallelism() == 1U);
  TEST_CHECK(environment.file_creation_mask() == 0022U);
  TEST_CHECK(environment.additional_variables().size() == 4U);
  TEST_CHECK(environment.additional_variables()[0].name() == "PKG_CHECK_INPUTS");
  TEST_CHECK(environment.additional_variables()[0].value() == "tester");
  TEST_CHECK(environment.additional_variables()[1].name() ==
             "PKG_CHECK_INPUT_ROOT");
  TEST_CHECK(environment.additional_variables()[1].value() == "/check/inputs");
  TEST_CHECK(environment.additional_variables()[2].name() == "PKG_PACKAGE_ROOT");
  TEST_CHECK(environment.additional_variables()[2].value() ==
             "/check/inputs/_package");
  TEST_CHECK(environment.additional_variables()[3].name() == "PKG_SOURCE_ROOT");
  TEST_CHECK(environment.additional_variables()[3].value() == "/check/source");

  const auto& credentials = request.credentials();
  TEST_CHECK(credentials.user_id() == 1000U);
  TEST_CHECK(credentials.group_id() == 1000U);
  TEST_CHECK(credentials.supplementary_groups() ==
             std::vector<std::uint64_t>({2000, 3000}));
  TEST_CHECK(credentials.no_new_privileges());
  TEST_CHECK(request.limits() == session.limits());
  TEST_CHECK(request.cancellation().mode() ==
             pkgexec::cancellation_mode::disabled);
  TEST_CHECK(!has_guarantee(request, pkgexec::execution_guarantee::cancellation));
  TEST_CHECK(has_guarantee(request, pkgexec::execution_guarantee::network_denied));
  TEST_CHECK(has_guarantee(request, pkgexec::execution_guarantee::resource_limits));
}

void prove_exact_bindings()
{
  const auto session = admit(single_input_fixture());
  const auto request = pkgcheck_exec::seal_execution_request(session);

  const auto source_slot = pkgexec::resource_slot::named(
      pkgexec::resource_role::source_tree, "checked-source");
  const auto package_slot = pkgexec::resource_slot::named(
      pkgexec::resource_role::build_input_tree, "checked-package");
  const auto temporary_slot = pkgexec::resource_slot::singleton(
      pkgexec::resource_role::private_temporary_root);

  const auto& source = request.resources().binding(source_slot);
  TEST_CHECK(source.resource() == session.source().tree);
  TEST_CHECK(source.access() == pkgexec::resource_access::read_only);
  TEST_CHECK(source.mount_point().string() == "/check/source");

  const auto& package = request.resources().binding(package_slot);
  TEST_CHECK(package.resource() == session.package().tree);
  TEST_CHECK(package.access() == pkgexec::resource_access::read_only);
  TEST_CHECK(package.mount_point().string() == "/check/inputs/_package");

  const auto& temporary = request.resources().binding(temporary_slot);
  TEST_CHECK(temporary.access() == pkgexec::resource_access::writable);
  TEST_CHECK(temporary.mount_point().string() == "/tmp");

  for (const auto& input : session.inputs()) {
    const auto found = std::find_if(
        session.request().inputs().inputs().begin(),
        session.request().inputs().inputs().end(),
        [&](const auto& logical) { return logical.identity() == input.input; });
    TEST_CHECK(found != session.request().inputs().inputs().end());
    const auto name = found->package().name();
    const auto slot = pkgexec::resource_slot::named(
        pkgexec::resource_role::check_input_tree, name);
    const auto& binding = request.resources().binding(slot);
    TEST_CHECK(binding.resource() == input.resource);
    TEST_CHECK(binding.access() == pkgexec::resource_access::read_only);
    TEST_CHECK(binding.mount_point().string() == "/check/inputs/" + name);
  }
}

void prove_multi_input_recipe_coordinates()
{
  auto scenario = check_fixture::make_multi_input_scenario();
  auto build = check_fixture::successful_multi_input_build(scenario);
  auto request = pkgcheck::check_request::seal(
      scenario.transaction,
      check_fixture::node(
          scenario.transaction,
          pkgtransaction::transaction_action_kind::check).identity(),
      build);

  std::vector<pkgcheck_exec::package_input_resource> resources;
  char seed = '4';
  for (const auto& input : request.inputs().inputs()) {
    resources.push_back({
        input.identity(),
        pkgexec::resource_identity::from_sha256(
            execution_fixture::hex(seed++)),
        "/trees/input/" + input.package().name(),
    });
  }
  const auto session = execution_fixture::multi_input_session(
      request, scenario, build, std::move(resources));
  const auto projected = pkgcheck_exec::seal_execution_request(session);

  std::string expected_names;
  for (const auto& input : request.inputs().inputs()) {
    if (!expected_names.empty())
      expected_names.push_back(':');
    expected_names += input.package().name();

    const auto slot = pkgexec::resource_slot::named(
        pkgexec::resource_role::check_input_tree, input.package().name());
    TEST_CHECK(projected.resources().binding(slot).mount_point().string() ==
               "/check/inputs/" + input.package().name());
  }

  const auto& variables = projected.environment().additional_variables();
  const auto names = std::find_if(
      variables.begin(), variables.end(),
      [](const auto& value) { return value.name() == "PKG_CHECK_INPUTS"; });
  TEST_CHECK(names != variables.end());
  TEST_CHECK(names->value() == expected_names);
}

void prove_host_coordinates_are_not_semantic()
{
  auto first = single_input_fixture();
  auto second = single_input_fixture();
  second.source.path = "/other/source";
  second.package.path = "/other/package";
  second.inputs[0].path = "/other/input";
  second.paths.root_view_path = "/other/root";
  second.paths.temporary_root = "/other/tmp";

  TEST_CHECK(pkgcheck_exec::seal_execution_request(admit(std::move(first)))
                 .identity() ==
             pkgcheck_exec::seal_execution_request(admit(std::move(second)))
                 .identity());
}

void prove_adapter_resource_collision_is_pure_refusal()
{
  const auto ordinary_session = admit(single_input_fixture());
  const auto ordinary = pkgcheck_exec::seal_execution_request(ordinary_session);
  const auto temporary_slot = pkgexec::resource_slot::singleton(
      pkgexec::resource_role::private_temporary_root);
  const auto temporary_resource =
      ordinary.resources().binding(temporary_slot).resource();

  auto collision = single_input_fixture();
  collision.source.tree = temporary_resource;
  const auto admitted = admit(std::move(collision));
  pkgcheck_exec_test::expect_error(
      pkgcheck_exec::error_code::invalid_session, [&] {
        (void)pkgcheck_exec::seal_execution_request(admitted);
      });
}

void prove_semantic_changes_change_identity()
{
  const auto ordinary =
      pkgcheck_exec::seal_execution_request(admit(single_input_fixture()));

  auto changed = single_input_fixture();
  changed.identity.user_id = 1001;
  TEST_CHECK(ordinary.identity() !=
             pkgcheck_exec::seal_execution_request(admit(std::move(changed)))
                 .identity());

  changed = single_input_fixture();
  changed.limits = pkgexec::resource_limits::make(
      std::nullopt, std::nullopt, 8192, std::nullopt, std::nullopt);
  TEST_CHECK(ordinary.identity() !=
             pkgcheck_exec::seal_execution_request(admit(std::move(changed)))
                 .identity());
}

} // namespace

int main()
{
  try {
    prove_exact_projection();
    prove_exact_bindings();
    prove_multi_input_recipe_coordinates();
    prove_host_coordinates_are_not_semantic();
    prove_adapter_resource_collision_is_pure_refusal();
    prove_semantic_changes_change_identity();
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
