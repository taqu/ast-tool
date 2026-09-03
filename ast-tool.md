# Phase 5 — Error Recovery UX

## Goal

Improve AST Tool error recovery so that a coding agent can recover from failed semantic queries with fewer exploratory tool calls, fewer retries, and fewer tokens.

This phase must improve recovery behavior without attempting to redesign semantic symbol resolution.

The desired trajectory is:

```text
semantic command
      ↓
failure
      ↓
actionable error
      ↓
one appropriate fallback
```

instead of:

```text
semantic command
      ↓
failure
      ↓
help
find
search
grep
read
retry
...
```

---

# Baseline

Use the accepted Phase 2 implementation as the starting point.

Do NOT use the experimental Phase 3 semantic-resolution implementation.

Phase 2 evaluation:

```text
tests:                       41
successes:                   37
failures:                     4
success rate:                90.24%

total tool calls:            519
average tool calls/test:     12.66

ast-tool calls:               70
ast-tool failures:            36
ast-tool failure rate:       51.43%
ast-tool retries:             23

average recovery distance:    2.23
max recovery distance:        5

grep calls:                   18
read calls:                  254

elapsed time:             2100.56 sec

total tokens:             162,628
average tokens/test:        3,966.5
```

Semantic command failures included:

```text
callers:       21 failures
callees:        9 failures
references:     5 failures
find:           1 failure
```

Despite these failures, the overall task success rate remained 90.24%.

This suggests that fallback behavior works but is unnecessarily expensive.

---

# Background

A previous attempt to improve semantic resolution caused a severe end-to-end regression:

```text
success rate:
90.24% → 60.98%

total tokens:
162,628 → 253,785

ast-tool usage:
70 → 15

grep usage:
18 → 71
```

Therefore:

> Do not attempt to solve semantic identity or declaration/definition unification in this phase.

The objective is to make failures cheap and recoverable.

---

# Scope

Improve error and empty-result behavior for the commands most commonly used by coding agents:

```text
search
callers
references
callees
find
symbols
```

Focus especially on:

```text
ambiguous symbol
symbol not found
no semantic result
invalid query
invalid argument
unknown option
unsupported query form
```

---

# Non-goals

Do NOT implement or modify:

```text
declaration/definition semantic unification
C++ overload resolution architecture
cross-translation-unit semantic identity
stable public Symbol IDs
callers --id
references --id
callees --id
Skill.md
JSON schema redesign
command deletion
command renaming
evaluation prompts
evaluation repositories
```

Do not introduce a new semantic resolver architecture.

Do not make command-specific semantic hacks whose purpose is to increase success rate.

Do not silently choose an ambiguous candidate.

Do not automatically run fallback commands internally.

The tool should provide useful guidance; the agent remains responsible for selecting the next action.

---

# Design Principle

Every failure should answer three questions whenever possible:

```text
1. What failed?
2. What useful information do we already know?
3. What is the cheapest reasonable next action?
```

The response must remain compact.

Avoid verbose tutorials.

This output will be consumed by an LLM, so optimize for:

```text
low token count
clear structure
machine readability
immediate actionability
```

---

# Error Categories

## 1. Symbol Not Found

Current behavior may resemble:

```text
error: symbol not found
```

Prefer something conceptually like:

```text
error: symbol not found: AuthToken::validate

next:
  search "validate"
```

If a qualified query appears too specific, the tool may suggest searching a shorter symbol name.

Example:

```text
error: symbol not found: auth::AuthToken::validate

next:
  ast-tool search validate .
```

Do not automatically execute the search.

---

## 2. Ambiguous Symbol

If multiple candidates exist, return a bounded candidate list.

Example:

```text
error: ambiguous symbol: validate

candidates:
  auth::AuthToken::validate
  auth::Validator::validate

next:
  retry using a fully-qualified name
```

If source locations are already available cheaply:

```text
candidates:
  auth::AuthToken::validate  src/auth/auth_token.cpp:7
  auth::Validator::validate  src/auth/validator.cpp:19
```

