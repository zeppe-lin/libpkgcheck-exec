// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fixtures/backend.h"
#include "../support/test.h"

#include <iostream>

int main()
{
  try {
    const auto session =
        execution_fixture::admit(execution_fixture::single_input_fixture());
    pkgcheck_exec_test::fixture_backend backend(
        pkgcheck_exec_test::backend_mode::program_failure);
    const auto result = pkgcheck_exec::execute(session, backend);

    TEST_CHECK(result.check().request().identity() ==
               session.request().identity());
    TEST_CHECK(result.execution().request().identity() ==
               pkgcheck_exec::seal_execution_request(session).identity());
    TEST_CHECK(result.check().execution_evidence().hex() !=
               result.check().failure_evidence()->hex());

    const auto again = pkgcheck_exec::execute(session, backend);
    TEST_CHECK(result.execution().identity() == again.execution().identity());
    TEST_CHECK(result.check().identity() == again.check().identity());
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
