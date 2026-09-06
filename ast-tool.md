# Phase 8b — C++ Receiver-Type Member Relationship Resolution

## Objective

Investigate and, only if justified by focused evidence, improve C++ relationship discovery for member calls whose target can be resolved from the receiver's static type.

The motivating failure pattern is:

```text
token_.validate(...)
validator_.validate(...)
```

where the relationship target itself is known and resolvable, but existing semantic relationship discovery fails to connect the member call to the canonical method.

Phase 8b must determine whether the current semantic layer can safely resolve these calls using existing AST IR and symbol information without expanding into general C++ type inference.

The primary commands affected are:

```text
callers
callees
references
```

The optimization target remains the Coding Agent as a whole, but Phase 8b first isolates the semantic capability itself.

---

# Baseline

Use the accepted Phase 8a implementation as the baseline.

Phase 8a target-resolution behavior must remain unchanged.

In particular, preserve:

```text
exact FQN precedence
unique FQN-suffix fallback
ambiguity preservation
not-found behavior
unqualified lookup behavior
declaration/definition canonicalization
```

Do not modify:

```text
SKILL.md
Skill description
Skill invocation behavior
relationship target resolver
search semantics
find semantics
CLI command surface
```

unless a later Phase 8b finding explicitly requires a separate follow-up phase.

Record the baseline revision and hashes before making changes.

---

# Background

Phase 7f identified a broader relationship limitation after target identity had already been resolved.

Observed examples include calls such as:

```text
token_.validate(...)
validator_.validate(...)
```

while queries for canonical methods such as:

```text
auth::AuthToken::validate
service::ValidationService::validate
```

returned empty caller/reference results.

A related `callees` case also returned no relationship even though the source body contained member calls.

The current implementation appears to resolve member-call identifiers without sufficiently connecting the receiver expression to its static type.

Phase 8b must validate that diagnosis before implementing a fix.

---

# Primary Questions

Phase 8b must answer:

```text
1. Which receiver forms can the current semantic model resolve safely?

2. Which required type information already exists in AST IR / semantic data?

3. Which member-call failures are caused specifically by missing receiver-type resolution?

4. Can callers/callees/references share one narrow member-resolution mechanism?

5. Can this be fixed without implementing broad C++ type inference?

6. How should ambiguous or unresolved receivers fail safely?
```

Do not assume all member calls should become resolvable.

---

# Scope

Start with ordinary non-template C++ member calls where the receiver's static type can be determined directly from local semantic information.

Candidate in-scope receiver categories are:

```text
1. data-member / field receiver
2. local variable receiver
3. reference receiver
4. pointer receiver
```

Examples:

```cpp
token_.validate();

Validator validator;
validator.validate();

Validator& validator = ...;
validator.validate();

Validator* validator = ...;
validator->validate();
```

These categories must be investigated separately.

Implementation authorization depends on the capability matrix described below.

---

# Explicit Non-Goals

Do not attempt to solve all of C++ member resolution.

Phase 8b must not expand into:

```text
general type inference
template instantiation
template-dependent member lookup
virtual dispatch target expansion
dynamic runtime type inference
full overload resolution
ADL
concepts
SFINAE
auto type deduction beyond already available semantic data
decltype reasoning
macro expansion redesign
cross-language resolution
```

Also do not change Phase 8a's target resolver.

If any of these become necessary for correctness, stop and classify the case as out of scope rather than broadening the implementation.

---

# Stage 1 — Trace the Current Member-Call Resolution Path

Before modifying production code, inspect and document the existing path used for member calls.

Determine:

```text
1. How a call expression is represented in AST IR.
2. How the member identifier is extracted.
3. Whether the receiver expression is retained.
4. Whether receiver symbol identity is retained.
5. Whether declared/static type information is available.
6. How IdentifierResolver currently resolves member names.
7. Where unresolved member calls are dropped.
8. Whether callers, callees, and references share infrastructure.
```

Produce a concise current-flow diagram such as:

```text
call expression
→ receiver/member extraction
→ identifier resolution
→ canonical target
→ relationship index
```

Mark exactly where receiver-type information is currently lost or ignored.

