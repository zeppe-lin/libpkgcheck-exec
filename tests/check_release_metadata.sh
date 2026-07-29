#!/bin/sh
set -eu
root=${1:?}
grep -q "version: '0.1.0'" "$root/meson.build"
grep -q "soversion: '0'" "$root/src/meson.build"
grep -q "libpkgcheck >= 0.1.0" "$root/src/meson.build"
grep -q "libpkgexec >= 1.3.0" "$root/src/meson.build"
