# Phase 7c.1 — Targeted Trajectory Restoration

## Executive Decision

**REVERT TO PHASE 7C**

No Phase 5 wording restoration satisfies the required causal standard. The current Phase 7c `SKILL.md` is therefore unchanged and remains the fallback candidate.

The observed regressions are real at aggregate level, but the traces do not connect them to weakened Phase 7c Skill wording:

- Five of six tests with fewer `find` calls use an equivalent targeted route, avoid a now-unnecessary query, or improve total cost.
- The only same-loaded `find → Glob` substitution, `level3-007`, saves one tool call, 707 tokens, and 0.96 seconds with unchanged reads and correctness.
- The largest glob/read regressions generally occur when Phase 7c did not load the Skill, so its body could not determine the trajectory.
- The recovery-distance-4 case, `smoke-001`, loaded no Skill in either phase. Its inefficient help sequence cannot be corrected by changing an unloaded body.

Under the phase rules, restoring text merely because it sounds useful would be speculative. Since no restoration candidate was implemented, targeted and full replays would reproduce Phase 7c only stochastically and provide no test of a changed variable; they were not rerun.

## 1. Phase 5 vs Phase 7c Trajectory Diagnosis

| Test | Phase 5 route | Phase 7c route | Extra cost (7c − 5) | Suspected lost rule | Confidence |
|---|---|---|---:|---|---|
| `level3-007` | `search×3 → find×3 → Read×3` | `search×4 → Glob → Read×3` | tools -1; tokens -707; time -0.96 s | Possible stronger `find`-before-file-discovery cue | LOW: replacement is cheaper |
| `level2-005` | `search → callees → search → find` | `search → callees → search → Glob → callees → symbols → callees → find → symbols` | tools +5; tokens +990; time +35.86 s | None identified; existing routing and recovery rules already prohibit this exploration | LOW |
| `level3-006` | `outline → Glob → Read` | `outline → Glob×2 → Read` | tools +1; tokens +463 | None; extra repository discovery, not replacement of `find` | LOW |
| `level4-003` | Skill; targeted searches/callers; Read×6 | no Skill; `Glob → Grep → Read×14` | tools +4; tokens +5,879 | Semantic-first rules unavailable because Skill was not loaded | NONE |
| `level4-005` | no Skill; Read×14 | no Skill; `Glob×2 → Read×25` | tools +11; tokens +5,321 | None in Skill body | NONE |
| `level4-008` | no Skill; `Glob → Read×4` | no Skill; `Glob×3 → Read×8` | tools +8; tokens +3,985 | None in Skill body | NONE |
| `smoke-001` | no Skill; failed `find` sequence, help, recovery | no Skill; failed `find`, Read, help×2, corrected `find`, failed references, help, reads | tools +1; reads +4; tokens +4,508 | Recovery rules unavailable because Skill was not loaded | NONE |

Only HIGH or well-supported MEDIUM cases qualify for restoration. This table contains none.

## 2. Per-Test `find` Disappearance

| Test | Find P5 → P7c | Replacement | Classification | Cost/result assessment |
|---|---:|---|---|---|
| `level1-006` | 1 → 0 | Extra targeted `search` | A: equivalent targeted search | Success preserved; tools -1; tokens +8 |
| `level2-001` | 1 → 0 | Grep/Read, with Skill loaded only in Phase 5 | E: textual/manual route | Success preserved; tools -3; tokens -693; not body-causal |
| `level2-002` | 1 → 0 | Extra targeted `search` | A: equivalent targeted search | Success preserved; tools equal; tokens -81 |
| `level3-003` | 1 → 0 | Existing search result followed directly by one Read | C: no additional query needed | Success preserved; tools -1; tokens +254 |
| `level3-007` | 3 → 0 | Extra targeted search plus one Glob | A/D: targeted search plus file discovery | Success preserved; reads equal; tools -1; tokens -707 |
| `smoke-001` | 4 → 3 | Additional reads around malformed calls | E: manual reasoning after failure | Success preserved; tokens +4,508; no Skill in either run |

The raw `find` decrease is not a restoration target. `level3-007` is the only candidate where loaded Phase 7c replaces structural lookup with file discovery, but the trajectory is measurably cheaper and uses no additional reads. A rule intended to force its Phase 5 route would optimize command count rather than agent efficiency.

## 3. Glob Regression Ranking

Positive per-test glob deltas:

| Task | Phase 5 | Phase 7c | Δglob | Δread | Δtools | Δtokens | Skill P5 → 7c | Classification |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `level5-002` | 0 | 5 | +5 | 0 | +4 | +1,081 | 0 → 0 | Repository discovery; no body causality |
| `level4-005` | 0 | 2 | +2 | +11 | +11 | +5,321 | 0 → 0 | Unnecessary broad exploration; no body causality |
| `level4-008` | 1 | 3 | +2 | +4 | +8 | +3,985 | 0 → 0 | Broad exploration; no body causality |
| `level5-008` | 0 | 2 | +2 | -4 | -3 | -1,220 | 0 → 0 | Benign alternative |
| `level2-005` | 0 | 1 | +1 | 0 | +5 | +990 | 1 → 1 | Repository discovery amid redundant semantic queries |
| `level3-006` | 1 | 2 | +1 | 0 | +1 | +463 | 1 → 1 | Benign repository discovery |
| `level3-007` | 0 | 1 | +1 | 0 | -1 | -707 | 1 → 1 | Cheaper substitute after targeted searches |
| `level4-003` | 0 | 1 | +1 | +8 | +4 | +5,879 | 1 → 0 | Symbol-search replacement, but Skill absent |
| `level5-004` | 0 | 1 | +1 | -5 | -8 | -2,691 | 0 → 0 | Benign alternative |

Negative deltas in five tasks offset six calls, yielding the aggregate 12 → 22 change. The largest suspicious glob/read chains are not attributable to the Phase 7c body.

## 4. Read Regression Ranking

The tests with the largest positive read deltas are:

| Task | Δread | Δglob | Δtools | Δtokens | Cause |
|---|---:|---:|---:|---:|---|
| `level4-005` | +11 | +2 | +11 | +5,321 | Broad context gathering; Skill absent both runs |
| `level4-003` | +8 | +1 | +4 | +5,879 | Manual replacement for Phase 5 semantic route; Phase 7c Skill absent |
| `level4-002` | +6 | -1 | +5 | +2,242 | Implementation/fixture inspection; Skill absent both runs |
| `level3-001` | +4 | 0 | +5 | -117 | Implementation/agent exploration; Skill present only in Phase 5 |
| `level4-004` | +4 | -2 | +2 | +238 | Implementation inspection; Skill absent both runs |
| `level4-008` | +4 | +2 | +8 | +3,985 | Broad repository/context gathering; Skill absent both runs |
| `smoke-001` | +4 | 0 | +1 | +4,508 | Manual inspection after malformed AST usage; Skill absent both runs |
| `level4-001` | +2 | 0 | +2 | -1,146 | Implementation inspection; Skill absent both runs |
| `level5-005` | +2 | 0 | +2 | +89 | Implementation inspection; Skill absent both runs |
| `level2-008` | +1 | 0 | +3 | +570 | Semantic recovery plus final verification; Skill loaded both |
| `level3-004` | +1 | 0 | +6 | +1,490 | Redundant semantic exploration; Skill loaded both |
| `level5-001` | +1 | -1 | -2 | -1,026 | Benign alternative; Skill absent both |

The net +34 reads are driven mainly by no-Skill or one-run-only cohorts. The same-loaded increases are small and relate to semantic recovery/verification rather than a missing `find` preference.

## 5. Recovery-Distance-4 Analysis

`smoke-001` produces Phase 7c's only recovery distance of 4. Neither run loads `semantic-analysis`.

Phase 7c sequence:

```text
find src/greeter.h greet          # malformed; fails with usage
Read main.cpp                     # parallel call cancelled
ast-tool --help                   # unnecessary top-level help
ast-tool find --help              # command help
find --text greet src/greeter.h   # useful recovery; distance 4
Read main.cpp
references src/greeter.h 4        # wrong symbol/root form; fails
references --help                 # unnecessary: diagnostic already identifies root
references greet src/             # same-FQN C++ ambiguity; fails
Read greeter.h + greeter.cpp
Edit + verification Read
```

Phase 5 also begins with malformed `find`, performs ineffective path/binary variations, and uses help before a useful `find`; its analyzer splits these into shorter recovery distances. It later corrects the invalid references root after help and succeeds with `references greet .`.

The Phase 7c error already showed `find [options] <file>`, so `find --text greet src/greeter.h` was the cheap correction. Top-level help was unnecessary. The invalid references command similarly returned a root diagnostic, making references help unnecessary. These decisions violate Phase 7c's existing recovery wording, but that wording was never loaded. No removed Phase 5 rule can causally explain the trajectory, so no restoration is justified.

## 6. Relevant Phase 5 vs Phase 7c Skill Diff

Phase 5 said:

```text
Use when the file is already identified and you need to locate a node by
type, text content, source position, or node ID.

Use search for cross-workspace symbol lookup; use find for targeted node
lookup within a specific file.
```

Phase 7c says:

```text
Find a node by text, type, ID, or position in a known file → find.
File-scoped structural lookup, not workspace symbol discovery.
```

The trigger, action, and `search` boundary remain explicit. Phase 7c additionally says to prefer direct semantic routes over grep/manual exploration, avoid workspace dumps, scope available filters/root, and read only required returned locations. Its recovery section already prohibits unchanged retries, cosmetic variations, routine help, and trial-and-error sequences.

There is therefore no demonstrably weakened rule corresponding to the measured costly trajectories.

## 7. Causal Restoration Candidates

