#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
for script in "$root"/tests/contracts/*.sh "$root"/ci/*.sh "$root"/tools/*.sh; do
  sh -n "$script"
done
"$root/tests/contracts/check_authority_contract.sh" "$root"
"$root/tests/contracts/check_codec_contract.sh" "$root"
"$root/tests/contracts/check_meson_sources.sh" "$root"
"$root/tests/contracts/check_release_metadata.sh" "$root"
"$root/tests/contracts/check_test_layout.sh" "$root"
"$root/tests/contracts/check_abi_contract.sh" "$root"
"$root/tests/contracts/check_ci_contract.sh" "$root"
git -C "$root" diff --check
git -C "$root" fsck --no-dangling
