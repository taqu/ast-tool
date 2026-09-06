# Phase 7f — Semantic Routing Value and Toolset Cost Audit

## Objective

Determine **when semantic routing provides real agent-level value** and identify where the current semantic-analysis toolset introduces unnecessary cost, retries, or decision overhead.

Phase 7f must not optimize Skill invocation rate in isolation.

The main questions are:

```text
1. For which task classes is semantic routing better
   than manual repository exploration?

2. When semantic routing is more expensive,
   where does the extra cost come from?

3. Which semantic calls provide unique useful information?

4. Which calls are resolution, retry, structural, or context-fetch overhead?

5. Can existing command semantics be improved
   before adding new commands or changing Skill guidance?
```

The output of Phase 7f should be an evidence-backed semantic toolset assessment, not necessarily a code change.

---

# Background

Phase 7e established several important facts.

## Invocation is task-dependent

Some relationship-oriented tasks consistently selected `semantic-analysis`, while other task classes consistently did not.

## Invocation is also stochastic

At least one unchanged task, `level3-007`, selected semantic routing in some runs and manual exploration in others.

## Invocation is an initial routing decision

When `semantic-analysis` was invoked, it was always the first action.

No observed trajectory began with exploration and loaded the Skill later.

Therefore:

```text
SKILL.md body changes cannot fix a trajectory
in which the Skill was never selected.
```

## More semantic routing is not automatically better

For `level3-007`, Skill-loaded runs:

```text
used targeted AST queries
used fewer Reads
used almost no Grep
```

but also:

```text
used more AST/Bash work
used more tokens
took more elapsed time
```

with unchanged correctness.

Therefore Phase 7f must not assume:

```text
semantic routing
=
lower agent-level cost
```

The goal is to determine where semantic routing is beneficial, neutral, or unnecessarily expensive.

---

# Baseline

Use the accepted Phase 7d semantic-analysis Skill body unchanged.

Do not change:

```text
Skill body
Skill description
Skill name
Skill metadata
Skill registration
AST Tool command behavior
```

during the initial audit.

Record hashes before and after evaluation when practical.

Phase 7f begins as an **observational and analytical phase**.

---

# Primary Questions

## 1. Which tasks benefit from semantic routing?

Identify task classes where semantic routing produces measurable improvement in one or more of:

```text
correctness
semantic precision
recovery
tool count
Read count
token usage
elapsed time
manual exploration
```

Do not require every metric to improve.

A semantic route may be valuable if it significantly reduces broad exploration or ambiguity even when total tool count remains similar.

---

## 2. Which tasks do not benefit?

Identify cases where:

```text
manual exploration is cheaper
or
semantic routing adds calls without improving correctness
or
semantic routing only replaces Reads with AST/Bash overhead
```

Do not classify these as failures automatically.

The goal is to understand whether semantic routing is unnecessary for that task class or whether the semantic toolset is inefficient.

---

## 3. Where does semantic cost come from?

For every semantic trajectory, classify each AST call by purpose.

Use at least these categories:

```text
A. Necessary semantic query
B. Identity / resolution overhead
C. Relationship retry overhead
D. Redundant structural lookup
E. Context-fetch overhead
F. Recovery / error-handling overhead
```

Examples:

```text
search exact target needed by task
    → A

search only because callers could not resolve an under-qualified name
    → B

callers
→ ambiguity/failure
→ search
→ callers
    → B + C

refined search already identifies target
→ find only to locate it
    → D

find
→ immediate Read of same implementation
    → possible E
```

The exact classification may evolve if the evidence requires additional categories.

---

## 4. Are command boundaries causing extra agent decisions?

Investigate whether the agent must perform multi-step protocols that could reasonably be handled by one existing semantic command.

Examples:

```text
search
→ callers

search
→ references

search
→ callees

search
→ find
→ Read
```

For each repeated pattern, ask:

```text
Was each step semantically necessary?

Did the later command require information
that the earlier command could have returned?

Could an existing command safely absorb
the resolution or context step?
```

Do not implement changes yet.

---

## 5. Are new commands actually needed?

A new semantic command must not be proposed merely because a trajectory is long.

First determine whether the inefficiency can be fixed by:

```text
better input resolution
better result metadata
better declaration/definition discrimination
better context in existing output
better ambiguity handling
```

Only recommend a new command if repeated evidence shows that the current command set cannot express the required operation cleanly.

---

# Evaluation Design

## Stage 1 — Build a Semantic-Value Probe Cohort

Select existing tasks representing at least the following categories:

```text
direct symbol lookup
references
callers
callees
ambiguous identity
multi-level relationships
distributed workflow
structural lookup
API evolution / broad modification
recovery-sensitive case
```

Prefer existing Phase 7 / Phase 7e tasks.

Do not create new fixtures unless a specific semantic inefficiency cannot be represented by the current suite.

Target approximately:

```text
8–12 tasks
```

---

## Stage 2 — Obtain Paired Routing Evidence

For each selected task, obtain both:

```text
semantic route
and
non-semantic/manual route
```

