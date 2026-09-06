# Phase 8a — Unique FQN-Suffix Relationship Resolution

## Objective

Improve the existing relationship-command target resolver so that a partially qualified symbol name can resolve to a **single unambiguous fully qualified symbol by suffix**.

Phase 8a must change exactly one semantic behavior:

```text
relationship target resolution
```

for:

```text
callers
callees
references
```

The goal is to eliminate the repeated protocol:

```text
relationship short-qualified-name
→ not found
→ search
→ relationship full-FQN
```

when the short-qualified name uniquely identifies one canonical symbol in the workspace.

Do not add a new command.

Do not change Skill guidance.

Do not change member-call receiver resolution.

---

# Background

Phase 7f isolated a repeated command-semantics limitation.

Observed recovery pattern:

```text
callers AuthToken::expire
→ target not found

search expire
→ auth::AuthToken::expire

callers auth::AuthToken::expire
→ success
```

The same pattern was observed repeatedly across more than one task.

The current resolver behavior is effectively:

```text
query contains "::"
→ treat as exact FQN
→ exact match fails
→ return not found
```

It does not attempt a unique suffix match.

This forces the agent to perform identity-resolution protocol that the semantic service can safely handle itself.

Phase 8a should test whether the relationship API can absorb this resolution step without changing ambiguity behavior.

---

# Scope

Modify only the shared target-resolution behavior used by:

```text
callers
callees
references
```

The desired lookup order is:

```text
1. Exact FQN match
2. If exact match is absent:
   attempt unique canonical FQN suffix match
3. If exactly one canonical symbol matches:
   resolve it
4. If multiple canonical symbols match:
   preserve ambiguity
5. If none match:
   preserve not-found behavior
```

Do not alter downstream relationship discovery.

---

# Required Semantics

## 1. Exact match has precedence

Given symbols:

```text
AuthToken::expire
auth::AuthToken::expire
```

and query:

```text
AuthToken::expire
```

if an exact canonical symbol named:

```text
AuthToken::expire
```

exists, it must win.

Do not prefer a suffix match over an exact match.

Resolution order must remain:

```text
exact
before
suffix fallback
```

---

## 2. Unique suffix fallback

Given:

```text
auth::AuthToken::expire
```

and query:

```text
AuthToken::expire
```

with no exact match, resolve the query to:

```text
auth::AuthToken::expire
```

only if it is the unique canonical suffix match.

The suffix relationship should be equivalent to matching:

```text
"::" + query
```

against the end of canonical FQNs.

Avoid arbitrary substring matching.

For example:

```text
Token::expire
```

must not accidentally match:

```text
SomeToken::expire
```

unless the qualified-name boundary semantics explicitly make that valid.

---

## 3. Preserve ambiguity

Given:

```text
auth::AuthToken::expire
legacy::AuthToken::expire
```

and query:

```text
AuthToken::expire
```

the resolver must not choose one arbitrarily.

Return the existing ambiguity behavior or equivalent ambiguity result.

The agent should still be required to disambiguate genuinely ambiguous identities.

Phase 8a is not intended to make relationship lookup permissive.

It is intended to remove unnecessary resolution work only when the identity is unique.

---

## 4. Collapse declaration/definition duplicates correctly

A declaration and definition belonging to the same canonical symbol must not cause false ambiguity.

For example, if:

```text
auth::AuthToken::expire
```

appears as:

```text
header declaration
source definition
```

they represent one semantic target for suffix-resolution purposes.

Reuse the existing declaration/definition collapse or canonicalization behavior where possible.

Do not create a second competing identity-resolution implementation unless necessary.

---

## 5. Preserve current unqualified-name behavior

Do not accidentally change queries such as:

```text
expire
```

unless the existing shared resolver already handles them through its current logic.

Phase 8a is specifically about:

```text
partially qualified names containing "::"
```

Do not broaden the change beyond the demonstrated limitation.

---

# Non-Goals

Do not modify:

```text
member receiver-type inference
member-call relationship discovery
overload resolution
virtual dispatch
template resolution
search output metadata
find output
Skill invocation
SKILL.md
command names
CLI surface
```

In particular, do not attempt to fix the separate Phase 7f limitation where:

```text
token_.validate(...)
validator_.validate(...)
```

may not resolve through receiver types.

That is a separate, broader candidate for a later phase.

---

# Implementation Principle

Prefer modifying the existing shared resolver rather than adding command-specific logic to:

```text
callers
callees
references
```

The expected architecture is:

```text
relationship command
→ shared target resolver
→ exact resolution
→ unique suffix fallback
→ existing relationship implementation
```

The semantic rule should be defined once and reused consistently.

Avoid duplicate suffix-resolution logic across commands.

---

# Stage 1 — Inspect Existing Resolution Path

Before modifying code:

1. Identify the shared target-resolution function used by:

   * callers
   * callees
   * references

2. Confirm:

   * exact FQN behavior
   * unqualified-name behavior
   * declaration/definition collapse behavior
   * ambiguity representation
   * not-found behavior

3. Document the current resolution flow briefly.

Do not change code until the exact boundary is understood.

---

# Stage 2 — Add Resolver-Level Tests First

Add focused tests covering at least the following cases.

## Case A — Exact FQN

Workspace:

```text
auth::AuthToken::expire
```

Query:

```text
auth::AuthToken::expire
```

Expected:

```text
exact match
```

No suffix fallback should change the result.

---

## Case B — Unique partial FQN suffix

Workspace:

```text
auth::AuthToken::expire
```

Query:

```text
AuthToken::expire
```

Expected:

```text
auth::AuthToken::expire
```

---

## Case C — Ambiguous suffix

Workspace:

```text
auth::AuthToken::expire
legacy::AuthToken::expire
```

Query:

```text
AuthToken::expire
```

Expected:

```text
ambiguity preserved
```

Do not choose either candidate.

---

## Case D — Declaration/definition duplicate

Workspace contains declaration and definition for:

```text
auth::AuthToken::expire
```

Query:

```text
AuthToken::expire
```

Expected:

```text
one canonical identity
```

Do not report false ambiguity.

---

## Case E — No suffix match

Workspace contains no matching canonical suffix.

Query:

```text
AuthToken::expire
```

Expected:

```text
existing not-found behavior
```

---

## Case F — Exact match plus suffix candidate

Workspace:

```text
AuthToken::expire
auth::AuthToken::expire
```

Query:

```text
AuthToken::expire
```

Expected:

```text
exact AuthToken::expire
```

This test is important for precedence.

---

# Stage 3 — Relationship Command Guards

Verify the new resolver through all commands that share it:

```text
callers
callees
references
```

At minimum, prove that:

```text
unique suffix
→ same target selected consistently
```

for each command.

Also prove that:

```text
ambiguous suffix
→ no arbitrary resolution
```

for each affected command or at the shared resolver level if command tests would be redundant.

---

# Stage 4 — Replay the Observed Phase 7f Recoveries

Replay the exact or equivalent tasks that previously produced:

```text
relationship short-qualified-name
→ failure
→ search
→ relationship full-FQN
```

The key Phase 8a behavioral target is:

```text
relationship short-qualified-name
→ success
```

without the intermediate resolution search and retry.

For each replay record:

```text
before trajectory
after trajectory
AST calls
AST failures
retries
tokens
elapsed
correctness
returned relationship set
```

The returned semantic answers must remain equivalent.

---

# Stage 5 — Targeted Agent-Level Validation

Repeat at minimum the relationship-sensitive tasks identified by Phase 7f, including:

```text
level2-008
level3-008
```

Use semantic routing.

Prefer repeated runs because Phase 7 showed substantial trajectory variance.

Recommended:

```text
5 runs per task
```

If a task still shows meaningful variance, extend to:

```text
10 runs
```

Do not interpret a single agent run as causal evidence.

---

# Stage 6 — Guard Against Regressions

Run targeted guards covering:

```text
exact fully qualified target
unique partial FQN
ambiguous partial FQN
unqualified target
declaration/definition duplicate
not-found target
```

Also run existing relationship-command tests.

If practical, include tasks previously known to use:

```text
search → callers
search → callees
search → references
```

and verify that valid existing flows remain valid.

---

# Causal Success Criterion

The intended improvement is very specific.

Before:

```text
callers AuthToken::expire
→ failure
→ search expire
→ callers auth::AuthToken::expire
```

After:

```text
callers AuthToken::expire
→ success
```

A successful Phase 8a should show:

```text
one relationship call per uniquely resolvable target
```

instead of:

```text
failed relationship
+ resolution search
+ relationship retry
```

while preserving:

```text
correctness
relationship answers
exact-match precedence
ambiguity behavior
not-found behavior
```

---

# Metrics

Record at minimum:

```text
success
tools
AST calls
AST failures
retries
help
search
callers
callees
references
grep
glob
read
tokens
elapsed
recovery mean/max
```

For targeted replays, also record:

```text
relationship target
resolved canonical FQN
number of candidates
whether suffix fallback was used
```

if this can be observed without adding production-only diagnostic behavior.

---

# Acceptance Criteria

Phase 8a may be accepted only if all of the following hold.

## Correctness preserved

Existing validators and semantic answers remain correct.

---

## Exact behavior preserved

Exact FQN resolution still has precedence.

---

## Ambiguity preserved

Multiple suffix matches do not silently select one target.

---

## Canonical duplicates handled

Declaration/definition duplicates do not create false ambiguity.

---

## Observed recovery removed

The Phase 7f three-call relationship-resolution protocol is eliminated in the targeted cases.

---

## Agent-level cost improves directionally

The targeted agent runs should show lower:

```text
AST calls
retries
elapsed
```

and preferably lower:

```text
tools
tokens
```

The primary causal expectation is fewer semantic calls and failures.

Do not require every stochastic metric to improve in every run.

---

## No unrelated semantic changes

Member receiver resolution and other semantic behavior remain unchanged.

---

# Possible Decisions

Choose one final decision.

## ACCEPT PHASE 8A

Use when:

```text
unique suffix resolution works
exact/ambiguity behavior is preserved
targeted recovery disappears
relationship answers are unchanged
and
no systematic regression is observed
```

---

## ACCEPT WITH CAVEATS

Use when:

```text
semantic behavior is correct
and
the recovery protocol is removed
but
agent-level token/time savings are noisy
```

This is acceptable if the command-level causal improvement is clear.

---

## REVISE

Use when:

```text
the approach is fundamentally correct
but
one specific reproducible edge case remains
```

Identify the exact case.

Do not broaden the resolver generically.

---

## REVERT

Use when:

```text
suffix fallback introduces ambiguity errors
changes exact-match behavior
returns wrong relationships
or
creates systematic regression
```

---

# Important Failure Modes to Avoid

## 1. Arbitrary fuzzy matching

Do not implement:

```text
substring search
edit distance
best effort
first match
```

Resolution must remain deterministic and semantic.

---

## 2. Silently resolving ambiguity

Never choose a candidate merely because it appears first.

---

## 3. Expanding into receiver-type resolution

Do not fix:

```text
object.member()
```

relationship resolution in this phase.

That would destroy causal isolation.

---

## 4. Changing Skill guidance

Do not teach the agent to rely on suffix resolution by changing SKILL.md during this experiment.

The command should first prove that its semantics are safe.

---

## 5. Adding a new command

Do not create:

```text
resolve-callers
smart-callers
search-callers
```

or similar.

The Phase 7f evidence supports improving the existing relationship resolver.

---

## 6. Optimizing only AST call count

A lower AST call count is useful only if semantic correctness and ambiguity handling remain sound.

The optimization target remains the Coding Agent as a whole.

---

# Evidence Standard

Use:

```text
strong:
    repeated targeted agent behavior
    + resolver-level tests

moderate:
    repeated semantic traces across multiple tasks

weak:
    one agent run

insufficient:
    aggregate improvement without trajectory evidence
```

Implementation correctness alone is not enough.

Agent-level replay is required.

---

# Deliverables

Produce a final Phase 8a report containing:

```text
1. Environment and revision

2. Baseline Skill / AST Tool verification

3. Existing resolver behavior

4. Exact implementation change

5. Resolver-level test matrix

6. Relationship-command regression tests

7. Before/after observed recovery trajectories

8. Targeted repeated agent results

9. Aggregate metrics

10. Exact-match precedence assessment

11. Ambiguity assessment

12. Declaration/definition canonicalization assessment

13. Regressions or outliers

14. Final decision

15. Recommendation for Phase 8b
```

Keep raw measurements separate from interpretation.

---

# Phase 8b Gate

Do not automatically proceed to receiver-type member resolution.

After Phase 8a, decide separately whether the broader Phase 7f limitation justifies Phase 8b:

```text
member call through receiver type
→ callers / references / callees relationship resolution
```

Phase 8b is justified only if Phase 8a is stable and the member-resolution limitation remains reproducible and important.

---

# Working Principle

Phase 8a continues the methodology established in Phase 7:

```text
stable baseline
→ isolate one reproducible inefficiency
→ modify one semantic behavior
→ test resolver semantics
→ replay exact failing trajectories
→ repeated agent-level validation
→ accept only on causal evidence
```

The isolated Phase 8a behavior is:

```text
unique FQN-suffix fallback
for relationship target resolution
```

Nothing else.
