# Phase 7d — Selective Backport of Phase 7c Improvements

## Decision

**REVISE**

The single backport reproducibly removes redundant `find` work on the intended tasks, but the targeted candidate did not preserve Phase 5's guard metrics. Both Phase 7d targeted runs passed 7/7 and used no help or grep/glob fallback, yet produced 5 and 6 AST failures versus Phase 5's 3 on the same tasks. Per the staged gate, the 18-task and 41-task Phase 7d evaluations were not run.

The candidate remains exact Phase 5 plus one narrow rule. It should not replace Phase 5 until the relationship-order and `smoke-001` recovery variance is resolved.

## 1. Phase 7c Improvement Inventory

Materially cheaper controlled tasks:

| Task | Phase 5 route | Phase 7c route | Δtools | Δtokens | Δtime | Candidate rule |
|---|---|---|---:|---:|---:|---|
| `level3-007` | `search×3 → find×6` | `search×4` | -4 | -2,494 | -21.42 s | Stop after refined search identifies the exact semantic targets |
| `level1-002` | `search → callers → references`, Read×3 | same semantic route, Read×2 | -1 | -1,024 | -3.80 s | None; unchanged route, likely sampling/context variance |

Supporting, but not aggregate-cost improvements:

| Task | Phase 5 route | Phase 7c route | Effect | Classification |
|---|---|---|---|---|
| `level1-001` | `search → find` | `search → refined search` | Targeted and correct, but +1 tool and +811 tokens | Supporting evidence for boundary only |
| `level3-003` | `search → find` | `search → refined search` | Targeted and correct, tools equal, +150 tokens | Supporting evidence for boundary only |

Small token-only changes without a supported trajectory cause were not treated as improvements.

## 2. Backport Classification

| Candidate | Decision | Evidence |
|---|---|---|
| Refined search may end exact symbol/member location without redundant `find` | **BACKPORT** | `level3-007` saved 5 AST calls, 4 tools, 2,494 tokens, and 21.42 s; correctness and reads were preserved |
| Compressed Phase 7c routing table/sections | **DO NOT BACKPORT** | Controlled Phase 7c increased tools 162→173 and AST failures 5→8 |
| Relationship command before identity discovery | **DO NOT BACKPORT** | `level2-006`, `level2-008`, and `level3-008` showed under-qualified failure then search/retry |
| Phase 7c recovery/help wording | **DO NOT BACKPORT** | `smoke-001` reached recovery distance 4 and Phase 7c used two help calls versus zero |
| Phase 7c output/compression wording | **INCONCLUSIVE** | No isolated positive causal trajectory |
| `level1-002` reduced Read count | **INCONCLUSIVE** | Same AST route; no changed decision boundary explains the saving |

## 3. Phase 5 → Phase 7d Diff

Phase 7d starts from the exact Phase 5 file at `e561505`. One sentence was added after Phase 5's `search`/`find` boundary:

```diff
 Use `search` for cross-workspace symbol lookup; use `find` for targeted node lookup within a specific file.
+If a refined `search` already identifies the exact symbol or member needed, do not add a redundant `find` solely to locate it; use `find` when AST structure or node detail is required.
```

No routing, identity-first, recovery, help, ambiguity, semantic/manual, output, trigger, or frontmatter text was otherwise changed.

## 4. Evidence for the Change

The sentence preserves the original boundary:

```text
exact semantic identity/location → refined search can be sufficient
AST structure or node detail     → find
```

It does not authorize search to replace structural lookup, nor does it relax search-before-relationship behavior.

The backport reproduced the intended behavior twice:

- `level3-007` Phase 7d run 1: `search×3`, 11 tools, 0 failures, 7,932 tokens, 57.49 s.
- `level3-007` Phase 7d run 2: `search×4`, 11 tools, 0 failures, 4,935 tokens, 56.47 s.
- Phase 5: `search×3 → find×6`, 15 tools, 0 failures, 8,434 tokens, 75.35 s.
- Phase 7c: `search×4`, 11 tools, 0 failures, 5,940 tokens, 53.93 s.

Both Phase 7d runs removed all six Phase 5 `find` calls, preserved correctness and targeted search, and saved four tools. The token saving reproduced in run 2; run 1 still saved 502 tokens against Phase 5.

`level3-003` also refined successfully: Phase 7d used a single `search` in both targeted runs, versus Phase 5 `search → find`. `level1-001` retained `search → find`, demonstrating that the rule did not prohibit structural lookup.

## 5. Skill Size

| Version | Lines | Characters | UTF-8 bytes | Approx. tokens |
|---|---:|---:|---:|---:|
| Phase 5 | 355 | 13,053 | 13,111 | 3,263 |
| Phase 7d | 356 | 13,238 | 13,296 | 3,310 |
| Delta | +1 | +185 | +185 | +47 |

Size is informational; no Phase 5 text was deleted to offset the rule.

