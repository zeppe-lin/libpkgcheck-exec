# Design

## Authority chain

A session is admitted from one `pkgcheck::check_request`, the exact source tree
for its source snapshot, the exact built-package tree for its successful build
artifact, every exact check-input tree, one root view, one temporary tree, and
explicit interpreter and credential policy.

Check inputs are an unordered caller-supplied set. Admission matches each
concrete tree by `resolved_package_input_identity`, verifies its exact
`input_tree_identity`, rejects missing and duplicate authorities, and retains
the set in the canonical order already sealed by `libpkgcheck`. Caller order
never changes the prepared execution request.

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
check-input trees are read-only. Only the private temporary tree is writable.
The checked package tree is the working directory.

The logical layout is fixed:

```
/check/source        read-only source tree
/check/package       read-only checked package tree and working directory
/check/inputs/<id>   one read-only exact check-input tree
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

This library does not discover or extract trees, mutate package state, choose
ready transaction units, retry checks, interpret test frameworks, or publish
progression evidence.