when possible.

Preferred evidence order:

```text
1. Naturally stochastic same-task runs
2. Historical same-task traces
3. Controlled runs with semantic routing forced
4. Manual/non-Skill control runs
```

Same-task comparisons are strongly preferred over unrelated-task aggregates.

Do not treat routing mode as randomized unless it actually is.

---

## Stage 3 — Repeat Where Variance Is Material

For tasks whose route or cost varies strongly, run repeated trials.

Recommended minimum:

```text
5 runs per routing condition
```

For highly stochastic tasks:

```text
10 runs per condition or observed state
```

Do not make a semantic-tool recommendation from one trace.

---

# Metrics

For every run record:

```text
task
success
routing mode
Skill invoked?
first action
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
symbols
grep
glob
read
bash
edit
tokens
elapsed
recovery mean/max
```

Also record the ordered AST trajectory.

Example:

```text
search
→ callers
→ references
```

or:

```text
callers
→ search
→ callers
```

---

# Semantic Call Annotation

For each AST call in the probe cohort, annotate:

```text
command
target
success/failure
information gained
reason for call
next action
classification
```

Example:

```text
command:
    find

reason:
    locate implementation after exact search

information gained:
    no new identity information

next action:
    Read same source region

classification:
    redundant structural lookup
```

The annotation must distinguish:

```text
call was unnecessary in hindsight
```

from:

```text
call was reasonable but command semantics
forced an extra step
```

This distinction is important.

---

# Required Pattern Analysis

At minimum investigate the following known patterns.

## Pattern 1 — Relationship target resolution

```text
under-qualified callers/callees/references
→ failure or ambiguity
→ search
→ relationship retry
```

Measure:

```text
frequency
token/tool cost
recovery cost
whether search uniquely resolved identity
whether relationship command could resolve internally
```

Question:

```text
Should callers/callees/references perform
built-in target resolution?
```

Do not answer until repeated traces support it.

---

## Pattern 2 — Search followed by redundant find

```text
refined search
→ exact target identified
→ find
```

Phase 7d already showed that some of these calls are redundant.

Measure whether any remaining occurrences are:

```text
necessary structural lookup
or
location-only overhead
```

Do not remove structural `find` use.

---

## Pattern 3 — Declaration versus definition

```text
search finds declaration
→ second search/find/read needed
→ implementation located
```

Measure how often this occurs.

Ask whether `search` result metadata should make declaration/definition roles clearer.

Possible useful fields may include:

```text
declaration
definition
qualified name
file
range
symbol kind
canonical identity
```

Do not add fields without evidence that they reduce downstream work.

---

## Pattern 4 — Find followed by Read

```text
find
→ Read
```

Determine why Read is still required.

Possible reasons:

```text
find provides node identity only
body context missing
surrounding class/function context missing
agent needs editable source text
output too structural
```

Measure whether this pattern is frequent and costly.

Do not assume that embedding more source in `find` is automatically better; larger output may increase context cost.

---

## Pattern 5 — Repeated relationship calls

Examples:

```text
callers×2
callees×2
references×2
```

Determine whether repetition comes from:

```text
pagination
ambiguity
partial results
wrong identity
different target forms
agent uncertainty
```

Separate legitimate repeated queries from avoidable retries.

---

# Semantic Value Classification

For each task classify semantic routing into one of:

```text
A. Clearly beneficial
B. Beneficial with tradeoffs
C. Roughly neutral
D. More expensive but semantically useful
E. Unnecessary / inferior for this task
F. Inconclusive due to variance
```

Base the classification on task-level evidence, not aggregate preference.

Include the reasoning.

---

# Tool-Level Efficiency Classification

For each command, summarize observed roles.

Example format:

```text
search
    essential:
        exact symbol resolution
        identity disambiguation

    overhead:
        repeated refinement
        declaration/definition rediscovery

find
    essential:
        AST structure
        node detail

    overhead:
        simple location after exact search

callers
    essential:
        relationship discovery

    overhead:
        under-qualified target recovery
```

Do this for:

```text
search
find
callers
callees
references
symbols
```

where evidence exists.

Do not infer behavior for commands not represented in the traces.

---

# Command-Boundary Analysis

For every repeated multi-call semantic pattern, evaluate three possible explanations:

```text
1. Correct decomposition
   Each command represents a genuinely separate question.

2. Agent protocol overhead
   Existing commands can answer the task,
   but the agent must manually coordinate them.

3. Semantic API limitation
   Existing commands do not expose enough information
   to express the operation efficiently.
```

This classification is central to Phase 7f.

---

# Candidate Improvement Generation

Only after the audit, propose candidate improvements.

Candidates must be narrow and evidence-backed.

Preferred order:

```text
1. Improve existing command semantics
2. Improve existing result metadata
3. Improve ambiguity/error response
4. Improve context returned by existing commands
5. Add a new command only if necessary
```

Examples of acceptable candidate hypotheses:

```text
callers should resolve an unambiguous short name internally

search should distinguish declaration and definition explicitly

find should optionally return a minimal implementation excerpt
```

