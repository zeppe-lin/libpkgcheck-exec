# Migration

## 0.2.0

The execution and session APIs are unchanged. Durable consumers may encode a
`check_execution_result` and later reopen it only when they also retain the
exact `pkgcheck::check_request`, `pkgexec::execution_request`, and
`pkgexec::backend_capability_profile` bodies. Identity strings alone are not
accepted as semantic authority.

The `libpkgexec` dependency floor is now 1.4.0. The shared-library SONAME
remains 0.

## 0.1.1

No API or ABI migration is required. Callers may continue supplying check-input
trees in any order; 0.1.1 now admits that set correctly and retains it in the
sealed request order.

Concrete source, checked-package, check-input, and temporary host paths must be
pairwise disjoint after lexical normalization. Resource identities for source,
checked-package, and check-input trees must also be distinct. Callers that used
aliased paths or identities must provide separate materializations.

## 0.1.0

This was the first native release. There is no legacy check-execution protocol.
Callers must provide exact already-materialized trees; paths are operational
coordinates, not source or artifact authority.
