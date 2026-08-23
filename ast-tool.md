# Create Agent Evaluation Tasks — Difficulty Level 5

## Objective

Create the fifth batch of Agent Evaluation tasks for `ast-tool`.

Create exactly **8 Level 5 tasks**.

Level 5 should represent a progression from semantic change-surface analysis toward realistic software maintenance and bug-fixing work.

The agent should not always be given the exact symbol that must be changed.

Instead, the task may begin with:

```text
a failing behavior
a bug symptom
an incorrect result
an architectural requirement
an incomplete refactoring
a behavioral regression
```

The agent must investigate the repository, identify the relevant semantic entities and relationships, determine the root cause or correct change surface, and implement a correct fix.

The purpose is to evaluate whether the agent can use `ast-tool` as part of a realistic investigation workflow.

Do not modify existing Level 1 through Level 4 tasks.

Do not modify the existing Agent-facing Skills.

---

# 1. Difficulty Progression

The evaluation progression is:

```text
Level 1
Direct local modification
        ↓
Level 2
Semantic identity and ambiguity
        ↓
Level 3
Multi-step relationship navigation
        ↓
Level 4
Semantic impact analysis and coordinated change
        ↓
Level 5
Problem investigation
        ↓
Root-cause discovery
        ↓
Semantic impact analysis
        ↓
Design / fix decision
        ↓
Implementation
        ↓
Regression avoidance
```

The defining characteristic of Level 5 is:

> The task describes a problem or required behavior, not necessarily the symbol or source location that must be changed.

---

# 2. Number of Tasks

Create exactly:

```text
level5-001
level5-002
level5-003
level5-004
level5-005
level5-006
level5-007
level5-008
```

Store them under:

```text
evaluation/tasks/
```

Do not create Level 6 tasks.

---

# 3. Core Level 5 Principle

A Level 5 task should require the agent to answer:

```text
What is actually wrong?
```

before it can reliably answer:

```text
What should be changed?
```

The intended reasoning shape is:

```text
Observed symptom
      ↓
Locate relevant behavior
      ↓
Identify candidate symbols
      ↓
Navigate semantic relationships
      ↓
Find root cause
      ↓
Determine change surface
      ↓
Implement fix
      ↓
Verify required behavior
      ↓
Ensure related behavior did not regress
```

The task must remain deterministic.

Do not create open-ended debugging puzzles with multiple equally valid fixes.

---

# 4. Required Task Categories

Create approximately the following distribution.

## A. Wrong Semantic Path

Create 2 tasks.

The observed behavior occurs because the wrong implementation, overload, dependency, or semantic path is being used.

Example:

```text
Request
   ↓
Service
   ↓
incorrect overload selected
```

or:

```text
Production workflow
   ↓
incorrect repository method
```

The repository should contain multiple plausible symbols with similar names.

The task prompt describes the incorrect behavior.

The agent must determine which semantic path is responsible.

The fix should not be discoverable merely by searching for the exact error message.

Useful semantic capabilities may include:

```text
search
find
references
callers
callees
```

---

## B. Incomplete Propagation Bug

Create 2 tasks.

A value, context, option, or state is intended to propagate through a workflow but disappears or is replaced at one layer.

Example:

```text
Handler
  ↓ request_id
Service
  ↓ request_id
Repository
  ✗ lost
Adapter
```

The task prompt should describe the observable failure.

For example:

> The request identifier is available at the API boundary but persistence records do not contain it.

The agent must determine where propagation breaks.

The correct solution may require modifying:

```text
declaration
definition
callers
intermediate layer
```

The task should include similar workflows where propagation must not change.

---

## C. Incorrect Caller / Workflow Behavior

Create 1 task.

A function behaves correctly in isolation.

The bug is caused by one or more callers using it incorrectly.

Example:

```text
PaymentService::authorize()
```

is correct, but:

```text
RetryWorker
```

passes an incorrect mode or skips a required step.

The prompt should describe the symptom.

The agent must trace:

```text
symptom
↓
target behavior
↓
callers
↓
incorrect caller
```

The task should contain multiple callers so that changing the target function itself would be an incorrect or overly broad fix.

---

## D. Incorrect Callee / Dependency Behavior

Create 1 task.

A higher-level workflow produces an incorrect result because it calls the wrong dependency or uses a dependency in the wrong order.

Example:

```text
OrderService::submit()
   ↓
validate()
   ↓
save()
   ↓
authorize()
```

when the correct sequence should be:

```text
validate()
   ↓
authorize()
   ↓
save()
```

Do not make the task merely about reordering obvious adjacent lines.

The repository should require identifying the relevant workflow and understanding its callees.

The task prompt should describe the externally visible behavior.

---

## E. Regression After Partial Refactoring

Create 1 task.

The repository should contain a partially completed refactoring.

For example:

```text
Old API
↓ partially migrated
New API
```

Some declarations and definitions may already use the new abstraction, while one semantic path still uses the old one.

The task prompt should describe the regression.

The agent must determine:

```text
which migration is incomplete
which callers are affected
which old paths must remain for compatibility
```

The fix should be selective.

Do not require a large mechanical rename.

---

