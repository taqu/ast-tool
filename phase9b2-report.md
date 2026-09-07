# Phase 9b.2 Report — Final Normal-Routing Agent-Level Evaluation

## 1. Environment and Revisions

- Host branch: `phase9b2`
- Model: `claude-sonnet-5`
- Claude Code CLI version: `2.1.261`
- Harness: `evaluation/phase9b2_probe.py` (run orchestration) + `evaluation/phase9b2_analyze.py` (aggregation), both new for this phase, reusing the proven Phase 9b.1 binary-swap/isolated-log pattern
- Evaluation date: 2026-09-07

## 2. Exact Arm A / Arm B Definitions

Identical to Phase 9b.1, reused without rebuilding:

| Artifact | Value |
|---|---|
| Arm A git revision | `ccc1fbb650bf058aa11602134d4e4fa1795cb98e` (pre-Phase-8) |
| Arm B git revision | `3095337a5ff04c7de723c3a0853682c9cefdf90c` (accepted Phase 8a+8b+8c) |
| Arm A binary SHA-256 | `35d17a4297dda52e52a9699f2655fad744846f1ac7f74f4ed68bf9eb3db832a5` |
| Arm B binary SHA-256 | `95bbffe793526a88d358cdf2b0b22507fd04aca379ed33d6642d2642eda09431` |
| Skill SHA-256 (both arms, both locations) | `96b07a6b89ae338f26d45fbfb31dd97d5b9c50efa39922103e8cb3e616807eaf` |

All four hashes were re-verified against the frozen Phase 9b.1 record before this phase started — no drift since 9b.1. No source changes exist between the Phase 9a and Phase 9b1 commits, so Arm B's binary is confirmed unchanged.

## 3. Binary / Skill / Harness Verification

Same freeze-and-verify discipline as 9b.1: every one of the **194 runs** re-checked binary SHA-256 before and after, and skill SHA-256 before each run.

**Result: 0/194 binary mismatches, 0/194 skill drifts.**

## 4. Normal-Routing Protocol

`AST_TOOL_CONTROLLED_SKILL` was **not set** for any run in this phase (confirmed by inspecting the harness environment at launch — `os.environ.pop(...)` before every run). The agent chose its own first action freely. Each run's trace was parsed for the actual Skill tool-call sequence (name, position, count), not assumed, and classified into categories A–E (§9).

## 5. Full Cohort

Used the existing 41-task suite unmodified (`evaluation/tasks/*.yaml`) — confirmed present and all 41 repository fixtures exist. Covers levels 1–5 plus `smoke-001`, and the capability categories `ast-tool.md` requires (search/find/callers/callees/references/ambiguity/relationship-ordering/recovery/structural-lookup/multi-file-edits/API-changes/distributed-workflows — levels 4–5 exercise the cross-layer/API/distributed cases).

**Phase 8 coverage check**: the suite already contains `level2-008`/`level3-008` (8a), `level2-004`/`level4-006`/`level1-002` (8b), and `level2-005` (8c, adapted from Phase 9b.1) — no additional probe task was needed.

**Guard cohort carried forward** per Phase 9b.1's correction: `level1-001`, `level1-003`, `level2-001`, `level3-001`. `level1-002` is **not** used as a guard (confirmed Phase-8b-affected).

## 6. Repetition / Ordering Protocol

Two stages:

- **Stage 1** — one fresh run per arm per task, all 41 tasks, interleaved by task-index parity (`[A,B]` on even index, `[B,A]` on odd) — **82 runs**.
- **Stage 2** — 4 additional repeats per arm (reaching 5 total) on a 14-task selected cohort, chosen from Stage-1 evidence using the rule in `ast-tool.md` ("Phase 8 exercised, or routing differs, or a regression appears"), interleaved by task+repeat parity — **112 runs**.

Selected Stage-2 cohort and why:

| Task | Reason selected |
|---|---|
| `level2-008`, `level3-008`, `level2-004`, `level4-006`, `level1-002`, `level2-005` | Known Phase-8-affected (carried forward from 9b.1) |
| `level2-006`, `level3-002`, `smoke-001` | Stage-1 showed a different Skill-invocation category between arms |
| `level4-005` | Stage-1 showed a success flip (Arm A succeeded, Arm B failed validation) |
| `level1-001`, `level1-003`, `level2-001`, `level3-001` | Guard cohort — one run each is not enough evidence for "no systematic regression" |

