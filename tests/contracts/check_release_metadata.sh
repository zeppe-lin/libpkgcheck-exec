#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
fail() { echo "release-metadata: $*" >&2; exit 1; }
version=$(sed -n "s/^[[:space:]]*version: '\([^']*\)'.*/\1/p" "$root/meson.build" | head -n 1)
[ "$version" = 0.5.0 ] || fail "project version is '$version', expected 0.5.0"
grep -F 'Version: 0.5.0' "$root/HISTORY.md" >/dev/null || fail 'HISTORY omits 0.5.0'
grep -F "soversion: '2'" "$root/src/meson.build" >/dev/null || fail 'shared library is not SONAME 2'
block() { sed -n "/^[[:space:]]*'$1',[[:space:]]*$/,/^[[:space:]]*)/p" "$root/meson.build"; }
for spec in 'libpkgcheck >=0.3.0 <1.0.0' 'libpkgexec >=2.1.1 <3.0.0'; do
  set -- $spec; dep=$1; lo=$2; hi=$3; b=$(block "$dep")
  printf '%s\n' "$b" | grep -F "'$lo'" >/dev/null || fail "$dep omits $lo"
  printf '%s\n' "$b" | grep -F "'$hi'" >/dev/null || fail "$dep omits $hi"
done
grep -F 'requires: public_deps' "$root/src/meson.build" >/dev/null || fail 'pkg-config requirements are not dependency-object backed'
grep -F 'requires_private: [libcrypto_dep]' "$root/src/meson.build" >/dev/null || fail 'private crypto metadata is not dependency-object backed'
