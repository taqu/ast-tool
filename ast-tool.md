# Phase 3 — Semantic Symbol Resolution

## Goal

Improve semantic symbol resolution so that declarations and definitions of the same C++ symbol are treated as one logical semantic symbol.

The primary objective is to eliminate false ambiguity in semantic queries such as:

```bash
ast-tool callers auth::AuthToken::validate .
ast-tool references auth::AuthToken::validate .
ast-tool callees auth::AuthToken::validate .
```

A declaration in a header and its corresponding definition in a source file must not be treated as two unrelated candidate symbols.

---

## Background

Phase 1 and Phase 2 improved agent behavior and overall efficiency, but semantic query failures remain the main bottleneck.

Current Phase 2 metrics:

```text
tests:                       41
success rate:                90.24%

total tool calls:            519
ast-tool calls:              70
ast-tool failures:           36
ast-tool failure rate:       51.43%
ast-tool retries:            23

average recovery distance:   2.23
max recovery distance:       5
```

Failures by semantic command:

```text
callers:       21 failures / 21 calls
callees:        9 failures / 9 calls
references:     5 failures / 6 calls
find:           1 failure
```

Phase 2 already reduced:

```text
total tool calls
total tokens
elapsed time
grep usage
read usage
```

without reducing the success rate.

Therefore, do not make further agent-instruction or output-format changes in this phase.

The remaining problem appears to be semantic resolution itself.

---

# Scope

Focus on C++ semantic symbol identity and resolution.

Start with:

```text
function declaration ↔ function definition
method declaration   ↔ method definition
```

The semantic layer should recognize these as the same logical symbol when appropriate.

Relevant architecture:

```text
Tree-sitter
    ↓
AST IR
    ↓
Semantic Layer
    ↓
Symbol Identity
    ↓
Resolver
    ↓
Semantic Services
    ├── callers
    ├── references
    └── callees
```

Fix the problem at the semantic identity / resolver layer rather than adding command-specific workarounds.

---

# Non-goals

Do NOT modify the following in this phase unless strictly required for a regression fix:

```text
Skill.md
CLI command structure
JSON output format
pretty-print behavior
command discoverability
error-message UX
symbol-ID CLI API
agent-facing command classification
trace analysis
evaluation task definitions
```

In particular:

* Do not add `callers --id`.
* Do not add `references --id`.
* Do not add `callees --id`.
* Do not redesign CLI error messages.
* Do not remove or rename commands.
* Do not solve ambiguity by arbitrarily selecting the first candidate.
* Do not special-case individual evaluation repositories.

Those belong to later phases.

---

# Primary Problem

Consider a C++ class:

```cpp
// auth_token.h

namespace auth {

class AuthToken {
public:
    bool validate() const;
};

}
```

and:

```cpp
// auth_token.cpp

bool auth::AuthToken::validate() const {
    ...
}
```

The declaration and definition represent the same logical semantic symbol.

They must not independently cause:

```text
error: symbol is ambiguous
```

for:

```bash
ast-tool callers auth::AuthToken::validate .
```

The resolver should resolve the query to one semantic symbol.

---

# Required Investigation

Before changing behavior, inspect the current implementation and determine:

1. How symbols are extracted from AST IR.
2. How symbol identity is currently constructed.
3. Whether declarations and definitions currently receive different identities.
4. Which properties are used for matching:

   * name
   * fully-qualified name
   * symbol kind
   * namespace
   * class scope
   * parameter types
   * qualifiers
   * return type
   * file
   * source range
5. Where ambiguity is introduced.
6. Whether `callers`, `references`, and `callees` use a common resolver or separate resolution logic.

Prefer fixing shared semantic infrastructure.

Do not patch each command independently if the same resolver is responsible.

---

# Semantic Identity Requirements

Introduce or improve the concept of a logical semantic symbol.

For compatible declaration/definition pairs:

```text
declaration
     │
     ├── semantic identity
     │
definition
```

Both should resolve to the same logical identity.

The identity must not depend only on:

```text
file path
AST node location
source range
parser node ID
```

because declaration and definition naturally differ in those properties.

At minimum, investigate whether C++ symbol identity should include:

```text
fully-qualified name
symbol category
enclosing namespace/class
parameter signature
cv/ref qualifiers where relevant
```

Be conservative.

Do not merge symbols merely because they share the same short name.

---

# Required Cases

At minimum, support and test the following.

## 1. Free function declaration + definition

```cpp
// foo.h
void process(int value);

// foo.cpp
void process(int value) {
}
```

These must resolve to one logical symbol.

---

## 2. Class method declaration + definition

```cpp
class Foo {
public:
    void run();
};
```

```cpp
void Foo::run() {
}
```

These must resolve to one logical symbol.

---

## 3. Namespace-qualified symbol

```cpp
namespace auth {
bool validate(Token token);
}
```

and an out-of-line definition.

Namespace identity must be respected.

---

## 4. Overloads

```cpp
void process(int);
void process(std::string);
```

These must remain distinct semantic symbols.

Do not collapse overloads by fully-qualified name alone.

---

## 5. Method overloads

```cpp
class Foo {
public:
    void run(int);
    void run(std::string);
};
```

These must remain distinct.

---

## 6. Qualifiers

Where the current AST IR provides sufficient information, preserve distinctions such as:

```cpp
void Foo::run();
void Foo::run() const;
```

