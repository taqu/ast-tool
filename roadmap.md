# Revised AST Tool Roadmap

## Project Objective

The goal is not to maximize AST Tool usage or semantic sophistication by itself.

The primary objective is to improve coding-agent efficiency:

```text
Agent efficiency
├── success rate ↑
├── total tokens ↓
├── latency ↓
├── recovery cost ↓
└── unnecessary exploration ↓
```

AST Tool call count and AST Tool failure rate are diagnostic metrics, not optimization targets by themselves.

A change is successful only when it improves end-to-end agent behavior.

---

# Current Experimental Findings

## Phase 1 — Skill.md

Completed.

Main result:

* Agent guidance improved.
* Help usage remained low.
* The decision-tree approach was useful.
* No major correctness regression was introduced.

---

## Phase 2 — Output / JSON UX

Completed and accepted as the current stable baseline.

Phase 2 metrics:

```text
tests:                       41
successes:                   37
failures:                     4
success rate:                90.24%

total tool calls:            519
average tool calls/test:     12.66

ast-tool calls:               70
ast-tool failures:            36
ast-tool failure rate:       51.43%
ast-tool retries:             23

grep calls:                   18
read calls:                  254

elapsed:                  2100.56 sec

total tokens:             162,628
average tokens/test:        3,966.5
```

Compared with Phase 1, Phase 2 reduced:

```text
total tool calls
total tokens
elapsed time
grep usage
read usage
```

while preserving the same 90.24% success rate.

Therefore:

> Phase 2 is the current known-good implementation baseline.

All subsequent phases should branch from or preserve Phase 2 behavior unless explicitly stated otherwise.

---

# Phase 3 — Semantic Symbol Resolution

## Status

```text
DEFERRED
```

Do not include the current Phase 3 implementation in the mainline baseline.

Restore or retain Phase 2 behavior.

## Experimental Result

Attempting to unify C++ declarations and definitions at the semantic-symbol layer caused a major end-to-end regression.

Phase 2 → Phase 3:

```text
success rate:
90.24% → 60.98%

total tokens:
162,628 → 253,785

average tokens/test:
3,966.5 → 6,189.9

ast-tool calls:
70 → 15

grep calls:
18 → 71
```

Although the reported AST Tool failure rate decreased:

```text
51.43% → 13.33%
```

this was not an end-to-end improvement.

AST Tool usage itself dropped sharply, while grep fallback increased significantly and task success collapsed.

The lower AST Tool failure rate is therefore not sufficient evidence of better semantic resolution.

## Decision

Do not continue trying to solve declaration/definition identity as part of the current optimization track.

Do not attempt additional broad C++ semantic-resolution changes unless future evidence demonstrates a low-risk implementation strategy.

The problem may be revisited later as an isolated research track.

---

# Phase 4 — Stable Semantic Symbol ID

## Status

```text
DEFERRED
```

The original Phase 4 depended on reliable logical semantic identity:

```text
declaration
definition
    ↓
same logical symbol
    ↓
stable semantic ID
```

Because Phase 3 is deferred, public stable semantic IDs are also deferred.

Do not expose:

```text
callers --id
references --id
callees --id
```

at this stage.

Internal IDs may continue to exist, but they must not become a new public API dependency until semantic identity is sufficiently stable.

---

# Phase 5 — Error Recovery UX

## Status

```text
NEXT
```

## Goal

Reduce the cost of recovering from failed semantic queries without attempting to make all semantic queries succeed.

The key design shift is:

```text
Old goal:
make semantic resolution always succeed

New goal:
when semantic resolution fails,
make the next correct action obvious and cheap
```

The agent already succeeds on approximately 90% of evaluation tasks despite a high semantic-command failure rate.

Therefore, improving recovery behavior may provide a better cost/benefit ratio than increasing resolver complexity.

## Focus

Improve actionable errors for:

```text
callers
references
callees
search
find
```

Especially:

```text
ambiguous symbol
symbol not found
invalid query
unsupported query form
unknown option
empty result
```

Errors should explain:

```text
what failed
why it failed, when known
what useful information is available
what the agent should try next
```

without producing excessive output.

---

# Phase 6 — Agent-facing Command Surface

## Goal

