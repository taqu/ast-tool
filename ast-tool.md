# Phase 9b.2 — Final Normal-Routing Agent-Level Evaluation

## Objective

Evaluate the complete current system under **normal agent routing**, without forced Skill invocation, and determine whether the accepted Phase 8 semantic capability set improves the Coding Agent as a whole.

Phase 9b.1 answered:

```text
Does Phase 8 improve the agent
when semantic routing is forced identically?
```

Phase 9b.2 must answer:

```text
Does the complete Phase 8 system improve
under ordinary Skill invocation,
first-action routing,
and normal agent behavior?
```

This is the final whole-system evaluation.

Do not modify semantic behavior during this phase.

---

# Baseline Conclusion from Phase 9b.1

Phase 9b.1 concluded:

```text
CONFIRM PHASE 8
```

Fresh controlled evidence established:

```text
Phase 8a
    partial-FQN target-resolution defect removed

Phase 8b
    receiver-typed member relationships restored

Phase 8c
    declaration-only callees false-empty removed

semantic precision
    preserved

unaffected guards
    stable

whole controlled cohort
    directionally cheaper
```

Therefore Phase 9b.2 does not need to re-prove Phase 8 internals.

Its purpose is to measure whether those capabilities remain beneficial when the agent is free to choose its own initial route.

---

# Systems Under Comparison

Use the same two frozen arms as Phase 9b.1.

## Arm A — Pre-Phase-8 System

Use the verified pre-Phase-8 revision:

```text
ccc1fbb650bf058aa11602134d4e4fa1795cb98e
```

with the accepted Phase 7d `semantic-analysis` Skill.

It must exclude:

```text
Phase 8a
Phase 8b
Phase 8c
```

---

## Arm B — Accepted Phase 8 System

Use the exact accepted Phase 8 implementation from Phase 9b.1.

It must contain:

```text
Phase 8a
Phase 8b
Phase 8c
```

and the exact same Phase 7d Skill as Arm A.

---

# Freeze Requirements

Before any measured run, record and freeze:

```text
Arm A Git revision
Arm B Git revision

Arm A binary SHA-256
Arm B binary SHA-256

Skill SHA-256

Claude Code version
model identifier

harness revision/hash
task definitions
fixture revisions
validator revisions
```

Verify binary and Skill hashes before and after every run.

No production or Skill change is permitted once measurement begins.

---

# Normal Routing Condition

Do **not** use:

```text
AST_TOOL_CONTROLLED_SKILL=1
```

or any equivalent forced-routing instruction.

The agent must decide normally whether to invoke:

```text
semantic-analysis
```

or use another route.

Record the actual first tool action and Skill invocation behavior from traces.

---

# Skill Invocation Measurement

For every run record:

```text
semantic-analysis invoked?
invocation count
invocation position
first action
other Skill invoked?
```

Classify routing as:

```text
A. semantic-analysis first
B. another Skill first
C. Agent/subagent exploration first
D. direct AST Tool first
E. Grep/Glob/Read/manual route first
```

Do not count any Skill call as `semantic-analysis`.

Use exact Skill identity.

---

# Primary Comparison

Compare:

```text
Arm A normal routing
vs
Arm B normal routing
```

using the same tasks, model, harness, environment, and repetition protocol.

The system-level comparison must include both:

```text
semantic capability quality
+
routing/invocation behavior
```

because this phase intentionally evaluates the whole Coding Agent.

---

# Cohort

Use a broad representative cohort.

Preferred option:

```text
the existing 41-task normal evaluation suite
```

if it remains current and valid.

The suite should cover:

```text
level 1
level 2
level 3
level 4
level 5
smoke
```

and include:

```text
search
find
callers
callees
references
ambiguity
relationship ordering
recovery
structural lookup
multi-file edits
API changes
distributed workflows
```

Do not silently remove difficult tasks.

---

# Phase 8 Coverage Requirement

Verify that the normal suite contains tasks capable of exercising:

```text
8a:
    partial-FQN relationship targets

8b:
    receiver-typed member relationships

8c:
    declaration→definition body identity
```

If the 41-task suite does not exercise 8c meaningfully, add one clearly labeled Phase-8c probe task to the analysis cohort, but keep its metrics separate from the historical 41-task aggregate.

Do not distort the historical suite merely to force Phase 8 coverage.

---

# Guard Cohort

Carry forward the corrected unaffected guards from Phase 9b.1:

```text
level1-001
level1-003
level2-001
level3-001
```

Do not use `level1-002` as an unaffected guard; it is confirmed Phase-8b-affected.

Track guard behavior separately even when those tasks are already part of the main suite.

---

# Repetition Strategy

A single normal 41-task run is useful but not sufficient for strong claims about invocation-sensitive behavior.

