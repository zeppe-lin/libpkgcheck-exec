// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgcheck-exec/libpkgcheck-exec.h>

#include "check_fixture.h"
#include "test.h"

#include <string>
#include <utility>
#include <vector>

namespace {

std::string hex(char digit)
{
  return std::string(64, digit);
}

std::vector<pkgexec::execution_guarantee> guarantees()
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
  explicit backend(bool pass) : pass_(pass) {}

  pkgexec::backend_capability_profile capabilities() const override
  {
    return pkgexec::backend_capability_profile::seal(
        pkgexec::backend_identity::from_sha256(hex('9')),
        guarantees());
  }

  pkgexec::execution_result execute(
      const pkgexec::execution_request& request,
      const pkgexec::execution_resources&) override
  {
    auto capabilities = this->capabilities();
    if (pass_)
      return pkgexec::execution_result::succeeded(
          request,
          std::move(capabilities),
          request.interpreter(),
          pkgexec::stream_capture::retained("ok\n"),
          pkgexec::stream_capture::retained(""),
          request.required_guarantees());

    return pkgexec::execution_result::failed_before_start(
        request,
        std::move(capabilities),
        pkgexec::execution_failure_kind::interpreter_unavailable,
        {},
        "missing");
  }

private:
  bool pass_;
};

pkgcheck_exec::admitted_check_session session()
{
  auto scenario = check_fixture::make_scenario();
  auto build = check_fixture::successful_build(
      scenario.checked, scenario.tester);
  auto request = pkgcheck::check_request::seal(
      scenario.transaction,
      check_fixture::node(
          scenario.transaction,
          pkgtransaction::transaction_action_kind::check).identity(),
      build);

  std::vector<pkgcheck_exec::package_input_tree> inputs;
  char seed = 'd';
  for (const auto& input : request.inputs().inputs()) {
    inputs.push_back({
        input.resolved().identity(),
        input.tree(),
        pkgexec::resource_identity::from_sha256(hex(seed++)),
        "/trees/input",
    });
  }

  return pkgcheck_exec::admitted_check_session::admit(
      std::move(request),
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

void reject_stale_source_authority()
{
  auto scenario = check_fixture::make_scenario();
  auto build = check_fixture::successful_build(
      scenario.checked, scenario.tester);
  auto request = pkgcheck::check_request::seal(
      scenario.transaction,
      check_fixture::node(
          scenario.transaction,
          pkgtransaction::transaction_action_kind::check).identity(),
      build);

  std::vector<pkgcheck_exec::package_input_tree> inputs;
  for (const auto& input : request.inputs().inputs()) {
    inputs.push_back({
        input.resolved().identity(),
        input.tree(),
        pkgexec::resource_identity::from_sha256(hex('f')),
        "/trees/input",
    });
  }

  bool rejected = false;
  try {
    (void)pkgcheck_exec::admitted_check_session::admit(
        std::move(request),
        {
            pkgsource::source_snapshot_identity::from_sha256(hex('0')),
            pkgexec::resource_identity::from_sha256(hex('a')),
            "/trees/source",
        },
        {
            build.artifact()->identity(),
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
  } catch (const pkgcheck_exec::error& exception) {
    rejected = exception.code() ==
               pkgcheck_exec::error_code::inconsistent_authority;
  }
  TEST_CHECK(rejected);
}

} // namespace

int main()
{
  try {
    auto admitted = session();
    auto prepared = pkgcheck_exec::prepare(admitted);
    TEST_CHECK(prepared.request.purpose().kind() ==
               pkgexec::execution_purpose_kind::check);
    TEST_CHECK(prepared.request.program() == admitted.request().program());

    backend succeeding_backend(true);
    auto passed = pkgcheck_exec::execute(admitted, succeeding_backend);
    TEST_CHECK(passed.check().outcome() == pkgcheck::check_outcome::passed);

    backend unavailable_backend(false);
    auto failed = pkgcheck_exec::execute(admitted, unavailable_backend);
    TEST_CHECK(failed.check().failure() ==
               pkgcheck::check_failure_kind::execution_unavailable);

    reject_stale_source_authority();
    return 0;
  } catch (const std::exception& exception) {
    std::cerr << "unexpected exception: " << exception.what() << '\n';
    return 1;
  }
}
