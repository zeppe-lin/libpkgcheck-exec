// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <libpkgcheck/libpkgcheck.h>
#include <libpkgexec/libpkgexec.h>

namespace pkgcheck_exec::detail {

[[nodiscard]] pkgcheck::check_execution_evidence_identity
execution_evidence_identity(const pkgexec::execution_result& execution);

[[nodiscard]] pkgcheck::check_failure_evidence_identity
failure_evidence_identity(const pkgexec::execution_result& execution);

[[nodiscard]] pkgcheck::check_failure_kind classify_failure(
    const pkgexec::execution_result& execution) noexcept;

[[nodiscard]] pkgcheck::check_result expected_check_result(
    const pkgexec::execution_result& execution,
    pkgcheck::check_request request);

} // namespace pkgcheck_exec::detail
