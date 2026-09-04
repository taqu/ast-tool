# Phase 7b — Further Skill.md Compression

## Objective

Further compress `Skill.md` beyond the accepted Phase 7a version while preserving the routing and recovery behavior established by Phase 5 and retained in Phase 7a.

Phase 7a demonstrated that a substantial reduction in instruction size is possible without evidence of a systematic regression among trajectories that actually loaded the skill.

Phase 7b should therefore explore **additional compression**, but must remain behavior-preserving.

The objective is:

```text
Phase 7a behavior preserved
+
Skill.md smaller
+
No measurable degradation in skill-guided trajectories
```

This phase is still an instruction-design optimization.

Do not modify AST Tool itself.

---

# Starting Point

Use the committed Phase 7a version of `Skill.md` as the baseline.

Do not restart from Phase 5.

Phase 7a is now the accepted behavioral and textual baseline for this phase.

The Phase 7a evaluation result was:

```text
tests                      41
successes                  38
failures                    3
success rate              92.68%

total tool calls           545
AST Tool calls              60
AST failures                 9
AST failure rate          15.00%
AST retries                 10

grep                         31
glob                         19
read                        275

avg recovery distance      2.56
max recovery distance         6

total tokens            156,439
avg tokens/test         3,815.6
```

Phase 7a reduced the Skill from approximately:

```text
~3263 tokens
→
~1546 tokens
```

The Phase 7a trajectory analysis found no convincing evidence that the compressed Skill body caused routing regression when the Skill was actually loaded.

That result allows Phase 7b to attempt further compression.

---

# Important Lesson from Phase 7a

Do not evaluate Phase 7b using total tokens alone.

Phase 7a showed that instruction compression can provide a large fixed token saving while unrelated or stochastic manual exploration consumes some of that saving.

Therefore distinguish:

```text
instruction-size efficiency
```

from:

```text
agent-trajectory efficiency
```

A smaller Skill.md is not automatically better if it causes:

```text
AST Tool usage ↓
grep/read exploration ↑
recovery complexity ↑
```

among tests that consumed the Skill.

---

# Protected Routing Contract

The following routing contract must remain obvious:

```text
Find symbol       → search
Find callers      → callers
Find references   → references
Find callees      → callees
Find file symbols → symbols
Need AST structure → find
```

These mappings may be expressed more compactly, but their meaning must not become weaker or ambiguous.

Do not replace this explicit routing with prose such as:

```text
Use the appropriate semantic command when useful.
```

The agent should not need to infer which command corresponds to each task.

---

# Protected Behavioral Contract

Preserve the semantics of the current Phase 7a rules governing:

```text
command selection
targeted semantic queries
fallback behavior
failed-command retry
help usage
large-output avoidance
grep/manual fallback
failure recovery
```

In particular, retain the equivalent of:

```text
Do not retry an unchanged failed command.

Do not use --help for ordinary discovery.

Do not use --pretty by default.

Do not dump broad workspace output when a targeted query is available.

Prefer AST Tool semantic/structural queries over grep/manual exploration
when the requested operation directly maps to an AST Tool command.
```

The exact wording may change.

The behavioral meaning must not.

---

# Phase 7b Compression Scope

Phase 7b may be more aggressive than Phase 7a in the following areas.

## 1. Merge closely related rules

If multiple rules express one decision boundary, merge them.

For example, separate sections for:

```text
command choice
when to use the command
examples of using the command
```

may be reducible to a compact routing table plus only the exceptions that matter.

---

## 2. Remove examples that no longer disambiguate behavior

Phase 7a retained examples conservatively.

Re-evaluate them.

An example may be removed if:

```text
the rule is already unambiguous
AND
the example introduces no additional routing or recovery information
```

Prefer zero or one example per important exceptional case.

Do not keep examples merely for completeness.

---

## 3. Compress repeated recovery guidance

If `search`, `callers`, `references`, and `callees` share the same recovery principle, express the common rule once rather than repeating it for every command.

For example:

```text
If a targeted semantic query fails,
use the error's cheapest suggested correction or refine the symbol,
rather than retrying the same command unchanged.
```

Then retain command-specific recovery text only where behavior genuinely differs.

---

## 4. Remove documentation-like material

`Skill.md` is agent guidance, not full CLI documentation.

Remove content whose primary purpose is to document commands rather than affect agent decisions.

