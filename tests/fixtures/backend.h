// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "session.h"

#include <libpkgcheck-exec/libpkgcheck-exec.h>

#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pkgcheck_exec_test {

inline std::vector<pkgexec::execution_guarantee> all_guarantees()
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

inline pkgexec::backend_capability_profile capabilities(char seed = '9')
{
  return pkgexec::backend_capability_profile::seal(
      pkgexec::backend_identity::from_sha256(execution_fixture::hex(seed)),
      all_guarantees());
}

enum class backend_mode {
  succeed,
  unavailable,
  program_failure,
  started_failure,
  throw_execute,
  throw_execute_nonstandard,
  throw_capabilities,
  throw_capabilities_nonstandard,
  return_other_request,
  return_other_backend,
};

class fixture_backend final : public pkgexec::execution_backend {
public:
  explicit fixture_backend(
      backend_mode mode,
      std::optional<pkgexec::execution_request> other_request = std::nullopt)
      : mode_(mode), other_request_(std::move(other_request))
  {
  }

  pkgexec::backend_capability_profile capabilities() const override
  {
    if (mode_ == backend_mode::throw_capabilities)
      throw std::runtime_error("capabilities unavailable");
    if (mode_ == backend_mode::throw_capabilities_nonstandard)
      throw 7;
    return pkgcheck_exec_test::capabilities();
  }

  pkgexec::execution_result execute(
      const pkgexec::execution_request& request,
      const pkgexec::execution_resources&) override
  {
    if (mode_ == backend_mode::throw_execute)
      throw std::runtime_error("backend escaped its evidence contract");
    if (mode_ == backend_mode::throw_execute_nonstandard)
      throw 9;

    const auto& result_request =
        mode_ == backend_mode::return_other_request ? *other_request_ : request;
    auto profile = mode_ == backend_mode::return_other_backend
        ? pkgcheck_exec_test::capabilities('8')
        : pkgcheck_exec_test::capabilities();

    if (mode_ == backend_mode::unavailable) {
      return pkgexec::execution_result::failed_before_start(
          result_request, std::move(profile),
          pkgexec::execution_failure_kind::interpreter_unavailable, {},
          "missing interpreter");
    }

    if (mode_ == backend_mode::program_failure) {
      return pkgexec::execution_result::failed_after_start(
          result_request, std::move(profile), result_request.interpreter(),
          pkgexec::process_termination::exited(2),
          pkgexec::stream_capture::retained(""),
          pkgexec::stream_capture::retained("failed\n"),
          result_request.required_guarantees(),
          pkgexec::cleanup_outcome::verified,
          pkgexec::execution_failure_kind::program_exited_nonzero,
          "check failed");
    }

    if (mode_ == backend_mode::started_failure) {
      return pkgexec::execution_result::failed_after_start(
          result_request, std::move(profile), result_request.interpreter(),
          pkgexec::process_termination::signaled(9),
          pkgexec::stream_capture::retained("partial\n"),
          pkgexec::stream_capture::retained(""),
          result_request.required_guarantees(),
          pkgexec::cleanup_outcome::verified,
          pkgexec::execution_failure_kind::program_terminated_by_signal,
          "check process terminated");
    }

    return pkgexec::execution_result::succeeded(
        result_request, std::move(profile), result_request.interpreter(),
        pkgexec::stream_capture::retained("ok\n"),
        pkgexec::stream_capture::retained(""),
        result_request.required_guarantees());
  }

private:
  backend_mode mode_;
  std::optional<pkgexec::execution_request> other_request_;
};

} // namespace pkgcheck_exec_test