## 6. Targeted Controlled Replay

Tasks: `level1-001`, `level3-003`, `level3-007`, plus guards `level2-006`, `level2-008`, `level3-008`, and `smoke-001`. Skill invocation was forced first exactly as in the controlled Phase 5/Phase 7c evaluation. Phase 7d was repeated once because the first guard result was mixed.

| Metric | Phase 5 | Phase 7c | Phase 7d R1 | Phase 7d R2 |
|---|---:|---:|---:|---:|
| Success | 7/7 | 7/7 | 7/7 | 7/7 |
| Tools | 59 | 64 | 59 | 61 |
| AST calls | 23 | 26 | 21 | 22 |
| AST failures | 3 | 7 | 5 | 6 |
| Retries | 2 | 7 | 6 | 4 |
| Help | 0 | 2 | 0 | 0 |
| Search | 8 | 12 | 9 | 10 |
| Find | 10 | 2 | 4 | 3 |
| Callers | 4 | 7 | 6 | 6 |
| References | 1 | 3 | 2 | 3 |
| Grep | 0 | 0 | 0 | 0 |
| Glob | 1 | 1 | 0 | 0 |
| Read | 13 | 14 | 16 | 13 |
| Tokens | 23,041 | 23,084 | 25,629 | 22,027 |
| Median tokens | 2,929 | 3,323 | 4,469 | 3,338 |
| p75 | 4,720 | 5,433 | 4,710 | 4,935 |
| p90 | 8,434 | 5,940 | 7,932 | 5,339 |
| Elapsed | 332.61 s | 362.68 s | 325.78 s | 359.88 s |

Phase 7d improves redundant structural work and never calls help, but it does not meet the guard target `AST failures <= Phase 5`.

## 7. Guard Trajectories

| Task | Phase 5 | Phase 7d observation | Classification |
|---|---|---|---|
| `level2-006` | `search → callers`, 0 failures | Same route in both Phase 7d runs | **BENIGN / PRESERVED** |
| `level2-008` | `search → callers`, 0 failures | `callers → search → callers`, 1 failure in both runs | **REGRESSION**, but unrelated to added find-only rule |
| `level3-008` | callers failure → search → callers | R1 added a callers attempt; R2 used search refinement then callers | **INCONCLUSIVE** |
| `smoke-001` | `find → corrected find → references`, 2 failures | R1 had 3 failures; R2 had 5; neither used help | **REGRESSION** in failures, Phase 5 help behavior preserved |

The relationship and references regressions cannot be causally derived from the new `find` exception. Their variation between repeated runs suggests stochastic adherence, but the phase gate is empirical: the candidate still failed to demonstrate Phase 5-level recovery stability.

## 8. Per-Test Outcome Classification

- **IMPROVEMENT:** `level3-007`, `level3-003` — redundant `find` eliminated through targeted semantic search.
- **BENIGN DIFFERENCE:** `level1-001`, `level2-006` — Phase 5 route retained.
- **REGRESSION:** `level2-008`, `smoke-001` — repeatable extra failures or longer recovery.
- **INCONCLUSIVE:** `level3-008` — trajectory changed substantially between Phase 7d replicates.

No targeted task replaced AST work with grep, glob, or broad manual exploration.

## 9. Stage 2 and Stage 3

The full 18-task controlled cohort was **not run** because the targeted gate was not favorable: Phase 7d exceeded Phase 5 AST failures in both targeted attempts. The full 41-task evaluation was therefore also **not run**, as required by the staged strategy.

Existing Phase 5 and Phase 7c full-run results remain comparison context, not Phase 7d verification:

| Metric | Phase 5 | Phase 7c |
|---|---:|---:|
| Success | 37/41 | 37/41 |
| Tools | 518 | 542 |
| AST calls/failures | 69 / 9 | 59 / 7 |
| Glob / Read | 12 / 252 | 22 / 286 |
| Recovery mean/max | 1.44 / 2 | 1.60 / 4 |
| Tokens | 158,303 | 178,116 |

## 10. Shared Inefficiencies for Phase 9

Recorded only; not fixed here:

- Under-qualified relationship target → search refinement → relationship retry.
- Same-FQN C++ declaration/definition ambiguity during `references`.
- `find` success followed by Read when structural output lacks implementation context.
- Search results identifying declarations but requiring a second query to locate definitions.

The traces do not yet prove that a new AST command is needed; inconsistent agent ordering and same-FQN disambiguation should be studied first.

## Final Recommendation

**REVISE**

Keep the Phase 7d one-sentence candidate for further controlled refinement because its intended efficiency effect reproduced. Do not promote it over Phase 5 yet. The next experiment should hold the candidate fixed and repeat the guard cohort—especially `level2-008` and `smoke-001`—or add a narrowly explicit statement that the refined-search exception does not relax identity-first relationship workflows, then rerun the targeted gate.
