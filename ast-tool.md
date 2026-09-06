# Phase 9a — Controlled Semantic Capability Evaluation

## Objective

Evaluate whether the completed Phase 8 semantic-command changes improve the Coding Agent under controlled semantic-routing conditions.

Phase 9a must isolate the value of the semantic capability changes themselves from:

```text
Skill invocation variance
normal first-action routing variance
unrelated manual exploration variance
```

The comparison is:

```text
pre-Phase-8 semantic baseline
vs
Phase 8a + Phase 8b + Phase 8c
```

under matched task conditions and forced semantic routing.

The primary question is:

```text
Did Phase 8 improve semantic information quality
and reduce avoidable semantic recovery / manual fallback
without reducing correctness?
```

Phase 9a is not a new feature-development phase.

Do not modify semantic behavior during this evaluation.

---

# Systems Under Comparison

Define two immutable arms.

## Arm A — Pre-Phase-8 Baseline

Use the last accepted semantic implementation before Phase 8 changes.

This arm must exclude:

```text
Phase 8a
    unique FQN-suffix relationship target resolution

Phase 8b
    receiver-type member relationship resolution

Phase 8c
    declaration→definition body selection for callees
```

Use the accepted Phase 7d Skill body.

Record:

```text
repository revision
AST Tool binary hash
Skill hash
evaluation harness hash
```

---

## Arm B — Current Phase 8 System

Use the current accepted implementation containing:

```text
Phase 8a
Phase 8b
Phase 8c
```

with the same accepted Phase 7d Skill body.

Record the same revision/hash information.

Do not change:

```text
Skill text
Skill metadata
Skill invocation policy
task prompts
fixtures
validators
evaluation harness behavior
```

between arms.

---

# Controlled Routing Condition

For both arms:

```text
semantic-analysis
= invoked exactly once
= first tool action
```

This is required to remove the invocation confound identified during Phase 7e.

Do not compare:

```text
forced semantic Arm A
vs
normal Arm B
```

or vice versa.

Both arms must use the same routing condition.

---

# Phase 9a Scope

The controlled cohort should emphasize tasks that exercise Phase 8 behavior while retaining enough unaffected controls to detect regressions.

The cohort must contain four categories:

```text
A. Phase 8a target-resolution tasks

B. Phase 8b receiver-type relationship tasks

C. Phase 8c body-identity / callees tasks

D. unaffected semantic guard tasks
```

Prefer existing evaluation tasks and fixtures.

Do not create broad new tasks merely to increase sample size.

---

# Cohort Construction

## Category A — Phase 8a Target Resolution

Include tasks that previously showed:

```text
partial FQN relationship query
→ not found
→ search
→ full-FQN relationship retry
```

At minimum include the existing tasks corresponding to:

```text
AuthToken::expire
DataStore::save
```

where available in the evaluation suite.

The expected Phase 8 improvement is:

```text
one successful relationship call
```

instead of:

```text
failed relationship
→ search
→ relationship retry
```

---

## Category B — Phase 8b Receiver-Type Resolution

Include tasks where previously valid member relationships were missing for calls such as:

```text
token_.validate(...)
validator_.validate(...)
```

At minimum include the existing tasks equivalent to:

```text
level2-004
level4-006
```

or the current canonical tasks exercising those relationships.

The expected Phase 8 improvement is:

```text
empty relationship result
→ populated exact relationship result
```

with no false-positive relationships.

---

## Category C — Phase 8c Body Identity

Include at least one task where:

```text
callees target
→ declaration selected
→ no body
→ empty result
```

before Phase 8c.

The canonical example is:

```text
auth::AuthService::refresh
```

where the implementation body exists out of line.

The expected Phase 8 improvement is:

```text
same callable identity
→ body-bearing definition
→ correct callees
```

---

## Category D — Unaffected Guards

Include semantic tasks that were already correct before Phase 8.

Cover several of:

