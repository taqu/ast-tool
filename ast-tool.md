# Phase 7e — Skill Invocation Reliability Investigation

## Objective

Investigate why the `semantic-analysis` Skill is invoked inconsistently during normal agent runs, and determine whether Skill invocation can be made more reliable without changing the validated Phase 7d Skill body.

Phase 7e is **not** a Skill-content optimization phase.

The Phase 7d Skill body has already passed controlled validation. The main unresolved problem is that normal agent-level evaluation is heavily affected by whether the Skill is invoked at all.

The optimization target remains the **Coding Agent as a whole**, but this phase isolates Skill invocation as a separate behavioral variable.

---

## Current Baseline

Use the current Phase 7e branch as the working branch.

The semantic-analysis Skill must initially remain exactly equivalent to the accepted Phase 7d version:

```text
Phase 5 Skill
+
one refined-search / redundant-find rule
```

The added Phase 7d rule is:

```text
If a refined `search` already identifies the exact symbol or member needed,
do not add a redundant `find` solely to locate it; use `find` when AST
structure or node detail is required.
```

Do not modify this Skill body unless Phase 7e evidence specifically demonstrates that invocation behavior depends on content inside the Skill.

---

## Background

Phase 7d.2 produced two different results.

### Controlled 18-task evaluation

When `semantic-analysis` was forced to be:

```text
exactly once
and
the first tool action
```

Phase 7d clearly passed the controlled gate.

Compared with Phase 5:

```text
success            17/18 → 17/18
tools                 170 → 155
AST calls              58 → 53
AST failures            8 → 4
retries                 7 → 4
tokens             58,968 → 53,525
elapsed           1035.94 → 922.91 sec
recovery mean/max    1.67/4 → 1.00/1
```

Semantic routing remained targeted and no systematic regression was found.

### Normal 41-task evaluation

In the normal run, however:

```text
semantic-analysis invocation
Phase 7d: 19/41
```

The normal Phase 7d aggregate was worse than Phase 5 in several metrics:

```text
tools
AST failures
retries
grep
glob
recovery
elapsed
tokens
```

Cohort analysis showed that invocation mismatch dominated the regression.

In particular:

```text
Skill loaded in both:
    Δtokens +525

Skill absent in both:
    Δtokens -8,506

Invocation mismatch:
    Δtokens +13,925
```

The mismatch cohort alone was larger than the total token regression.

Therefore:

```text
Skill body quality
!=
Skill invocation reliability
```

Phase 7e must investigate the second problem without reopening the first one.

---

# Primary Questions

Answer the following questions with evidence.

## 1. When is the Skill invoked?

Determine which task characteristics correlate with invocation of `semantic-analysis`.

Possible dimensions include:

```text
task level
requested operation
symbol lookup
relationship queries
cross-file work
structural lookup
ambiguity
repository size
prompt wording
task complexity
first agent action
early tool selection
```

Do not assume any of these are causal.

Measure them.

---

## 2. Is invocation deterministic?

For representative tasks, repeat normal runs without forcing the Skill.

Determine whether invocation is:

```text
stable per task
mostly stable
strongly stochastic
```

For example, distinguish:

```text
task A:
    Skill invoked 10/10

task B:
    Skill invoked 0/10
```

from:

```text
task C:
    Skill invoked 4/10
```

The third case is especially important because it indicates within-task behavioral variance rather than task classification alone.

---

## 3. What happens before invocation?

Inspect the trajectory before the Skill is loaded.

Record:

```text
first tool/action
second tool/action
whether repository exploration already started
whether grep/glob/read occurred first
whether AST Tool was attempted first
whether Skill invocation happened after failure
```

We need to know whether late invocation is useful or whether missing the Skill at the first decision point effectively determines the rest of the trajectory.

---

## 4. What happens when the Skill is not invoked?

Compare:

```text
Skill-loaded runs
vs
no-Skill runs
```

for the same tasks where possible.

Measure:

```text
correctness
AST usage
grep
glob
read
tool count
tokens
recovery
elapsed
```

The purpose is to quantify the actual agent-level cost of invocation failure.

Do not infer this only from the existing 41-task aggregate.

Use paired or repeated evidence.

---

## 5. Is invocation affected by Skill body wording?

This is a secondary question only.

Do **not** begin by changing the Skill.

First establish invocation behavior using the current Phase 7d body.

Only if evidence suggests that Skill description, metadata, naming, or visible introductory wording affects discovery/invocation should a minimal controlled experiment be considered.