Do not implement a fix until this path is understood.

---

# Stage 2 — Build Typed Receiver Fixtures

Create minimal focused C++ fixtures before changing behavior.

The fixtures must isolate receiver forms and avoid unrelated complexity.

At minimum include the following.

## Case A — Field receiver

```cpp
class Session {
    AuthToken token_;
public:
    void refresh() {
        token_.validate();
    }
};
```

Expected target:

```text
auth::AuthToken::validate
```

Test:

```text
callers
references
callees
```

as applicable.

---

## Case B — Local object receiver

```cpp
void run() {
    Validator validator;
    validator.validate();
}
```

Expected:

```text
Validator::validate
```

---

## Case C — Reference receiver

```cpp
void run(Validator& validator) {
    validator.validate();
}
```

Expected:

```text
Validator::validate
```

---

## Case D — Pointer receiver

```cpp
void run(Validator* validator) {
    validator->validate();
}
```

Expected:

```text
Validator::validate
```

---

## Case E — Same member name on unrelated types

```cpp
struct A {
    void validate();
};

struct B {
    void validate();
};

void run(A& a, B& b) {
    a.validate();
    b.validate();
}
```

Expected:

```text
a.validate()
→ A::validate

b.validate()
→ B::validate
```

This is a critical guard.

Do not resolve only by member name.

---

## Case F — Unresolved receiver

Construct a case where the receiver type is unavailable to the semantic layer.

Expected:

```text
remain unresolved
```

Do not guess based on the member name.

---

## Case G — Ambiguous receiver/type information

If the semantic model can represent an ambiguous receiver type, test it.

Expected:

```text
preserve ambiguity or unresolved state
```

Never choose an arbitrary type.

---

# Stage 3 — Capability Matrix

Before authorizing implementation, produce a matrix like:

| Receiver form         | Receiver symbol known? | Static type available? | Canonical type resolvable? | Member target resolvable? |
| --------------------- | ---------------------- | ---------------------- | -------------------------- | ------------------------- |
| field                 | yes/no                 | yes/no                 | yes/no                     | yes/no                    |
| local object          |                        |                        |                            |                           |
| reference             |                        |                        |                            |                           |
| pointer               |                        |                        |                            |                           |
| implicit `this`       |                        |                        |                            |                           |
| unresolved expression |                        |                        |                            |                           |

This is a decision gate.

Classify each category as:

```text
A. Safely implementable with existing semantic data
B. Implementable with one narrow local extension
C. Requires broader type inference
D. Inconclusive
```

Only A and possibly narrowly justified B categories may proceed to implementation in Phase 8b.

---

# Stage 4 — Define the Narrow Resolution Rule

If implementation is justified, define the rule before coding.

The preferred conceptual rule is:

```text
member call:
    receiver.member(args)
or
    receiver->member(args)

1. resolve receiver symbol
2. obtain receiver's declared/static type
3. resolve that type to one canonical class/struct identity
4. search member candidates within that type
5. resolve only when the member target is unambiguous
6. otherwise preserve unresolved/ambiguous behavior
```

Do not use:

```text
global member-name match
first matching method
suffix-only method guessing
source-order preference
```

Receiver type must constrain the member search.

---

# Stage 5 — Reuse Existing Semantic Infrastructure

Prefer existing symbol/type infrastructure over creating a separate C++ type system.

Before adding new data structures, search for existing support for:

```text
variable declared type
field declared type
parameter type
reference/pointer base type
canonical type FQN
class/struct symbol identity
```

Reuse existing declaration/definition canonicalization where applicable.

Avoid implementing command-specific member resolution independently in:

```text
callers
callees
references
```

The relationship commands should share the semantic resolution result.

---

# Stage 6 — Overload Boundary

Overloads require special care.

Fixture:

```cpp
struct Validator {
    void validate();
    void validate(int);
};
```

Do not claim Phase 8b supports general overload resolution unless argument information already makes selection deterministic using existing infrastructure.

If current semantic data cannot safely choose among overloads:

```text
receiver type known
member name known
multiple overload candidates
```

then preserve ambiguity or the existing unresolved behavior.

It is acceptable for Phase 8b to resolve:

```text
type + member name
```

only when that combination identifies one canonical member.

Do not expand scope merely to make the overload fixture pass.

---

# Stage 7 — Implement the Smallest Shared Change

If the capability matrix supports implementation, modify the narrowest shared semantic layer that can improve:

```text
callers
callees
references
```

together.

Expected architecture:

```text
member call
→ receiver symbol/type resolution
→ canonical receiver type
→ member lookup constrained by type
→ existing relationship indexing
```

Avoid command-specific patches.

The implementation should fail closed:

```text
type unknown
→ unresolved

type ambiguous
→ unresolved/ambiguity

member ambiguous
→ unresolved/ambiguity
```

Never fall back to broad same-name resolution.

---

# Stage 8 — Unit / Semantic Tests

Add focused tests for every implemented receiver category.

At minimum verify:

```text
field object
local object
reference
pointer
same member name on unrelated types
unresolved receiver
ambiguous member candidate
```

If a category remains unsupported, add a regression test proving that it remains safely unresolved rather than incorrectly linked.

The goal is not maximum resolution coverage.

The goal is:

```text
more correct relationships
without false relationships
```

---

# Stage 9 — Cross-Command Guards

For each supported receiver category, verify behavior through all relevant commands.

## Callers

Query the canonical member:

```text
callers Validator::validate
```

Expected:

```text
functions containing correctly typed receiver calls
```

---

## References

Query the canonical member:

```text
references Validator::validate
```

Expected:

```text
member-call references included
```

with no references from unrelated receiver types.

---

## Callees

Query a function containing:

```cpp
validator.validate();
```

Expected:

```text
Validator::validate
```

in its callees.

---

# Stage 10 — Reproduce the Phase 7f Failures

Replay the exact motivating fixtures/tasks.

At minimum validate cases equivalent to:

```text
token_.validate(...)
validator_.validate(...)
```

Record before and after:

```text
relationship command
target
returned set
AST calls
failures
retries
tools
tokens
elapsed
```

The primary semantic criterion is:

```text
previously missing valid relationship
→ now present
```

without introducing false positives.

---

# Stage 11 — False-Positive Guards

False relationships are more dangerous than unresolved relationships.

Create explicit negative tests.

Examples:

```cpp
struct AuthToken {
    void validate();
};

struct ValidationService {
    void validate();
};
```

Then ensure:

```text
token_.validate()
```

does not resolve to:

```text
ValidationService::validate
```

and vice versa.

Also test two namespaces containing similarly named types where practical.

Phase 8b must remain conservative.

---

# Stage 12 — Targeted Agent-Level Validation

After semantic tests pass, replay existing evaluation tasks that previously suffered from empty member relationships.

Use the accepted Phase 8a Skill and resolver unchanged.

Force semantic routing for causal comparison if needed.

Recommended:

```text
5 runs per task
```

Extend to:

```text
10 runs
```

for highly variable tasks.

Record:

```text
success
tools
AST calls
AST failures
retries
search
callers
callees
references
grep
glob
read
tokens
elapsed
recovery
```

Also compare trajectory shape.

Example target improvement:

```text
Before:
relationship returns empty
→ source inspection / fallback

After:
relationship returns populated result
→ targeted edit/work
```

---

# Stage 13 — Evaluate Information Value

Do not accept Phase 8b solely because more relationships are returned.

For each changed task, ask:

```text
Did the newly resolved relationship:
- eliminate manual exploration?
- reduce Reads?
- reduce retries?
- improve correctness?
- reduce tokens?
- reduce elapsed time?
```

It is possible for semantic correctness to improve without immediate token savings.

That can still justify the capability if the relationship result is demonstrably more correct and useful.

But report the tradeoff clearly.

---

# Metrics

Record at minimum:

```text
success
relationship answer accuracy
false-positive count
false-negative count in fixtures
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

For fixture-level semantic validation, report:

```text
expected relationships
actual relationships
missing relationships
unexpected relationships
```

---

# Primary Acceptance Criterion

Phase 8b succeeds only if receiver-constrained resolution improves relationship correctness without creating false relationships.

The highest-priority criterion is:

```text
precision before recall
```

A valid unresolved relationship is preferable to an incorrect relationship.

---

# Acceptance Criteria

## Correct receiver types resolve correctly

Supported field/local/reference/pointer cases must resolve to the expected canonical member.

---

## Same-name unrelated methods stay separate

Receiver type must prevent cross-type contamination.

---

## Unresolved receivers remain safe

Unknown receiver type must not trigger broad member-name matching.

---

## Relationship commands agree

Where applicable:

```text
callers
references
callees
```

must reflect the same canonical member identity.

---

## Phase 8a remains unchanged

All Phase 8a target-resolution tests must continue to pass.

---

## Existing relationship behavior is preserved

No systematic regression in already-correct direct function relationships.

---

## Agent-level usefulness is demonstrated

At least one motivating task should show a newly useful semantic relationship at agent level.

Prefer evidence of reduced fallback/manual exploration, but semantic correctness itself is the primary gate.

---

# Possible Decisions

Choose one final result.

## ACCEPT PHASE 8B

Use when:

```text
receiver-type resolution is narrow and deterministic
member relationships become correct
false-positive guards pass
Phase 8a remains stable
and
agent-level usefulness is demonstrated
```

---

## ACCEPT WITH CAVEATS

Use when:

```text
semantic correctness clearly improves
and
false positives are controlled
but
coverage is intentionally limited
or
agent-level cost savings are noisy
```

Document the supported receiver categories explicitly.

---

## PARTIAL ACCEPT

Use when only a strict subset is safely supported.

For example:

```text
field receiver       supported
local object         supported
reference            supported
pointer              unsupported
overloads            unsupported
```

This is preferable to overextending the implementation.

---

## REVISE

Use when the architecture is sound but one narrow reproducible issue remains.

Do not broaden the type system generically.

---

## REVERT

Use when:

```text
false relationships appear
ambiguity is silently collapsed
Phase 8a behavior regresses
or
the implementation requires broad C++ type inference
```

---

# Stop Conditions

Stop implementation and report instead if any of the following becomes necessary:

```text
full overload resolution
template instantiation
dynamic type inference
virtual dispatch modeling
major AST IR redesign
general C++ type checker
```

Such a finding is valuable evidence that the capability belongs in a larger future phase.

---

# Evidence Standard

Use:

```text
strong:
    deterministic fixture tests
    + repeated agent-level replay
    + explicit false-positive guards

moderate:
    repeated behavior across several fixtures/tasks

weak:
    one agent trace

insufficient:
    plausible source-level reasoning without reproduced behavior
```

Do not accept broad receiver-resolution semantics from weak evidence.

---

# Deliverables

Produce a final Phase 8b report containing:

```text
1. Environment and revisions

2. Phase 8a baseline verification

3. Current member-resolution flow

4. Typed receiver capability matrix

5. Authorized implementation scope

6. Exact semantic change

7. Fixture/test matrix

8. Supported receiver categories

9. Unsupported receiver categories

10. Cross-command callers/callees/references results

11. False-positive guards

12. Before/after motivating relationship results

13. Targeted repeated agent results

14. Tool/token/recovery metrics

15. Overload and ambiguity assessment

16. Regressions/outliers

17. Final decision

18. Recommendation for the next Phase 8 step
```

Keep raw measurements separate from interpretation.

---

# Recommendation Gate for the Next Phase

Do not automatically expand Phase 8b after success.

After the report, separately decide whether evidence supports work on:

```text
overload-aware member resolution
implicit-this member calls
more complex receiver expressions
declaration/definition identity metadata
```

Each should require its own repeated failure pattern.

A successful Phase 8b should not become an open-ended C++ semantic engine project.

---

# Working Principle

Continue the established methodology:

```text
stable baseline
→ reproduce one semantic limitation
→ characterize available information
→ define a narrow safe capability
→ fixture-first tests
→ implement one shared behavior
→ false-positive guards
→ replay exact motivating cases
→ repeated agent validation
→ expand only on new evidence
```

For Phase 8b, the isolated capability is:

```text
receiver static type
→ canonical member identity
→ callers / callees / references
```

only when that resolution is unambiguous and conservative.
