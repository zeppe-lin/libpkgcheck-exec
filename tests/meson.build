# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

unit_tests = {
  'error-value': 'unit/error_value_test.cpp',
}

integration_tests = {
  'session-admission': 'integration/session_admission_test.cpp',
  'request-projection': 'integration/request_projection_test.cpp',
  'preparation': 'integration/preparation_test.cpp',
  'backend-contract': 'integration/backend_contract_test.cpp',
  'execution-failure': 'integration/execution_failure_test.cpp',
  'execution-success': 'integration/execution_success_test.cpp',
  'result-binding': 'integration/result_binding_test.cpp',
}

protocol_tests = {
  'result-codec-roundtrip': 'protocol/result_codec_roundtrip_test.cpp',
  'result-codec-refusal': 'protocol/result_codec_refusal_test.cpp',
}

foreach name, source : unit_tests
  executable_test = executable(
    name + '-test',
    source,
    dependencies: libpkgcheck_exec_dep,
  )
  test(name, executable_test, suite: 'unit')
endforeach

foreach name, source : integration_tests
  executable_test = executable(
    name + '-test',
    source,
    dependencies: libpkgcheck_exec_dep,
  )
  test(name, executable_test, suite: 'integration')
endforeach

foreach name, source : protocol_tests
  executable_test = executable(
    name + '-test',
    source,
    dependencies: libpkgcheck_exec_dep,
  )
  test(name, executable_test, suite: 'protocol')
endforeach

public_headers = [
  'error.h',
  'model.h',
  'executor.h',
  'result_codec.h',
  'libpkgcheck-exec.h',
]
foreach header : public_headers
  header_test = executable(
    'header-' + header.underscorify() + '-test',
    'header/public_header_test.cpp',
    cpp_args: [
      '-DPKGCHECK_EXEC_TEST_HEADER=<libpkgcheck-exec/' + header + '>',
    ],
    dependencies: libpkgcheck_exec_dep,
  )
  test('header-' + header.underscorify(), header_test, suite: 'header')
endforeach

contract_tests = {
  'authority-contract': 'contracts/check_authority_contract.sh',
  'codec-contract': 'contracts/check_codec_contract.sh',
  'release-metadata': 'contracts/check_release_metadata.sh',
  'meson-sources': 'contracts/check_meson_sources.sh',
  'test-layout': 'contracts/check_test_layout.sh',
}
foreach name, program : contract_tests
  test(
    name,
    find_program(program),
    args: [meson.project_source_root(), meson.project_version()],
    suite: 'contract',
  )
endforeach
