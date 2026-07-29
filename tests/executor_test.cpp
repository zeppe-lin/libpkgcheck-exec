// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgcheck-exec/libpkgcheck-exec.h>

#include "session_fixture.h"
#include "test.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
namespace fs = std::filesystem;
using execution_fixture::admit;
using execution_fixture::hex;
using execution_fixture::single_input_fixture;

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

pkgexec::backend_capability_profile capability_profile(char seed = '9')
{
  return pkgexec::backend_capability_profile::seal(
      pkgexec::backend_identity::from_sha256(hex(seed)), guarantees());
}

enum class backend_mode {
  succeed,
  unavailable,
  program_failure,
  throw_exception,
  throw_capabilities,
  return_other_request,
  return_other_backend,
};

class backend final : public pkgexec::execution_backend {
public:
  explicit backend(
      backend_mode mode,
      std::optional<pkgexec::execution_request> other_request = std::nullopt)
      : mode_(mode), other_request_(std::move(other_request))
  {
  }

  pkgexec::backend_capability_profile capabilities() const override
  {
    if (mode_ == backend_mode::throw_capabilities)
      throw std::runtime_error("capabilities unavailable");
    return capability_profile();
  }

  pkgexec::execution_result execute(
      const pkgexec::execution_request& request,
      const pkgexec::execution_resources&) override
  {
    if (mode_ == backend_mode::throw_exception)
      throw std::runtime_error("backend escaped its evidence contract");

    const auto& result_request =
        mode_ == backend_mode::return_other_request
            ? *other_request_
            : request;
    auto profile = mode_ == backend_mode::return_other_backend
        ? capability_profile('8')
        : capabilities();

    if (mode_ == backend_mode::unavailable) {
      return pkgexec::execution_result::failed_before_start(
          result_request,
          std::move(profile),
          pkgexec::execution_failure_kind::interpreter_unavailable,
          {},
          "missing interpreter");
    }

    if (mode_ == backend_mode::program_failure) {
      return pkgexec::execution_result::failed_after_start(
          result_request,
          std::move(profile),
          result_request.interpreter(),
          pkgexec::process_termination::exited(2),
          pkgexec::stream_capture::retained(""),
          pkgexec::stream_capture::retained("failed\n"),
          result_request.required_guarantees(),
          pkgexec::cleanup_outcome::verified,
          pkgexec::execution_failure_kind::program_exited_nonzero,
          "check failed");
    }

    return pkgexec::execution_result::succeeded(
        result_request,
        std::move(profile),
        result_request.interpreter(),
        pkgexec::stream_capture::retained("ok\n"),
        pkgexec::stream_capture::retained(""),
        result_request.required_guarantees());
  }

private:
  backend_mode mode_;
  std::optional<pkgexec::execution_request> other_request_;
};

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

void prove_request_and_resource_translation()
{
  auto admitted = admit(single_input_fixture());
  auto prepared = pkgcheck_exec::prepare(admitted);

  TEST_CHECK(prepared.request.purpose().kind() ==
             pkgexec::execution_purpose_kind::check);
  TEST_CHECK(prepared.request.program() == admitted.request().program());
  TEST_CHECK(prepared.request.resources().working_directory() ==
             pkgexec::resource_slot::named(
                 pkgexec::resource_role::build_input_tree,
                 "checked-package"));
  TEST_CHECK(prepared.resources.root_view() == admitted.paths().root_view);
  TEST_CHECK(prepared.resources.root_view_path() == fs::path("/"));

  const auto source_slot = pkgexec::resource_slot::named(
      pkgexec::resource_role::source_tree, "checked-source");
  const auto package_slot = pkgexec::resource_slot::named(
      pkgexec::resource_role::build_input_tree, "checked-package");
  const auto temporary_slot = pkgexec::resource_slot::singleton(
      pkgexec::resource_role::private_temporary_root);

  const auto& source = prepared.request.resources().binding(source_slot);
  TEST_CHECK(source.resource() == admitted.source().tree);
  TEST_CHECK(source.access() == pkgexec::resource_access::read_only);
  TEST_CHECK(source.mount_point().string() == "/check/source");

  const auto& package = prepared.request.resources().binding(package_slot);
  TEST_CHECK(package.resource() == admitted.package().tree);
  TEST_CHECK(package.access() == pkgexec::resource_access::read_only);
  TEST_CHECK(package.mount_point().string() == "/check/package");

  const auto& temporary = prepared.request.resources().binding(temporary_slot);
  TEST_CHECK(temporary.access() == pkgexec::resource_access::writable);
  TEST_CHECK(temporary.mount_point().string() == "/tmp");

  TEST_CHECK(prepared.resources.materialization(admitted.source().tree)
                 .host_path() == fs::path("/trees/source"));
  TEST_CHECK(prepared.resources.materialization(admitted.package().tree)
                 .host_path() == fs::path("/trees/package"));
  TEST_CHECK(prepared.resources.materialization(temporary.resource())
                 .host_path() == fs::path("/tmp/check-session"));
}

