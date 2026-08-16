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
grep -F '"PKG_SOURCE_ROOT"' "$executor" >/dev/null
grep -F '"PKG_PACKAGE_ROOT"' "$executor" >/dev/null
if grep -R -n 'ZEPPE_LIN_CHECK_' "$root/src" "$root/include" \
    "$root/tests/integration" "$root/tests/unit" "$root/tests/protocol" \
    "$root/tests/fixtures" "$root/tests/installed" \
    "$root/README.md" "$root/DESIGN.md" "$root/TESTING.md" "$root/man" >/dev/null; then
  echo 'authority-contract: branded check environment ABI resurfaced' >&2
  exit 1
fi
! grep -R -E 'fork\(|execve\(|waitpid\(' "$root/src" >/dev/null
for required in \
  'session.request().build().request().policy().environment()' \
  'policy.parallelism()' \
  'policy.file_creation_mask()' \
  'policy.source_date_epoch()' \
  'variables.emplace_back("PKG_JOBS", std::to_string(policy.parallelism()))'; do
  grep -F -- "$required" "$executor" >/dev/null || {
    echo "check environment omits admitted build policy authority: $required" >&2
    exit 1
  }
done
if grep -E '^[[:space:]]+1,[[:space:]]*$|^[[:space:]]+0022,[[:space:]]*$' "$executor" >/dev/null; then
  echo 'check executor regained a private hard-coded build environment field' >&2
  exit 1
fi
