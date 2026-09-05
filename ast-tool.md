# Phase 7d.1 — Repeated Controlled Guard Validation

## Objective

Determine whether the guard-regression signals observed in Phase 7d are systematic consequences of the Phase 7d Skill change or ordinary run-to-run stochastic variation.

Phase 7d.1 is an evaluation-only phase.

Do not modify the Phase 7d candidate.

Do not add new Skill wording.

Do not revert the existing Phase 7d backport.

The goal is:

```text
Hold Phase 7d constant
+
repeat the same controlled guard tasks
+
measure trajectory distributions
+
decide whether the observed regressions are systematic
```

The central question is:

```text
Does Phase 7d reliably produce worse relationship/recovery behavior
than Phase 5 on the guard cohort,
or were the previous Phase 7d runs unlucky samples?
```

---

# Background

Phase 7d starts from the exact Phase 5 Skill and adds only one narrow rule:

```text
If a refined `search` already identifies the exact symbol or member needed,
do not add a redundant `find` solely to locate it; use `find` when AST
structure or node detail is required.
```

This change reproducibly improved the intended structural-lookup behavior.

For example, `level3-007` changed from:

```text
Phase 5:
search ×3 → find ×6
```

to:

```text
Phase 7d:
search ×3 or ×4
```

while preserving correctness and reducing redundant AST work.

However, the targeted Phase 7d guard cohort showed more AST failures than Phase 5.

Previous targeted results:

```text
Phase 5:
AST failures = 3

Phase 7d run 1:
AST failures = 5

Phase 7d run 2:
AST failures = 6
```

The problematic trajectories were mainly:

```text
level2-008
level3-008
smoke-001
```

but these behaviors are not causally explained by the new `find` rule.

Phase 7d.1 must determine whether this difference is reproducible.

---

# Experimental Principle

Do not change anything except the Skill version being compared.

The experiment must compare:

```text
Phase 5 Skill
vs
Phase 7d Skill
```

under the same controlled conditions.

Hold constant:

```text
task
fixture
AST Tool implementation
evaluation harness
permissions
timeouts
environment
model/runtime configuration where observable
Skill invocation timing
```

For every run:

```text
semantic-analysis must be loaded exactly once
as the first tool action before repository exploration
```

If the Skill is not invoked first, mark the run invalid and rerun it.

---

# Guard Cohort

Use the following guard tasks:

```text
level2-006
level2-008
level3-008
smoke-001
```

These are the primary cohort.

They cover:

```text
identity discovery before relationship queries
under-qualified callers/references recovery
relationship-query ordering
error-directed semantic recovery
help behavior
retry behavior
```

Do not add unrelated tasks unless needed to diagnose a discovered anomaly.

---

# Optional Positive Control

Also include:

```text
level3-007
```

as a positive-control task.

The purpose is to confirm that the intended Phase 7d improvement remains present while the guard behavior is being measured.

Do not include its metrics in the primary guard-regression decision unless explicitly separated.

The expected positive-control property is:

```text
Phase 7d should avoid redundant `find`
after refined `search` has already identified the exact targets.
```

---

# Number of Repeats

Run each guard task multiple times for both Skill versions.

Preferred:

```text
5 runs per task per Skill version
```

This gives:

```text
4 guard tasks
× 2 Skill versions
× 5 repeats
= 40 primary runs
```

If resource constraints are significant, use a minimum of:

```text
3 runs per task per Skill version
```

but prefer 5 because the previous Phase 7d runs already showed substantial within-version variation.

Do not compare one Phase 5 run against many Phase 7d runs.

Both arms must be repeated.

---

# Run Ordering

Reduce temporal/runtime bias.

Do not run all Phase 5 samples first and all Phase 7d samples later if avoidable.

Prefer interleaving:

```text
task A / Phase 5
task A / Phase 7d
task A / Phase 5
task A / Phase 7d
...
```

or alternate the starting arm:

```text
repeat 1: Phase 5 → Phase 7d
repeat 2: Phase 7d → Phase 5
```

The exact schedule is less important than avoiding a strong systematic ordering bias.

Record the execution order.

---

# Environment Recording

For every run, record if observable:

```text
timestamp
model identifier
runtime/version
repository revision
AST Tool revision
evaluation harness revision
Skill version/hash
```

If any of these cannot be recorded, state that limitation.

If the model/runtime changes during the experiment, do not silently combine those runs.

Separate or invalidate them as appropriate.

---

# Metrics Per Run

Collect for every run:

```text
task ID
Skill version
repeat number
success/failure

total tool calls

AST Tool calls
AST failures
AST retries
AST help calls

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

tokens
elapsed time

AST command sequence
relevant trajectory
```

---

# Primary Metrics

The primary purpose is to characterize guard behavior.

Prioritize:

```text
AST failures
AST retries
AST help calls
recovery distance
relationship-command ordering
```

Secondary metrics:

