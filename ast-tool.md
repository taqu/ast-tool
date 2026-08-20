# Reorganize the Agent-Facing Skills

## Goal

Reorganize the existing `skills/` directory into a smaller, clearer set of agent-facing skills.

Do not delete all existing skills and recreate them blindly.

Instead:

1. Inspect all existing skills.
2. Identify useful content and workflows.
3. Classify existing content as:
   - keep
   - move
   - merge
   - remove
4. Consolidate the useful content into the new target structure.
5. Remove obsolete skills only after their useful content has been preserved or intentionally discarded.
6. Verify that every command referenced by the resulting skills is part of the intended agent-facing CLI surface.

The goal is to reduce skill-selection ambiguity for coding agents while preserving useful existing guidance.

---

# Current Structure

The current structure is approximately:

```text
skills
├── api-review
├── ast-inspection
├── context-export
├── semantic-analysis
└── workspace-analysis
````

Inspect the actual repository contents rather than assuming that the existing files exactly match this structure.

Each existing `SKILL.md` may contain useful workflows, command guidance, or architectural constraints that should be preserved.

---

# Target Structure

The final agent-facing structure should be:

```text
skills
├── semantic-analysis
│   └── SKILL.md
│
├── ast-inspection
│   └── SKILL.md
│
└── api-review
    └── SKILL.md
```

The intended responsibilities are described below.

---

# 1. `semantic-analysis`

This skill is the primary entry point for understanding semantic relationships in a workspace.

It should cover:

```text
ast-tool search
ast-tool find
ast-tool references
ast-tool callers
ast-tool callees
```

The skill should help an agent choose the correct command based on intent.

Conceptually:

```text
Need to discover matching symbols
        ↓
      search

Need to identify a symbol
        ↓
       find

Need to find usages of a symbol
        ↓
    references

Need to know who directly calls a function
        ↓
      callers

Need to know what a function directly calls
        ↓
      callees
```

The skill should clearly distinguish these operations.

Do not include commands that are not part of the semantic-analysis workflow unless there is a strong, demonstrated reason.

---

# 2. `ast-inspection`

This skill is for structural inspection of source code and AST-derived information.

It should primarily cover:

```text
ast-tool symbols
ast-tool outline
ast-tool parent
ast-tool children
```

The intended conceptual roles are:

```text
symbols
    → inspect symbols in a source file

outline
    → inspect the structural outline of a source file

parent
    → inspect the parent relationship of an AST-derived element

children
    → inspect child relationships of an AST-derived element
```

Adapt these descriptions to the actual behavior of the commands.

This skill should not duplicate the semantic-analysis workflow.

Use it when structural or syntactic inspection is needed rather than semantic relationship analysis.

---

# 3. `api-review`

This skill should describe a higher-level workflow for reviewing or changing an API.

It does not need to introduce a separate set of commands.

Instead, it should compose the existing semantic-analysis commands.

A typical workflow may be:

```text
1. Locate the target API
        ↓
      find

2. Inspect usages
        ↓
   references

3. Inspect incoming dependencies
        ↓
      callers

4. Inspect outgoing dependencies
        ↓
      callees
