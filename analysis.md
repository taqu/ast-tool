# Phase 7b — Further Skill.md Compression Report

## 1. Executive Summary

The Phase 7b candidate further compresses `skills/semantic-analysis/SKILL.md` while retaining the protected routing and recovery rules.

The Skill was reduced from approximately 1,546 to 875 tokens, a further 43.4% reduction. Repository tests pass, and the 31 valid evaluation runs show preserved correctness and targeted routing. However, the external Claude account limit interrupted the remaining ten tests before their first tool call, so the required full 41-test comparison is incomplete.

**Recommendation: REVISE — evaluation incomplete.**

No wording restoration is currently indicated. The completed evidence is favorable, but Phase 7b should not be accepted until the ten infrastructure-failed tasks are rerun.

## 2. Textual Diff Summary

Phase 7b removes documentation while retaining decisions:

- Merged the command-selection and per-command sections into one routing table and one syntax block.
- Removed ordinary command examples and output schemas that duplicate CLI help.
- Merged shared `references`/`callers`/`callees` boundaries.
- Condensed flag-validity guidance into one rule list.
- Replaced the expanded semantic workflow and error table with one five-step recovery sequence.
- Retained the C++ declaration/definition ambiguity exception.
- Merged output-volume and fallback policy into two short paragraphs.

The following protected behavior remains explicit:

```text
Find symbol       → search
Find callers      → callers
Find references   → references
Find callees      → callees
Find file symbols → symbols
Need AST structure → find
```

The candidate also retains:

- AST Tool as the default targeted semantic/structural path.
- Exact CLI syntax for all routed commands.
- Directory-root and FQN matching rules.
- `find`/`search` flag boundaries.
- No unchanged retries and a maximum of two failed attempts.
- No routine `--help` or default `--pretty`.
- Targeted queries instead of broad workspace dumps.
- Grep only for textual content or the documented unresolved-ambiguity fallback.

## 3. Skill Size

| Metric | Phase 7a | Phase 7b | Reduction |
|---|---:|---:|---:|
| Bytes | 6,195 | 3,503 | 43.5% |
| Characters | 6,182 | 3,501 | 43.4% |
| Lines | 144 | 73 | 49.3% |
| Approximate tokens | 1,546 | 875 | 43.4% |

## 4. Verification

Repository test result:

```text
144 passed, 2 skipped
```

`git diff --check` passes for the modified Skill.

## 5. Evaluation Status

The 41-task evaluation produced 31 valid agent runs. The Claude account then reached its usage limit. The following ten tasks exited before any tool call and are infrastructure failures rather than behavioral results:

- `level4-008`
- `level5-001`
- `level5-002`
- `level5-003`
- `level5-004`
- `level5-005`
- `level5-006`
- `level5-007`
- `level5-008`
- `smoke-001`

The CLI reported a reset time of 9:30am JST. These tasks must be rerun before producing a full aggregate recommendation.

## 6. Valid 31-Task Aggregate Comparison

| Metric | Phase 7a | Phase 7b | Change |
|---|---:|---:|---:|
| Successes | 28/31 | 28/31 | unchanged |
| Total tool calls | 327 | 315 | -12 |
| AST Tool calls | 50 | 55 | +5 |
| Grep | 28 | 23 | -5 |
| Glob | 14 | 9 | -5 |
| Read | 126 | 120 | -6 |
| Total tokens | 100,449 | 118,840 | +18,391 |
| Skill invocations | 16 | 16 | unchanged |

The same three valid tasks failed in both runs:

- `level4-002`
- `level4-006`
- `level4-007`

There is no correctness regression in the completed cohort. Total tool calls and manual exploration decreased, while AST Tool usage increased. Tokens increased substantially, so the instruction-size saving was consumed by trajectory/output variation.

## 7. Skill Invocation Comparison

Sixteen valid Phase 7b tasks loaded the skill, the same number as the corresponding Phase 7a cohort.

Fourteen tests loaded the Skill in both phases. These tests are the primary evidence for effects caused by the compressed body. The other 17 valid tests did not load the skill in one or both runs and are not direct evidence about Phase 7b wording.