```text
tool calls
AST calls
tokens
elapsed
```

Do not reject Phase 7d merely because token totals are noisy if routing/recovery behavior is equivalent.

---

# Per-Task Trajectory Classification

For every run classify the trajectory.

## `level2-006`

Preferred pattern:

```text
search
→ callers
```

Potential regression:

```text
under-qualified callers
→ failure
→ search
→ callers
```

Classify each run as:

```text
IDENTITY_FIRST
RELATIONSHIP_FIRST_RECOVERED
OTHER
```

---

## `level2-008`

Use the same core classification:

```text
IDENTITY_FIRST
RELATIONSHIP_FIRST_RECOVERED
OTHER
```

Track:

```text
number of failures before successful callers
number of retries
```

This task is especially important because both prior Phase 7d runs showed:

```text
callers → search → callers
```

while the reference Phase 5 run used:

```text
search → callers
```

---

## `level3-008`

Classify as:

```text
IDENTITY_FIRST
RELATIONSHIP_FIRST_RECOVERED
EXTRA_SEMANTIC_EXPLORATION
OTHER
```

Track whether the run begins with an under-qualified relationship query.

Also record any use of:

```text
symbols
extra search
extra callers/callees
```

---

## `smoke-001`

This task should be analyzed separately because it has shown high stochasticity.

Track the complete failure/recovery sequence.

Specifically record:

```text
initial malformed find handling
references argument choice
callers argument choice
help usage
unchanged retry
search-based disambiguation
recovery distance
```

Classify each run as:

```text
SHORT_RECOVERY
MODERATE_RECOVERY
LONG_RECOVERY
```

Use an explicit definition, for example:

```text
SHORT      max recovery distance <= 2
MODERATE   max recovery distance 3
LONG       max recovery distance >= 4
```

---

# Do Not Use Aggregate Failure Count Alone

A total such as:

```text
Phase 5 failures = 12
Phase 7d failures = 15
```

is not sufficient by itself.

Analyze where the failures occur.

The important distinction is:

```text
same task repeatedly worse under Phase 7d
```

versus:

```text
failures distributed inconsistently across both versions
```

Systematic evidence requires consistency.

---

# Per-Task Distribution Report

For each task and Skill version report:

```text
runs
successes

mean AST failures
median AST failures
failure-count range

mean retries
median retries

help-call frequency

mean recovery distance
max recovery distance

trajectory-class frequencies
```

For example:

```text
level2-008

Phase 5:
IDENTITY_FIRST                4/5
RELATIONSHIP_FIRST_RECOVERED  1/5

Phase 7d:
IDENTITY_FIRST                1/5
RELATIONSHIP_FIRST_RECOVERED  4/5
```

would be meaningful evidence.

In contrast:

```text
Phase 5:
3/5 vs 2/5

Phase 7d:
2/5 vs 3/5
```

would be much weaker evidence.

---

# Paired Comparison

Where practical, compare repeat pairs run under similar temporal conditions.

For each pair record:

```text
Δ failures
Δ retries
Δ recovery distance
Δ tool calls
Δ tokens
Δ elapsed
```

Do not require exact deterministic pairing, but use it to identify broad runtime drift.

---

# Systematic Regression Standard

Classify a Phase 7d guard regression as **SYSTEMATIC** only when the evidence shows a repeated directional difference.

Examples of strong evidence:

```text
Phase 5 repeatedly chooses search-first
while Phase 7d repeatedly chooses relationship-first
```

or:

```text
Phase 5 consistently recovers in <=2 steps
while Phase 7d repeatedly reaches >=4
```

or:

```text
Phase 7d consistently adds failures/retries on the same task
across most repeated runs
```

One or two outlier runs are not enough.

---

# Stochastic Variation Standard

Classify the difference as **STOCHASTIC / NOT SYSTEMATIC** when:

```text
both versions show the same trajectory variants
```

and their frequencies overlap substantially.

Also favor this conclusion when:

```text
within-version variance
≈ or >
between-version difference
```

For example:

```text
Phase 7d smoke failures:
3, 5, 2, 4, 2

Phase 5:
2, 4, 3, 2, 4
```

would not support a meaningful Phase 7d regression.

---

# Possible Interaction

Use **POSSIBLE INTERACTION** when:

```text
Phase 7d appears directionally worse
```

but the sample is not strong enough to distinguish a true Skill interaction from stochastic variation.

Do not add new wording during this phase.

Record the possible interaction for a later narrow experiment.

---

# Positive-Control Analysis

For `level3-007`, verify that Phase 7d continues to show the intended benefit.

Report per version:

```text
find calls
search calls
AST calls
total tools
tokens
elapsed
correctness
```

A strong result is:

```text
Phase 7d repeatedly avoids redundant find calls
without introducing manual fallback or failures.
```

This establishes that the known benefit remains reproducible.

---

# Statistical Treatment

Do not overstate formal significance from a small sample.

