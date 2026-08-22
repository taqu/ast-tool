# Create Agent Evaluation Tasks — Difficulty Level 3

## Objective

Create the third batch of Agent Evaluation tasks for `ast-tool`.

Create **8 tasks** representing a clear progression from Level 2.

Level 2 introduced:

```text
symbol ambiguity
references
basic caller discovery
basic callee discovery
mixed semantic relationships
```

Level 3 should focus on:

```text
call graph reasoning
multi-step caller/callee navigation
impact analysis
semantic relationship chains
distributed code modification
```

The purpose is to evaluate whether Claude Code can use `ast-tool` as part of a multi-step investigation rather than only as a replacement for `grep`.

Do not modify the existing Level 1 or Level 2 tasks.

Do not modify the existing Agent-facing Skills.

---

# 1. Existing Evaluation Evidence

Level 1 tasks were successfully solved without `ast-tool`.

Typical workflows were:

```text
Grep
Read
Edit
```

Level 2 introduced situations where semantic navigation became useful.

At least one Level 2 task demonstrated the following workflow:

```text
Skill
  ↓
ast-tool search
  ↓
ast-tool callers
  ↓
source inspection
  ↓
ast-tool references
  ↓
additional semantic navigation
  ↓
Edit
```

For example, the agent used:

```text
callers: 4
search: 2
references: 1
```

and successfully modified multiple source files.

This indicates that the existing Skills and semantic commands can be discovered and used by Claude Code during realistic coding work.

Level 3 should build on this evidence.

The goal is not merely to increase `ast-tool` usage.

The goal is to test whether the agent can use semantic relationships to reason about a larger portion of the codebase.

---

# 2. Level 3 Goal

Level 3 should introduce tasks where the required modification depends on understanding a chain such as:

```text
Entry point
    ↓
Caller
    ↓
Service
    ↓
Target function
```

or:

```text
Target function
    ↓
Callee
    ↓
Dependency
    ↓
Relevant behavior
```

or:

```text
Symbol
    ↓
References
    ↓
Caller classification
    ↓
Selective modification
```

The agent should need to investigate semantic relationships across multiple files before determining the correct modification set.

The task should be difficult to solve safely using a single text search.

---

# 3. Number of Tasks

Create exactly **8 tasks**:

```text
level3-001
level3-002
level3-003
level3-004
level3-005
level3-006
level3-007
level3-008
```

Store them under:

```text
evaluation/tasks/
```

Do not create Level 4 tasks yet.

---

# 4. Level 3 Core Principle

The main distinction between Level 2 and Level 3 is:

```text
Level 2

Identify the correct semantic entity
        ↓
Inspect references or relationships
        ↓
Modify
```

versus:

```text
Level 3

Identify semantic entity
        ↓
Navigate relationship
        ↓
Navigate another relationship
        ↓
Determine relevant subset
        ↓
Modify multiple locations
        ↓
Validate
```

A Level 3 task should usually require at least **two semantic navigation decisions**.

The exact commands must not be prescribed.

---

# 5. Required Task Categories

Create approximately the following distribution.

## A. Multi-level caller chains

Create 2 tasks.

Example:

```text
main()
  ↓
RequestHandler::handle()
  ↓
OrderService::submit()
  ↓
PaymentService::authorize()
```

The task should require identifying callers at more than one level.

For example:

> Add logging to every request-processing path that eventually invokes `PaymentService::authorize()`.

The task must define a deterministic interpretation of “request-processing path”.

Do not simply ask the agent to modify every textual occurrence of `authorize`.

The repository should contain other callers that must not be modified.

The intended semantic capability is primarily:

```text
callers
```

and possibly:

```text
references
```

---

## B. Multi-level callee chains

Create 2 tasks.

Example:

```text
OrderService::submit()
  ↓
validateOrder()
  ↓
InventoryService::reserve()
  ↓
Database::commit()
```

The task should require identifying a relevant downstream dependency.

For example:

> Add instrumentation immediately before every call to the inventory reservation operation made by the order submission workflow.

The repository should contain other unrelated calls to the same dependency.

The intended semantic capability is primarily:

```text
callees
```

and possibly:

```text
references
```

---

## C. Caller subset / selective modification

Create 1 task.

The target function should have multiple callers.

Only a semantically defined subset should be modified.

For example:

```text
CheckoutService::process()
  ↑
  ├── WebCheckoutHandler
  ├── MobileCheckoutHandler
  ├── RetryWorker
  └── TestHelper
```

The task may require modifying only production request handlers.

The prompt must provide enough semantic criteria to determine the correct subset.

Do not require modifying every caller.

The challenge is:

```text
find target
    ↓
find callers
    ↓
classify callers
    ↓
modify correct subset
```

