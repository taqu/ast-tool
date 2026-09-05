# Phase 7c — Controlled Phase 5 vs Phase 7c Evaluation

## Recommendation

**ACCEPT WITH CAVEATS**

Phase 7c preserves correctness and semantic command coverage when Skill invocation is controlled: both versions passed 17 of 18 tasks, and both failed the same `level4-006` validation. All 36 runs invoked `semantic-analysis` exactly once as their first tool call.

The result is not cost-neutral. Phase 7c used 11 more tools, 7 more AST calls, 3 more AST failures, 4 more retries, 2 more help calls, 5,096 more tokens, and 57.44 more seconds. These costs are concentrated rather than universal. Its largest structural change—replacing six `find` calls with one targeted `search` route in `level3-007`—saved four tools and 2,494 tokens. The clearest regressions came from relationship commands attempted before FQN discovery and from `smoke-001`, even though Phase 7c retains the applicable search-first and error-directed recovery rules.

This supports semantic preservation with behavioral variance, not strict trajectory equivalence.

## Controlled Method

The representative cohort was:

```text
level1-001  level1-002  level1-005  level1-006
level2-001  level2-004  level2-005  level2-006  level2-008
level3-002  level3-003  level3-004  level3-005  level3-007  level3-008
level4-003  level4-006  smoke-001
```

Coverage includes `search`, `find`, `callers`, `callees`, `references`, `symbols`, C++ ambiguity, not-found recovery, invalid-root recovery, help behavior, and grep fallback. Task YAML, fixtures, agent invocation, permissions, timeouts, and the forced-load instruction were identical. Only the installed skill body changed:

- Phase 5: commit `e561505`
- Phase 7c: commit `78c79af`
- Control: a constant system instruction required invoking `semantic-analysis` before exploration
- Verification: Skill was called exactly once and first in 18/18 runs per arm

Raw results, traces, and summaries are under `evaluation/controlled_phase5/` and `evaluation/controlled_phase7c/`.

## Aggregate Results

| Metric | Phase 5 | Phase 7c | Delta |
|---|---:|---:|---:|
| Successes | 17/18 | 17/18 | 0 |
| Success rate | 94.44% | 94.44% | 0 pp |
| Total tools | 162 | 173 | +11 |
| Average tools/test | 9.00 | 9.61 | +0.61 |
| AST calls | 57 | 64 | +7 |
| AST failures | 5 | 8 | +3 |
| AST failure rate | 8.77% | 12.50% | +3.73 pp |
| AST retries | 4 | 8 | +4 |
| AST help calls | 0 | 2 | +2 |
| Search | 24 | 29 | +5 |
| Callers | 14 | 16 | +2 |
| References | 4 | 6 | +2 |
| Callees | 2 | 4 | +2 |
| Find | 13 | 6 | -7 |
| Symbols | 0 | 2 | +2 |
| Other AST | 0 | 1 | +1 top-level help |
| Grep | 3 | 2 | -1 |
| Glob | 4 | 7 | +3 |
| Read | 38 | 39 | +1 |
| Bash | 60 | 68 | +8 |
| Edit | 38 | 39 | +1 |
| Recovery mean/max | 1.00 / 1 | 1.50 / 4 | +0.50 / +3 |
| Elapsed | 912.59 s | 970.03 s | +57.44 s (+6.3%) |
| Tokens | 54,344 | 59,440 | +5,096 (+9.4%) |
| Average tokens/test | 3,019.1 | 3,302.2 | +283.1 |

Recovery distances were `1,1,1,1` for Phase 5 and `1,1,1,2,1,1,4,1` for Phase 7c.

## Token Distribution

| Statistic | Phase 5 | Phase 7c | Paired delta |
|---|---:|---:|---:|
| Median | 1,962.5 | 2,598 | +216 |
| p75 | 4,720 | 4,915 | +774 |
| p90 | 5,839 | 5,940 | +1,243 |
| Total | 54,344 | 59,440 | +5,096 |

Largest regressions were `level2-004` (+2,527), `level2-001` (+1,243), `level1-001` (+811), `level2-005` (+775), and `level1-006` (+774). Largest improvements were `level3-007` (-2,494) and `level1-002` (-1,024). The largest token regression had one fewer tool and an unchanged AST sequence, so it is generation variance rather than semantic-routing loss.

