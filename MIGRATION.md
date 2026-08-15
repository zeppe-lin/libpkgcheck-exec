# Migration

## 0.8.0

The package under test is now a distinct singleton execution subject.
`PKG_PACKAGE_ROOT` moves from `/check/inputs/_package` to `/check/package`, and
`/check/inputs` contains check dependencies only. Callers must provide the exact
`/check/package` structural destination in the root view. No `_package`
alias is retained.

This changes semantic execution-request identity but not public C++ carrier
layout; the SONAME remains 2. Existing private 0.7 execution evidence requires
its exact old request authority.

## 0.7.0

Check-input execution is now recipe-addressable by canonical package name.
`PKG_CHECK_INPUT_ROOT=/check/inputs` and `PKG_CHECK_INPUTS` are exported; each
check dependency is mounted at `$PKG_CHECK_INPUT_ROOT/<package-name>`. The
checked package moves from `/check/inputs/package` to the reserved
`/check/inputs/_package` child so a real package named `package` cannot collide
with a dependency mount.

This changes semantic execution-request identity but not the public C++ carrier
layout, so the SONAME remains 2. Existing private execution evidence from 0.6
is not reconstructed through compatibility aliases; reopen requires the exact
old request authority.

## 0.6.0

Check programs now use the common recipe execution environment:

- `PKG_SOURCE_ROOT` replaces `ZEPPE_LIN_CHECK_SOURCE`;
- `PKG_PACKAGE_ROOT` replaces `ZEPPE_LIN_CHECK_ROOT`;
- `ZEPPE_LIN_CHECK_PACKAGE` is removed.

No compatibility aliases are exported. The source root still names the exact
admitted read-only source resource; the package root names the sealed checked
package resource and working directory. This is an execution-policy change, not
a public C++ carrier change, so the SONAME remains 2.

## 0.4.0

The published `libpkgcheck 0.2.0` authority is `libpkgcheck.so.1`. Its public
`check_request` and `check_result` carrier layouts differ from the generation
used by the released `libpkgcheck-exec.so.0` line. `libpkgcheck-exec 0.4.0`
therefore publishes `libpkgcheck-exec.so.1`. Rebuild consumers; do not mix
objects compiled against the old carrier layout with the new provider.

The project version remains 0.4.0 because this generation had not been tagged.
No compatibility adapter is provided for the old check ABI.

The execution dependency is now `libpkgexec >= 2.0.0, < 3.0.0`. The exec1 to
exec2 transition preserves every execution carrier retained by this adapter, so
`libpkgcheck-exec.so.1` remains the truthful ABI generation. Durable subordinate
execution evidence remains libpkgexec execution-result encoding version 1.

## 0.3.0

`package_input_tree` is replaced by `package_input_resource`.

Callers no longer provide `resolved_package_input_identity` or
`input_tree_identity`. For each logical input retained by
`pkgcheck::check_request::inputs()`, provide:

- its exact `pkgbuild::build_input_identity`;
- one distinct `pkgexec::resource_identity` for the concrete execution
  resource;
- the absolute call-scoped host path at which that resource is available.

The adapter matches resources by logical input identity and binds the concrete
resource into `libpkgexec`. It does not claim to independently verify a package
filesystem-tree digest.

The `libpkgcheck` dependency is now `>= 0.2.0, < 1.0.0`. The shared-library
SONAME remains 0.

## 0.2.0

Durable consumers may encode a `check_execution_result` and later reopen it
only when they also retain the exact `pkgcheck::check_request`,
`pkgexec::execution_request`, and `pkgexec::backend_capability_profile` bodies.
Identity strings alone are not accepted as semantic authority.

The `libpkgexec` dependency floor is 1.4.0. The shared-library SONAME remains
0.

## 0.1.1

Callers may supply check-input resources in any order; admission retains them
in the sealed request order. Concrete source, checked-package, check-input, and
temporary host paths must be pairwise disjoint after lexical normalization.
Resource identities must also be distinct.

## 0.1.0

Initial native check-execution boundary.
