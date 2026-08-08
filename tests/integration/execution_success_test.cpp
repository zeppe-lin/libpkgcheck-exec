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
        pkgcheck_exec_test::backend_mode::succeed);
    const auto result = pkgcheck_exec::execute(session, backend);

    TEST_CHECK(result.check().outcome() == pkgcheck::check_outcome::passed);
    TEST_CHECK(result.check().request() == session.request());
    TEST_CHECK(result.check().execution_evidence().hex().size() == 64U);
    TEST_CHECK(!result.check().failure());
    TEST_CHECK(!result.check().failure_evidence());
    TEST_CHECK(result.execution().request() ==
               pkgcheck_exec::seal_execution_request(session));
    TEST_CHECK(result.execution().established_guarantees() ==
               result.execution().request().required_guarantees());
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
