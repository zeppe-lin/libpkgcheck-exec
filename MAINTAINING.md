# Maintaining

Treat session admission as a security and reproducibility boundary. A caller
may supply vectors in arbitrary order, but stored values must be canonical and
must preserve the exact lower authority order. Never compare independently
canonicalized sets positionally unless their ordering contracts are identical.

Before release, run shared and static Meson matrices, strict GCC and Clang
builds, ASan/UBSan, contract scripts, `git diff --check`, and `git fsck`.
Inspect the shared object's SONAME, exported symbols, and `DT_NEEDED` closure.
Compare public value layouts and symbol sets with the preceding release. ABI
changes require an explicit SONAME decision.

`admitted_check_session` retains `pkgcheck::check_request` by value and
`check_execution_result` retains `pkgcheck::check_result` by value. Any
`libpkgcheck` ABI-generation change therefore requires a fresh layout and
SONAME review here before widening or reusing the dependency interval. Equal
outer size is not sufficient evidence of ABI compatibility.

Release qualification must execute the installed product, not merely the
source tree. The shared product must match `abi/libpkgcheck-exec.exports`
exactly and name the reviewed `libpkgcheck` and `libpkgexec` provider
generations. Static qualification must use `pkg-config --static` so private
crypto and thread closure is exercised.