```text
exact search
direct callers
direct references
direct callees
structural find
unqualified resolution
exact FQN resolution
```

These guard tasks are necessary to detect regressions from broader semantic changes.

Prefer approximately:

```text
6–10 affected tasks
+
4–8 unaffected guards
```

for a total controlled cohort of approximately:

```text
12–18 tasks
```

If the existing historical 18-task controlled cohort remains suitable, reuse as much of it as practical.

---

# Repetition Strategy

One fresh paired run per task is insufficient for strong claims.

Use repeated controlled trials for the tasks most directly affected by Phase 8.

Recommended:

```text
Affected tasks:
    minimum 5 runs per arm

Unaffected guards:
    minimum 3 runs per arm
```

For tasks with high historical trajectory variance:

```text
extend to 10 runs per arm
```

if needed.

Rotate task order between rounds.

If practical, interleave arms at the task or round level to reduce temporal bias.

Example:

```text
round 1:
    A task1
    B task1
    B task2
    A task2

round 2:
    reverse / rotate order
```

Do not run all Arm A first and all Arm B second unless unavoidable.

If interleaving is not possible, record that as a limitation.

---

# Primary Metrics

For every run record:

```text
task
arm
success

total tools

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

Also record:

```text
ordered AST trajectory
first semantic query
relationship result cardinality where relevant
```

---

# Semantic Information Metrics

Phase 9a must measure not only tool count but semantic information quality.

For affected tasks, record:

```text
expected semantic result
actual semantic result
missing relationships
unexpected relationships
empty-result fallback
identity-resolution recovery
body-resolution recovery
```

For relationship queries, include:

```text
target
canonical resolved identity
returned relationship count
expected relationship count
false positives
false negatives
```

This is especially important for Phase 8b.

---

# Phase-Specific Causal Metrics

## Phase 8a Metrics

Count occurrences of:

```text
relationship
→ failure
→ search
→ relationship retry
```

The Phase 8 system should reduce these to zero for uniquely resolvable partial FQNs.

Measure:

```text
target-resolution failures
target-resolution searches
relationship retries
```

---

## Phase 8b Metrics

Count:

```text
relationship query returned empty
despite valid typed member relationship
```

Measure:

```text
correct member relationships returned
missing member relationships
unexpected cross-type relationships
manual fallback after empty result
```

False positives are a hard regression.

---

## Phase 8c Metrics

Count:

```text
callees query resolved declaration-only representation
and returned empty despite existing definition body
```

Measure:

```text
body-bearing definition successfully used
correct callee count
empty false-negative result
unexpected callee
```

---

# Trajectory Classification

For each task, compare Arm A and Arm B trajectories and classify the difference.

Use:

```text
1. Equivalent

2. Semantic recovery eliminated

3. Manual fallback eliminated

4. Semantic result improved but agent trajectory unchanged

5. Lower tool/token cost

6. Higher cost with better semantic information

7. Regression

