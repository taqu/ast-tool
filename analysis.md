# Phase 7d.2 — 18-Task Controlled Evaluation

## Objective

Perform a controlled 18-task comparison between:

```text
Phase 5 semantic-analysis/SKILL.md
vs
Phase 7d semantic-analysis/SKILL.md
```

using the **current repository state** and holding all other variables as constant as practical.

Phase 7d.2 is an evaluation-only phase.

Do not modify:

* Phase 7d `SKILL.md`
* Phase 5 `SKILL.md`
* AST Tool implementation
* CLI behavior
* evaluation tasks
* fixtures
* validators
* metrics logic
* harness behavior

The purpose is to determine whether the current Phase 7d candidate can preserve Phase 5's routing and recovery stability while providing a reproducible efficiency improvement.

---

# Experimental Principle

Use the current repository and current AST Tool implementation for both arms.

The experiment should be:

```text
same repository
same AST Tool
same evaluation harness
same task fixtures
same permissions
same runtime/environment
same forced Skill invocation

variable:
Phase 5 Skill body
vs
Phase 7d Skill body
```

Do not compare different repository implementations.

Only the installed `semantic-analysis/SKILL.md` should differ between the two controlled arms.

---

# Baseline and Candidate

Use:

```text
Phase 5 = stable behavioral baseline
Phase 7d = candidate
```

Phase 7d should remain exactly:

```text
Phase 5
+
one narrow refined-search rule
```

The intentional Phase 7d addition is:

```text
If a refined `search` already identifies the exact symbol or member needed,
do not add a redundant `find` solely to locate it; use `find` when AST
structure or node detail is required.
```

Do not make any additional wording changes during this phase.

---

# Prior Gate Result

Phase 7d.1 repeated guard validation concluded:

```text
MINOR STOCHASTIC DIFFERENCE
```

Across 12 primary guard runs per version:

```text
                    Phase 5   Phase 7d
success              12/12      12/12
tools                   110        110
AST calls                39         36
AST failures             12         12
retries                  11         10
help                      0          0
tokens               44,184     41,245
```

No systematic guard regression was reproduced.

Therefore Phase 7d may proceed to the full controlled 18-task cohort.

---

# Controlled Task Set

Use exactly the same 18-task cohort from the previous controlled Phase 5 vs Phase 7c evaluation:

```text
level1-001
level1-002
level1-005
level1-006

level2-001
level2-004
level2-005
level2-006
level2-008

level3-002
level3-003
level3-004
level3-005
level3-007
level3-008

level4-003
level4-006

smoke-001
```

Do not add or remove tasks.

This cohort provides coverage for:

```text
search
find
callers
references
callees
symbol identity
C++ ambiguity
relationship ordering
error recovery
help behavior
grep fallback
structural lookup
```

---

# Forced Skill Invocation

For every run:

```text
semantic-analysis must be invoked exactly once
and must be the first tool action before any repository exploration.
```

This applies to both Phase 5 and Phase 7d.

If the Skill is:

```text
not invoked
invoked after repository exploration
invoked more than once
```

mark the run invalid and rerun it.

Do not allow Skill invocation variance to contaminate this comparison.

---

# Skill Verification

Before running the experiment, record:

```text
Phase 5 Skill hash
Phase 7d Skill hash
```

Also record:

```text
lines
characters
bytes
approximate tokens
```

Verify that Phase 7d differs from Phase 5 only by the intended narrow backport.

If unexpected Skill differences exist, stop and report them before evaluation.

---

# Environment Recording

Record, where observable:

```text
Claude Code/runtime version
repository revision
AST Tool revision
evaluation harness revision
Skill hashes
timestamps
model identifier
```

If the model identifier is unavailable, state that explicitly.

Do not silently combine runs performed under different runtime versions.

---

# Run Ordering

Reduce temporal/runtime bias by interleaving Phase 5 and Phase 7d where practical.

