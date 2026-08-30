# Phase 1 — Improve `Skill.md` for Agent Tool Selection

## Goal

Improve the `semantic-analysis` Skill so that coding agents use `ast-tool` more efficiently and with less trial-and-error.

This phase should focus only on **agent guidance**.

Do not change:

* `ast-tool` implementation
* semantic resolution
* CLI behavior
* subcommands
* JSON output format
* error messages
* cache behavior
* evaluation task definitions

The goal is to determine how much improvement can be achieved through better tool-selection guidance alone.

---

# Background

Existing evaluation traces show that agents often enter inefficient exploration loops.

A common pattern looks like:

```text
callers
  ↓ failure

search
  ↓

callers
  ↓ failure

find
  ↓ failure

search --json --pretty
  ↓

references
  ↓ failure

help
  ↓

symbols
  ↓

grep
  ↓

Read
  ↓

Edit
```

Several recurring behaviors have been observed:

1. `callers` is often attempted before the exact semantic symbol is known.
2. Failed commands are retried several times with only minor argument changes.
3. `find` is used as a fallback for semantic symbol resolution.
4. Agents frequently call `--help`.
5. Agents tend to prefer `--json --pretty`.
6. Agents sometimes dump the entire workspace with broad `search` commands.
7. Generic tools such as Grep are eventually used after several failed AST Tool attempts.

The Skill should provide a much clearer default workflow.

---

# Scope

Update the existing `semantic-analysis` `Skill.md`.

The Skill should become primarily a **decision guide for tool selection**, not a long reference manual.

The Skill should help an agent answer:

```text
What kind of question am I trying to answer?
        ↓
Which ast-tool command should I use first?
        ↓
What should I do if that command fails?
```

Keep the Skill concise.

The objective is not to duplicate `ast-tool --help`.

---

# Core Design Principle

The Skill should make the common semantic workflow obvious:

```text
search
  ↓
identify exact symbol
  ↓
callers / references / callees
  ↓
Read only relevant files
  ↓
Edit
```

For many tasks, this should be the preferred trajectory.

The Skill should explicitly distinguish:

```text
Semantic analysis
```

from:

```text
AST structure inspection
```

These should not be mixed casually.

---

# Required Decision Tree

Add a short decision tree near the beginning of the Skill.

Use guidance equivalent to:

```text
Need to locate a symbol?
→ search

Need all direct callers of a function?
→ callers

Need semantic references to a symbol?
→ references

Need functions called by a function?
→ callees

Need semantic symbols from one known file?
→ symbols

Need to inspect AST structure or syntax nodes?
→ find / outline

Need parent/child AST relationships?
→ parent / children / range
```

The exact wording may be adjusted to match the existing Skill style.

The important requirement is that agents can determine the correct first command quickly.

---

# Recommended Semantic Workflow

Document the preferred workflow for symbol-based tasks.

Example:

```text
1. Use `search` to identify the target symbol when its exact identity is uncertain.
2. Use the fully-qualified symbol name returned by `search`.
3. Use `callers`, `references`, or `callees` depending on the task.
4. Read only the files returned by semantic analysis.
5. Make the required edits.
```

Example:

```bash
ast-tool search --name validate .
ast-tool callers auth::AuthToken::validate .
```

Avoid adding many examples.

One or two compact examples are sufficient.

---

# Guidance for `callers`

Add explicit guidance that `callers` should normally be used after the target symbol is known.

Prefer:

```text
search
  ↓
callers
```

when the symbol name may be ambiguous.

Do not encourage repeated `callers` invocations with guessed symbol forms.

If `callers` fails because the symbol cannot be resolved or is ambiguous:

```text
Use `search` or `symbols` to identify the exact target.
Do not repeatedly retry `callers` with minor command-line variations.
```

This is important.

---

# Guidance for `references`

Clarify that `references` performs semantic reference lookup.

Use it when the task asks for:

* usages
* references
* semantic occurrences
* symbol impact analysis

Do not recommend `find --text` as the primary substitute for semantic references.

If semantic resolution fails, prefer:

```text
search
```

or:

```text
symbols <known-file>
```

before falling back to textual tools.

---

# Guidance for `find`

The Skill must explicitly define `find` as an AST-inspection tool.

Add guidance equivalent to:

```text
Use `find` when you need AST nodes, syntax structure, node types, text matches within AST nodes, or node IDs.

Do not use `find` as the default tool for semantic symbol resolution.
```

This distinction should be very clear.

For example:

```text
Find the declaration of a semantic symbol
→ search / symbols

Inspect a function_definition AST node
→ find
```

---