## Per-Test Comparison

| Task | Result P5/P7c | Δtools | ΔAST | Failures | Find | Glob | Read | Δtokens |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| `level2-004` | pass/pass | -1 | 0 | 0→0 | 0→0 | 0→0 | 3→3 | +2,527 |
| `level2-001` | pass/pass | +2 | +1 | 0→0 | 0→1 | 0→1 | 1→1 | +1,243 |
| `level1-001` | pass/pass | +1 | 0 | 0→0 | 1→0 | 0→0 | 1→2 | +811 |
| `level2-005` | pass/pass | +2 | +1 | 0→0 | 2→2 | 0→1 | 2→2 | +775 |
| `level1-006` | pass/pass | +1 | 0 | 0→0 | 1→1 | 0→1 | 1→1 | +774 |
| `smoke-001` | pass/pass | +4 | +4 | 2→3 | 2→2 | 0→0 | 1→1 | +737 |
| `level2-006` | pass/pass | +1 | +1 | 0→1 | 0→0 | 0→0 | 4→4 | +713 |
| `level4-006` | fail/fail | 0 | 0 | 0→0 | 0→0 | 1→1 | 4→4 | +402 |
| `level3-004` | pass/pass | +2 | +2 | 0→0 | 0→0 | 1→1 | 2→2 | +282 |
| `level3-003` | pass/pass | 0 | 0 | 0→0 | 1→0 | 0→0 | 1→1 | +150 |
| `level2-008` | pass/pass | +1 | +1 | 0→1 | 0→0 | 0→0 | 1→1 | +71 |
| `level4-003` | pass/pass | 0 | 0 | 0→0 | 0→0 | 1→1 | 6→6 | +61 |
| `level3-008` | pass/pass | +2 | +2 | 1→2 | 0→0 | 0→0 | 2→2 | +55 |
| `level3-005` | pass/pass | 0 | 0 | 1→0 | 0→0 | 0→0 | 1→1 | +13 |
| `level3-002` | pass/pass | 0 | 0 | 1→1 | 0→0 | 0→0 | 1→1 | +12 |
| `level1-005` | pass/pass | +1 | 0 | 0→0 | 0→0 | 0→0 | 1→2 | -12 |
| `level1-002` | pass/pass | -1 | 0 | 0→0 | 0→0 | 0→0 | 3→2 | -1,024 |
| `level3-007` | pass/pass | -4 | -5 | 0→0 | 6→0 | 1→1 | 3→3 | -2,494 |

## Trajectory Analysis

### Preserved and beneficial

- `level3-007`: Phase 5 used `search×3 → find×6`; Phase 7c used `search×4`. Phase 7c found the same targets, preserved correctness and reads, and saved five AST calls, four tools, 2,494 tokens, and 21.42 seconds. This is a valid targeted substitution.
- `level1-001` and `level3-003`: Phase 5 used `search → find`; Phase 7c used `search → refined search`. Both are appropriate targeted routes; no broad manual chain appeared.
- `level1-002`, `level1-005`, `level2-004`, `level3-002`, `level3-005`, and `level4-003` preserved or equivalently reordered their substantive semantic sequence.
- Grep decreased 3→2. Phase 7c did not systematically replace semantic commands with text search.

### Recovery regressions

- `level2-006` and `level2-008`: Phase 5 followed `search → callers`. Phase 7c tried an under-qualified callers target first, then recovered via the diagnostic with `search → callers`. Each added one failure and one tool.
- `level3-008`: Phase 7c began with an under-qualified callers query, then used `symbols` and corrected callers. It remained semantic and succeeded, but added two AST calls and one net failure.
- `smoke-001`: both versions corrected malformed `find` immediately. Phase 5 then made one ambiguous `references` attempt. Phase 7c first passed a source file and line as relationship arguments, used `references --help`, retried ambiguously, and searched candidates. This produced recovery distance 4, one extra failure, two help calls, four extra tools, and 737 tokens.

