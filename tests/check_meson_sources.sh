#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu
root=${1:?}

for path in \
  include/libpkgcheck-exec/error.h \
  include/libpkgcheck-exec/model.h \
  include/libpkgcheck-exec/executor.h \
  include/libpkgcheck-exec/result_codec.h \
  include/libpkgcheck-exec/libpkgcheck-exec.h \
  src/error.cpp \
  src/model.cpp \
  src/result_identity.cpp \
  src/result_codec.cpp \
  src/executor.cpp \
  tests/executor_test.cpp \
  tests/session_test.cpp \
  tests/public_headers.cpp \
  tests/check_authority_contract.sh \
  tests/check_codec_contract.sh \
  tests/check_meson_sources.sh \
  tests/check_release_metadata.sh \
  man/libpkgcheck-exec.7.scdoc \
  man/pkgcheck_exec_result_codec.3.scdoc
do
  test -f "$root/$path" || {
    echo "missing Meson input: $path" >&2
    exit 1
  }
done
