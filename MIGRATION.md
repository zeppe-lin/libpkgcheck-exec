# Migration

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
