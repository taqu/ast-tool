# Phase 2 — Fix Semantic Resolution for C++ Declaration / Definition Ambiguity

## Goal

Fix semantic symbol resolution for:

```text
callers
references
callees
```

with particular focus on C++ functions and methods that have both:

```text
declaration
definition
```

in different files.

The current behavior frequently treats the declaration and definition of the same logical C++ symbol as two independent candidates.

Example:

```text
auth::AuthToken::validate
```

may currently appear as:

```text
Method   auth::AuthToken::validate   auth_token.h
Function auth::AuthToken::validate   auth_token.cpp
```

and commands such as:

```bash
ast-tool callers auth::AuthToken::validate .
```

fail with an ambiguity error.

This phase must make declaration and definition resolve to the same logical semantic symbol.

The primary objective is:

```text
search
  ↓
exact semantic symbol
  ↓
callers / references / callees
  ↓
successful result
```

without requiring Grep-based fallback.

---

# Background

Phase 1 significantly improved agent tool selection.

Observed improvements included:

```text
total tool calls:
614 → 553

ast-tool calls:
152 → 67

ast-tool retries:
60 → 22

ast-tool help calls:
8 → 1

total elapsed:
2685.64s → 2290.81s

total tokens:
186155 → 176983
```

However, semantic relationship commands still fail at a very high rate.

Current Phase 1 results include:

```text
callers:
23 calls
23 failures

callees:
3 calls
3 failures

references:
4 calls
4 failures
```

At the same time:

```text
Grep:
17 → 25

Read:
259 → 279
```

This strongly suggests that agents are now selecting the right semantic commands more often, but the commands themselves fail and force fallback exploration.

Phase 2 should address this semantic-resolution bottleneck.

---

# Scope

Fix semantic symbol identity and lookup behavior used by:

```text
callers
references
callees
```

Focus first on C++.

Primary cases:

```text
free function declaration + definition
class method declaration + out-of-line definition
constructor declaration + definition
destructor declaration + definition
namespace-qualified definitions
```

The implementation should make declaration and definition behave as one logical semantic entity where they represent the same C++ symbol.

---

# Core Problem

The current semantic layer appears to distinguish symbols based partly on syntax/extractor representation.

For example:

```cpp
// auth_token.h

namespace auth {

class AuthToken {
public:
    bool validate(const std::string& token);
};

}
```

and:

```cpp
// auth_token.cpp

namespace auth {

bool AuthToken::validate(const std::string& token) {
    ...
}

}
```

may currently produce something conceptually like:

```text
Method:
  fqn = auth::AuthToken::validate

Function:
  fqn = auth::AuthToken::validate
```

The resolver then sees:

```text
2 matching symbols
```

and reports ambiguity.

That is incorrect at the semantic level.

These are two syntax-level representations of one logical C++ function.

---

# Design Principle

Do not solve this by adding special cases only inside the CLI command handlers.

Avoid:

```text
callers:
  if two candidates have same FQN,
  pick one
```

or equivalent command-specific hacks.

The correct architecture should remain:

```text
AST IR
  ↓
Semantic Layer
  ↓
Workspace
  ↓
Semantic Resolver
  ↓
callers / references / callees
```

The declaration/definition relationship should be represented or normalized in the semantic layer or workspace symbol model.

All semantic services should benefit from the same fix.

---

# Required Investigation

Before implementing the fix, inspect the current semantic model and identify:

1. How declarations are represented.
2. How definitions are represented.
3. How symbol kind is assigned.
4. How fully-qualified names are generated.
5. How function/method signatures are stored.
6. How Workspace indexes symbols.
7. How `search` locates symbols.
8. How `callers`, `references`, and `callees` resolve query names.
9. Whether the project already has any concept of:

   * canonical symbol
   * declaration link
   * definition link
   * symbol key
   * signature key
   * canonical declaration
10. Whether node IDs are syntax-node-specific or semantic-symbol-specific.

Do not redesign the semantic layer before understanding the current model.

---

# Canonical Semantic Identity