8. Stochastic / inconclusive
```

Do not force every improvement into a cost-saving category.

---

# Task-Level Comparison

Produce a table containing at minimum:

```text
Task
Phase exercised
Arm A success
Arm B success
Δtools
ΔAST
Δfailures
Δretries
Δreads
Δtokens
Δelapsed
semantic result change
assessment
```

For repeated runs, report means and medians where useful.

---

# Pairwise Distribution Analysis

For repeated same-task runs, calculate Arm B minus Arm A deltas for:

```text
tools
AST calls
AST failures
retries
reads
tokens
elapsed
```

Report:

```text
mean
median
p75
p90
min
max
```

where sample size makes those statistics meaningful.

Do not rely only on aggregate totals.

---

# Correctness Gate

Correctness must not regress.

For each task:

```text
Arm B success rate
>=
Arm A success rate
```

unless a difference is clearly attributable to an unrelated pre-existing validator defect.

Any such defect must be documented separately and should not be counted as semantic regression if the semantic output and intended edit set can be independently verified.

Do not silently exclude failures.

---

# Semantic Precision Gate

Phase 8 must not introduce false semantic relationships.

For all fixture-backed or task-backed relationship results:

```text
unexpected relationship count
=
0
```

is the preferred requirement.

A false positive is more serious than a remaining false negative.

Any new false relationship should trigger investigation before acceptance.

---

# Recovery Gate

Phase 8 was explicitly designed to eliminate known semantic recovery patterns.

Therefore compare:

```text
AST failures
retries
recovery distance
```

particularly on affected tasks.

Expected direction:

```text
Phase 8
<=
pre-Phase-8
```

for known resolver/body-identity defects.

Do not require every unaffected stochastic task to improve.

---

# Manual Exploration Gate

Measure whether better semantic information reduces:

```text
Grep
Glob
Read
manual source inspection
```

on affected tasks.

This is supporting evidence, not a hard requirement.

A semantic capability may still be valuable if correctness improves while manual exploration remains similar.

---

# Token and Time Interpretation

Do not treat tokens or elapsed time as the sole acceptance criterion.

Phase 8b already showed that:

```text
better semantic information
```

can coexist with noisy cost metrics.

Use the following interpretation:

```text
best:
    semantic correctness improves
    and cost falls

acceptable:
    semantic correctness improves
    and cost is approximately neutral

caveat:
    semantic correctness improves
    but cost rises materially

regression:
    cost rises
    without meaningful semantic benefit
```

---

# Unaffected Guard Analysis

For guard tasks, specifically look for:

```text
new AST failures
new retries
new ambiguity
changed exact FQN resolution
changed unqualified resolution
new false relationships
new empty relationships
extra semantic calls
manual fallback increases
```

Phase 8 should not improve affected tasks by destabilizing already-correct ones.

---

# Required Before/After Pattern Analysis

At minimum report these three patterns.

## Pattern A — Phase 8a

```text
Before:
partial-FQN relationship
→ not found
→ search
→ full-FQN relationship

After:
partial-FQN relationship
→ success
```

---

## Pattern B — Phase 8b

```text
Before:
canonical relationship target
→ empty result
→ fallback/manual reasoning

After:
canonical relationship target
→ exact populated relationship result
```

---

## Pattern C — Phase 8c

```text
Before:
callee target
→ declaration-only body
→ empty

After:
same callable identity
→ body-bearing definition
→ correct callees
```

These are the primary causal signatures of Phase 8.

---

# Historical Comparisons

Historical Phase 7/8 measurements may be included for context.

However:

```text
historical normal runs
!=
controlled causal evidence
```

Do not mix historical single-run results directly into fresh repeated controlled aggregates.

Keep:

```text
fresh controlled comparison
```

and:

```text
historical context
```

separate.

---

# No Changes During Evaluation

Once Phase 9a begins, freeze:

```text
AST Tool implementation
Skill files
fixtures
validators
task prompts
evaluation scripts
```

If a measurement bug is found:

1. stop;
2. fix the measurement;
3. document it;
4. rerun affected measurements symmetrically for both arms.

Do not patch semantic behavior mid-evaluation.

---

# Acceptance Criteria

Phase 9a may conclude favorably if all of the following are true.

## 1. Correctness preserved

Phase 8 does not reduce task success.

---

## 2. Known Phase 8 defects are actually removed

The affected tasks show the intended semantic changes:

```text
8a:
resolution recovery removed

8b:
missing typed-member relationships restored