## F. Bug Requiring Combined Semantic Investigation

Create 1 task.

This should be the most complex Level 5 task.

The agent should plausibly need to combine:

```text
symbol discovery
+
references
+
callers
+
callees
```

The exact sequence must not be prescribed.

Example conceptual shape:

```text
Observed duplicate side effect
        ↓
Locate operation
        ↓
Find callers
        ↓
Trace one caller's workflow
        ↓
Find indirect callee
        ↓
Identify duplicated semantic path
        ↓
Fix only the duplicate path
```

The repository should contain plausible but incorrect alternative hypotheses.

The validator must clearly distinguish the intended fix from incomplete or overly broad fixes.

---

# 5. Repository Fixtures

Create approximately **2–4 reusable Level 5 repository fixtures**.

Suggested conceptual structure:

```text
evaluation/repositories/
├── level5-request-bugs/
├── level5-order-workflow/
├── level5-refactor-regression/
└── level5-state-propagation/
```

Adapt to the existing repository layout where appropriate.

Aim for approximately:

```text
25–60 source/header files
```

as a guideline.

Do not add meaningless files.

Repository complexity should come from:

```text
multiple semantic paths
layered architecture
similar workflows
realistic abstractions
partial migrations
semantic exclusions
```

rather than file count alone.

---

# 6. Problem-First Prompt Design

Level 5 prompts should primarily describe:

```text
symptom
expected behavior
scope
constraints
regression boundaries
```

Do not immediately identify the exact function to modify.

Good:

```text
Orders submitted through the web checkout path are being persisted
without the request identifier, while direct internal imports work
correctly. Fix the propagation so that checkout-originated orders
retain the request identifier through persistence.
```

Poor:

```text
Add RequestContext to OrderRepository::save().
```

The first requires investigation.

The second already identifies the solution.

---

# 7. Do Not Reveal the Root Cause

Do not write the root cause in the prompt.

Avoid:

```text
The bug is caused by OrderService::submit() calling save()
without the context.
```

Instead:

```text
Orders submitted through checkout lose their request identifier
before persistence.
```

The agent should discover the responsible semantic path.

---

# 8. Root Cause Must Be Deterministic

Although Level 5 begins from a symptom, the intended root cause must be unambiguous.

Before finalizing a task, verify:

```text
Observed symptom
      ↓
Specific semantic path
      ↓
Specific defect
      ↓
Deterministic correct fix
```

Avoid tasks where several unrelated fixes could satisfy the validator.

Do not create tasks requiring subjective architectural preferences.

---

# 9. Semantic Investigation Requirements

Each task should make at least **three** semantic capabilities plausibly useful.

Possible combinations:

```text
search
+
references
+
callers
```

```text
find
+
callers
+
callees
```

```text
search
+
references
+
callers
+
callees
```

Not every capability must actually be used by the agent.

Do not prescribe commands.

The benchmark measures whether the agent can select useful investigation tools.

---

# 10. Multiple Plausible Candidates

Most Level 5 tasks should contain more than one plausible location for the bug.

Examples:

```text
OrderService::submit()
CheckoutService::submit()
MigrationService::submit()
```

or:

```text
Repository::save()
CacheRepository::save()
AuditRepository::save()
```

The correct path must be identifiable through semantic relationships.

Avoid trivial filename-based clues.

---

# 11. Semantic Exclusion and Regression Boundaries

Every Level 5 task must define behavior that should remain unchanged.

Examples:

```text
Web checkout
    → fix required

Internal import
    → already correct
    → must remain unchanged
```

or:

```text
Production retry workflow
    → fix required

Migration compatibility path
    → must remain unchanged
```

The validator must explicitly check these boundaries.

A solution that fixes the symptom by modifying all similar paths should fail if it violates the intended scope.

---

# 12. Root Cause vs Symptom Fix

Validators should reject superficial fixes.

For example, if the task is:

```text
Duplicate authorization occurs during retry.
```

the validator should reject solutions that merely suppress visible logging if authorization is still performed twice.

The task should test the underlying behavioral requirement.

Prefer validation of:

```text
correct call count
correct propagated value
correct selected overload
correct dependency ordering
correct caller behavior
```

over checking for a particular text insertion.

---

# 13. Validation Requirements

Every Level 5 task must have deterministic validation.

The validator should test:

### Symptom resolved

```text
the observed incorrect behavior no longer occurs
```

### Root behavior corrected

```text
the relevant semantic path now behaves correctly
```

### Completeness

```text
all required declarations / definitions / callers are consistent
```

when applicable.

### Regression boundaries

```text
excluded workflows remain unchanged
```

### Partial fixes

Detect common incomplete solutions.

Examples:

```text
fixed one caller but missed another affected caller
```

```text
updated declaration but not implementation
```

```text
fixed production path but broke compatibility path
```

```text
changed target function globally instead of fixing incorrect caller
```

---

# 14. Validation Should Prefer Behavioral Tests

Where practical, validators should execute code or inspect structured behavior rather than only searching source text.

Examples:

```text
compile and run a small test program
```

or:

```text
execute repository-provided Python test harness
```

or:

```text
verify call counters or recorded state
```