Introduce or improve a semantic identity mechanism that can determine when multiple syntax-level symbols represent the same logical symbol.

Conceptually, a C++ callable identity should be based on information such as:

```text
qualified name
callable kind
parameter signature
possibly cv/ref qualifiers
possibly template identity
```

The exact representation must match the existing architecture.

Do not use source location as the primary identity because declaration and definition live at different locations.

Do not use AST node ID as the semantic identity if node IDs are syntax-node-specific.

---

# Minimum Matching Requirements

At minimum, support the common non-template cases used by the evaluation repositories.

## Free functions

Example:

```cpp
bool validate(const std::string&);
```

and:

```cpp
bool validate(const std::string& value) {
    ...
}
```

should resolve to the same semantic callable.

---

## Namespaced functions

Example:

```cpp
namespace auth {
bool validate(const std::string&);
}
```

and:

```cpp
bool auth::validate(const std::string& value) {
    ...
}
```

should resolve to the same symbol if supported by the parser/extractor.

---

## Class methods

Example:

```cpp
class AuthToken {
public:
    bool validate(const std::string&);
};
```

and:

```cpp
bool AuthToken::validate(const std::string& value) {
    ...
}
```

must resolve to the same logical method.

This is the highest-priority case.

---

## Constructors

Example:

```cpp
class Foo {
public:
    Foo(int value);
};
```

and:

```cpp
Foo::Foo(int value) {
}
```

should resolve to the same callable.

---

## Destructors

Example:

```cpp
class Foo {
public:
    ~Foo();
};
```

and:

```cpp
Foo::~Foo() {
}
```

should resolve to the same callable.

---

# Overloads Must Remain Distinct

Do not collapse distinct overloads.

Example:

```cpp
void parse(int);
void parse(std::string);
```

must remain two independent semantic symbols.

Likewise:

```cpp
void Foo::set(int);
void Foo::set(std::string);
```

must remain distinct.

Therefore:

```text
FQN alone is not sufficient
```

for canonical identity.

The implementation must include enough signature information to distinguish overloads.

---

# Declaration and Definition Relationship

Prefer an explicit semantic relationship if it fits the current design.

Conceptually:

```text
SemanticSymbol
    identity
    declarations[]
    definition?
```

or equivalent.

A logical callable may have:

```text
0..N declarations
0..1 definition
```

for the common case.

Do not force this exact data model if the project already has a more appropriate representation.

The important property is that:

```text
header declaration
+
source definition
```

can be queried as one semantic symbol.

---

# Search Behavior

`search` may still display declaration and definition separately if that is intentional and useful.

However, semantic query resolution must understand that they belong to the same logical symbol.

There are two acceptable approaches.

## Option A — Search returns canonical symbols

Example:

```text
Method auth::AuthToken::validate src/auth/auth_token.h:9
```

once.

## Option B — Search returns both physical occurrences

Example:

```text
Declaration auth::AuthToken::validate ...
Definition  auth::AuthToken::validate ...
```

but both contain or resolve to the same semantic identity.

Either approach is acceptable if it fits the current CLI contract.

Do not make unnecessary breaking changes to `search` output in this phase.

---

# `callers` Behavior

After this phase:

```bash
ast-tool callers auth::AuthToken::validate .
```

must not fail merely because both declaration and definition exist.

The resolver should first resolve:

```text
auth::AuthToken::validate
```

to the canonical semantic callable.

Then caller analysis should use that semantic identity.

Expected result:

```text
auth::AuthService::login src/auth/auth_service.cpp:14:...
web::AuthController::handleLogin src/web/auth_controller.cpp:13:...
web::AuthController::handleRefresh src/web/auth_controller.cpp:26:...
web::SessionController::handle src/web/session_controller.cpp:12:...
```

Exact formatting should follow the existing CLI.

---

# `references` Behavior

Likewise:

```bash
ast-tool references auth::AuthToken::validate .
```

should resolve the declaration/definition pair as one semantic symbol.

References should represent genuine uses of the logical symbol.

