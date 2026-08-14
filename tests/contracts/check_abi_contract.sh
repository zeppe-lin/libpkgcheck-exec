#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
fail() { echo "abi-contract: $*" >&2; exit 1; }
manifest=$root/abi/libpkgcheck-exec.exports
[ -s "$manifest" ] || fail 'reviewed ELF ABI manifest is absent'
[ "$(sed -n '/^_Z[A-Za-z0-9_]*$/p' "$manifest" | wc -l)" -eq 25 ] || fail 'reviewed ELF ABI manifest must contain exactly 25 symbols'
[ "$(LC_ALL=C sort -u "$manifest" | wc -l)" -eq 25 ] || fail 'reviewed ELF ABI manifest contains duplicate symbols'
! grep -E '^_ZNSt|^_ZN9__gnu_cxx' "$manifest" >/dev/null || fail 'standard-library implementation symbol entered ABI manifest'
demangled=$(mktemp); trap 'rm -f "$demangled"' EXIT HUP INT TERM
c++filt < "$manifest" > "$demangled"
! grep -F 'pkgcheck_exec::detail::' "$demangled" >/dev/null || fail 'private detail symbol entered ABI manifest'
! grep -F 'admitted_check_session::admitted_check_session(' "$demangled" >/dev/null || fail 'private admitted-session constructor entered ABI manifest'
! grep -F 'check_execution_result::check_execution_result(' "$demangled" >/dev/null || fail 'private result constructor entered ABI manifest'
grep -F 'pkgcheck_exec::admitted_check_session::admit(' "$demangled" >/dev/null || fail 'session admission is absent from reviewed ABI'
grep -F 'pkgcheck_exec::execute(' "$demangled" >/dev/null || fail 'execution entry point is absent from reviewed ABI'
grep -F 'pkgcheck_exec::encode_check_execution_result(' "$demangled" >/dev/null || fail 'durable encoder is absent from reviewed ABI'
grep -F 'typeinfo for pkgcheck_exec::error' "$demangled" >/dev/null || fail 'public error RTTI is absent from reviewed ABI'
grep -F "soversion: '2'" "$root/src/meson.build" >/dev/null || fail 'SONAME generation is not 2'
grep -F -- '--version-script=' "$root/src/meson.build" >/dev/null || fail 'reviewed ELF export manifest is not linked'
