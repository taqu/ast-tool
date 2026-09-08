# Phase 7d.2 — Controlled and Full Evaluation Report

## Decision

**ACCEPT WITH CAVEATS**

The fresh controlled comparison supports `ACCEPT PHASE 7D`: correctness matched Phase 5 at 17/18, routing remained semantic, recovery improved, and Phase 7d used 15 fewer tools, 5 fewer AST calls, 4 fewer AST failures, 3 fewer retries, 6 fewer reads, 5,443 fewer tokens, and 113.03 fewer seconds. No help was used by either arm.

The subsequent normal 41-task run matched Phase 5 correctness at 37/41 and used 13,869 fewer tokens than Phase 7c, but it did not preserve Phase 5 aggregate efficiency: 558 tools, 14 AST failures, 30 Greps, 29 Globs, recovery maximum 6, and 164,247 tokens. Most cost regression is concentrated in Skill-invocation mismatch and no-Skill trajectories, so it does not overturn the controlled body result, but it prevents an unconditional acceptance.

## 1. Setup and Environment

Controlled cohort: `level1-001`, `level1-002`, `level1-005`, `level1-006`, `level2-001`, `level2-004`, `level2-005`, `level2-006`, `level2-008`, `level3-002`, `level3-003`, `level3-004`, `level3-005`, `level3-007`, `level3-008`, `level4-003`, `level4-006`, and `smoke-001`.

- One fresh run per task and version; 36 controlled runs.
- `semantic-analysis` was invoked exactly once and first in all controlled runs.
- Actual arm order was Phase 7d followed by fresh Phase 5. Task-level interleaving was not achieved; this is a temporal-bias limitation.
- Claude Code: 2.1.113.
- Repository/AST Tool revision: `39ce7c7c12c46abb78f3c320a5a92b544ace6412`.
- Phase 5 SHA-256: `155B0D154E1DC9FEF7E3193A01277D98B317C86EA37C13F53D20EA219661BF90`.
- Phase 7d SHA-256: `96B07A6B89AE338F26D45FBFB31DD97D5B9C50EFA39922103E8CB3E616807EAF`.
- Harness SHA-256: `8E7B5DBD67B7287E100D2C281E99A698FFB7F37CC9184428D1CD4424DCCD7131`.
- Model identifier was unavailable. The AST Tool CLI has no working `--version`; repository revision records its implementation.

Phase 7d was verified as exact Phase 5 plus this one sentence:

```text
If a refined `search` already identifies the exact symbol or member needed,
do not add a redundant `find` solely to locate it; use `find` when AST
structure or node detail is required.
```

No Skill, tool, task, fixture, validator, metrics, or harness behavior was modified during evaluation.

## 2. Fresh 18-Task Controlled Aggregate

| Metric | Phase 5 | Phase 7d | Delta | Assessment |
|---|---:|---:|---:|---|
| Success | 17/18 | 17/18 | 0 | Preserved; same `level4-006` failure |
| Tools | 170 | 155 | -15 | Improvement |
| AST calls | 58 | 53 | -5 | Improvement |
| AST failures | 8 | 4 | -4 | Improvement |
| Failure rate | 13.79% | 7.55% | -6.24 pp | Improvement |
| Retries | 7 | 4 | -3 | Improvement |
| Help | 0 | 0 | 0 | Preserved |
| Search | 25 | 26 | +1 | Equivalent refinement |
| Callers | 18 | 14 | -4 | Fewer redundant/recovery calls |
| References | 5 | 5 | 0 | Preserved |
| Callees | 3 | 3 | 0 | Preserved |
| Find | 7 | 5 | -2 | Intended direction |
| Symbols/other | 0 | 0 | 0 | — |
| Grep | 0 | 2 | +2 | Localized; no broad fallback chain |
| Glob | 4 | 5 | +1 | Small regression |
| Read | 42 | 36 | -6 | Improvement |
| Bash | 66 | 56 | -10 | Improvement |
| Edit | 39 | 38 | -1 | Equivalent |
| Tokens | 58,968 | 53,525 | -5,443 (-9.2%) | Improvement |
| Elapsed | 1,035.94 s | 922.91 s | -113.03 s (-10.9%) | Improvement |
| Recovery mean/max | 1.67 / 4 | 1.00 / 1 | -0.67 / -3 | Improvement |

## 3. Per-Test Paired Trajectories