Do not incorrectly report the definition as a reference if current semantics exclude declaration/definition sites.

Preserve the existing documented behavior for declaration-site inclusion/exclusion.

---

# `callees` Behavior

`callees` should also operate on the canonical callable.

Example:

```bash
ast-tool callees auth::AuthService::login .
```

must work even when:

```text
AuthService::login
```

has both a header declaration and source definition.

The command should analyze the callable definition and return semantic callees.

---

# Resolver Behavior

The resolver should distinguish between:

```text
multiple syntax occurrences of one semantic symbol
```

and:

```text
multiple distinct semantic symbols
```

Example that should NOT be ambiguous:

```text
Method declaration:
auth::AuthToken::validate(std::string const&)

Function definition:
auth::AuthToken::validate(std::string const&)
```

Example that SHOULD remain ambiguous if queried without enough information:

```text
auth::Parser::parse(int)
auth::Parser::parse(std::string)
```

If the CLI query syntax currently cannot specify a signature, preserve current ambiguity behavior for overloads.

Do not silently select an arbitrary overload.

---

# Symbol Kind Normalization

Investigate whether:

```text
Method
```

in a class declaration and:

```text
Function
```

for an out-of-line method definition is a source of incorrect identity.

If so, normalize callable kind at the semantic layer.

For example, an out-of-line definition:

```cpp
bool AuthToken::validate(...)
```

should semantically still be a:

```text
method
```

even if Tree-sitter syntax structurally resembles a function definition.

Do not leak parser/extractor syntax distinctions into logical semantic identity when they are not semantically meaningful.

---

# Extractor Responsibility

Follow the existing design principle:

```text
language-specific processing belongs in extractors
```

If the C++ extractor has enough information to identify:

```text
AuthToken::validate
```

as a class method definition, prefer fixing that representation close to extraction.

However, cross-file declaration/definition linking likely belongs in Workspace/Semantic Layer rather than the individual file extractor.

Use the architecture that best preserves these existing principles:

```text
Tree-sitter only parses
AST IR remains stable
Semantic Layer depends on AST IR
Workspace aggregates cross-file semantic information
Semantic Services stay parser-independent
```

---

# Avoid Tree-sitter Leakage

Do not make:

```text
callers
references
callees
```

directly inspect Tree-sitter nodes to resolve this issue.

All parser-specific logic must stay below the semantic-service layer.

Semantic services should continue to depend on semantic/workspace abstractions.

---

# Stable Identity and IDs

Do not implement the Phase 3 public symbol-ID CLI API yet.

However, if canonical semantic identity requires introducing an internal stable semantic key, that is allowed and encouraged.

For example:

```text
SemanticSymbolKey
```

may be useful internally.

But do not add:

```bash
callers --id ...
references --id ...
callees --id ...
```

in this phase.

That belongs to the next phase.

---

# Possible Internal Key

A conceptual callable key might look like:

```text
namespace/class scope
+
callable name
+
normalized parameter types
+
qualifiers
```

For example:

```text
auth::AuthToken::validate(std::string const&)
```

Do not use this exact textual format unless it fits the existing code.

Prefer an explicit structured key over fragile string concatenation if practical.

---

# Type Normalization

Do not over-engineer full C++ type canonicalization in this phase.

Support the level required by current evaluation repositories and existing semantic extraction.

At minimum, declarations and definitions written in the normal corresponding forms should match.

Be cautious with:

```text
const placement
reference syntax
parameter names
whitespace
```

Parameter names must not affect identity.

Example:

```cpp
bool validate(const std::string& token);
```

and:

```cpp
bool AuthToken::validate(const std::string& value)
```

must match.

---

# Templates

Templates are not a primary goal for this phase.

Do not implement a large C++ template-instantiation system.

If templates already work, preserve them.

If declaration/definition linking for templates is straightforward using the same canonical key, support it.

Otherwise document limitations.

---

# Virtual Dispatch

Do not expand the scope into virtual dispatch analysis.

Existing `callers` behavior may only report statically resolvable direct calls.

