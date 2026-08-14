# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

public_inc = include_directories('../include')
installed_public_headers = files(
  '../include/libpkgcheck-exec/error.h',
  '../include/libpkgcheck-exec/model.h',
  '../include/libpkgcheck-exec/executor.h',
  '../include/libpkgcheck-exec/result_codec.h',
  '../include/libpkgcheck-exec/libpkgcheck-exec.h',
)

elf_export_generator = find_program(
  meson.project_source_root() / 'tools/generate-elf-export-script.sh',
)
elf_export_script_name = 'libpkgcheck-exec.exports.map'
elf_export_script_path = meson.current_build_dir() / elf_export_script_name
elf_export_script = configure_file(
  input: '../abi/libpkgcheck-exec.exports',
  output: elf_export_script_name,
  command: [elf_export_generator, '@INPUT@'],
  capture: true,
)

public_deps = [libpkgcheck_dep, libpkgexec_dep]

libpkgcheck_exec = library(
  'pkgcheck-exec',
  ['error.cpp', 'model.cpp', 'result_identity.cpp', 'result_codec.cpp', 'executor.cpp'],
  include_directories: public_inc,
  dependencies: public_deps + [libcrypto_dep],
  link_args: ['-Wl,--version-script=' + elf_export_script_path],
  link_depends: [elf_export_script],
  install: true,
  version: meson.project_version(),
  soversion: '2',
)

libpkgcheck_exec_consumer_deps = public_deps
if want_static
  libpkgcheck_exec_consumer_deps += [libcrypto_dep]
endif

libpkgcheck_exec_dep = declare_dependency(
  link_with: libpkgcheck_exec,
  include_directories: public_inc,
  dependencies: libpkgcheck_exec_consumer_deps,
  version: meson.project_version(),
)

meson.override_dependency('libpkgcheck-exec', libpkgcheck_exec_dep)

install_headers(
  installed_public_headers,
  subdir: 'libpkgcheck-exec',
)

pkgconf.generate(
  libpkgcheck_exec,
  name: 'libpkgcheck-exec',
  filebase: 'libpkgcheck-exec',
  description: 'Native check execution adapter',
  subdirs: '.',
  requires: public_deps,
  requires_private: [libcrypto_dep],
)