Candidates include:

```text
exhaustive flag lists
obvious syntax explanations
descriptions repeated by CLI help
implementation details
background rationale
examples covering ordinary syntax
```

Keep only what changes agent behavior.

---

## 5. Convert prose into compact decision rules

Prefer:

```text
Need callers → callers
Need references → references
```

over several sentences explaining why each command exists.

Prefer:

```text
Failed unchanged command → do not retry
```

over explanatory prose unless the explanation prevents a known mistake.

---

## 6. Eliminate duplicate warnings

If the same warning appears in:

```text
command guidance
common mistakes
best practices
fallback section
```

keep it once in the location where it most strongly affects the decision.

---

# What Phase 7b Must Not Do

Do not:

* change command names;
* change CLI syntax;
* change AST Tool implementation;
* change semantic resolution;
* modify Phase 5 error UX;
* redesign JSON output;
* modify evaluation tasks;
* change Skill trigger metadata unless explicitly required by the task;
* add new routing behavior;
* broaden grep fallback;
* add speculative workarounds;
* optimize for fewer AST calls;
* optimize individual AST metrics independently.

Do not intentionally change Skill invocation behavior.

Phase 7b concerns the Skill body.

---

# Special Attention: Skill Invocation

Phase 7a showed substantial run-to-run variation in whether some tests loaded the Skill at all.

Do not incorrectly attribute a missing Skill invocation to compressed body text.

For any Phase 7b regression, distinguish:

```text
Skill not invoked
```

from:

```text
Skill invoked, but compressed guidance changed behavior
```

Only the second is direct evidence against the Phase 7b compression.

Record Skill invocation per test during evaluation.

---

# Recommended Editing Process

Use a conservative iterative approach.

## Step 1 — Inspect Phase 7a

Identify every remaining section of Phase 7a `Skill.md`.

Classify each piece as:

```text
ROUTING CONTRACT
RECOVERY CONTRACT
IMPORTANT EXCEPTION
REDUNDANT EXPLANATION
DOCUMENTATION
EXAMPLE
RATIONALE
```

The first three categories are protected.

The remaining categories are primary compression candidates.

---

## Step 2 — Produce a proposed compressed version

Do not rewrite semantics from memory.

Compress the existing Phase 7a text directly.

Prefer structural merging and deletion over semantic rewriting.

---

## Step 3 — Diff the behavioral contract

Before evaluation, compare Phase 7a and Phase 7b.

For each removed or merged section, answer:

```text
What agent decision did this text influence?
Is that decision still represented?
Could its removal make grep/manual exploration more attractive?
Could its removal make command choice ambiguous?
Could its removal weaken recovery guidance?
```

If uncertain, restore the smallest necessary wording.

---

## Step 4 — Measure Skill size

Report before/after:

```text
lines
characters or bytes
approximate tokens
percentage reduction
```

Use Phase 7a as the size baseline.

---

## Step 5 — Run the same 41-test evaluation

Do not change the test set.

Collect the same metrics used previously.

---

# Evaluation Metrics

Collect at least:

```text
tests
successes
failures
success rate

total tool calls
average tool calls/test

AST Tool calls
AST Tool failures
AST Tool failure rate
AST retries
AST help calls

grep
glob
read
bash
edit

elapsed time

input tokens
output tokens
total tokens
average tokens/test

AST Tool command usage
failures by command
recovery distances

Skill invocation count
Skill invocation per test
```

---

# Additional Phase 7b Analysis

In addition to aggregate metrics, compute metrics separately for:

```text
A. tests that loaded the Skill in both Phase 7a and Phase 7b

B. tests that did not load the Skill in one or both runs
```

Group A is the primary evidence for whether compression changed agent behavior.

Do not let Group B dominate the conclusion about the Skill body.

---

# Same-Skill-Invocation Comparison

For tests that loaded the Skill in both runs, compare:

```text
success/failure
AST Tool calls
search
callers
references
callees
find
grep
glob
read
recovery distance
total tool calls
tokens
```

Look specifically for transitions such as:

```text
Phase 7a:
search → callers → Read

Phase 7b:
grep → Read → Grep → Read
```

or:

```text
Phase 7a:
find → targeted result

Phase 7b:
manual file inspection
```

These would be stronger evidence of a compression-induced regression than aggregate AST-call changes.

---

# Token Decomposition

