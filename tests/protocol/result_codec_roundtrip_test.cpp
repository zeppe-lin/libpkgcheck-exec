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

void require_same_result(
    const pkgcheck_exec::check_execution_result& lhs,
    const pkgcheck_exec::check_execution_result& rhs)
{
  TEST_CHECK(lhs.execution().identity() == rhs.execution().identity());
  TEST_CHECK(lhs.execution().request() == rhs.execution().request());
  TEST_CHECK(lhs.execution().backend() == rhs.execution().backend());
  TEST_CHECK(lhs.execution().status() == rhs.execution().status());
  TEST_CHECK(lhs.execution().start_state() == rhs.execution().start_state());
  TEST_CHECK(lhs.execution().failure() == rhs.execution().failure());
  TEST_CHECK(lhs.execution().diagnostic() == rhs.execution().diagnostic());
  TEST_CHECK(lhs.check().identity() == rhs.check().identity());
  TEST_CHECK(lhs.check().request() == rhs.check().request());
  TEST_CHECK(lhs.check().outcome() == rhs.check().outcome());
  TEST_CHECK(lhs.check().execution_evidence() ==
             rhs.check().execution_evidence());
  TEST_CHECK(lhs.check().failure() == rhs.check().failure());
  TEST_CHECK(lhs.check().failure_evidence() == rhs.check().failure_evidence());
}

void prove_roundtrip(backend_mode mode)
{
  const auto session = admit(single_input_fixture());
  fixture_backend backend(mode);
  const auto original = pkgcheck_exec::execute(session, backend);
  const auto encoding =
      pkgcheck_exec::encode_check_execution_result(original);
  const auto decoded = pkgcheck_exec::decode_check_execution_result(
      encoding, session.request(), original.execution().request(),
      original.execution().backend());

  require_same_result(original, decoded);
  TEST_CHECK(pkgcheck_exec::encode_check_execution_result(decoded) == encoding);
}

} // namespace

int main()
{
  try {
    prove_roundtrip(backend_mode::succeed);
    prove_roundtrip(backend_mode::unavailable);
    prove_roundtrip(backend_mode::program_failure);
    prove_roundtrip(backend_mode::started_failure);
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