Simple descriptive statistics are sufficient.

Report:

```text
mean
median
range
frequency / proportion
```

If convenient, confidence intervals or simple non-parametric comparisons may be included, but they are not required.

Do not manufacture a p-value threshold as the acceptance criterion.

Trajectory consistency matters more.

---

# Decision Matrix

After repeated runs, choose one of four conclusions.

## A. GUARD BEHAVIOR PRESERVED

Use when:

```text
Phase 5 and Phase 7d trajectory distributions are substantially similar
```

and the previous 5/6 vs 3 failure difference is not reproducible.

Then:

```text
Phase 7d's known efficiency backport is supported
and the previous guard failure should be treated as sampling variance.
```

Recommend proceeding to the full 18-task controlled cohort.

---

## B. MINOR STOCHASTIC DIFFERENCE

Use when Phase 7d is slightly worse in one metric but:

```text
trajectory distributions overlap
no repeated command-order regression is clear
recovery remains broadly Phase 5-like
```

If the positive-control improvement remains strong, recommend proceeding to the 18-task cohort with a caveat.

---

## C. SYSTEMATIC GUARD REGRESSION

Use when Phase 7d repeatedly produces:

```text
relationship-before-identity
extra failures/retries
longer recovery
```

relative to Phase 5 on the same tasks.

Do not proceed to the 18-task cohort.

Do not immediately add generic wording.

Instead identify the smallest reproducible interaction and design a separate narrowly controlled follow-up.

---

## D. INCONCLUSIVE

Use when runtime/model variation or sample variance remains too high.

Recommend additional repeats or a stricter runtime-controlled experiment.

Do not modify the Skill.

---

# Phase 7d Acceptance Gate

Proceed to the full 18-task controlled evaluation when:

```text
positive-control improvement is reproduced
AND
guard behavior is classified as
GUARD BEHAVIOR PRESERVED
or
MINOR STOCHASTIC DIFFERENCE
```

Do not require Phase 7d's raw AST failure count to be numerically identical to Phase 5 in every repeat.

The gate should be based on repeated trajectory distributions, not one aggregate sample.

---

# If the Gate Passes

Do not run the 41-task full evaluation immediately unless the existing Phase 7d plan explicitly requires it.

First proceed to:

```text
full 18-task controlled Phase 5 vs Phase 7d comparison
```

using the same controlled Skill invocation.

This confirms that the backport does not introduce regressions outside the guard cohort.

Only after that result is favorable should the normal 41-task evaluation be considered.

---

# Required Report

Produce the report in this order.

## 1. Executive Summary

Choose:

```text
GUARD BEHAVIOR PRESERVED
MINOR STOCHASTIC DIFFERENCE
SYSTEMATIC GUARD REGRESSION
INCONCLUSIVE
```

State whether Phase 7d may proceed.

---

## 2. Experimental Setup

Document:

```text
tasks
repeat count
run ordering
Skill hashes
tool/runtime versions
forced Skill invocation
```

---

## 3. Aggregate Guard Metrics

Compare Phase 5 and Phase 7d across all repeated guard runs.

Include:

```text
success
AST calls
AST failures
retries
help
recovery
tools
tokens
elapsed
```

---

## 4. Per-Task Distribution

For every guard task report:

```text
failure distributions
retry distributions
recovery distributions
trajectory-class frequencies
```

---

## 5. Relationship-Ordering Analysis

Focus on:

```text
level2-006
level2-008
level3-008
```

Answer:

```text
Does Phase 7d actually make relationship-first behavior more likely?
```

---

## 6. `smoke-001` Variance Analysis

Compare the full recovery distribution.

Answer:

```text
Is Phase 7d systematically worse,
or is smoke-001 intrinsically high-variance?
```

---

## 7. Positive-Control Result

Report whether the `level3-007` improvement remains reproducible.

---

## 8. Between-Version vs Within-Version Variance

Summarize whether observed differences are larger than ordinary run-to-run variation.

---

## 9. Final Decision

Choose:

```text
PROCEED TO 18-TASK CONTROLLED EVALUATION
PROCEED WITH CAVEAT
DO NOT PROCEED
REPEAT WITH STRONGER CONTROL
```

---

# Do Not Modify Anything

During Phase 7d.1:

```text
Do not edit SKILL.md.
Do not add relationship-order wording.
Do not change recovery wording.
Do not modify AST Tool.
Do not modify evaluation tasks.
```

This phase exists specifically to avoid reacting to stochastic samples with unnecessary instruction changes.

---

# Final Question

The report must answer:

```text
When Phase 5 and Phase 7d are each sampled repeatedly
under the same forced-Skill conditions,
is the apparent Phase 7d guard regression reproducible,
or is it ordinary trajectory variance?
```

If the regression is not reproducible while the targeted `find` improvement is reproducible, Phase 7d has stronger evidence that it genuinely improves Phase 5.
