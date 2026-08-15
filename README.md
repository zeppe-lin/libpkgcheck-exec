# libpkgcheck-exec

`libpkgcheck-exec` is the backend-neutral realization adapter for one sealed
`libpkgcheck` request. It admits exact call-scoped source, checked-package, and
logical check-input resources, translates the retained source program into
`libpkgexec`, calls one execution backend, and maps returned execution evidence
into terminal check evidence.

Session admission is set-based. Concrete check-input resources may arrive in
any order, but every exact `pkgbuild::build_input_identity` retained by the
sealed check request must appear once. The adapter stores resources in request
order. The caller supplies a distinct `pkgexec::resource_identity` and host
path for each logical input; the adapter does not claim that those paths were
independently hashed into package-tree authority.

Source, checked-package, input, and temporary host paths must be absolute,
non-root, and mutually disjoint. Distinct semantic resources must not alias one
concrete resource identity. Credential groups are normalized before execution request construction.
`seal_execution_request()` reproduces that request without touching host
resources; `prepare()` adds only the call-scoped materialization bindings.

Check programs receive the common `PKG_*` recipe environment.
`PKG_SOURCE_ROOT=/check/source` names the exact admitted source resource and
`PKG_PACKAGE_ROOT=/check/package` names the sealed checked package subject.
`PKG_CHECK_INPUT_ROOT=/check/inputs` names the check-input namespace, whose
children use canonical package names, and `PKG_CHECK_INPUTS` lists those names
in sealed request order. No distribution-branded check aliases are exported.

The library does not compose transactions, build packages, acquire or unpack
artifacts, materialize package inputs, select interpreters, implement a process
backend, or advance controller state.

## Durable evidence

`libpkgcheck-exec` provides a versioned canonical codec for
`check_execution_result`. It embeds `libpkgexec`'s execution-result record and
retains the adapter-owned terminal check evidence. Decode requires the exact
check request, execution request, and backend profile bodies; it never
reconstructs those authorities from identity strings.
