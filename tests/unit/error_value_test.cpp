// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/test.h"

#include <array>
#include <iostream>
#include <string_view>
#include <utility>

int main()
{
  try {
    using pkgcheck_exec::error_code;
    const std::array<std::pair<error_code, std::string_view>, 10> expected{{
        {error_code::invalid_session, "invalid-session"},
        {error_code::inconsistent_authority, "inconsistent-authority"},
        {error_code::invalid_path, "invalid-path"},
        {error_code::duplicate_input, "duplicate-input"},
        {error_code::missing_input, "missing-input"},
        {error_code::backend_contract_violation, "backend-contract-violation"},
        {error_code::identity_failed, "identity-failed"},
        {error_code::inconsistent_result, "inconsistent-result"},
        {error_code::corrupt_encoding, "corrupt-encoding"},
        {error_code::authority_mismatch, "authority-mismatch"},
    }};
    for (const auto& value : expected)
      TEST_CHECK(pkgcheck_exec::to_string(value.first) == value.second);
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
