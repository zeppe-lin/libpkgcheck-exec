# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

scdoc = find_program('scdoc', required: get_option('man_pages'))
if scdoc.found()
  custom_target(
    'libpkgcheck-exec.7',
    input: 'libpkgcheck-exec.7.scdoc',
    output: 'libpkgcheck-exec.7',
    command: [scdoc],
    feed: true,
    capture: true,
    install: true,
    install_dir: get_option('mandir') / 'man7'
  )
endif