Prefer alternating the starting arm by task.

For example:

```text
task 1: Phase 5 → Phase 7d
task 2: Phase 7d → Phase 5
task 3: Phase 5 → Phase 7d
...
```

Record actual execution order.

Do not run all Phase 5 tasks first and all Phase 7d tasks much later unless unavoidable.

---

# Number of Runs

For this stage, run:

```text
1 fresh controlled Phase 5 run per task
1 fresh controlled Phase 7d run per task
```

for all 18 tasks.

This gives:

```text
18 Phase 5 runs
18 Phase 7d runs
36 total controlled runs
```

Do not reuse historical runs as the primary result when fresh paired runs can be produced.

Historical controlled results may be used only as supporting context.

---

# Metrics Per Run

Collect for every run:

```text
task ID
Skill version
success/failure

total tool calls

AST Tool calls
AST Tool failures
AST Tool retries
AST Tool help calls

search
callers
references
callees
find
symbols
other AST commands

grep
glob
read
bash
edit

all recovery distances
maximum recovery distance

input tokens
output tokens
total tokens

elapsed time

AST Tool command sequence
relevant full trajectory
```

---

# Primary Evaluation Questions

The controlled evaluation must answer four questions.

## 1. Correctness

Does Phase 7d preserve Phase 5 correctness?

Primary requirement:

```text
Phase 7d successes >= Phase 5 successes
```

Previous Phase 5 controlled result was:

```text
17/18
```

Inspect every differing result individually.

Do not attribute a success/failure difference to the Skill without trajectory evidence.

---

## 2. Routing Stability

Does Phase 7d preserve targeted semantic routing?

Look for direct semantic routes such as:

```text
search → callers
search → references
search → callees
search → find
refined search
```

Flag transitions such as:

```text
Phase 5:
semantic query

Phase 7d:
grep → read → manual reasoning
```

or:

```text
Phase 5:
targeted search/find

Phase 7d:
glob → broad reads
```

A lower `find` count is not automatically a regression.

---

## 3. Recovery Stability

Does Phase 7d remain approximately Phase 5-like in:

```text
AST failures
retries
help calls
recovery distance
```

Inspect every long recovery trajectory.

Do not reject based on a single stochastic outlier unless it reproduces a clear directional pattern.

---

## 4. Efficiency

Does Phase 7d reduce redundant semantic work without introducing hidden cost?

Evaluate:

```text
total tools
AST calls
find calls
reads
grep/glob
tokens
elapsed time
```

At least one meaningful efficiency improvement should be visible without meaningful regression elsewhere.

---

# Per-Test Paired Analysis

For each task, compare:

```text
Phase 5
vs
Phase 7d
```

and classify as:

```text
CLEAR IMPROVEMENT
POSSIBLE IMPROVEMENT
EQUIVALENT
BENIGN DIFFERENCE
POSSIBLE REGRESSION
CLEAR REGRESSION
```

Include the substantive trajectory, not only metric deltas.

Use a table such as:

| Task | P5 route | P7d route | Δtools | ΔAST | Δfailures | Δtokens | Assessment |
| ---- | -------- | --------- | -----: | ---: | --------: | ------: | ---------- |

---

# Special Focus — Refined Search vs `find`

The intended Phase 7d optimization concerns redundant `find`.

For every Phase 5 `find` that disappears in Phase 7d, classify it as:

```text
A. redundant find eliminated after exact/refined search
B. replaced by equally targeted semantic search
C. structurally necessary find incorrectly skipped
D. replaced by glob/read/manual exploration
```

Interpretation:

```text
A = improvement
B = benign or improvement
C = regression
D = regression
```

Do not optimize toward a specific raw `find` count.

---

# Positive-Control Task — `level3-007`

Inspect `level3-007` explicitly.

Compare:

```text
search calls
find calls
AST calls
total tools
failures
tokens
elapsed
correctness
```

Determine whether Phase 7d again avoids redundant structural lookup.

