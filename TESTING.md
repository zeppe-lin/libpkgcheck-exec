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
role-separated test layout, exact ABI surface, carrier layouts, generated
pkg-config requirements, hosted-CI geometry, and provider generations. Generated header test names use Meson-safe
`header-...` names rather than deprecated colon-bearing names.

Shared and static builds are tested separately. Hosted release qualification
runs GCC and Clang debug shared/static products, a GCC release product, and GCC
and Clang ASan+UBSan shared products against one isolated current authority
prefix. Each product runs the categorized native suite and then installs.

The installed consumer uses only installed headers and pkg-config metadata. It
constructs source/catalog/state authority, resolves and composes a transaction,
creates build/check authority, admits a concrete check session, executes a
public fake backend, round-trips durable evidence, and catches a
`pkgcheck_exec::error` thrown inside the shared library. Static qualification
uses `pkg-config --static`.

On x86-64, the ABI-layout contract pins the foreign `libpkgcheck` and
`libpkgexec` carriers plus the adapter values that retain them. The exec2 bridge
keeps those layouts byte-for-byte stable while shared builds require a direct
`libpkgexec.so.2` edge and reject exec0/exec1 providers. Shared builds also add
exact export-manifest equality. The final
release gate also includes public-header independence, `git diff --check`,
`git fsck`, and independent mailbox replay.
