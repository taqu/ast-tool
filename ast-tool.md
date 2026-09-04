# Phase 7c.1 — Targeted Trajectory Restoration

## Objective

Improve the current Phase 7c candidate by restoring only the Phase 5 guidance that can be shown to prevent measurable trajectory regressions.

Do **not** restart from Phase 5.

Use the current Phase 7c `semantic-analysis/SKILL.md` as the working baseline.

The goal is:

```text
Keep Phase 7c improvements
+
restore only behaviorally necessary Phase 5 guidance
+
reduce structural/manual exploration cost
```

Phase 7c.1 is not another general compression phase.

It is a **targeted behavioral restoration phase**.

---

# Background

Phase 7c preserved correctness and recovered most of the semantic-routing behavior lost in Phase 7b.

Phase 5:

```text
tests                      41
successes                  37
success rate              90.24%

total tool calls           518
AST Tool calls              69
AST failures                 9
AST failure rate          13.04%

grep                         15
glob                         12
read                        252

avg recovery distance      1.44
max recovery distance         2

total tokens            158,303
avg tokens/test           3,861
```

Phase 7c:

```text
tests                      41
successes                  37
success rate              90.24%

total tool calls           542
AST Tool calls              59
AST failures                 7
AST failure rate          11.86%

grep                         17
glob                         22
read                        286

avg recovery distance      1.60
max recovery distance         4

total tokens            178,116
avg tokens/test         4,344.3
```

Phase 7c is therefore not a broad semantic-routing failure.

The remaining regression is narrower:

```text
find usage         12 → 4
glob               12 → 22
read              252 → 286
tool calls        518 → 542
tokens        158,303 → 178,116
```

while several metrics improved:

```text
AST failures        9 → 7
AST failure rate 13.04% → 11.86%
bash               120 → 102
edit                86 → 82
elapsed       2224.27 → 2214.34 sec
```

Phase 7c.1 should preserve these improvements while addressing the localized exploration regressions.

---

# Core Principle

Use this rule throughout:

```text
Do not restore Phase 5 text because Phase 5 was better in aggregate.

Restore Phase 5 text only when:
Phase 5 instruction
→ was removed or weakened in Phase 7c
→ changed an agent decision
→ caused a measurable trajectory cost.
```

Every restoration must have trajectory evidence.

---

# Primary Investigation Targets

Focus on four areas.

## 1. `find` Usage Drop

Phase 5:

```text
find = 12
```

Phase 7c:

```text
find = 4
```

Identify every test where a Phase 5 `find` call disappeared.

For each test, classify the replacement as:

```text
A. find → equivalent targeted search
B. find → symbols or another appropriate AST command
C. find → no query needed because earlier discovery improved
D. find → glob/read/manual structural inspection
E. find → grep/read/manual reasoning
F. other
```

Categories A–C may be benign.

Categories D–E are primary candidates for regression analysis.

Do not try to restore the raw count of `find` calls.

The question is whether a targeted structural AST query was replaced by a broader and more expensive path.

---

## 2. `glob` Increase

Phase 5:

```text
glob = 12
```

Phase 7c:

```text
glob = 22
```

Produce a per-test delta ranking:

```text
task
Phase 5 glob
Phase 7c glob
delta
```

Inspect the largest contributors.

For each additional glob sequence, determine why the agent used it.

Classify it as:

```text
necessary repository discovery
benign alternative
replacement for AST structural lookup
replacement for symbol search
unnecessary broad exploration
```

Pay particular attention to:

```text
glob
→ read
→ manual inspection
```

when Phase 5 used `find`, `search`, or another targeted AST query.

---

## 3. `read` Increase

Phase 5:

```text
read = 252
```

Phase 7c:

```text
read = 286
```

Identify which tests account for the net +34 reads.

Rank tests by:

```text
Δread
Δtokens
Δtool calls
```

Determine whether the additional reads are:

```text
implementation-related
validation-related
semantic recovery
manual structural exploration
broad context gathering
```

The main target is extra context gathering that replaced a targeted AST Tool result.

---

## 4. Recovery Distance = 4

Phase 7c recovery distances are:

```text
1, 1, 1, 1, 4
```

Identify the task producing distance 4.

Reconstruct the exact sequence:

```text
failed AST query
→ intervening actions
→ useful recovery
```

Compare it with Phase 5.

Determine:

* which failure occurred;
* whether the same command was retried unchanged;
* whether help was used;
* whether the error already suggested a cheaper correction;
* whether a Phase 5 recovery rule was weakened in Phase 7c;
* whether a minimal wording restoration could have shortened the path.

