# Phase 8 — Final Quantitative Evaluation

## Objective

Perform the final quantitative evaluation of the accepted Phase 7b state.

Phase 8 is **not an optimization phase**.

Do not modify:

* `SKILL.md`
* AST Tool implementation
* semantic resolution
* CLI behavior
* evaluation tasks
* recovery behavior
* metrics collection logic, unless a clearly identified metrics bug prevents valid comparison

The purpose of this phase is to determine whether the final Phase 7b system is measurably better than the established stable baseline at the **Coding Agent level**.

The primary comparison is:

```text
Phase 5 stable baseline
        ↓
Phase 7b final candidate
```

Intermediate phases may be included for context, but Phase 5 is the formal behavioral baseline.

---

# Evaluation Principle

The optimization target is not AST Tool in isolation.

The optimization target is:

```text
Coding Agent efficiency
```

Evaluate the final system across:

```text
correctness
targeted semantic routing
manual exploration cost
recovery behavior
tool-use efficiency
latency
token efficiency
```

Do not declare success or failure based on one metric alone.

In particular:

```text
lower AST calls != automatically better
higher AST calls != automatically worse

lower tool calls != automatically better
higher total tokens != automatically worse without trajectory analysis
```

Always interpret metrics together.

---

# Final Candidate

Use the committed Phase 7b state.

Do not modify it before or during the final evaluation.

Phase 7b reduced the accepted Phase 7a Skill from approximately:

```text
1546 tokens
→
809 tokens
```

while preserving the explicit routing contract:

```text
Find symbol       → search
Find callers      → callers
Find references   → references
Find callees      → callees
Find file symbols → symbols
Need AST structure → find
```

and the intended recovery/fallback rules.

---

# Formal Baseline

Use Phase 5 as the stable baseline.

Phase 5 metrics:

```text
tests                      41
successes                  37
failures                    4
success rate              90.24%

total tool calls           518
avg tool calls/test       12.63

AST Tool calls              69
AST failures                 9
AST failure rate          13.04%
AST retries                  9

avg recovery distance      1.44
max recovery distance         2

grep                         15
read                        252
glob                         12

total tokens            158,303
avg tokens/test           3,861

elapsed                 2224.27 sec
```

Use this as the main comparison point.

---

# Contextual Phases

For historical context, include:

```text
Phase 1
Phase 2
Phase 5
Phase 7a
Phase 7b
```

Treat:

```text
Phase 3
Phase 6
```

as rejected experimental branches.

Do not mix their results into the main success criterion.

They may be cited only to illustrate known failure modes such as:

```text
AST usage collapse
grep fallback increase
token increase
routing regression
```

---

# Evaluation Dataset

Run the same 41 evaluation tasks used for the previous phases.

Do not change prompts, repositories, expected files, validators, or timeout rules.

Ensure the environment is valid before starting the run, including required compiler/tool availability.

Infrastructure failures must not be counted as agent failures.

If an infrastructure failure occurs:

1. identify it explicitly;
2. fix only the environment issue;
3. rerun the affected task;
4. retain enough information to distinguish the invalid run from the valid rerun.

The final comparison must contain 41 valid results.

---

# Metrics to Collect

Collect all previously established metrics.

## Correctness

```text
tests
successes
failures
success rate
```

---

## Tool Usage

```text
total tool calls
average tool calls/test

AST Tool calls
AST Tool failures
AST Tool failure rate
AST Tool retries
AST Tool help calls

grep
glob
read
bash
edit
```

---

## AST Tool Commands

Collect per-command usage:

```text
search
callers
references
callees
find
symbols
outline
children
parent
range
other parsed commands
```

Also collect:

```text
AST failures by command
```

Identify parser false positives separately.

Do not count known shell-redirection artifacts such as:

```text
which ast-tool 2>/dev/null
```

as real AST Tool commands.

---

## Recovery

Collect:

```text
all recovery distances
average recovery distance
maximum recovery distance
```

For unusually long recovery cases, retain the task ID and trajectory.

---

## Time

Collect:

```text
total elapsed time
average elapsed time/test
```

---

## Tokens

Collect:

```text
input tokens
output tokens
total tokens
average tokens/test
```

Do not stop at aggregate token totals.

Phase 8 must also compute per-test token distributions.

---

# Required Per-Test Metrics

For every task, produce at least:

```text
task ID
success/failure
total tool calls
AST Tool calls
grep
glob
read
tokens
elapsed time
skill invocation count
AST command sequence
```

This data is required for outlier analysis.

---

# Three-Layer Evaluation

Phase 8 must evaluate the result at three different levels.

---

## Layer 1 — Full 41-Task Aggregate

This is the primary Agent-level result.

Compare Phase 5 and Phase 7b across the full 41-task set.

Answer:

