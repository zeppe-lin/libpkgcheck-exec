#!/bin/sh
set -eu
root=${1:?}
grep -q 'execution_purpose::check()' "$root/src/executor.cpp"
grep -q 'check_input_tree' "$root/src/executor.cpp"
! grep -R 'fork\|execve\|waitpid' "$root/src" >/dev/null