Preserve that behavior.

This phase is about target symbol resolution, not dispatch modeling.

---

# Multiple Declarations

Support multiple declarations when they refer to the same logical symbol.

For example:

```text
forward declaration
header declaration
definition
```

should not create three semantic candidates.

They should belong to the same canonical symbol where identifiable.

---

# Duplicate / Conflicting Definitions

Do not silently merge genuinely conflicting definitions.

If the workspace contains invalid C++ such as multiple incompatible definitions, preserve an error or ambiguity state.

The resolver should only canonicalize occurrences that are semantically compatible.

---

# Workspace Index

Inspect the workspace symbol index.

If it currently resembles:

```text
FQN → vector<Symbol>
```

consider whether a second canonical index is needed.

Conceptually:

```text
SemanticKey → CanonicalSymbol
```

and possibly:

```text
FQN → set<SemanticKey>
```

This naturally supports:

```text
same FQN + same signature
→ same semantic symbol

same FQN + different signature
→ overloads
```

Do not force this exact implementation if the existing index provides a cleaner extension point.

---

# Regression Tests

Add focused tests before or alongside the fix.

At minimum add cases for the following.

## 1. Method declaration + definition

```cpp
// foo.h
class Foo {
public:
    bool run(int value);
};

// foo.cpp
bool Foo::run(int value) {
    return true;
}
```

Expected:

```text
one logical callable
```

and no ambiguity for:

```text
Foo::run
```

when there is no overload.

---

## 2. Callers across declaration / definition

Create call sites in multiple files.

Verify:

```bash
ast-tool callers ns::Foo::run <root>
```

returns all expected direct callers.

---

## 3. References across declaration / definition

Verify:

```bash
ast-tool references ns::Foo::run <root>
```

returns semantic uses without ambiguity.

---

## 4. Callees across declaration / definition

Verify:

```bash
ast-tool callees ns::Foo::run <root>
```

analyzes the definition correctly.

---

## 5. Free function declaration + definition

Verify canonicalization.

---

## 6. Constructor declaration + definition

Verify canonicalization.

---

## 7. Destructor declaration + definition

Verify canonicalization.

---

## 8. Overloads

Example:

```cpp
void run(int);
void run(std::string);
```

Verify they remain distinct.

A query that cannot disambiguate overloads should still produce ambiguity.

---

## 9. Same name in different classes

Example:

```cpp
A::validate()
B::validate()
```

must remain distinct.

---

## 10. Same name in different namespaces

Example:

```cpp
foo::parse()
bar::parse()
```

must remain distinct.

---

## 11. Multiple declarations of the same function

Verify they canonicalize correctly.

---

## 12. Definition without visible declaration

A standalone definition should continue to work as a semantic callable.

---

# Existing Evaluation Regression

After unit tests pass, rerun the same 41 evaluation tests used in Phase 1.

Do not modify:

* Skill.md
* test YAML
* repositories
* agent prompts
* evaluation model
* result analysis rules

The only intended behavioral change should come from improved semantic resolution.

---

# Metrics to Compare

Compare Phase 1 and Phase 2 using the existing analysis metrics.

At minimum:

```text
validation success

total_tool_calls
average_tool_calls_per_test

ast_tool_calls
ast_tool_failures
ast_tool_failure_rate

ast_tool_retries
ast_tool_help_calls

callers calls
callers failures

references calls
references failures

callees calls
callees failures

grep_calls
read_calls

average_ast_tool_recovery_distance
max_ast_tool_recovery_distance

total_elapsed_seconds
average_elapsed_seconds

total_input_tokens
total_output_tokens
total_tokens
average_tokens_per_test
```

---

# Primary Success Metrics

The most important Phase 2 metrics are:

```text
callers failure rate ↓
references failure rate ↓
callees failure rate ↓
```

with no correctness regression.

A strong result would look conceptually like:

```text
Before:

callers
23 calls
23 failures
100%

After:

callers
N calls
few or zero resolution failures
```

Likewise for:

```text
references
callees
```

---

