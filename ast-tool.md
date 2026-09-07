# Phase 9b.1 — Fresh Controlled Pre-Phase-8 vs Phase-8 Confirmation

## Objective

Perform a fresh, symmetric, controlled comparison between:

```text
Arm A:
    pre-Phase-8 semantic implementation

Arm B:
    accepted Phase 8a + 8b + 8c implementation
```

under identical agent-routing conditions.

Phase 9b.1 exists to remove the primary limitation of Phase 9a:

```text
Arm A relied partly on historical proxy runs.
```

The central question is:

```text
When semantic-analysis is forced identically in both arms,
does the Phase 8 capability set still preserve correctness
and reproduce the semantic/recovery improvements observed
during Phase 8 and Phase 9a?
```

Phase 9b.1 is a validation phase.

Do not modify semantic behavior during the experiment.

---

# Relationship to Phase 9a

Phase 9a concluded:

```text
ACCEPT PHASE 8 CAPABILITY SET
```

based on:

```text
Phase 8a:
    partial-FQN relationship recovery eliminated

Phase 8b:
    typed-member relationships restored

Phase 8c:
    declaration-only callees false-empty corrected

No false-positive relationship regression observed.
```

However, Phase 9a retained three important limitations:

```text
1. Fresh forced-routing Arm A agent runs
   were not collected.

2. Phase 8b historical Arm A agent evidence
   was sparse for some tasks.

3. Phase 8c had strong direct semantic evidence
   but no dedicated agent-level comparison.
```

Phase 9b.1 must address these limitations directly.

Do not reopen the Phase 8 implementations unless a fresh reproducible regression is found.

---

# Systems Under Comparison

Freeze exactly two arms.

## Arm A — Fresh Pre-Phase-8 Baseline

Use:

```text
Git revision:
ccc1fbb650bf058aa11602134d4e4fa1795cb98e
```

or the exact verified pre-Phase-8 revision used in Phase 9a.

Arm A must exclude:

```text
Phase 8a
    unique FQN-suffix relationship resolution

Phase 8b
    receiver-type member relationship resolution

Phase 8c
    declaration/definition body identity
```

Use the same accepted Phase 7d `semantic-analysis` Skill.

---

## Arm B — Accepted Phase 8 System

Use the current accepted implementation containing:

```text
Phase 8a
Phase 8b
Phase 8c
```

Use the exact same Skill body as Arm A.

---

# Immutable Evaluation Inputs

Before running any agent evaluation, record and verify:

```text
Arm A Git revision
Arm B Git revision

Arm A binary SHA-256
Arm B binary SHA-256

Skill SHA-256

task prompt hashes or exact task revisions
fixture revisions
validator revisions
harness revision/hash
model identifier
Claude Code version
```

Verify before and after the experiment that no arm changed.

Do not allow working-tree semantic changes during measured runs.

---

# Controlled Routing Requirement

For every measured run in both arms:

```text
semantic-analysis
= exactly once
= first tool action
```

Use the same forced-routing mechanism used previously.

Verify this from trace logs rather than assuming the control succeeded.

Any run that does not satisfy the routing condition must be excluded and rerun.

Record exclusions explicitly.

---

# Same Runtime Conditions

Use the same:

```text
model
Claude Code version
launcher
environment
task prompt
repository fixture
validator
Skill
routing control
```

for both arms.

Reset the evaluation repository before every run.

Each measured run must use a fresh agent process/session.

Do not reuse agent memory/state across runs.

---

# Cohort Design

Phase 9b.1 should be smaller than the final normal-routing suite.

Its purpose is causal confirmation, not broad system coverage.

Use four groups.

---

## Group A — Phase 8a Target Resolution

Include:

```text
level2-008
level3-008
```

or their exact current equivalents.

These tasks historically exercised:

```text
partial-FQN relationship
→ failure
→ search
→ exact relationship retry
```

Expected Arm B behavior:

```text
partial-FQN relationship
→ success
```

without the recovery sequence.

---

## Group B — Phase 8b Receiver-Type Relationships