Do not change recovery guidance unless this causal link is visible.

---

# Secondary Command Analysis

Also compare:

```text
                 Phase 5   Phase 7c
search              30        25
callers             14        11
references           6         6
callees              3         7
symbols              1         4
find                12         4
```

The goal is not to reproduce Phase 5 command counts exactly.

Investigate meaningful substitutions.

Examples:

```text
find → scoped search
```

may be acceptable.

```text
find → glob → several reads
```

is more suspicious.

Likewise:

```text
callers → references
```

may be wrong if the task specifically requires call sites.

Judge semantic appropriateness, not count equality.

---

# Phase 5 vs Phase 7c Skill Diff

Compare the exact Phase 5 and Phase 7c `SKILL.md` versions.

Focus only on wording relevant to the observed regressions.

Search specifically for Phase 5 guidance concerning:

```text
find
AST structure
node lookup
search vs find
file discovery
workspace discovery
glob
manual file inspection
targeted context
recovery after find failure
help usage
```

Do not conduct another broad rewrite of the entire Skill.

---

# Causal Standard for Restoration

A restoration is justified only when the evidence supports:

```text
Phase 5 wording
        ↓
Phase 7c removed / weakened it
        ↓
agent chose a different route
        ↓
route became broader or more expensive
```

For example:

```text
Phase 5 explicitly says:
"Use find for AST structure before manual file inspection."

Phase 7c shortens this to:
"AST structure → find."

Observed:
Phase 5: find → useful result
Phase 7c: glob → read → read

Conclusion:
the removed preference boundary may be behaviorally important
```

This is a valid restoration candidate.

By contrast:

```text
find usage decreased
```

alone is not enough.

---

# Minimal Restoration Rule

When a restoration is justified, restore the smallest wording that recreates the lost decision boundary.

Prefer:

```text
For AST structure or node lookup, use `find` before manual file inspection.
```

over restoring an entire Phase 5 section.

Prefer one sentence over a paragraph.

Do not restore examples unless the example itself defines the missing boundary.

---

# Do Not Reintroduce Phase 5 Wholesale

Phase 7c already improves:

```text
AST failure count
AST failure rate
bash usage
edit usage
```

Do not discard these gains.

Do not restore Phase 5 text that has no relationship to a measured Phase 7c regression.

The target is:

```text
Phase 7c
+
minimal behavioral restoration
```

not:

```text
Phase 5 with cosmetic compression
```

---

# Editing Budget

Keep the restoration deliberately small.

As a guideline:

```text
prefer 1–5 restored rules
```

and avoid large increases in Skill size.

There is no strict token limit, but Phase 7c.1 should remain materially smaller than Phase 5 unless the evidence clearly requires otherwise.

Report the exact size increase caused by restoration.

---

# Analysis Before Editing

Do not edit immediately.

First produce a diagnostic table.

For each suspicious test, include:

| Test | Phase 5 route | Phase 7c route | Extra cost | Suspected lost rule | Confidence |
| ---- | ------------- | -------------- | ---------- | ------------------- | ---------- |

Use confidence values such as:

```text
HIGH
MEDIUM
LOW
NONE
```

Only HIGH or well-supported MEDIUM cases should normally lead to restoration.

---

# Candidate Restoration Plan

After analysis, propose the smallest restoration set.

For each candidate rule provide:

```text
1. Phase 5 wording or meaning
2. Phase 7c wording
3. observed trajectory difference
4. proposed minimal restored wording
5. expected behavioral effect
```

Do not add a rule merely because it sounds useful.

---

# Implementation

After identifying justified restorations:

1. Edit the Phase 7c `SKILL.md`.
2. Change only the minimum necessary wording.
3. Do not modify AST Tool implementation.
4. Do not modify evaluation tasks.
5. Do not modify CLI behavior.
6. Do not modify metrics logic.
7. Do not change Skill trigger/frontmatter metadata.

---

# Verification Strategy

Use two stages.

## Stage 1 — Targeted Replay

Before running all 41 tests, rerun the tests directly affected by the restored guidance.

At minimum include:

```text
tests where find disappeared into glob/read
the recovery-distance-4 test
largest read/glob regressions plausibly related to Skill wording
```

Compare:

```text
Phase 5
Phase 7c
Phase 7c.1
```

for those tests.

Measure:

```text
success
AST sequence
find usage
search usage
grep
glob
read
tool calls
recovery distance
tokens
elapsed
```