# Secondary Expected Effects

If semantic resolution works correctly, we expect downstream reductions in:

```text
Grep
Read
retries
recovery distance
elapsed time
tokens
```

However, do not artificially optimize these metrics in this phase.

They should improve naturally because successful semantic queries replace fallback exploration.

---

# Failure Classification

Where practical, distinguish semantic-resolution failures from legitimate empty results.

For example:

```text
0 callers found
```

is not a failed command.

But:

```text
symbol not found
ambiguous symbol
cannot resolve target
```

is a resolution failure.

Do not conflate:

```text
successful empty result
```

with:

```text
resolver failure
```

This distinction is important for evaluation.

---

# Diagnostic Logging

If needed for development, add narrowly scoped debug instrumentation for:

```text
query
candidate symbols
canonical keys
declaration/definition links
resolution result
```

Do not leave verbose debug output enabled in the normal CLI.

If the project has an existing debug logging mechanism, reuse it.

---

# Backward Compatibility

Preserve existing CLI syntax:

```bash
ast-tool callers <symbol> <root>
ast-tool references <symbol> <root>
ast-tool callees <symbol> <root>
```

Do not require users or agents to pass new arguments.

Existing scripts should continue to work.

---

# Non-Goals

Do NOT implement:

* Skill.md changes
* prompt changes
* `--id` public CLI support
* output/JSON optimization
* command removal
* CLI redesign
* actionable retry suggestions
* automatic fuzzy symbol selection
* full C++ template resolution
* virtual dispatch analysis
* repository-wide type inference redesign
* cache changes
* workspace performance optimization
* Tree-sitter usage in semantic services

Do not proceed to the next phase.

---

# Recommended Implementation Order

Use this order:

```text
1. Inspect current symbol representation
2. Inspect resolver and Workspace indices
3. Add focused failing regression tests
4. Define canonical callable identity
5. Normalize method/function semantic kind where necessary
6. Link declaration and definition
7. Update resolver to use canonical symbols
8. Verify callers
9. Verify references
10. Verify callees
11. Run all unit tests
12. Run the same 41 agent evaluation tests
13. Compare Phase 1 vs Phase 2 metrics
```

Do not start with CLI workarounds.

---

# Acceptance Criteria

Phase 2 is complete when:

1. C++ method declarations and out-of-line definitions resolve to one logical semantic callable.
2. C++ free-function declarations and definitions resolve correctly.
3. Constructors and destructors are handled for common declaration/definition cases.
4. Overloads remain distinct.
5. Same-named symbols in different classes/namespaces remain distinct.
6. `callers` no longer fails solely because a declaration and definition both exist.
7. `references` no longer fails solely because a declaration and definition both exist.
8. `callees` no longer fails solely because a declaration and definition both exist.
9. Semantic services do not directly depend on Tree-sitter.
10. Existing CLI syntax remains unchanged.
11. Focused regression tests are added.
12. Existing tests continue to pass.
13. The same 41 evaluation cases are rerun.
14. Phase 1 vs Phase 2 semantic failure rates are reported.
15. Phase 1 vs Phase 2 Grep/Read fallback usage is reported.
16. Phase 1 vs Phase 2 token and elapsed-time metrics are reported.
17. No public symbol-ID API is added yet.
18. No unrelated output or Skill optimization is performed.

---

# Deliverables

Provide:

1. A short explanation of the root cause of declaration/definition ambiguity.
2. The semantic identity / canonicalization design used.
3. The implementation.
4. New regression tests.
5. Any existing tests updated because they encoded incorrect ambiguity behavior.
6. Results for the same 41 evaluation tests.
7. A Phase 1 vs Phase 2 comparison including:

   * callers failure rate
   * references failure rate
   * callees failure rate
   * AST Tool retries
   * Grep calls
   * Read calls
   * total tool calls
   * elapsed time
   * total tokens
   * validation success
8. Remaining known semantic-resolution limitations.
9. Any findings that should be handled by the later symbol-ID or error-UX phases.

Do not proceed to the next phase.
