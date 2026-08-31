# Phase 2 — Reduce AST Tool Output and JSON Token Cost

## Goal

Reduce the number of tokens consumed by `ast-tool` output without changing semantic behavior.

Phase 1 improved agent command selection through `Skill.md`.

Phase 2 should now improve the CLI output itself so that successful AST Tool usage produces less context for the coding agent to consume.

The primary objectives are:

```text
smaller command output
        ↓
fewer output tokens
        ↓
smaller agent context
        ↓
lower total token usage
```

This phase must preserve correctness and existing semantic behavior.

---

# Phase 1 Results

Phase 1 preserved evaluation correctness:

```text
validation success:
37 / 41
```

Token usage improved:

```text
Before Phase 1:
total tokens = 186155
average tokens/test = 4540.4

After Phase 1:
total tokens = 176983
average tokens/test = 4316.7
```

This is approximately a 4.9% reduction in total tokens.

AST Tool command selection also became significantly narrower.

Examples:

```text
callers:
51 → 23

find:
27 → 9

references:
16 → 4

callees:
16 → 3

search:
37 → 22
```

This suggests that Phase 1 successfully reduced unnecessary semantic exploration.

Phase 2 should preserve those gains and focus specifically on reducing the cost of each AST Tool response.

---

# Important: Re-establish the Phase 1 Baseline First

Before modifying AST Tool output behavior, regenerate the Phase 1 analysis from the current evaluation result files.

There appears to be a possible inconsistency between the Phase 1 command table and previously generated aggregate statistics.

For example, the command table indicates substantially fewer AST Tool calls, while some aggregate values appear unchanged from the pre-Phase-1 run.

Do not modify metrics to make them agree manually.

Instead:

1. Run the current `analyze_results.py` against the actual Phase 1 result files.
2. Regenerate all aggregate metrics.
3. Save this output as the authoritative Phase 1 baseline.
4. Record the exact result directory/input used.
5. Use that same evaluation configuration for the Phase 2 comparison.

Do not change evaluation prompts, repositories, model configuration, or test selection.

---

# Scope

Phase 2 is limited to AST Tool output efficiency.

Focus on:

* JSON output formatting
* default textual output
* unnecessary fields
* unnecessarily broad result sets
* unnecessarily verbose error-independent successful output

Do NOT change semantic resolution in this phase.

Do NOT fix declaration/definition ambiguity yet.

That belongs to Phase 3.

---

# 1. Preserve Plain Text as the Preferred Agent Output

Plain-text output should remain concise and optimized for coding-agent consumption.

For semantic commands, prefer one result per line.

Examples:

```text
auth::AuthToken::validate src/auth/auth_token.h:9:5
```

or:

```text
web::AuthController::handleLogin src/web/auth_controller.cpp:13:9
```

Avoid explanatory prose around successful results.

Do not print headers such as:

```text
Results:
Found symbols:
Search results:
```

unless they materially improve interpretation.

The agent already knows which command it executed.

---

# 2. Keep `--json` Compact by Default

`--json` should emit compact JSON.

Example:

```json
[{"kind":"Method","fqn":"auth::AuthToken::validate","file":"src/auth/auth_token.h","line":9,"column":5}]
```

Do not insert indentation or unnecessary whitespace.

Pretty printing should occur only when explicitly requested with:

```text
--pretty
```

If the current implementation already behaves this way, preserve it and verify it with tests.

---

# 3. `--pretty` Must Remain Explicit

Do not automatically enable pretty output when `--json` is used.

These should have clearly different behavior:

```text
--json
```

→ compact machine-readable output

```text
--json --pretty
```

→ formatted human-readable output

Phase 1 already instructs agents to avoid `--pretty` by default.

Phase 2 should make compact JSON genuinely inexpensive enough that structured output remains viable when needed.

---

# 4. Audit JSON Fields for Agent Value

Review JSON output for the core semantic commands:

```text
search
symbols
callers
references
callees
```

Identify fields that are always emitted but rarely useful to coding agents.

For example, current symbol output may contain values such as:

```text
access
static
constexpr
inline
owning_scope
```

Do not remove fields blindly.

For each field determine:

1. Is it required by existing tests or consumers?
2. Is it required for semantic identification?
3. Is it useful to the agent in the common navigation workflow?
4. Can it be omitted from a compact/default representation without breaking compatibility?

Prefer backward compatibility.

If removing existing JSON fields would break the documented schema or likely external consumers, do not remove them in this phase.

Instead document the finding for a future schema/versioning change.

