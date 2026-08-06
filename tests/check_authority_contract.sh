#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu
root=${1:?}

grep -q 'execution_purpose::check()' "$root/src/executor.cpp"
grep -q 'check_input_tree' "$root/src/executor.cpp"
grep -q 'std::lower_bound' "$root/src/model.cpp"
grep -q 'paths_overlap' "$root/src/model.cpp"
grep -q 'supplementary credential groups must be unique' "$root/src/model.cpp"
grep -q 'execution.backend() != advertised_backend' "$root/src/executor.cpp"
! grep -q 'expected\[index\]' "$root/src/model.cpp"
! grep -R 'fork\|execve\|waitpid' "$root/src" >/dev/null


grep -q 'seal_execution_request' "$root/include/libpkgcheck-exec/executor.h"
grep -q '^pkgexec::execution_request seal_execution_request(' "$root/src/executor.cpp"
grep -q 'auto request = seal_execution_request(session);' "$root/src/executor.cpp"
grep -q 'prepared.request == sealed_request' "$root/tests/session_test.cpp"
