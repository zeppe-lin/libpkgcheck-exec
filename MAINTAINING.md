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

`admitted_check_session`, `prepared_execution`, and `check_execution_result`
retain `libpkgexec` values by value. Any `libpkgexec` ABI-generation change
therefore requires explicit carrier-layout and provider-edge review before the
dependency interval is widened. The exec1 -> exec2 transition preserved every retained execution carrier layout.
The check1 -> check2 transition does not preserve the retained check authority
generation, so `libpkgcheck-exec 0.5.0` publishes `libpkgcheck-exec.so.2`.

Release qualification must execute the installed product, not merely the
source tree. The shared product must match `abi/libpkgcheck-exec.exports`
exactly and name the reviewed `libpkgcheck` and `libpkgexec` provider
generations. Static qualification must use `pkg-config --static` so private
crypto and thread closure is exercised.

The check-program environment is part of execution policy. Keep
`PKG_SOURCE_ROOT` and `PKG_PACKAGE_ROOT` semantic meanings stable across build
and check integration. Check dependencies must remain recipe-addressable by
canonical package name through `PKG_CHECK_INPUT_ROOT` and `PKG_CHECK_INPUTS`;
do not expose opaque `build_input_identity` text as a recipe pathname contract.
Do not add distribution-branded aliases or infer an unpacked workspace from the
admitted source resource. The environment contract must pin these projections
and documentation claims before release.