---

# 5. Do Not Introduce a Breaking JSON Schema Change

Existing JSON consumers may depend on the current fields.

Therefore:

* do not rename existing fields
* do not change their meaning
* do not change arrays into objects or vice versa
* do not remove documented fields without explicit compatibility handling

Token reduction is not worth silently breaking the CLI API.

If the existing JSON schema is intrinsically too verbose, document that as a finding rather than performing an incompatible redesign.

---

# 6. Prevent Accidental Workspace-Wide Output Explosion

A major token-cost pattern is broad commands such as:

```bash
ast-tool search --json --pretty .
```

which can return every symbol in the workspace.

Phase 1 discourages this behavior, but the CLI should also be reasonably safe.

Inspect the current `search` behavior.

If `search` without a filter means "return all symbols", determine whether a safe result limit can be introduced without breaking existing documented behavior.

Possible approaches include:

```text
default result limit
```

or:

```text
explicit --all for unrestricted enumeration
```

However, compatibility is important.

Do NOT implement a breaking behavior change merely to reduce tokens.

If changing default enumeration would break existing usage, defer the behavior change and document it.

---

# 7. Prefer Narrow Search Results

When filters such as:

```text
--name
```

are supplied, ensure `search` only returns matching symbols and does not include unrelated workspace information.

Example:

```bash
ast-tool search --name validate .
```

should produce only relevant matching symbols.

It should not require the agent to process unrelated symbol records.

Review this path carefully because it is part of the preferred Phase 1 workflow:

```text
search
  ↓
exact symbol
  ↓
callers / references / callees
```

---

# 8. Minimize Duplicate Semantic Information

Inspect output for cases where the same information is repeated.

For example, avoid unnecessarily emitting both:

```text
name: AuthToken::validate
fqn: auth::AuthToken::validate
qualified_name: auth::AuthToken::validate
```

unless those fields are intentionally part of a stable API.

Likewise, avoid repeating file/line information in multiple textual forms.

Again, do not break existing JSON compatibility.

Prioritize eliminating duplication in plain-text output first.

---

# 9. Keep Error Handling Out of Scope

Do not redesign error messages in this phase.

Although verbose or non-actionable errors can increase token usage, error recovery is a later phase.

In particular, do NOT yet implement:

* suggested retry commands
* semantic candidate IDs in errors
* automatic ambiguity recovery
* declaration/definition unification
* resolver changes

Those belong to later phases.

Phase 2 should isolate the effect of successful-output reduction.

---

# 10. Measure Actual Output Size

Add lightweight measurements to tests or evaluation analysis so that output reduction is measurable.

For each AST Tool invocation, if the trace already contains command output, derive at least:

```text
ast_tool_output_bytes
```

and preferably:

```text
ast_tool_output_chars
```

per invocation and in aggregate.

Do not introduce tokenizer dependencies.

Character/byte counts are sufficient for Phase 2 output-volume analysis.

If convenient, aggregate by command:

```json
{
  "ast_tool_output_by_command": {
    "search": {
      "calls": 22,
      "bytes": 18420
    },
    "callers": {
      "calls": 23,
      "bytes": 3280
    }
  }
}
```

This will help determine which subcommands generate the most context.

---

# 11. Measure JSON and Pretty JSON Separately

Using the Phase 0 analyzer infrastructure, report output volume for:

```text
plain AST Tool calls
JSON AST Tool calls
pretty JSON AST Tool calls
```

Suggested aggregate metrics:

```text
ast_tool_plain_calls
ast_tool_json_calls
ast_tool_pretty_json_calls

ast_tool_plain_output_bytes
ast_tool_json_output_bytes
ast_tool_pretty_json_output_bytes
```

If the analyzer cannot reliably distinguish these today, add the smallest necessary extension.

Do not turn this into a general logging-framework redesign.

---

# 12. Core Commands to Audit

Prioritize:

```text
search
callers
references
callees
symbols
```

These are the primary semantic-agent interface.

Secondary commands:

```text
find
outline
```

may also be reviewed if their output is clearly excessive.

Low-level AST commands do not require optimization unless a trivial improvement is obvious.

---

# 13. Tests

Add or update tests for output behavior.

At minimum cover the following.

## Compact JSON

Command:

```text
search --json
```

Verify that output contains no pretty-print indentation/newlines beyond what is structurally necessary.

---

## Pretty JSON

Command:

```text
search --json --pretty
```

Verify that formatted JSON is still available.

---

## Plain Search

Command:

```text
search --name validate
```

