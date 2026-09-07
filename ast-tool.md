# Phase 10 — Semantic Routing Opportunity / Trigger Reliability

## Objective

Phase 10 evaluates and improves **semantic routing reliability**.

The current semantic capability is already the stable baseline. Do not redesign or broaden the semantic engine unless the experiment reveals a concrete blocker.

The goal of this phase is **not** to maximize `semantic-analysis` invocation rate.

The goal is:

```text
increase appropriate semantic routing
where semantic routing has demonstrated agent-level value
without materially increasing low-value invocation
```

Treat routing as a classification problem:

```text
                     Semantic route useful?
                     Yes          No

Invoked              TP           FP
Not invoked          FN           TN
```

The primary optimization target is:

```text
reduce FN
without materially increasing FP
```

In other words, reduce missed high-value semantic opportunities without making the agent invoke semantic tooling indiscriminately.

---

## Stable Baseline

Keep the current accepted implementation unchanged unless a Phase 10 experiment explicitly requires a routing-related modification.

The semantic baseline is:

```text
Phase 7d Skill
+
Phase 8a
    unique FQN suffix resolution
+
Phase 8b
    receiver-type member relationship resolution
+
Phase 8c
    callable declaration/definition body identity
```

Do not revert any Phase 8 capability.

Do not use pre-Phase-8 behavior as the implementation baseline.

---

## Core Hypothesis

Phase 9 established that semantic routing can produce clear agent-level value when the task requires relationship discovery.

The strongest positive evidence comes from Phase-8b-like tasks such as:

```text
who calls X?
where is X referenced?
which callers need modification?
cross-file member relationship discovery
member-method relationship discovery
```

For these tasks, the desired trajectory is approximately:

```text
semantic routing
→ targeted relationship query
→ small relevant context
→ edit / solution
```

rather than:

```text
Grep / Glob
→ repeated Read
→ manual reconstruction
→ edit
```

Phase 10 should determine why the agent sometimes fails to select the semantic route for these opportunities and whether a small routing change can improve that decision.

---

## Important Constraint

Do not optimize for raw invocation frequency.

The following outcome is not sufficient:

```text
semantic-analysis invocation rate increased
```

A change is useful only if the additional invocations occur primarily on tasks where semantic routing provides demonstrated value.

A change that increases semantic invocation while also increasing unnecessary AST calls, tokens, latency, retries, or manual recovery should not be accepted merely because routing frequency improved.

---

# Phase 10a — Opportunity Set Construction

Before changing any routing behavior, build an evaluation corpus that separates positive semantic opportunities from negative or neutral cases.

## Positive Set

Create a focused set of tasks where semantic relationship queries are expected to have concrete value.

Prioritize Phase-8b-like cases.

Examples:

```text
Find every caller of a member method and modify those callers.

Determine where a class member function is referenced.

Find which call sites of a specific member need an update.

Trace a member-method relationship across multiple files.

Locate callers of methods accessed through:
- object fields
- pointer fields
- local objects
- local pointers
- reference parameters
- pointer parameters
```

Prefer tasks where manual exploration would otherwise require multiple Grep/Read operations.

Include cases with:

```text
same member name on unrelated classes
qualified and partially qualified names
multiple source files
declaration / implementation separation
```

The positive set should contain opportunities where the current semantic baseline is known to return useful and precise results.

Do not include unsupported semantic cases merely to increase difficulty.

---

## Negative / Control Set

Create a control set where semantic routing is unnecessary, low-value, or clearly inferior to simpler inspection.

Examples may include:

```text
small local edits
single-file literal changes
obvious nearby code modifications
formatting or mechanical edits
tasks where the relevant code is already directly visible
tasks where no relationship discovery is required
```

The control set is required to detect over-triggering.

A routing change that improves the positive set but causes semantic-analysis to fire broadly on these tasks may be a regression.

---

## Dataset Requirements

The dataset must allow each run to be classified independently.

For every task, record at minimum:

```text
task id
expected opportunity class:
    positive
    negative/control

semantic-analysis invoked?
first agent action
first discovery action
AST commands used
manual discovery tools used
success/failure
tool count
AST call count
AST failures
AST retries
Read count
Grep count
Glob count
token count
elapsed time
```

