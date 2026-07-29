// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
namespace pkgcheck_exec {
enum class error_code { invalid_session, inconsistent_authority, invalid_path, duplicate_input, missing_input, backend_contract_violation, identity_failed };
[[nodiscard]] std::string_view to_string(error_code) noexcept;
class error final : public std::runtime_error { public: error(error_code, std::string); [[nodiscard]] error_code code() const noexcept; private: error_code code_; };
}