```text
Did the final system become better overall?
```

Evaluate:

```text
success rate
tool calls
manual exploration
AST Tool usage
recovery
elapsed time
tokens
```

Do not interpret any one metric in isolation.

---

## Layer 2 — Per-Test Delta Analysis

Aggregate averages can be distorted by a few stochastic outliers.

For every task, compute:

```text
Phase 7b - Phase 5
```

for at least:

```text
tokens
tool calls
AST calls
grep
read
elapsed time
```

Then report the distribution.

For token deltas, include:

```text
median
p75
p90
minimum
maximum
```

Also report:

```text
number of tests improved
number approximately unchanged
number worsened
```

Use a clearly stated tolerance when defining "approximately unchanged".

The purpose is to distinguish:

```text
broad regression
```

from:

```text
a few high-cost outliers
```

For example:

```text
30 tests near zero
3 tests extremely expensive
```

must not be interpreted the same way as:

```text
30 tests moderately worse
```

even if the aggregate token total is similar.

---

## Layer 3 — Skill-Invocation Cohorts

Skill invocation varies stochastically between runs.

Therefore separate tests into:

```text
A. Skill loaded in both Phase 5 and Phase 7b
B. Skill loaded in only one run
C. Skill loaded in neither run
```

Group A is the strongest evidence for whether the final compressed Skill body preserves routing behavior.

For Group A, compare:

```text
success
AST Tool calls
search
callers
references
callees
find
grep
glob
read
recovery
tokens
tool calls
elapsed time
```

Look for trajectory changes such as:

```text
Phase 5:
search → callers → targeted reads

Phase 7b:
grep → broad reads → manual reasoning
```

A systematic change of this form would be evidence of routing degradation.

A difference that occurs only when the Skill was not loaded must not be blamed on Skill body compression.

---

# Routing Quality Analysis

Do not use raw AST Tool call count as the main quality metric.

Evaluate whether trajectories remain targeted.

Good examples:

```text
search → callers
search → references
search → find
callers → refined search → callers
```

Potential regressions:

```text
grep → read many files → grep → read

broad workspace dump

repeated unchanged failed AST command

routine help exploration

semantic query skipped despite an obvious direct mapping
```

Classify observed changes as:

```text
BENIGN
SUSPICIOUS
REGRESSION
```

Require trajectory evidence before labeling a change as regression.

---

# Manual Exploration Analysis

Compare:

```text
grep
glob
read
```

both in aggregate and per test.

Identify the tests responsible for the largest increases.

For each major outlier, determine whether the increase came from:

```text
AST Tool avoidance
semantic recovery
implementation work
fixture inspection
unrelated stochastic reasoning
```

Do not assume all grep/read usage is bad.

The issue is whether broad manual exploration replaced a cheaper targeted semantic path.

---

# Recovery Analysis

Compare Phase 5 and Phase 7b recovery behavior.

Report:

```text
average recovery distance
maximum recovery distance
recovery-distance distribution
```

Inspect all unusually long recovery paths.

For each, answer:

```text
What failed?
What was tried next?
Was an unchanged command retried?
Was help used unnecessarily?
Did the error message lead to a cheap correction?
Was the Skill loaded at the time?
```

Distinguish tool/recovery regressions from runs where the Skill was never invoked.

---

# Token Analysis

Token analysis must be more robust than aggregate total-token comparison.

Phase 8 must report:

```text
total tokens
average tokens/test
median tokens/test
p75
p90
max
```

Also compute per-test token deltas between Phase 5 and Phase 7b.

Identify how much of the aggregate difference is caused by the largest outliers.

For example, report:

```text
Top 1 outlier contribution
Top 3 outlier contribution
Top 5 outlier contribution
```

to the total token delta.

---

# Skill Compression Cost Adjustment

Estimate the fixed prompt saving produced by the smaller Phase 7b Skill.

Let:

```text
S5  = approximate Phase 5 Skill tokens
S7b = approximate Phase 7b Skill tokens
N   = comparable Skill invocations
```

Estimate:

```text
fixed Skill saving
≈
(S5 - S7b) × N
```

If Phase 5 Skill size is not readily available, use Phase 7a or another known reference and clearly state the limitation.

This estimate is secondary.

Do not over-interpret it because prompt caching and runtime accounting may make exact attribution impossible.

The purpose is only to distinguish:

```text
instruction cost
```

from:

```text
trajectory/output cost
```

---

# Token Accounting Consistency Check

Phase 7 results showed cases where:

```text
tool calls ↓
reads ↓
elapsed time ↓
but reported output tokens ↑ sharply
```

Therefore explicitly check whether token reporting is comparable between runs.

Inspect:

```text
input/output token accounting
model/runtime configuration
cache behavior if observable
log format/version
evaluation harness version
```

Do not modify the harness merely to force metrics to match.

