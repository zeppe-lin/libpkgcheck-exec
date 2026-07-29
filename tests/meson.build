# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

executor_test = executable(
  'executor-test',
  'executor_test.cpp',
  dependencies: libpkgcheck_exec_dep
)
test('check execution authority', executor_test)

session_test = executable(
  'session-test',
  'session_test.cpp',
  dependencies: libpkgcheck_exec_dep
)
test('check session admission', session_test)

headers_test = executable(
  'public-headers-test',
  'public_headers.cpp',
  dependencies: libpkgcheck_exec_dep
)
test('public headers', headers_test)

test('authority contract',
  find_program('check_authority_contract.sh'),
  args:[meson.project_source_root(), meson.project_version()])

test('release metadata',
  find_program('check_release_metadata.sh'),
  args:[meson.project_source_root(), meson.project_version()])
