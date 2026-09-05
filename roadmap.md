# AST Tool — Roadmap / Progress Summary

## 1. Optimization Goal

The optimization target is the **Coding Agent as a whole**, not AST Tool metrics in isolation.

Primary concerns remain:

```text
1. Correctness / success rate
2. Targeted semantic routing
3. Token / context efficiency
4. Recovery cost
5. Manual exploration
6. Latency / tool-call efficiency
```

A lower AST Tool call count is not inherently good.

The desired trajectory is:

```text
targeted semantic query
→ small relevant context
→ edit / solution
```

rather than:

```text
grep / glob
→ broad reads
→ manual reasoning
```

---

# 2. Current Roadmap

```text
P0  Baseline / Trace Metrics
    ✓ COMPLETE

P1  Skill.md Decision Tree
    ✓ ACCEPTED

P2  Output / JSON UX
    ✓ ACCEPTED

P3  Semantic Resolver
    ✗ DEFERRED

P4  Stable Semantic Symbol ID
    ✗ DEFERRED

P5  Error Recovery UX
    ✓ ACCEPTED
    │
    └─ Stable behavioral baseline

P6  Agent-facing Command Surface
    ✗ REJECTED

P7  Skill / Agent Guidance Optimization
    ├─ 7a Conservative Compression
    │   ✓ useful experiment
    │
    ├─ 7b Further Compression
    │   ✗ rejected as final candidate
    │
    ├─ 7c Semantic-Preserving Compression
    │   △ semantically preserved with caveats
    │
    ├─ 7c.1 Targeted Restoration
    │   ✓ CLOSED — no justified restoration
    │
    ├─ 7d Selective Backport onto Phase 5
    │   → current candidate
    │
    ├─ 7d.1 Repeated Guard Validation
    │   ✓ PASSED WITH CAVEAT
    │
    └─ 7d.2 Full 18-task Controlled Validation
        → CURRENT GATE

P8  Final Agent-Level Evaluation
    → after Phase 7d.2 gate

P9  Targeted Semantic Capability Research
    → only after final Skill behavior is established
```

---

# 3. Phase 5 — Stable Baseline

Phase 5 remains the formal stable baseline.

```text
tests                      41
successes                  37
success rate              90.24%

total tool calls           518
AST Tool calls              69
AST failures                 9
failure rate              13.04%
AST retries                  9
help calls                   2

grep                         15
glob                         12
read                        252
bash                        120
edit                         86

avg recovery distance      1.44
max recovery distance         2

total tokens            158,303
avg tokens/test           3,861

elapsed                 2224.27 sec
```

Phase 5 established the strongest combination so far of:

```text
semantic routing stability
+
low grep fallback
+
short recovery
+
acceptable correctness
```

Its weakness is that some semantic trajectories contain redundant AST work.

---

# 4. Phase 6 — Important Failed Experiment

Phase 6 changed the agent-facing command surface.

Although correctness remained around Phase 5 level, behavior changed substantially:

```text
AST semantic usage ↓↓↓
grep/manual exploration ↑↑↑
tokens ↑ substantially
recovery worsened
```

The key lesson was:

```text
correctness preserved
≠
optimization succeeded
```

Agent-facing presentation can alter routing even when the underlying commands remain unchanged.

Phase 6 was rejected.

---

# 5. Phase 7a / 7b — Compression Experiments

## Phase 7a

Conservative compression produced promising aggregate results and even one additional success in one run.

However, later analysis showed that a significant portion of the apparent differences came from stochastic Skill invocation and trajectory variation.

Phase 7a was useful primarily because it demonstrated that substantial instruction reduction was possible.

---

## Phase 7b

Further aggressive compression reduced Skill size dramatically.

However, the final Phase 8 comparison showed an undesirable agent-level pattern:

```text
AST semantic commands ↓
grep ↑
recovery worsened
reported tokens ↑
```

The compressed body itself could not be proven causal because Skill invocation collapsed independently.

Nevertheless, Phase 7b was rejected as the final candidate because the **observed complete system** did not preserve the required semantic-routing behavior.

