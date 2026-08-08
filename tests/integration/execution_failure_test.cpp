// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fixtures/backend.h"
#include "../support/test.h"

#include <iostream>

namespace {

using execution_fixture::admit;
using execution_fixture::single_input_fixture;
using pkgcheck_exec_test::backend_mode;
using pkgcheck_exec_test::fixture_backend;

void prove_failure_mapping()
{
  const auto session = admit(single_input_fixture());

  fixture_backend unavailable(backend_mode::unavailable);
  const auto before = pkgcheck_exec::execute(session, unavailable);
  TEST_CHECK(before.check().outcome() == pkgcheck::check_outcome::failed);
  TEST_CHECK(before.check().failure() ==
             pkgcheck::check_failure_kind::execution_unavailable);
  TEST_CHECK(before.execution().start_state() ==
             pkgexec::execution_start_state::not_started);

  fixture_backend failed(backend_mode::program_failure);
  const auto after = pkgcheck_exec::execute(session, failed);
  TEST_CHECK(after.check().failure() ==
             pkgcheck::check_failure_kind::program_failed);
  TEST_CHECK(after.execution().start_state() ==
             pkgexec::execution_start_state::started);

  fixture_backend signaled(backend_mode::started_failure);
  const auto other = pkgcheck_exec::execute(session, signaled);
  TEST_CHECK(other.check().failure() ==
             pkgcheck::check_failure_kind::program_failed);
  TEST_CHECK(other.execution().termination()->kind() ==
             pkgexec::process_termination_kind::signaled);
}

} // namespace

int main()
{
  try {
    prove_failure_mapping();
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