Do not rewrite the semantic decision tree.

---

# Constraints

## Do not optimize the semantic Skill body

Phase 7e must not add generic guidance such as:

```text
Always use semantic-analysis.
Prefer AST Tool.
Do not use grep.
```

unless a specific experiment explicitly tests invocation behavior.

Do not respond to no-Skill trajectories by adding semantic routing rules inside the existing Skill.

Those instructions cannot affect runs in which the Skill was never loaded.

---

## Change one invocation-related variable at a time

If experiments modify invocation-facing configuration, modify only one factor per experiment.

Possible factors may include, if present in the system:

```text
Skill name
Skill description
Skill metadata
trigger wording
discovery text
registration mechanism
available-Skill presentation
```

Do not change several simultaneously.

For every change:

```text
baseline
→ one modification
→ repeated comparison
```

---

## Preserve the Phase 7d body

Before and after each experiment, verify that the semantic Skill body has not changed unintentionally.

Record a hash when practical.

---

## Do not force invocation in the main Phase 7e experiment

Forced invocation was already used in Phase 7d.2 to isolate Skill-body behavior.

Phase 7e must primarily observe **normal invocation behavior**.

Forced runs may be used only as a control arm.

---

# Suggested Evaluation Design

## Stage 1 — Instrument Existing Behavior

Before changing anything, collect invocation data from the current Phase 7e branch.

For every run record at least:

```text
task
success
Skill invoked?
Skill invocation index
first action
tools before Skill invocation
AST calls
AST failures
retries
help
grep
glob
read
bash
edit
total tools
tokens
elapsed
recovery distance
```

If available, also record the exact Skill invocation timestamp or event sequence.

---

## Stage 2 — Select an Invocation Probe Cohort

Choose a small representative cohort from the existing evaluation tasks.

Include at minimum:

```text
tasks that consistently use semantic routing
tasks that previously showed invocation mismatch
relationship-query tasks
search/find tasks
one recovery-sensitive task
one higher-level task
```

Prefer approximately 6–10 tasks.

Do not create new tasks unless the existing suite cannot expose invocation behavior.

---

## Stage 3 — Repeat Normal Runs

Run each probe task multiple times without forced Skill invocation.

Recommended minimum:

```text
5 runs per task
```

Use more repetitions for tasks showing mixed invocation behavior.

Record invocation frequency:

```text
invoked / total
```

Example:

```text
level3-003    5/5
level3-007    3/5
level4-003    1/5
```

Do not interpret single-run differences as causal.

---

## Stage 4 — Classify Tasks

Classify each probe task as:

```text
A. Stable invocation
B. Stable non-invocation
C. Stochastic invocation
```

Suggested initial thresholds:

```text
A: 80–100% invoked
B: 0–20% invoked
C: 21–79% invoked
```

The exact thresholds are not important; the observed distributions are.

---

## Stage 5 — Compare Same-Task Trajectories

For tasks in category C, compare runs where the Skill was invoked with runs where it was not.

This is the most valuable cohort because the task itself is held constant.

Ask:

```text
Does Skill invocation change semantic routing?

Does no-Skill execution increase grep/glob/read?

Does invocation reduce recovery?

Does invocation affect correctness?

Does invocation reduce token/tool cost?
```

This comparison is stronger than comparing unrelated tasks.

---

## Stage 6 — Identify the Earliest Divergence

For stochastic tasks, find the first meaningful difference between:

```text
Skill-loaded trajectory
and
no-Skill trajectory
```

Examples:

```text
Skill
→ search
→ callers
```

versus:

```text
Grep
→ Glob
→ Read
→ manual reasoning
```

or:

```text
Read
→ Skill
→ search
```

Determine whether invocation is:

```text
a first-decision routing mechanism
```

or merely:

```text
late contextual assistance
```

This distinction matters for any later fix.

---

# Optional Controlled Invocation Experiments

Only perform these after the baseline behavior has been characterized.

If the invocation mechanism exposes a meaningful invocation-facing field, test the smallest possible modification.

Possible experiment types:

```text
description-only change
name/description salience change
trigger metadata change
Skill-list presentation change
```

Do not change the semantic body at the same time.

For each candidate:

```text
current Phase 7d invocation surface
vs
one modified invocation surface
```

Run repeated normal executions on the same probe cohort.

Measure:

```text
invocation rate
correctness
semantic routing
tools
tokens
recovery
```

A higher invocation rate is not automatically better.

The goal is:

```text
appropriate invocation
```

not:

```text
maximum invocation
```

Tasks that do not need semantic analysis should not be forced through the Skill merely to improve an invocation metric.

---

# Important Failure Modes to Avoid

## 1. Optimizing invocation rate in isolation

Do not optimize for:

```text
Skill invocation = 100%
```

The system-level objective remains:

```text
correctness
+
targeted semantic routing
+
efficient context use
+
low recovery cost
```

Invocation is only useful when it improves those outcomes.

---

## 2. Confusing correlation with causation

For example:

```text
Skill-loaded runs have fewer tokens
```

does not prove:

```text
Skill invocation caused lower token use
```

unless same-task repeated comparisons support that conclusion.

---

## 3. Fixing no-Skill behavior inside SKILL.md

If the Skill was not loaded, changes inside its body cannot explain or fix the trajectory.

Treat this as an invocation-layer problem unless evidence shows otherwise.

---

## 4. Reacting to one stochastic run

Phase 7d already showed that within-version variance can be as large as version differences.

Do not make changes from one run.

Require repetition.

---

## 5. Reopening Phase 7d semantic optimization

Do not use this phase to revisit:

```text
search vs find wording
relationship ordering
help restrictions
general compression
```

unless invocation experiments produce direct evidence that those contents affect Skill discovery.

Those belong to separate investigations.

---

# Analysis Requirements

Produce both aggregate and task-level analysis.

At minimum include:

## Invocation table

```text
task
runs
invoked
invocation rate
mean invocation position
```

## Loaded vs no-Skill comparison

```text
success
tools
AST calls
AST failures
grep
glob
read
tokens
elapsed
recovery
```

## Trajectory examples

Show representative paired trajectories for:

```text
stable invocation
stable non-invocation
stochastic invocation
```

## Earliest-divergence analysis

For mixed tasks, identify where the trajectories first diverge.

---

# Decision Criteria

At the end of Phase 7e, choose one of the following.

## INVOCATION IS RELIABLE ENOUGH

Use this if:

```text
invocation behavior is mostly stable
and
mismatch observed in Phase 7d.2 appears to be ordinary model variance
and
normal agent-level results are not systematically harmed
```

Then proceed to repeat normal Phase 7d evaluation or move to the final baseline decision.

---

## INVOCATION SURFACE IMPROVEMENT FOUND

Use this if:

```text
one narrow invocation-facing change
reproducibly improves appropriate Skill invocation
and
preserves correctness
and
improves or preserves agent-level routing/cost
```

Keep the semantic Phase 7d body unchanged.

Validate the change on the full normal evaluation suite.

---

## INVOCATION PROBLEM CONFIRMED, NO SAFE FIX YET

Use this if:

```text
invocation is materially stochastic or systematically missed
but
no narrow causal change has been validated
```

Document the limitation.

Do not compensate by expanding SKILL.md.

---

## NO MEANINGFUL INVOCATION EFFECT

Use this if:

```text
same-task Skill-loaded and no-Skill trajectories
do not show meaningful agent-level differences
```

In that case the Phase 7d normal-run variance should not be attributed primarily to invocation, and another source of variance must be investigated.

---

# Deliverables

Produce a final Phase 7e report containing:

```text
1. Environment and revisions

2. Exact Phase 7d Skill verification

3. Probe cohort

4. Number of repeated runs

5. Skill invocation frequency per task

6. Invocation position / first-action analysis

7. Same-task loaded vs no-Skill comparison

8. Aggregate tool/token/recovery metrics

9. Representative paired trajectories

10. Earliest-divergence analysis

11. Any invocation-facing experiment performed

12. Causal assessment

13. Final decision

14. Recommendation for the next phase
```

Keep raw measurements separate from interpretation.

---

# Expected Phase 7e Outcome

The desired output is not necessarily a code change.

A successful Phase 7e may simply establish:

```text
where Skill invocation variance comes from
how large its system-level cost is
whether it is task-dependent or stochastic
and whether a narrow intervention is justified
```

Do not modify the system merely to produce a Phase 7e patch.

Evidence is the deliverable.

---

# Working Principle

Use the methodology established during Phase 7:

```text
stable baseline
→ identify one reproducible behavior
→ isolate the variable
→ repeat under controlled conditions
→ compare same-task trajectories
→ make one narrow change only if causally justified
→ validate at agent level
```

For Phase 7e, the isolated variable is:

```text
Skill invocation reliability
```

not:

```text
SKILL.md semantic content
```