Include:

```text
level2-004
level4-006
```

or their current equivalents.

The key semantic signal is:

```text
Arm A:
    canonical relationship query
    → empty / incomplete result

Arm B:
    same relationship query
    → exact populated result
```

`level4-006` has a known validator defect.

Do not use its raw validator success alone as a semantic correctness signal.

Record separately:

```text
semantic result correctness
intended edit set correctness
validator result
```

---

## Group C — Phase 8c Agent-Level Body Identity

Add at least one task that requires `callees` on a declaration-only target whose body is defined out of line.

Prefer a real existing evaluation task.

If no existing task exercises the behavior cleanly, use the smallest existing task that can be adapted without changing the semantic fixture itself.

The target behavior should be equivalent to:

```text
callees auth::AuthService::refresh
```

Arm A:

```text
declaration selected
→ no body
→ false empty
```

Arm B:

```text
same callable identity
→ body-bearing definition
→ populated correct callees
```

The task should require the agent to make meaningful use of this result.

Do not count a direct CLI probe as the Group C agent-level run.

---

## Group D — Unaffected Semantic Guards

Include at least four tasks that were already correct before Phase 8.

Cover a mix of:

```text
exact FQN callers
unqualified search
direct references
direct callees
structural find
already-correct relationship routing
```

The purpose is to detect regressions in unaffected semantic behavior.

---

# Recommended Cohort Size

Target approximately:

```text
Affected tasks:
    5–6

Unaffected guards:
    4–6

Total:
    approximately 9–12 tasks
```

Do not expand to the full 41-task suite in Phase 9b.1.

That belongs to Phase 9b.2.

---

# Repetition Strategy

Use fresh repeated runs for both arms.

Recommended minimum:

```text
Affected tasks:
    5 runs per arm

Unaffected guards:
    3 runs per arm
```

For historically high-variance tasks:

```text
10 runs per arm
```

may be used if the initial 5 runs are mixed.

Do not increase repetition merely to chase a desired result.

---

# Arm Ordering

Avoid temporal bias.

Prefer interleaving.

Example:

```text
Task A round 1:
    Arm A
    Arm B

Task B round 1:
    Arm B
    Arm A

Task A round 2:
    Arm B
    Arm A
```

Rotate order across tasks and rounds.

Do not run all Arm A sessions first and all Arm B sessions afterward unless unavoidable.

If full interleaving cannot be achieved, document the limitation.

---

# Required Metrics

For every measured agent run record:

```text
task
arm
success

Skill invoked?
Skill invocation position

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

Also retain:

```text
ordered full tool trajectory
ordered AST trajectory
```

---

# Semantic Result Metrics

For Phase 8-affected tasks, record the actual semantic answer.

At minimum:

```text
query
resolved canonical target
expected relationship set
actual relationship set

missing relationships
unexpected relationships

empty result?
failure?
retry?
```

Do not infer semantic correctness only from final task success.

---

# Phase 8a Confirmation

For every relevant run classify whether this pattern occurs:

```text
relationship(partial FQN)
→ failure
→ search
→ relationship(exact FQN)
```

Report:

```text
Arm A occurrence count
Arm B occurrence count
```

Expected:

```text
Arm B = 0
```

for uniquely resolvable targets.

Also verify that ambiguity guards are not weakened.

---

# Phase 8b Confirmation

For receiver-typed member relationships measure:

```text
correct callers returned
correct references returned
correct callees returned

missing relationship count
unexpected relationship count
```

Track whether an empty relationship causes:

```text
references fallback
search fallback
Read/manual exploration
```

Expected Phase 8 signature:

```text
previously empty relationship
→ populated exact relationship
→ fallback reduced or eliminated
```

---

# Phase 8c Confirmation

Phase 9b.1 must add agent-level evidence for Phase 8c.

For the selected Group C task, record whether the agent uses:

```text
callees
```

and whether the result is:

```text
Arm A:
    false empty due to declaration-only body

Arm B:
    populated using body-bearing definition
