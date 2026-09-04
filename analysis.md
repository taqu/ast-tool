# Phase 8 — Final Quantitative Evaluation

## 1. Executive Summary

**Decision: REJECT**

Compared with the Phase 5 stable baseline, the committed Phase 7b result does not provide a better Coding Agent overall while preserving AST Tool's intended semantic-routing behavior.

Correctness is preserved at 37/41 and elapsed time falls by 50.3%, but the central routing signal regresses: real AST Tool commands fall from 68 to 22, grep rises from 15 to 64, and the Skill is invoked on only 2 tasks rather than 20. Reported tokens rise 52.3%, with 23/41 paired tasks worsening by more than 250 tokens. Recovery is also slower after failure (mean distance 1.44 to 2.60; maximum 2 to 5). These changes are not explained by one token outlier.

The compressed Skill body cannot be isolated as the cause of the invocation collapse: Phase 5 and Phase 7b have the same frontmatter metadata, and the body is unavailable until the Skill is loaded. However, Phase 8 evaluates the committed agent-level system, and that observed system exhibits the failure pattern the evaluation was designed to reject: semantic routing collapse plus broad grep fallback.

## 2. Final Phase 5 vs Phase 7b Comparison

| Metric | Phase 5 | Phase 7b | Absolute change | Relative change | Assessment |
|---|---:|---:|---:|---:|---|
| Success rate | 37/41 (90.24%) | 37/41 (90.24%) | 0 pp | 0.0% | Preserved |
| Total tool calls | 518 | 411 | -107 | -20.7% | Improved |
| Average tool calls/test | 12.63 | 10.02 | -2.61 | -20.7% | Improved |
| AST Tool calls (raw parser) | 69 | 22 | -47 | -68.1% | Suspicious |
| Real AST commands | 68 | 22 | -46 | -67.6% | Regression with trajectory evidence |
| AST failures | 9 | 5 | -4 | -44.4% | Count lower, rate worse |
| AST failure rate | 13.04% | 22.73% | +9.69 pp | +74.3% | Worse |
| AST retries | 9 | 8 | -1 | -11.1% | Little improvement |
| AST help calls | 2 | 5 | +3 | +150.0% | Worse |
| Grep | 15 | 64 | +49 | +326.7% | Regression |
| Glob | 12 | 6 | -6 | -50.0% | Improved |
| Read | 252 | 181 | -71 | -28.2% | Improved in aggregate |
| Bash | 120 | 73 | -47 | -39.2% | Improved |
| Edit | 86 | 81 | -5 | -5.8% | Similar |
| Average recovery distance | 1.44 | 2.60 | +1.16 | +80.6% | Worse |
| Maximum recovery distance | 2 | 5 | +3 | +150.0% | Worse |
| Elapsed time | 2,224.27 s | 1,104.85 s | -1,119.42 s | -50.3% | Improved |
| Average elapsed/test | 54.25 s | 26.95 s | -27.30 s | -50.3% | Improved |
| Total tokens | 158,303 | 241,130 | +82,827 | +52.3% | Worse; accounting caveat applies |
| Average tokens/test | 3,861 | 5,881 | +2,020 | +52.3% | Worse |
| Median tokens/test | 2,322 | 3,452 | +1,130 | +48.7% | Worse |
| p90 tokens/test | 7,592 | 15,267 | +7,675 | +101.2% | Worse |

The Phase 5 raw parser count includes one known false positive, `2`, from `which ast-tool 2>/dev/null`. It is excluded from the 68-command corrected count. Phase 7b's `top_level` entry is an actual top-level help invocation, not that shell-redirection artifact.

Complete Phase 7b per-task records—including success, tools, AST calls, grep/glob/read, token counts, elapsed time, Skill calls, and AST sequence—are in `evaluation/final_analysis/phase7b_full/`. The compact table is `evaluation/final_analysis/phase7b/task_summary.csv`; corresponding Phase 5 records are in `evaluation/final_analysis/phase5_full/`.

## 3. Correctness

Phase 5 and Phase 7b fail exactly the same four tasks: `level4-002`, `level4-005`, `level4-006`, and `level4-007`. There are no Phase 5 failure → Phase 7b success transitions and no new Phase 7b failures.

All four validators report the same compilation defect in `api_handler.cpp`: references to `processor_` and `registry_` do not match declarations in the fixture's `api_handler.h`. Because the failure set and diagnostics are unchanged from the baseline, these are existing task/fixture defects or repeated implementation failures, not evidence of a new Phase 7b correctness regression. No infrastructure failure or timeout is present; all 41 Phase 7b records are valid and complete.

## 4. Routing Behavior

| Command | Phase 5 | Phase 7b |
|---|---:|---:|
| `search` | 30 | 5 |
| `callers` | 14 | 4 |
| `references` | 6 | 7 |
| `callees` | 3 | 0 |
| `find` | 12 | 4 |
| `symbols` | 1 | 1 |
| `outline` | 1 | 0 |
| Top-level help | 1 | 1 |