| Candidate | Phase 5 meaning | Observed difference | Proposed wording considered | Expected effect | Decision |
|---|---|---|---|---|---|
| Structural lookup before manual inspection | Known-file nodes use `find` | `level3-007`: find×3 became search+Glob | “For AST structure or node lookup in a known file, use `find` before Glob or manual inspection.” | Might increase `find` | Reject: existing rule is equivalent and observed route is cheaper |
| Diagnostic-first recovery | Correct failure using diagnostic | `smoke-001` used help/read before correction | “Apply a diagnostic-specified correction immediately before help or reads.” | Could shorten recovery | Reject: existing rule already says this and was not loaded |
| Limit repository discovery | Scope queries; avoid broad dumps | Several Glob/Read increases | “Do not Glob when search/find identifies the target file.” | Could reduce exploration | Reject: major cases had no Skill; same-loaded case not costly |

Confidence is LOW or NONE for all candidates. None qualifies for implementation.

## 8. Final Minimal Restoration Set

**Empty set.** `skills/semantic-analysis/SKILL.md` remains byte-for-byte Phase 7c. Frontmatter, triggers, and metadata are unchanged. No AST Tool, CLI, task, harness, or metrics code was modified.

## 9. Skill Size

| Version | Lines | Characters | UTF-8 bytes | Approx. tokens |
|---|---:|---:|---:|---:|
| Phase 7c | 135 | 8,745 | 8,759 | 2,186 |
| Phase 7c.1 | 135 | 8,745 | 8,759 | 2,186 |
| Restoration increase | 0 | 0 | 0 | 0 |

## 10. Verification Results

### Stage 1 — Targeted replay

Not run: there is no changed rule to test. A replay would compare identical Skill bodies and measure only stochastic variation, contrary to the phase's causal purpose.

### Stage 2 — Full 41-task evaluation

Not rerun for the same reason. Phase 7c.1 is identical to Phase 7c, so the existing valid Phase 7c run remains the applicable result:

| Metric | Phase 5 | Phase 7c | Phase 7c.1 |
|---|---:|---:|---:|
| Success | 37/41 | 37/41 | identical candidate; no rerun |
| Tool calls | 518 | 542 | identical candidate; no rerun |
| AST calls | 69 raw / 68 corrected | 59 | identical candidate; no rerun |
| AST failures | 9 | 7 | identical candidate; no rerun |
| AST failure rate | 13.04% | 11.86% | identical candidate; no rerun |
| Search | 30 | 25 | identical candidate; no rerun |
| Callers | 14 | 11 | identical candidate; no rerun |
| References | 6 | 6 | identical candidate; no rerun |
| Callees | 3 | 7 | identical candidate; no rerun |
| Find | 12 | 4 | identical candidate; no rerun |
| Symbols | 1 | 4 | identical candidate; no rerun |
| Grep | 15 | 17 | identical candidate; no rerun |
| Glob | 12 | 22 | identical candidate; no rerun |
| Read | 252 | 286 | identical candidate; no rerun |
| Bash | 120 | 102 | identical candidate; no rerun |
| Edit | 86 | 82 | identical candidate; no rerun |
| Recovery mean/max | 1.44 / 2 | 1.60 / 4 | identical candidate; no rerun |
| Elapsed | 2,224.27 s | 2,214.34 s | identical candidate; no rerun |
| Tokens | 158,303 | 178,116 | identical candidate; no rerun |

## 11. Token and Outlier Analysis

For paired Phase 7c − Phase 5 token deltas: minimum -2,737; median +89; p75 +990; p90 +2,522; maximum +5,879. The net increase is 19,813 tokens.

Largest regressions are `level4-003` (+5,879), `level4-005` (+5,321), `smoke-001` (+4,508), `level4-008` (+3,985), and `level5-006` (+2,522). The top one accounts for 29.7% of the net delta and the top three for 79.3%. The top five exceed the net increase (112.1%) because improvements elsewhere offset them.

Largest improvements are `level5-007` (-2,737), `level5-004` (-2,691), `level2-006` (-2,014), `level5-008` (-1,220), and `level4-001` (-1,146).

The token regression is strongly outlier-driven. The top four costly cases associated with glob/read/recovery did not load the Phase 7c Skill, so no candidate restoration can credibly claim to reduce their context cost.

## 12. Per-Test Material Changes

There are no Phase 7c.1 changes and thus no Phase 7c.1-vs-Phase-7c per-test deltas. The material Phase 5-vs-Phase-7c differences relevant to the investigation are documented above. No restored rule can be assigned as their cause.

## 13. Final Recommendation

**REVERT TO PHASE 7C**

Keep the current Phase 7c candidate unchanged. The data does not support even a one-sentence restoration under the required causal standard. The principal regressions arose outside Skill-loaded trajectories, while the clearest same-loaded `find` substitution was cheaper than Phase 5.

This does not establish that Phase 7c is ready to replace Phase 5; it establishes only that targeted Skill-body restoration is not supported by these traces. Addressing stochastic Skill invocation or no-Skill agent behavior would be a different phase and should not be smuggled into Phase 7c.1.
