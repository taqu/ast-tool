# Phase 9b.1 Report — Fresh Controlled Pre-Phase-8 vs Phase-8 Confirmation

## 1. Environment and Revisions

- Host branch: `phase9b1`
- Model: `claude-sonnet-5` (verified from nested session logs)
- Claude Code CLI version: `2.1.261`
- Harness: `evaluation/phase9b1_probe.py` (run orchestration) + `evaluation/phase9b1_analyze.py` (aggregation), both new for this phase
- Evaluation date: 2026-09-07

## 2. Arm A / Arm B Definitions

### Arm A — Fresh Pre-Phase-8 Baseline
- Git revision: `ccc1fbb650bf058aa11602134d4e4fa1795cb98e`
- Built fresh in an isolated `git worktree` (`../ast-tool-arm-a`), Release/x64, MSVC 19.44, from a clean configure
- Excludes Phase 8a (FQN-suffix resolution), Phase 8b (receiver-type member resolution), Phase 8c (declaration/definition body identity)

### Arm B — Accepted Phase 8 System
- Git revision: `3095337a5ff04c7de723c3a0853682c9cefdf90c` (current `phase9b1` branch HEAD; contains Phase 8a+8b+8c unchanged — the only tracked diff on this branch is `ast-tool.md`, the task file itself)
- Rebuilt in the main tree immediately before staging, confirming it matches the working binary

### Skill (identical for both arms)
- `skills/semantic-analysis/SKILL.md` — the accepted Phase 7d skill, SHA-256 `96b07a6b89ae338f26d45fbfb31dd97d5b9c50efa39922103e8cb3e616807eaf`

**Pre-run correction (measurement-only fix, applied before any measured run):** the *user-level* skill copy (`~/.claude/skills/semantic-analysis/SKILL.md`), which is what agent sessions actually resolve since their `cwd` is a fixture repository outside this project, had drifted to an older/shorter draft (SHA-256 `11c24ed4...`, different content entirely). It was synced to the accepted Phase 7d content before starting any agent run. This is a harness-correctness fix, not a semantic change, and was made prior to any measured run per the "fix measurement only" provision.

## 3. Binary / Skill / Harness Verification

| Artifact | SHA-256 |
|---|---|
| Arm A binary (`evaluation/binaries/arm_a/ast-tool.exe`) | `35d17a4297dda52e52a9699f2655fad744846f1ac7f74f4ed68bf9eb3db832a5` |
| Arm B binary (`evaluation/binaries/arm_b/ast-tool.exe`) | `95bbffe793526a88d358cdf2b0b22507fd04aca379ed33d6642d2642eda09431` |
| Skill (both locations) | `96b07a6b89ae338f26d45fbfb31dd97d5b9c50efa39922103e8cb3e616807eaf` |

Both binaries and the skill hash were frozen into `evaluation/phase9b1/manifest.json` before any agent run, and verified again before **and after every single one of the 84 agent runs** (`binary_stable` field). Result: **0/84 binary mismatches, 0/84 skill drifts.** No working-tree semantic changes occurred during measurement.

## 4. Controlled-Routing Verification

Routing was forced identically in both arms via `AST_TOOL_CONTROLLED_SKILL=1`, which appends a system-prompt instruction directing the agent to invoke the `semantic-analysis` Skill exactly once, as the first action. Every trace was parsed for the actual `Skill` tool-call position and count (not assumed).

**Result: 84/84 runs — Skill invoked exactly once, as the first tool action, with `skill=="semantic-analysis"`. 0 exclusions required.**

## 5. Cohort and Group Definitions

Direct-probe reconnaissance (§9) was used to *validate* group assignment before running any agent session — one originally-planned "guard" task turned out to be Phase-8b-affected and was reassigned; see below.