# Guidance for `symbols`

Explain the intended role of `symbols`.

Use `symbols` when:

```text
The relevant file is already known and you need the semantic symbols defined or declared in that file.
```

It is especially useful as a narrow fallback after `search` identifies a likely file.

Do not recommend workspace-wide `symbols` behavior because `symbols` is file-oriented.

---

# Avoid Repeated Failed Commands

Add a clear retry rule.

For example:

```text
If an ast-tool command fails, do not repeat the same subcommand with small argument variations unless the error message clearly indicates the required correction.
```

Prefer:

```text
failed semantic query
  ↓
search / symbols
  ↓
identify exact target
  ↓
retry once
```

Avoid:

```text
callers
callers
callers
callers
```

with guessed inputs.

The Skill should encourage strategy changes after failure, not blind retries.

---

# Help Usage

Reduce unnecessary `--help` calls.

The Skill should contain enough command-selection information that ordinary semantic tasks do not require help lookup.

Add guidance equivalent to:

```text
Do not call `ast-tool --help` or `<command> --help` for normal command discovery.

Use help only when:
- a command option or syntax is genuinely unknown, or
- an error indicates that the command was invoked incorrectly.
```

Do not completely forbid help.

It should remain available as a fallback.

---

# JSON Output Guidance

Agents currently tend to prefer:

```bash
--json --pretty
```

even when plain output is sufficient.

Add explicit guidance:

```text
Prefer plain-text output for normal interactive exploration.

Use `--json` only when structured fields are required.

Avoid `--pretty` unless human-readable JSON formatting is specifically useful.
```

Examples where plain output is preferred:

```bash
ast-tool search --name validate .
ast-tool callers auth::AuthToken::validate .
```

Structured JSON may be useful when:

* stable IDs are required
* multiple fields must be processed programmatically
* the next step requires machine-readable output

Do not state that JSON is forbidden.

The goal is to avoid unnecessary token-heavy output.

---

# Avoid Workspace-Wide Dumps

Add guidance against broad workspace enumeration unless it is genuinely needed.

Avoid patterns such as:

```bash
ast-tool search --json --pretty .
```

when the task is about one symbol.

Prefer a narrow query:

```bash
ast-tool search --name validate .
```

or equivalent filters already supported by the CLI.

The Skill should communicate:

```text
Query narrowly first.
Expand scope only when necessary.
```

This should apply generally, not only to `search`.

---

# Prefer Semantic Tools Before Grep

For semantic questions, the Skill should encourage AST Tool usage before generic text search.

Examples:

```text
"Who calls this function?"
→ callers

"Where is this symbol referenced?"
→ references

"Where is this symbol defined?"
→ search / symbols
```

Grep remains a valid fallback when:

* semantic resolution is unsupported
* AST Tool fails after a reasonable recovery attempt
* the task is explicitly textual rather than semantic

Do not ban Grep.

The goal is to avoid using it before the semantic tools have had a reasonable opportunity to answer the question.

---

# Suggested Skill Structure

Keep the Skill compact.

A recommended structure is:

```text
# Semantic Analysis with ast-tool

## Use ast-tool when

## Command Decision Tree

## Preferred Workflow

## Failure Recovery

## Output Guidelines

## Command Reference

## Examples
```

The `Command Reference` section should be short.

For example:

```text
search      Find semantic symbols across the workspace.
symbols     List semantic symbols in a known file.
callers     Find direct callers.
callees     Find direct callees.
references  Find semantic references.
find        Inspect AST nodes and syntax structure.
outline     Show file structure.
```

Do not reproduce the full CLI help text.

Low-level commands such as:

```text
parent
children
range
```

may be mentioned briefly under AST inspection but do not need extensive documentation.

---

# Recommended Guidance Example

The final Skill should communicate a workflow approximately like this:

```text
For semantic code navigation, prefer ast-tool before Grep.

Start narrow.

Symbol lookup:
  ast-tool search --name <name> <root>

Known file:
  ast-tool symbols <file>

Direct callers:
  ast-tool callers <fqn> <root>

References:
  ast-tool references <fqn> <root>

Callees:
  ast-tool callees <fqn> <root>

AST structure:
  ast-tool find ...
  ast-tool outline ...

Typical workflow:

  search
    ↓
  exact symbol
    ↓
  callers / references / callees
    ↓
  Read relevant files
    ↓
  Edit

If a semantic command fails:
- do not repeatedly retry it with guessed arguments
- use search or symbols to identify the target
- retry after resolving the ambiguity

Prefer plain output.
Use --json only when structured output is needed.
Avoid --pretty by default.
Avoid workspace-wide dumps.
Use --help only as a fallback.
```

