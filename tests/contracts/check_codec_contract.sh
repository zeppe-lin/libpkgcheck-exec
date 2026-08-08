#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?}
api=$root/include/libpkgcheck-exec/result_codec.h
codec=$root/src/result_codec.cpp
model=$root/include/libpkgcheck-exec/model.h
meson=$root/src/meson.build

test -f "$api"
test -f "$codec"
grep -Fq 'encode_check_execution_result' "$api"
grep -Fq 'decode_check_execution_result' "$api"
grep -Fq 'pkgexec::decode_execution_result' "$codec"
grep -Fq 'expected_check_result' "$codec"
grep -Fq 'class codec_access' "$model"
grep -Fq 'maximum_check_execution_result_encoding_size' "$api"
grep -Fq 'check_execution_result_encoding_version = 1' "$api"
grep -Fq 'checksum mismatch' "$codec"
grep -Fq 'authority_mismatch' "$codec"
grep -Fq 'is not canonical' "$codec"
grep -Fq 'libpkgexec >= 1.4.0' "$meson"
! grep -Eq 'admitted_check_session|execution_backend|execution_resources|filesystem::path' "$api"
! grep -Eq 'prepare\(|execute\(' "$codec"
