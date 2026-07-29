// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <libpkgcheck-exec/model.h>

namespace pkgcheck_exec {

[[nodiscard]] prepared_execution prepare(
    const admitted_check_session& session);

[[nodiscard]] check_execution_result execute(
    const admitted_check_session& session,
    pkgexec::execution_backend& backend);

} // namespace pkgcheck_exec