Phase 7c already documents search-before-callers, references syntax, directory roots, command-specific flags, restricted help, and diagnostic-first recovery. These paths violate retained rules rather than demonstrating a deleted decision boundary. Reduced salience or sampling variance remains plausible.

### Structural/manual exploration

Glob increased 4→7 and Read 38→39. Extra Globs were isolated (`level2-001`, `level2-005`, `level1-006`) and did not become multi-read manual inspection chains. The controlled cohort does not show a systematic semantic-to-manual regression.

## Rule-Level Review

| Phase 5 behavior | Phase 7c equivalent | Controlled evidence |
|---|---|---|
| Unknown symbol/file → `search` | Workspace symbol/declaration route | Preserved broadly; two callers-first exceptions recovered immediately |
| Known-file AST node → `find` | Known-file node route and file boundary | Refined search sometimes substituted; largest change was cheaper |
| Usages → `references` | Dedicated references route | Preserved |
| Incoming/outgoing calls → callers/callees | Dedicated routes and workflow | Preserved, with recovery variants |
| Diagnostic-first, no unchanged retry | Error-directed recovery | Usually preserved; `smoke-001` is the material exception |
| Restricted help | Help only if syntax and diagnostic are insufficient | Violated once despite retained wording |
| Grep only for text/fallback | Semantic preference and fallback boundary | Preserved; Grep decreased |
| Narrow scope and targeted reads | Output/cost boundaries | No systematic broad-read regression |

No exact deleted Phase 5 sentence cleanly explains the regressions. The problematic rules remain present in Phase 7c, but adherence was weaker in this paired sample.

## Skill Size

| Version | Lines | Characters | UTF-8 bytes | Approx. tokens |
|---|---:|---:|---:|---:|
| Phase 5 | 355 | 13,053 | 13,111 | 3,263 |
| Phase 7c | 135 | 8,745 | 8,759 | 2,186 |
| Reduction | 220 (62.0%) | 4,308 (33.0%) | 4,352 (33.2%) | 1,077 (33.0%) |

The fixed prompt saving is approximately 1,077 tokens whenever the skill loads. Reported run tokens do not directly attribute cached system/skill context, so the 5,096 observed increase is not the literal cost of the shorter skill.

## Existing Normal 41-Task Result

The controlled run answers body equivalence. The existing unchanged runs answer full agent behavior:

| Metric | Phase 5 | Phase 7c |
|---|---:|---:|
| Successes | 37/41 | 37/41 |
| Tool calls | 518 | 542 |
| AST calls/failures | 69 / 9 | 59 / 7 |
| AST failure rate | 13.04% | 11.86% |
| Grep / Glob / Read | 15 / 12 / 252 | 17 / 22 / 286 |
| Bash / Edit | 120 / 86 | 102 / 82 |
| Recovery mean/max | 1.44 / 2 | 1.60 / 4 |
| Elapsed | 2,224.27 s | 2,214.34 s |
| Tokens | 158,303 | 178,116 |

The full run preserves correctness and improves AST failures, Bash, Edit, and elapsed, but regresses Glob, Read, total tools, recovery maximum, and tokens. Prior trace analysis shows its largest broad-exploration regressions occurred when the skill was not loaded, so they cannot be assigned to the compressed body alone.

## Separate Conclusions

### A. Body semantic equivalence

**Preserved with caveats.** Correctness, command-family coverage, and direct semantic routing remain available under forced loading. There is no systematic replacement by Grep/manual inspection. Phase 7c nevertheless has worse AST failure and recovery efficiency in this sample, especially `smoke-001` and three relationship-first paths.

### B. Full agent-level behavior

**Correctness preserved, efficiency mixed.** The normal run maintains 37 successes and improves several metrics, but regresses Glob, Read, tools, recovery maximum, and tokens. Invocation variance explains much of the broad exploration.

## Final Decision

**ACCEPT WITH CAVEATS**

Phase 7c passes the essential semantic-preservation test: equal controlled correctness, complete command-family coverage, no systematic semantic-to-manual fallback, and a materially smaller skill. It does not demonstrate trajectory parity. Retain Phase 5 as the stable fallback until the relationship-first and `smoke-001` recovery differences are replicated or disproved with repeated controlled trials. Do not proceed automatically to later phases from this single pair.