```

Then determine whether this changes:

```text
subsequent search
Read usage
manual exploration
AST retries
total tools
tokens
elapsed
```

The key new evidence required from 9b.1 is:

```text
correct Phase 8c semantic result
→ useful agent trajectory change
```

or, if trajectory does not improve:

```text
correct semantic result
→ neutral agent-level effect
```

Either is informative.

---

# Guard Requirements

For unaffected guard tasks verify:

```text
same success rate
same semantic result sets
no new failures
no new retries
no new ambiguity
no false relationships
no systematic extra semantic protocol
```

A Phase 8 change must not improve its target cases by destabilizing unrelated semantic behavior.

---

# False-Positive Gate

This is a hard semantic guard.

Across all relationship queries where expected sets are known:

```text
unexpected relationships
=
0
```

should hold.

Any new false relationship must be investigated before acceptance.

Do not trade precision for lower recovery cost.

---

# Correctness Analysis

Use three levels where relevant:

```text
1. semantic answer correctness
2. intended edit/work correctness
3. validator success
```

This is especially important for tasks with known fixture defects.

Do not label a Phase 8 semantic regression solely because a known-broken validator fails.

Conversely, do not label a semantic result correct merely because the final validator passes.

---

# Primary Comparison Tables

Produce at least the following.

## Table 1 — Aggregate

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
tokens
elapsed
recovery
```

---

## Table 2 — Task-Level

```text
Task
Group
Runs A/B
Success A/B
Δtools
ΔAST
Δfailures
Δretries
ΔRead
Δtokens
Δelapsed
Semantic result change
Assessment
```

---

## Table 3 — Semantic Accuracy

```text
Task / query
Expected
Arm A
Arm B
Missing A
Missing B
Unexpected A
Unexpected B
```

---

## Table 4 — Phase-Specific Causal Patterns

```text
Pattern
Arm A occurrences
Arm B occurrences
Calls saved
Failures removed
Retries removed
```

Cover:

```text
8a partial-FQN recovery
8b empty member relationship fallback
8c declaration-body false empty
```

---

# Distribution Analysis

Because the experiment is repeated, do not rely only on means.

For each affected task report Arm B minus Arm A distributions for:

```text
tools
AST calls
AST failures
retries
reads
tokens
elapsed
```

Include where meaningful:

```text
mean
median
min
max
```

Use p75/p90 only if the number of runs is large enough to make them useful.

---

# Trajectory Analysis

For every affected task include at least one representative Arm A and Arm B trajectory.

Example Phase 8a:

```text
Arm A:
Skill
→ callers(partial)
→ FAILURE
→ search
→ callers(exact)
→ Read/Edit

Arm B:
Skill
→ callers(partial)
→ SUCCESS
→ Read/Edit
```

Example Phase 8b:

```text
Arm A:
Skill
→ search
→ callers
→ empty
→ references
→ Read/Edit

Arm B:
Skill
→ search
→ callers
→ populated
→ Read/Edit
```

Example Phase 8c:

```text
Arm A:
Skill
→ callees
→ empty
→ fallback

Arm B:
Skill
→ callees
→ populated
→ targeted continuation
```

---

# Acceptance Criteria

Phase 9b.1 may pass only if the fresh symmetric comparison supports the Phase 9a conclusion.

## 1. Correctness preserved

No reproducible decrease in semantic or task correctness.

---

## 2. Phase 8a effect reproduced

Known partial-FQN recovery disappears under Arm B.

---

## 3. Phase 8b effect reproduced

Previously missing receiver-typed relationships become exact populated results.

---

## 4. Phase 8c agent-level usefulness characterized

The body-identity improvement is exercised by an agent task and its trajectory impact is measured.

---

## 5. Semantic precision preserved

No new false relationships.

---

## 6. Unaffected guards stable

No systematic regression in already-correct semantic tasks.

---

## 7. Recovery does not regress

Across affected tasks, Phase 8 should reduce or preserve:

```text
failures
retries
recovery distance
```

---

