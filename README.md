# libpkgcheck-exec

`libpkgcheck-exec` is the backend-neutral realization adapter for one sealed
`libpkgcheck` request. It admits exact concrete trees, translates the retained
source program into `libpkgexec`, calls one execution backend, and maps returned
execution evidence into terminal check evidence.

The library does not compose transactions, build packages, unpack artifacts,
select interpreters, implement a process backend, or advance controller state.