Reduce decision complexity for the agent without breaking CLI compatibility.

Classify commands into:

```text
Core
Support
Debug / Low-level
Infrastructure
```

Recommended classification:

### Core

```text
search
callers
references
callees
symbols
```

### Support

```text
find
outline
```

### Debug / Low-level

```text
parent
children
range
```

### Infrastructure

```text
cache
setup
```

Do not initially delete commands.

Prefer reducing discoverability of low-value commands for agent workflows.

The desired result is that normal coding-agent trajectories mostly operate on a small command vocabulary.

---

# Phase 7 — Skill.md Compression

## Goal

After Phase 5 and Phase 6 stabilize, simplify Skill.md again.

Do not prematurely encode workarounds for every resolver limitation.

The target Skill should eventually be close to:

```text
Find a symbol       → search
Find callers        → callers
Find references     → references
Find callees        → callees
Inspect file symbols → symbols
Inspect AST structure → find

If a semantic query fails, follow the recovery guidance in the error.

Use compact output.
Do not use --help unless necessary.
Do not repeat an unchanged failed command.
```

The purpose is to minimize both:

```text
Skill prompt tokens
agent decision complexity
```

---

# Phase 8 — Evaluation and Trajectory Analysis

## Goal

Measure the complete revised system against the Phase 2 stable baseline.

Compare:

```text
Phase 2 baseline
      ↓
Error Recovery UX
      ↓
Command Surface
      ↓
Compressed Skill
```

Do not compare only the final version against Phase 1.

Preserve intermediate results so the effect of each change remains identifiable.

---

# Phase 9 — Optional Semantic Research Track

This phase is optional and independent from the main optimization roadmap.

Only revisit semantic identity if:

```text
1. Error UX and command-surface improvements plateau.
2. Semantic failures remain a dominant cause of task failure.
3. A narrow, testable resolver improvement becomes available.
```

Any future semantic work should be incremental.

Do not begin again with general declaration/definition unification across C++.

Instead isolate narrowly scoped cases, for example:

```text
exact qualified method lookup
same-file declaration/definition pairing
specific header/source pairing
resolver ranking without identity merging
```

Each experiment must pass end-to-end evaluation before being retained.

---

# Revised Implementation Order

```text
P0  Baseline / Trace Metrics
 │
 ✓
 │
P1  Skill.md Decision Tree
 │
 ✓
 │
P2  Output / JSON UX
 │
 ✓  ← STABLE BASELINE
 │
 ├─────────────────────────────┐
 │                             │
P3  Semantic Resolver          │
 │   DEFERRED                  │
 │                             │
P4  Stable Symbol ID           │
 │   DEFERRED                  │
 │                             │
 └─────────────────────────────┘
 │
P5  Error Recovery UX
 │
 ├─ actionable errors
 ├─ bounded candidate hints
 ├─ recommended next action
 └─ no semantic redesign
 │
P6  Agent-facing Command Surface
 │
 ├─ Core
 ├─ Support
 ├─ Debug
 └─ Infrastructure
 │
P7  Skill.md Compression
 │
 └─ encode the stabilized workflow
 │
P8  Quantitative Evaluation
 │
 └─ compare against Phase 2
 │
P9  Optional Semantic Research
     └─ only narrow, isolated experiments
```

---

# Experimental Discipline

For all future phases:

```text
One Coding Agent run = one phase
```

Do not mix:

```text
semantic implementation
CLI redesign
Skill changes
error UX
evaluation changes
```

in one experiment.

Every phase must state:

```text
Goal
Scope
Non-goals
Implementation
Tests
Evaluation
Acceptance Criteria
```

If a phase decreases task success rate substantially, revert it even if a local metric improves.

The evaluation suite is the source of truth.

---

# Acceptance Philosophy

A change such as:

```text
AST Tool failures ↓
```

is not automatically an improvement.

Similarly:

```text
AST Tool calls ↑
```

or:

```text
AST Tool calls ↓
```

is neither inherently good nor bad.

The primary evaluation order should be:

```text
1. Success rate
2. Total tokens
3. Recovery / exploration cost
4. Latency
5. Tool-call count
6. Individual AST Tool metrics
```

Large regressions in success rate must not be accepted in exchange for improvements in secondary metrics.