These are hypotheses, not required changes.

---

# Do Not Do During the Audit

Do not:

```text
rewrite SKILL.md
change Skill invocation description
add generic routing instructions
merge commands
delete commands
add new commands
modify AST Tool output
optimize for lower AST-call count alone
```

Phase 7f must first establish the cost model.

---

# Important Non-Goals

Phase 7f is not trying to:

```text
maximize semantic-analysis invocation
minimize AST Tool calls at any cost
eliminate Grep
eliminate Read
force every task through AST Tool
redesign the entire CLI
```

A manual route may be correct and cheaper for some tasks.

That is an acceptable result.

---

# Acceptance Criteria

Phase 7f is successful if it produces evidence sufficient to answer:

```text
1. Which task classes clearly benefit from semantic routing?

2. Which task classes do not?

3. Which semantic commands contribute unique useful information?

4. Which repeated call patterns represent avoidable overhead?

5. Which inefficiencies are caused by:
   - agent behavior
   - command semantics
   - missing result information
   - unavoidable task complexity

6. Is there at least one narrow semantic-tool improvement
   worth implementing and validating?
```

A successful Phase 7f does not require finding an improvement.

The valid result may be:

```text
CURRENT TOOLSET IS ADEQUATE;
NO CHANGE JUSTIFIED
```

---

# Decision Outcomes

Choose one primary outcome.

## CURRENT TOOLSET ADEQUATE

Use when:

```text
semantic routing is beneficial where expected
and
observed overhead is mostly stochastic or task-inherent
and
no repeated command-boundary problem is found
```

Proceed without semantic API changes.

---

## EXISTING COMMAND SEMANTICS SHOULD BE IMPROVED

Use when:

```text
one or more repeated trajectories
show avoidable multi-call protocol overhead
and
the operation can be absorbed safely
into existing commands
```

Prefer this over adding new commands.

---

## RESULT METADATA SHOULD BE IMPROVED

Use when:

```text
the command finds the right semantic object
but the agent immediately performs another query/read
because required identity or context information is missing
```

---

## NEW SEMANTIC CAPABILITY JUSTIFIED

Use only when:

```text
repeated tasks require an operation
that the current command set cannot express cleanly
```

Document why existing command semantics cannot solve it.

---

## INCONCLUSIVE

Use when:

```text
within-task variance is too large
or
the probe cohort does not contain enough paired evidence
```

Do not implement speculative changes.

---

# If an Improvement Candidate Is Found

Do not immediately modify multiple behaviors.

Select exactly one candidate.

Create a follow-up experiment:

```text
baseline
→ one semantic API change
→ targeted replay
→ repeated guard validation
→ controlled cohort
→ normal agent-level validation
```

This should follow the methodology established in Phase 7.

---

# Deliverables

Produce a final Phase 7f report containing:

```text
1. Environment and revisions

2. Baseline Skill / AST Tool verification

3. Probe cohort

4. Routing conditions and repetition counts

5. Task-level semantic-value classification

6. Loaded/semantic versus manual paired comparisons

7. Annotated AST-call dataset

8. Command-level efficiency summary

9. Repeated semantic trajectory patterns

10. Command-boundary analysis

11. Agent protocol overhead findings

12. Semantic API limitation findings

13. Candidate improvements, ranked by evidence

14. Explicit rejected hypotheses

15. Final decision

16. Recommended next experiment
```

Keep raw measurements separate from interpretation.

---

# Preferred Reporting Tables

## Task-level routing value

```text
Task
Semantic runs
Manual runs
Success delta
Tool delta
Token delta
Elapsed delta
Read delta
Semantic-value class
```

## Semantic overhead

```text
Task
AST trajectory
Necessary calls
Resolution overhead
Retry overhead
Structural overhead
Context-fetch overhead
```

## Command assessment

```text
Command
Useful cases
Repeated overhead pattern
Frequency
Estimated cost
Candidate change
Evidence strength
```

---

# Evidence Standard

Use the following hierarchy:

```text
strong:
    repeated same-task paired behavior

moderate:
    repeated pattern across several tasks

weak:
    single-run trajectory

insufficient:
    aggregate correlation without trajectory evidence
```

Do not promote a semantic API change from weak evidence alone.

---

# Working Hypothesis

The current hypothesis is not that the semantic toolset is fundamentally wrong.

The hypothesis to test is:

```text
The current semantic commands are useful,
but some boundaries may force the agent
to perform extra identity resolution,
relationship retries,
structural lookup,
or source-context retrieval.
```

Phase 7f should determine whether that hypothesis is true.

---

# Development Principle

Continue using the Phase 7 methodology:

```text
stable baseline
→ observe repeated inefficiency
→ isolate the semantic decision
→ measure same-task cost
→ classify the source of overhead
→ propose one narrow change
→ validate causally
```

For Phase 7f, optimize:

```text
semantic information gained
per agent decision / tool cost
```

not:

```text
number of AST Tool calls
```

and not:

```text
Skill invocation rate
```
