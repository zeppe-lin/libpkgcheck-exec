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
  TEST_CHECK(environment.additional_variables().size() == 2U);
  TEST_CHECK(environment.additional_variables()[0].name() == "PKG_PACKAGE_ROOT");
  TEST_CHECK(environment.additional_variables()[0].value() ==
             "/check/inputs/package");
  TEST_CHECK(environment.additional_variables()[1].name() == "PKG_SOURCE_ROOT");
  TEST_CHECK(environment.additional_variables()[1].value() == "/check/source");

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
  TEST_CHECK(package.mount_point().string() == "/check/inputs/package");

  const auto& temporary = request.resources().binding(temporary_slot);
  TEST_CHECK(temporary.access() == pkgexec::resource_access::writable);
  TEST_CHECK(temporary.mount_point().string() == "/tmp");

  for (const auto& input : session.inputs()) {
    const auto slot = pkgexec::resource_slot::named(
        pkgexec::resource_role::check_input_tree, input.input.hex());
    const auto& binding = request.resources().binding(slot);
    TEST_CHECK(binding.resource() == input.resource);
    TEST_CHECK(binding.access() == pkgexec::resource_access::read_only);
    TEST_CHECK(binding.mount_point().string() ==
               "/check/inputs/" + input.input.hex());
  }
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
    prove_host_coordinates_are_not_semantic();
    prove_adapter_resource_collision_is_pure_refusal();
    prove_semantic_changes_change_identity();
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
