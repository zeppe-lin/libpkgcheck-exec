# Design

## Authority chain

A session is admitted from one `pkgcheck::check_request`, the exact source tree
for its source snapshot, the exact built-package tree for its successful build
artifact, every exact check-input tree, one root view, one temporary tree, and
explicit interpreter and credential policy.

`prepare()` projects those authorities into one
`pkgexec::execution_request` whose purpose is `check`. Source, package and
check-input trees are read-only. Only the private temporary tree is writable.
The checked package tree is the working directory.

`execute()` calls exactly one backend. A successful execution becomes a passed
check result. Cancellation maps to `cancelled`; a failure before process start
maps to `execution-unavailable`; every other terminal process failure maps to
`program-failed`. Backend exceptions and evidence for another request are
contract violations.

## Exclusions

This library does not discover or extract trees, mutate package state, choose
ready transaction units, retry checks, interpret test frameworks, or publish
progression evidence.
