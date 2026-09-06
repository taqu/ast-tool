# Phase 8b raw measurement tables

Phase 7f is the one-run historical semantic baseline per task. Phase 8b is five
final-binary controlled semantic runs per task. Deltas are Phase 8b mean minus
Phase 7f mean.

| Task | Before | After | Raw success | Tools Δ | AST Δ | References Δ | Reads Δ | Tokens Δ | Elapsed Δ |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| level2-004 | 1 | 5 | 5/5 | -2.00 | -1.00 | -1.00 | 0.00 | -732.20 | -15.29 s |
| level4-006 | 1 | 5 | 0/5* | +1.20 | -0.40 | -0.80 | +1.00 | +547.00 | +2.35 s |
| Equal-task mean | 2 | 10 | 5/10* | -0.40 | -0.70 | -0.90 | +0.50 | -92.60 | -6.47 s |

`*` Every `level4-006` run made exactly the three intended edits. Its committed
`ApiHandler` declaration lacks the `processor_` and `registry_` members used by
the unchanged implementation, so the validator's whole-project compile step
fails independently of the task edits. The Phase 7f run failed for the same
fixture.

| Task | Repeat | Tools | AST | Search | Callers | References | Reads | Edits | Tokens | Elapsed | Trajectory |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| level2-004 | 1 | 10 | 2 | 1 | 1 | 0 | 3 | 4 | 6,133 | 41.93 | search→callers |
| level2-004 | 2 | 10 | 2 | 1 | 1 | 0 | 3 | 4 | 5,086 | 44.51 | search→callers |
| level2-004 | 3 | 10 | 2 | 1 | 1 | 0 | 3 | 4 | 5,476 | 45.48 | search→callers |
| level2-004 | 4 | 10 | 2 | 1 | 1 | 0 | 3 | 4 | 5,089 | 46.59 | search→callers |
| level2-004 | 5 | 10 | 2 | 1 | 1 | 0 | 3 | 4 | 5,935 | 47.39 | search→callers |
| level4-006 | 1 | 10 | 3 | 2 | 1 | 0 | 3 | 3 | 2,575 | 61.00 | search→callers→search |
| level4-006 | 2 | 11 | 3 | 1 | 1 | 1 | 3 | 3 | 3,175 | 58.24 | search→callers→references |
| level4-006 | 3 | 10 | 3 | 2 | 1 | 0 | 3 | 3 | 4,551 | 47.87 | search→callers→search |
| level4-006 | 4 | 10 | 2 | 1 | 1 | 0 | 3 | 3 | 4,259 | 48.19 | search→callers |
| level4-006 | 5 | 10 | 2 | 1 | 1 | 0 | 3 | 3 | 2,960 | 55.54 | search→callers |

All final runs invoked the Skill first. AST failures, retries, help, Find, and
Grep were zero, so recovery mean/max are not applicable.

| Relationship | Before | After | Expected | Missing | Unexpected | Stable |
| --- | --- | --- | ---: | ---: | ---: | --- |
| callers `auth::AuthToken::validate` | empty | four callers | 4 | 0 | 0 | 5/5 |
| references `auth::AuthToken::validate` | empty | four sites | 4 | 0 | 0 | 5/5 |
| callers `service::ValidationService::validate` | empty | one caller | 1 | 0 | 0 | 5/5 |
| references `service::ValidationService::validate` | empty | one site | 1 | 0 | 0 | 5/5 |

The fixture matrix has 16 command cases. Every expected fragment is present,
and no forbidden fragment appears. Six existing direct relationship queries
were also replayed five times each with stable exact answer cardinality.
