# History

Version: 0.4.0
Date: 2026-08-07

- Add a pure canonical execution-request projection for admitted check sessions.
- Keep call-scoped host materialization binding in the existing prepare path.
- Let restart composition reproduce request authority without filesystem effects.
- Preserve the shared-library SONAME at 0; this is an additive API release.
- Separate unit, integration, protocol, header, and contract qualification.
- Pin request-scoped cancellation, backend-contract, and resource-admission semantics independently.

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