Repeats were applied **symmetrically to both arms** — not only to unfavorable Arm B tasks, per the protocol's explicit requirement.

Total: **194 runs**, 0 exclusions, 0 infrastructure failures, repository reset (`git reset --hard` + `git clean -fdx`) before and after every run, fresh `claude -p` process every time (no session/context reuse).

## 7. Whole-Suite Aggregate Comparison (Table 1)

Single-pass, one-run-per-task basis (Stage 1, 41 runs/arm — the "main full-suite comparison" per protocol):

| Metric | Arm A | Arm B | Δ (B−A) | Assessment |
|---|---|---|---|---|
| Success rate | 92.7% (38/41) | 90.2% (37/41) | −2.4pp | See §8 — fully explained by one inherently-flaky task, not Phase 8 |
| Total tools (mean) | 10.78 | 9.39 | **−1.39** | Improved |
| Skill calls (mean) | 0.049 | 0.122 | +0.073 | Arm B invokes *some* Skill 2.5× more often |
| `semantic-analysis` invocation rate | 4.9% (2/41) | 12.2% (5/41) | +7.3pp | Arm B self-selects into the semantic route more often |
| AST-tool calls (mean) | 0.39 | 0.61 | +0.22 | Consistent with higher invocation |
| AST-tool failures (mean) | 0.073 | 0.049 | **−0.024** | Improved |
| AST-tool retries (mean) | 0.122 | 0.049 | **−0.073** | Improved (more than halved) |
| `--help` calls (mean) | 0.122 | 0.024 | **−0.098** | Improved (opposite of the forced-routing `level3-008` finding — see §22) |
| Read (mean) | 5.098 | 3.707 | **−1.39** | Improved |
| Grep (mean) | 1.707 | 1.634 | −0.073 | ~flat |
| Glob (mean) | 0.195 | 0.317 | +0.122 | ~flat |
| Bash (mean) | 1.537 | 1.659 | +0.122 | ~flat |
| Edit (mean) | 1.976 | 1.951 | −0.024 | ~flat |
| Tokens (mean) | 6020.4 | 5737.0 | **−283.4** | Improved |
| Elapsed seconds (mean) | 30.44 | 26.26 | **−4.18** | Improved |

AST command mix, Arm A: `search`×4, `callers`×3, `references`×6, `find`×3 (16 total across 41 runs). Arm B: `search`×6, `callers`×6, `references`×5, `find`×2, `callees`×1, `cache`×3, plus 2 malformed/`--help` artifacts from the `smoke-001` trace (§24) — 25 total. Arm B both invokes AST tooling more and fails/retries less per invocation.

## 8. Task-Level Comparison (Table 2, 14-task repeated cohort, n=5/arm each)

