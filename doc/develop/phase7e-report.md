# Phase 7e — Skill Invocation Reliability

## 1. Environment and revisions

The investigation ran on `features/phase7e`, revision `4927b0caa055ee26f4c1e4a663f9383c38c82627`, on Windows, September 6, 2026 JST. Fresh session logs identify Claude Code `2.1.113`, `claude-sonnet-4-6` for primary sessions, and `claude-haiku-4-5-20251001` for spawned Explore agents. Sampling settings and server-side randomness were not recorded; the historical report did not identify its model.

The installed AST executable was `D:/Programs/ast-tool/ast-tool.exe`, SHA-256 `d52918518f2b5dad59d0634a024e28bac7320880041dbfc49e65ce97c7f8029e`. This identifies the binary; its build-to-source provenance was not independently established. The unchanged Claude launcher hash is `8e7b5dbd67b7287e100d2c281e99a698ffb7f37cc9184428d1cd4424dccd7131`, matching Phase 7d.2. Tasks, fixtures, validators, launcher, and Skill files have no working-tree changes. The pre-existing edit to `ast-tool.md` was preserved.

Each run used a fresh Claude process and reset the selected evaluation repository to its committed revision. Runs were sequential, with task order rotated between rounds. No forced-invocation system prompt was used. The probe driver replaces global session-log deletion with capture of newly created project-session logs, including child-agent logs. It does not change prompts, model selection, or Skill registration. Project auto-memory directories were empty when audited. Log preservation is a harness difference from the historical run, so historical comparisons are descriptive rather than controlled causal estimates.

An initial sandbox attempt timed out without tools or a session log; the sandbox batch was interrupted. Its available record is retained separately under `evaluation/phase7e/r1/` and excluded from behavioral measurements. All measured normal runs executed outside that restriction.

## 2. Exact Phase 7d Skill verification

Repository and installed Claude copies matched SHA-256 `96b07a6b89ae338f26d45fbfb31dd97d5b9c50efa39922103e8cb3e616807eaf` before and after every measured run. The Phase 5 blob at `e561505:skills/semantic-analysis/SKILL.md` matches the accepted hash `155b0d154e1dc9fef7e3193a01277d98b317c86ea37c13f53d20ea219661bf90`. Exact text comparison confirms that Phase 7d adds only:

> If a refined `search` already identifies the exact symbol or member needed, do not add a redundant `find` solely to locate it; use `find` when AST structure or node detail is required.

No body, description, name, trigger, or registration field was changed. The historical controlled Phase 7d traces were also rechecked: all 18 invoked **semantic-analysis**, exactly once and first. The normal-run counting correction below does not invalidate that controlled result.

## 3. Probe cohort

Eight existing tasks cover normal semantic routing, historical mismatch, lookup, relationships, recovery, and a higher-level change. The same task prompts and repository revisions were retained throughout.

| Task | Selection purpose | Task category | C/C++ source files | Source bytes | Prompt words |
| --- | --- | --- | ---: | ---: | ---: |
| level1-001 | Simple edit / prior non-invocation control | multi-file | 7 | 2,329 | 58 |
| level2-006 | Historically semantic / relationship query | references | 20 | 6,714 | 52 |
| level2-008 | Ambiguous identity and relationship lookup | mixed | 16 | 8,199 | 82 |
| level3-003 | Search/find and delegation-chain lookup | multi-level-callees | 27 | 10,882 | 71 |
| level3-007 | Distributed chain / prior routing variance | distributed-workflow | 27 | 10,882 | 53 |
| level3-008 | Indirect caller relationship query | multi-level-callers | 23 | 7,703 | 82 |
| level4-003 | Historical invocation mismatch / higher-level API change | api-evolution | 20 | 11,224 | 105 |
| smoke-001 | Recovery-sensitive explicit AST-command control | unlabelled | 3 | 296 | 92 |

Source sizes are measured from fixture `.c`, `.cpp`, `.h`, and `.hpp` files, excluding generated files. These are small C++ fixtures, not evidence about large or multilingual repositories. All selected tasks passed the historical normal Phase 7d run; this cohort does not include that full suite's four persistent failures.

## 4. Repetition design and measurement definitions