Do not dump every candidate in a large workspace.

Use a small bounded number of candidates.

If more exist:

```text
showing 5 of 18 candidates
```

is sufficient.

---

# Important Declaration / Definition Case

Do not attempt to unify a header declaration and source definition in this phase.

If they appear as separate candidates, report them compactly.

For example:

```text
error: ambiguous symbol: auth::AuthToken::validate

candidates:
  src/auth/auth_token.h:12
  src/auth/auth_token.cpp:7

next:
  inspect candidates with search or symbols
```

Do not claim they are the same logical symbol unless the current semantic layer already knows that reliably.

The purpose is recovery, not semantic inference.

---

# 3. Empty Semantic Result

Distinguish:

```text
command succeeded but found no results
```

from:

```text
command failed
```

For example:

```text
no callers found for: auth::AuthToken::validate
```

should not automatically be represented as a generic error if the query itself was valid.

Where useful, provide a short fallback:

```text
next:
  search for direct references
```

But only if that recommendation is semantically reasonable.

Avoid suggesting unrelated commands merely to produce guidance.

---

# 4. Unknown Option

Example:

```text
error: unknown option: --foo

available:
  --json
  --pretty
```

If there is an obvious likely correction, it may be shown.

Do not print the entire command help.

Do not instruct the agent to call `--help` unless necessary.

The goal is specifically to reduce help calls.

---

# 5. Invalid Arguments

Example:

```text
error: missing PATH

usage:
  ast-tool callers SYMBOL PATH
```

Return the minimal valid invocation shape.

Do not dump the complete CLI documentation.

---

# 6. Unsupported Query Form

If a command receives a query type it cannot process, make that explicit.

Example:

```text
error: callers requires a semantic symbol query

next:
  use search to resolve the symbol first
```

Avoid generic parser or internal-error messages where a user-facing classification is available.

---

# 7. Internal Errors

Do not hide real internal failures as user mistakes.

Use a clear distinction such as:

```text
error: internal semantic analysis failure
```

and preserve enough diagnostic information for debugging.

However, keep normal agent-facing output compact.

If the project already has verbose/debug logging, detailed diagnostics should go there rather than into the default response.

---

# Recovery Recommendation Rules

Recommendations should follow a small deterministic decision table.

Prefer rules such as:

```text
symbol not found
    → search

ambiguous short name
    → retry with qualified name

ambiguous qualified name
    → inspect candidate locations / symbols

invalid syntax
    → show minimal valid syntax

unknown option
    → show relevant valid options

valid query with no callers
    → no error; report zero result
```

Do not create a complex planner inside AST Tool.

The tool should recommend at most one primary next action in normal cases.

A second alternative is acceptable only when genuinely useful.

---

# Output Size Constraints

Error UX must not create a new token problem.

Use:

```text
short messages
bounded candidate lists
compact locations
minimal examples
```

Avoid:

```text
full help output
large AST dumps
workspace-wide candidate dumps
long prose explanations
repeated field names
stack traces in normal output
```

Candidate output should have a configurable or fixed conservative upper bound.

A default such as 3–5 candidates is preferable to returning dozens.

Follow existing project conventions where possible.

---

# JSON Behavior

If the command is executed with `--json`, errors should remain structured and compact.

Use the existing JSON architecture if one already exists.

Conceptually, a recoverable error might contain fields equivalent to:

```json
{
  "error": "ambiguous_symbol",
  "query": "validate",
  "candidates": [
    {
      "name": "auth::AuthToken::validate",
      "file": "src/auth/auth_token.cpp",
      "line": 7
    }
  ],
  "next": "retry_with_qualified_name"
}
```

This is an example of intent, not a required schema.

Do not introduce a major incompatible JSON schema change solely to match this example.

Preserve backwards compatibility wherever practical.

---

# Human-readable Output

Plain-text output should remain concise enough for agent consumption.

Prefer:

```text
error: ambiguous symbol: validate
candidates:
  auth::AuthToken::validate  src/auth/auth_token.cpp:7
  auth::Validator::validate  src/auth/validator.cpp:19
next: retry with a fully-qualified name
```

over long explanatory paragraphs.

---

# Implementation Guidance

First identify the existing error-generation paths shared by:

```text
callers
references
callees
search
find
symbols
```

Prefer centralized error classification and rendering where the architecture supports it.

Avoid duplicating recovery-message logic in every command.

A useful conceptual separation is:

```text
operation
   ↓
typed failure/result
   ↓
recovery classification
   ↓
CLI / JSON rendering
```

Do not refactor unrelated architecture merely to achieve this separation.

Use the smallest implementation that produces consistent behavior.

---

# Tests

Add tests for the following categories.

## Symbol not found

Verify:

```text
clear error category
query included
search recommendation where appropriate
no full help dump
```

---

## Ambiguous symbol

Verify:

```text
candidate names included
candidate count is bounded
qualified-name recommendation
no arbitrary candidate selection
```

---

## Large candidate set

Create enough matching symbols to exceed the output bound.

Verify:

```text
only N candidates shown
total candidate count may be indicated
output remains compact
```

---

## Unknown option

Verify:

```text
bad option identified
small relevant option set shown
no entire help text
```

---

## Invalid arguments

Verify that minimal usage information is returned.

---

## Empty successful result

Verify that:

```text
valid query + zero callers
```

is distinguishable from:

```text
failed symbol resolution
```

where the current architecture permits this distinction.

---

## JSON errors

Verify JSON output is:

```text
valid
compact by default
machine-readable
backwards-compatible where required
```

---

# Regression Constraints

The Phase 2 behavior is the regression baseline.

All existing Phase 2 tests should continue to pass except where a test explicitly validates an unhelpful error message that is intentionally being improved.

Do not modify semantic query results merely to satisfy the new tests.

Error UX tests must not require semantic behavior that Phase 2 did not already provide.

---

# Evaluation

Run the unchanged 41-test evaluation suite after implementation.

Collect exactly the same metrics as Phase 2:

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

Also retain per-test command sequences.

---

# Primary Evaluation Metrics

The primary expected improvements are:

```text
ast-tool retries ↓
recovery distance ↓
help calls ↓
fallback exploration ↓
```

Secondary expected improvements:

```text
total tool calls ↓
grep calls ↓
read calls ↓
total tokens ↓
elapsed time ↓
```

The success rate must remain approximately stable or improve.

---

# Acceptance Criteria

## Required

Phase 5 is acceptable only if:

```text
success rate does not materially regress from 90.24%
```

A one-test variation may be investigated individually, but a broad correctness regression is unacceptable.

Additionally, at least one recovery-efficiency metric should improve materially:

```text
AST retries
average recovery distance
max recovery distance
help calls
fallback tool calls
```

without causing substantial token inflation.

---

## Preferred

A strong result would look like:

```text
success rate:              stable or ↑
total tokens:              ↓
AST retries:               ↓
recovery distance:         ↓
grep/read fallback:        ↓
elapsed time:              stable or ↓
```

---

# Stop Conditions

Stop and revert the phase if any of the following occurs:

```text
success rate drops substantially
total tokens increase substantially
grep fallback increases substantially
AST Tool usage collapses unexpectedly
error messages become significantly larger
```

Do not continue modifying unrelated components in an attempt to recover the evaluation within the same phase.

Instead report the regression.

---

# Deliverables

At completion, provide:

1. Summary of the previous error behavior.
2. Error categories introduced or improved.
3. Recovery recommendation rules.
4. Files/components changed.
5. Tests added.
6. Unit/integration test results.
7. Full unchanged evaluation results.
8. Phase 2 vs Phase 5 metric comparison.
9. Representative before/after tool trajectories.
10. Any error categories intentionally left unchanged.
11. Any observed cases where recommendations caused unexpected agent behavior.

Do not begin Phase 6 work.

Do not modify Skill.md as part of this phase.
