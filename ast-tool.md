# Phase 7a — Targeted Trajectory Analysis

## Objective

Analyze the Phase 7a evaluation results and determine whether the observed changes relative to the Phase 5 baseline are:

1. normal run-to-run variation, or
2. a behavioral regression caused by the compressed `Skill.md`.

Do **not** modify `Skill.md`, AST Tool, the evaluation tasks, or the metrics collector during this analysis.

This is an investigation-only task.

---

## Baseline

Use Phase 5 as the stable baseline:

```text
tests                      41
successes                  37
success rate              90.24%

total tool calls           518
AST Tool calls              69
AST failures                 9
AST retries                  9

grep                         15
read                        252
glob                         12

avg recovery distance      1.44
max recovery distance         2

total tokens            158,303
avg tokens/test           3,861
```

Important Phase 5 command usage:

```text
search       30
callers      14
references    6
callees       3
find         12
symbols       1
```

---

## Phase 7a Result

```text
tests                      41
successes                  38
success rate              92.68%

total tool calls           545
AST Tool calls              60
AST failures                 9
AST retries                 10

grep                         31
read                        275
glob                         19

avg recovery distance      2.56
max recovery distance         6

total tokens            156,439
avg tokens/test         3,815.6
```

Phase 7a command usage:

```text
search       25
callers      14
references    6
callees       4
find          6
symbols       2
children      1
top_level     1
```

The trace also contains:

```text
"2": 1
```

This may be the already-known trace-parser false positive from:

```bash
which ast-tool 2>/dev/null
```

Do not treat `"2"` as a real AST Tool command unless the raw trace proves otherwise.

---

# Main Questions

The analysis should answer the following questions.

## 1. Why did recovery distance worsen?

Phase 7a recovery distances are:

```text
1, 1, 1, 1, 6, 5, 4, 2, 2
```

Identify the tests corresponding to recovery distances:

```text
6
5
4
```

For each of them:

* reconstruct the relevant tool trajectory;
* identify the initial AST Tool failure;
* identify each action taken before useful recovery;
* compare it with the same test's Phase 5 trajectory;
* determine whether Phase 7a performed unnecessary exploration;
* determine whether a removed or compressed Skill.md instruction could plausibly explain the difference.

Do not infer causality merely from correlation.

---

## 2. Why did `find` usage drop from 12 to 6?

Identify all Phase 5 tests that used `find`.

For each, determine whether Phase 7a:

```text
A. still used find
B. solved the task with another AST command
C. replaced find with grep/read/glob
D. no longer needed the query because the trajectory changed earlier
E. failed to use AST Tool where it should have
```

Pay particular attention to cases in category C or E.

The main question is not whether `find` itself must remain at exactly 12 calls.

The question is whether:

```text
targeted AST structural query
```

was replaced by:

```text
grep
→ read
→ manual reasoning
```

because of Skill.md compression.

---

## 3. Why did `search` usage decrease from 30 to 25?

Perform the same comparison for `search`.

Identify which Phase 5 `search` calls disappeared in Phase 7a.

Classify each disappearance as:

```text
benign:
  unnecessary query avoided
  equivalent targeted semantic route used
  task solved earlier

or

suspicious:
  replaced by grep/glob/manual file discovery
  broader context was read instead
  agent lost an obvious symbol-routing cue
```

Do not assume lower AST usage is automatically bad.

Only flag it when the replacement trajectory is less targeted or more expensive.

---

## 4. Explain the increase in grep/glob/read

Compare:

```text
               Phase 5    Phase 7a

grep              15          31
glob              12          19
read             252         275
```

Determine which tests account for most of the increase.

Produce a ranked list of the largest per-test increases in:

```text
grep
glob
read
```

Then inspect those trajectories.

Determine whether the increase came from:

* a small number of outlier tests;
* a broad behavioral shift across many tests;
* changed task success paths;
* AST Tool recovery;
* unrelated coding/editing work;
* Skill.md routing changes.

This distinction is important.

For example:

```text
3 tests causing +15 grep calls
```

is materially different from:

```text
15 tests each adding one grep call
```

---

## 5. Investigate the additional success

Phase 5 succeeded on 37 tests.

Phase 7a succeeded on 38.

Identify the test that changed from failure to success.

Compare its Phase 5 and Phase 7a trajectories.

Determine:

* what caused Phase 5 to fail;
* what changed in Phase 7a;
* whether the improved result plausibly came from Skill.md compression;
* whether it came from stochastic agent behavior instead;
* whether the successful Phase 7a path should be considered desirable and repeatable.

