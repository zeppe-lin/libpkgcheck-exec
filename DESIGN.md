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

`prepare()` projects admitted authorities into one
`pkgexec::execution_request` whose purpose is `check`. Source, package, and
check-input resources are read-only. Only the private temporary tree is
writable. The checked package tree is the working directory.

The logical layout is fixed:

```
/check/source        read-only source resource
/check/package       read-only checked package resource and working directory
/check/inputs/<id>   one read-only logical check-input resource
/tmp                 writable private temporary tree
```

Host paths remain call-scoped operational coordinates. Changing only those
coordinates changes `execution_resources`, not the semantic execution-request
identity.

## Backend evidence

`execute()` snapshots the backend capability profile, calls exactly one
backend, and accepts only evidence for both the exact prepared request and the
same advertised backend profile. Standard and non-standard backend exceptions
are converted to `backend-contract-violation`.

A successful execution becomes a passed check result. Cancellation maps to
`cancelled`; a failure before process start maps to
`execution-unavailable`; every other terminal process failure maps to
`program-failed`.

## Exclusions

This library does not discover, acquire, extract, or materialize resources;
mutate package state; choose ready transaction units; retry checks; interpret
test frameworks; or publish progression evidence.

## Durable check-execution evidence

The durable record belongs to this adapter because it binds exact
`libpkgexec` process evidence to the corresponding terminal `libpkgcheck`
result. The record embeds the canonical `libpkgexec 1.4` execution-result
encoding and adds only adapter-owned outcome, execution-evidence,
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
execution-unavailable, program-failed, and cancelled results remain distinct
owner evidence shapes.

The codec performs no session admission, resource discovery, backend
invocation, program execution, retry, progression publication, filesystem
access, or mutation.
