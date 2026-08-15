#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
fail() { echo "ci-contract: $*" >&2; exit 1; }
workflow=$root/.github/workflows/ci.yml
driver=$root/ci/configure-and-test.sh
qualify=$root/ci/qualify.sh
[ -s "$workflow" ] || fail 'hosted CI workflow is absent'
[ -x "$driver" ] || fail 'qualification driver is absent or not executable'
[ -x "$qualify" ] || fail 'repository qualification driver is absent or not executable'
for mode in 'GCC shared' 'GCC static' 'Clang shared' 'Clang static' 'GCC release'; do grep -F "$mode" "$workflow" >/dev/null || fail "CI omits $mode"; done
grep -F 'address,undefined' "$workflow" >/dev/null || fail 'CI omits ASan/UBSan qualification'
for pin in 'libpkgsource, ref: v4.1.0' 'libpkgcatalog, ref: v4.0.0' 'libpkgresolve, ref: v4.0.0' 'libpkgbuild, ref: v3.0.1' 'libpkgtransaction, ref: v4.0.0' 'libpkgcheck, ref: v0.3.0' 'libpkgexec, ref: v2.2.0'; do
  grep -F "$pin" "$workflow" >/dev/null || fail "CI omits authority pin $pin"
done
! grep -F 'ref: master' "$workflow" >/dev/null || fail 'CI admits floating dependency authority'
grep -F 'pkg-config --static --libs libpkgcheck-exec' "$driver" >/dev/null || fail 'static installed consumer does not use pkg-config --static'
grep -F 'tests/installed/consumer.cpp' "$driver" >/dev/null || fail 'installed consumer is not executed'
for var in LIBPKGCHECK_SOURCE LIBPKGEXEC_SOURCE; do grep -F "$var" "$driver" >/dev/null || fail "$var is absent from isolated closure driver"; done
for script in check_authority_contract.sh check_codec_contract.sh check_meson_sources.sh check_release_metadata.sh check_test_layout.sh check_abi_contract.sh check_ci_contract.sh; do
  grep -F "tests/contracts/$script" "$qualify" >/dev/null || fail "repository qualification omits $script"
done
! grep -F 'tests/check_' "$qualify" >/dev/null || fail 'repository qualification still names pre-role-split contract paths'