The main experiment uses five normal repetitions of each of eight tasks: 40 runs. A task showing both invocation states receives five additional repetitions. Historical Phase 5 and Phase 7d observations remain separate from fresh repetitions.

Invocation means a successful `Skill` call naming `semantic-analysis`, not any `Skill` call. Other Skills and direct AST use are recorded separately. No direct alternative reads of the semantic Skill were found in the audited non-invoking traces. First action, second action, invocation position, pre-invocation exploration/failure, and displayed trajectories use tool-start timestamps. The original tracer orders log files by modification time and can list a child agent's tools before its parent `Agent` call. Original sequences are retained alongside chronological ones; retry and recovery metrics are checked under both orders.

Tokens use the established input-plus-output metric, not a monetary billing estimate; original cache fields remain in results. Named tool counts do not include shell equivalents (`Read` excludes `cat`; `Edit` excludes `Write`). Recovery distance counts tools to the next successful non-help AST call. Unrecovered failures have no distance, not zero. Means pool recovery events rather than run averages.

The measurement tests passed: **27 passed, 2 skipped**, covering existing trace metrics, exact Skill identity, parent/child chronology, chronological retries/recovery, and event-weighted recovery means. Raw evidence and reproduction instructions are in [evaluation/phase7e/README.md](evaluation/phase7e/README.md); measurements are in [tables.md](evaluation/phase7e/tables.md), [measurements.json](evaluation/phase7e/measurements.json), [summary.json](evaluation/phase7e/summary.json), and [environment.json](evaluation/phase7e/environment.json).

## 5. Raw invocation results

| Task | Runs | Invoked | Rate | Mean invocation position | Observed class |
| --- | ---: | ---: | ---: | ---: | --- |
| level1-001 | 5 | 0 | 0% | — | B: stable non-invocation |
| level2-006 | 5 | 5 | 100% | 1 | A: stable invocation |
| level2-008 | 5 | 5 | 100% | 1 | A: stable invocation |
| level3-003 | 5 | 5 | 100% | 1 | A: stable invocation |
| level3-007 | 10 | 6 | 60% | 1 | C: stochastic invocation |
| level3-008 | 5 | 5 | 100% | 1 | A: stable invocation |
| level4-003 | 5 | 0 | 0% | — | B: stable non-invocation |
| smoke-001 | 5 | 0 | 0% | — | B: stable non-invocation |

The 40-run balanced baseline invoked `semantic-analysis` in 23/40 runs. The five-run extension deliberately oversampled the mixed task; combined invocation was 26/45 and should not be treated as a suite-wide rate. Observed A/B/C classes use the requested thresholds. With only five runs for stable-looking tasks, the 95% Wilson interval for 0/5 is 0–43.4% and for 5/5 is 56.6–100%; “stable” therefore describes these samples, not proven determinism.

All 45 runs passed validation. No fresh run invoked another Skill. The balanced baseline totals were 448 tools, 98 AST calls, 21 AST failures, 32 retries, 15 help calls, 33 Greps, 13 Globs, 159 Reads, 181,979 tokens, and 2,020.83 seconds. Recovery mean/max was 2.69/5 across 16 recovered failures. Eighteen of 21 AST failures and all 15 help calls came from the five `smoke-001` runs, which directly prescribed obsolete or invalid command forms. Three terminal failures had no recovery distance.

Across all 45 runs, totals were 513 tools, 115 AST calls, 21 AST failures, 32 retries, 15 help calls, 33 Greps, 16 Globs, 182 Reads, 210,538 tokens, and 2,297.94 seconds. Those totals include the intentionally unbalanced extension and are retained for accounting, not comparison.

## 6. Invocation position and the first decision

Every one of the 26 fresh invocations occurred at position 1. The first-action distribution was:

| First action | Runs |
| --- | ---: |
| semantic-analysis Skill | 26 |
| Agent | 9 |
| Grep | 5 |
| direct ast-tool find | 5 |

There were no reads, Greps, Globs, Bash commands, AST attempts, or failures before an invocation. No late invocation occurred after exploration or recovery. Historical Phase 7d normal behavior is identical on this point: all 16 semantic invocations were first. Missing the Skill at the initial routing decision determined the rest of every observed trajectory. In this sample, `semantic-analysis` acts as a first-decision router, not late contextual assistance.

