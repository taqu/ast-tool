# Phase 6 Regression Analysis

## 1. Executive Conclusion

The measured "Phase 6 regression" is dominated by an unrelated environment defect, not by the CLI help reorganization.

All 16 failures in the evaluation run (100%) are `level4-*`/`level5-*` tasks that fail at the exact same validation step: a `clang++ -fsyntax-only` compile-check against the MSVC STL, which now requires Clang ≥19.0.0 while the installed toolchain is Clang 18.1.1. In every failing task, the validation script's content checks (correct edit present, no leakage into the wrong file, etc.) all pass — the script only fails on the trailing compile-check line. This is a pure toolchain-version-skew artifact of the local machine, unrelated to `ast-tool`, `help.cpp`, or `Skill.md`.

There is a second, real, separate phenomenon in the same run: `ast-tool` was genuinely invoked in only 2 of 41 tasks (`level4-006`, `smoke-001`). That is a real behavioral collapse worth explaining — but the evidence does not support "Skill.md vs. Phase 6 help conflict" as its cause. The `skills/*/SKILL.md` files are byte-identical to the Phase 5 baseline, and `ast-tool --help` was read in only 2 of 41 trajectories — the reorganized help text is essentially never seen, so it cannot be confusing the agent in the other 39.

## 2. Evidence

**A. The compile-gate is an environment defect, not a code defect.**

```
$ clang++ --version
clang version 18.1.1 ... InstalledDir: D:\Programs\LLVM\bin
```

Every failing `validate_*.py`'s stdout is literally:

```
FAIL: Compilation error
... error STL1000: Unexpected compiler version, expected Clang 19.0.0 or newer.
```

for all 16 failures (`level4-001..008`, `level5-001..008`), zero exceptions. `level5-002`'s trace shows the agent discovering this live: `g++` isn't installed, `clang++` fails on the STL guard, and only succeeds once `-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH` is added manually — a flag the validate scripts don't pass.

`level4-commerce/validate_001.py` (used by `level4-001`) runs its content checks first (audit-log-present, no-admin-leakage) and only then a `clang++ -fsyntax-only` compile step. Since the printed failure is only "Compilation error" (not the earlier content-check messages), the earlier checks passed — i.e. the agent's edit was substantively correct.

