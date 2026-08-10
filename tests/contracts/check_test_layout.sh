#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?}
meson=$root/tests/meson.build
for directory in contracts fixtures header installed integration protocol support unit; do
  test -d "$root/tests/$directory" || {
    echo "missing test role directory: $directory" >&2
    exit 1
  }
done
for suite in unit integration protocol header contract; do
  grep -F "suite: '$suite'" "$meson" >/dev/null || {
    echo "missing Meson test suite: $suite" >&2
    exit 1
  }
done
for stale in executor_test.cpp session_test.cpp public_headers.cpp check_fixture.h fixture.h fixture_transaction.h session_fixture.h test.h; do
  test ! -e "$root/tests/$stale" || {
    echo "flat test artifact remains: $stale" >&2
    exit 1
  }
done
grep -F "'integration/session_admission_test.cpp'" "$meson" >/dev/null
grep -F "'integration/request_projection_test.cpp'" "$meson" >/dev/null
grep -F "'integration/preparation_test.cpp'" "$meson" >/dev/null
grep -F "'integration/backend_contract_test.cpp'" "$meson" >/dev/null
grep -F "'integration/execution_failure_test.cpp'" "$meson" >/dev/null
grep -F "'integration/execution_success_test.cpp'" "$meson" >/dev/null
grep -F "'integration/result_binding_test.cpp'" "$meson" >/dev/null
grep -F "'protocol/result_codec_roundtrip_test.cpp'" "$meson" >/dev/null
grep -F "'protocol/result_codec_refusal_test.cpp'" "$meson" >/dev/null
grep -F "test('header-' + header.underscorify()" "$meson" >/dev/null
! grep -F "test('header:" "$meson" >/dev/null

grep -F "'pkgconfig-metadata'" "$meson" >/dev/null
grep -F "'abi-layout'" "$meson" >/dev/null
grep -F "'abi-surface'" "$meson" >/dev/null
grep -F "'dependency-abi'" "$meson" >/dev/null
grep -F "'abi-contract': 'contracts/check_abi_contract.sh'" "$meson" >/dev/null
grep -F "'ci-contract': 'contracts/check_ci_contract.sh'" "$meson" >/dev/null