void prove_host_coordinates_do_not_change_semantic_request()
{
  auto first = single_input_fixture();
  auto second = single_input_fixture();
  second.source.path = "/other/source";
  second.package.path = "/other/package";
  second.inputs[0].path = "/other/input";
  second.paths.root_view_path = "/other/root";
  second.paths.temporary_root = "/other/tmp";

  const auto prepared_first = pkgcheck_exec::prepare(admit(std::move(first)));
  const auto prepared_second = pkgcheck_exec::prepare(admit(std::move(second)));
  TEST_CHECK(prepared_first.request.identity() ==
             prepared_second.request.identity());
  TEST_CHECK(prepared_first.resources.root_view_path() !=
             prepared_second.resources.root_view_path());
}

void prove_terminal_result_mapping()
{
  auto admitted = admit(single_input_fixture());

  backend succeeding(backend_mode::succeed);
  const auto passed = pkgcheck_exec::execute(admitted, succeeding);
  TEST_CHECK(passed.check().outcome() == pkgcheck::check_outcome::passed);
  TEST_CHECK(passed.check().request() == admitted.request());
  TEST_CHECK(passed.execution().request() ==
             pkgcheck_exec::prepare(admitted).request);

  backend unavailable(backend_mode::unavailable);
  const auto unavailable_result = pkgcheck_exec::execute(admitted, unavailable);
  TEST_CHECK(unavailable_result.check().failure() ==
             pkgcheck::check_failure_kind::execution_unavailable);

  backend program_failure(backend_mode::program_failure);
  const auto program_result =
      pkgcheck_exec::execute(admitted, program_failure);
  TEST_CHECK(program_result.check().failure() ==
             pkgcheck::check_failure_kind::program_failed);
}

void prove_backend_contract_enforcement()
{
  auto admitted = admit(single_input_fixture());

  backend throwing(backend_mode::throw_exception);
  expect_error(pkgcheck_exec::error_code::backend_contract_violation, [&] {
    (void)pkgcheck_exec::execute(admitted, throwing);
  });

  backend capability_failure(backend_mode::throw_capabilities);
  expect_error(pkgcheck_exec::error_code::backend_contract_violation, [&] {
    (void)pkgcheck_exec::execute(admitted, capability_failure);
  });

  backend wrong_profile(backend_mode::return_other_backend);
  expect_error(pkgcheck_exec::error_code::backend_contract_violation, [&] {
    (void)pkgcheck_exec::execute(admitted, wrong_profile);
  });

  auto other_fixture = single_input_fixture();
  other_fixture.identity.user_id = 1001;
  auto other = admit(std::move(other_fixture));
  backend stale(backend_mode::return_other_request,
                pkgcheck_exec::prepare(other).request);
  expect_error(pkgcheck_exec::error_code::backend_contract_violation, [&] {
    (void)pkgcheck_exec::execute(admitted, stale);
  });
}

} // namespace

int main()
{
  try {
    prove_request_and_resource_translation();
    prove_host_coordinates_do_not_change_semantic_request();
    prove_terminal_result_mapping();
    prove_backend_contract_enforcement();
    return 0;
  } catch (const std::exception& exception) {
    std::cerr << "unexpected exception: " << exception.what() << '\n';
    return 1;
  }
}
