#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
executor=$root/src/executor.cpp
fail() { echo "environment-contract: $*" >&2; exit 1; }

# The current recipe environment is an exact, fixed two-variable projection.
grep -F 'pkgexec::environment_policy environment_for()' "$executor" >/dev/null || \
  fail 'environment projection unexpectedly depends on admitted session state'
grep -F 'variables.emplace_back("PKG_SOURCE_ROOT", std::string(source_path));' "$executor" >/dev/null || \
  fail 'PKG_SOURCE_ROOT projection is missing'
grep -F 'variables.emplace_back("PKG_PACKAGE_ROOT", std::string(package_path));' "$executor" >/dev/null || \
  fail 'PKG_PACKAGE_ROOT projection is missing'
count=$(grep -c 'variables.emplace_back(' "$executor")
[ "$count" -eq 2 ] || fail "environment projection exports $count variables, expected 2"
actual_names=$(grep -o '"PKG_[A-Z0-9_]*"' "$executor" | tr -d '"' | sort -u)
expected_names=$(printf '%s\n' PKG_PACKAGE_ROOT PKG_SOURCE_ROOT)
[ "$actual_names" = "$expected_names" ] || \
  fail "unexpected PKG_* environment vocabulary: $actual_names"

for doc in README.md DESIGN.md TESTING.md MAINTAINING.md man/libpkgcheck-exec.7.scdoc; do
  grep -F 'PKG_SOURCE_ROOT' "$root/$doc" >/dev/null || fail "$doc omits PKG_SOURCE_ROOT"
  grep -F 'PKG_PACKAGE_ROOT' "$root/$doc" >/dev/null || fail "$doc omits PKG_PACKAGE_ROOT"
done

grep -F 'session-independent' "$root/README.md" >/dev/null || \
  fail 'README does not state session-independent environment authority'
grep -F 'session-independent' "$root/MAINTAINING.md" >/dev/null || \
  fail 'MAINTAINING does not protect session-independent environment authority'
grep -F 'identity convenience variables are not part of this execution ABI' "$root/DESIGN.md" >/dev/null || \
  fail 'DESIGN does not exclude package identity convenience variables'
grep -F 'package identity convenience variables' "$root/man/libpkgcheck-exec.7.scdoc" >/dev/null || \
  fail 'manual does not exclude package identity convenience variables'

if grep -R -n 'ZEPPE_LIN_CHECK_' \
    "$root/src" "$root/include" "$root/tests/integration" "$root/tests/unit" \
    "$root/tests/protocol" "$root/tests/fixtures" "$root/tests/installed" \
    "$root/README.md" "$root/DESIGN.md" "$root/TESTING.md" "$root/MAINTAINING.md" \
    "$root/man" >/dev/null; then
  fail 'distribution-branded check environment ABI resurfaced'
fi
