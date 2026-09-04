# Phase 7b — Further `SKILL.md` Compression

## Recommendation

**ACCEPT**

Phase 7b reduces the accepted Phase 7a skill by about 47.7% while retaining the explicit routing and recovery decisions. The new 41-task run scored 37/41. This is one success below the accepted Phase 7a run, but the trace and validator evidence does not connect the difference to the compressed body:

- All four failures share a pre-existing fixture defect in `level4-api`: `api_handler.cpp` uses `processor_` and `registry_`, while the baseline `api_handler.h` does not declare them.
- The preserved paired Phase 7a replay in `evaluation/results` fails the same four tests, so its per-test outcomes are identical to Phase 7b.
- Only four Phase 7b tests invoked `semantic-analysis`; three used it before exploration and routed through targeted AST Tool commands. The smoke test invoked it only after its problematic AST sequence.
- The two tests that loaded the skill in both available Phase 7a and Phase 7b traces retained targeted routing.

No removed wording has the causal chain required for rejection:

```text
removed instruction -> skill loaded -> route degraded -> measurable failure/cost
```

## Textual Change

The frontmatter, triggers, languages, and command names are unchanged. The body was compressed structurally:

- The command-selection prose and repeated decision guides became one routing table.
- Five command-documentation sections became one minimal syntax block plus shared semantics.
- Workflow, ambiguity, error, retry, and fallback sections became one recovery section.
- Flag, output, grep, large-output, and transitive-call warnings became four boundary rules.
- Ordinary examples, output schemas, rationale, and duplicate warnings were removed.

The following protected decisions remain explicit:

| Need | Route |
|---|---|
| Symbol/declaration | `search` |
| Callers | `callers` |
| References/usages | `references` |
| Callees | `callees` |
| File symbols | `symbols` |
| AST structure/node lookup | `find` |

The compressed body also still says to target `search`, avoid broad output and manual substitution, never retry an unchanged failure, avoid ordinary `--help`, avoid default `--pretty`, use the cheapest error-directed correction, narrow duplicate C++ FQNs by root, and fall back to grep only after two unresolved semantic attempts.

## Size

| Measure | Phase 7a | Phase 7b | Reduction |
|---|---:|---:|---:|
| Lines | 145 | 68 | 53.1% |
| Characters | 6,183 | 3,235 | 47.7% |
| UTF-8 bytes | 6,195 | 3,239 | 47.7% |
| Approx. tokens (`characters / 4`) | 1,546 | 809 | 47.7% |

The Phase 7b body saves approximately 737 tokens per invocation relative to Phase 7a.

## Evaluation Procedure

The unchanged 41 tasks were run sequentially with tool tracing. The first Level 4/5 validation pass lacked `clang++` in the elevated process `PATH`; those 16 infrastructure-only records were rerun with `D:\Programs\LLVM\bin` added. Analysis selects the latest record per task. No task or AST Tool implementation was changed.

## Aggregate Results

| Metric | Accepted Phase 7a | Phase 7b | Change |
|---|---:|---:|---:|
| Tests | 41 | 41 | 0 |
| Successes | 38 | 37 | -1 |
| Failures | 3 | 4 | +1 |
| Success rate | 92.68% | 90.24% | -2.44 pp |
| Total tool calls | 545 | 421 | -124 |
| Avg. tool calls/test | 13.29 | 10.27 | -3.02 |
| AST Tool calls | 60 | 28 | -32 |
| AST failures | 9 | 7 | -2 |
| AST failure rate | 15.00% | 25.00% | +10.00 pp |
| AST retries | 10 | 5 | -5 |
| AST help calls | 4 | 4 | 0 |
| Grep | 31 | 73 | +42 |
| Glob | 19 | 10 | -9 |
| Read | 275 | 170 | -105 |
| Bash | 102 | 83 | -19 |
| Edit | 85 | 80 | -5 |
| Total elapsed | 2,250.54s | 1,165.06s | -1,085.48s |
| Avg. elapsed/test | 54.89s | 28.42s | -26.47s |
| Input tokens | 2,943 | 1,378 | -1,565 |
| Output tokens | 153,496 | 238,890 | +85,394 |
| Total tokens | 156,439 | 240,268 | +83,829 |
| Avg. tokens/test | 3,815.6 | 5,860.2 | +2,044.6 |
| Avg. recovery distance | 2.56 | 2.00 | -0.56 |
| Max recovery distance | 6 | 3 | -3 |

The token increase is not consistent with the large reductions in elapsed time, tool calls, and reads, indicating that agent/runtime token reporting differs from the accepted run. It is reported as observed, not treated as evidence caused by the skill.

