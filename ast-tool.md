# Phase 6 Regression Analysis

Analyze the Phase 6 regression in the AST Tool / Coding Agent evaluation.

Do not modify the implementation yet. First identify the most likely causes from the repository, Skill.md, CLI help text, and evaluation traces.

## Context

Phase 5 is the current stable baseline.

Phase 5:

```text
tests                      41
successes                  37
success rate               90.24%

total tool calls           518
AST Tool calls              69
AST failures                 9
AST failure rate           13.04%
AST retries                  9

avg recovery distance       1.44
max recovery distance          2

grep                         15
read                        252

total tokens            158,303
avg tokens/test           3,861
```

Phase 6:

```text
tests                      41
successes                  25
success rate               60.98%

total tool calls           444
AST Tool calls              15
AST failures                 4
AST failure rate           26.67%
AST retries                  4

avg recovery distance       2.25
max recovery distance          5

grep                         73
read                        188

total tokens            252,242
avg tokens/test           6,152.2
```

AST Tool command usage in Phase 6:

```text
search       4
callers      2
references   5
find         3
"2"          1
```

The important regression pattern is:

```text
AST Tool usage: 69 -> 15
grep:           15 -> 73
success:        37 -> 25
tokens:         158k -> 252k
```

This is very similar to the earlier Phase 3 failure mode, where AST Tool usage collapsed and the agent fell back to grep/manual exploration.

## Phase 6 Intent

Phase 6 was intended to change only the agent-facing command surface:

* help grouping
* command ordering
* category headings
* discoverability

It was not intended to change:

* command semantics
* command syntax
* semantic resolution
* Skill.md
* JSON output
* Phase 5 error recovery behavior

The intended command classification was approximately:

```text
Primary:
  search
  callers
  references
  callees
  find
  symbols

Secondary:
  outline

Debug / low-level:
  parent
  children
  range

Infrastructure:
  cache
  setup
```

## Existing Skill.md Routing Model

The Skill.md already gives the agent a direct task-to-command decision tree similar to:

```text
Find symbol       -> search
Find callers      -> callers
Find references   -> references
Find callees      -> callees
Find file symbols -> symbols
Need AST structure -> find
```

It also tells the agent:

```text
Do not retry unchanged failed commands.
Do not use --pretty by default.
Do not dump entire workspace.
Do not use --help for ordinary discovery.
```

## Main Hypothesis to Investigate

Investigate whether Phase 6 introduced a conflict, ambiguity, or competing routing model between:

1. Skill.md
2. top-level `ast-tool --help`
3. per-command help or descriptions
4. any command metadata exposed to the Coding Agent

Do not limit the analysis to literal contradictions.

A contradiction can also be semantic or behavioral, for example:

```text
Skill.md strongly maps an intent to `search`
but
CLI help makes `find`, `search`, and generic text search look like overlapping alternatives.
```

This can increase command-selection uncertainty even if no statement is literally contradictory.

Pay particular attention to why:

```text
search:  30 calls in Phase 5 -> 4 in Phase 6
callers: 14 calls in Phase 5 -> 2 in Phase 6
```

while grep increased dramatically.

## Tasks

1. Inspect the Phase 5 -> Phase 6 diff.

2. Identify every change that can affect what the Coding Agent sees or infers about available commands.

3. Compare the routing model presented by Skill.md with the routing model implied by CLI help.

4. For each primary intent, build a table:

```text
Intent | Skill.md recommendation | Phase 6 help implication | Conflict/ambiguity
```

At minimum analyze:

* locate a symbol
* find callers
* find references
* find callees
* inspect AST structure
* inspect symbols in a file

5. Look specifically for:

   * duplicated guidance
   * competing command recommendations
   * changed wording
   * changed command ordering
   * reduced salience of `search`
   * `search` vs `find` ambiguity
   * semantic commands appearing less important or more specialized
   * grouping that may cause the agent to ignore the AST Tool
   * help text that encourages generic grep/manual inspection
   * accidental changes outside the intended Phase 6 scope

6. Inspect failed evaluation traces where possible.

Compare successful Phase 5 trajectories with failed Phase 6 trajectories and identify the first meaningful divergence, especially transitions such as:

```text
expected:
search -> callers/references -> edit

actual:
grep -> read -> grep -> read -> ...
```

7. Determine whether the regression is primarily:

A. Skill.md/help inconsistency
B. command discoverability/salience regression
C. accidental command behavior regression
D. evaluation/tooling artifact
E. another cause

Rank hypotheses by evidence.

8. Investigate the recurring command classification:

```text
"2": 1
```

but keep this separate unless there is evidence that it contributed materially to the Phase 6 regression.

## Important Constraints

Do not implement fixes during this analysis.

Do not assume that lower AST Tool failure counts indicate an improvement. AST Tool usage itself collapsed.

Optimize for Coding Agent end-to-end behavior, not AST Tool-local metrics.

Phase 5 remains the stable baseline.

## Output

Produce:

### 1. Executive conclusion

A concise explanation of the most likely cause of the Phase 6 regression.

### 2. Evidence

Reference concrete files, diffs, help text, Skill.md rules, and evaluation traces.

### 3. Skill.md vs Help comparison

Show conflicting or ambiguous routing guidance explicitly.

### 4. Failure trajectory analysis

Explain why the agent moved from AST Tool usage toward grep/manual exploration.

### 5. Ranked root-cause hypotheses

For each hypothesis provide:

* evidence for
* evidence against
* confidence

### 6. Minimal remediation recommendation

Recommend the smallest Phase 6b change likely to restore Phase 5 behavior.

Prefer restoring Phase 5 discoverability and removing conflicting guidance over adding more instructions.

Do not propose Phase 7 Skill.md compression until Phase 6 behavior is stable again.
