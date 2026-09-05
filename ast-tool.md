# Phase 7d — Selective Backport of Phase 7c Improvements

## Objective

Create a new `semantic-analysis/SKILL.md` candidate starting from the **Phase 5 stable Skill**, and selectively backport only the Phase 7c changes that have demonstrated measurable behavioral value.

Phase 7d is not a compression experiment.

The goal is:

```text
Phase 5 routing/recovery stability
+
Phase 7c evidence-backed efficiency improvements
```

Instruction-size reduction is secondary.

Do not optimize for a smaller Skill at the expense of behavioral salience.

---

# Starting Point

Use the exact Phase 5 `semantic-analysis/SKILL.md` as the base.

Do not start from Phase 7c.

Phase 5 remains the behavioral reference because it showed:

```text
tests                      41
successes                  37
success rate              90.24%

total tool calls           518
AST Tool calls              69
AST failures                 9
AST failure rate          13.04%

grep                         15
glob                         12
read                        252

avg recovery distance      1.44
max recovery distance         2

total tokens            158,303
avg tokens/test           3,861

elapsed                 2224.27 sec
```

Phase 7d should preserve the properties that made this baseline stable.

---

# Why Phase 7d Exists

Phase 7c demonstrated that some changes were beneficial, but the compressed Skill did not outperform Phase 5 overall under controlled comparison.

In the controlled Phase 5 vs Phase 7c evaluation:

```text
correctness:
17/18 → 17/18

total tools:
162 → 173

AST calls:
57 → 64

AST failures:
5 → 8

AST retries:
4 → 8

help calls:
0 → 2

tokens:
54,344 → 59,440

elapsed:
912.59s → 970.03s
```

However, Phase 7c also contained clear local improvements.

The strongest example was:

```text
level3-007

Phase 5:
search ×3 → find ×6

Phase 7c:
search ×4
```

The Phase 7c trajectory:

```text
preserved correctness
preserved relevant context
used 5 fewer AST calls
used 4 fewer tools
saved 2,494 tokens
saved 21.42 seconds
```

This is the type of improvement Phase 7d should selectively backport.

---

# Core Principle

Use this rule throughout Phase 7d:

```text
Start with Phase 5 behavior.

Add only Phase 7c changes that have positive trajectory evidence.
```

Do not import Phase 7c wording merely because it is shorter or cleaner.

Every backported change must answer:

```text
What Phase 5 behavior does this change?

What controlled trajectory shows that the change is beneficial?

Does it preserve semantic targeting?

Does it avoid weakening recovery or command-selection salience?
```

If there is no clear evidence, do not backport it.

---

# Explicit Non-Goal

Do not attempt to recreate Phase 7c in a larger form.

Do not attempt to merge Phase 5 and Phase 7c mechanically.

Do not use:

```text
Phase 5 text
+
all Phase 7c wording that seems useful
```

Instead use:

```text
Phase 5
+
small evidence-backed behavioral improvements
```

---

# Protected Phase 5 Behavior

Preserve the Phase 5 semantics governing:

## Routing

```text
Find symbol       → search
Find callers      → callers
Find references   → references
Find callees      → callees
Find file symbols → symbols
Need AST structure → find
```

These should remain explicit and salient.

---

## Identity Discovery

Preserve Phase 5's preference for discovering symbol identity before relationship queries when identity is uncertain.

The controlled Phase 7c run showed several cases where relationship commands were attempted too early:

```text
level2-006
level2-008
level3-008
```

These trajectories used an under-qualified relationship target first and then recovered through `search`.

Phase 7d must not weaken Phase 5's search-first behavior.

---

## Recovery

Preserve Phase 5 recovery behavior, including:

```text
diagnostic-first correction
no unchanged failed-command retry
restricted help usage
cheap targeted recovery
limited trial-and-error
```

Do not import compressed Phase 7c recovery wording unless it demonstrably improves behavior.

---

## Help Behavior