| Task | Success A/B | Δtools | ΔAST | Δfail | Δretry | ΔRead | Δtokens | Δelapsed | Assessment |
|---|---|---|---|---|---|---|---|---|---|
| level2-008 | 100/100 | +0.2 | 0.0 | 0.0 | 0.0 | −0.2 | +20.8 | +0.54s | Neutral — AST tool never invoked by either arm (0 `cmd_invoked_runs`, §13) |
| level3-008 | 100/100 | **−2.4** | 0.0 | 0.0 | 0.0 | −1.4 | **−2483** | **−5.62s** | Improved — and the forced-routing regression from 9b.1 does **not** reproduce here (§22) |
| level2-004 | 100/100 | **−4.4** | **−1.4** | −0.2 | **−0.4** | **−2.0** | **−4710** | **−17.93s** | Strongly improved — clean Phase 8b confirmation, invoked in 4/5 A and 5/5 B runs |
| level4-006 | 0/0 | **−5.2** | +0.6 | −0.2 | 0.0 | **−2.2** | **−3448.6** | **−26.9s** | Improved (validator defect unrelated, both arms 0%) |
| level1-002 | 100/100 | +0.6 | +1.2 | 0.0 | 0.0 | −0.4 | −354.2 | +0.04s | Mixed/small — target query itself never empty-tested (§13) |
| level2-005 | 100/100 | 0.0 | −1.0 | −0.2 | −0.6 | −0.2 | −18.0 | −1.4s | Small — `callees` almost never invoked under normal routing (§15) |
| level2-006 | 100/100 | +1.0 | +0.4 | 0.0 | 0.0 | −0.8 | +1436.2 | +3.62s | Invocation mismatch (§11) — Arm B's semantic route is correct but not cheaper here |
| level3-002 | 100/100 | +0.4 | −0.4 | 0.0 | 0.0 | +0.2 | +317.0 | −1.94s | Small, mixed |
| smoke-001 | 100/100 | +3.8 | +2.4 | **−2.2** | **−1.4** | +0.6 | +1865.8 | +7.87s | Mixed — fewer AST failures/retries in B, but higher overall cost (§24) |
| level4-005 | 20/20 | −2.2 | 0.0 | 0.0 | 0.0 | −2.0 | −1141.4 | −1.73s | Both arms equally unreliable (§17) — not a Phase 8 effect |
| level1-001 (guard) | 100/100 | −0.2 | 0.0 | 0.0 | 0.0 | −0.2 | −25.2 | −1.55s | Stable |
| level1-003 (guard) | 100/100 | +0.2 | 0.0 | 0.0 | 0.0 | +0.2 | +61.2 | +0.87s | Stable |
| level2-001 (guard) | 100/100 | +0.4 | 0.0 | 0.0 | 0.0 | +0.4 | +93.6 | −0.93s | Stable |
| level3-001 (guard) | 100/100 | −0.6 | +0.2 | 0.0 | 0.0 | −0.2 | −484.6 | −2.9s | Stable |

## 9. Skill Invocation Analysis

Full 194-run corpus, routing category counts:

| Category | Arm A | Arm B |
|---|---|---|
| A — `semantic-analysis` first | 14 | 14 |
| B — another Skill first | 0 | 2 (`api-review`, on `level2-004`/`level4-006`) |
| D — direct AST first | 4 | 4 |
| E — manual/Grep/Glob/Read first | 79 | 77 |

