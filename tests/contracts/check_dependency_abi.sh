#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
library=${1:?shared library required}
fail() { echo "dependency-abi-test: $*" >&2; exit 1; }
command -v readelf >/dev/null 2>&1 || fail 'readelf is required'
needed=$(readelf -d "$library" | sed -n 's/.*Shared library: \[\([^]]*\)\].*/\1/p')
for expected in libpkgcheck.so.1 libpkgexec.so.1; do
  printf '%s\n' "$needed" | grep -Fx "$expected" >/dev/null || fail "missing direct NEEDED $expected"
done
! printf '%s\n' "$needed" | grep -Fx libpkgcheck.so.0 >/dev/null || fail 'obsolete libpkgcheck.so.0 provider admitted'
