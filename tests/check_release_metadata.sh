#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu
root=${1:?}
version=${2:?}

grep -Fq "version: '$version'" "$root/meson.build"
grep -Fq "soversion: '0'" "$root/src/meson.build"
grep -Fq "libpkgcheck >= 0.1.0" "$root/src/meson.build"
grep -Fq "libpkgexec >= 1.3.0" "$root/src/meson.build"
grep -Fq "args:[meson.project_source_root(), meson.project_version()]" \
  "$root/tests/meson.build"
