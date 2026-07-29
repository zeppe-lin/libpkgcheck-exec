# Testing

The suite separates session-admission and execution-projection contracts.

Session tests prove:

- unordered multi-input sets canonicalize to sealed request order;
- exact logical `/check/inputs/<identity>` bindings and host materializations;
- missing, duplicate, unrelated, and forged input authority rejection;
- source and artifact authority rejection;
- canonical supplementary groups and duplicate-group rejection;
- absolute, non-root, mutually disjoint concrete paths;
- concrete resource-identity alias rejection.

Executor tests prove:

- exact check-purpose request translation and working-directory selection;
- read-only source/package/input resources and writable private temporary root;
- host coordinates do not contaminate semantic execution-request identity;
- passed, execution-unavailable, and program-failed result mapping;
- backend exception, request-evidence, and capability-profile contract checks.

Shared and static builds must be tested separately. Release qualification also
requires strict GCC and Clang builds, ASan/UBSan execution, public-header
independence, SONAME and exported-symbol comparison, `DT_NEEDED` inspection,
and independent patch replay.