However, Phase 7d.1 showed that Phase 5 can also spontaneously take the cheaper route.

Therefore classify the result as:

```text
deterministic improvement
directional improvement
no detectable version effect
regression
```

Do not overstate one sample.

---

# Relationship-Ordering Guards

Pay special attention to:

```text
level2-006
level2-008
level3-008
```

Classify the initial semantic strategy as:

```text
IDENTITY_FIRST
RELATIONSHIP_FIRST_RECOVERED
OTHER
```

Examples:

```text
IDENTITY_FIRST:
search → callers
```

```text
RELATIONSHIP_FIRST_RECOVERED:
callers → failure → search → callers
```

Compare Phase 5 and Phase 7d.

A single relationship-first difference is not sufficient evidence of regression.

A repeated directional pattern across multiple tasks is more meaningful.

---

# `smoke-001` Recovery Guard

Analyze `smoke-001` in detail.

Record:

```text
malformed find handling
references argument choice
callers argument choice
help usage
unchanged retries
search refinement
failure count
retry count
maximum recovery distance
```

Compare the full Phase 5 and Phase 7d recovery trajectories.

Phase 7d.1 showed both versions can vary substantially, so do not infer a Skill effect from raw failure count alone.

---

# Help Behavior

Phase 5's controlled behavior strongly avoided help.

Check whether Phase 7d preserves:

```text
no routine --help exploration
```

Any Phase 7d help call should be inspected.

Classify whether it was:

```text
justified
unnecessary
causally related to the Phase 7d rule
unrelated stochastic behavior
```

---

# Manual Exploration

Compare:

```text
grep
glob
read
```

per task and in aggregate.

Flag cases where a Phase 5 semantic route becomes:

```text
grep/glob
→ broad reads
→ manual inference
```

Phase 7d should not gain efficiency merely by abandoning semantic queries.

---

# Aggregate Comparison

Produce a controlled aggregate table:

| Metric            | Phase 5 | Phase 7d | Delta | Assessment |
| ----------------- | ------: | -------: | ----: | ---------- |
| Success           |         |          |       |            |
| Tools             |         |          |       |            |
| AST calls         |         |          |       |            |
| AST failures      |         |          |       |            |
| Failure rate      |         |          |       |            |
| Retries           |         |          |       |            |
| Help              |         |          |       |            |
| Search            |         |          |       |            |
| Callers           |         |          |       |            |
| References        |         |          |       |            |
| Callees           |         |          |       |            |
| Find              |         |          |       |            |
| Grep              |         |          |       |            |
| Glob              |         |          |       |            |
| Read              |         |          |       |            |
| Tokens            |         |          |       |            |
| Elapsed           |         |          |       |            |
| Recovery mean/max |         |          |       |            |

---

# Token Analysis

Token variance has been large in previous experiments.

Report:

```text
total
mean
median
p75
p90
min
max
```

for both versions.

For paired deltas:

```text
Phase 7d - Phase 5
```

report:

```text
median delta
p75 delta
p90 delta
minimum
maximum
```

Also classify tasks using an explicit tolerance such as:

```text
improved:   delta < -250
unchanged: -250 <= delta <= +250
worsened:   delta > +250
```

or another clearly justified threshold.

---

# Token Outlier Contribution

Identify:

```text
largest positive token delta
largest negative token delta
top 3 positive contributors
top 5 positive contributors
```

Report how much of the aggregate token difference they explain.

Distinguish:

```text
broad version effect
```

from:

```text
few stochastic generation outliers
```

---

# Recovery Analysis

For each AST failure record:

```text
failed command
failure reason
next action
recovery distance
unchanged retry?
help?
diagnostic followed?
```

Then compare recovery distributions.

Report:

```text
mean
median
maximum
all observed distances
```

A Phase 7d recovery regression matters most when:

```text
same task
same type of failure
repeatedly longer recovery
```

appears.

---