## 8. Same-Skill-Invocation Comparison

| Metric | Phase 7a | Phase 7b | Change |
|---|---:|---:|---:|
| Tests | 14 | 14 | — |
| Successes | 13 | 13 | unchanged |
| Total tool calls | 116 | 123 | +7 |
| AST Tool calls | 42 | 48 | +6 |
| `search` | 20 | 25 | +5 |
| `callers` | 14 | 16 | +2 |
| `references` | 3 | 3 | unchanged |
| `callees` | 4 | 4 | unchanged |
| `find` | 1 | 0 | -1 |
| Grep | 2 | 2 | unchanged |
| Glob | 1 | 2 | +1 |
| Read | 25 | 27 | +2 |
| Tokens | 36,537 | 41,448 | +4,911 |

The command trajectories remained semantic and targeted. Representative comparisons include:

```text
level1-002
7a: search → callers → references
7b: search → callers → references

level2-008
7a: callers → search → callers
7b: callers → search → callers

level3-002
7a: callers → search → callers
7b: callers → search → callers

level3-008
7a: search → search → search → callers → callers
7b: search → search → search → callers → callers

level4-006
7a: search → callers → references
7b: search → references → callers
```

No comparable test replaced a semantic route with grep plus broad reads. The one lost `find` call in `level2-005` was replaced with scoped `search`; the task read the same two files and passed.

## 9. Outliers

### `level2-004`

Phase 7a:

```text
search → callers → references
```

Phase 7b:

```text
callers → search → callers → references → search
```

The Phase 7b route remained semantic but performed two extra AST queries and used approximately 4,480 more tokens. This is the largest same-invocation token outlier. The compressed workflow still says to search first when identity is uncertain, so the callers-first choice is not directly explained by removed guidance. One trajectory is insufficient to infer causality.

### `level4-005`

This task loaded the skill in neither run. Phase 7b again passed, but used 24 reads and 20,848 reported tokens. It is a major aggregate token outlier but provides no evidence about the compressed Skill body.

### `level4-002`, `level4-006`, and `level4-007`

These failed in both Phase 7a and Phase 7b with the same fixture-related compilation state. They do not constitute new correctness regressions.

## 10. Token Decomposition

Let:

```text
S7a = 1,546 approximate tokens
S7b =   875 approximate tokens
N   =    14 comparable Skill invocations
```

Estimated fixed saving:

```text
(S7a - S7b) × N
= 671 × 14
≈ 9,394 tokens
```

Observed comparable-group tokens increased by 4,911:

```text
36,537 → 41,448
```

After accounting for estimated fixed Skill savings, underlying trajectory/output cost increased by roughly:

```text
9,394 + 4,911 ≈ 14,305 tokens
```

This estimate is approximate because the evaluation records do not expose complete prompt, cache, and tool-result accounting. The increase is concentrated in a few stochastic output-heavy runs rather than a systematic move to manual exploration.

## 11. Causal Assessment

The completed traces do not support a causal regression chain of:

```text
removed guidance
→ different decision after Skill load
→ grep/manual replacement
→ measurable routing cost
```

Among the 14 directly comparable Skill-loaded tests:

- Correctness was unchanged.
- AST Tool calls increased.
- Grep was unchanged.
- Reads increased by only two.
- Core relationship-command usage was preserved or increased.
- No targeted AST route became broad manual exploration.

The token increase warrants further observation, but there is no current evidence tying it to removal of a specific instruction.

## 12. Recommendation

**REVISE — evaluation incomplete.**

The Phase 7b candidate is promising and no wording restoration is currently justified. It is materially smaller, preserves correctness in the completed cohort, and retains targeted routing among tests that loaded both Skill versions.

It cannot be accepted until the ten infrastructure-failed tasks are rerun after the account limit resets. Resume the existing result set with `--retry-failed`, then recompute:

- Full aggregate metrics.
- Per-command usage.
- Recovery distances and failures by command.
- Skill invocation groups.
- Same-Skill-invocation trajectories.
- Full token decomposition.

Do not proceed to Phase 8 automatically.
