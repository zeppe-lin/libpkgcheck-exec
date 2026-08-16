# History

Version: 0.9.0
Date: 2026-08-16

- Consume the exact environment policy retained by the checked build result.
  Check execution no longer hard-codes parallelism 1, umask 0022, or an absent
  source-date epoch; it projects admitted parallelism as `PKG_JOBS` and carries
  the build policy's mask and epoch into the sealed execution request.
- Keep the public C++ ABI and `libpkgcheck-exec.so.2` generation unchanged.

Version: 0.8.0
Date: 2026-08-15

- Project the checked package as the singleton `pkgexec::package_tree` subject
  instead of misclassifying it as a named build input.
- Move `PKG_PACKAGE_ROOT` to `/check/package`; keep `/check/inputs/<name>`
  exclusively for recipe-addressable check dependencies.
- Require `libpkgexec >= 2.2.0, < 3.0.0`; keep exact check-input admission by
  `build_input_identity` and keep `libpkgcheck-exec.so.2` unchanged.
- Remove the `_package` namespace workaround rather than retaining an alias.

Version: 0.7.0
Date: 2026-08-15

- Mount logical check inputs by canonical package name instead of opaque
  `build_input_identity` text so recipe check programs can consume them.
- Export `PKG_CHECK_INPUT_ROOT=/check/inputs` and deterministic
  `PKG_CHECK_INPUTS`; reserve `/check/inputs/_package` for the checked package.
- Keep exact logical-input admission by `build_input_identity`; only the
  recipe-facing logical coordinate changes.
- Keep the public C++ ABI and `libpkgcheck-exec.so.2` generation unchanged.

Version: 0.6.0
Date: 2026-08-15

- Normalize the check recipe environment to the common `PKG_*` execution ABI.
- Export `PKG_SOURCE_ROOT=/check/source` for the exact admitted read-only source
  resource and `PKG_PACKAGE_ROOT=/check/inputs/package` for the sealed checked
  package resource.
- Remove the branded `ZEPPE_LIN_CHECK_*` variables instead of retaining aliases.
- Keep the public C++ ABI and `libpkgcheck-exec.so.2` generation unchanged.

Version: 0.5.0
Date: 2026-08-14

- Rebuild the public execution carrier as `libpkgcheck-exec.so.2` for
  `libpkgcheck 0.3.0` / `libpkgcheck.so.2`.
- Preserve check-execution semantics and durable codec generation; this release
  changes the retained subordinate check ABI authority, not the outer protocol.
- Keep the reviewed `libpkgexec.so.2` execution provider generation unchanged.
- Require `libpkgexec >= 2.1.1, < 3.0.0`, excluding the tagged
  execution-2 build that still admitted source ABI 3.

Version: 0.4.0
Date: 2026-08-10

- Add a pure canonical execution-request projection for admitted check sessions.
- Keep call-scoped host materialization binding in the existing prepare path.
- Let restart composition reproduce request authority without filesystem effects.
- Rebuild the public ABI as `libpkgcheck-exec.so.1` for the published
  `libpkgcheck 0.2.0` / `libpkgcheck.so.1` authority.
- Freeze an exact reviewed ELF surface and anchor public exception RTTI.
- Qualify exact check/exec provider generations, installed shared/static
  consumption, GCC/Clang builds, and ASan/UBSan execution.
- Bind execution authority to `libpkgexec 2.x` / `libpkgexec.so.2` after
  proving all retained execution carriers preserve the 0.4 ABI layouts.
- Separate unit, integration, protocol, header, and contract qualification.
- Pin request-scoped cancellation, backend-contract, and resource-admission
  semantics independently.

Version: 0.3.0
Date: 2026-08-05

- Admit concrete check resources by exact logical `build_input_identity`.
- Remove the unissued `input_tree_identity` and materialized-package-input ABI.
- Keep package-input paths and resource identities call-scoped to execution.
- Require libpkgcheck 0.2 and its resolver-backed logical check-input model.
- Preserve canonical request order, resource isolation, and durable evidence.
- Keep the libpkgcheck-exec SONAME at 0.

## 0.2.0 — 2026-08-02

- Establish a canonical versioned codec for complete check-execution evidence.
- Embed libpkgexec 1.4 execution records without duplicating execution schema.
- Retain exact terminal outcome, failure classification, and evidence identities.
- Require exact check request, execution request, and backend authorities at decode.
- Reject corruption, noncanonical bytes, and substituted semantic authorities.
- Keep the libpkgcheck-exec SONAME at 0 and raise libpkgexec to 1.4.0.

## 0.1.1 — 2026-07-30

Hardened concrete session admission. Multi-input resources are matched by exact
authority and canonicalized into sealed request order. Concrete resource paths
and identities cannot alias, credentials are canonicalized before projection,
and backend evidence must match the advertised capability profile.

## 0.1.0 — 2026-07-29

Initial transaction-bound check execution authority.
