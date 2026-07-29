# libpkgcheck-exec

`libpkgcheck-exec` is the backend-neutral realization adapter for one sealed
`libpkgcheck` request. It admits exact concrete trees, translates the retained
source program into `libpkgexec`, calls one execution backend, and maps returned
execution evidence into terminal check evidence.

Session admission is set-based. Concrete check-input trees may arrive in any
order, but every exact input authority must appear once and is retained in the
canonical order of the sealed check request. Source, checked-package, input,
and temporary host paths must be absolute, non-root, and mutually disjoint.
Distinct semantic resources must not alias one concrete resource identity.
Credential groups are normalized before execution-request construction.

The library does not compose transactions, build packages, unpack artifacts,
select interpreters, implement a process backend, or advance controller state.