The validator must confirm that excluded callers remain unchanged.

---

## D. Callee subset / dependency classification

Create 1 task.

A target function should invoke multiple dependencies.

Only one category of dependency should be modified.

For example:

```text
SyncService::run()
  ↓
validate()
authorize()
loadConfig()
saveState()
emitMetrics()
```

The task may require adding instrumentation only around persistence operations.

The agent must identify the relevant callee(s) through code structure and semantic relationships.

Do not reveal the exact implementation file.

---

## E. References + caller/callee combination

Create 1 task.

The task should require combining reference discovery with call graph navigation.

Example conceptual flow:

```text
locate target symbol
        ↓
find references
        ↓
identify which references are calls
        ↓
inspect callers
        ↓
modify selected callers
```

The repository should contain declarations, definitions, and non-call references so that naïve text matching is insufficient.

---

## F. Distributed workflow modification

Create 1 task.

The correct solution should require modifying multiple source files.

The files must not be listed in the prompt.

Example:

```text
Request workflow
    ↓
Authentication
    ↓
Authorization
    ↓
Persistence
```

The task may require adding markers or logging to a particular semantic transition across all relevant workflows.

The expected result should involve approximately:

```text
2–5 source files
```

Avoid excessive patch size.

The challenge is discovering the complete modification set.

---

# 6. Repository Fixtures

Create approximately **2–3 reusable repository fixtures**.

Suggested conceptual structure:

```text
evaluation/repositories/
├── level3-request-flow/
├── level3-order-pipeline/
└── level3-mixed-callgraph/
```

Adapt to the existing repository structure.

Prefer repositories with approximately:

```text
15–35 source/header files
```

This is only a guideline.

Do not inflate repositories with meaningless files.

Every relevant file should contribute to a plausible call graph or source of ambiguity.

---

# 7. Call Graph Design

The call graphs should be intentional and understandable.

Example:

```text
WebHandler::handle()
        │
        ▼
OrderService::submit()
        │
        ├── validateOrder()
        │
        ├── PaymentService::authorize()
        │         │
        │         ▼
        │      Gateway::charge()
        │
        └── Repository::save()
```

Add unrelated paths where necessary:

```text
RetryWorker::retry()
        │
        ▼
PaymentService::authorize()
```

This allows tasks to distinguish:

```text
all callers
```

from:

```text
callers belonging to a specific workflow
```

The graph should support deterministic validation.

---

# 8. Do Not Create Artificial Graph Puzzles

Avoid:

```text
FunctionA
 ↓
FunctionB
 ↓
FunctionC
 ↓
FunctionD
 ↓
FunctionE
```

when the chain exists only to force multiple tool calls.

The call graph should resemble realistic application structure.

Prefer:

```text
Handler
 ↓
Service
 ↓
Repository
```

or:

```text
Worker
 ↓
Coordinator
 ↓
External adapter
```

The difficulty must come from deciding **which semantic path matters**, not simply traversing arbitrary depth.

---

# 9. Task Prompt Rules

Task prompts must describe:

```text
What behavior must change?
Which semantic concept identifies the target?
What must remain unchanged?
```

Task prompts must not describe:

```text
Which files to open
Which ast-tool command to run
Which exact workflow to follow
```

Do not include:

```text
Use ast-tool callers.
```

Do not include:

```text
Run ast-tool callees.
```

Do not include CLI syntax.

The existing Skills remain the source of usage information.

---

# 10. Make Multiple Semantic Steps Useful

Each task should provide a realistic opportunity for a workflow such as:

```text
Skill
 ↓
search / find
 ↓
callers
 ↓
callers
 ↓
Read
 ↓
Edit
```

or:

```text
Skill
 ↓
search
 ↓
callees
 ↓
references
 ↓
Read
 ↓
Edit
```

However, do not require this exact sequence.

The agent may discover an alternative valid strategy.

The evaluation should measure:

```text
whether semantic navigation was useful
```

rather than:

```text
whether a predetermined command sequence was followed
```

---

# 11. Validation Requirements

Every task must have deterministic validation.

Validators should check:

* all required semantic targets were modified;
* unrelated callers or callees were not modified;
* the correct number of modifications was made;
* required includes or dependencies were added where necessary;
* the repository remains buildable if the existing infrastructure supports it.

Do not validate exact file paths unless they are part of the expected repository state.

Do not validate:

```text
ast-tool callers was executed
```

or:

```text
ast-tool callees was executed
```

Tool usage remains evaluation metadata.

---

# 12. Include Negative Paths

At least 3 of the 8 tasks should contain semantically related paths that must **not** be modified.

Examples:

```text
Production handler
        ↓
authorize()

Test helper
        ↓
authorize()

Retry worker
        ↓
authorize()
```

If the task targets production request handling, the retry worker and test helper should remain unchanged.

This is important.

Level 3 should evaluate whether the agent can identify the relevant semantic subset rather than blindly modifying every reference.

---

# 13. Expected Semantic Capabilities

The approximate capability distribution should be:

| Capability        |      Primary in Tasks |
| ----------------- | --------------------: |
| `callers`         |                   3–4 |
| `callees`         |                   3–4 |
| `references`      |                   2–3 |
| `search` / `find` | Supporting capability |

These categories may overlap.

Do not force exact usage counts.

---

# 14. Evaluation Metadata

If the existing task schema supports evaluation-only metadata, record information such as:

```yaml
evaluation:
  category: multi-level-callers
  relevant_capabilities:
    - callers
    - references
```

or:

```yaml
evaluation:
  category: selective-callee-analysis
  relevant_capabilities:
    - callees
    - search
```

Do not add metadata that prescribes an exact workflow.

Do not modify the evaluation runner solely to support new metadata.

---

# 15. Keep a Control Case

At least one Level 3 task may still be solvable through careful manual inspection without `ast-tool`.

This is intentional.

The benchmark should evaluate:

```text
appropriate tool selection
```

not:

```text
maximum ast-tool usage
```

However, most Level 3 tasks should make manual exploration significantly more expensive than semantic navigation.

---

# 16. Measure Complete Workflow Quality

Level 3 should allow later comparison of:

```text
Task success
```

with:

```text
Semantic tool usage
```

and:

```text
Exploration cost
```

Relevant metrics include:

```text
elapsed time
token usage
Grep calls
Read calls
Bash calls
Skill usage
ast-tool command counts
changed files
validation success
```

Do not add new metrics unless required.

Use the existing evaluation infrastructure.

---

# 17. Preserve the Evaluation Baseline

Do not modify:

```text
ast-tool/.claude/skills/
```

Do not modify:

```text
evaluation/tasks/smoke-001.yaml
```

Do not modify:

```text
evaluation/tasks/level1-*.yaml
```

Do not modify:

```text
evaluation/tasks/level2-*.yaml
```

Do not redesign the evaluation runner.

The Level 3 dataset should be evaluated against the same existing Agent-facing environment.

---

# 18. Inspect Existing Level 2 Design First

Before creating Level 3:

1. Inspect all Level 2 tasks.
2. Inspect the Level 2 repository fixtures.
3. Inspect the validators.
4. Inspect the evaluation runner.
5. Inspect the existing Skills.
6. Identify which Level 2 tasks already exercise caller/callee behavior.
7. Avoid duplicating those scenarios without increasing semantic depth.

Level 3 must represent a genuine difficulty progression.

Do not simply rename a Level 2 scenario.

---

# 19. Acceptance Criteria

The work is complete when:

* [ ] Exactly 8 Level 3 task YAML files exist.
* [ ] Each task introduces meaningful call graph or multi-step semantic reasoning.
* [ ] Most tasks require discovering relationships across multiple files.
* [ ] At least 3 tasks contain semantically related paths that must remain unchanged.
* [ ] At least one task combines references with caller/callee reasoning.
* [ ] At least one task requires distributed modification across multiple source files.
* [ ] No task prescribes an `ast-tool` command.
* [ ] No task embeds CLI syntax.
* [ ] Existing Skills remain unchanged.
* [ ] Existing Level 1 and Level 2 tasks remain unchanged.
* [ ] Every task has deterministic validation.
* [ ] The evaluation runner requires no redesign.

---

# 20. Final Deliverables

Create:

```text
evaluation/tasks/level3-001.yaml
evaluation/tasks/level3-002.yaml
evaluation/tasks/level3-003.yaml
evaluation/tasks/level3-004.yaml
evaluation/tasks/level3-005.yaml
evaluation/tasks/level3-006.yaml
evaluation/tasks/level3-007.yaml
evaluation/tasks/level3-008.yaml
```

Create only the additional repository fixtures and validators required to support these tasks.

After implementation, report:

1. The task ID.
2. The repository fixture.
3. The semantic challenge.
4. The intended relevant capabilities.
5. The semantic paths that must be modified.
6. The semantically related paths that must remain unchanged.
7. The expected number of modified files.
8. The validation strategy.
9. Any task that may overlap too heavily with Level 2.
10. Any concern about task determinism.

The main objective is to determine whether Claude Code can use:

```text
Skill
  ↓
Semantic navigation
  ↓
Multi-step relationship analysis
  ↓
Selective code modification
```

as a practical workflow for realistic coding tasks.