Verify that the output contains only matching results and remains concise.

---

## Callers Plain Output

Verify that each caller is represented compactly without unnecessary descriptive text.

---

## References Plain Output

Verify concise one-reference-per-line output.

---

## Callees Plain Output

Verify concise output.

---

## Symbols Plain Output

Verify concise symbol + identifier output.

---

## JSON Compatibility

Existing JSON-format tests must continue to pass.

If documented JSON fields are retained, explicitly test that they are still present.

---

# 14. Evaluation Procedure

After implementation, rerun exactly the same 41 evaluation tests used for Phase 1.

Do not modify:

* task YAML
* repositories
* validation scripts
* Skill.md
* agent prompt
* AST Tool semantic behavior

Compare Phase 1 vs Phase 2.

At minimum report:

```text
validation success

total tokens
average tokens per test

total tool calls
average tool calls per test

ast_tool_calls
ast_tool_failures
ast_tool_retries

ast_tool_json_calls
ast_tool_pretty_json_calls

total AST Tool output bytes
average AST Tool output bytes per call

output bytes by AST Tool command

elapsed time
```

---

# 15. Primary Success Criteria

Phase 2 should be considered successful if:

```text
validation success does not regress
```

and at least one of:

```text
total tokens decreases
AST Tool output volume decreases
average tokens per test decreases
```

shows a meaningful improvement.

The most important metric is:

```text
total tokens
```

not merely JSON byte count.

Reducing output bytes is useful only if it improves or preserves actual agent efficiency.

---

# 16. Watch for Compensating Behavior

A smaller AST Tool response is not automatically better.

For example:

```text
Before:
search → enough information → edit
```

could become:

```text
After:
search → insufficient information
      → search
      → symbols
      → Read
      → edit
```

This would reduce per-command output while increasing total exploration.

Therefore compare:

```text
tool_sequence
ast_tool_sequence
total_tool_calls
total_tokens
```

as well as raw output size.

Do not optimize an individual command at the expense of the overall trajectory.

---

# Non-Goals

Do NOT implement:

* Skill.md changes
* semantic resolver changes
* declaration/definition unification
* `callers` ambiguity fixes
* `references` ambiguity fixes
* symbol-ID-based lookup
* new semantic symbol identity
* actionable error recommendations
* subcommand removal
* CLI restructuring
* cache performance work
* workspace analysis changes
* evaluation task changes

Do not proceed to Phase 3.

---

# Implementation Strategy

Prefer small, reviewable changes.

Recommended order:

```text
1. Regenerate authoritative Phase 1 baseline
2. Audit current output behavior
3. Measure output size
4. Verify compact JSON behavior
5. Reduce obvious plain-text verbosity
6. Reduce safe redundant output
7. Preserve JSON compatibility
8. Run unit tests
9. Run the same 41 evaluation tests
10. Compare Phase 1 vs Phase 2
```

Avoid introducing a generalized output-formatting framework unless the project already has one that should be reused.

---

# Acceptance Criteria

Phase 2 is complete when:

1. The authoritative Phase 1 baseline has been regenerated from the actual result files.
2. Any discrepancy in the previous Phase 1 aggregate report has been identified or documented.
3. Core semantic command output has been audited.
4. `--json` output is compact by default.
5. `--pretty` remains explicit.
6. Plain-text output remains concise.
7. Narrow `search` queries do not return unrelated workspace symbols.
8. Existing JSON compatibility is preserved.
9. AST Tool output volume can be measured.
10. Output volume can be compared by command.
11. Plain/JSON/pretty-JSON usage can be distinguished where practical.
12. Existing unit tests continue to pass.
13. New output-specific tests are added where necessary.
14. The same 41 evaluation tests are rerun.
15. Validation success does not regress unexpectedly.
16. Phase 1 vs Phase 2 token usage is reported.
17. Phase 1 vs Phase 2 AST Tool output volume is reported.
18. Any proposed breaking output/schema changes are documented but not implemented.

---

# Deliverables

Provide:

1. The authoritative regenerated Phase 1 baseline.
2. A short explanation of the apparent previous aggregate-statistics discrepancy, if identified.
3. The Phase 2 implementation.
4. Updated/new tests.
5. Phase 2 evaluation results for the same 41 tests.
6. A Phase 1 vs Phase 2 comparison table.
7. AST Tool output-size statistics by command.
8. Token usage comparison.
9. At least one representative before/after command output.
10. Any findings that should be addressed in Phase 3 rather than this phase.

Do not proceed to Phase 3.