## 8. Agent-level cost is acceptable

Preferred:

```text
correctness improves/preserves
+
tools/tokens/time improve
```

Acceptable:

```text
semantic correctness improves
+
cost is approximately neutral
```

Caveat:

```text
semantic correctness improves
+
cost rises materially
```

A cost increase must be justified by semantic value.

---

# Possible Decisions

## CONFIRM PHASE 8

Use when:

```text
fresh Arm A vs Arm B
reproduces the Phase 9a semantic improvements,
guards remain stable,
and no systematic regression appears.
```

Proceed to Phase 9b.2.

---

## CONFIRM WITH CAVEATS

Use when:

```text
semantic correctness is clearly better,
guards remain stable,
but cost or trajectory metrics are mixed.
```

Proceed to Phase 9b.2 while carrying the caveats.

---

## REVISE SPECIFIC PHASE 8 CHANGE

Use only when a fresh reproducible regression is isolated to:

```text
8a
8b
or
8c
```

Do not reopen all Phase 8 changes.

---

## INCONCLUSIVE

Use when:

```text
fresh A/B variance
prevents reliable interpretation
```

Increase repetitions only for the affected tasks.

Do not modify semantic behavior.

---

# Hard Stop Conditions

Do not proceed directly to Phase 9b.2 if fresh Arm B introduces:

```text
false semantic relationships
wrong body identity
ambiguity collapse
systematic correctness loss
systematic guard regression
```

Investigate the specific causal Phase 8 change first.

---

# No Semantic Changes During Phase 9b.1

Once measurement begins, freeze:

```text
AST Tool source
binaries
Skill
tasks
fixtures
validators
harness
```

If an evaluation measurement bug is found:

```text
stop
fix measurement only
rerun both arms symmetrically
document the change
```

Do not patch production semantics mid-run.

---

# Phase 9b.2 Gate

Proceed to Phase 9b.2 only if the final Phase 9b.1 decision is:

```text
CONFIRM PHASE 8
```

or:

```text
CONFIRM WITH CAVEATS
```

Phase 9b.2 will remove the forced-routing control and answer:

```text
Does the complete Phase 8 system improve
under normal Skill invocation and ordinary agent behavior?
```

Phase 9b.1 answers only:

```text
Does Phase 8 remain better
under a fresh symmetric controlled comparison?
```

Do not mix these conclusions.

---

# Deliverables

Produce a final Phase 9b.1 report containing:

```text
1. Environment and revisions

2. Exact Arm A / Arm B definitions

3. Binary / Skill / harness verification

4. Controlled-routing verification

5. Cohort and group definitions

6. Repetition and arm-order protocol

7. Fresh aggregate comparison

8. Fresh task-level comparison

9. Semantic accuracy results

10. Phase 8a confirmation

11. Phase 8b confirmation

12. Phase 8c agent-level confirmation

13. Recovery analysis

14. Manual exploration analysis

15. Tool/token/time distributions

16. Unaffected guard results

17. Representative trajectories

18. Regressions and outliers

19. Experimental limitations

20. Final decision

21. Phase 9b.2 recommendation
```

Keep raw measurements separate from interpretation.

---

# Evidence Standard

Use:

```text
strong:
    fresh repeated same-task Arm A/B comparison
    + exact semantic-result verification

moderate:
    fresh repeated controlled trajectory comparison

weak:
    one fresh run

context only:
    historical Phase 7/8 data
```

Historical data may explain a pattern, but Phase 9b.1 conclusions must come from fresh evidence.

---

# Working Principle

Phase 9b.1 is the final causal confirmation before normal-routing evaluation.

Use:

```text
freeze pre-Phase-8
freeze Phase 8
→ force the same Skill routing
→ run both arms fresh
→ interleave repetitions
→ verify exact semantic answers
→ compare trajectories and recovery
→ confirm or challenge Phase 9a
```

The central question is:

```text
Does the Phase 8 capability set still win
when both systems are evaluated fresh,
symmetrically,
and under identical semantic routing?
```

Nothing else.
