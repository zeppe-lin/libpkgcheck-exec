// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fixtures/backend.h"
#include "../support/test.h"

#include <iostream>
#include <utility>

namespace {

using execution_fixture::admit;
using execution_fixture::single_input_fixture;
using pkgcheck_exec_test::backend_mode;
using pkgcheck_exec_test::capabilities;
using pkgcheck_exec_test::expect_error;
using pkgcheck_exec_test::fixture_backend;

pkgcheck::check_request foreign_check_request()
{
  auto scenario = check_fixture::make_scenario("printf 'other\\n'\n");
  auto build = check_fixture::successful_build(scenario.transaction, '6');
  return pkgcheck::check_request::seal(
      scenario.transaction,
      check_fixture::node(
          scenario.transaction,
          pkgtransaction::transaction_action_kind::check).identity(),
      std::move(build));
}

void prove_corrupt_bytes_are_refused()
{
  const auto session = admit(single_input_fixture());
  fixture_backend backend(backend_mode::succeed);
  const auto result = pkgcheck_exec::execute(session, backend);
  const auto encoding = pkgcheck_exec::encode_check_execution_result(result);

  auto corrupted = encoding;
  corrupted[corrupted.size() / 2U] ^= 0x01U;
  expect_error(pkgcheck_exec::error_code::corrupt_encoding, [&] {
    (void)pkgcheck_exec::decode_check_execution_result(
        corrupted, session.request(), result.execution().request(),
        result.execution().backend());
  });

  auto truncated = encoding;
  truncated.pop_back();
  expect_error(pkgcheck_exec::error_code::corrupt_encoding, [&] {
    (void)pkgcheck_exec::decode_check_execution_result(
        truncated, session.request(), result.execution().request(),
        result.execution().backend());
  });

  auto extended = encoding;
  extended.push_back(0U);
  expect_error(pkgcheck_exec::error_code::corrupt_encoding, [&] {
    (void)pkgcheck_exec::decode_check_execution_result(
        extended, session.request(), result.execution().request(),
        result.execution().backend());
  });
}

void prove_authority_substitution_is_refused()
{
  const auto session = admit(single_input_fixture());
  fixture_backend backend(backend_mode::program_failure);
  const auto result = pkgcheck_exec::execute(session, backend);
  const auto encoding = pkgcheck_exec::encode_check_execution_result(result);

  expect_error(pkgcheck_exec::error_code::authority_mismatch, [&] {
    (void)pkgcheck_exec::decode_check_execution_result(
        encoding, foreign_check_request(), result.execution().request(),
        result.execution().backend());
  });

  auto foreign = single_input_fixture();
  foreign.identity.user_id = 1001;
  const auto foreign_request =
      pkgcheck_exec::seal_execution_request(admit(std::move(foreign)));
  expect_error(pkgcheck_exec::error_code::authority_mismatch, [&] {
    (void)pkgcheck_exec::decode_check_execution_result(
        encoding, session.request(), foreign_request,
        result.execution().backend());
  });

  expect_error(pkgcheck_exec::error_code::authority_mismatch, [&] {
    (void)pkgcheck_exec::decode_check_execution_result(
        encoding, session.request(), result.execution().request(),
        capabilities('8'));
  });
}

} // namespace

int main()
{
  try {
    prove_corrupt_bytes_are_refused();
    prove_authority_substitution_is_refused();
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
