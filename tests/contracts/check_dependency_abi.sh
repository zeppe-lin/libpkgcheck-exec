#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
library=${1:?shared library required}
fail() { echo "dependency-abi-test: $*" >&2; exit 1; }
command -v readelf >/dev/null 2>&1 || fail 'readelf is required'
needed=$(readelf -d "$library" | sed -n 's/.*Shared library: \[\([^]]*\)\].*/\1/p')
for expected in libpkgcheck.so.2 libpkgexec.so.2; do
  printf '%s\n' "$needed" | grep -Fx "$expected" >/dev/null || fail "missing direct NEEDED $expected"
done
for obsolete_check in libpkgcheck.so.0 libpkgcheck.so.1; do
  ! printf '%s\n' "$needed" | grep -Fx "$obsolete_check" >/dev/null || fail "obsolete $obsolete_check provider admitted"
done
for obsolete in libpkgexec.so.0 libpkgexec.so.1; do
  ! printf '%s\n' "$needed" | grep -Fx "$obsolete" >/dev/null || fail "obsolete $obsolete provider admitted"
done