| Group | Tasks | Rationale |
|---|---|---|
| A — Phase 8a target resolution | `level2-008`, `level3-008` | Partial-FQN caller/reference resolution |
| B — Phase 8b receiver-type relationships | `level2-004`, `level4-006`, `level1-002` | Receiver-typed member relationship queries |
| C — Phase 8c body-identity | `level2-005` | `callees` on a declaration whose body is defined out-of-line (`auth::AuthService::refresh`) |
| D — Unaffected guards | `level1-001`, `level1-003`, `level2-001`, `level3-001` | Confirmed arm-invariant by direct probe before inclusion |

**Cohort correction:** `level1-002` was originally slated as a Group D guard (per analogy with Phase 9a). A direct probe (`references store::InventoryService::save`, full FQN) showed it fails on Arm A exactly like the Phase 8b pattern — it is not an unaffected guard. It was moved to Group B as bonus evidence, and `level1-003` (a `find`/`search`-only ambiguity task, confirmed arm-invariant by direct probe) was substituted into Group D. This is exactly the kind of correction fresh, controlled measurement is meant to catch versus reusing historical assumptions.

Total cohort: **10 tasks** (within the 9–12 recommendation).

## 6. Repetition and Arm-Order Protocol

- Affected tasks (Groups A/B/C, 6 tasks): 5 runs per arm = 60 runs
- Guard tasks (Group D, 4 tasks): 3 runs per arm = 24 runs
- **Total: 84 fresh agent sessions**, each a new `claude --dangerously-skip-permissions -p` process (no session/state reuse)
- Arm order was interleaved per task and per repeat: for task index `i` and repeat `r`, order = `[A,B]` if `(i+r)` even else `[B,A]`, alternating across the whole cohort rather than running all-A-then-all-B
- Repository state (`git reset --hard` + `git clean -fdx`) was reset before and after every single run

## 7. Fresh Aggregate Comparison (Table 1)

All 42 runs per arm (6 affected × 5 + ... — see note below; guards counted separately where relevant), whole-cohort aggregate:

| Metric | Arm A | Arm B | Δ (B−A) | Assessment |
|---|---|---|---|---|
| Success rate | 88.1% (37/42) | 88.1% (37/42) | 0 | Identical — the 5/42 failures on each side are all `level4-006` (known validator defect, both arms) |
| Total tool calls (mean) | 10.50 | 8.48 | **−2.02** | Improved |
| AST-tool calls (mean) | 3.24 | 2.67 | **−0.57** | Improved |
| AST-tool failures (mean) | 0.095 | 0.048 | **−0.048** | Improved |
| AST-tool retries (mean) | 0.119 | 0.071 | **−0.048** | Improved |
| Tokens (mean) | 5518.4 | 3984.9 | **−1533.5** | Improved |
| Elapsed seconds (mean) | 33.64 | 27.40 | **−6.25** | Improved |

Affected-tasks-only subset (Groups A+B+C, 30 runs/arm):

| Metric | Arm A | Arm B | Δ |
|---|---|---|---|
| Success rate | 83.3% | 83.3% | 0 |
| Total tools | 12.07 | 9.13 | **−2.93** |
| AST calls | 3.60 | 2.77 | **−0.83** |
| Tokens | 6686.5 | 4481.7 | **−2204.8** |
| Elapsed | 37.51 | 29.04 | **−8.47** |

Guard-tasks-only subset (Group D, 12 runs/arm):

| Metric | Arm A | Arm B | Δ |
|---|---|---|---|
| Success rate | 100% | 100% | 0 |
| Total tools | 6.58 | 6.83 | +0.25 |
| AST calls | 2.33 | 2.42 | +0.08 |
| AST failures | 0.25 | 0.083 | −0.17 |
| AST retries | 0.33 | 0 | −0.33 |
| Tokens | 2598.2 | 2742.8 | +144.7 |
| Elapsed | 23.97 | 23.29 | −0.69 |

Guard-task cost is essentially flat (small, non-systematic deltas within a 12-run sample — see §16).

## 8. Fresh Task-Level Comparison (Table 2)

