# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

executor_test = executable(
  'executor-test',
  'executor_test.cpp',
  dependencies: libpkgcheck_exec_dep
)
test('check execution authority', executor_test)
test(
  'durable check evidence codec',
  executor_test,
  args: ['--codec'],
)

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

test(
  'codec contract',
  find_program('check_codec_contract.sh'),
  args: [meson.project_source_root()],
)

test(
  'Meson source paths',
  find_program('check_meson_sources.sh'),
  args: [meson.project_source_root()],
)