```

Use `search` when the exact symbol is not known.

Use structural inspection commands only when they provide information required for the review.

The purpose of this skill is workflow guidance, not command duplication.

Avoid copying the entire contents of `semantic-analysis/SKILL.md`.

Instead, describe when and why the commands should be combined during an API review.

---

# 4. Existing `context-export`

Inspect the current `context-export` skill.

Determine whether it contains useful workflow guidance that should be moved into:

```text
semantic-analysis
```

or:

```text
api-review
```

The current target CLI surface does not include a dedicated agent-facing `context` command.

Therefore, do not preserve `context-export` as a separate agent-facing skill merely because a corresponding internal Semantic Service exists.

If useful content describes combining:

```text
references
callers
callees
```

then move that guidance into the appropriate remaining skill.

After useful content has been migrated or intentionally discarded, remove:

```text
skills/context-export
```

---

# 5. Existing `workspace-analysis`

Inspect the current `workspace-analysis` skill.

Determine whether it contains:

* useful agent workflows
* command usage guidance
* architectural background
* implementation details only

The current target agent-facing CLI does not expose a dedicated workspace-analysis workflow.

Do not preserve `workspace-analysis` as a separate skill if it mainly explains internal infrastructure.

Useful guidance may be moved into:

```text
semantic-analysis
```

if it affects how agents should use workspace-wide commands.

Architectural details that do not affect agent behavior should not be copied into the remaining skills.

After useful content has been migrated or intentionally discarded, remove:

```text
skills/workspace-analysis
```

---

# 6. Command Classification

Use the following intended classification.

## Core semantic commands

```text
search
find
references
callers
callees
```

These should be documented in:

```text
skills/semantic-analysis/SKILL.md
```

---

## Structural inspection commands

```text
symbols
outline
parent
children
```

These should be documented in:

```text
skills/ast-inspection/SKILL.md
```

---

## Advanced or non-default commands

Commands such as:

```text
range
dump
```

should not appear in the normal agent workflows unless inspection of the existing skills demonstrates a concrete and necessary use case.

Do not delete these CLI commands.

The goal is to remove them from the default agent-facing guidance, not necessarily from the binary.

If they need to be mentioned at all, treat them as advanced or internal functionality.

Do not encourage agents to use them before the higher-level semantic or structural commands.

---

# 7. Avoid Skill Duplication

The final skills must have clearly separated responsibilities.

Avoid this:

```text
semantic-analysis
    explains references, callers, callees

api-review
    repeats references, callers, callees in full

ast-inspection
    also explains find and references
```

Prefer this:

```text
semantic-analysis
    command capabilities and command selection

ast-inspection
    structural inspection capabilities

api-review
    workflow combining the appropriate skills and commands
```

`api-review` should reference or compose semantic-analysis concepts rather than duplicating the entire command documentation.

---

# 8. Keep Skills Agent-Oriented

The skills are intended for coding agents.

Each `SKILL.md` should help answer:

```text
When should I use this skill?
What information can I obtain?
Which ast-tool command should I run?
What should I do with the result?
```

Avoid unnecessary internal details about:

* Tree-sitter internals
* AST IR implementation details
* parser architecture
* extractor implementation
* Workspace internal data structures

unless such information directly changes the agent's command-selection behavior.

The agent should interact with `ast-tool` through stable semantic and inspection commands rather than parser internals.

---

# 9. Do Not Document Unsupported Commands or Syntax

Before writing or modifying examples:

1. Inspect the actual CLI implementation.
2. Inspect the current help output where possible.
3. Verify the supported command syntax.
4. Use only commands and arguments that actually exist.

Do not invent:

```text
ast-tool context
ast-tool definition
ast-tool dependencies
ast-tool deps
JSON output
file filters
persistent sessions
```

unless those features already exist in the repository.

Do not document future ideas as current capabilities.

---

# 10. Preserve Useful Existing Content

Before removing any existing skill:

1. Read its `SKILL.md`.
2. Identify useful guidance.
3. Decide explicitly whether each important section should be:

   * preserved
   * moved
   * merged
   * discarded
4. Only then remove the obsolete skill directory.

Do not delete existing skill content before completing this review.

The desired process is:

```text
Existing skills
      │
      ▼
Audit contents
      │
      ├── Keep
      ├── Move
      ├── Merge
      └── Remove
      │
      ▼
Rewrite target skills
      │
      ▼
Remove obsolete directories
```

Do not use:

```text
Delete everything
      ↓
Recreate from memory
```

as the strategy.

---

# 11. Recommended Workflow for This Task

Perform the work in the following order.

## Step 1 — Audit

Inspect:

```text
skills/api-review
skills/ast-inspection
skills/context-export
skills/semantic-analysis
skills/workspace-analysis
```

Identify:

* purpose
* referenced commands
* duplicated guidance
* obsolete guidance
* useful workflows

Do not modify files until the audit is complete.

---

## Step 2 — Create a Migration Plan

Before making changes, determine:

```text
Existing content
        ↓