Major lesson:

```text
A smaller Skill is not the goal.

Instruction salience and behavioral stability
matter more than instruction length.
```

---

# 6. Phase 7c — Semantic-Preserving Compression

Phase 7c restarted from Phase 5 and attempted to preserve every behaviorally meaningful rule rather than simply shorten the document.

Normal 41-task result:

```text
Phase 5                  Phase 7c

success     37/41        37/41
tools          518          542
AST calls       69           59
AST failures     9            7
grep             15           17
glob             12           22
read            252          286
bash            120          102
edit             86           82
tokens      158,303      178,116
elapsed     2224.27      2214.34
```

Phase 7c recovered most of the routing behavior lost in Phase 7b.

It also improved:

```text
AST failures
AST failure rate
bash usage
edit usage
```

but regressed:

```text
glob
read
total tools
tokens
maximum recovery
```

---

# 7. Controlled Phase 5 vs Phase 7c

Skill invocation was then forced in both versions to isolate the Skill body.

18-task controlled result:

```text
                    Phase 5    Phase 7c

success              17/18       17/18
tools                   162         173
AST calls                57          64
AST failures              5           8
retries                   4           8
help                      0           2
grep                      3           2
tokens               54,344      59,440
elapsed              912.59      970.03
```

The main conclusion was:

```text
Phase 7c preserves semantic capability,
but not strict trajectory equivalence.
```

There was no systematic semantic → grep/manual collapse.

However, Phase 7c showed weaker adherence to:

```text
search before under-qualified relationship queries
diagnostic-first recovery
restricted help usage
```

even though equivalent rules were still present.

This suggested that **wording salience**, not only semantic content, matters.

---

# 8. Important Positive Finding from Phase 7c

Phase 7c also exposed a genuine Phase 5 inefficiency.

The clearest case was `level3-007`.

Historical controlled trajectory:

```text
Phase 5:
search ×3
→ find ×6
```

versus:

```text
Phase 7c:
refined search only
```

The Phase 7c route could identify the required semantic targets without redundant structural lookup.

This produced evidence that:

```text
find is appropriate for AST structure/node detail,
but is sometimes redundant once refined search
has already identified the exact semantic target.
```

This became the basis for Phase 7d.

---

# 9. Phase 7c.1 — Targeted Restoration

Phase 7c.1 investigated whether the Phase 7c regressions could be fixed by restoring selected Phase 5 wording.

Result:

```text
REVERT TO PHASE 7C / NO CHANGE
```

No restoration met the required causal standard.

The main findings were:

```text
- Most glob/read regressions happened when the Skill was not loaded.
- The same-loaded find → Glob substitution was actually cheaper.
- The long recovery case occurred without the Skill.
- Phase 7c already contained equivalent routing/recovery guidance.
```

Therefore adding more wording would have been instruction bloat without demonstrated benefit.

This closed the general compression/restoration direction.

---

# 10. Strategic Pivot — Phase 7d

The strategy changed from:

```text
make Phase 5 smaller
```

to:

```text
keep Phase 5 where Phase 5 wins
+
backport only improvements that Phase 7c demonstrated
```

Phase 7d therefore started from the **exact Phase 5 Skill**.

Only one behavioral rule was added:

```text
If a refined `search` already identifies the exact symbol or member needed,
do not add a redundant `find` solely to locate it; use `find` when AST
structure or node detail is required.
```

Phase 7d is therefore:

```text
Phase 5
+
one evidence-backed micro-optimization
```

rather than a compressed rewrite.

---

# 11. Initial Phase 7d Targeted Test

The new rule reproduced the intended effect.

In `level3-007`, Phase 7d repeatedly eliminated the redundant six-`find` path while preserving correctness and targeted semantic search.

Other structural cases such as `level3-003` also successfully used refined `search`.

However, the first targeted guard runs showed more AST failures than the single Phase 5 reference run.

Because those failures involved:

```text
relationship ordering
references ambiguity
smoke-001 recovery
```

rather than the new search/find rule, causality was unclear.

The initial Phase 7d decision was therefore:

```text
REVISE / NEED VARIANCE CHARACTERIZATION
```

rather than rejection.

---

# 12. Phase 7d.1 — Repeated Controlled Guard Validation

Phase 5 and Phase 7d were each repeatedly tested on:

```text
level2-006
level2-008
level3-008
smoke-001
```

with `level3-007` as a positive control.

Three fresh repeats were run per task/version.

Primary guard aggregate:

```text
                    Phase 5    Phase 7d

runs                  12           12
successes             12           12

tools                 110          110
AST calls              39           36
AST failures           12           12
retries                11           10
help                    0            0

tokens             44,184       41,245
```

The originally observed guard regression disappeared.

The conclusion was:

```text
MINOR STOCHASTIC DIFFERENCE
```

not:

```text
SYSTEMATIC GUARD REGRESSION
```

Important observations:

### `level2-006`

```text
search → callers
```

in every Phase 5 and Phase 7d run.

### `level2-008`

Relationship-first:

```text
Phase 5:  1/3
Phase 7d: 2/3
```

Slight directional difference, but both versions exhibited both trajectories.

### `level3-008`

Relationship-first:

```text
Phase 5:  1/3
Phase 7d: 1/3
```

One Phase 7d run produced recovery distance 5, but it was an isolated outlier.

### `smoke-001`

All six runs had short recovery.

Phase 7d actually had fewer mean failures and retries than Phase 5.

Therefore the previous Phase 7d recovery regression was not reproducible.

---

# 13. Positive-Control Result

`level3-007` continued to directionally favor Phase 7d:

```text
mean AST calls
Phase 5:  5.67
Phase 7d: 5.00

mean tools
Phase 5:  13.33
Phase 7d: 12.33

mean tokens
Phase 5:  6,872
Phase 7d: 6,423
```

However, Phase 5 itself spontaneously avoided `find` in 2/3 new runs.

Therefore the current evidence is:

```text
Phase 7d creates a useful efficiency bias
```

rather than:

```text
Phase 7d deterministically changes the trajectory
```

This is enough to continue testing because no corresponding systematic regression has been observed.

---

# 14. Current Phase 7d Status

Current interpretation:

```text
Phase 7d concept
    SUPPORTED

Intended refined-search optimization
    DIRECTIONALLY REPRODUCIBLE

Systematic guard regression
    NOT OBSERVED

Correctness risk
    NOT OBSERVED

Ready for full controlled validation
    YES
```

The key methodological lesson is that **within-version stochastic variation is often as large as the difference between Skill versions**.

Therefore single-run aggregate differences should no longer be used to justify Skill changes.

---

# 15. Phase 7d.2 — Current Gate

Phase 7d.2 is the full controlled 18-task comparison:

```text
Phase 5 Skill
vs
Phase 7d Skill
```

using the current repository and current AST Tool implementation, with everything else held constant.

The controlled cohort contains 18 representative tasks covering search, find, callers, references, callees, C++ ambiguity, relationship ordering, recovery, help behavior, grep fallback, and structural lookup.

Skill invocation is forced:

```text
semantic-analysis
= exactly once
= first tool action
```

for both arms, eliminating the major invocation confound observed in earlier phases.

Phase 7d remains exactly:

```text
Phase 5
+
one narrow refined-search rule
```

No other Skill modification is permitted during the experiment.

---

# 16. Phase 7d.2 Acceptance Gate

The controlled evaluation should answer:

```text
1. Does Phase 7d preserve Phase 5 correctness?

2. Does semantic routing remain targeted?

3. Is recovery approximately Phase 5-like?

4. Does the refined-search rule provide
   a credible efficiency benefit?
```

The possible decisions are:

```text
ACCEPT PHASE 7D
ACCEPT WITH CAVEATS
REVISE
REVERT TO PHASE 5
```

A favorable controlled result requires Phase 5-level correctness, targeted routing, approximately Phase 5-level recovery, and at least one meaningful efficiency benefit without systematic regression.

---

# 17. Important Current Limitation

