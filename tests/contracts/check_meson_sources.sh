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
  src/error.cpp src/model.cpp src/result_identity.cpp src/result_codec.cpp src/executor.cpp \
  tests/unit/error_value_test.cpp \
  tests/integration/session_admission_test.cpp \
  tests/integration/request_projection_test.cpp \
  tests/integration/preparation_test.cpp \
  tests/integration/backend_contract_test.cpp \
  tests/integration/execution_failure_test.cpp \
  tests/integration/execution_success_test.cpp \
  tests/integration/result_binding_test.cpp \
  tests/protocol/result_codec_roundtrip_test.cpp \
  tests/protocol/result_codec_refusal_test.cpp \
  tests/header/public_header_test.cpp \
  tests/contracts/check_authority_contract.sh \
  tests/contracts/check_codec_contract.sh \
  tests/contracts/check_release_metadata.sh \
  tests/contracts/check_meson_sources.sh \
  tests/contracts/check_test_layout.sh \
  man/libpkgcheck-exec.7.scdoc man/pkgcheck_exec_result_codec.3.scdoc
do
  test -f "$root/$path" || { echo "missing Meson/test input: $path" >&2; exit 1; }
done
