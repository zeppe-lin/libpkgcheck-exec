// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgcheck-exec/error.h>

#include <utility>

namespace pkgcheck_exec {

std::string_view to_string(error_code value) noexcept
{
  switch (value) {
  case error_code::invalid_session:
    return "invalid-session";
  case error_code::inconsistent_authority:
    return "inconsistent-authority";
  case error_code::invalid_path:
    return "invalid-path";
  case error_code::duplicate_input:
    return "duplicate-input";
  case error_code::missing_input:
    return "missing-input";
  case error_code::backend_contract_violation:
    return "backend-contract-violation";
  case error_code::identity_failed:
    return "identity-failed";
  case error_code::inconsistent_result:
    return "inconsistent-result";
  case error_code::corrupt_encoding:
    return "corrupt-encoding";
  case error_code::authority_mismatch:
    return "authority-mismatch";
  }
  return "unknown";
}

error::error(error_code code, std::string message)
    : std::runtime_error(std::move(message)), code_(code)
{
}

error::~error() = default;

error_code error::code() const noexcept
{
  return code_;
}

} // namespace pkgcheck_exec