Use:

```text
one complete fresh run per arm
```

as the main full-suite comparison.

Additionally, repeat selected tasks where:

```text
Phase 8 behavior is directly exercised
or
routing differs between arms
or
a meaningful regression appears
```

Recommended:

```text
5 repeats per selected task/arm
```

Use 10 only if the first five remain strongly mixed.

Do not repeat only unfavorable Arm B tasks.

Apply the same rule symmetrically.

---

# Arm Ordering

Interleave arms where practical.

For the full suite, prefer:

```text
task1 A
task1 B

task2 B
task2 A

task3 A
task3 B
...
```

or another deterministic alternating scheme.

Do not run the entire Arm A suite followed much later by the entire Arm B suite unless unavoidable.

If full interleaving is not possible, document temporal bias.

---

# Repository Reset

Before every run:

```text
git reset --hard
git clean -fdx
```

or the existing equivalent fixture reset mechanism.

Each run must use:

```text
fresh process
fresh session
clean repository state
```

Do not reuse Claude session context.

---

# Primary Whole-System Metrics

Record for every run:

```text
success

total tools

Skill calls
semantic-analysis calls

AST calls
AST failures
AST retries
help

search
find
callers
callees
references
symbols

grep
glob
read
bash
edit

tokens
elapsed

recovery mean
recovery max
```

Also retain the full ordered trajectory.

---

# Invocation Metrics

Report:

```text
semantic-analysis invocation rate
first-action semantic-analysis rate
late invocation rate
no-Skill rate
other-Skill rate
```

Break these down by:

```text
task level
task category
arm
```

where useful.

Do not optimize for invocation rate itself.

Invocation is explanatory, not the objective.

---

# Routing Quality Metrics

For each task classify the dominant trajectory as:

```text
1. targeted semantic route

2. semantic route with short recovery

3. semantic route followed by manual fallback

4. manual-first exploration

5. direct AST without Skill

6. mixed / stochastic
```

The desired direction is not:

```text
more AST calls
```

It is:

```text
correct targeted semantic information
with less avoidable exploration/recovery
```

---

# Phase 8a Normal-Routing Signal

Track whether the agent ever produces:

```text
partial relationship query
→ failure
→ search
→ exact relationship retry
```

Compare Arm A and Arm B.

Expected:

```text
Arm B
<=
Arm A
```

However, Phase 9b.1 showed that the current Skill often avoids this defect by searching first.

Therefore a low occurrence rate in both arms is acceptable.

Do not treat lack of exposure as failure of Phase 8a.

---

# Phase 8b Normal-Routing Signal

Track queries whose canonical target is valid but whose relationship result is empty because of receiver typing.

Known affected forms include:

```text
callers
references
callees
```

for member calls such as:

```text
token_.validate()
validator_.validate()
```

Record:

```text
empty relationship result
fallback relationship query
manual Read fallback
Grep fallback
```

Expected Arm B behavior:

```text
fewer false-empty results
fewer fallback calls
less manual verification
```

Phase 8b is expected to be the strongest Phase 8 contributor at whole-agent level.

---

# Phase 8c Normal-Routing Signal

Track `callees` queries on declaration-only targets with out-of-line definitions.

Record whether:

```text
Arm A:
    false empty
    → search/read fallback

Arm B:
    populated partial/correct result
```

Also carry forward the known residual gap:

```text
callees may still omit a callee
whose own identity is represented as a declaration/definition pair
```

Do not fix this during Phase 9b.2.

Record it separately as a future semantic candidate.

---

# Known Residual Semantic Gap

Phase 9b.1 discovered a new pattern:

```text
caller body correctly selected
→ one member callee omitted
because the callee itself has declaration/definition identity complexity
```

Canonical example:

```text
repo_.update(u)
→ UserRepository::update
```

This is not a Phase 8c regression.

It is a separate semantic limitation.

During Phase 9b.2:

```text
detect
count
document
```

but do not modify it.

If it repeatedly affects multiple tasks, recommend it as a future Phase 10 candidate.

---

# Windows Path-Quoting Artifact

Phase 9b.1 observed several runs with malformed Windows workspace paths.

Track this explicitly.

Classify errors such as:

```text
D:MyDocuments...
```

where backslashes/escaping are lost as:

```text
environment / command quoting artifact
```

not AST semantic failure.

Record:

```text
task
arm
command
recovery
cost
```

Do not silently exclude affected runs.

If the artifact becomes frequent enough to materially distort normal-routing comparison, stop the full interpretation and recommend a harness-level fix before repeating.

---

# Correctness Layers

For every task distinguish:

```text
1. semantic result correctness
2. intended edit/work correctness
3. validator success
```

This is required for known problematic fixtures such as:

```text
level4-006
```