**B. `Skill.md` (skills/*/SKILL.md) is unmodified.**

```
$ git diff --stat -- skills/
(empty)
```

The Phase 6 diff touches exactly `src/help.cpp` (plus test scaffolding with no runtime effect). Nothing that shapes `SKILL.md` routing changed.

**C. `ast-tool --help` is almost never read.**

```
$ grep -l "\-\-help\|ast-tool help" evaluation/results/traces/*.jsonl
level4-005.jsonl
smoke-001.jsonl
```

Only 2 of 41 trajectories touch help output at all. 39 trajectories never see the Phase 6 category headers, so they cannot be the thing steering those 39 away from `ast-tool`.

**D. Real `ast-tool` usage, reconstructed directly from trace text (not `logs.py`'s parser):**

```
level4-006: search, search, callers, references
smoke-001:  ast-tool --help, find --help, find, references --help, references,
            references ::greet, search, search --help, callers --help,
            references+callers (chained), search --fqn --json --pretty
(everything else: 0 ast-tool invocations)
```

This reconciles exactly with the documented Phase 6 per-command table (search 4 / callers 2 / references 5 / find 3 / "2" 1) once accounting for `logs.py`'s regex only catching the first `ast-tool <word>` match per Bash call. `ast-tool` was attempted in 2 of 41 tasks, full stop — not "reduced usage across many tasks," but near-total absence in 39/41.

**E. The `"2": 1` artifact is a parser bug, confirmed harmless.**

`smoke-001` seq 1: `which ast-tool 2>/dev/null; ast-tool --help 2>&1 | head -30`. `logs.py`'s regex `\bast-tool\s+(\w+)` matches the first `ast-tool` occurrence — inside `which ast-tool 2>/dev/null` — capturing the stray `2` from `2>/dev/null` as a fake "subcommand." Cosmetic only; does not represent a real invocation and contributed nothing to the regression.

**F. The one real `ast-tool` → grep fallback (`level4-006`) was caused by a resolver false negative, not a discoverability problem.**

```
$ ast-tool callers service::ValidationService::validate src/
note: no callers found for: service::ValidationService::validate
$ ast-tool references service::ValidationService::validate src/
note: no references found for: service::ValidationService::validate
```

But `request_processor.cpp` does call `validator_.validate(...)` — a real call site the resolver missed (a member call through an instance variable). The agent invoked the `semantic-analysis` Skill correctly, ran `search`/`callers`/`references` correctly, got a wrong empty answer, and then fell back to Grep — exactly the Skill.md-sanctioned behavior ("if `callers` fails twice, Grep fallback"), just triggered by a tool correctness gap rather than a UX one. This resolver behavior is unchanged code from Phase 5 (Phase 6 touched only `help.cpp`), so it is not something Phase 6 introduced — but it is real and worth tracking separately.

## 3. Skill.md vs. Help comparison

| Intent | Skill.md recommendation | Phase 6 help implication | Conflict/ambiguity |
|---|---|---|---|
| Locate a symbol | `search` (semantic-analysis: "discover matching symbols" → `search`) | `search`, first item under **Primary** | None — aligned |
| Find callers | `callers` | `callers`, 2nd under **Primary** | None — aligned |
| Find references | `references` | `references`, 3rd under **Primary** | None — aligned |
| Find callees | `callees` | `callees`, 4th under **Primary** | None — aligned |
| Inspect AST structure in a known file | semantic-analysis: `find` for "locate node by text/type/position"; ast-inspection: `outline`/`parent`/`children` as 4 co-equal commands | `find` = **Primary**, `outline` = **Secondary**, `parent`/`children` = **Debug/Low-level** | Minor framing mismatch: ast-inspection's SKILL.md presents its 4 commands as peers; the CLI help now splits them across 3 prominence tiers. Not a contradicted recommendation, just inconsistent salience — and low-impact since help is rarely read (evidence C) |
| Inspect symbols in a file | ast-inspection: `symbols` | `symbols`, listed last under **Primary** | None — both treat it as important; only ordering differs, not a conflict |

No literal or semantic contradiction was found between `Skill.md` routing and the Phase 6 help categorization — they agree on which commands are "the important ones." The one soft inconsistency (ast-inspection's flat command list vs. the CLI's now-tiered grouping) is real but was reachable by only 2/41 trajectories, so it cannot explain a 39/41 absence.

## 4. Failure trajectory analysis

The literal `grep → read → grep → read → …` pattern is real (e.g. `level4-001` through `level4-005`, `level5-001`), but it starts from sequence 1 — there is no visible attempt, hesitation, or `--help` call preceding it. The agent simply never considers `ast-tool` for the vast majority of tasks; this is not "tries `ast-tool`, gets confused by ambiguous help, retreats to grep" (the hypothesized pattern) — for 38 of the 39 non-`ast-tool` trajectories, `ast-tool` and its Skills are never referenced at all, help or otherwise. The one trajectory that does show the classic pattern (`ast-tool` → grep) is `level4-006`, and there the cause is a resolver false negative (section 2F), not help-text ambiguity.

Where the trajectories diverge from an expected `search → callers/references → edit` path, the actual divergence is "no `ast-tool` step ever appears" — a discoverability gap, but one that the trace evidence (rare `--help` reads, unchanged `SKILL.md`) cannot pin on the Phase 6 diff specifically. It looks more like whatever governs the model's own decision to invoke the globally-installed Skills in this single-shot run.

## 5. Ranked root-cause hypotheses

### D. Evaluation/tooling artifact — high confidence, dominant for the success-rate metric

- **For:** 16/16 failures share the identical Clang/MSVC-STL version-skew signature; all upstream content checks pass first; the defect is externally verifiable (`clang++ --version` = 18.1.1, error demands ≥19.0.0); it mechanically explains the entire 37→25 success delta (level4+level5 = exactly 16 tasks, 0 failures elsewhere).
- **Against:** Doesn't explain the separate `ast-tool`-usage collapse (69→15 raw count, 39/41 tasks not touching it at all) — that's a second, independent phenomenon layered on top.
- **Confidence: high** for the success-rate portion specifically.

### E. Sampling/harness variance in Skill invocation — moderate confidence, best explanation for the usage-collapse metric

- **For:** `Skill.md`/`SKILL.md` files are byte-identical to baseline; `--help` is read in only 2/41 trajectories, ruling out help-text as the trigger for the other 39; Skill invocation appears to be a model-discretionary choice (not a hard keyword trigger) with no forcing function, so it's plausible this is single-shot stochastic variance rather than a systematic regression, especially since this run is one sample per task (no repetition to average out).
- **Against:** Can't be proven without Phase 5's raw traces for a true apples-to-apples comparison — no archived Phase 5 trace data exists to check whether Skill/`ast-tool` invocation was actually more frequent under identical conditions, or whether the Phase 5 numbers came from a different model/CLI version/averaging methodology.
- **Confidence: moderate** — plausible and consistent with available evidence, but not independently verified against a real baseline trace.

### C. Accidental command-behavior regression (resolver false negative) — low confidence as a Phase 6 cause, but real and worth separate tracking

- **For:** Confirmed concretely in `level4-006`: `callers`/`references` return empty for a real call site through a member-variable instance.
- **Against:** This is pre-existing resolver code, untouched by the Phase 6 diff (`help.cpp` only) — it isn't something Phase 6 introduced, and it only appears in 1 of 41 trajectories.
- **Confidence: low** as an explanation for the regression; separately, **high confidence** this specific resolver gap is real and independent of Phase 6.

### A/B. Skill.md/help inconsistency or discoverability/salience regression — low confidence

- **For:** The hypothesized mechanism is plausible in the abstract, and there is one minor cross-document framing inconsistency (section 3, AST-inspection tiering).
- **Against:** No literal contradiction found; the one inconsistency found is reachable by only 2/41 trajectories; `SKILL.md` and `help.cpp`'s Primary category agree on the exact same 6 commands; 39/41 trajectories never read help text at all, so text-level ambiguity in it cannot be steering their behavior.
- **Confidence: low**.

## 6. Minimal remediation recommendation

This was analysis only — no implementation changes were made. Recommended smallest next steps, in order:

1. **Fix the environment first, before touching any code.** Either update Clang to ≥19.0.0, or add `-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH` to the `level4`/`level5` validate scripts' compile-check invocations (whichever the project considers "supported"). This alone is expected to restore most of the lost 12 successes, independent of any Phase 6 CLI change — re-run the eval after this fix before concluding anything further about Phase 6 itself.
2. **Re-run the 41-task eval once more** after the environment fix, ideally with N>1 samples per task if feasible, before drawing conclusions about the `ast-tool`-usage collapse — a single non-repeated run cannot distinguish "Phase 6 regressed discoverability" from "this run happened to under-sample Skill invocation."
3. **Do not touch `Skill.md`/`SKILL.md` or `help.cpp` wording yet** — no literal or semantic conflict was found that would justify a text change, and the dominant metric (success rate) is not caused by either file.
4. If, after step 2, `ast-tool` usage is still concentrated in ~2/41 tasks, the next-smallest change would be tightening the AST-inspection SKILL.md's own command framing to match the CLI's tiering (section 3) — but this should wait until a clean re-run confirms the collapse persists under a fixed environment.
