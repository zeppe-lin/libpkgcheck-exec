#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
build_root=${1:?build root required}
metadata=$build_root/meson-private/libpkgcheck-exec.pc
fail() {
  echo "metadata-test: $*" >&2
  if [ -s "${metadata:-}" ]; then echo '--- generated metadata ---' >&2; cat "$metadata" >&2; echo '--- end generated metadata ---' >&2; fi
  exit 1
}
if [ ! -s "$metadata" ]; then metadata=$(find "$build_root" -type f -name libpkgcheck-exec.pc -print | sed -n '1p'); fi
[ -n "${metadata:-}" ] && [ -s "$metadata" ] || fail 'generated libpkgcheck-exec.pc was not found'
[ "$(sed -n 's/^Name:[[:space:]]*//p' "$metadata")" = libpkgcheck-exec ] || fail 'wrong module name'
[ "$(sed -n 's/^Version:[[:space:]]*//p' "$metadata")" = 0.5.0 ] || fail 'wrong module version'
normalize() { sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e 's/[[:space:]][[:space:]]*/ /g' -e 's/ *\([<>]=\|[<>=]\) */ \1 /' -e '/^$/d'; }
requires=$(sed -n 's/^Requires:[[:space:]]*//p' "$metadata" | tr ',' '\n' | normalize)
expected='libpkgcheck >= 0.3.0
libpkgcheck < 1.0.0
libpkgexec >= 2.1.1
libpkgexec < 3.0.0'
for requirement in 'libpkgcheck >= 0.3.0' 'libpkgcheck < 1.0.0' 'libpkgexec >= 2.1.1' 'libpkgexec < 3.0.0'; do
  count=$(printf '%s\n' "$requires" | grep -Fxc "$requirement" || true)
  [ "$count" -eq 1 ] || fail "metadata contains $count copies of '$requirement', expected exactly one"
done
[ "$(printf '%s\n' "$requires" | LC_ALL=C sort)" = "$(printf '%s\n' "$expected" | LC_ALL=C sort)" ] || fail 'public requirements are not the exact check-exec dependency intervals'
private=$(sed -n 's/^Requires\.private:[[:space:]]*//p' "$metadata" | tr ',' '\n' | normalize)
[ "$private" = libcrypto ] || fail "private requirements are '$private', expected libcrypto"
printf ' %s \n' "$(sed -n 's/^Libs:[[:space:]]*//p' "$metadata")" | grep -F ' -lpkgcheck-exec ' >/dev/null || fail 'metadata omits -lpkgcheck-exec'
