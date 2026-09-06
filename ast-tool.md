# Phase 8c — C++ Declaration/Definition Body Identity for Callees

## Objective

Investigate and, if justified by focused evidence, improve `callees` so that when a callable target resolves to a declaration without a body, the semantic layer can safely locate the corresponding body-bearing definition of the same canonical callable.

Phase 8c must address exactly this semantic boundary:

```text
callable identity
→ body-bearing definition
→ callee traversal
```

The motivating failure is:

```text
callees auth::AuthService::refresh
→ target resolves successfully
→ selected symbol is a declaration without a body
→ result is empty
```

even though the same canonical callable has an out-of-line definition whose body contains calls.

The goal is not to redesign callable identity generally.

The goal is to connect an already-resolved callable identity to the correct implementation body when that relationship is unambiguous.

---

# Baseline

Use the accepted Phase 8b implementation as the baseline.

Preserve all accepted Phase 8a and Phase 8b behavior.

Do not modify:

```text
Phase 8a FQN-suffix target resolution
Phase 8b receiver-type resolution
SKILL.md
Skill description
Skill invocation behavior
search semantics
find semantics
callers behavior
references behavior
CLI command surface
```

unless a separate regression proves that a shared internal fix is strictly required.

Record baseline revision and relevant hashes before changing code.

---

# Background

Phase 8b demonstrated that member-call identity can now be resolved conservatively for supported receiver forms.

However, one motivating `callees` case remained empty:

```text
auth::AuthService::refresh
```

The callable identity is resolvable, but the selected workspace symbol corresponds to a declaration in a header.

That declaration has no body.

The implementation body exists in an out-of-line definition with the same callable identity.

Therefore the remaining failure is not:

```text
receiver-type resolution
```

and not:

```text
relationship target resolution
```

It is:

```text
declaration / definition body identity
```

Phase 8c must isolate that boundary.

---

# Primary Questions

Answer these before broad implementation:

```text
1. How does callees choose the callable symbol whose body it traverses?

2. When declaration and definition share one FQN,
   how are they represented in workspace symbols?

3. Is there already a canonical declaration/definition relationship?

4. Can a declaration without a body be mapped safely
   to exactly one body-bearing definition?

5. What happens when:
   - there is no definition,
   - multiple definitions exist,
   - overloads share the same name,
   - the definition is inline,
   - the definition is namespace-qualified?

6. Can this be fixed without changing target identity semantics?
```

Do not assume FQN alone is always sufficient until fixtures verify it.

---

# Scope

Phase 8c should initially support ordinary non-template C++ callables whose declaration and definition can be matched using existing semantic identity.

Candidate in-scope cases:

```text
1. free function declaration + source definition
2. class method declaration + out-of-line definition
3. namespace-qualified out-of-line definition
4. inline definition
5. constructor/destructor declaration + definition
   only if already represented consistently
```

Only include constructors/destructors if existing infrastructure already supports them cleanly.

Do not expand scope merely for coverage.

---

# Explicit Non-Goals

Do not implement:

```text
general overload resolution
template instantiation
template specialization matching
ODR analysis
linker-level identity
virtual dispatch expansion
inheritance-aware body selection
macro-aware definition synthesis
cross-language definition mapping
```

Also do not extend Phase 8b receiver inference.

Phase 8c is not a general C++ symbol linker.

---

# Stage 1 — Trace Current Callees Body Selection

Before changing production code, document the current path.

Determine:

```text
1. how `callees` receives its resolved target;
2. which workspace symbol is selected;
3. how the body node is obtained;
4. what happens when that symbol has no body;
5. whether declaration and definition entries share:
   - FQN,
   - symbol kind,
   - scope,
   - canonical ID,
   - source identity;
6. whether an existing helper already collapses or pairs them.
```

Produce a concise current-flow diagram, for example:

```text
CLI target
→ resolve_symbol_query
→ selected callable symbol
→ locate AST node
→ extract body
→ walk calls
→ resolve callees
```

