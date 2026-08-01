#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu
root=${1:?}

test -f "$root/include/libpkgcheck-exec/result_codec.h"
test -f "$root/src/result_codec.cpp"
test -f "$root/src/result_identity.cpp"
test -f "$root/tests/executor_test.cpp"

grep -Fq 'encode_check_execution_result' \
  "$root/include/libpkgcheck-exec/result_codec.h"
grep -Fq 'decode_check_execution_result' \
  "$root/include/libpkgcheck-exec/result_codec.h"
grep -Fq 'pkgexec::decode_execution_result' "$root/src/result_codec.cpp"
grep -Fq 'expected_check_result' "$root/src/result_codec.cpp"
grep -Fq "'executor_test.cpp'" "$root/tests/meson.build"
grep -Fq "args: ['--codec']" "$root/tests/meson.build"
grep -Fq 'libpkgexec >= 1.4.0' "$root/src/meson.build"