Phase 5 produced zero help calls in the controlled cohort.

Phase 7c produced two.

Preserve the stronger Phase 5 help restriction.

`--help` should remain exceptional, not an ordinary discovery or recovery step.

---

## Semantic vs Manual Exploration

Preserve the Phase 5 preference for semantic commands over:

```text
grep
glob
broad reads
manual structural reasoning
```

when a direct AST Tool operation exists.

---

# Candidate Phase 7c Improvements

Review Phase 7c controlled trajectories and identify only improvements with positive evidence.

The following is the strongest known candidate.

---

## Candidate A — Allow Targeted `search` Refinement to Replace Redundant `find`

Phase 7c demonstrated that `find` is not always necessary after `search` when refined semantic search already retrieves the needed symbol or member.

Known evidence:

```text
level3-007

Phase 5:
search ×3 → find ×6

Phase 7c:
search ×4
```

The Phase 7c path was substantially cheaper while remaining targeted.

Also:

```text
level1-001
level3-003
```

used refined `search` instead of `search → find` without broad manual exploration.

Investigate whether Phase 5 wording can be minimally adjusted so that:

```text
find
```

remains the preferred route for actual AST structure/node lookup,

while:

```text
refined search
```

is acceptable when the task only requires locating the exact semantic target.

Do not weaken the rule into:

```text
search can replace find
```

The distinction must remain:

```text
Need exact symbol/member identity
→ refined search may be sufficient

Need AST structure/node relationship
→ find
```

---

# Backport Candidate Standard

For every Phase 7c change considered for Phase 7d, classify it as:

```text
BACKPORT
DO NOT BACKPORT
INCONCLUSIVE
```

Use the following evidence standard.

## BACKPORT

Only when:

```text
controlled Phase 7c trajectory
→ clearly cheaper or more direct
→ correctness preserved
→ semantic targeting preserved
→ no recovery/manual exploration regression
```

---

## DO NOT BACKPORT

When the Phase 7c change is associated with:

```text
under-qualified relationship queries
extra AST failures
extra retries
extra help
longer recovery
broader exploration
```

or when Phase 5 is clearly better.

---

## INCONCLUSIVE

When the difference is likely stochastic or cannot be tied to Skill wording.

Do not backport inconclusive changes.

---

# Known Phase 7c Behaviors Not to Backport Automatically

Do not import wording associated with the following controlled regressions unless new evidence proves otherwise.

## Relationship-before-search behavior

Observed in:

```text
level2-006
level2-008
level3-008
```

Pattern:

```text
under-qualified callers/callees/references
→ failure
→ search
→ corrected relationship query
```

Phase 5's identity-first guidance is preferable.

---

## Weaker recovery/help adherence

Especially:

```text
smoke-001
```

Phase 7c produced:

```text
relationship misuse
→ help
→ ambiguous retry
→ search/recovery
```

with recovery distance 4.

Phase 5 recovery was shorter.

Do not weaken Phase 5 recovery wording.

---

# Investigation Before Editing

Before modifying the Skill, create a Phase 7c improvement inventory.

For every controlled task where Phase 7c was materially cheaper than Phase 5, report:

| Task | Phase 5 route | Phase 7c route | Tool delta | Token delta | Time delta | Candidate rule |
| ---- | ------------- | -------------- | ---------: | ----------: | ---------: | -------------- |

Focus first on:

```text
level3-007
level1-002
```

and any other clear improvement.

Do not treat small token differences as behavioral improvements unless the trajectory also supports the conclusion.

---

# Rule-Level Diff Analysis

Compare Phase 5 and Phase 7c Skill text only for candidate improvements.

For each candidate:

1. Identify the exact Phase 5 rule.
2. Identify the Phase 7c equivalent or changed wording.
3. Determine what decision boundary changed.
4. Determine whether that boundary caused the better controlled trajectory.
5. Propose the smallest Phase 5 modification that captures the benefit.

Do not broadly rewrite nearby sections.

---