If token accounting is not fully comparable, report that limitation and rely more heavily on:

```text
paired per-test deltas
median behavior
trajectory evidence
tool-use metrics
elapsed time
```

Do not silently treat incompatible token accounting as precise efficiency evidence.

---

# Correctness Analysis

For every Phase 7b failure:

1. identify the task;
2. identify the validator failure;
3. compare with Phase 5;
4. determine whether it is:

   * a new agent regression,
   * an existing task/fixture defect,
   * stochastic implementation failure,
   * infrastructure failure.

Do not count known baseline fixture defects as evidence against Phase 7b unless the final system introduced or exposed them differently.

Also inspect improvements:

```text
Phase 5 failure
→
Phase 7b success
```

and determine whether they are likely systematic or stochastic.

---

# Statistical Interpretation

There are only 41 tasks.

Do not overstate small differences.

For example:

```text
37/41 vs 38/41
```

should be treated as a one-task difference requiring trajectory inspection, not automatically as proof of improvement or regression.

Prefer language such as:

```text
preserved
likely improved
localized regression
outlier-driven
inconclusive
```

unless the evidence is strong.

Do not manufacture statistical significance from this small sample.

---

# Final Comparison Table

Produce a main table containing:

```text
Metric
Phase 5
Phase 7b
Absolute change
Relative change
Assessment
```

Include at least:

```text
success rate
total tool calls
AST Tool calls
AST failures
AST retries
grep
glob
read
average recovery distance
maximum recovery distance
elapsed time
total tokens
median tokens/test
p90 tokens/test
```

---

# Historical Trend Table

Also provide a compact historical table for:

```text
Phase 1
Phase 2
Phase 5
Phase 7a
Phase 7b
```

Include only the core metrics:

```text
success rate
total tool calls
AST Tool calls
grep
read
total tokens
elapsed time
```

Do not force comparisons where metrics were collected differently.

Clearly mark unavailable or non-comparable values.

---

# Final Decision

Choose exactly one:

```text
ACCEPT FINAL
ACCEPT WITH CAVEATS
REVISE
REJECT
```

---

## ACCEPT FINAL

Use when the evidence supports:

```text
correctness preserved
+
targeted routing preserved
+
manual exploration acceptable
+
recovery acceptable
+
agent-level efficiency improved or clearly not worse
```

Minor stochastic differences are acceptable.

---

## ACCEPT WITH CAVEATS

Use when the final system is still preferable overall, but one metric—such as token accounting—cannot be compared reliably.

State the caveat precisely.

---

## REVISE

Use when the overall direction is promising but a localized, reproducible issue should be corrected before declaring the final state.

Do not propose unrelated feature work.

---

## REJECT

Use when Phase 7b produces a systematic regression such as:

```text
targeted AST routing collapse
manual exploration broadly increases
correctness materially drops
recovery consistently worsens
agent-level cost increases across many tests
```

Do not reject based solely on one aggregate token outlier.

---

# Final Report Structure

Produce the final report in this order:

## 1. Executive Summary

State the final decision and the main evidence.

## 2. Final Phase 5 vs Phase 7b Comparison

Main quantitative table.

## 3. Correctness

Failures, improvements, and causal classification.

## 4. Routing Behavior

AST Tool usage and targeted semantic trajectories.

## 5. Manual Exploration

grep/glob/read analysis and major contributors.

## 6. Recovery

Recovery-distance comparison and notable trajectories.

## 7. Tool and Latency Efficiency

Tool-call and elapsed-time comparison.

## 8. Token Analysis

Aggregate, median, percentiles, per-test deltas, and outlier contribution.

## 9. Skill-Invocation Cohort Analysis

Especially tests loading the Skill in both runs.

## 10. Historical Trend

Phase 1 → Phase 2 → Phase 5 → Phase 7a → Phase 7b.

## 11. Limitations

Include stochasticity, fixture defects, token-accounting limitations, and any unavailable paired traces.

## 12. Final Recommendation

Choose:

```text
ACCEPT FINAL
ACCEPT WITH CAVEATS
REVISE
REJECT
```

and explain why.

---

# Do Not Make Further Changes

Phase 8 is an evaluation phase.

Do not modify the final candidate in response to minor metric differences during this task.

If a real regression is discovered:

```text
identify it
demonstrate the causal trajectory
recommend the smallest follow-up
```

but leave the committed Phase 7b state unchanged.

Do not begin Phase 9 automatically.

---

# Final Question

The report must ultimately answer:

```text
Compared with the Phase 5 stable baseline,
does the committed Phase 7b system provide a better
Coding Agent overall without sacrificing the targeted
semantic-routing behavior that AST Tool is intended to provide?
```

Judge the final system at the agent level, not by Skill size or AST Tool metrics alone.