# Rule Causality

Phase 7d contains one behavioral change.

For every meaningful trajectory difference ask:

```text
Can the added refined-search rule plausibly explain this?
```

Require:

```text
added rule
→ changed search/find decision
→ changed trajectory
→ measurable effect
```

Do not attribute relationship-ordering, help, or reference ambiguity differences to the Phase 7d rule without evidence.

---

# Shared Inefficiency Inventory for Phase 9

While comparing trajectories, separately record inefficiencies that appear under both Phase 5 and Phase 7d.

Examples:

```text
relationship query
→ ambiguity/failure
→ search
→ relationship retry
```

```text
search
→ declaration result
→ second query/read needed for definition
```

```text
same-FQN declaration/definition ambiguity
```

```text
find
→ useful node result
→ read still needed for implementation context
```

These are possible Phase 9 semantic-capability research targets.

Do not fix them in Phase 7d.2.

---

# Controlled Acceptance Decision

Choose exactly one:

```text
ACCEPT PHASE 7D
ACCEPT WITH CAVEATS
REVISE
REVERT TO PHASE 5
```

## ACCEPT PHASE 7D

Use when:

```text
correctness >= Phase 5
AND
routing remains targeted
AND
recovery is Phase 5-like
AND
at least one meaningful efficiency benefit is observed
AND
no systematic regression appears
```

---

## ACCEPT WITH CAVEATS

Use when:

```text
correctness and routing are preserved
no systematic regression appears
but efficiency evidence is weak/noisy
```

This is acceptable if the Phase 7d change remains low-risk.

---

## REVISE

Use when the intended refined-search optimization is valid but causes a localized reproducible side effect.

Identify the smallest correction, but do not implement it during this evaluation.

---

## REVERT TO PHASE 5

Use when Phase 7d:

```text
fails to reproduce any meaningful benefit
or
systematically weakens Phase 5 routing/recovery behavior
```

---

# 41-Task Gate

Do not run the normal 41-task evaluation unless the controlled result is:

```text
ACCEPT PHASE 7D
```

or:

```text
ACCEPT WITH CAVEATS
```

If the controlled result is unfavorable, stop.

---

# If the Controlled Gate Passes

Then run the unchanged normal 41-task evaluation.

Use the current repository and Phase 7d Skill.

Compare the final normal run against Phase 5.

Because normal Skill invocation is stochastic, report separately:

```text
full aggregate
Skill-loaded cohort
Skill-not-loaded cohort
```

Do not confuse invocation variance with body behavior.

---

# Required Deliverables

Provide:

1. Experimental setup.
2. Environment/runtime information.
3. Phase 5 and Phase 7d Skill hashes.
4. Verification that Phase 7d differs only by the intended rule.
5. 18-task controlled aggregate comparison.
6. Per-command comparison.
7. Per-task paired trajectory table.
8. Refined-search / `find` substitution analysis.
9. `level3-007` positive-control analysis.
10. Relationship-ordering guard analysis.
11. `smoke-001` recovery analysis.
12. Help/retry analysis.
13. Manual exploration analysis.
14. Token distribution and outlier analysis.
15. Recovery distribution.
16. Rule-causality assessment.
17. Shared inefficiency inventory for Phase 9.
18. Final decision:

    * `ACCEPT PHASE 7D`
    * `ACCEPT WITH CAVEATS`
    * `REVISE`
    * `REVERT TO PHASE 5`
19. If the gate passes, normal 41-task Phase 7d evaluation.

---

# Final Principle

Do not try to prove that Phase 7d is always cheaper on every stochastic run.

The required standard is:

```text
Phase 5 stability
+
no reproducible regression
+
a credible efficiency bias from removing redundant semantic work
```

The final question is:

```text
When Skill invocation is controlled across the full 18-task cohort,
does the current Phase 7d candidate preserve Phase 5's stable behavior
well enough to justify promotion to the normal 41-task evaluation?
```
