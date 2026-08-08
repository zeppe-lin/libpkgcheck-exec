# Testing

The suite is organized by evidence role rather than by historical implementation
file. `unit` covers adapter-owned values, `integration` covers the real
`libpkgcheck` -> `libpkgcheck-exec` -> `libpkgexec` semantic seam, `protocol`
covers durable check-execution bytes, `header` compiles every public header
independently, and `contract` checks static repository and boundary invariants.

## Unit

Unit qualification pins the complete adapter error vocabulary.

## Integration

Session-admission tests prove unordered multi-input canonicalization, exact
logical input authority, source/artifact binding, canonical credentials,
resource-limit retention, absolute/disjoint host paths, and distinct concrete
resource identities.

Request-projection tests prove exact check purpose, program/interpreter/root
authority, read-only source/package/input bindings, the writable private
temporary root, closed I/O, denied networking, exact credentials and limits,
and that the current adapter deliberately seals cancellation as disabled. Host
materialization paths do not contaminate semantic request identity. An
adapter-owned temporary-resource collision is refused by the pure projection
before preparation.

Preparation tests prove that effect-free request sealing and call-scoped
materialization binding agree exactly. Backend-contract tests separately attack
standard and non-standard exceptions from capability observation and execution,
foreign request evidence, and foreign backend-profile evidence.

Execution tests distinguish passed checks, failures before start, non-zero
program exit, and other started process failures. Current ordinary check
execution does not claim a reachable cancelled outcome because its request has
cancellation disabled. Result-binding tests pin the exact execution/check
identity relationship.

## Protocol

The canonical codec round-trips passed, unavailable, non-zero-exit, and other
started-failure evidence through the real subordinate `libpkgexec` codec and
requires byte-for-byte canonical re-encoding. Refusal tests cover checksum
corruption, truncation, trailing bytes, and substitution of the check request,
execution request, or backend profile.

## Header and contract qualification

Every installed public header is compiled independently. Static contracts pin
the authority boundary, codec exclusions, release metadata, Meson source set,
and role-separated test layout. Generated header test names use Meson-safe
`header-...` names rather than deprecated colon-bearing names.

Shared and static builds must be tested separately. Release qualification also
requires strict GCC and Clang builds, ASan/UBSan execution, public-header
independence, SONAME and exported-symbol comparison, `DT_NEEDED` inspection,
and independent patch replay.