Phase 7c.1 should move the relevant trajectories toward the Phase 5 targeted path without introducing extra exploration elsewhere.

---

## Stage 2 — Full 41-Task Evaluation

Only after targeted replay is favorable, run the unchanged 41-task evaluation.

Collect:

```text
tests
successes
failures
success rate

total tool calls
avg tool calls/test

AST Tool calls
AST failures
AST failure rate
AST retries
AST help calls

search
callers
references
callees
find
symbols
other AST commands

grep
glob
read
bash
edit

recovery distances
avg recovery distance
max recovery distance

elapsed
tokens
per-test metrics
Skill invocation
```

---

# Primary Comparison

Compare Phase 7c.1 primarily against both:

```text
Phase 5
and
Phase 7c
```

Phase 5 answers:

```text
Did we recover the efficient baseline behavior?
```

Phase 7c answers:

```text
Did we preserve the improvements already gained?
```

Do not compare only against Phase 5.

---

# Desired Direction

Phase 7c.1 should ideally preserve:

```text
successes              >= 37
AST failures           <= 7–9
grep                    near 15–17
bash                    <= Phase 7c
edit                    <= Phase 5/7c range
elapsed                 <= Phase 5
```

while improving:

```text
find-related targeted routing
glob
read
total tool calls
recovery distance
tokens
```

These are directional goals, not hard command-count requirements.

---

# Strong Success Case

A strong Phase 7c.1 result would look approximately like:

```text
success rate          >= 90.24%
AST failures          <= Phase 5
grep                  ~ Phase 5
glob                  ~ Phase 5
read                  <= Phase 5
recovery avg          <= Phase 5
total tool calls      < Phase 5
tokens                < Phase 5
elapsed               <= Phase 5
```

Exact equality is not required.

The real target is a cheaper targeted trajectory.

---

# Per-Test Analysis

For every test where Phase 7c.1 differs materially from Phase 7c, record:

```text
what changed
which restored rule plausibly caused it
whether the change was desirable
cost delta
```

This is especially important if aggregate metrics improve.

Do not accept a result solely because totals look better.

---

# Token Analysis

Phase 7c used:

```text
178,116 total tokens
```

versus Phase 5:

```text
158,303
```

Identify whether token reduction in Phase 7c.1 comes from:

```text
fewer broad reads
fewer glob/read chains
shorter recovery
fewer redundant semantic queries
smaller context gathered
```

Use per-test token deltas.

Report:

```text
median delta
p75
p90
largest regressions
largest improvements
top outlier contribution
```

Do not rely only on aggregate total tokens.

---

# Acceptance Decision

Choose one:

```text
ACCEPT
ACCEPT WITH CAVEATS
REVISE
REVERT TO PHASE 7C
```

## ACCEPT

Use when:

```text
Phase 7c improvements are preserved
+
targeted structural routing improves
+
manual exploration decreases
+
no new correctness/recovery regression appears
```

Ideally the result should outperform Phase 5 on agent-level efficiency.

---

## ACCEPT WITH CAVEATS

Use when the targeted restoration clearly improves the intended trajectories but aggregate results contain plausible stochastic noise.

---

## REVISE

Use when the right regression was identified but the restored wording is too broad or introduces a new behavioral cost.

---

## REVERT TO PHASE 7C

Use when the restoration does not improve the targeted trajectories or causes broader regression.

Phase 7c remains the fallback candidate.

Do not automatically revert all the way to Phase 5.

---

# Deliverables

Provide:

1. Phase 5 vs Phase 7c trajectory diagnosis.
2. Per-test `find` disappearance analysis.
3. Per-test glob/read regression ranking.
4. Recovery-distance-4 analysis.
5. Phase 5 vs Phase 7c relevant Skill diff.
6. Causal restoration candidates.
7. Final minimal restoration set.
8. Modified Phase 7c.1 `SKILL.md`.
9. Skill size before/after restoration.
10. Targeted replay results.
11. Full 41-task evaluation results.
12. Phase 5 vs Phase 7c vs Phase 7c.1 comparison.
13. Per-test token/outlier analysis.
14. Final recommendation.

---

# Final Principle

Do not try to make Phase 7c.1 look like Phase 5.

Instead:

```text
Keep what Phase 7c improved.
Restore only what Phase 7c accidentally weakened.
```

The final question is:

```text
Can a very small restoration of Phase 5 decision cues
eliminate Phase 7c's extra structural/manual exploration
without giving up Phase 7c's lower AST failure rate and
other efficiency gains?
```

Phase 7c.1 succeeds only if the answer is supported by trajectory evidence.
