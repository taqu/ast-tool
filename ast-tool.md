# Phase 2-2 — Residual Failure and Agent Trajectory Analysis

## Goal

Analyze the evaluation traces after the Phase 2-1 semantic resolution improvements.

Phase 2-1 significantly reduced `ast-tool` failures, especially for `callers`, `callees`, and `references`, but the reduction in failures did not translate into a comparable reduction in total tool calls or elapsed time.

The goal of this phase is to identify why.

Do **not** modify `ast-tool` behavior in this phase.

---

## Background

Phase 2-1 improved C++ semantic resolution, especially declaration/definition ambiguity.

Observed results:

* AST tool failures: `36 -> 12`
* AST tool failure rate: `51.43% -> 19.05%`
* AST tool retries: `23 -> 17`
* Average recovery distance: `2.23 -> 1.70`
* Total tokens: `162,628 -> 160,530`

However:

* Total tool calls: `519 -> 563`
* Read calls: `254 -> 281`
* Glob calls: `15 -> 35`
* Elapsed time: `2100.56s -> 2329.46s`

This suggests that semantic commands are succeeding more often, but the agent may still perform redundant exploration afterward.

---

## Scope

Analyze the existing evaluation traces and classify two things:

1. Remaining `ast-tool` failures
2. Tool-call trajectories after successful semantic queries

Focus especially on:

* `callers`
* `references`
* `callees`
* `search`

Also investigate unexpected fallback usage of:

* `Glob`
* `Grep`
* `Read`
* repeated `search`
* `find`
* `symbols`

---

## Task 1 — Classify Remaining AST Tool Failures

Inspect every remaining failed `ast-tool` invocation from the Phase 2-1 evaluation.

Group failures into meaningful categories, for example:

* unresolved declaration/definition ambiguity
* overload ambiguity
* namespace/class resolution issue
* invalid or overly broad query
* unsupported symbol form
* CLI usage error
* empty result interpreted as failure
* unrelated parser/semantic issue
* agent misuse

For each category, report:

* number of occurrences
* affected commands
* representative command sequence
* likely root cause
* whether the issue belongs to:

  * Semantic Layer
  * Resolver
  * CLI/API UX
  * Skill.md
  * Agent behavior

Do not fix the failures yet.

---

## Task 2 — Analyze Post-Success Trajectories

For every successful invocation of:

```text
callers
references
callees
```

inspect the following tool calls in the same evaluation task.

Identify cases such as:

```text
search
→ callers succeeds
→ Glob
→ Read
→ Read
```

or:

```text
callers succeeds
→ search
→ find
→ symbols
```

Determine whether the agent is performing redundant verification after already receiving a usable semantic result.

Classify post-success behavior into categories such as:

* semantic result used directly
* result followed by required source inspection
* redundant Glob fallback
* redundant Grep fallback
* redundant repeated semantic query
* result lacked enough information
* agent did not appear to trust the result
* task genuinely required additional exploration

---

## Task 3 — Measure Post-Success Exploration Cost

Add trace-analysis metrics if needed.

At minimum, report:

```text
successful semantic queries
successful semantic queries followed by Glob
successful semantic queries followed by Grep
successful semantic queries followed by Read
successful semantic queries followed by another semantic lookup
```

Also report, where practical:

```text
average tool calls after successful semantic query
average Read calls after successful semantic query
average fallback calls after successful semantic query
```

A small bounded window such as the next 3–5 tool calls is acceptable, but document the chosen definition.

---

## Task 4 — Validate Trace Analyzer Consistency

There are inconsistencies between the command summary table and the JSON statistics.

Examples observed include differing counts for:

```text
search
callers
references
help
```

and an unexpected command entry:

```text
"2": 1
```

Investigate the trace parser / command classifier.

Fix the analyzer if necessary, but do not alter `ast-tool` itself.

The table and JSON summary must be generated from the same normalized command classification and must agree.

Unknown or malformed commands should be reported explicitly rather than silently classified as a valid command.

---

## Non-goals

Do not:

* modify semantic resolution
* modify CLI output
* add symbol-ID APIs
* change `Skill.md`
* change command behavior
* remove subcommands
* optimize performance
* change evaluation tasks

This phase is analysis and measurement only, except for fixes to the trace-analysis tooling itself.

---

## Deliverables

Produce a report containing:

### 1. Remaining Failure Summary

Example:

```text
Category                         Count
--------------------------------------
overload ambiguity                  3
references resolution issue         2
agent CLI misuse                    2
...
```

### 2. Post-Success Trajectory Summary

Example:

```text
Behavior                                      Count
---------------------------------------------------
semantic result used directly                   12
followed by necessary Read                       8
followed by redundant Glob                       6
followed by redundant Grep                       2
followed by repeated semantic lookup             4
```

### 3. Representative Traces

Include a few short examples showing the exact command trajectory.

For example:

```text
search validate
callers auth::AuthToken::validate .
Glob **/*.cpp
Read src/auth/auth_token.cpp
```

Explain whether the additional calls were necessary.

### 4. Root-Cause Assessment

Determine which of these is currently the dominant bottleneck:

```text
A. remaining resolver defects
B. insufficient semantic command output
C. Skill.md guidance
D. redundant agent verification / distrust
E. trace-analysis artifact
```

Multiple causes may exist, but rank them.

### 5. Recommended Next Phase

Recommend exactly one primary implementation target for the next phase.

Examples:

```text
Improve remaining resolver cases
```

or:

```text
Improve semantic output so successful callers/references results
contain enough context to avoid Glob/Read fallback
```

or:

```text
Update Skill.md to explicitly trust successful semantic results
and avoid redundant filesystem verification
```

Do not implement that recommendation in this phase.

---

## Acceptance Criteria

This phase is complete when:

1. Every remaining Phase 2-1 `ast-tool` failure is classified.
2. Successful `callers`, `references`, and `callees` calls have been analyzed for follow-up exploration.
3. The increase in `Glob` / `Read` usage is explained with trace evidence.
4. Command-count inconsistencies in the trace analyzer are understood and fixed if they are analyzer bugs.
5. The report identifies the dominant remaining bottleneck.
6. One concrete next implementation phase is recommended.
7. No `ast-tool` behavior has been changed.

The purpose of this phase is to avoid making another implementation change before we know whether the next bottleneck is semantic correctness, output usability, Skill guidance, or redundant agent exploration.