| Task | Phase 5 AST route | Phase 7d AST route | Δtools | ΔAST | Δfail | Δtokens | Assessment |
|---|---|---|---:|---:|---:|---:|---|
| `level1-001` | `search×2` | `search×2` | 0 | 0 | 0 | +212 | Equivalent |
| `level1-002` | `search→callers→references` | same | 0 | 0 | 0 | +1,160 | Possible regression; extra Grep, generation cost |
| `level1-005` | `search→callers` | same | +1 | 0 | 0 | +94 | Benign difference |
| `level1-006` | `search×2→find` | `search×2` | -5 | -1 | 0 | -887 | Clear improvement; redundant find removed |
| `level2-001` | `search→find` | same | 0 | 0 | 0 | -407 | Equivalent / cheaper |
| `level2-004` | `search→callers×2→references` | `search→callers→references` | -1 | -1 | 0 | -2,774 | Clear improvement |
| `level2-005` | `search→callees→find→search→find` | `search→callees×2→find` | 0 | -1 | 0 | -721 | Possible improvement; one find removed |
| `level2-006` | `search→callers` | same | 0 | 0 | 0 | -1,791 | Equivalent route; token variance favorable |
| `level2-008` | `search→callers` | `callers→search→callers` | +1 | +1 | +1 | +207 | Possible regression; immediate recovery |
| `level3-002` | `callers×2→search→callers` | `callers→search→callers` | -1 | -1 | -1 | +167 | Clear recovery improvement |
| `level3-003` | `search→find` | same | 0 | 0 | 0 | +734 | Equivalent route, token variance |
| `level3-004` | `search→callees→search→callees→search` | `search→callees→search` | -2 | -2 | 0 | -809 | Clear improvement |
| `level3-005` | `search→callers×2→search→callers×2` | `search→callers` | -7 | -4 | -3 | -1,613 | Clear improvement |
| `level3-007` | `search` | `search×6` | +5 | +5 | 0 | +4,356 | Clear stochastic regression; Phase 5 chose cheap route |
| `level3-008` | `search×2→callers×2` | `search→callers×2` | -1 | -1 | 0 | +463 | Benign semantic simplification |
| `level4-003` | `search×2→callers×2` | same | +1 | 0 | 0 | -502 | Benign difference; one Glob |
| `level4-006` | `search→callers→references` | same | -3 | 0 | 0 | -441 | Same failure, cheaper supporting work |
| `smoke-001` | `find×2→references×2` | same | -3 | 0 | -1 | -2,891 | Clear recovery/context improvement |

No task changed from a direct semantic route to a broad Grep/Glob→multi-Read manual chain. The two added Greps occurred with preserved semantic routes.

## 4. Refined Search / `find` Substitution

Two Phase 5 find calls disappeared:

- `level1-006`: `search×2→find` became `search×2`; category A/B, a targeted exact-search result removed a redundant find. It saved one AST call, five total tools, two reads, one Glob, 887 tokens, and preserved correctness.
- `level2-005`: two finds became one after a shorter semantic route; category A/B. AST calls dropped by one and tokens by 721, with no extra reads. One Glob was added, so this is a possible rather than unequivocal improvement.

No structurally necessary `find` was replaced by manual inspection.

## 5. Positive Control — `level3-007`

In this fresh pair Phase 5 spontaneously chose the cheapest route (`search` only), while Phase 7d used `search×6`. Phase 7d added five tools/AST calls and 4,356 tokens, though correctness and zero failures were preserved.

Classification: **no deterministic version effect in this pair**. Across Phase 7d.1 repeats Phase 7d still had a directional mean advantage, but Phase 5 frequently found the same cheap route. This confirms high task variance and prevents attributing every refined-search saving to the added sentence.

## 6. Relationship Guards

- `level2-006`: identity-first `search→callers` under both versions.
- `level2-008`: Phase 5 identity-first; Phase 7d relationship-first then immediate `search→callers` recovery. This is one localized regression, consistent with known stochastic variants.
- `level3-008`: both identity-first; Phase 7d used one fewer search/AST/tool.

There is no repeated directional relationship-first pattern across the three tasks.

## 7. `smoke-001`, Help, and Recovery

Both versions used `find→corrected find→references→references`. Phase 7d used no help, no unchanged retry, one fewer failure, three fewer tools, two fewer reads, and 2,891 fewer tokens. Controlled Phase 7d recovery distances were all 1; Phase 5 distances included 2 and 4 elsewhere in the cohort.

No Phase 7d help call occurred. Retry behavior improved 7→4 in aggregate.

## 8. Controlled Token Distribution

| Statistic | Phase 5 | Phase 7d |
|---|---:|---:|
| Total | 58,968 | 53,525 |
| Mean | 3,276.0 | 2,973.6 |
| Median | 2,596.5 | 2,051 |
| p75 | 4,094 | 4,708 |
| p90 | 6,499 | 6,276 |
| Min | 1,114 | 1,238 |
| Max | 9,050 | 7,014 |