Do not incorrectly merge semantically distinct methods.

If the current IR cannot reliably represent a qualifier, document the limitation rather than introducing unsafe heuristics.

---

## 7. Multiple translation units

A declaration may be visible in multiple translation units while corresponding to one implementation symbol.

Ensure that symbol resolution does not create false ambiguity merely because the same declaration is encountered from multiple files or translation units.

Do not attempt to implement a complete C++ compiler or linker model.

Only implement enough semantic identity to reliably handle the supported AST Tool use cases.

---

# Resolver Behavior

Given:

```bash
ast-tool callers auth::AuthToken::validate .
```

if the matching declaration and definition correspond to one logical symbol, the resolver should treat them as a single semantic target.

Expected conceptual behavior:

```text
query
  ↓
candidate declarations / definitions
  ↓
semantic identity normalization
  ↓
one logical symbol
  ↓
callers
```

Not:

```text
query
  ↓
header declaration
cpp definition
  ↓
two candidates
  ↓
ambiguous
```

True ambiguity must still remain an error.

For example, if multiple overloads match an insufficiently specific query and cannot be safely distinguished, do not silently select one.

---

# Implementation Guidance

Prefer a layered implementation.

A good direction is:

```text
AST symbol instances
      ↓
semantic symbol key / identity
      ↓
logical symbol grouping
      ↓
resolver
```

Keep source occurrences separate from logical identity.

For example, conceptually:

```text
SemanticSymbol
    identity
    declarations[]
    definitions[]
```

or an equivalent design may be appropriate.

The exact data model should follow the existing architecture rather than forcing this specific structure.

The important invariant is:

> Source locations are occurrences of a symbol, not necessarily separate semantic symbols.

---

# Backward Compatibility

Preserve existing CLI behavior wherever possible.

Existing valid queries must continue to work.

Do not change public output schemas unnecessarily.

Do not change command syntax.

Do not introduce Symbol ID-based public APIs yet.

Internal semantic IDs may be changed or introduced if necessary, but Phase 4 will separately define the stable public Symbol ID workflow.

---

# Tests

Add focused unit and/or integration tests for semantic identity and resolver behavior.

At minimum, add regression coverage for:

```text
free function declaration + definition
class method declaration + definition
namespace-qualified declaration + definition
free-function overloads
method overloads
const/non-const methods where supported
multiple translation units
```

Also add command-level regression tests for:

```text
callers
references
callees
```

where applicable.

A declaration/definition pair must not generate false ambiguity.

An actual overload ambiguity must not be accidentally removed.

---

# Evaluation

After implementation, run the same evaluation suite used for Phase 1 and Phase 2.

Do not modify the evaluation prompts or repositories in order to improve the result.

Collect the same metrics:

```text
tests
successes
failures
success rate

total tool calls
average tool calls per test

ast-tool calls
ast-tool failures
ast-tool failure rate
ast-tool retries
ast-tool help calls

ast-tool failures by command

bash calls
read calls
edit calls
grep calls
glob calls

elapsed time
average elapsed time

input tokens
output tokens
total tokens
average tokens per test

average ast-tool recovery distance
max ast-tool recovery distance
```

Also preserve the AST Tool command sequences where available.

---

# Primary Evaluation Questions

Phase 3 should answer:

1. Did `callers` failure rate decrease substantially?
2. Did `references` failure rate decrease substantially?
3. Did `callees` failure rate decrease substantially?
4. Did AST Tool retries decrease?
5. Did recovery distance decrease?
6. Did overall success rate remain stable or improve?
7. Did total token usage remain stable or improve?
8. Did the agent perform less fallback exploration through `grep`, `read`, `find`, or repeated `search` calls?

Do not optimize specifically for AST Tool call count.

An increase in successful semantic queries is acceptable if total trajectory cost improves.

---

# Acceptance Criteria

The phase is complete when all of the following are true.

### Semantic correctness

A declaration and definition of the same supported C++ function or method resolve to one logical semantic symbol.

Example:

```bash
ast-tool callers auth::AuthToken::validate .
```

must not fail solely because one declaration exists in a header and one definition exists in a source file.

The same applies to:

```bash
ast-tool references auth::AuthToken::validate .
ast-tool callees auth::AuthToken::validate .
```

where semantically applicable.

### Overload safety

Distinct overloads must remain distinct.

Do not resolve ambiguity by merging unrelated overloads.

### Architecture

The fix belongs primarily in shared semantic identity / resolution infrastructure.

Avoid command-specific hacks.

### Regression safety

Existing tests continue to pass, except where an existing test explicitly encoded incorrect declaration/definition ambiguity behavior and must be updated.

### Evaluation

Run the unchanged evaluation suite and provide a before/after comparison against Phase 2.

The most important expected movement is:

```text
ast-tool semantic failures ↓
ast-tool retries           ↓
recovery distance          ↓
```

while maintaining or improving:

```text
success rate
total tokens
elapsed time
```

---

# Deliverables

At the end of the phase, report:

1. Root cause of declaration/definition ambiguity.
2. Files and components changed.
3. Semantic identity strategy implemented.
4. Cases intentionally supported.
5. Known unsupported or ambiguous C++ cases.
6. Tests added.
7. Test results.
8. Phase 2 vs Phase 3 evaluation metrics.
9. Examples of semantic command trajectories before and after the change.

Do not begin Phase 4 work as part of this implementation.