Where possible, also record whether the semantic result directly contributed to the final edit.

---

# Phase 10b — Baseline Routing Characterization

Run the opportunity corpus using the unchanged stable baseline.

Do not add routing hints yet.

The purpose is to measure current routing behavior.

For every run, classify the outcome as:

```text
TP
semantic route was valuable and was selected

FN
semantic route would have been valuable but was not selected

FP
semantic route was selected but provided little or no value

TN
semantic route was not selected and was not needed
```

For positive tasks, inspect the first-decision trajectory.

Especially distinguish:

```text
semantic-analysis first
direct ast-tool first
Grep first
Glob first
Read first
other
```

The main question is:

```text
Where does the trajectory diverge before semantic-analysis gets a chance to help?
```

Do not assume all non-invocation is caused by the same trigger failure.

---

# Phase 10c — First-Decision Analysis

Analyze false negatives individually.

For each FN, determine whether the missed semantic route appears related to:

```text
Skill description
trigger wording
tool / Skill discovery metadata
task phrasing
agent interpretation of relationship intent
competition with Grep / Read
direct ast-tool use without Skill invocation
other first-decision behavior
```

Pay particular attention to the observation that `semantic-analysis`, when invoked, tends to be invoked as the first action.

Therefore, treat this primarily as a **first-decision routing problem**, not a late-recovery problem.

Do not attempt to fix missed invocation by adding instructions deep inside the Skill body unless evidence shows the Skill has already been loaded before the decision failure.

---

# Phase 10d — Minimal Trigger Experiments

After the baseline classification is complete, test routing interventions one variable at a time.

Candidate variables include:

```text
1. Skill description
2. trigger phrasing
3. discovery metadata
4. system-level routing hint
```

Do not change multiple routing variables in the same initial experiment.

For each candidate:

```text
baseline
vs.
one-variable modification
```

Keep the following constant:

```text
repository
task prompt
semantic implementation
Skill body unless it is the tested variable
agent configuration
tool availability
evaluation procedure
```

Use repeated runs where routing is stochastic.

---

## Preferred Intervention Style

Prefer small, explicit trigger cues describing **when semantic relationship discovery is valuable**.

Good routing guidance should emphasize task shape rather than blanket tool preference.

Conceptually, guidance should resemble:

```text
Use semantic analysis when the task requires discovering callers,
references, callees, or cross-file symbol/member relationships.
```

Avoid guidance equivalent to:

```text
Always use semantic-analysis for code tasks.
```

or:

```text
Try semantic-analysis before Grep.
```

unless evidence strongly justifies such behavior.

The intervention should improve discrimination, not simply bias the agent toward another tool.

---

# Evaluation Metrics

Evaluate routing quality and agent performance together.

## Primary Metrics

```text
positive-set semantic routing recall
    TP / (TP + FN)

false-positive routing rate
    FP / (FP + TN)

correctness
```

The primary routing objective is:

```text
positive-set recall ↑
while FP remains stable or increases only negligibly
```

---

## Secondary Metrics

Track:

```text
semantic precision
tool count
AST call count
AST failures
AST retries
manual fallback
Read count
Grep count
Glob count
tokens
elapsed time
recovery cost
```

The intended improvement pattern is:

```text
FN ↓
manual fallback ↓
Read / Grep exploration ↓
tokens ↓ or stable
elapsed ↓ or stable
correctness stable
FP approximately stable
```

Not every metric must improve on every task, but broad cost regressions must not be hidden by a higher invocation rate.

---

# Semantic Precision Check

When semantic routing occurs, verify that the semantic result itself is useful.

Do not count an invocation as a TP merely because the Skill or AST command was called.

A useful semantic route should satisfy most of the following:

```text
returned the relevant relationship
avoided false-positive cross-linking
reduced manual discovery
contributed directly to the solution
did not require unnecessary recovery
```

If the agent invokes semantic-analysis but ignores or cannot use the result, classify and report that separately.

---

# Repeated Evaluation

Routing has previously shown stochastic behavior.

For tasks near the decision boundary, use repeated runs rather than relying on one trajectory.

Prefer repeated evaluation for:

```text
tasks that alternate between semantic and manual routing
tasks affected by a proposed trigger change
tasks responsible for apparent aggregate improvements
tasks that produce unexpected FP or FN behavior
```