Paired Phase 7d−Phase 5 deltas: median -424, p75 +212, p90 +1,160, minimum -2,891, maximum +4,356. With a ±250-token tolerance: 10 improved, 4 unchanged, 4 worsened.

The top three positive contributors (`level3-007`, `level1-002`, `level3-003`) total +6,250 tokens; the top five total +6,925. They exceed the net -5,443 delta because broad improvements elsewhere offset them. This is a mixed per-task distribution, not a universal token reduction.

## 9. Rule Causality and Shared Inefficiencies

The new rule plausibly explains the missing `find` in `level1-006` and possibly `level2-005`. It cannot explain callers ordering, references ambiguity, help, or unrelated token-generation changes.

Shared Phase 9 candidates, not fixed here:

- under-qualified relationship query → search → relationship retry;
- same-FQN declaration/definition ambiguity;
- repeated relationship calls after partial results;
- search locating a declaration but requiring another query/read for definitions;
- successful find followed by Read for implementation context.

## 10. Controlled Gate

**ACCEPT PHASE 7D**

The fresh controlled result meets every gate: equal correctness, targeted routing, better recovery, meaningful redundant-work reduction, fewer tools/tokens/time, and no systematic regression. This authorized the normal 41-task run.

## 11. Normal 41-Task Evaluation

| Metric | Phase 5 | Phase 7c | Phase 7d |
|---|---:|---:|---:|
| Success | 37/41 | 37/41 | 37/41 |
| Tools | 518 | 542 | 558 |
| AST calls | 69 | 59 | 74 |
| AST failures | 9 | 7 | 14 |
| AST failure rate | 13.04% | 11.86% | 18.92% |
| Retries | 9 | 9 | 19 |
| Help | 2 | 3 | 3 |
| Search | 30 | 25 | 25 |
| Callers | 14 | 11 | 15 |
| References | 6 | 6 | 9 |
| Callees | 3 | 7 | 8 |
| Find | 12 | 4 | 9 |
| Symbols | 1 | 4 | 4 |
| Grep | 15 | 17 | 30 |
| Glob | 12 | 22 | 29 |
| Read | 252 | 286 | 263 |
| Bash | 120 | 102 | 117 |
| Edit | 86 | 82 | 85 |
| Recovery mean/max | 1.44 / 2 | 1.60 / 4 | 1.79 / 6 |
| Elapsed | 2,224.27 s | 2,214.34 s | 2,271.05 s |
| Tokens | 158,303 | 178,116 | 164,247 |
| Token median / p75 / p90 | 2,322 / 5,414 / 7,592 | 3,114 / 6,140 / 7,312 | 2,659 / 5,474 / 8,093 |
| Skill invocation | historical mixed | historical mixed | 19/41 |

All four Phase 7d failures (`level4-002`, `level4-005`, `level4-006`, `level4-007`) are the same failures seen in both Phase 5 and Phase 7c.

### Invocation cohorts versus Phase 5

| Cohort | Tasks | Δtools | ΔAST | Δfailures | ΔGrep | ΔGlob | ΔRead | Δtokens |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Skill loaded in both | 17 | +2 | +4 | +2 | +2 | +3 | -3 | +525 |
| Skill absent in both | 19 | +13 | 0 | 0 | +6 | +14 | -1 | -8,506 |
| Invocation mismatch | 5 | +25 | +1 | +3 | +7 | 0 | +15 | +13,925 |

The invocation-mismatch cohort dominates the token regression: its +13,925 tokens exceed the total +5,944 because no-Skill improvements offset it. The largest outlier, `level4-003` (+7,002), loaded Phase 5 but not Phase 7d and added 12 tools, six Greps, and ten Reads. This is not evidence against the Phase 7d body.

`smoke-001` is the main loaded-body outlier (+5,578 tokens, +8 tools, +6 AST calls, +3 failures), showing that normal-run recovery remains stochastic even after favorable repeated guards.

## 12. Final Recommendation

**ACCEPT WITH CAVEATS**

Accept Phase 7d as the stronger Skill-body candidate: the fresh controlled comparison is decisively favorable and the one-sentence change is low-risk and causally useful on at least one refined-search/find trajectory. Do not claim full agent-level superiority over Phase 5. Normal invocation and recovery variance still produce worse tools, AST failures, Grep/Glob, recovery, elapsed, and tokens than Phase 5.

Before replacing Phase 5 as the global stable baseline, address Skill invocation reliability or repeat the normal run. Do not add more Phase 7d wording in response to no-Skill trajectories.