Do not copy this section mechanically if the existing Skill has a better structure.

Preserve useful existing content while making the decision path substantially clearer.

---

# Preserve Useful Existing Guidance

Review the existing `Skill.md` before editing it.

Do not blindly replace the entire file.

Preserve existing guidance that is:

* correct
* concise
* useful to coding agents
* not redundant with the new decision tree

Remove or shorten content that:

* duplicates CLI help
* over-explains implementation details
* describes parser internals that agents do not need
* makes command selection harder
* encourages broad output unnecessarily

The Skill should describe how to use the semantic interface, not how Tree-sitter or AST internals work.

---

# Token Efficiency

One explicit objective of this phase is reducing total token usage.

Optimize the Skill itself for low token cost.

Avoid:

* long prose explanations
* repeated command descriptions
* large JSON examples
* exhaustive option listings
* internal architecture descriptions
* multiple examples of the same workflow

Prefer:

```text
short rules
small decision tables
compact examples
clear fallback behavior
```

A shorter Skill that reliably directs the agent is preferable to a comprehensive manual.

---

# Evaluation

Use the Phase 0 metrics to evaluate Phase 1.

Run the same evaluation cases used for the baseline.

Do not change the test prompts or repositories.

Compare at least:

```text
success rate
total tool calls
ast_tool_calls
ast_tool_failures
ast_tool_retries
ast_tool_help_calls
ast_tool_json_calls
ast_tool_pretty_json_calls
grep_calls
read_calls
elapsed_seconds
input_tokens
output_tokens
total_tokens
```

Also inspect:

```text
tool_sequence
ast_tool_sequence
recovery distance
```

where available.

---

# Expected Behavioral Improvements

The expected trajectory should move from patterns like:

```text
callers
  ↓ fail
search
  ↓
callers
  ↓ fail
find
  ↓
help
  ↓
symbols
  ↓
grep
```

toward:

```text
search
  ↓
callers
  ↓
Read
  ↓
Edit
```

For a known unambiguous symbol, an even shorter path may be valid:

```text
callers
  ↓
Read
  ↓
Edit
```

Do not force extra `search` calls when the exact symbol is already known.

The Skill should reduce unnecessary work, not enforce a rigid sequence.

---

# Acceptance Criteria

Phase 1 is complete when:

1. The existing `semantic-analysis` Skill has been updated.
2. The Skill contains a clear command decision tree.
3. Semantic commands and AST-inspection commands are clearly separated.
4. `search` is recommended for uncertain symbol identity.
5. `callers`, `references`, and `callees` are recommended for their corresponding semantic questions.
6. `symbols` is documented as a file-scoped semantic inspection tool.
7. `find` is documented primarily as an AST-inspection tool.
8. Repeated retries after failed commands are explicitly discouraged.
9. `--help` is described as a fallback rather than a normal workflow step.
10. Plain output is preferred for normal exploration.
11. `--json` is used only when structured output is useful.
12. `--pretty` is discouraged by default.
13. Narrow queries are preferred over workspace-wide dumps.
14. Existing useful Skill guidance is preserved where appropriate.
15. No `ast-tool` implementation or CLI behavior is changed.
16. Existing evaluation tests are rerun without changing their prompts.
17. Phase 0 baseline metrics and Phase 1 metrics are compared.
18. The implementation reports whether help calls, retries, broad JSON usage, tool calls, and total tokens improved or regressed.

---

# Non-Goals

Do NOT implement any of the following in Phase 1:

* declaration/definition symbol unification
* fixes to `callers`
* fixes to `references`
* fixes to `callees`
* symbol-ID-based queries
* CLI argument changes
* CLI command removal
* CLI command renaming
* compact JSON implementation
* JSON field removal
* output limiting
* error-message changes
* automatic command recommendations
* cache changes
* workspace performance changes
* evaluation task changes

Do not modify `ast-tool` behavior to make the Phase 1 metrics look better.

This phase must isolate the effect of better agent instructions.

---

# Deliverables

Provide:

1. The updated `Skill.md`.
2. A concise summary of the changes made.
3. The evaluation results using the same tests as the Phase 0 baseline.
4. A comparison of Phase 0 vs Phase 1 for the key metrics.
5. At least one before/after tool trajectory if available.
6. Any observed cases where the Skill guidance is insufficient because the underlying CLI or semantic resolver prevents an efficient workflow.

Do not proceed to Phase 2.

If evaluation reveals problems that require CLI or semantic changes, document them as findings for later phases rather than implementing them in this phase.