Do not introduce a heavyweight build system.

Keep validation:

```text
fast
deterministic
self-contained
```

Source inspection may supplement behavioral validation.

---

# 15. Avoid Artificial Debugging Tricks

Do not create bugs based on:

```text
undefined behavior
race conditions
timing
filesystem order
randomness
compiler-specific quirks
hidden environment variables
network access
```

Do not require external services.

The task must be reproducible.

The difficulty should come from semantic investigation.

---

# 16. Distinguish Level 5 From Level 4

Level 4:

```text
Requirement
↓
Determine affected semantic surface
↓
Implement coordinated change
```

Level 5:

```text
Symptom
↓
Investigate
↓
Find root cause
↓
Determine affected semantic surface
↓
Choose correct fix
↓
Implement
↓
Prevent regression
```

When designing a task, ask:

> Could the agent reasonably begin editing immediately after reading the prompt?

If the answer is yes, the task is probably Level 4 or below.

A Level 5 task should require investigation before the correct edit location is known.

---

# 17. Preserve Evaluation Environment

Do not modify:

```text
ast-tool/.claude/skills/
```

Do not modify existing:

```text
evaluation/tasks/smoke-001.yaml
evaluation/tasks/level1-*.yaml
evaluation/tasks/level2-*.yaml
evaluation/tasks/level3-*.yaml
evaluation/tasks/level4-*.yaml
```

Do not redesign:

```text
evaluation runner
statistics collection
Claude Code invocation
task schema
```

Only create the new Level 5 tasks and the repository fixtures and validators required by them.

---

# 18. Inspect Existing Tasks Before Creation

Before creating Level 5:

1. Inspect all existing Level 1–4 tasks.
2. Inspect repository fixtures.
3. Inspect validators.
4. Inspect the evaluation runner.
5. Inspect current Agent-facing Skills.
6. Identify existing bug-fix scenarios.
7. Avoid duplicating existing semantic patterns.
8. Ensure every Level 5 task represents genuine progression.

Do not assume prior task designs.

Use the actual current repository state.

---

# 19. Evaluation Metadata

If the current task schema supports evaluation-only metadata, add information such as:

```yaml
evaluation:
  category: root-cause-investigation
  relevant_capabilities:
    - search
    - references
    - callers
    - callees
  expected_reasoning_depth: very_high
```

Other possible categories:

```text
wrong-semantic-path
incomplete-propagation
caller-bug
callee-ordering
refactor-regression
combined-investigation
```

Do not modify the evaluation framework merely to add metadata.

Do not specify an exact workflow.

---

# 20. Acceptance Criteria

The work is complete when:

* [ ] Exactly 8 Level 5 task YAML files exist.
* [ ] Every task begins from a symptom, regression, or behavioral requirement.
* [ ] The prompt does not reveal the exact root-cause location.
* [ ] Every task has a deterministic root cause.
* [ ] Every task contains at least one plausible but incorrect candidate path.
* [ ] Every task requires investigation before the correct edit location is known.
* [ ] Every task defines regression or exclusion boundaries.
* [ ] Most tasks make at least three semantic capabilities potentially useful.
* [ ] At least 2 tasks involve incomplete propagation.
* [ ] At least 1 task involves an incorrect caller.
* [ ] At least 1 task involves an incorrect callee or dependency sequence.
* [ ] At least 1 task involves a partial refactoring regression.
* [ ] At least 1 task combines references, callers, and callees.
* [ ] Validators detect superficial or partial fixes.
* [ ] Validators test behavioral correctness where practical.
* [ ] Existing Levels 1–4 remain unchanged.
* [ ] Existing Skills remain unchanged.
* [ ] The existing evaluation runner can execute all Level 5 tasks.

---

# 21. Final Deliverables

Create:

```text
evaluation/tasks/level5-001.yaml
evaluation/tasks/level5-002.yaml
evaluation/tasks/level5-003.yaml
evaluation/tasks/level5-004.yaml
evaluation/tasks/level5-005.yaml
evaluation/tasks/level5-006.yaml
evaluation/tasks/level5-007.yaml
evaluation/tasks/level5-008.yaml
```

Create only the repository fixtures and validation scripts necessary to support these tasks.

After implementation, provide a summary table:

| Task | Repository | Category | Symptom | Root Cause Type | Relevant Capabilities |
| ---- | ---------- | -------- | ------- | --------------- | --------------------- |

For every task, report:

1. The observable symptom.
2. The expected behavior.
3. The hidden root-cause location.
4. The plausible incorrect candidate locations.
5. The expected semantic investigation surface.
6. The likely useful `ast-tool` capabilities.
7. The files expected to be modified.
8. The semantic paths that must remain unchanged.
9. The validation strategy.
10. The partial or superficial fixes that the validator rejects.
11. Any possible overlap with Levels 1–4.

The central question for Level 5 is:

```text
Can the coding agent use ast-tool as part of a real debugging workflow?

Symptom
   ↓
Investigation
   ↓
Semantic navigation
   ↓
Root-cause identification
   ↓
Impact analysis
   ↓
Correct fix
   ↓
Regression-safe result
```