# Minimal Change Principle

Prefer one sentence or one bullet.

For example, if supported by evidence:

```text
Use `find` for AST structure or node lookup. If a refined `search`
already identifies the exact symbol/member needed, do not add a
redundant `find`.
```

is preferable to replacing the entire `search` / `find` guidance.

This is only an example.

Use the actual Phase 5 wording and evidence to determine the final text.

---

# Do Not Optimize Skill Size

Phase 7d has no compression target.

Report Skill size before and after, but do not treat a larger Skill as failure if the behavioral result improves.

Likewise, do not delete unrelated Phase 5 text merely to offset newly added wording.

The target is Agent efficiency, not instruction length.

---

# Change Budget

Keep Phase 7d deliberately narrow.

As a guideline:

```text
1–3 behavioral modifications
```

should be sufficient unless the evidence clearly supports more.

If analysis identifies many candidate changes, stop and reconsider whether the phase is becoming an uncontrolled rewrite.

---

# Stage 1 — Targeted Controlled Replay

After implementing Phase 7d, first run a small controlled replay.

Force `semantic-analysis` to load first exactly as in Phase 8c.

Include:

```text
level3-007
level1-001
level3-003
```

for the refined-search / find behavior.

Also include regression guards:

```text
level2-006
level2-008
level3-008
smoke-001
```

to verify that Phase 5's relationship/recovery behavior was not weakened.

Add any task directly affected by another backported rule.

---

# Targeted Replay Comparison

Compare:

```text
Phase 5
Phase 7c
Phase 7d
```

for:

```text
success
tool calls
AST calls
AST failures
retries
help
search
find
callers
references
callees
grep
glob
read
recovery
tokens
elapsed
```

and the exact command trajectory.

---

# Targeted Acceptance Criteria

Phase 7d should preserve the Phase 7c improvement on intended tasks.

For example, for the `level3-007` class:

```text
avoid redundant structural queries
while preserving targeted semantic discovery
```

At the same time, regression-guard tasks should remain Phase 5-like:

```text
identity discovery before relationship queries
short diagnostic-driven recovery
minimal help usage
```

If the backport improves the target but weakens guards, revise it.

---

# Stage 2 — Full Controlled Cohort

If the targeted replay succeeds, rerun the same 18-task controlled cohort used in Phase 8c.

The experiment should be:

```text
Phase 5 Skill
vs
Phase 7d Skill
```

with forced first-call Skill invocation.

Use the same:

```text
tasks
fixtures
AST Tool
harness
environment
permissions
timeouts
```

---

# Controlled Success Criteria

Phase 7d should aim to outperform Phase 5, not merely match Phase 7c.

Primary requirements:

```text
correctness >= Phase 5

AST failures <= Phase 5

retries <= Phase 5 or approximately equal

help <= Phase 5 or approximately equal

recovery approximately Phase 5 or better

no systematic AST → grep/manual regression
```

Efficiency should improve in at least one meaningful dimension:

```text
fewer redundant AST calls
fewer total tools
lower median paired token cost
lower elapsed time
```

without meaningful regression elsewhere.

---

# Strong Controlled Target

A strong result would look conceptually like:

```text
Phase 5 correctness
+
Phase 5 recovery stability
+
Phase 7c targeted-search efficiency
```

For example:

```text
success          >= 17/18
AST failures     <= 5
retries          <= 4
help             ~0
grep             <= Phase 5
recovery max     <= Phase 5 or close
tools            < 162
tokens           < 54,344
elapsed          < 912.59s
```

These are directional targets, not rigid thresholds.

Trajectory quality takes priority over exact counts.

---

# Stage 3 — Full 41-Task Evaluation

Only after the controlled cohort is favorable, run the unchanged 41-task evaluation.

Compare:

```text
Phase 5
Phase 7c
Phase 7d
```

Collect:

```text
success rate
total tools
AST calls
AST failures
failure rate
retries
help
search
callers
references
callees
find
symbols
grep
glob
read
bash
edit
recovery
elapsed
tokens
Skill invocation
```