Do not use validator success as the only correctness metric where the validator is known defective.

---

# level4-006 Handling

Keep `level4-006` in the suite for continuity unless the task definition is explicitly replaced in a separate evaluation revision.

Report:

```text
validator success
semantic answer correctness
edit-set correctness
```

separately.

Do not count its known failure as a new Phase 8 regression.

Do not silently remove it from the raw aggregate either.

Provide both:

```text
raw suite success
and
semantic-adjusted interpretation
```

---

# Aggregate Comparison

Produce a full-suite table:

```text
Metric
Arm A
Arm B
Delta
Assessment
```

Include:

```text
success
tools
Skill calls
semantic-analysis invocation
AST calls
AST failures
retries
help
search
find
callers
callees
references
grep
glob
read
bash
edit
tokens
elapsed
recovery mean/max
```

---

# Phase-Affected vs Unaffected Cohorts

Split the analysis into:

```text
Phase-8-affected tasks
unaffected guard/control tasks
invocation-mismatch tasks
no-Skill tasks
semantic-analysis-loaded-in-both tasks
```

This decomposition is important.

Phase 7 showed that invocation mismatch can dominate whole-suite aggregates.

Do not interpret the overall delta without cohort decomposition.

---

# Exact Skill Cohorts

For Arm A vs Arm B classify each task into:

```text
1. semantic-analysis loaded in both
2. semantic-analysis absent in both
3. semantic-analysis loaded only in Arm A
4. semantic-analysis loaded only in Arm B
```

Do not use:

```text
any Skill call
```

as a proxy.

Phase 7e corrected this measurement error already.

---

# Same-Loaded Cohort

The most informative normal-routing cohort is:

```text
semantic-analysis loaded in both arms
```

For these tasks compare:

```text
semantic routing
AST failures
retries
fallback
manual exploration
tools
tokens
elapsed
```

This cohort helps determine whether Phase 8 continues to improve behavior even without forced routing.

---

# Invocation-Mismatch Cohort

Analyze mismatch tasks separately.

For each mismatch ask:

```text
Did the cost difference come from:
- Phase 8 semantics,
- Skill invocation,
- another Skill,
- manual exploration,
- ordinary generation variance?
```

Do not attribute mismatch cost automatically to Phase 8.

Show representative trajectories.

---

# No-Skill Cohort

If neither arm invokes `semantic-analysis`, Phase 8 semantic changes may still matter if the agent invokes AST Tool directly.

Distinguish:

```text
no Skill + direct AST
```

from:

```text
no Skill + manual Grep/Read
```

Do not assume Phase 8 is irrelevant merely because the Skill was not loaded.

---

# Tool/Token Distribution

For the full suite report:

```text
total
mean
median
p75
p90
min
max
```

for tokens where useful.

Also report task-level Arm B−Arm A deltas.

Identify:

```text
top positive cost regressions
top cost improvements
```

and inspect their trajectories.

Do not let one or two outliers dominate interpretation without disclosure.

---

# Recovery Analysis

Report:

```text
AST failures
retries
help usage
recovery mean
recovery max
```

Also count known recovery signatures:

```text
Phase 8a target recovery
Phase 8b empty-result fallback
Phase 8c false-empty callees fallback
```

Expected whole-system direction:

```text
Phase 8
<=
pre-Phase-8
```

for avoidable semantic recovery.

---

# Manual Exploration Analysis

Compare:

```text
Grep
Glob
Read
```

both suite-wide and by cohort.

Phase 8b in particular should reduce manual verification where populated relationships replace empty semantic results.

A decrease in AST calls is not required if useful semantic information increases.

---

# `--help` Usage

Phase 9b.1 observed `--help`-related exploration on `level3-008`.

Track:

```text
help calls
ast-tool --help Bash invocations
```

where the harness distinguishes them.

If `level3-008` again regresses materially in Arm B, inspect whether the same `--help` / extra Read pattern reproduces.

Only consider it systematic if repeated under normal routing.

---

# Guard Analysis

For:

```text
level1-001
level1-003
level2-001
level3-001
```

verify:

```text
correctness
semantic result stability
failures/retries
routing
tools/tokens
```

No systematic semantic regression should appear.

---

# Representative Trajectories

Include representative comparisons for:

```text
1. clear Phase 8 improvement
2. neutral task
3. invocation mismatch
4. no-Skill route
5. largest Arm B regression
6. largest Arm B improvement
```

Do not report aggregate numbers without trajectory evidence.

---

# Acceptance Criteria

Phase 9b.2 should answer whether Phase 8 becomes the new stable global baseline.

A favorable result requires:

## 1. Correctness preserved

Arm B must maintain approximately Arm A-level correctness.

No new reproducible semantic correctness regression.

---

## 2. Semantic precision preserved

