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
using pkgcheck_exec_test::expect_error;
using pkgcheck_exec_test::fixture_backend;

void prove_backend_contract()
{
  const auto session = admit(single_input_fixture());
  for (const auto mode : {
           backend_mode::throw_execute,
           backend_mode::throw_execute_nonstandard,
           backend_mode::throw_capabilities,
           backend_mode::throw_capabilities_nonstandard,
           backend_mode::return_other_backend,
       }) {
    fixture_backend backend(mode);
    expect_error(pkgcheck_exec::error_code::backend_contract_violation, [&] {
      (void)pkgcheck_exec::execute(session, backend);
    });
  }

  auto other = single_input_fixture();
  other.identity.user_id = 1001;
  fixture_backend stale(
      backend_mode::return_other_request,
      pkgcheck_exec::seal_execution_request(admit(std::move(other))));
  expect_error(pkgcheck_exec::error_code::backend_contract_violation, [&] {
    (void)pkgcheck_exec::execute(session, stale);
  });
}

} // namespace

int main()
{
  try {
    prove_backend_contract();
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
