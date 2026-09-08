# Phase 8a raw measurement tables

Phase 7f is the before condition. Phase 8a is five controlled semantic runs per
task. Deltas are Phase 8a mean minus Phase 7f semantic mean.

| Task | Before runs | After runs | Success delta | Tool delta | Traced AST delta | Failure delta | Retry delta | Search delta | Caller delta | Read delta | Token delta | Elapsed delta (s) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| level2-008 | 6 | 5 | 0.00 | -0.77 | -0.77 | -0.33 | -0.33 | -0.43 | -0.33 | 0.00 | -42.57 | -4.59 |
| level3-008 | 6 | 5 | 0.00 | -0.40 | +0.07 | -0.33 | -0.33 | -0.20 | +0.27 | +0.23 | -445.00 | +0.99 |
| Equal-task aggregate | 12 | 10 | 0.00 | -0.58 | -0.35 | -0.33 | -0.33 | -0.32 | -0.03 | +0.12 | -243.78 | -1.80 |

“Traced AST” counts tool events containing AST Tool, matching the established
evaluation metric. Some Bash events contain two AST executable invocations; the
per-run executable count is included separately below.

| Task | Repeat | Tools | Traced AST | AST executable invocations | Failures | Retries | Searches | Caller events | Reads | Tokens | Elapsed (s) | Trajectory |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| level2-008 | 1 | 4 | 1 | 1 | 0 | 0 | 0 | 1 | 1 | 1,229 | 32.32 | callers |
| level2-008 | 2 | 5 | 2 | 2 | 0 | 0 | 1 | 1 | 1 | 1,309 | 38.07 | search→callers |
| level2-008 | 3 | 4 | 1 | 1 | 0 | 0 | 0 | 1 | 1 | 1,192 | 29.12 | callers |
| level2-008 | 4 | 5 | 2 | 2 | 0 | 0 | 1 | 1 | 1 | 1,328 | 39.29 | search→callers |
| level2-008 | 5 | 4 | 1 | 1 | 0 | 0 | 0 | 1 | 1 | 1,240 | 31.19 | callers |
| level3-008 | 1 | 7 | 2 | 3 | 0 | 0 | 1 | 1 | 2 | 2,385 | 52.07 | search→callers |
| level3-008 | 2 | 8 | 2 | 3 | 0 | 0 | 0 | 2 | 2 | 2,912 | 49.36 | callers→callers |
| level3-008 | 3 | 9 | 3 | 4 | 0 | 0 | 2 | 1 | 2 | 3,707 | 56.88 | search→search→callers |
| level3-008 | 4 | 8 | 2 | 3 | 0 | 0 | 0 | 2 | 2 | 3,261 | 54.03 | callers→callers |
| level3-008 | 5 | 11 | 3 | 4 | 0 | 0 | 1 | 2 | 4 | 3,820 | 59.45 | search→callers→callers |

| Replay target | Historical occurrences | Before | After | Answers equal to exact FQN | After failures |
| --- | ---: | --- | --- | --- | ---: |
| AuthToken::expire | 2 | callers→search→callers | callers | yes, two callers | 0/5 |
| DataStore::save | 2 | callers→search→callers | callers | yes, one caller | 0/5 |

Each partial and exact replay was executed five times. All returned code zero and
all partial outputs exactly equaled their fully qualified counterparts.