No systematic false relationship generation.

Any false positive is high severity.

---

## 3. Known Phase 8 improvements remain visible

At least Phase 8b's false-empty relationship elimination should remain observable under normal routing when affected commands are exercised.

8a and 8c may be less frequently exercised depending on routing.

---

## 4. Recovery is not systematically worse

AST failures, retries, and long recovery chains should remain at or below baseline, allowing ordinary stochastic variation.

---

## 5. Manual fallback is not systematically worse

Grep/Glob/Read should not show a broad semantic→manual collapse.

---

## 6. Agent-level cost is acceptable

Preferred:

```text
tools/tokens/time lower
```

Acceptable:

```text
cost approximately neutral
with better semantic correctness
```

Potential rejection:

```text
substantial systematic cost increase
without corresponding semantic value
```

---

## 7. Guard tasks remain stable

No systematic regression on unaffected semantic behavior.

---

# Final Decision Options

Choose exactly one.

## PROMOTE PHASE 8 TO STABLE BASELINE

Use when:

```text
correctness preserved
semantic precision preserved
known semantic defects improved
recovery/manual fallback stable or better
and
whole-agent cost is favorable or acceptable
```

This becomes the new stable semantic baseline.

---

## PROMOTE WITH CAVEATS

Use when:

```text
semantic correctness is clearly better
and
no hard regression exists
but
normal-routing cost/invocation variance remains mixed
```

Document the caveats and still promote if the net system behavior is preferable.

---

## KEEP PRE-PHASE-8 AS STABLE BASELINE

Use when:

```text
Phase 8 is semantically better in controlled mode
but
normal-routing whole-agent behavior is systematically worse
enough to outweigh that benefit
```

Do not delete Phase 8 work; retain it as a validated capability branch/candidate.

---

## REVISE SPECIFIC PHASE 8 CHANGE

Use only if normal routing exposes a reproducible semantic regression caused by:

```text
8a
8b
or
8c
```

Do not reopen Phase 8 wholesale.

---

## INCONCLUSIVE

Use when:

```text
invocation variance
environmental artifacts
or
run variance
```

are large enough that no stable system-level comparison can be made.

Repeat only the ambiguous cohorts.

---

# Promotion Standard

Do not require Arm B to win every metric.

The promotion question is:

```text
Is Phase 8 the better stable system
for real Coding Agent use?
```

Evaluate:

```text
correctness
semantic information quality
routing stability
recovery
manual exploration
token/context cost
latency
```

together.

---

# Phase 10 Gate

If Phase 8 is promoted, Phase 9b.2 should also identify the next evidence-backed target.

Possible examples include:

```text
callee-side declaration/definition identity
Windows command-path quoting
systematic --help overuse
another repeated semantic relationship gap
```

Do not automatically start another semantic phase.

Recommend Phase 10 only when the normal-routing evidence shows a repeated and meaningful limitation.

---

# Deliverables

Produce a final Phase 9b.2 report containing:

```text
1. Environment and revisions

2. Exact Arm A / Arm B definitions

3. Binary / Skill / harness verification

4. Normal-routing protocol

5. Full cohort

6. Repetition / ordering protocol

7. Whole-suite aggregate comparison

8. Task-level comparison

9. Skill invocation analysis

10. Same-loaded cohort

11. Invocation-mismatch cohort

12. No-Skill/direct-AST cohort

13. Phase 8a normal-routing evidence

14. Phase 8b normal-routing evidence

15. Phase 8c normal-routing evidence

16. Semantic precision analysis

17. Recovery analysis

18. Manual exploration analysis

19. Tool/token/time distributions

20. Guard-task analysis

21. Windows path-artifact analysis

22. level3-008 follow-up

23. Residual semantic-gap observations

24. Representative trajectories

25. Regressions and outliers

26. Experimental limitations

27. Final baseline decision

28. Recommended next phase
```

Keep raw measurements separate from interpretation.

---

# Evidence Standard

Use:

```text
strong:
    repeated same-task normal-routing evidence
    + exact semantic verification

moderate:
    fresh full-suite task-level evidence

weak:
    one stochastic trajectory

context only:
    historical Phase 5 / Phase 7 / earlier Phase 8 data
```

Do not promote or reject Phase 8 based on a single outlier.

---

# Working Principle

Phase 9b.2 is the final system-level decision.

Use:

```text
freeze both systems
→ remove forced routing
→ evaluate normal behavior
→ measure exact Skill invocation
→ decompose by routing cohort
→ inspect semantic value and recovery
→ compare whole-agent cost
→ promote only on system-level evidence
```

The central question is:

```text
Should Phase 8 replace the pre-Phase-8 system
as the stable Coding Agent baseline
under normal real-world routing?
```

Nothing else.
