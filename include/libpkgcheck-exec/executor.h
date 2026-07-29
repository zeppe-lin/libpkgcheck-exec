// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <libpkgcheck-exec/model.h>
namespace pkgcheck_exec {
[[nodiscard]] prepared_execution prepare(const admitted_check_session&);
[[nodiscard]] check_execution_result execute(const admitted_check_session&, pkgexec::execution_backend&);
}