Do not count the extra success as evidence for compression unless the trajectory supports that interpretation.

---

# Skill.md Diff Analysis

Compare the Phase 5 and Phase 7a versions of `Skill.md`.

Create a concise inventory of every meaningful removed, merged, or shortened instruction.

For each change, classify it as:

```text
SAFE
LIKELY SAFE
POSSIBLE BEHAVIORAL EFFECT
LIKELY BEHAVIORAL EFFECT
```

Focus specifically on text related to:

```text
search
find
semantic queries
fallback to grep
retry behavior
failure recovery
command selection
when AST Tool should be preferred
```

Do not treat wording changes as behaviorally relevant merely because they are different.

Look for actual evidence in the trajectories.

---

# Cross-Check Hypotheses Against Traces

For every suspected Skill.md regression, require evidence of this form:

```text
Skill.md change
        ↓
plausible change in agent decision
        ↓
observable trajectory difference
        ↓
measurable cost or routing change
```

For example:

```text
Removed explicit "use find for AST structure"
        ↓
agent stops selecting find
        ↓
same tests now use grep + read
        ↓
more exploration / longer recovery
```

would be meaningful evidence.

In contrast:

```text
find usage decreased
        ↓
therefore compression caused regression
```

is not sufficient.

---

# Token Analysis

Total tokens improved slightly:

```text
158,303 → 156,439
```

while grep/read/tool calls increased.

Investigate why.

If possible, separate:

```text
Skill.md prompt-token reduction

from

trajectory/output-token changes
```

Estimate how much of the total token reduction is attributable simply to the smaller Skill.md being included in each test.

For example, if the compressed skill saves approximately `N` tokens per test:

```text
N × 41
```

gives an approximate fixed prompt-saving contribution.

Then assess whether the actual agent trajectories themselves became:

```text
more token efficient
approximately neutral
less token efficient
```

after removing that fixed Skill.md saving.

This is important before deciding whether further Phase 7b compression is worthwhile.

---

# Outlier Analysis

Do not rely only on aggregate metrics.

Identify tests that are outliers in any of the following:

```text
recovery distance
grep increase
read increase
glob increase
AST retries
token increase
elapsed-time increase
```

For the most significant outliers, compare Phase 5 and Phase 7a side by side.

A small number of pathological trajectories should not automatically be interpreted as a global routing regression.

---

# Required Output

Produce a report with the following sections.

## 1. Executive Summary

State one of:

```text
A. Phase 7a behavior is effectively preserved.
B. Phase 7a has a minor localized regression.
C. Phase 7a has a meaningful routing regression.
D. Evidence is inconclusive.
```

Explain the decision briefly.

---

## 2. Phase 5 vs Phase 7a Metric Comparison

Include the important metrics and percentage/absolute changes.

Do not evaluate metrics independently; explain relationships between them.

---

## 3. Long-Recovery Cases

Analyze the tests corresponding to recovery distances 6, 5, and 4.

---

## 4. `find` Usage Analysis

Explain all meaningful `find` disappearances.

---

## 5. `search` Usage Analysis

Explain all meaningful `search` disappearances.

---

## 6. grep / glob / read Increase

Identify where the additional exploration came from and whether it represents a broad or localized shift.

---

## 7. Additional Successful Test

Explain the 37 → 38 success change.

---

## 8. Skill.md Diff Correlation

List only Skill.md changes that have plausible trajectory evidence.

Separate proven evidence from speculation.

---

## 9. Token Decomposition

Estimate:

```text
fixed savings from smaller Skill.md
vs
trajectory-related token change
```

If the available logs do not permit exact decomposition, provide the best defensible estimate and clearly label assumptions.

---

## 10. Recommendation

Choose one:

```text
ACCEPT
ACCEPT WITH MINOR RESTORATION
REVISE
REJECT
```

If recommending restoration, specify the **smallest possible wording** that should be restored.

Do not propose broad Skill.md expansion.

Do not proceed to Phase 7b automatically.

---

# Analysis Principles

Use these principles throughout:

```text
Do not optimize individual AST Tool metrics in isolation.

Do not require exact command-count equality between runs.

Distinguish stochastic variation from systematic behavior change.

Prefer per-test trajectory evidence over aggregate speculation.

Treat grep/read increases as a problem only when they replace a cheaper targeted path.

Treat reduced AST usage as a problem only when routing quality or context efficiency worsens.

Do not modify anything during the investigation.
```

The final question to answer is:

```text
Did Phase 7a preserve the useful Phase 5 routing behavior,
or did compression remove instructions that were quietly
important to keeping the agent on targeted AST Tool paths?
```