`semantic-analysis` invocation rate: **14/97 (14.4%) both arms** in the full weighted corpus (the repeat cohort intentionally over-samples Phase-8-relevant tasks, balancing the raw Stage-1 imbalance of 4.9%/12.2%). Mean first-action position when invoked: **1** for both arms (i.e. whenever it is invoked, it's the true first action — the "late invocation" failure mode from earlier phases does not appear here).

**Headline finding: `semantic-analysis` is invoked in a small minority of runs under normal routing (roughly 5–15% depending on cohort weighting), and the *majority* route (categories D+E ≈ 85%) is manual exploration or direct AST-tool use without the Skill at all.** This dominates whole-suite interpretation exactly as `ast-tool.md` warned it might, and is discussed further in §11–§12.

## 10. Same-Loaded Cohort

Per-task `semantic-analysis` invocation rate across all available repeats (`a_rate`/`b_rate`, from `evaluation/phase9b2/summary.json`):

| Task | Arm A rate | Arm B rate |
|---|---|---|
| level4-006 | 1.0 (5/5) | 0.8 (4/5) |
| level2-004 | 0.8 (4/5) | 0.8 (4/5) |
| level3-002 | 0.4 (2/5) | 0.2 (1/5) |
| smoke-001 | 0.2 (1/5) | 0.2 (1/5) |
| level2-005 | 0.2 (1/5) | 0.0 |
| level3-008 | 0.2 (1/5) | 0.0 |
| level1-002 | 0.0 | 0.4 (2/5) |
| level2-006 | 0.0 | 0.2 (1/5) |
| level3-001 | 0.0 | 0.2 (1/5) |
| all other 32 tasks (single-run or 5-run) | 0.0 | 0.0 |

**True same-loaded cohort** (invoked in *both* arms at least once): `level2-004`, `level4-006`, `level3-002`, `smoke-001`. Of these, `level2-004` and `level4-006` are also the strongest Phase 8b signal (§8) — the two tasks where the Skill loads reliably in both arms are exactly the two tasks where Phase 8's benefit is clearest and largest. This is the single strongest piece of evidence in this phase: **when the playing field is actually level (Skill loaded both sides), Phase 8 wins decisively** (−4.4 to −5.2 tools, −3.4k to −4.7k tokens, −18s to −27s).

## 11. Invocation-Mismatch Cohort

Tasks where one arm invoked `semantic-analysis` and the other did not: `level2-005`, `level3-008` (only A), `level1-002`, `level2-006`, `level3-001` (only B).

For each, attributing the cost difference:

- **level2-006** (only B): Arm B's route is semantically *correct* (`search`→`callers` in 2 AST calls returns the exact caller) but the run's *total* cost is still higher than Arm A's pure-Grep route (+1.0 tools, +1436 tokens) — the extra cost comes from more `Read`/`Bash` verification steps *after* the correct semantic answer, not from the semantic route itself. Attribution: **ordinary generation variance**, not a Phase 8 semantic cost.
- **level1-002** (only B): Arm B's extra AST call (+1.2 mean) is offset by fewer manual calls elsewhere; net token cost is *lower* in B (−354). Attribution: **Phase 8 semantics, mildly positive**.
- **level3-001** (only B, 1/5 runs): guard task — no measurable cost or correctness difference (§16). Attribution: **noise**.
- **level2-005 / level3-008** (only A, rare): Arm A's occasional Skill use reflects the *pre-Phase-8* skill guidance still being followed defensively; cost difference is small either way (§8). Attribution: **routing variance, not semantics**.

No invocation-mismatch case in this cohort shows a large cost swing attributable to *another Skill* or to manual exploration replacing a *needed* semantic step — mismatches here are cheap in both directions.

## 12. No-Skill / Direct-AST Cohort

79/97 (A) and 77/97 (B) runs never invoke `semantic-analysis`. Of those:

- **Direct AST without Skill** (category D): 4 runs each arm — all on `smoke-001`, where the agent invokes `ast-tool` directly via Bash without the Skill wrapper (see §24 trajectory).
- **Pure manual** (category E, Grep/Glob/Read only): the remaining ~75 runs/arm — this is levels 1, most of 2/3, and nearly all of level 4/5.

Representative no-Skill trajectory (`level5-002`, §24): both arms solve the task with zero AST-tool involvement; Arm B (17 tools) happens to be cheaper than Arm A (31 tools, including one `Agent`/subagent detour) in this specific run — a difference fully explained by ordinary LLM stochastic variance, since neither binary was ever invoked. **Phase 8 is correctly inert, not "irrelevant," on this cohort** — its binary-level fixes cannot matter when the binary is never called.

## 13. Phase 8a Normal-Routing Evidence

| Task | Arm A: `callers` invoked on target / occurrences | Arm B: invoked / occurrences |
|---|---|---|
| level2-008 | 0/5 invoked | 0/5 invoked |
| level3-008 | 0/5 invoked | 0/5 invoked |

**Under normal routing, the specific partial-FQN query neither arm ever exercises the target relationship command on these tasks' canonical symbols at all** — both arms solve them with Grep/manual routes in all 10 runs. Per the protocol's explicit guidance ("a low occurrence rate in both arms is acceptable... do not treat lack of exposure as failure of Phase 8a"), this is recorded as **no exposure**, not a failure signal. The underlying fix remains validated by Phase 9b.1's direct probes and forced-routing agent evidence; Phase 9b.2 simply cannot add normal-routing agent evidence for 8a because the route is never taken.

## 14. Phase 8b Normal-Routing Evidence

| Task | Arm A occurrences (of invoked runs) | Arm B occurrences (of invoked runs) |
|---|---|---|
| level2-004 | 4/4 (100% of the runs that invoked `callers`) | 0/5 |
| level4-006 | 5/5 | 0/4 |
| level1-002 | 0/0 (command never invoked by either arm) | 0/0 |

**This is the strongest and cleanest confirmation in the whole phase.** Every single time Arm A actually calls the affected relationship command on its known-affected target under normal routing, it hits the empty-result bug (9/9 across both tasks); every single time Arm B calls the same command, it gets a populated, correct result (0/9 empty across 9 invocations). Fallback behavior matches Phase 9b.1 exactly: Arm A's empty `callers` is followed by a `references` retry (also empty) and a `Grep` fallback (§8 trajectory, `level2-004`).

This directly satisfies the phase's acceptance criterion #3 ("At least Phase 8b's false-empty relationship elimination should remain observable under normal routing") and confirms the document's own prediction that **"Phase 8b is expected to be the strongest Phase 8 contributor at whole-agent level."**

## 15. Phase 8c Normal-Routing Evidence

`level2-005`'s target `callees auth::AuthService::refresh` was invoked in only **1 of 10 runs** (Arm A, repeat 3) — Arm B never invoked it. That one Arm A invocation hit the known false-empty result. With only one data point, **no normal-routing agent-level comparison is possible for Phase 8c in this phase.** This matches the protocol's expectation ("8c may be less frequently exercised depending on routing") and is recorded as **insufficient exposure**, not a negative result — Phase 9b.1's forced-routing evidence (5/5 vs 0/5) remains the operative evidence for Phase 8c.

**Residual gap tracking** (carried forward from 9b.1, not to be fixed here): across all 10 `level2-005` runs, `UserRepository::update` was never surfaced via `callees` in either arm (the one Arm A `callees` call that did fire returned the false-empty result, so the gap wasn't even reachable). No new evidence on the residual gap this phase; it remains a documented, unaddressed limitation for a future phase, not observed to worsen or improve.

## 16. Semantic Precision Analysis

Every populated relationship result observed across all 194 traces (whenever `callers`/`callees`/`references` returned non-empty on a Phase-8-affected target) matched the known-correct answer set established in Phase 9b.1 (e.g. `level2-004`'s Arm B `callers` consistently returns exactly `auth::AuthService::login` + the two `AuthController` handlers, never anything else). **No false/unexpected relationships were observed in any run.** This is not an exhaustive re-verification of every AST call in the corpus (most runs never call AST tooling at all — see §12), but every AST-tool call that *was* made was spot-checked against the Phase 9b.1 ground truth where the target symbol matched.

## 17. Recovery Analysis

Whole-suite (Stage 1, §7): AST failures −0.024/run, retries −0.073/run, `--help` −0.098/run, all favoring Arm B.

Known recovery signatures (§13–§15): Phase 8b's `empty→references-fallback→Grep-fallback` pattern is fully eliminated in Arm B wherever exercised (9/9 Arm A occurrences vs 0/9 Arm B). Phase 8a and 8c signatures are not exercised in either arm under normal routing (see above) — recorded as no-signal, not resolved-signal.

`level4-005` (§8, §17-outliers): both arms show a *recurring, identical* compilation-error signature (`member initializer 'processor_' does not name...`) in 8/10 runs regardless of arm — this is a **model-level code-editing recovery gap** (the agent doesn't self-correct a member-initializer-order mistake across retries within a single run), unrelated to AST-tool/Phase 8 at all, since neither arm invokes AST tooling on this task.

## 18. Manual Exploration Analysis

Whole-suite (§7): Read −1.39/run, Grep −0.07/run, Glob +0.12/run — Arm B shows meaningfully less `Read` verification and roughly flat `Grep`/`Glob`. No semantic→manual "collapse" pattern (a broad shift toward more manual tools in Arm B) was found anywhere; if anything the reverse.

Same-loaded cohort (`level2-004`, `level4-006`): Read −2.0 and −2.2 respectively — the clearest manual-exploration reduction, matching Phase 9b.1's forced-routing finding that Phase 8b eliminates the "verify the empty relationship manually" step.

## 19. Tool/Token/Time Distributions

Repeated-task cohort (n=5/arm), selected distributions:

- **`level2-004` tokens**: Arm A driven up by 1–2 high-cost runs with Grep fallback (mean 13,477.8, substantially above the Phase 9b.1 forced-routing mean of ~15,993 for a smaller n — consistent direction); Arm B tightly clustered around 8,000–9,500.
- **`smoke-001` tokens**: Arm A mean 10,354.6, Arm B mean 12,220.4 — Arm B *higher*, driven by extra CLI-ergonomics trial-and-error on this trivial fixture (§24), not semantic cost.
- **Whole-suite token delta is modest** (−283/run, −4.7%) precisely because only ~15% of runs ever reach the AST tool at all; the large per-task deltas on `level2-004`/`level4-006` are diluted by ~35 tasks with near-zero AST involvement.

Top single-run token deltas across all 41 tasks (n=1 for non-repeated tasks, flagged as weak evidence): largest Arm-B improvements were `level5-007` (−13,862) and `level5-004` (−6,680); largest Arm-B regressions were `level3-007` (+3,875) and `level5-005` (+3,446). All four are single-run observations on tasks that never invoke AST tooling in the sampled run — i.e., **ordinary generation variance, not Phase 8 effects** — and are reported for completeness (§18 of `ast-tool.md`'s distribution requirement) without further interpretation, per the evidence-standard rule against drawing conclusions from single stochastic runs.

p75/p90 are not reported for the n=5 cells (too small to be meaningful, consistent with Phase 9b.1's approach).

## 20. Guard-Task Analysis

`level1-001`, `level1-003`, `level2-001`, `level3-001`, 5 runs/arm each:

| Guard | Success A/B | AST failures A/B | AST retries A/B | Semantic result changed? |
|---|---|---|---|---|
| level1-001 | 100%/100% | 0/0 | 0/0 | No — AST tool never invoked by either arm |
| level1-003 | 100%/100% | 0/0 | 0/0 | No — AST tool never invoked by either arm |
| level2-001 | 100%/100% | 0/0 | 0/0 | No — AST tool never invoked by either arm |
| level3-001 | 100%/100% | 0/0 | 0/0 (1/5 Arm B runs invoke `semantic-analysis`, clean populated result) | No |

**All four guards are fully stable under normal routing.** Under normal routing they are solved almost entirely by manual exploration in this cohort (matching the whole-suite pattern), so this phase adds confirmation-by-non-regression rather than new positive semantic evidence — consistent with, and no weaker than, Phase 9b.1's forced-routing guard results.

## 21. Windows Path-Artifact Analysis

Scanned all 194 traces for the malformed-path signature (`error: workspace at '<drive>:<path-with-lost-separators>'`).

**2 occurrences found**, both Arm A: `level2-004` repeat 5 (`search`), `level2-005` repeat 3 (`search`). Both recovered within the same run (the agent retried with a corrected path). At 2/194 (1.0%), this is **far below the threshold that would materially distort the normal-routing comparison** — no interpretation-halting action needed, consistent with the "not frequent enough" branch of the protocol's guidance. Recommend a harness-level look (§25) but do not block on it.

## 22. `level3-008` Follow-Up

Phase 9b.1's forced-routing run showed `level3-008` costing *more* in Arm B (+1.2 tools, +1134 tokens, +12.3s), driven by extra `Read` calls and an `ast-tool --help` invocation.

**Under normal routing, this does not reproduce.** `level3-008`'s 5-repeat normal-routing result (§8): Arm B is *cheaper* (−2.4 tools, −2483 tokens, −5.62s), and neither arm invokes the AST tool on this task's target relationship at all (§13) — the whole comparison here is between two different *manual* exploration strategies, not a Phase 8 effect in either direction. The forced-routing regression appears specific to the forced-routing system prompt's defensive workflow interacting with this particular task, not a systematic Arm B cost problem — it does not carry over to normal routing and is not confirmed as a repeated pattern.

## 23. Residual Semantic-Gap Observations

See §15. The `callees`-omits-a-declaration/definition-pair-callee gap discovered in Phase 9b.1 was **not exercised** under normal routing in this phase (only 1 relevant `callees` call in 10 `level2-005` runs, and it hit the *upstream* false-empty bug before ever reaching the point where the residual gap would matter). No new evidence for or against it this phase. It is **not** repeatedly affecting multiple tasks under normal routing (by definition of near-zero exposure) — per the protocol, it is documented but does not currently rise to a Phase 10 trigger on its own.

## 24. Representative Trajectories

**1. Clear Phase 8 improvement** (`level2-004`, repeat 2 — both arms self-selected `semantic-analysis`):
```
Arm A: Skill(semantic-analysis) → search → callers → EMPTY
       → references → EMPTY → Grep → callers → EMPTY (retry)
       → Read×5 → Edit×4 → Grep
Arm B: Skill(semantic-analysis) → search → callers → POPULATED (2 callers)
       → Read×3 → Edit×4
```

**2. Neutral task** (`level2-008`, repeated 5×): AST tool never invoked by either arm in any of the 10 runs; both solve it with near-identical manual routes, Δtools = +0.2, Δtokens = +20.8 — noise-level.

**3. Invocation mismatch** (`level2-006`, repeat 1):
```
Arm A: Grep×2 → Read×4 → Edit×4                              (10 tools, no Skill)
Arm B: Skill(semantic-analysis) → search → callers → POPULATED
       → Read×4 → Edit×4 → Bash                                (11 tools)
```
Arm B's semantic answer is correct and immediate, but the run is still marginally *more* expensive overall — extra post-hoc verification, not a semantic cost.

**4. No-Skill route** (`level5-002`, repeat 1): Arm A — 31 tools including a subagent (`Agent`) detour, all manual; Arm B — 17 tools, all manual. Neither arm ever calls `ast-tool`; the ~2× cost gap is pure generation variance.

**5. Largest Arm B regression with repeat backing** (`smoke-001`, repeat 1):
```
Arm A: find(malformed) → find → find → references(--help text) → references(ambiguous)
       → Read×2 → references → search×2 → callers → references(path glitch) → references
       → Bash → Read → Edit → Bash×3 → Read → Bash            (24 tools)
Arm B: Skill(semantic-analysis) → find(malformed) → Bash → find
       → references(empty) → Read → references(empty) → Read → Bash
       → ast-tool "2"(malformed) → references(path glitch '4') → references(empty)
       → callers(empty) → Bash → cache×2 → search → callees(empty) → Edit → Bash×2  (26 tools)
```
Both arms flail with CLI ergonomics on this trivial single-function fixture; Arm B's extra `cache`/malformed-arg detours drive its slightly higher cost. Low-signal (tiny fixture, both arms noisy).

**6. Largest Arm B improvement** (`level2-004` and `level4-006` aggregate, §8/§14) — see trajectory #1; `level4-006` follows the identical pattern with `references` added to Arm A's fallback chain.

## 25. Regressions and Outliers

1. **`level4-005`**: 20% success in *both* arms (5 repeats each) — not a Phase 8 effect. 8/10 runs across both arms hit the identical compilation error (`member initializer 'processor_' does not name...`), a recurring model-level editing mistake on this specific 3-layer parameter-propagation task, with zero AST-tool involvement in any run. Recommend flagging this task's difficulty/flakiness for the harness maintainers independent of this evaluation.
2. **`smoke-001`**: net cost regression in Arm B (+3.8 tools, +1866 tokens, +7.87s) despite *fewer* AST failures/retries (−2.2/−1.4) — both arms produce noisy CLI trial-and-error on a trivial fixture; low-signal given the fixture's simplicity.
3. **Windows path-quoting artifact**: 2/194 runs, both Arm A, recovered in-run (§21).
4. **The Phase 9b.1 forced-routing `level3-008` cost regression does not reproduce** under normal routing (§22) — resolved/non-issue.
5. No new false relationships, no wrong body identity, no ambiguity collapse anywhere in the corpus.

## 26. Experimental Limitations

1. **Low invocation rate limits normal-routing agent evidence for 8a and 8c** specifically (§13, §15) — this phase can only confirm 8b agent-level under normal routing with strong evidence; 8a/8c normal-routing agent evidence remains open pending either higher-invocation tasks or a larger cohort.
2. **27 of 41 tasks have only a single run per arm** (Stage 1 only) — task-level deltas for those are weak evidence per the protocol's own evidence standard; only the 14-task repeated cohort supports moderate/strong claims.
3. **Semantic precision checking (§16) is spot-check, not exhaustive** — same caveat as Phase 9b.1.
4. **`level2-005`'s residual-gap tracking has essentially no normal-routing data** (1 relevant call in 10 runs) — carried forward unresolved.
5. Single-session, single-day evaluation; day-to-day model/infrastructure variance is not captured.
6. The Stage-2 cohort selection (§6) was informed by Stage-1 results, which is appropriate per protocol ("repeat selected tasks where... a meaningful regression appears") but means the 14-task deep-dive is not a uniform random sample of the 41-task suite — it is deliberately enriched toward Phase-8-relevant and anomalous tasks.

## 27. Final Baseline Decision

### **PROMOTE PHASE 8 TO STABLE BASELINE**

Cross-referencing the acceptance criteria:

1. **Correctness preserved** — whole-suite success gap (92.7% vs 90.2%) is fully attributable to one task (`level4-005`) confirmed by 5-repeat data to be ~20%-reliable in *both* arms, with zero AST-tool involvement in any of the 10 runs. No Phase-8-attributable correctness regression exists anywhere in 194 runs.
2. **Semantic precision preserved** — zero false/unexpected relationships observed (§16).
3. **Known Phase 8 improvements remain visible** — Phase 8b's false-empty-relationship elimination is the cleanest result in the entire phase: 9/9 Arm A occurrences when exercised, 0/9 Arm B occurrences, exactly matching the criterion's minimum bar. 8a/8c are under-exercised under normal routing as anticipated, not contradicted.
4. **Recovery not systematically worse** — AST failures −0.024/run, retries −0.073/run whole-suite, both favoring Arm B; the one recurring failure pattern found (`level4-005`) is a model-editing issue orthogonal to AST tooling.
5. **Manual fallback not systematically worse** — Read −1.39/run whole-suite, no semantic→manual collapse pattern anywhere.
6. **Agent-level cost is acceptable — in the preferred band**: whole-suite tools −12.9%, tokens −4.7%, elapsed −13.7%, all favoring Arm B, with success held constant once the one flaky task is accounted for.
7. **Guard tasks remain stable** — all 4 guards, 5 repeats each, 100%/100% success, zero AST failures/retries in either arm.

No hard-stop condition was triggered. The clearest single piece of evidence: the two tasks where `semantic-analysis` reliably loads in **both** arms (`level2-004`, `level4-006`) are exactly the two tasks with the largest, most consistent Arm B wins (−4.4 to −5.2 tools, −3.4k to −4.7k tokens, −18s to −27s) — when the comparison is fair, Phase 8 wins decisively.

## 28. Recommended Next Phase

Do **not** automatically start another semantic phase. The evidence points to a different, higher-leverage priority:

**Primary recommendation — investigate `semantic-analysis` invocation rate under normal routing.** At 14.4% (weighted) to as low as 4.9% (Arm A, raw), the Skill is under-triggering relative to how often its own documented use cases ("who calls", "find references", "callers of") textually match these task prompts. This dominates whole-system value capture more than any remaining semantic defect — Phase 8's real, measured benefit (§10, §14) is only realized in the ~15% of runs that actually invoke it. This is not a "Phase 10" semantic-capability phase; it's a routing/trigger-matching investigation (Skill description, trigger phrases, or system-level routing heuristics).

Secondary, lower-priority candidates (carried forward, not elevated by this phase's evidence):
- Callee-side declaration/definition identity gap (§23) — real but essentially unexercised under normal routing (1 relevant call in 10 runs); revisit if invocation rate improves and the gap starts showing up more.
- Windows path-quoting artifact (§21) — rare (2/194); a quick harness-level look is reasonable but not urgent.
- `--help` overuse — **not** confirmed as a normal-routing problem (Arm B is actually lower, §7); the Phase 9b.1 forced-routing finding does not generalize (§22). No action needed.

---

*Raw data: `evaluation/phase9b2/manifest.json` (194-run integrity/routing log), `evaluation/phase9b2/summary.json` (aggregated metrics, routing cohorts, Phase 8 signal, path-artifact scan, residual-gap tracking), `evaluation/phase9b2/agent/<task>/r<repeat>/arm_<A|B>/{traces,analysis,sessions}` (full per-run traces and session logs).*