---

# Agent-Level Goal

The desired Phase 7d full-run behavior is:

```text
Phase 5 semantic routing stability
+
Phase 7c lower AST failure / Bash / Edit tendencies
+
reduced redundant structural work
```

Do not require the AST command distribution to match Phase 5 exactly.

A lower `find` count is acceptable when it reflects a cheaper targeted semantic path rather than manual fallback.

---

# Per-Test Analysis

For every meaningful difference between Phase 5 and Phase 7d, classify:

```text
IMPROVEMENT
BENIGN DIFFERENCE
REGRESSION
INCONCLUSIVE
```

Require trajectory evidence for improvement or regression.

Pay special attention to:

```text
find → refined search
relationship command ordering
recovery after semantic failure
help usage
grep/glob/read substitution
```

---

# Token Analysis

Report:

```text
total tokens
mean
median
p75
p90
paired deltas
top outlier contributions
```

Because token variance has been substantial in previous runs, do not accept or reject based solely on total tokens.

Prefer paired trajectory evidence and distributional metrics.

---

# Skill Size

Report:

```text
Phase 5 lines / chars / bytes / approximate tokens
Phase 7d lines / chars / bytes / approximate tokens
delta
```

Skill size is informational only.

A small increase over Phase 5 is acceptable if agent-level efficiency measurably improves.

---

# Final Decision

Choose exactly one:

```text
ACCEPT
ACCEPT WITH CAVEATS
REVISE
REVERT TO PHASE 5
```

---

## ACCEPT

Use when Phase 7d demonstrates:

```text
Phase 5-level correctness
+
Phase 5-level routing/recovery stability
+
at least one reproducible Phase 7c-derived efficiency improvement
+
no meaningful new regression
```

---

## ACCEPT WITH CAVEATS

Use when the intended backport works but stochastic metrics prevent a strong aggregate conclusion.

---

## REVISE

Use when the candidate improvement is valid but its wording is too broad and causes localized side effects.

Recommend the smallest narrowing change.

---

## REVERT TO PHASE 5

Use when the backported changes do not reproduce the Phase 7c improvement or weaken Phase 5's stable behavior.

Do not fall back to Phase 7c automatically.

---

# Deliverables

Provide:

1. Phase 7c improvement inventory.
2. Backport / do-not-backport classification.
3. Phase 5 → Phase 7d Skill diff.
4. Evidence for every behavioral change.
5. Modified Phase 7d `SKILL.md`.
6. Skill-size comparison.
7. Targeted controlled replay results.
8. Phase 5 vs Phase 7c vs Phase 7d targeted trajectory comparison.
9. Full 18-task controlled comparison.
10. Full 41-task evaluation, if the controlled result passes.
11. Per-test regression/improvement analysis.
12. Token distribution analysis.
13. Final recommendation.

---

# Phase 9 Preparation

Phase 7d should also identify inefficiencies shared by Phase 5 and Phase 7d.

Do not fix them during this phase.

Record repeated patterns such as:

```text
search
→ ambiguity
→ extra search/read

callers
→ refinement
→ callers

find
→ read for missing structural context

references
→ ambiguity
→ manual disambiguation
```

These shared inefficiencies are candidates for Phase 9 semantic-capability research.

Do not propose a new AST Tool command unless the trajectory evidence shows that the existing command set cannot express the needed operation cleanly.

---

# Final Principle

Phase 7d is not:

```text
make Phase 5 smaller
```

and not:

```text
merge Phase 5 and Phase 7c
```

It is:

```text
keep Phase 5 where Phase 5 wins
+
backport Phase 7c only where Phase 7c demonstrably wins
```

The final question is:

```text
Can the stable Phase 5 Skill adopt a small number of
Phase 7c's proven targeted-efficiency improvements and
thereby outperform Phase 5 without sacrificing its
routing and recovery stability?
```