Mark exactly where declaration-only selection causes the result to become empty.

---

# Stage 2 — Build Body-Identity Fixtures First

Create minimal C++ fixtures before modifying behavior.

At minimum cover the following.

## Case A — Free function declaration + definition

```cpp
// api.h
void run();

// api.cpp
void run() {
    helper();
}
```

Expected:

```text
callees run
→ helper
```

even if target resolution initially lands on the header declaration.

---

## Case B — Class method declaration + out-of-line definition

```cpp
class AuthService {
public:
    void refresh();
};

void AuthService::refresh() {
    token_.validate();
}
```

Expected:

```text
callees AuthService::refresh
→ AuthToken::validate
```

assuming Phase 8b can resolve the member call.

---

## Case C — Namespace-qualified method definition

```cpp
namespace auth {

class AuthService {
public:
    void refresh();
};

}

void auth::AuthService::refresh() {
    ...
}
```

Expected:

```text
declaration
→ correct namespaced definition
```

---

## Case D — Inline definition

```cpp
class AuthService {
public:
    void refresh() {
        token_.validate();
    }
};
```

Expected:

```text
use the existing body directly
```

No declaration-to-definition fallback should be required.

---

## Case E — Declaration only

```cpp
void run();
```

with no definition.

Expected:

```text
no body found
→ existing empty/unresolved behavior
```

Do not invent a definition.

---

## Case F — Multiple incompatible body-bearing candidates

Construct a controlled ambiguous case if the semantic model can represent one.

Expected:

```text
do not select arbitrarily
```

Fail closed or preserve the existing ambiguity behavior.

---

## Case G — Declaration/definition kind mismatch

If current C++ extraction represents:

```text
Method declaration
+
Function-classified out-of-line definition
```

test that the existing callable duplicate/canonicalization semantics are reused.

Do not introduce a competing identity rule.

---

# Stage 3 — Body Identity Matrix

Before authorizing implementation, produce a matrix:

| Callable form       | Declaration found | Definition found | Same canonical identity | Unique body | Safe? |
| ------------------- | ----------------- | ---------------- | ----------------------- | ----------- | ----- |
| free function       |                   |                  |                         |             |       |
| method              |                   |                  |                         |             |       |
| namespaced method   |                   |                  |                         |             |       |
| inline method       |                   |                  |                         |             |       |
| declaration only    |                   |                  |                         |             |       |
| duplicate/ambiguous |                   |                  |                         |             |       |

Classify each as:

```text
A. Existing body works directly
B. Safe declaration→definition mapping
C. Ambiguous / must remain unresolved
D. Unsupported / requires broader semantics
```

Only A and B categories may be implemented.

---

# Stage 4 — Define the Narrow Resolution Rule

If implementation is justified, define the rule before coding.

Preferred semantics:

```text
resolved callable target
→ inspect selected callable

if selected callable has a traversable body:
    use it

otherwise:
    find canonical callable candidates
    representing the same semantic identity

    keep only body-bearing candidates

    apply existing callable duplicate collapse /
    canonicalization

    if exactly one valid body-bearing definition remains:
        use it

    if none:
        preserve existing empty/unresolved behavior

    if multiple incompatible candidates remain:
        fail closed / preserve ambiguity
```

The fallback must be identity-based.

Do not use:

```text
same unqualified name
first source definition
nearest file
substring FQN
source-order preference
```

---

# Stage 5 — Reuse Existing Canonicalization

Phase 8a already relies on existing callable declaration/definition collapse.

Phase 8c should reuse the same semantic identity rules where possible.

Investigate whether the current system already recognizes pairs such as:

```text
Method declaration
+
Function-classified out-of-line definition
```

with the same FQN.

Prefer:

```text
one canonical callable identity
→ choose body-bearing representation when body traversal is required
```

over adding a second ad hoc map used only by `callees`.

---

# Stage 6 — Separate Identity from Body Selection

Do not change what callable the user query means.

Phase 8c must preserve:

```text
target resolution identity
```

and only change:

```text
which representation supplies the body
```

Conceptually:

```text
AuthService::refresh
        │
        ├─ declaration
        └─ definition with body
```

Both should represent the same semantic callable.

`callees` may use the definition body without changing the resolved target identity presented to the user.

This separation is important.

---

# Stage 7 — Implement the Smallest Change

Prefer a shared callable-body helper such as conceptually:

```text
resolve_body_for_callable(symbol)
```

rather than embedding declaration/definition search directly into command logic.

Expected responsibility:

```text
input:
    already-resolved canonical callable

output:
    zero or one safe body-bearing representation
```

Do not let this helper perform unrelated name resolution.

If only `callees` currently needs body traversal, it is acceptable for usage to remain `callees`-specific while the identity logic itself remains semantically clean.

---

# Stage 8 — Focused Semantic Tests

For every implemented case, verify:

```text
target identity
selected body source
expected callees
unexpected callees
```

At minimum assert:

```text
inline body remains unchanged
out-of-line body is found
declaration-only stays empty
ambiguous bodies are not guessed
same-name unrelated function is not selected
```

---

# Stage 9 — Phase 8a and 8b Regression Guards

Run all focused Phase 8a and Phase 8b tests.

Specifically verify:

```text
partial FQN relationship targets still resolve correctly
receiver-typed member calls still resolve correctly
unrelated receiver types remain separated
overloaded members still fail closed where unsupported
```

Phase 8c must not alter these behaviors.

---

# Stage 10 — Replay the Motivating Failure

Replay the exact motivating case:

```text
callees auth::AuthService::refresh
```

Record:

```text
before:
    selected representation
    body availability
    returned callees

after:
    selected body representation
    returned callees
```

Expected causal change:

```text
Before:
declaration selected
→ no body
→ empty callees

After:
same callable identity
→ body-bearing definition selected
→ actual callees returned
```

If Phase 8b is active, expected member relationships should include the correctly resolved member calls from that body.

---

# Stage 11 — Direct Answer Equivalence Guards

Where both declaration and definition can be queried or resolved independently, verify that they map to the same effective callable relationship result.

For example:

```text
query declaration identity
→ callees set X

query exact definition representation
→ callees set X
```

Do this only where the CLI/API makes such comparison meaningful.

The purpose is to prove that body selection does not change semantic identity.

---

# Stage 12 — False-Positive Guards

False body association is more dangerous than an empty result.

Create explicit negative cases.

Examples:

```cpp
namespace a {
    void run();
}

namespace b {
    void run() {
        helper();
    }
}
```

Query:

```text
a::run
```

must not use:

```text
b::run
```

Likewise, two classes with same method name must remain separate.

Do not allow same-name fallback across canonical identities.

---

# Stage 13 — Targeted Agent-Level Validation

After semantic tests pass, replay tasks where empty `callees` previously forced additional search/read/manual reasoning.

Use the accepted Skill and Phase 8a/8b semantics unchanged.

Recommended:

```text
5 runs per motivating task
```

Extend to 10 only if trajectory variance is material.

Record:

```text
success
tools
AST calls
AST failures
retries
search
find
callers
callees
references
grep
glob
read
tokens
elapsed
```

Also record whether the newly populated `callees` result changes the agent trajectory.

---

# Stage 14 — Evaluate Agent-Level Value

Do not accept Phase 8c merely because `callees` returns more entries.

Ask:

```text
Did the corrected body identity:
- eliminate a fallback search?
- eliminate a Read?
- reduce manual exploration?
- reduce retries?
- improve correctness?
- reduce tokens/time?
```

Semantic correctness is the primary gate.

Agent-level savings are supporting evidence.

---

# Metrics

At fixture level, record:

```text
expected body identity
actual body identity
expected callees
actual callees
missing callees
unexpected callees
```

At agent level, record:

```text
success
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
grep
glob
read
bash
edit
tokens
elapsed
recovery mean/max
```

