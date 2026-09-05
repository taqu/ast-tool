# Phase 7d.1 — Repeated Controlled Guard Validation

## 1. Executive Summary

**MINOR STOCHASTIC DIFFERENCE**

The apparent Phase 7d guard regression is not reproducible as an aggregate or consistent per-task directional effect. Across 12 primary guard runs per version, Phase 5 and Phase 7d each produced 12 AST failures and 110 total tools. Phase 7d used three fewer AST calls, one fewer retry, 2,939 fewer tokens, and no help calls, matching Phase 5's zero help. Both arms passed 12/12.

Trajectory distributions overlap. `level2-006` was identity-first in every run. `level2-008` was relationship-first in 1/3 Phase 5 runs and 2/3 Phase 7d runs—a small directional difference, but both variants occur under both versions. `level3-008` began relationship-first in 1/3 runs for each version. `smoke-001` was short-recovery in all six runs, and Phase 7d actually had fewer failures and retries there.

One Phase 7d `level3-008` run reached recovery distance 5 after a failed search and intervening Bash exploration. This is a real outlier, but it occurred once, was not relationship-first, and was offset by two Phase 7d runs with maximum recovery 0 and 2. Paired differences reverse direction across repeats.

The positive control remains directionally present but is less discriminative than earlier evidence: Phase 7d avoided `find` in 2/3 runs, but Phase 5 also avoided it in 2/3. Phase 7d averaged fewer AST calls and tools, though the distributions overlap.

**Phase 7d may proceed to the 18-task controlled evaluation with a caveat.**

## 2. Experimental Setup

### Tasks and repeats

Primary guards:

```text
level2-006
level2-008
level3-008
smoke-001
```

Positive control, reported separately:

```text
level3-007
```

Three fresh repeats were run per task per version: 24 primary guard runs plus six positive-control runs. This is the permitted resource-constrained minimum. All 30 runs succeeded and invoked `semantic-analysis` exactly once as the first tool action.

### Execution order

The runner's task order within each arm was:

```text
level2-006 → level2-008 → level3-007 → level3-008 → smoke-001
```

Arm schedule:

```text
repeat 1: Phase 5 → Phase 7d
repeat 2: Phase 7d → Phase 5
repeat 3: Phase 5 → Phase 7d
```

### Recorded environment

| Item | Value |
|---|---|
| Claude Code runtime | 2.1.113 |
| Repository / AST Tool revision | `39ce7c7c12c46abb78f3c320a5a92b544ace6412` |
| Phase 5 skill SHA-256 | `155B0D154E1DC9FEF7E3193A01277D98B317C86EA37C13F53D20EA219661BF90` |
| Phase 7d skill SHA-256 | `96B07A6B89AE338F26D45FBFB31DD97D5B9C50EFA39922103E8CB3E616807EAF` |
| Harness SHA-256 | `8E7B5DBD67B7287E100D2C281E99A698FFB7F37CC9184428D1CD4424DCCD7131` |
| Forced invocation | Constant system instruction; Skill exactly once and first in 30/30 |

The CLI exposes no working `--version`, and the model identifier is not present in the evaluation records, so the repository revision and Claude Code runtime are the strongest observable controls. No runtime change was observed. Timestamps and fixture repository revisions are retained in each `results.jsonl`.

## 3. Aggregate Guard Metrics

Positive control excluded:

| Metric | Phase 5 | Phase 7d | Phase 7d − Phase 5 |
|---|---:|---:|---:|
| Runs / successes | 12 / 12 | 12 / 12 | 0 |
| Total tools | 110 | 110 | 0 |
| AST calls | 39 | 36 | -3 |
| AST failures | 12 | 12 | 0 |
| AST retries | 11 | 10 | -1 |
| AST help | 0 | 0 | 0 |
| Mean per-run maximum recovery | 0.50 | 1.00 | +0.50 |
| Maximum recovery | 2 | 5 | +3, one outlier |
| Tokens | 44,184 | 41,245 | -2,939 (-6.7%) |
| Elapsed | 624.33 s | 632.46 s | +8.13 s (+1.3%) |

