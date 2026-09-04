# Phase 7c — Semantic-Preserving `SKILL.md` Compression

## Objective

Create a compressed version of the Phase 5 `semantic-analysis/SKILL.md` while preserving its behavioral meaning as closely as possible.

Phase 7c is not a continuation of the Phase 7b text.

Start from the **Phase 5 stable `SKILL.md`**.

The goal is:

```text
Preserve every behaviorally meaningful Phase 5 instruction
+
Remove only non-behavioral redundancy
+
Reduce instruction size conservatively
```

The primary requirement is **semantic equivalence**, not maximum compression.

---

# Background

Phase 8 rejected the Phase 7b candidate as the final agent-level system.

Correctness remained unchanged, but the final run showed:

```text
real AST commands    68 → 22
grep                 15 → 64
avg recovery         1.44 → 2.60
max recovery            2 → 5
total tokens    158,303 → 241,130
```

The Phase 8 analysis did not prove that the compressed Skill body itself caused the regression, because Skill invocation varied heavily between runs.

However, Phase 7b also did not provide sufficient evidence that the compressed body was behaviorally equivalent to Phase 5.

Therefore Phase 7c must use a stricter methodology.

---

# Starting Point

Use the exact Phase 5 version of:

```text
semantic-analysis/SKILL.md
```

as the source of truth.

Do not start from Phase 7a or Phase 7b.

Do not copy their compressed structure unless a specific transformation can be shown to preserve a Phase 5 behavioral rule.

Do not assume that text removed in Phase 7a or Phase 7b was safe merely because previous evaluations did not prove a causal regression.

---

# Core Principle

Use this rule throughout the phase:

```text
Compress wording,
not decisions.
```

Every Phase 5 instruction that can influence:

```text
which command the agent chooses
when it chooses that command
how it scopes the query
what it does after failure
when it retries
when it uses help
when grep is allowed
how ambiguity is resolved
how much output it requests
```

must remain represented in the compressed version.

---

# Definition of Semantic Preservation

A compressed instruction is acceptable only when an agent following it would be expected to make the same decision as an agent following the corresponding Phase 5 instruction.

For every behavioral rule:

```text
Phase 5 rule
        ↓
compressed equivalent
```

must preserve:

```text
trigger condition
recommended action
prohibited action
fallback condition
exception
```

where applicable.

Do not preserve only the general intent while dropping decision boundaries.

---

# Rule Classification

Before editing `SKILL.md`, classify the Phase 5 content.

Every meaningful section, paragraph, bullet, or rule should be assigned to one of the following categories.

## A. ROUTING CONTRACT

Instructions that determine which AST Tool command should be used.

Examples include:

```text
Find symbol       → search
Find callers      → callers
Find references   → references
Find callees      → callees
Find file symbols → symbols
Need AST structure → find
```

These are protected.

---

## B. BEHAVIORAL DISAMBIGUATION

Instructions that clarify boundaries between commands or constrain how they are used.

Examples may include:

```text
search vs find
callers vs references
directory root selection
FQN matching
duplicate C++ declarations/definitions
query narrowing
textual lookup vs semantic lookup
```

These are also protected.

Do not classify them as redundant explanation merely because they elaborate on the routing table.

---

## C. RECOVERY CONTRACT

Instructions controlling behavior after failure.

Examples include:

```text
do not retry an unchanged failed command
use the cheapest error-directed correction
limit failed attempts
avoid unnecessary help
narrow ambiguous queries
fallback only after semantic attempts fail
```

These are protected.

---

## D. OUTPUT / COST BOUNDARY

Instructions preventing unnecessarily expensive behavior.

Examples include:

```text
avoid broad workspace dumps
avoid --pretty by default
prefer targeted output
avoid manual grep/read substitution when a semantic query directly applies
```

These are protected if they affect agent behavior.

---

## E. BEHAVIORAL EXAMPLE

An example that resolves an otherwise ambiguous rule.

These may be compressed, but only if the ambiguity remains resolved.

---

## F. NON-BEHAVIORAL EXPLANATION

