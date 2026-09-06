# Phase 7f raw measurement tables

All deltas are semantic-route mean minus non-semantic-route mean. Times are
seconds. These are same-task descriptive comparisons; routing was not randomized.

| Task | Semantic runs | Manual runs | Success delta | Tool delta | Token delta | Elapsed delta | Read delta | Grep delta | AST calls delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| level1-006 | 1 | 1 | 0.00 | +1.00 | +501.00 | +16.88 | 0.00 | -1.00 | +2.00 |
| level2-001 | 1 | 1 | 0.00 | +1.00 | +669.00 | +22.56 | 0.00 | -2.00 | +2.00 |
| level2-004 | 1 | 1 | 0.00 | +3.00 | +3,028.00 | +30.39 | 0.00 | -2.00 | +3.00 |
| level2-005 | 1 | 1 | 0.00 | +4.00 | +316.00 | +37.98 | +1.00 | -3.00 | +4.00 |
| level2-006 | 6 | 1 | 0.00 | +2.00 | -1,240.33 | +19.56 | 0.00 | -1.00 | +2.00 |
| level2-008 | 6 | 1 | 0.00 | +0.17 | -302.83 | +17.70 | -1.00 | -2.00 | +2.17 |
| level3-005 | 1 | 1 | 0.00 | 0.00 | +77.00 | +16.38 | 0.00 | -3.00 | +2.00 |
| level3-007 | 7 | 5 | 0.00 | +0.17 | +3,128.63 | +16.25 | -3.91 | -0.40 | +5.43 |
| level3-008 | 6 | 1 | 0.00 | -15.00 | -3,526.00 | -10.66 | -17.83 | 0.00 | +2.33 |
| level4-003 | 1 | 6 | 0.00 | -8.50 | -6,578.00 | +22.95 | -8.67 | -4.00 | +4.00 |
| level4-006 | 1 | 1 | 0.00 | -6.00 | +48.00 | -15.02 | -5.00 | -2.00 | +3.00 |
| smoke-001 | 1 | 6 | 0.00 | -6.50 | -3,288.50 | -51.76 | -1.83 | 0.00 | -4.83 |

| Command | Calls | Effective successes | Mean time | Total time | Output characters | A | B | C | D | E | F |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| search | 56 | 55 | 10.057 | 563.183 | 26,751 | 27 | 28 | 0 | 0 | 0 | 1 |
| find | 33 | 25 | 5.945 | 196.179 | 16,968 | 3 | 0 | 0 | 0 | 16 | 14 |
| callers | 27 | 23 | 5.446 | 147.046 | 7,371 | 20 | 0 | 3 | 0 | 0 | 4 |
| callees | 2 | 2 | 4.416 | 8.832 | 108 | 0 | 0 | 2 | 0 | 0 | 0 |
| references | 30 | 11 | 4.320 | 129.603 | 15,995 | 3 | 0 | 2 | 0 | 0 | 25 |
| symbols | 1 | 1 | 3.907 | 3.907 | 14 | 1 | 0 | 0 | 0 | 0 | 0 |
| top_level (obsolete smoke form) | 4 | 4 | 4.277 | 17.107 | 4,540 | 0 | 0 | 0 | 0 | 0 | 4 |
| 2 (malformed smoke token) | 1 | 1 | 6.863 | 6.863 | 632 | 0 | 0 | 0 | 0 | 0 | 1 |

Call classifications total 154: A 54, B 28, C 7, D 0, E 16, F 49.
Within semantic routes the totals are A 51, B 28, C 7, E 8, F 7. Direct-AST
smoke runs account for A 3, E 8, and F 42.

Search output role evidence: 39 declaration-only calls, 5 containing both a
declaration and definition, 3 definition-only, and 9 with no role evidence.
There were four `search -> find` sequences and 16 context-bridge `find` calls;
manual review found no clearly redundant location-only `find` remaining.