Keep / Move / Merge / Remove
        ↓
Target SKILL.md
```

The plan does not need to become a permanent repository document unless the project already uses such planning artifacts.

The important requirement is to perform the classification deliberately rather than deleting files blindly.

---

## Step 3 — Rewrite `semantic-analysis`

Make it the canonical command-selection guide for:

```text
search
find
references
callers
callees
```

Avoid duplicating structural inspection details.

---

## Step 4 — Rewrite `ast-inspection`

Make it the canonical guide for:

```text
symbols
outline
parent
children
```

Keep the focus on structural inspection.

Do not make `dump` or `range` part of the normal workflow unless existing agent usage clearly requires them.

---

## Step 5 — Rewrite `api-review`

Make it a workflow-oriented skill.

It should explain how to combine semantic inspection operations when reviewing or modifying APIs.

Do not duplicate all command reference material from `semantic-analysis`.

---

## Step 6 — Remove Obsolete Skills

After useful content has been migrated:

```text
skills/context-export
skills/workspace-analysis
```

should be removed if they are no longer needed as independent agent-facing skills.

Do not leave empty compatibility directories.

Do not leave obsolete references to them in indexes, documentation, or skill registries.

---

## Step 7 — Verify References

Search the repository for references to:

```text
context-export
workspace-analysis
```

Update or remove stale references.

Also verify that the final skills only reference intended commands.

The expected primary command set is:

```text
Semantic:
  search
  find
  references
  callers
  callees

Structural:
  symbols
  outline
  parent
  children
```

---

# 12. Examples

Examples should be realistic but concise.

Do not create a long tutorial inside each skill.

Examples should demonstrate command selection.

For example:

```text
Need to understand a symbol:

1. If the exact symbol is unknown:
   ast-tool search <query>

2. Locate the target:
   ast-tool find <symbol>

3. Inspect usages:
   ast-tool references <symbol>
```

For call relationships:

```text
Who calls this function?
  ast-tool callers <symbol>

What does this function call?
  ast-tool callees <symbol>
```

Use only syntax confirmed to be supported by the actual CLI.

---

# 13. Validation

After the reorganization:

Verify that:

```text
skills/
├── semantic-analysis/
│   └── SKILL.md
├── ast-inspection/
│   └── SKILL.md
└── api-review/
    └── SKILL.md
```

is the final agent-facing skill structure.

Verify that:

```text
context-export
workspace-analysis
```

have either been removed or explicitly retained only if the repository structure proves they are still required for another purpose.

Verify that:

* no remaining skill references deleted skills
* no skill documents unsupported commands
* no skill unnecessarily recommends `dump`
* `range` is not part of the default workflow unless justified by actual usage
* command responsibilities are clearly separated
* `api-review` does not duplicate the full semantic command reference

---

# 14. Out of Scope

Do not:

* implement new CLI commands
* remove existing CLI commands
* modify Semantic Services
* modify Workspace Analysis
* redesign AST IR
* add a persistent Workspace
* add a `context` command
* add dependency commands
* introduce JSON output
* redesign CLI output formats

This task is limited to reorganizing agent-facing skills and documentation.

---

# Expected Result

The final skill structure should be:

```text
skills
├── semantic-analysis
│   └── Symbol discovery and semantic relationships
│
├── ast-inspection
│   └── Structural source and AST-derived inspection
│
└── api-review
    └── Workflow for evaluating API changes and impact
```

The key design principle is:

> Minimize the number of skills an agent must choose between while keeping each remaining skill responsible for a clearly distinct type of task.

The final structure should make the agent's decision process straightforward:

```text
Need semantic relationships?
        ↓
semantic-analysis

Need structural inspection?
        ↓
ast-inspection

Need to review or modify an API and assess impact?
        ↓
api-review
```

Preserve useful existing guidance, merge overlapping content deliberately, and remove obsolete skills only after the migration is complete.