Content whose removal would not change the agent's next action.

Typical candidates:

```text
background rationale
why AST Tool is useful
historical explanation
restatement of an already explicit rule
ordinary syntax examples
output-schema documentation
implementation details
```

These are the primary deletion candidates.

---

# Required Rule-Equivalence Map

Before producing the final compressed Skill, create an internal mapping of Phase 5 instructions to Phase 7c instructions.

The mapping should conceptually look like:

| Phase 5 rule                                     | Category       | Phase 7c equivalent           | Transformation |
| ------------------------------------------------ | -------------- | ----------------------------- | -------------- |
| Symbol lookup uses `search`                      | Routing        | `Symbol/declaration → search` | Shortened      |
| Never retry unchanged failed command             | Recovery       | Same concise rule             | Shortened      |
| Narrow duplicate FQNs by root                    | Disambiguation | Equivalent rule retained      | Reworded       |
| Explanation of why semantic queries save context | Non-behavioral | Removed                       | Deleted        |

Do not delete a Phase 5 rule unless it is classified as non-behavioral or fully subsumed by an explicitly equivalent compressed rule.

---

# Protected Routing Contract

At minimum, preserve the following explicitly:

```text
Find symbol       → search
Find callers      → callers
Find references   → references
Find callees      → callees
Find file symbols → symbols
Need AST structure → find
```

The mappings must remain direct and obvious.

Do not replace them with generic language such as:

```text
Use the most appropriate semantic command.
```

Do not rely on the agent to infer command selection from CLI names.

---

# Protected Recovery Behavior

Preserve the Phase 5 semantics governing failure and recovery.

At minimum, retain equivalent guidance for:

```text
Do not retry an unchanged failed command.

Use diagnostics or known information to make the cheapest useful correction.

Avoid ordinary --help discovery.

Do not repeatedly explore command syntax after a targeted failure.

Prefer one useful recovery action over trial-and-error command sequences.

Use textual fallback only when the semantic path remains unresolved under the documented fallback condition.
```

If Phase 5 contains more precise boundaries, preserve those exact semantics.

---

# Protected Semantic Boundaries

Pay particular attention to any Phase 5 wording involving:

```text
search vs find
references vs callers
references vs callees
FQN matching
source root
duplicate symbols
declaration/definition ambiguity
C++ semantic ambiguity
transitive vs direct relationships
```

These sections may appear verbose, but they are likely to contain decision boundaries.

Do not generalize several distinct cases into one rule unless the resulting rule produces the same action in every original case.

---

# Protected Cost Boundaries

Preserve instructions that keep the agent on a targeted path.

Examples include:

```text
Prefer targeted AST Tool queries when the task maps directly to them.

Do not dump an entire workspace when a scoped query is available.

Do not use --pretty by default.

Do not substitute grep/manual exploration for a direct semantic query.

Use grep for textual content or only under the documented semantic fallback condition.
```

Do not weaken these into suggestions such as:

```text
AST Tool can be useful.
```

The preference ordering must remain explicit.

---

# What May Be Compressed

Phase 7c may safely target the following categories.

## 1. Duplicate Restatements

If a Phase 5 rule is repeated in multiple sections, preserve it once.

Before deleting duplicates, confirm that no copy adds a distinct exception or condition.

---

## 2. Ordinary Examples

Remove examples that merely demonstrate syntax already defined by a rule.

Keep examples only when they define or clarify a behavioral boundary.

---

## 3. CLI Documentation

Remove exhaustive documentation that is available from the CLI and does not alter command selection.

Examples may include:

```text
complete output field descriptions
routine flag listings
ordinary command syntax repeated multiple times
```

However, retain syntax or flags when misunderstanding them is known to cause agent failures.

---

## 4. Rationale

Remove explanations of why a rule exists when the rule itself is already explicit.

For example:

```text
Rule
+
three sentences explaining why targeted output saves context
```

may become:

```text
Rule
```

if the explanation does not change application of the rule.

---

## 5. Verbose Wording

Shorten sentences while preserving all conditions.

For example:

```text
When a command fails, do not invoke the exact same command with the exact same arguments again because doing so will normally produce the same failure.
```

may become:

```text
Never retry an unchanged failed command.
```

This is valid because the decision boundary is unchanged.

---

# What Must Not Be Compressed Away

Do not remove text merely because:

```text
it looks repetitive
the command name seems self-explanatory
the CLI also has help
the model should be able to infer it
the behavior is obvious to a human
```

Phase 5 is the behavioral baseline.

If wording plausibly affects command selection or fallback probability, keep it unless an equivalent rule clearly replaces it.

---

# Compression Target

Do not optimize for the smallest possible Skill.

Phase 7c should use a moderate target.

Phase 5 is approximately:

```text
~3263 tokens
```

A reasonable first target is approximately:

```text
2200–2600 tokens
```

or roughly:

```text
20–30% reduction
```

This is a guideline, not a hard requirement.

If semantic preservation requires a larger Skill, keep it larger.

Do not force the Skill below the target.

A 15% reduction with strong semantic equivalence is preferable to a 50% reduction with uncertain behavior.

---

# Editing Strategy

Use a conservative transformation process.

## Step 1 — Restore Phase 5

Ensure the working Skill matches the accepted Phase 5 version before editing.

---

## Step 2 — Inventory Phase 5 Rules

Read the full Skill.

Identify:

```text
routing rules
recovery rules
fallback rules
scope/output rules
semantic ambiguity rules
command-specific boundaries
examples
documentation
rationale
```

---

## Step 3 — Classify Each Rule

Assign each meaningful item to:

```text
ROUTING CONTRACT
BEHAVIORAL DISAMBIGUATION
RECOVERY CONTRACT
OUTPUT / COST BOUNDARY
BEHAVIORAL EXAMPLE
NON-BEHAVIORAL EXPLANATION
```

Do this before deleting content.

---

## Step 4 — Compress Locally

Prefer local transformations:

```text
delete duplicate
merge equivalent wording
shorten sentence
remove non-behavioral explanation
replace repeated prose with table
```

Avoid rewriting the whole document from scratch.

---

## Step 5 — Build the Equivalence Map

For every removed or changed behavioral passage, record:

```text
original meaning
compressed representation
why they are equivalent
```

If equivalence is uncertain, restore the original rule.

---

## Step 6 — Review for Semantic Loss

Before evaluation, ask for every Phase 5 rule:

```text
Can the compressed Skill still answer:

What should the agent do?
When should it do it?
When should it not do it?
What should happen after failure?
What exception changes the decision?
```

If any answer is weaker or more ambiguous, revise the Skill before evaluation.

---

# Controlled Evaluation

Phase 7c requires a controlled evaluation before the normal 41-task agent-level evaluation.

The purpose is to remove Skill-invocation stochasticity from the body-compression test.

Select a representative subset of tasks covering at least:

```text
search
callers
references
callees
find
symbol ambiguity
failure recovery
grep fallback
```

Prefer approximately 15–20 tasks if the existing evaluation set provides enough coverage.

For the controlled comparison:

```text
Phase 5 Skill
vs
Phase 7c Skill
```

ensure `semantic-analysis` is loaded for both versions before task exploration begins.

Do not change the task itself.

The only controlled variable should be the Skill body.

---

# Controlled Comparison Metrics

For every paired task, record:

```text
success/failure
total tool calls
AST Tool calls
search
callers
references
callees
find
symbols
grep
glob
read
AST failures
AST retries
help calls
recovery distance
tokens
elapsed time
AST command sequence
```

Also compare the actual trajectory.

---

# Controlled Acceptance Criteria

The controlled cohort is the primary semantic-equivalence test.

Reject or revise Phase 7c if the compressed Skill systematically causes:

```text
direct semantic query
→ replaced by grep/manual lookup

search-first behavior
→ replaced by broad discovery

successful narrow recovery
→ replaced by help/trial-and-error

command-specific behavior
→ replaced by generic exploration
```

Exact command counts do not need to match.

Equivalent targeted semantic trajectories are acceptable.

Examples:

```text
Phase 5:
search → callers

Phase 7c:
search → search(refined) → callers
```

may be acceptable if the second search is justified.

But:

```text
Phase 5:
search → callers

Phase 7c:
grep → read → grep → read
```

is a likely regression.

---

# Rule-Level Causal Analysis

If a controlled trajectory changes materially, identify the exact compressed rule that could explain it.

Require evidence of the form:

```text
Phase 5 instruction
        ↓
compressed/removed wording
        ↓
changed agent decision
        ↓
changed trajectory
```

Do not infer causality from aggregate command counts alone.

---

# Full 41-Task Evaluation

Only after the controlled evaluation shows acceptable semantic preservation, run the normal unchanged 41-task evaluation.

This measures the full agent-level system, including stochastic Skill invocation.

Collect the same established metrics:

```text
success rate
total tool calls
AST Tool calls
AST failures
AST retries
help calls
grep
glob
read
bash
edit
recovery distance
elapsed time
tokens
per-command usage
Skill invocation
```

---

# Separate Two Questions

Phase 7c must report two different conclusions.

## Question A — Body Equivalence

```text
When both agents load the Skill,
does the compressed body preserve Phase 5 behavior?
```

Answer from the controlled cohort.

---

## Question B — Agent-Level Result

```text
Under normal agent behavior,
does the full system retain Phase 5 routing and efficiency?
```

Answer from the 41-task run.

Do not merge these two conclusions.

A Skill body may be semantically equivalent even if stochastic Skill invocation differs.

Likewise, a good aggregate run does not prove body equivalence.

---

# Token Evaluation

Do not optimize Phase 7c based solely on total reported tokens.

Report:

```text
Skill size reduction
fixed approximate Skill saving
total tokens
median per-test tokens
p75
p90
paired token deltas
```

For the controlled cohort, compare token deltas directly because Skill invocation is held constant.

This is more informative than aggregate totals alone.

---

# Final Deliverables

Provide:

1. The Phase 7c `SKILL.md`.
2. Phase 5 → Phase 7c textual diff summary.
3. Rule-classification summary.
4. Phase 5 → Phase 7c rule-equivalence map.
5. Skill size comparison:

   * lines
   * characters
   * bytes
   * approximate tokens
   * percentage reduction.
6. Controlled evaluation task list.
7. Controlled Phase 5 vs Phase 7c metrics.
8. Controlled trajectory comparison.
9. Any rule-level semantic regressions found.
10. Full 41-task evaluation metrics, if controlled evaluation passes.
11. Separate conclusions for:

    * body semantic equivalence
    * full agent-level behavior.
12. Final recommendation.

---

# Final Recommendation Values

Choose one:

```text
ACCEPT
ACCEPT WITH CAVEATS
REVISE
REJECT
```

## ACCEPT

Use when:

```text
meaningful size reduction
+
controlled Skill-loaded behavior preserved
+
no systematic routing/recovery regression
+
full evaluation acceptable
```

## ACCEPT WITH CAVEATS

Use when semantic preservation is strong but the normal full run contains clearly stochastic or measurement-related uncertainty.

## REVISE

Use when a small number of specific compressed rules weaken Phase 5 behavior and can be restored without abandoning the approach.

## REJECT

Use when compression removes decision boundaries broadly enough that Phase 5 behavior cannot be reproduced.

---

# Do Not Proceed Automatically

Do not proceed to Phase 8 or Phase 9 automatically.

Do not commit the Phase 7c candidate solely because it is smaller.

Do not replace Phase 5 as the stable baseline until both:

```text
controlled semantic equivalence
AND
acceptable full agent-level behavior
```

have been demonstrated.

---

# Final Standard

The success criterion is not:

```text
The compressed Skill contains the same major topics.
```

It is:

```text
For every behaviorally meaningful Phase 5 instruction,
the compressed Skill contains an equivalent decision rule,
and controlled agent trajectories demonstrate that those
rules preserve the intended semantic-routing behavior.
```

Phase 7c should optimize only the instruction volume that is genuinely non-essential to that behavior.