---

# Primary Acceptance Principle

Use:

```text
correct body identity
before
higher recall
```

A declaration with no usable body is preferable to traversal of the wrong definition.

Fail closed when body identity is ambiguous.

---

# Acceptance Criteria

## Inline behavior preserved

Callables that already contain a body must behave exactly as before.

---

## Unique out-of-line definition works

A declaration without a body must map to its unique canonical body-bearing definition.

---

## Declaration-only remains safe

No definition means no guessed body.

---

## Ambiguity remains safe

Multiple incompatible definitions must not be selected arbitrarily.

---

## Semantic identity is preserved

Body selection must not change which callable the target represents.

---

## Phase 8a remains stable

All target-resolution guards must continue to pass.

---

## Phase 8b remains stable

All receiver-resolution and false-positive guards must continue to pass.

---

## Motivating empty `callees` is corrected

At least the known `AuthService::refresh`-style case should move from incorrect empty output to the exact expected callee set.

---

## No false callees introduced

Unexpected relationship count in focused fixtures must remain zero.

---

# Possible Decisions

## ACCEPT PHASE 8C

Use when:

```text
unique declaration→definition body mapping works
correct callees are returned
ambiguity is preserved
no false body association appears
Phase 8a/8b remain stable
and
agent-level usefulness is demonstrated
```

---

## ACCEPT WITH CAVEATS

Use when:

```text
semantic correctness is clear
but
coverage is intentionally narrow
or
agent-level cost improvements are noisy
```

Document exactly which callable forms are supported.

---

## PARTIAL ACCEPT

Use when only a subset is safely supported.

Example:

```text
free functions             supported
ordinary class methods     supported
constructors/destructors   deferred
templates                  unsupported
```

Do not expand implementation just to avoid a partial result.

---

## REVISE

Use when one narrow reproducible body-identity edge case remains.

Do not broaden into general C++ linking.

---

## REVERT

Use when:

```text
wrong definitions are selected
same-name callables cross-link
ambiguity is silently collapsed
Phase 8a or 8b regresses
or
the change requires broad C++ semantic redesign
```

---

# Stop Conditions

Stop and report rather than expanding scope if the fix requires:

```text
general overload resolution
template specialization matching
ODR/linker analysis
inheritance-based body lookup
large AST IR redesign
general symbol graph rewrite
```

Such evidence belongs to a later architectural phase.

---

# Deliverables

Produce a final Phase 8c report containing:

```text
1. Environment and revisions

2. Phase 8a/8b baseline verification

3. Existing callees body-selection flow

4. Body-identity capability matrix

5. Authorized implementation scope

6. Exact semantic change

7. Fixture/test matrix

8. Supported callable forms

9. Unsupported callable forms

10. Declaration→definition mapping behavior

11. Ambiguity and false-positive guards

12. Before/after motivating callees result

13. Phase 8a/8b regression results

14. Targeted repeated agent results

15. Tool/token/recovery metrics

16. Regressions/outliers

17. Final decision

18. Recommendation for the next Phase 8 step
```

Keep raw measurements separate from interpretation.

---

# Gate for the Next Phase

Do not automatically continue semantic expansion after Phase 8c.

After the report, separately decide whether repeated evidence justifies work on:

```text
search declaration/definition metadata
overload-aware member resolution
explicit this-> receiver resolution
complex receiver expressions
other relationship gaps
```

If none has sufficiently strong repeated evidence, Phase 8 may be ready for closure and final agent-level evaluation.

---

# Working Principle

Continue the established methodology:

```text
accepted baseline
→ isolate one reproduced semantic defect
→ characterize identity representation
→ fixture-first validation
→ define one narrow body-selection rule
→ implement conservatively
→ false-positive guards
→ replay the exact motivating failure
→ repeated agent validation
→ expand only on new evidence
```

For Phase 8c, the isolated capability is:

```text
canonical callable identity
→ unique body-bearing definition
→ correct callees traversal
```

Nothing else.