The supplied Phase 7d.2 material contains the **evaluation protocol**, but not the resulting 18-task measurements or final decision.

Therefore the current state should be recorded as:

```text
Phase 7d.2
FULL CONTROLLED VALIDATION
→ READY / IN PROGRESS / RESULT NOT YET RECORDED HERE
```

rather than marking it accepted or rejected.

The protocol states that the normal 41-task evaluation should run only if Phase 7d.2 concludes either:

```text
ACCEPT PHASE 7D
```

or:

```text
ACCEPT WITH CAVEATS
```

---

# 18. Next Decision Tree

The next discussion should begin here:

```text
Phase 7d.2 controlled 18-task result
                │
        ┌───────┴────────┐
        │                │
    favorable        unfavorable
        │                │
        ↓                ↓
normal 41-task       investigate the
Phase 7d run         specific reproducible
        │            regression
        ↓
Phase 5 vs Phase 7d
agent-level comparison
        │
        ↓
final stable candidate
```

If Phase 7d.2 is favorable:

```text
Phase 7d
→ normal 41-task evaluation
→ determine whether it replaces Phase 5
```

If unfavorable:

```text
do not add generic Skill wording

identify:
specific task
→ repeated trajectory difference
→ causal relation to added rule
```

Only then consider revising the rule.

---

# 19. Phase 9 — Current Intended Role

Phase 9 should not be another Skill wording phase.

Once Phase 5 vs Phase 7d is resolved, Phase 9 should investigate **semantic inefficiencies shared by both Skills**.

Current candidate patterns include:

```text
1. Under-qualified relationship target
   → search refinement
   → relationship retry

2. Same-FQN C++ declaration/definition ambiguity

3. search finds declaration
   → second query/read needed for definition

4. find returns useful structural information
   → Read still required for implementation context
```

These are candidates because they may indicate an AST Tool capability limitation rather than an instruction problem.

Phase 7d.2 explicitly records these shared patterns for later Phase 9 research rather than changing them now.

Phase 9 should first determine whether:

```text
existing command semantics can be improved
```

before considering:

```text
new subcommands
```

A new command should be justified only by repeated trajectories showing that the existing command set cannot express the needed operation cleanly.

---

# 20. Current Working Hypothesis

The accumulated evidence now supports:

```text
Broad Skill compression
    → too difficult to control behaviorally

Phase 5 stable wording
+
small evidence-backed changes
    → much easier to validate causally
```

Therefore the preferred development strategy is now:

```text
stable baseline
→ identify one reproducible inefficiency
→ make one narrow change
→ targeted replay
→ repeated guard validation
→ full controlled cohort
→ normal agent-level evaluation
```

This methodology is itself one of the main outcomes of Phase 7.

---

# 21. Current Baseline / Candidate

For the next discussion, keep these roles explicit:

```text
Stable baseline:
    Phase 5

Candidate:
    Phase 7d

Phase 7d contents:
    exact Phase 5 Skill
    +
    one refined-search / redundant-find rule

Phase 7d.1:
    passed guard validation with minor stochastic differences

Current gate:
    Phase 7d.2 18-task controlled validation
```

Do not promote Phase 7d to stable baseline until Phase 7d.2 and, if it passes, the normal 41-task evaluation are complete.

---

# 22. Next Discussion Starting Point

Start the next discussion with:

```text
1. What is the Phase 7d.2 18-task controlled result?

2. Did Phase 7d preserve:
   - correctness
   - semantic routing
   - recovery stability?

3. Did it produce a credible efficiency improvement
   beyond ordinary stochastic variance?

4. If yes:
   run/evaluate the normal 41-task Phase 7d result.

5. If Phase 7d becomes the stable baseline:
   use shared Phase 5/7d inefficiencies
   to define Phase 9 research targets.
```

The immediate question is no longer:

```text
How can SKILL.md be compressed further?
```

It is:

```text
Can the one evidence-backed Phase 7d micro-optimization
be promoted safely on top of Phase 5?
```

After that question is resolved, the optimization effort should move away from Skill rewriting and toward evidence-backed Phase 9 semantic-capability research.