Phase 7b AST failures are `search` 1, `find` 1, `references` 2, and `callers` 1. Phase 5 failures are `search` 1, `callers` 3, `callees` 1, `find` 3, and `references` 1.

Classification:

- **BENIGN:** fewer total reads, Bash calls, and tools; unchanged correctness.
- **SUSPICIOUS:** the AST failure count falls only because semantic calls nearly disappear, while the failure rate and help use rise.
- **REGRESSION:** semantic commands fall 67.6% while grep rises 326.7%; 30/41 tasks increase grep. This is trajectory evidence that broad textual lookup replaced direct semantic routing across much of the run.

The strongest body-specific evidence is limited. In the two tasks loading the Skill in both phases, routing stays semantic but becomes more exploratory: `level2-004` changes from `search → callers → references` to `search → search → callers → search → references`; `level4-006` changes from the same Phase 5 sequence to `search → references → callers → search`. Thus the loaded Skill does not cause AST avoidance in those two cases, but it also does not demonstrate an efficiency improvement.

## 5. Manual Exploration

Aggregate manual calls change as follows: grep 15 → 64, glob 12 → 6, and read 252 → 181. The read reduction is useful, but it does not neutralize the routing regression: grep rises on 30 tasks, is unchanged on 8, and falls on only 3. Positive grep deltas total 53 calls, partially offset by four fewer calls across the three improved tasks.

Largest grep increases are `level4-004` (+4), `level5-008` (+4), `level2-005` (+3), `level4-001` (+3), then several tasks at +2. The increase is distributed rather than concentrated in one task. The largest read increases are `level3-003` (+5), `level2-004` (+4), `level1-002` (+2), and `smoke-001` (+2); read rises on 11 tasks, is unchanged on 16, and falls on 14.

For the 18 tests loading the Skill only in Phase 5, AST calls fall from 53 to 0 while grep rises from 0 to 27 and reads rise from 37 to 48. That is direct evidence of AST avoidance and textual fallback in those trajectories, but not proof that the compressed body caused it: the body was never loaded in Phase 7b. In the neither-loaded cohort, grep also rises from 15 to 34, consistent with broader run-to-run routing variance.

## 6. Recovery

Phase 5 recovery distances are `[1, 1, 1, 2, 1, 2, 1, 2, 2]` (mean 1.44, max 2). Phase 7b distances are `[1, 2, 5, 2, 3]` (mean 2.60, max 5).

`level2-004`, with the Skill loaded, recovers from a failed search after one intervening step; the correction remains targeted. The long paths occur in `smoke-001`, where the Skill was not loaded. It starts with an invalid `find`, uses help, and corrects it; later ambiguous `references` and `callers` attempts lead through reads, repeated semantic attempts, five help calls in total, and shell discovery before recovery. The maximum-distance path is 5 and another is 3. The trace includes unnecessary help/format exploration and does not consistently turn diagnostics into the cheapest correction.

Recovery therefore regresses at the agent-system level, but the notable long case cannot be attributed to the compressed Skill body because that Skill was absent.

## 7. Tool and Latency Efficiency

Phase 7b uses 107 fewer tool calls and finishes 1,119.42 seconds faster. These are material operational improvements. The lower totals come mainly from 71 fewer reads, 47 fewer Bash calls, and 5 fewer edits.

However, speed and call-count improvements are not sufficient for acceptance because the remaining discovery work shifts away from the intended semantic interface. The result is faster but less targeted, and its reported output cost is substantially higher.

## 8. Token Analysis

| Distribution | Phase 5 | Phase 7b |
|---|---:|---:|
| Total | 158,303 | 241,130 |
| Mean | 3,861 | 5,881 |
| Median | 2,322 | 3,452 |
| p75 | 5,414 | 8,774 |
| p90 | 7,592 | 15,267 |
| Maximum | 16,378 | 21,125 |

For paired deltas (`Phase 7b - Phase 5`), the minimum is -1,957, median +427, p75 +2,725, p90 +7,576, and maximum +13,690. Using an explicit ±250-token tolerance, 8 tasks improve, 10 are approximately unchanged, and 23 worsen.

The largest token increases are `level2-004` (+13,690), `level4-005` (+11,968), `smoke-001` (+11,309), `level4-003` (+7,675), and `level5-002` (+7,576). The top one contributes 16.5% of the net +82,827 delta; the top three contribute 44.6%; the top five contribute 63.0%. Since the median is positive and 23 tasks worsen beyond tolerance, this is not solely a one-outlier result.

Token accounting deserves caution. Phase 5 reports 2,566 input and 155,737 output tokens; Phase 7b reports 1,364 input and 239,766 output tokens. Cache-read and cache-creation values are zero in both. The result schema and evaluation harness are the same, and no intervening harness-code change was found, but the stored results do not identify an exact model/runtime version. The combination of 54% more output tokens with half the elapsed time and fewer tools suggests the totals may not be perfectly comparable. The paired distribution, routing traces, manual-tool counts, and latency are therefore stronger evidence than the exact token ratio.

