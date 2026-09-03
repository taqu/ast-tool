# Phase 7a — Targeted Trajectory Analysis

## 1. Executive Summary

**A. Phase 7a behavior is effectively preserved.**

The aggregate changes initially resemble a routing regression, but the per-test evidence does not connect them to the compressed `Skill.md`:

- All recovery distances of 6, 5, and 4 came from `smoke-001`, which did not load the skill in either run.
- Every test that lost AST Tool usage also stopped invoking the skill entirely. The skill's trigger metadata was unchanged, so its compressed body could not have influenced those trajectories.
- Most grep/glob/read growth is concentrated in Level 4, especially one test that did not load the skill in either phase.
- When the compressed skill was loaded, routing remained targeted: `search`, `callers`, `references`, and `callees` retained their intended roles.
- The six-call decline in `find` consists of benign substitutions plus tests where the skill was never loaded—not evidence that compressed `find` guidance redirected an active skill user to grep.

The data supports normal stochastic variation and localized outliers, not a systematic compression-induced regression.

## 2. Phase 5 vs Phase 7a Metric Comparison

| Metric | Phase 5 | Phase 7a | Change |
|---|---:|---:|---:|
| Success | 37/41 | 38/41 | +1, +2.44 pp |
| Total tool calls | 518 | 545 | +27, +5.2% |
| AST Tool calls | 69 | 60 | -9, -13.0% |
| AST failures | 9 | 9 | unchanged |
| AST failure rate | 13.04% | 15.00% | +1.96 pp |
| AST retries | 9 | 10 | +1 |
| Grep | 15 | 31 | +16, +106.7% |
| Glob | 12 | 19 | +7, +58.3% |
| Read | 252 | 275 | +23, +9.1% |
| Average recovery distance | 1.44 | 2.56 | +1.12, +77.8% |
| Maximum recovery distance | 2 | 6 | +4 |
| Total tokens | 158,303 | 156,439 | -1,864, -1.18% |
| Average elapsed time | 54.25s | 54.89s | +0.64s, +1.2% |

The headline increases are not broad:

- Level 4 accounts for +12 of the net +16 grep calls.
- Level 4 accounts for +27 reads, exceeding the overall +23 because other levels decreased.
- One test, `level4-004`, accounts for nine additional grep calls.
- Recovery deterioration is entirely attributable to one smoke-test trajectory.

## 3. Long-Recovery Cases

All distances 6, 5, and 4 belong to `smoke-001`; they are overlapping measurements from the same sequence, not three independent pathological tests.

### Phase 7a trajectory

```text
1  find src/greeter.h greet                     failure
2  same invalid find + stderr redirect          failure
3  top-level --help                             cancelled/failure
4  which ast-tool + top-level --help            success
5  same invalid find again                      failure
6  find --help                                  success
7  find --text greet src/greeter.h              useful recovery
8  references src/greeter.h 4                   failure
9  references --help                            success
10 references greet src/                        useful recovery
11 Read
12 Edit
```

The distances are:

- Sequence 1 → 7: 6
- Sequence 2 → 7: 5
- Sequence 3 → 7: 4
- Sequence 5 → 7: 2
- Sequence 8 → 10: 2

The `"2"` command is confirmed as a parser false positive from `which ast-tool 2>&1`. It is not a real AST Tool command.

### Phase 5 comparison

Phase 5 followed an almost identical erroneous path:

```text
find failure
→ find failure
→ which ast-tool
→ find failure
→ top-level help
→ symbols
→ corrected find
→ references failure
→ references help
→ corrected references
```

Its maximum recovery distance stayed at two because the successful `symbols` call was counted as recovery before the corrected `find`. Phase 7a omitted `symbols`, causing multiple earlier failures to resolve against the later corrected `find` and mechanically inflating three recovery measurements.

The Phase 7a trajectory did contain unnecessary exploration: it repeated the invalid `find`, used help, and corrected its syntax late. However, `smoke-001` invoked the skill zero times in both phases. Compression therefore cannot explain the difference; the behavior predates it and represents stochastic variation in an unassisted trajectory.

## 4. `find` Usage Analysis

Phase 5 used `find` in seven tests:

| Test | Phase 5 → Phase 7a | Classification |
|---|---|---|
| `level1-006` | `search → find` became grep/read | C/E, suspicious trajectory but skill not loaded |
| `level2-001` | `search → search → find` became grep/read | C/E, suspicious trajectory but skill not loaded |
| `level2-002` | `search → find` became `search → search` | B, equivalent targeted semantic route |
| `level2-005` | Same `search → callees → search → find` | A |
| `level3-003` | `search → find` became two reads | C/E, but skill not loaded |
| `level3-007` | Three `find` calls became two additional scoped `search` calls | B/D, benign |
| `smoke-001` | Four `find` calls became five | A; more rather than less |

`level2-002` replaced structural lookup with an exact-FQN search and read the same one file. `level3-007` replaced three structural queries with searches for the relevant methods; both runs read the same three files and Phase 7a used seven fewer tokens. These are benign targeted alternatives.

The three genuine AST-to-manual replacements all coincide with the skill not being invoked in Phase 7a. Thus, the 12 → 6 decline does not show that agents consumed compressed `find` guidance and then abandoned `find`.

## 5. `search` Usage Analysis

| Test | Search | Assessment |
|---|---:|---|
| `level1-006` | 1 → 0 | Suspicious replacement by grep, but skill invocation also 1 → 0 |
| `level2-001` | 2 → 0 | Suspicious replacement by grep, but skill invocation also 1 → 0 |
| `level2-006` | 3 → 1 | Benign: same targeted `callers` route with redundant searches avoided |
| `level3-001` | 2 → 0 | Suspicious manual route, but skill invocation also 1 → 0 |
| `level3-003` | 1 → 0 | Manual reads, but skill invocation also 1 → 0 |
| `level3-005` | 2 → 1 | Benign: `callers → search → callers`; shorter recovery |
| `level4-003` | 3 → 0 | Most concerning manual replacement, but skill invocation also 1 → 0 |

`level4-003` changed from a skill-guided `search ×3 → callers ×2 → Read ×6` path to reads, grep, a subagent, and further reads. This is clearly worse routing, but the Phase 7a agent never loaded the skill. The description, triggers, name, and language metadata were unchanged, so compression of the body cannot explain the missing invocation.

All five tests that lost AST Tool entirely show the same pattern:

| Test | Skill calls | AST calls |
|---|---:|---:|
| `level1-006` | 1 → 0 | 2 → 0 |
| `level2-001` | 1 → 0 | 3 → 0 |
| `level3-001` | 1 → 0 | 2 → 0 |
| `level3-003` | 1 → 0 | 2 → 0 |
| `level4-003` | 1 → 0 | 5 → 0 |

This supports stochastic skill-selection variation rather than body-content causation.

## 6. grep / glob / read Increase

### Largest increases

| Test | Δ Grep | Δ Glob | Δ Read |
|---|---:|---:|---:|
| `level4-004` | +9 | +5 | +2 |
| `level4-005` | 0 | +2 | +11 |
| `level4-003` | +1 | 0 | +10 |
| `level4-008` | +1 | 0 | +5 |
| `level3-001` | +1 | -1 | +4 |
| `level1-003` | +2 | -1 | 0 |
| `level1-008` | +2 | 0 | +1 |

The main outlier, `level4-004`, changed from `Glob ×3 → Grep → Read ×6` to `Glob ×6 → Grep ×10 → Read ×8`. Neither run loaded `semantic-analysis`. This one test produced 56% of the net grep increase, 71% of the net glob increase, +1,874 tokens, and +28.54 seconds.

The largest read increases were also localized:

- `level4-005`: +11 reads; neither run loaded the skill.
- `level4-003`: +10 reads after the skill was not invoked.
- `level4-008`: +5 reads; neither run loaded the skill.
- `level3-001`: +4 reads after the skill was not invoked.

The increase is primarily a localized Level 4 phenomenon, not a broad shift among agents that consumed the compressed skill.

## 7. Additional Successful Test

The only outcome improvement was `level4-005`.

Phase 5 did not load the skill or use AST Tool. It edited all seven expected files but compilation failed because `api_handler.h` omitted or lost the `processor_` and `registry_` members still used by `api_handler.cpp`.

