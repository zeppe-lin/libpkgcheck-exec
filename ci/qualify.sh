#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sh -n "$root"/tests/*.sh
"$root/tests/check_authority_contract.sh" "$root"
"$root/tests/check_release_metadata.sh" "$root"
git -C "$root" diff --check
git -C "$root" fsck --no-dangling