8c:
declaration-only callees false empty removed
```

---

## 3. No new false semantic relationships

False-positive guards remain clean.

---

## 4. Unaffected semantic routes remain stable

No systematic regression appears in guard tasks.

---

## 5. Recovery improves or remains controlled

Known semantic failures/retries decrease.

---

## 6. Agent-level trajectory shows credible value

At least one of the following should improve meaningfully across affected tasks:

```text
fewer failed semantic calls
fewer retries
less manual exploration
fewer Reads
fewer total tools
lower tokens
lower elapsed
```

Semantic correctness itself remains the primary benefit.

---

# Possible Final Decisions

## ACCEPT PHASE 8 CAPABILITY SET

Use when:

```text
correctness preserved
known semantic defects removed
no false-positive regression
guards stable
and
agent-level value is directionally favorable
```

Proceed to Phase 9b normal full-suite evaluation.

---

## ACCEPT WITH CAVEATS

Use when:

```text
semantic correctness clearly improves
and
guards remain stable
but
token/time/tool improvements are mixed or noisy
```

Proceed to Phase 9b, carrying the caveats explicitly.

---

## REVISE PHASE 8

Use only if a specific reproducible regression is isolated to one Phase 8 behavior.

Identify whether it belongs to:

```text
8a
8b
8c
```

Do not reopen all Phase 8 changes together.

---

## REVERT SPECIFIC PHASE 8 CHANGE

Use if one isolated Phase 8 change introduces:

```text
wrong relationships
wrong body identity
ambiguity collapse
or systematic correctness regression
```

Revert only the causal change if possible.

---

## INCONCLUSIVE

Use when variance or measurement limitations prevent a reliable comparison.

Do not compensate by adding more semantic behavior.

---

# Phase 9b Gate

Proceed to Phase 9b only if Phase 9a concludes:

```text
ACCEPT PHASE 8 CAPABILITY SET
```

or:

```text
ACCEPT WITH CAVEATS
```

Phase 9b will answer a different question:

```text
Does the complete current system improve
under normal agent routing and Skill invocation?
```

Phase 9a answers only:

```text
Are the Phase 8 semantic capabilities themselves better
when semantic routing is held constant?
```

Keep these questions separate.

---

# Deliverables

Produce a final Phase 9a report containing:

```text
1. Environment and revisions

2. Exact Arm A / Arm B definitions

3. Skill and harness verification

4. Controlled cohort

5. Repetition and ordering protocol

6. Aggregate metrics

7. Task-level paired metrics

8. Phase 8a causal analysis

9. Phase 8b causal analysis

10. Phase 8c causal analysis

11. Semantic precision / false-positive analysis

12. Recovery analysis

13. Manual exploration analysis

14. Token/time distribution

15. Unaffected guard results

16. Representative before/after trajectories

17. Regressions and outliers

18. Limitations

19. Final decision

20. Phase 9b recommendation
```

Keep raw measurements separate from interpretation.

---

# Preferred Report Tables

## Controlled Aggregate

```text
Metric
Pre-Phase-8
Phase 8
Delta
Assessment
```

---

## Task-Level Comparison

```text
Task
Phase
Success A/B
Δtools
ΔAST
Δfail
Δretry
ΔRead
Δtokens
Δelapsed
Semantic change
Assessment
```

---

## Semantic Correctness

```text
Task
Expected relationships
Pre-Phase-8 result
Phase 8 result
Missing before
Missing after
Unexpected after
```

---

## Phase-Specific Recovery

```text
Pattern
Pre-Phase-8 occurrences
Phase 8 occurrences
Calls saved
Failures removed
Retries removed
```

---

# Evidence Standard

Use:

```text
strong:
    repeated same-task controlled comparison
    + exact semantic-result verification

moderate:
    repeated task behavior with controlled routing

weak:
    single paired run

insufficient:
    historical aggregate or cross-task correlation
```

Do not make broad Phase 8 claims from weak evidence alone.

---

# Working Principle

Phase 9a is a validation phase, not an optimization phase.

Use:

```text
freeze both systems
→ hold semantic routing constant
→ repeat the same tasks
→ compare exact semantic information
→ compare recovery and exploration
→ measure agent-level cost
→ accept or reject the Phase 8 capability set
```

The central question is:

```text
Did Phase 8 make semantic analysis
more correct and more useful
when the agent actually uses it?
```

Nothing else.
