# Design

## Authority chain

A session is admitted from one `pkgcheck::check_request`, the exact source
resource for its source snapshot, the exact checked-package resource for its
successful build artifact, every exact logical check input, one root view, one
temporary tree, and explicit interpreter and credential policy.

Check inputs are an unordered caller-supplied set. Admission matches each
concrete resource by `pkgbuild::build_input_identity`, rejects missing,
duplicate, and unrelated authorities, and retains the set in the canonical
order already sealed by `libpkgcheck`. Caller order never changes the prepared
execution request.

A `package_input_resource` contains three deliberately separate facts:

- the logical build-input identity owned by `libpkgbuild`;
- the concrete semantic resource identity owned by `libpkgexec`;
- the call-scoped host path at which that resource is available.

The adapter does not issue or verify a content-addressed package-tree identity.
No production owner for such an authority exists in the current stack.

Concrete source, package, input, and temporary paths are normalized before they
are retained. They must be absolute, must not name the host filesystem root,
and must be pairwise disjoint, including ancestor/descendant overlaps. The root
view path is a separate namespace coordinate and may be `/`. Distinct source,
package, and input resources must carry distinct concrete resource identities.

Supplementary groups are sorted and validated during session admission. The
primary group cannot reappear as a supplementary group. The admitted session
therefore retains the same canonical credential set later sealed by
`libpkgexec`.

## Execution projection

`seal_execution_request()` purely projects admitted authorities into one
`pkgexec::execution_request`; it performs no filesystem access or mutation.
`prepare()` calls that projection and separately binds the call-scoped host
materializations. This lets restart recovery reproduce exact request authority
without pretending that path realization is semantic reconstruction.

The canonical request has
`pkgexec::execution_request` whose purpose is `check`. Source, package, and
check-input resources are read-only. Only the private temporary tree is
writable. The checked package tree is the working directory.

The logical layout is fixed:

```
/check/source           read-only source resource
/check/package         read-only checked package resource and working directory
/check/inputs/<name>    one read-only logical check-input resource
/tmp                    writable private temporary tree
```

The check-program environment consumes the exact environment policy already
sealed in `session.request().build().request().policy()`. The adapter does not
invent a second check-only parallelism, umask, or source-date epoch. It names
that admitted policy and the check authorities without phase-specific or
distribution-branded aliases:

```
PKG_SOURCE_ROOT=/check/source
PKG_PACKAGE_ROOT=/check/package
PKG_CHECK_INPUT_ROOT=/check/inputs
PKG_CHECK_INPUTS=<colon-separated canonical package names>
PKG_JOBS=<admitted build-policy parallelism>
```

`PKG_SOURCE_ROOT` is the exact caller-admitted source resource. This adapter
does not reinterpret it as an unpacked workspace. `PKG_PACKAGE_ROOT` is the
sealed checked-package resource and working directory. Check dependencies are
recipe-facing inputs, so their logical mount names use the canonical package
names already retained by each `pkgbuild::build_input`; recipes never need to
derive or embed opaque `build_input_identity` values. `PKG_CHECK_INPUTS` follows
the canonical input order already sealed by `libpkgcheck`.

The checked package is the singleton `package_tree` subject of the check. It is
not a member of the named check-input namespace. The caller-owned root view
therefore supplies the exact `/check/package` structural destination, while a
process backend owns only the package-name children beneath the otherwise empty
`/check/inputs` dependency namespace.

Host paths remain call-scoped operational coordinates. Changing only those
coordinates changes `execution_resources`, not the semantic execution-request
identity.

## Backend evidence

`execute()` snapshots the backend capability profile, calls exactly one
backend, and accepts only evidence for both the exact prepared request and the
same advertised backend profile. Standard and non-standard backend exceptions
are converted to `backend-contract-violation`.

A successful execution becomes a passed check result. The current check
projection deliberately disables request cancellation, so ordinary adapter
execution can produce `passed`, `execution-unavailable`, or `program-failed`
check evidence. The shared classifier retains the `cancelled` vocabulary for a
request that carries valid cancellation authority, but `seal_execution_request()`
does not create such a request in this release. A failure before process start
maps to `execution-unavailable`; every other reachable terminal process failure
maps to `program-failed`.

## Exclusions

This library does not discover, acquire, extract, or materialize resources;
mutate package state; choose ready transaction units; retry checks; interpret
test frameworks; or publish progression evidence.

## Durable check-execution evidence

The durable record belongs to this adapter because it binds exact
`libpkgexec` process evidence to the corresponding terminal `libpkgcheck`
result. The record embeds canonical `libpkgexec` execution-result encoding
version 1 and adds only adapter-owned outcome, execution-evidence,
failure-classification, failure-evidence, and check-result identities.

The record does not serialize a check request, execution request, backend
profile, admitted session, concrete resource, host path, credential policy, or
execution materialization. Decode requires the exact
`pkgcheck::check_request`, `pkgexec::execution_request`, and
`pkgexec::backend_capability_profile` bodies from their owning authorities.
Identity strings alone are not rehydration authority.

Decode verifies the whole-record checksum, delegates subordinate process
evidence to `libpkgexec`, reconstructs terminal check evidence through the
public `pkgcheck::check_result` factories, verifies every retained identity,
and requires canonical byte-for-byte re-encoding. Passed,
execution-unavailable, program-failed, and cancelled remain distinct owner
vocabulary; records produced by the current adapter use the first three because
its sealed execution request has cancellation disabled.

The codec performs no session admission, resource discovery, backend
invocation, program execution, retry, progression publication, filesystem
access, or mutation.