Phase 7a also did not load the skill or use AST Tool. It used two globs and 25 reads, inspected related background-processing paths, preserved the members, and passed validation.

The successful path cost +11 reads, +637 tokens, and +3.10 seconds. Because neither run loaded the skill, the improvement is stochastic implementation quality rather than evidence for compression. Its correctness is desirable, but the 25-read path is not an efficiency pattern to encourage.

## 8. Skill.md Diff Correlation

| Change | Classification | Trajectory evidence |
|---|---|---|
| Merged command table and decision tree | SAFE | Routing remains explicit; callers and references counts are unchanged |
| Shortened `search` description and examples | LIKELY SAFE | Harmful search losses occurred only where the skill was not loaded |
| Shortened `find` examples | LIKELY SAFE | Skill-loaded replacements used targeted `search`, not grep/read |
| Consolidated root/FQN/empty-result rules | SAFE | Relevant semantics remain explicit |
| Merged ambiguity cases into three bullets | LIKELY SAFE | Failure counts by `callers`/`callees` did not increase |
| Generalized caller recovery to references/callees | SAFE | Recovery stayed distance one in skill-loaded semantic tasks |
| Condensed flag-validity guidance | SAFE | No new wrong-flag pattern appeared in skill-loaded tasks |
| Removed repeated `When to Use` section | POSSIBLE BEHAVIORAL EFFECT in theory | No supporting trace evidence after skill invocation |
| Removed repeated grep warnings | POSSIBLE BEHAVIORAL EFFECT in theory | Explicit prohibition remains; grep growth occurred mainly without skill loading |
| Merged Common Mistakes and Best Practices | LIKELY SAFE | No systematic regression among skill-loaded tests |
| Reduced command examples | LIKELY SAFE | Ordinary semantic syntax remained correct in skill-loaded trajectories |
| Retained retry/help/fallback rules | SAFE | Violations were confined to the no-skill smoke test |

No suspected regression satisfies the complete causal chain:

```text
removed wording
→ agent consumed compressed wording
→ changed route
→ measurable cost
```

The harmful cases fail at the second step because the skill was not consumed.

## 9. Token Decomposition

The skill shrank from approximately 3,263 to 1,546 tokens, saving about 1,717 tokens per full load.

Phase 7a invoked it in 16 tests:

```text
1,717 × 16 ≈ 27,472 tokens
```

This is the best same-invocation estimate of fixed content savings.

Phase 5 invoked the skill in 20 tests. Comparing actual injected bodies gives a larger theoretical difference:

```text
Phase 5: 20 × 3,263 ≈ 65,260
Phase 7a: 16 × 1,546 ≈ 24,736
difference ≈ 40,524
```

About 13,052 tokens of that difference come from four fewer skill invocations, which should be treated as stochastic routing variation rather than compression savings.

Actual reported total tokens fell by only 1,864. Under the same-invocation estimate, non-skill trajectory activity therefore consumed approximately `27,472 - 1,864 ≈ 25,608` additional tokens. Under the raw 20-versus-16 comparison, the offset is about 38,660 tokens.

These are estimates. The result records report only 2,943 input tokens across all Phase 7a tasks, so they do not expose full prompt, cache, and tool-result accounting sufficiently for exact decomposition.

The defensible conclusions are:

- The smaller skill likely provided substantial fixed savings when loaded.
- Those savings were mostly consumed by stochastic manual Level 4/5 trajectories.
- Trajectory efficiency was worse after removing the estimated fixed saving.
- The traces do not show that compression caused that inefficiency.

## 10. Recommendation

**ACCEPT**

Do not restore wording based on this run. The aggregate warning signals are real, but targeted analysis does not connect them to the compressed skill body.

The apparent regressions are explained by:

1. A single no-skill smoke trajectory inflating recovery distance.
2. Five tests not invoking the skill despite unchanged trigger metadata.
3. One no-skill Level 4 outlier causing most grep/glob growth.
4. Localized manual exploration in tests that did not consume either version's instructions.

A minor restoration would add instruction volume without addressing the observed cause. Before Phase 7b, a repeat evaluation would be useful for measuring run-to-run variance in skill invocation, but Phase 7b should not be started automatically.