The failure total that motivated Phase 7d.1 disappeared: repeated Phase 5 and Phase 7d totals are identical. Recovery's mean/max is worse solely because of one Phase 7d `level3-008` distance-5 path; the median per-run maximum is 0 for both versions.

### Paired repeat deltas

| Repeat | Δfailures | Δretries | Δmax recovery | Δtools | Δtokens | Δelapsed |
|---|---:|---:|---:|---:|---:|---:|
| 1 | +1 | +2 | +4 | +5 | -891 | +37.41 s |
| 2 | +1 | -1 | 0 | -3 | -2,238 | -8.96 s |
| 3 | -2 | -2 | 0 | -2 | +190 | -20.32 s |

No metric shows a stable repeat-by-repeat direction. Within-version/run variance is at least as large as the between-version difference.

## 4. Per-Task Distributions

Recovery values below are maximum recovery distance per run; no recovery is recorded as 0.

### `level2-006`

| Version | Success | Failures mean / median / range | Retries mean / median | Help frequency | Recovery mean / max | Classes |
|---|---:|---|---|---:|---|---|
| Phase 5 | 3/3 | 0 / 0 / 0–0 | 0 / 0 | 0/3 | 0 / 0 | IDENTITY_FIRST 3/3 |
| Phase 7d | 3/3 | 0 / 0 / 0–0 | 0 / 0 | 0/3 | 0 / 0 | IDENTITY_FIRST 3/3 |

Exact route in all six runs: `search → callers`.

### `level2-008`

| Version | Success | Failure counts | Failures mean / median / range | Retry counts | Help | Recovery | Classes |
|---|---:|---|---|---|---:|---|---|
| Phase 5 | 3/3 | 0, 0, 1 | 0.33 / 0 / 0–1 | 0, 0, 1 | 0/3 | 0, 0, 1 | IDENTITY_FIRST 2/3; RELATIONSHIP_FIRST_RECOVERED 1/3 |
| Phase 7d | 3/3 | 1, 0, 1 | 0.67 / 1 / 0–1 | 1, 0, 1 | 0/3 | 1, 0, 1 | IDENTITY_FIRST 1/3; RELATIONSHIP_FIRST_RECOVERED 2/3 |

Phase 7d is directionally worse by one relationship-first sample, but the distributions overlap and every failure recovered immediately in one step.

### `level3-008`

| Version | Success | Failure counts | Failures mean / median / range | Retry counts | Help | Recovery | Classes |
|---|---:|---|---|---|---:|---|---|
| Phase 5 | 3/3 | 0, 1, 0 | 0.33 / 0 / 0–1 | 0, 3, 0 | 0/3 | 0, 2, 0 | IDENTITY_FIRST 2/3; RELATIONSHIP_FIRST_RECOVERED 1/3 |
| Phase 7d | 3/3 | 1, 2, 0 | 1.00 / 1 / 0–2 | 2, 2, 0 | 0/3 | 5, 2, 0 | IDENTITY_FIRST 2/3; RELATIONSHIP_FIRST_RECOVERED 1/3 |

Both versions have the same relationship-order class frequency. Phase 7d's first run performed extra semantic/manual recovery after a failed initial search and is classified EXTRA_SEMANTIC_EXPLORATION; this creates the distance-5 outlier. The other Phase 7d runs are within the Phase 5 trajectory family.

### `smoke-001`

| Version | Success | Failure counts | Failures mean / median / range | Retry counts | Help | Recovery | Classes |
|---|---:|---|---|---|---:|---|---|
| Phase 5 | 3/3 | 3, 3, 4 | 3.33 / 3 / 3–4 | 2, 2, 3 | 0/3 | 1, 1, 1 | SHORT_RECOVERY 3/3 |
| Phase 7d | 3/3 | 2, 3, 2 | 2.33 / 2 / 2–3 | 1, 2, 1 | 0/3 | 1, 1, 1 | SHORT_RECOVERY 3/3 |

