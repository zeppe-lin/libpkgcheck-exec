# History

Version: 0.2.0
Date: 2026-08-02

- Establish a canonical versioned codec for complete check-execution evidence.
- Embed libpkgexec 1.4 execution records without duplicating execution schema.
- Retain exact terminal outcome, failure classification, and evidence identities.
- Require exact check request, execution request, and backend authorities at decode.
- Reject corruption, noncanonical bytes, and substituted semantic authorities.
- Keep the libpkgcheck-exec SONAME at 0 and raise libpkgexec to 1.4.0.

## 0.1.1 — 2026-07-30

Hardened concrete session admission. Multi-input trees are matched by exact
authority and canonicalized into sealed request order. Concrete resource paths
and identities cannot alias, credentials are canonicalized before projection,
and backend evidence must match the advertised capability profile.

## 0.1.0 — 2026-07-29

Initial transaction-bound check execution authority.
