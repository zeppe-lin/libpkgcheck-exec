#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?}
model=$root/src/model.cpp
executor=$root/src/executor.cpp
api=$root/include/libpkgcheck-exec/model.h
executor_api=$root/include/libpkgcheck-exec/executor.h

grep -q 'pkgcheck::check_request' "$api"
grep -q 'pkgbuild::build_input_identity' "$api"
grep -q 'pkgexec::resource_identity' "$api"
grep -q 'std::lower_bound' "$model"
grep -q 'paths_overlap' "$model"
grep -q 'distinct check resources share one concrete identity' "$model"
grep -q 'supplementary credential groups must be unique' "$model"
grep -q 'seal_execution_request' "$executor_api"
grep -q '^pkgexec::execution_request seal_execution_request(' "$executor"
grep -q 'require_temporary_resource_unique' "$executor"
grep -q 'auto request = seal_execution_request(session);' "$executor"
grep -q 'const auto advertised_backend = backend_capabilities(backend);' "$executor"
grep -q 'execution.backend() != advertised_backend' "$executor"
grep -q 'execution backend threw non-standard capability evidence' "$executor"
grep -q 'execution backend threw non-standard execution evidence' "$executor"
grep -q 'network_policy::denied' "$executor"
grep -q 'cancellation_policy::disabled()' "$executor"
! grep -R -E 'fork\(|execve\(|waitpid\(' "$root/src" >/dev/null
