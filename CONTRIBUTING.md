# Contributing

Preserve the authority boundary. New code must not perform transaction
composition, artifact extraction, package installation, state publication, or
process syscalls. Every concrete path must remain call-scoped and outside
semantic identities.
