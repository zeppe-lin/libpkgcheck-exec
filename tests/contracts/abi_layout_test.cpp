// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgcheck-exec/libpkgcheck-exec.h>

static_assert(sizeof(void*) == 8,
              "libpkgcheck-exec 0.4 ABI layout contract requires 64-bit pointers");
static_assert(sizeof(pkgcheck::check_request) == 3944);
static_assert(sizeof(pkgcheck::check_result) == 4064);
static_assert(sizeof(pkgexec::execution_request) == 720);
static_assert(sizeof(pkgexec::execution_resources) == 96);
static_assert(sizeof(pkgexec::execution_result) == 1160);
static_assert(sizeof(pkgcheck_exec::source_tree) == 104);
static_assert(sizeof(pkgcheck_exec::checked_package_tree) == 104);
static_assert(sizeof(pkgcheck_exec::package_input_resource) == 104);
static_assert(sizeof(pkgcheck_exec::session_paths) == 112);
static_assert(sizeof(pkgcheck_exec::execution_identity) == 72);
static_assert(sizeof(pkgcheck_exec::admitted_check_session) == 4472);
static_assert(sizeof(pkgcheck_exec::prepared_execution) == 816);
static_assert(sizeof(pkgcheck_exec::check_execution_result) == 5224);
int main() { return 0; }