Using the approximate Phase 5 Skill size of 3,263 tokens, Phase 7b's 809 tokens, and only the 2 comparable both-loaded invocations gives a nominal fixed saving of `(3,263 - 809) × 2 = 4,908` prompt tokens. Yet that cohort's observed total tokens rise by 16,824. Adjusting for the nominal saving would make trajectory/output cost roughly 21,732 tokens worse. With only two observations and no reported prompt-cache usage, this estimate is illustrative, not causal.

## 9. Skill-Invocation Cohort Analysis

| Cohort | Tasks | Success P5 → P7b | AST calls | Grep | Read | Tool calls | Tokens | Elapsed |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| A: Skill in both | 2 | 1 → 1 | 6 → 9 | 0 → 3 | 7 → 10 | 25 → 35 | 9,802 → 26,626 | 125.93 → 84.13 s |
| B: Skill in one run | 18 | 18 → 18 | 53 → 0 | 0 → 27 | 37 → 48 | 157 → 114 | 53,185 → 63,431 | 910.11 → 276.19 s |
| C: Skill in neither | 21 | 18 → 18 | 9 → 12 | 15 → 34 | 208 → 123 | 336 → 262 | 95,316 → 151,073 | 1,188.23 → 744.53 s |

Group A is the best evidence about body compression, but `n=2` is too small for a strong general conclusion. Both paths remain semantically targeted, while tools, AST calls, grep, reads, and tokens all rise. This does not show a systematic route-to-grep collapse after loading, but it provides no efficiency validation either.

Group B contains 18 Phase-5-only loads. Its 53 → 0 AST and 0 → 27 grep shift explains most of the routing collapse. Because Phase 7b never loaded the Skill on these tasks and the frontmatter is unchanged, this cohort cannot establish a causal body-compression defect. It does establish that the final run did not deliver the required agent-level behavior.

Group C shows that some variance is independent of Skill text: grep more than doubles and tokens increase substantially even when neither run loads the Skill.

## 10. Historical Trend

| Phase | Success | Tool calls | AST calls | Grep | Read | Tokens | Elapsed |
|---|---:|---:|---:|---:|---:|---:|---:|
| Phase 1 | 90.24% | 553 | 67 | 25 | 279 | 176,983 | 2,290.81 s |
| Phase 2 | 90.24% | 519 | 70 | 18 | 254 | 162,628 | 2,100.56 s |
| Phase 5 | 90.24% | 518 | 69 raw / 68 corrected | 15 | 252 | 158,303 | 2,224.27 s |
| Phase 7a | 92.68% | 545 | 60 | 31 | 275 | 156,439 | 2,250.54 s |
| Phase 7b | 90.24% | 411 | 22 | 64 | 181 | 241,130 | 1,104.85 s |

Phase 7a values come from its accepted report. The currently stored `results_phase7a.jsonl` is an interrupted/non-comparable replay, so it is not substituted into this table. Phase 3 and Phase 6 are rejected experimental branches and are excluded from the success criterion.

The trend makes Phase 7b anomalous: it is fastest and uses the fewest tools, but also uses by far the most grep, far fewer semantic commands, and the most reported tokens.

## 11. Limitations

- Forty-one tasks are enough to expose a large routing shift but not to claim statistical significance for small differences.
- Skill loading is stochastic. Only two paired tasks loaded the Skill in both runs, limiting body-specific inference.
- The four recurring Level 4 failures are confounded by a known fixture/declaration defect.
- Exact model/runtime configuration is absent from stored results. Token accounting has the same schema and zero cache fields but is not fully auditable across runs.
- Repository revision hashes vary per task/run due to fixture setup; exact tree identity was not established from the result records alone.
- Phase 7a historical metrics rely on the accepted report because the remaining local replay is incomplete.
- Parser false positives were corrected only where trace evidence was clear; Phase 5's `2` artifact is reported separately.

## 12. Final Recommendation

**REJECT**

Phase 7b preserves correctness and materially improves latency and total tool count, but it fails the central acceptance condition: targeted semantic routing is not preserved at the Coding Agent level. AST commands fall by roughly two-thirds, grep rises more than fourfold across 30 tasks, recovery worsens, and token growth is broad enough that it cannot be dismissed as one outlier.

This decision does **not** establish that the compressed Skill body itself caused the regression. The smallest follow-up is a controlled rerun that holds Skill invocation constant—or explicitly invokes the Skill on the same paired task set—while recording model/runtime identity. Until that evidence demonstrates preserved routing, the committed Phase 7b result should not replace Phase 5 as the stable baseline. No Phase 7b candidate, tool implementation, evaluation task, or metrics logic was changed during this evaluation.