The runtime's recorded discovery attachment consistently presented Skill names and descriptions. It included the `semantic-analysis` name and description, but not the Skill's `triggers` metadata or body. This supports an invocation-surface hypothesis: body text below the frontmatter is not visible at the observed routing point. It does not establish which description words caused a decision.

## 7. Task characteristics and correlations

The strongest observed split is requested operation, not prompt length, repository size, or nominal task level within the probe:

- Direct references, mixed identity/relationship lookup, multi-level callees, and multi-level callers invoked 5/5 each.
- The distributed three-method workflow invoked 6/10, despite sharing the same 27-file, 10,882-byte repository with `level3-003`, which invoked 5/5. Repository size therefore does not explain their difference.
- The 105-word `level4-003` API-evolution prompt invoked 0/5, while 52–82-word relationship prompts invoked 5/5. Prompt length and task complexity alone do not predict invocation.
- `smoke-001` explicitly says `ast-tool` and specifies commands, but invoked `semantic-analysis` 0/5. All five runs started with direct `ast-tool find`; explicit tool wording can bypass Skill routing.
- `level1-001` started with Grep 5/5. `level4-003` started with an exploration Agent 5/5. The non-invocation routes are themselves task-specific.

Historical Phase 7d normal semantic invocation was level 1: 2/8, level 2: 7/8, level 3: 7/8, level 4: 0/8, level 5: 0/8, smoke: 0/1. This correlation is real in that run, but level is bundled with different operation types, prompts, and fixtures, so it is not a causal level effect.

## 8. Loaded versus absent measurements

The unpaired all-task aggregate is shown for completeness and is not a causal comparison:

| Per-run mean | Loaded (26) | Semantic absent (19) | Loaded minus absent |
| --- | ---: | ---: | ---: |
| Success | 100% | 100% | 0 pp |
| Tools | 8.88 | 14.84 | -5.96 |
| AST calls | 2.73 | 2.32 | +0.42 |
| AST failures | 0.12 | 0.95 | -0.83 |
| Retries | 0.12 | 1.53 | -1.41 |
| Help | 0.00 | 0.79 | -0.79 |
| Grep | 0.00 | 1.74 | -1.74 |
| Glob | 0.19 | 0.58 | -0.39 |
| Read | 2.35 | 6.37 | -4.02 |
| Tokens | 3,730 | 5,976 | -2,246 |
| Elapsed seconds | 46.50 | 57.31 | -10.81 |
| Recovery mean/max | 1.00/1 | 3.08/5 | — |

This aggregate is dominated by task selection: the absent arm contains all five recovery-sensitive smoke runs and all five higher-level API runs. It cannot estimate invocation's effect.

`level3-007` is the only randomized-by-model, same-task mixed cohort available. Its ten unchanged runs provide the stronger observational comparison:

| Per-run mean | Loaded (6) | Absent (4) | Loaded minus absent |
| --- | ---: | ---: | ---: |
| Success | 100% | 100% | 0 pp |
| Tools | 13.67 | 12.75 | +0.92 |
| AST calls | 5.33 | 0.00 | +5.33 |
| AST failures / retries / help | 0 / 0 / 0 | 0 / 0 / 0 | 0 |
| Grep | 0.00 | 0.25 | -0.25 |
| Glob | 0.67 | 0.50 | +0.17 |
| Read | 3.33 | 6.75 | -3.42 |
| Bash | 5.67 | 1.25 | +4.42 |
| Edit | 3.00 | 3.00 | 0 |
| Tokens | 7,192 | 3,930 | +3,262 |
| Elapsed seconds | 61.98 | 45.17 | +16.82 |

Loaded runs used targeted semantic search, eliminated Grep, and halved Read calls, but they did not improve correctness, failures, retries, total tools, tokens, or elapsed time in this task. Loaded token cost ranged from 5,155 to 9,966, showing substantial variation after invocation as well. Invocation changed routing; it was not an efficiency benefit here.

## 9. Representative trajectories and earliest divergence

Stable invocation (`level2-006`, representative):

