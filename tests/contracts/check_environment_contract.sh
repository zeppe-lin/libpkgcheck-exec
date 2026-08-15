#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
executor=$root/src/executor.cpp
fail() { echo "environment-contract: $*" >&2; exit 1; }

# Recipe-facing check dependencies must be addressable by canonical package name,
# not by an opaque build-input identity that a recipe cannot derive.
grep -F 'pkgexec::environment_policy environment_for(' "$executor" >/dev/null || \
  fail 'environment projection is missing'
grep -F 'const admitted_check_session& session' "$executor" >/dev/null || \
  fail 'environment projection does not retain admitted check-input authority'
for variable in PKG_SOURCE_ROOT PKG_PACKAGE_ROOT PKG_CHECK_INPUT_ROOT PKG_CHECK_INPUTS; do
  grep -F "variables.emplace_back(\"$variable\"" "$executor" >/dev/null || \
    fail "$variable projection is missing"
done
count=$(grep -c 'variables.emplace_back(' "$executor")
[ "$count" -eq 4 ] || fail "environment projection exports $count variables, expected 4"
actual_names=$(grep -o '"PKG_[A-Z0-9_]*"' "$executor" | tr -d '"' | LC_ALL=C sort -u)
expected_names=$(printf '%s\n' PKG_CHECK_INPUTS PKG_CHECK_INPUT_ROOT PKG_PACKAGE_ROOT PKG_SOURCE_ROOT | LC_ALL=C sort -u)
[ "$actual_names" = "$expected_names" ] || \
  fail "unexpected PKG_* environment vocabulary: $actual_names"

grep -F 'package_input_name(logical)' "$executor" >/dev/null || \
  fail 'logical check inputs are not projected by package name'
grep -F 'std::string(input_path_prefix) + name' "$executor" >/dev/null || \
  fail 'check-input mount path is not package-addressable'
grep -F 'resource_role::package_tree' "$executor" >/dev/null || \
  fail 'checked package is not projected with package-tree authority'
grep -F 'resource_role::build_input_tree, "checked-package"' "$executor" >/dev/null && \
  fail 'checked package is still misclassified as a build input'
grep -F 'constexpr std::string_view package_path = "/check/package";' "$executor" >/dev/null || \
  fail 'checked package does not use its phase-local subject path'

for doc in README.md DESIGN.md TESTING.md MAINTAINING.md man/libpkgcheck-exec.7.scdoc; do
  for variable in PKG_SOURCE_ROOT PKG_PACKAGE_ROOT PKG_CHECK_INPUT_ROOT PKG_CHECK_INPUTS; do
    grep -F "$variable" "$root/$doc" >/dev/null || fail "$doc omits $variable"
  done
done

if grep -R -n 'ZEPPE_LIN_CHECK_' \
    "$root/src" "$root/include" "$root/tests/integration" "$root/tests/unit" \
    "$root/tests/protocol" "$root/tests/fixtures" "$root/tests/installed" \
    "$root/README.md" "$root/DESIGN.md" "$root/TESTING.md" "$root/MAINTAINING.md" \
    "$root/man" >/dev/null; then
  fail 'distribution-branded check environment ABI resurfaced'
fi