## AST Tool Commands

| Command | Phase 7a | Phase 7b | Change |
|---|---:|---:|---:|
| `search` | 25 | 10 | -15 |
| `callers` | 14 | 6 | -8 |
| `references` | 6 | 9 | +3 |
| `callees` | 4 | 0 | -4 |
| `find` | 6 | 2 | -4 |
| `symbols` | 2 | 0 | -2 |
| `children` | 1 | 0 | -1 |
| help | 1 top-level plus other help-classified calls | 1 parsed command | not directly comparable |

Phase 7b AST failures were `references` 4, `find` 1, `callers` 1, and help 1. The higher failure rate is denominator-driven: there were two fewer failures but 32 fewer AST calls.

## Skill Invocation

The accepted Phase 7a aggregate reported 16 skill invocations. Phase 7b invoked `semantic-analysis` in four tests:

| Test | Invocation timing | Targeted route | Outcome |
|---|---|---|---|
| `level2-004` | first call | `search -> callers -> references -> callers` | success |
| `level2-006` | first call | `search -> search -> references -> callers` | success |
| `level4-006` | first call | `search -> callers -> references`, refined queries | failure |
| `smoke-001` | call 18, after AST exploration | no post-load AST query | success |

The 16-to-4 invocation drop is substantial, but trigger metadata did not change and the body is unavailable until after invocation. It therefore cannot be caused by body compression. The smoke trace likewise cannot be used to blame the body for its earlier unchanged retries and help usage.

## Same-Skill-Invocation Comparison

The accepted Phase 7a per-test trace set is not present in the repository. The preserved paired Phase 7a replay provides two tests that loaded the skill in both runs:

| Test | Phase 7a route | Phase 7b route | Tool calls | Tokens | Assessment |
|---|---|---|---:|---:|---|
| `level2-004` | grep, skill, `search x3 -> callers -> references`, reads/edits | skill, `search -> callers -> references -> callers`, reads/edits | 19 -> 17 | 19,381 -> 8,340 | Targeted; shorter and cheaper |
| `level4-006` | skill, `search -> references -> callers`, grep, refinement | skill, `search -> callers -> references`, retry with narrower root, refinement | 16 -> 17 | 7,245 -> 6,326 | Targeted; failure is fixture compile defect |

Across these comparable traces there is no `AST query -> grep/read-only` replacement. Phase 7b uses one more total call across the pair, three fewer reads, approximately the same grep activity, and 11,960 fewer reported tokens.

## Outliers and Failures

The four failures are `level4-002`, `level4-005`, `level4-006`, and `level4-007`. All compile against the same broken baseline header and fail on the same missing `ApiHandler::processor_` and `ApiHandler::registry_` members.

- Three failures did not invoke the skill or AST Tool.
- `level4-006` invoked the skill and used targeted commands successfully enough to locate the requested method and its relationships. Its edits concern validation paths, while compilation fails in the unrelated baseline `api_handler.cpp`.
- The preserved paired Phase 7a replay has the same 37/41 outcome and the same four failures, ruling out a Phase 7b-specific correctness transition in that comparison.

Manual exploration rose versus the accepted aggregate (`grep` +42), but reads fell by 105 and only four tests loaded the skill. In the comparable skill-loaded pair there is no systematic manual-routing increase. The aggregate grep change is therefore not attributable to compressed guidance.

## Recovery

Phase 7b recorded recovery distances `[3, 1]` (average 2.0, maximum 3), improving on the accepted aggregate average 2.56 and maximum 6. The distance-3 case belongs to `smoke-001` before the skill was invoked. No new long recovery is plausibly caused by the compressed body.

## Token Decomposition

At approximately 737 saved tokens per load:

```text
strictly comparable available loads: 737 x 2 ≈ 1,474 fixed tokens saved
all Phase 7b loads:                  737 x 4 ≈ 2,948 fixed tokens saved
```

Against the preserved paired replay, observed total tokens fell from 241,130 to 240,268, a saving of 862. After subtracting the estimated 1,474-token fixed saving for the two known common invocations, trajectory activity is approximately 612 tokens more expensive. This is small relative to 240k tokens and is dominated by stochastic paths.

Against the accepted aggregate, observed tokens increased by 83,829 despite fixed instruction savings. Because model/runtime accounting and trajectories differ sharply, that delta cannot isolate instruction cost. These estimates exclude unreported prompt/cache/tool-result effects.

## Decision

**ACCEPT**

The skill is materially smaller, the correctness fluctuation is localized and non-causal, comparable skill-loaded routes remain targeted, recovery did not regress, and no compressed rule can be linked to manual exploration or failure. No restoration is justified by the evidence.