| Task | Group | Runs A/B | Succ A/B | Δtools | ΔAST | Δfail | Δretry | ΔRead | Δtokens | Δelapsed | Assessment |
|---|---|---|---|---|---|---|---|---|---|---|---|
| level2-008 | A | 5/5 | 100/100 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | +56 | +1.3s | Neutral (skill's search-first guidance already avoids the failure on both arms for this task) |
| level3-008 | A | 5/5 | 100/100 | +1.2 | −0.2 | 0.0 | 0.0 | +0.6 | +1134 | +12.3s | **Regressed on cost** (see §17 — driven by extra manual `Read`/`--help` exploration, not AST failures) |
| level2-004 | B | 5/5 | 100/100 | **−6.2** | **−1.8** | 0.0 | 0.0 | **−2.4** | **−8043** | **−23.2s** | Strongly improved |
| level4-006 | B | 5/5 | 0/0 | **−6.2** | −0.6 | **−0.2** | **−0.2** | −1.4 | **−3073** | **−24.4s** | Improved (semantic + cost); validator defect unrelated |
| level1-002 | B | 5/5 | 100/100 | **−6.2** | **−1.6** | 0.0 | 0.0 | **−2.8** | **−3177** | **−17.8s** | Strongly improved |
| level2-005 | C | 5/5 | 100/100 | −0.2 | −0.8 | +0.2 | +0.6 | +0.2 | −125 | +0.9s | Modest/neutral (see §12 — residual gap on a different callee, both arms) |
| level1-001 | D | 3/3 | 100/100 | −1.0 | −0.33 | 0.0 | 0.0 | −0.33 | −388 | +0.6s | Stable guard |
| level1-003 | D | 3/3 | 100/100 | −0.33 | −0.33 | 0.0 | 0.0 | 0.0 | −115 | −7.0s | Stable guard |
| level2-001 | D | 3/3 | 100/100 | 0.0 | −0.33 | −0.67 | **−1.33** | +0.33 | +43 | −4.7s | Stable guard (Arm A failures were a Windows path-escaping artifact, not semantic — see §16) |
| level3-001 | D | 3/3 | 100/100 | +2.33 | +1.33 | 0.0 | 0.0 | +1.67 | +1039 | +8.4s | Stable guard, higher agent variance in Arm B (see §16) |

## 9. Semantic Accuracy (Table 3)

Direct CLI probes, 5 runs per probe per arm, both binaries invoked directly (no PATH swap needed):

| Group | Command | Query | Root | Missing (A) | Missing (B) | Unexpected (A/B) | Stable×5 |
|---|---|---|---|---|---|---|---|
| A | callers | `AuthToken::expire` | level2-auth | 2/2 | 0/2 | 0/0 | Yes |
| A | callers | `DataStore::save` | level3-pipeline | 1/1 | 0/1 | 0/0 | Yes |
| B | callers | `auth::AuthToken::validate` | level2-auth | 4/4 | 0/4 | 0/0 | Yes |
| C | callees | `auth::AuthService::refresh` | level2-auth | 3/3 | 0/3 | 0/0 | Yes |
| B (bonus) | references | `auth::AuthToken::validate` | level2-auth | 2/2 | 0/2 | 0/0 | Yes |
| B (bonus) | callees | `auth::AuthService::login` | level2-auth | 1/1 | 0/1 | 0/0 | Yes |
| B (bonus) | references | `store::InventoryService::save` (full FQN) | level1-store | 3/3 | 0/3 | 0/0 | Yes |
| D | callers | `auth::AuthToken::expire` (exact FQN) | level2-auth | 0/2 | 0/2 | 0/0 | Yes |
| D | callers | `service::CheckoutService::process` | level3-order | 0/2 | 0/2 | 0/0 | Yes |
| D | search | `--name update` | level2-auth | 0/4 | 0/4 | 0/0 | Yes |

Notable finding beyond the planned probe set: Phase 8b's receiver-type fix is **not limited to `callers`** — the bonus probes show the same missing→populated pattern on `references` and `callees` for receiver-typed member queries. This generalizes the Phase 8b confirmation beyond what Phase 9a tested.

`unexpected` counts are computed conservatively (fragment-containment against a documented expected set, per the same method Phase 9a used) — see §19 limitations.

## 10. Phase 8a Confirmation

Pattern: `relationship(partial/unresolved) → failure → search → relationship(exact) → success`, measured precisely against the task's known target symbol (regex-anchored to the exact `no <cmd> found for: <ns>::Sym` / `symbol not found: <ns>::Sym` message, not just any empty relationship result — this matters, see §17):

| Task | Arm A occurrences | Arm B occurrences |
|---|---|---|
| level2-008 | 0/5 | 0/5 |
| level3-008 | 1/5 | 0/5 |

**Interpretation:** fresh data shows a *much lower* Arm A occurrence rate than Phase 9a's historical 2/6 (~33%) estimate. Inspecting the traces shows why: the current, more detailed Phase 7d skill explicitly instructs "search first, then call the mapped command with the confirmed FQN" — this defensive workflow already sidesteps the partial-FQN bug in most runs on **both** arms, because the agent rarely calls `callers`/`references` with an unqualified name at all. Where it still surfaces (`level3-008`, 1/5 Arm A), Arm B is clean (0/5). Ambiguity guards are not weakened (§9, Group D probes 0 missing / 0 unexpected on both arms).

This is a genuine, fresh finding that **differs from Phase 9a's historical estimate** in magnitude (though not direction) — the direct-probe layer (§9) still proves the underlying binary-level bug is real and eliminated by Phase 8a; it is simply exercised less often at the agent level than the historical proxy data suggested, because skill quality has independently improved since then.

## 11. Phase 8b Confirmation

| Task | Arm A occurrences (empty canonical query) | Arm B occurrences |
|---|---|---|
| level2-004 | 5/5 | 0/5 |
| level4-006 | 4/5 | 0/5 |
| level1-002 (bonus) | 5/5 | 0/5 |

Fallback behavior on Arm A: in level2-004 and level1-002, an empty `callers` result is consistently followed by a `references` fallback attempt (also empty) and sometimes a repeated `callers` retry — extra AST calls that Arm B never needs (§8: −1.6 to −1.8 AST calls, −2.4 to −2.8 Read calls). This is exactly the Phase 8b signature: `previously empty relationship → populated exact relationship → fallback reduced/eliminated`, now confirmed with a full fresh Arm A sample (not the single historical run Phase 9a had for `level2-004`) and a fresh Arm A sample for `level4-006` (Phase 9a had none for this task under forced routing).

## 12. Phase 8c Confirmation

`callees auth::AuthService::refresh` (the target symbol behind `level2-005`):

| | Arm A occurrences | Arm B occurrences |
|---|---|---|
| `callees` returns "no callees found" | 5/5 | 0/5 |

Direct probe (§9): Arm A returns `note: no callees found`; Arm B returns 3 populated callees (`auth::AuthToken::expire`, `auth::TokenCache::invalidate`, `auth::AuthToken::refresh`), stable across 5/5 direct replays. Agent-level: **5/5 Arm A runs** hit the empty note and fall back to searching for the function's own definition and other class members; **0/5 Arm B runs** do.

**Residual gap discovered during this phase (out of scope for Phase 8c acceptance, flagged for future work):** `AuthService::refresh` actually calls `repo_.update(u)` (`auth_service.cpp:22`), a `UserRepository::update` call — but `callees` on **both** arms omits it. `UserRepository::update` has a declaration/definition pair (`user_repository.h`/`.cpp`) — i.e., the *callee* itself is a declaration/definition pair, the mirror image of the case Phase 8c fixed (where the *query target* was a declaration/definition pair). This is why the `level2-005` task — which specifically asks for "the user-update operation that refresh calls" — needs an extra `search` for `UserRepository::update` in **both** arms, even in Arm B. Net effect: Phase 8c's fix is real and clearly demonstrated (`callees` goes from totally empty to 3/4 correct results), but level2-005's task-level cost delta is small (−0.8 AST calls, −125 tokens, +0.9s — noise-level, not the large win seen in Groups A/B) because the task's specific answer sits behind a second, unrelated gap that neither arm resolves via `callees` alone.

Per ast-tool.md's two acceptable outcomes, this lands as: **correct partial semantic result → modest/neutral agent-level effect**, with the underlying `callees`-on-declaration-only-target defect unambiguously fixed and demonstrated at both the direct-probe and agent-trajectory level.

## 13. Recovery Analysis

| Pattern | Arm A occurrences | Arm B occurrences | AST calls saved (typical) |
|---|---|---|---|
| 8a: partial→fail→search→retry (level2-008+level3-008) | 1/10 | 0/10 | 2 per occurrence |
| 8b: empty→references/callers fallback (level2-004+level4-006+level1-002) | 14/15 | 0/15 | 1–2 per occurrence |
| 8c: callees→empty (level2-005) | 5/5 | 0/5 | direct-probe confirmed; agent-level cost effect modest (§12) |

Aggregate (§7): AST failures −0.048/run, AST retries −0.048/run across the whole cohort; on the affected-tasks subset alone the fallback elimination shows up clearly as −2.4 to −2.8 fewer `Read` calls and −6.2 fewer total tools on the three Group B tasks.

## 14. Manual Exploration Analysis

| Task | Arm A Read (mean) | Arm B Read (mean) | Δ |
|---|---|---|---|
| level2-008 | 1.0 | 1.0 | 0 |
| level3-008 | 3.6 | 4.2 | +0.6 |
| level2-004 | 5.4 | 3.0 | **−2.4** |
| level4-006 | 4.4 | 3.0 | −1.4 |
| level1-002 | 4.8 | 2.0 | **−2.8** |
| level2-005 | 2.0 | 2.2 | +0.2 |

Group B shows the clearest reduction in manual exploration (Arm A's empty relationship results push the agent toward more `Read` calls to manually verify call sites; Arm B's populated results remove that need). Group A and C show small increases in Arm B `Read` usage that track with agent-level variance rather than semantic causes (§17).

## 15. Tool/Token/Time Distributions

Per-task min/median/mean/max (tokens, elapsed_seconds), n=5 for affected tasks, n=3 for guards — full per-run values are in `evaluation/phase9b1/summary.json`. Selected distributions (affected tasks, where the effect is largest):

**level2-004 tokens:** Arm A `[7409, 27691, 15993(mean), ...]` range roughly 7.4k–27.7k (one run drove the mean up via extra `Grep` fallback); Arm B tightly clustered 6.9k–9.2k. Arm B is both lower **and** less variable.

**level1-002 tokens:** Arm A range ~5.9k–9.8k; Arm B range ~3.3k–4.9k — consistent separation across all 5 runs, no overlap.

**level2-005 AST calls:** Arm A `[5,6,4,6,4]` (mean 5.0); Arm B `[4,5,3,5,4]` (mean 4.2) — consistent small reduction, no overlap in medians.

**level3-008 elapsed:** Arm A `[35.7, 23.1, 35.4, 31.0, 29.3]` (mean 30.9); Arm B `[25.5, 54.4, 38.4, 65.0, 32.5]` (mean 43.2, driven by two long outlier runs — see §17).

p75/p90 are not reported given n=3–5 per cell (too small to be meaningful, per protocol).

## 16. Unaffected Guard Results (Group D)

| Guard task | Arm A success | Arm B success | Semantic result changed? | New failures/retries? |
|---|---|---|---|---|
| level1-001 | 3/3 | 3/3 | No | No |
| level1-003 | 3/3 | 3/3 | No | No |
| level2-001 | 3/3 | 3/3 | No | 2/3 Arm A runs hit a **Windows path-escaping artifact** (`error: workspace at 'D:MyDocumentsProjects...'` — colons/backslashes stripped from the root argument), unrelated to Phase 8 semantics; Arm B 0/3. This drove level2-001's Δfail/Δretry in Table 2; the underlying `search --name update` query itself was direct-probe-confirmed identical (§9). |
| level3-001 | 3/3 | 3/3 | No | No AST failures/retries beyond one shared-across-arms `symbol not found` from an intentionally-incomplete overload search — Arm B used more exploratory `search`/`callees` calls in 2/3 runs (agent variance), not indicative of a semantic difference (`callers service::CheckoutService::process` direct probe: 0 missing, identical on both arms). |

No new false relationships, no new ambiguity, no systematic guard regression. The path-escaping artifact and the extra exploratory search calls are both explained by underlying trace inspection as agent-level variance / an environmental quoting issue, not semantic capability differences — confirmed by direct probes showing byte-identical output on the actual guarded queries.

**Phase 8 regression gate: PASSED**, with the caveat that guard direct-probe selection required one correction (§5) — a reminder that "already correct" status should be verified, not assumed.

## 17. Regressions and Outliers

1. **level3-008 cost regression (Arm B costs more)**: −0.2 AST calls but +1.2 tools, +1134 tokens, +12.3s. Trace inspection (§8, raw data) shows the extra cost in Arm B comes from **`ast-tool --help` invocations and extra `Read` calls** in 2/5 runs — not from AST failures or retries (both arms: 0 failures, 0 retries on this task). This is agent-level exploration variance in a 5-run sample, not a Phase 8a semantic regression; it directly contradicts Phase 9a's historical claim of improvement for this specific task and should be read as a genuine fresh finding rather than smoothed over.
2. **level2-005 one path-quoting failure (Arm B)**: 1/5 runs hit a Windows path-escaping `search` error (`error: workspace at 'D:MyDocumentsProj...'`), recovered in the same run — same class of artifact as the level2-001 guard finding, unrelated to Phase 8c semantics.
3. **level4-006**: 0% validation success on both arms, all 10 runs — pre-existing fixture defect (documented since Phase 9a): the semantic calls succeed and are more accurate on Arm B, but the validator's expected diff doesn't match what the task, as currently specified, produces on either arm.
4. No new false relationships were observed anywhere in the cohort (§9, §11, §16).

## 18. Experimental Limitations

1. **Windows path-escaping artifacts**: 3 of 84 runs (2 in `level2-001` Arm A, 1 in `level2-005` Arm B) hit a Bash/Windows path-quoting failure unrelated to either arm's semantic behavior. This is a harness/environment characteristic, not corrected mid-run (would have required modifying agent behavior), and is noted wherever it affects a task's failure/retry counts.
2. **`unexpected` relationship checking in Table 3** is fragment-containment based (same method as Phase 9a), not an exhaustive cross-check against every possible relationship — a conservative, not exhaustive, false-positive gate.
3. **Causal-pattern counting (§10–§12)** is regex-anchored to the specific known-affected target symbol per task to avoid false positives from unrelated exploratory queries (an earlier, looser detector overcounted by ~3–5× before this correction — documented for transparency, not just the final numbers).
4. **A new residual semantic gap was discovered** (§12: `callees` omitting a call to a declaration/definition-pair callee) that is out of Phase 9b.1's scope to fix, but affects interpretation of `level2-005`'s task-level cost delta.
5. **Small per-cell sample sizes** (n=5 affected, n=3 guards) mean single-run outliers (§17.1, §17.2) move task-level means noticeably; the direct-probe layer (§9, n=5 deterministic replays each) is the higher-confidence evidence for underlying semantic correctness, while agent-level numbers should be read with those sample sizes in mind.
6. This is a single fresh interleaved session, not a multi-day repeat; day-to-day model or infrastructure variance is not captured.

## 19. Final Decision

### **CONFIRM PHASE 8**

**Justification, cross-referencing the acceptance criteria:**

1. **Correctness preserved** — every task where Arm A succeeded, Arm B also succeeded; no new failures anywhere. `level4-006`'s validation failure is identical (and pre-existing) on both arms.
2. **Phase 8a effect reproduced** — direct probes show the exact partial-FQN pattern eliminated (2/2, 1/1 fragments fixed, 0 unexpected). Agent-level occurrence is now low on both arms (skill-quality improvement independently reduces exposure), but where it appears, Arm B is clean (0/5 vs 1/5).
3. **Phase 8b effect reproduced, and more strongly than Phase 9a**: a full fresh 5-run Arm A sample (not 1 historical run) shows 5/5, 4/5, and 5/5 occurrence rates across three tasks, all eliminated in Arm B (0/5 each), plus 3 bonus direct probes showing the fix generalizes beyond `callers` to `references` and `callees`.
4. **Phase 8c agent-level usefulness characterized** — this was Phase 9a's biggest gap. Fresh evidence: 5/5 Arm A runs hit the false-empty `callees` result; 0/5 Arm B runs do. The agent-level cost effect is modest/neutral rather than dramatic, because of a distinct, newly-discovered residual gap (§12) — both outcomes ("useful trajectory change" and "neutral agent-level effect") were anticipated as acceptable by the protocol, and this lands closer to neutral while the underlying semantic fix is unambiguous.
5. **Semantic precision preserved** — 0 unexpected relationships across all 10 direct-probe families and all agent traces inspected.
6. **Unaffected guards stable** — 4/4 guard tasks show 100% success on both arms, identical semantic results, and the two apparent guard "instabilities" (level2-001, level3-001) are fully explained by a Windows path-quoting artifact and ordinary agent variance, not semantic regression, verified against direct-probe ground truth.
7. **Recovery does not regress** — AST failures −0.048/run and retries −0.048/run cohort-wide; on the 3 Group B tasks specifically, fallback (extra `references`/`Read` calls) is reduced by 1.4–2.8 calls per run.
8. **Agent-level cost is acceptable — mostly in the "preferred" band**: cohort-wide tools −2.02, tokens −1533, elapsed −6.25s, all in Arm B's favor. The one exception (`level3-008`, cost up in Arm B, §17.1) is a genuine, disclosed fresh finding, not smoothed over, and is not accompanied by any failure/retry increase — i.e. even the regressed task shows no semantic degradation, only sample-level exploration noise.

No hard-stop condition was triggered: no false semantic relationships, no wrong body identity, no ambiguity collapse, no systematic correctness loss, no systematic guard regression.

## 20. Phase 9b.2 Recommendation

**Proceed to Phase 9b.2** (remove forced routing, evaluate under normal Skill invocation).

Carry forward:
- The `level3-008` cost regression should be watched under normal routing — if it reproduces there too, investigate whether it's `--help` overuse or excess `Read` verification specific to that task/repo, independent of Phase 8.
- The residual `callees`-omits-declaration/definition-pair-callee gap (§12) is a candidate for a future Phase 8d, but is out of scope here — flag it, do not fix it mid-phase.
- The Windows path-escaping artifact (§16, §18) is worth a harness-level look (how the agent quotes root paths in generated Bash commands) since it recurred independently in two different arms/tasks and could affect normal-routing runs too.
- Keep the corrected Group D cohort (`level1-001`, `level1-003`, `level2-001`, `level3-001`) — `level1-002` is confirmed Phase-8b-affected, not a guard.

---

*Raw data: `evaluation/phase9b1/manifest.json` (integrity/routing), `evaluation/phase9b1/direct_probes.json` (semantic probes), `evaluation/phase9b1/summary.json` (aggregated metrics), `evaluation/phase9b1/agent/<task>/r<repeat>/arm_<A|B>/{traces,analysis,sessions}` (84 full per-run traces and session logs).*