```text
Skill → ast-tool:search → ast-tool:callers → Read/Edit work
```

Stable non-invocation (`level4-003`, representative):

```text
Agent → Glob/Grep/Read exploration → edits
```

Stochastic invocation (`level3-007`, paired examples):

```text
Skill → search×4 → find×3 → Read×3 → Edit×3
```

versus:

```text
Agent → Glob/Bash/Grep → Read×6 → Edit×3
```

The earliest meaningful divergence is the first tool action: `Skill` versus `Agent`. Loaded runs then use 4–7 AST calls; absent runs use an exploration agent and manual reads, with zero AST calls. No trajectory converged by loading the Skill later. Exact sequences for every run are in the generated raw table.

## 10. Correction to the historical invocation analysis

The Phase 7d.2 report's stated 19/41 “semantic-analysis invocation” count actually counts any `Skill` tool call. The trace audit found:

```text
semantic-analysis: 16
ast-inspection:      2
api-review:          1
no Skill:           22
```

The old any-Skill cohort calculation is reproducible exactly: both loaded `+525` tokens, neither loaded `-8,506`, mismatch `+13,925`. Recomputing by the named target Skill gives:

| Semantic-analysis cohort | Tasks | Phase 7d minus Phase 5 tokens | Tools |
| --- | ---: | ---: | ---: |
| Loaded in both | 15 | +1,239 | -1 |
| Absent in both | 21 | -1,007 | +26 |
| Invocation mismatch | 5 | +5,712 | +15 |

The corrected mismatch cohort still contributes most of the historical net token regression, but it no longer exceeds it by the previously reported margin. More importantly, that cohort is unpaired single-run evidence. The fresh same-task evidence shows that invocation can change routing while increasing cost. Therefore the historical aggregate does not establish that missing `semantic-analysis` caused the regression.

## 11. Invocation-facing experiment

No invocation-facing field was modified. Baseline evidence did not justify optimizing invocation rate: the one stochastic task was correct in both arms and was cheaper without the Skill. The runtime discovery record suggests that a description-only experiment is technically meaningful because the description is visible before invocation and trigger/body text is not, but changing it now would optimize toward an outcome that has not shown agent-level benefit.

## 12. Causal assessment

The evidence supports four bounded conclusions:

1. Invocation is task-dependent. Four semantic relationship tasks invoked in every baseline repetition; three controls never invoked.
2. Invocation is also stochastic for at least one fixed task: `level3-007` invoked 6/10 under unchanged observable conditions.
3. Invocation, when it occurs, is exclusively an initial routing decision in these traces. Skill-body wording cannot affect runs where that initial decision is missed because the body was never loaded.
4. Invocation changes tool choice but is not proven to reduce system-level cost. In the only same-task mixed cohort, it reduced manual reads but increased AST/Bash calls, tokens, and time with unchanged correctness.

The design does not randomize invocation, expose model sampling parameters, or isolate hidden service state. It therefore cannot prove why the same prompt selected `Skill` in six runs and `Agent` in four. Task characteristics are correlated and the cohort is small. No claim is made about large repositories, other languages, other models, or later Claude Code versions.

## 13. Final decision

**INVOCATION PROBLEM CONFIRMED, NO SAFE FIX YET**

The first-decision router is materially stochastic for `level3-007`, and normal runs can systematically choose other routes for tasks that plausibly admit semantic analysis. No narrow causal intervention was validated. A blanket increase in invocation is not supported because the same-task loaded arm was more expensive and no more correct. The Phase 7d semantic body remains accepted and unchanged.

## 14. Recommendation

Treat the Phase 7d body as the retained semantic baseline. Do not expand `SKILL.md` to compensate for no-Skill trajectories, and do not promote the old 19/41 any-Skill count as semantic invocation.

If invocation work continues, use a separate Phase 7f description-only experiment on tasks where semantic routing has an explicit expected benefit, including at least one stable positive, one stable negative, and the mixed task. Change only the discovery description, preserve the body hash, repeat at least ten runs per mixed task, and predefine success as correctness plus routing/cost improvement rather than invocation rate. Until such an experiment demonstrates benefit, document invocation as an initial-routing variance and keep the system unchanged.