Report both aggregate results and per-task distributions.

Do not let one unusually cheap or expensive run dominate the conclusion.

---

# Acceptance Criteria

A Phase 10 routing change should be accepted only if the evidence shows that it improves **appropriate semantic selection**.

A good result should demonstrate:

```text
1. meaningful reduction in FN on the positive set

2. no material correctness regression

3. no material increase in FP on the control set

4. semantic results remain precise

5. no broad increase in AST failure/retry behavior

6. agent-level cost is improved or at least reasonably neutral
   on the tasks where additional routing occurs
```

Prefer evidence from repeated task-level behavior over a single aggregate invocation percentage.

---

# Rejection Conditions

Reject or revert a routing change if it mainly produces any of the following:

```text
semantic invocation increases everywhere

positive routing recall improves only slightly
but control-set invocation rises substantially

AST calls increase without reducing manual discovery

tokens or elapsed time increase broadly

semantic-analysis is invoked on trivial local tasks

new routing causes repeated redundant semantic queries

correctness regresses

the apparent gain depends on one or two stochastic runs
```

Also reject changes whose only demonstrated benefit is:

```text
higher semantic-analysis invocation rate
```

Raw invocation rate is diagnostic, not the optimization target.

---

# Out of Scope

Do not expand Phase 10 into general semantic capability development.

The following are not current priorities unless repeated Phase 10 evidence shows that they block valuable routing:

```text
additional receiver forms
auto / decltype inference
templates
inheritance / virtual dispatch
overload resolution
complex receiver expressions
explicit this-> support
new semantic commands
stable semantic symbol IDs
```

Also do not prioritize the known callee-side residual gap merely because it exists.

A real semantic limitation is not automatically the next optimization target.

The criterion is agent-level exposure and demonstrated cost.

---

# Known Low-Priority Issues

Keep the following documented, but do not let them distract from the routing experiment unless they materially affect the results:

```text
callee-side declaration/definition residual gap

Windows path quoting issues

sporadic --help overuse
```

If one of these invalidates a run, classify the run separately rather than treating it as evidence for or against the routing hypothesis.

---

# Implementation Discipline

Follow these rules during Phase 10:

```text
measure before modifying

change one routing variable at a time

preserve the Phase 8 semantic baseline

do not optimize raw invocation rate

separate positive opportunities from controls

inspect first-decision behavior

repeat stochastic cases

prefer small reversible changes

require agent-level evidence
```

Do not make speculative cleanup changes during the experiment.

Do not combine unrelated improvements into a routing candidate.

Keep every tested change easy to revert and compare.

---

# Expected Deliverables

Produce the following artifacts.

## 1. Opportunity Corpus

Document:

```text
positive tasks
negative/control tasks
why each task belongs to its class
```

---

## 2. Baseline Routing Report

Include:

```text
TP
FP
FN
TN

positive-set routing recall
control-set false-positive rate

first-action distribution
semantic-analysis invocation rate
direct ast-tool usage
manual routing usage
```

---

## 3. False-Negative Analysis

For each important FN, summarize:

```text
task
first action
actual trajectory
expected semantic opportunity
likely routing decision cause
manual fallback cost
```

---

## 4. Candidate Experiment Results

For every routing intervention:

```text
exact change
hypothesis
task cohort
number of runs
TP / FP / FN / TN
correctness
tool metrics
token metrics
elapsed metrics
manual fallback
observed regressions
```

---

## 5. Final Recommendation

End Phase 10 with one of:

```text
ACCEPT

ACCEPT WITH CAVEATS

REJECT

NO SAFE ROUTING IMPROVEMENT FOUND
```

The conclusion must be evidence-based.

If no candidate reliably reduces false negatives without increasing low-value semantic invocation, keep the existing routing behavior and report that result rather than forcing a change.

---

# Final Principle

The semantic engine is no longer the main question.

Phase 10 asks:

```text
Can the agent recognize the tasks where the existing semantic capability
has already proven valuable?
```

The desired trajectory is:

```text
valuable semantic opportunity
→ semantic route selected
→ targeted query
→ small relevant context
→ correct edit
```

The objective is not more semantic tooling.

The objective is **better routing decisions**.