Estimate the fixed instruction saving separately from trajectory cost.

Let:

```text
S7a = Phase 7a Skill token count
S7b = Phase 7b Skill token count
N   = number of comparable Skill invocations
```

Estimate:

```text
fixed compression saving
≈
(S7a - S7b) × N
```

Then compare this with the observed token delta.

Use this to estimate whether trajectory behavior became:

```text
more efficient
neutral
or less efficient
```

after accounting for the smaller Skill.

Exact accounting may not be possible because prompt/cache/tool-result tokens may not all be exposed.

Clearly label any estimate.

---

# Acceptance Criteria

Phase 7b should not require exact equality with Phase 7a because agent trajectories are stochastic.

Use the following hierarchy.

## 1. Correctness

Success rate must remain approximately Phase 7a / Phase 5 level.

A one-test fluctuation should be investigated rather than automatically accepted or rejected.

A broad correctness decline is a rejection signal.

---

## 2. Skill-loaded routing behavior

Among tests that loaded the Skill in both runs:

```text
search/callers/references/callees/find routing
```

must remain targeted.

Reject if compression systematically replaces targeted AST queries with:

```text
grep
glob
broad reads
manual exploration
```

---

## 3. Recovery behavior

Do not require identical recovery-distance aggregates.

Instead inspect new long-recovery trajectories.

A regression matters when:

```text
Skill was loaded
AND
Phase 7b guidance plausibly caused the longer recovery
```

---

## 4. Token efficiency

Phase 7b should provide a meaningful additional Skill-size reduction.

Total tokens should ideally improve.

However, evaluate total-token changes together with normalized trajectory behavior.

Do not accept a large trajectory regression merely because fixed prompt savings hide it.

---

## 5. Manual exploration

A large increase in grep/read/glob among comparable Skill-loaded tests is suspicious.

Localized outliers or tests without Skill invocation should be analyzed separately.

---

# Suggested Decision Threshold

Phase 7b should produce one of:

```text
ACCEPT
ACCEPT WITH MINOR RESTORATION
REVISE
REJECT
```

### ACCEPT

Use when:

```text
Skill is materially smaller
AND
correctness is preserved
AND
skill-loaded routing is effectively preserved
AND
no systematic recovery/manual-exploration regression appears
```

### ACCEPT WITH MINOR RESTORATION

Use when one narrowly identifiable removed instruction causes a localized regression and restoring a very small rule fixes the ambiguity.

### REVISE

Use when several compressed sections appear too aggressive but the overall approach remains viable.

### REJECT

Use when further compression causes broad routing degradation, increased manual exploration, or correctness loss among Skill-loaded trajectories.

---

# Do Not Optimize Toward Command Counts

Do not attempt to preserve:

```text
search = exactly 25
find = exactly 6
AST calls = exactly 60
```

Those are observations, not targets.

The actual target is:

```text
cheap targeted trajectory
```

For example:

```text
search → callers → edit
```

and:

```text
search → find → edit
```

may both be good depending on the task.

Similarly, fewer AST calls can be an improvement when redundant queries disappear.

Judge trajectory quality, not raw command count.

---

# Deliverables

Provide:

1. Modified Phase 7b `Skill.md`.
2. Phase 7a → Phase 7b textual diff summary.
3. Skill size comparison:

   * lines
   * characters/bytes
   * approximate tokens
   * percentage reduction.
4. Full 41-test evaluation results.
5. Phase 7a vs Phase 7b aggregate comparison.
6. Per-command AST Tool comparison.
7. Skill invocation comparison.
8. Same-Skill-invocation trajectory comparison.
9. Outlier analysis.
10. Token decomposition estimate.
11. Any suspected causal link between removed guidance and trajectory changes.
12. Final recommendation:

    * `ACCEPT`
    * `ACCEPT WITH MINOR RESTORATION`
    * `REVISE`
    * `REJECT`

Do not modify the accepted Phase 7a commit if Phase 7b fails.

Do not proceed to Phase 8 automatically.

---

# Guiding Principle

Use this principle throughout Phase 7b:

```text
Compress documentation,
not decisions.
```

And evaluate success at the agent level:

```text
smaller instructions
+
preserved targeted routing
+
preserved correctness
+
no hidden trajectory cost
```

Phase 7b succeeds only when additional compression is genuinely cheaper for the Coding Agent as a whole.