Definitions: SHORT ≤2, MODERATE =3, LONG ≥4. Every run was SHORT. Phase 7d had one fewer mean failure and one fewer mean retry than Phase 5.

## 5. Relationship-Ordering Analysis

Phase 7d does not reliably make relationship-first behavior more likely:

- `level2-006`: Phase 5 0/3 relationship-first; Phase 7d 0/3.
- `level2-008`: Phase 5 1/3; Phase 7d 2/3.
- `level3-008`: Phase 5 1/3; Phase 7d 1/3.
- Combined: Phase 5 2/9; Phase 7d 3/9.

The combined difference is one run, both versions exhibit the same variants, and all relationship-first cases recover semantically through search. This is not strong evidence of a systematic Skill interaction.

## 6. `smoke-001` Variance Analysis

`smoke-001` is intrinsically variable in raw failure count, but not in recovery class during this experiment. All six runs:

- corrected the initial malformed `find` without help;
- used no unchanged failed-command retry detected by the analyzer;
- used no help;
- had maximum recovery distance 1;
- validated successfully.

References ambiguity generated most failures. Phase 5 ranged 3–4 failures; Phase 7d ranged 2–3. The prior Phase 7d regression is not reproducible here.

## 7. Positive Control — `level3-007`

| Metric | Phase 5 runs | Phase 7d runs |
|---|---|---|
| Success | 3/3 | 3/3 |
| Search calls | 4, 5, 5 | 3, 3, 6 |
| Find calls | 3, 0, 0 | 0, 3, 0 |
| AST calls | 7, 5, 5 | 3, 6, 6 |
| Total tools | 16, 12, 12 | 10, 13, 14 |
| Tokens | 7,424; 7,182; 6,011 | 5,276; 6,932; 7,060 |
| Elapsed | 72.16; 59.63; 52.73 s | 50.38; 57.46; 77.39 s |

Phase 7d avoided redundant `find` in 2/3 runs and introduced no failures or manual fallback. Mean AST calls were 5.00 versus Phase 5's 5.67; mean tools were 12.33 versus 13.33; mean tokens were 6,423 versus 6,872. The earlier strong difference is directionally reproduced but weakened because Phase 5 independently avoided `find` in 2/3 new samples. This positive control supports the candidate only with a caveat, not as a deterministic version effect.

## 8. Between-Version vs Within-Version Variance

Within-version variance dominates most between-version differences:

- Guard paired failure deltas were +1, +1, and -2.
- Guard tool deltas were +5, -3, and -2.
- Guard elapsed deltas were +37.41, -8.96, and -20.32 seconds.
- Phase 7d positive-control searches ranged 3–6 and tools 10–14.
- Phase 5 positive-control finds ranged 0–3 despite an unchanged Skill.

The only conspicuous between-version outlier is Phase 7d's one recovery-distance-5 `level3-008` run. One sample is insufficient for SYSTEMATIC GUARD REGRESSION, especially because relationship ordering matched Phase 5 on that task and the other Phase 7d runs recovered within 0–2.

## 9. Final Decision

**PROCEED WITH CAVEAT**

Classify guard behavior as **MINOR STOCHASTIC DIFFERENCE**. The previous aggregate Phase 7d failure excess was not reproduced: failures and tools are equal, retries slightly favor Phase 7d, help remains zero, relationship-order frequencies substantially overlap, and `smoke-001` favors Phase 7d.

Proceed to the full 18-task controlled Phase 5 vs Phase 7d evaluation. Carry forward two caveats:

1. Monitor `level3-008` for rare long recovery after failed search.
2. Treat the positive-control benefit as directional rather than deterministic because Phase 5 also found the cheaper route in two new samples.

Do not run the normal 41-task evaluation until the 18-task controlled result is favorable.
