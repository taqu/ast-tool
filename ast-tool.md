# Phase 13 — CLI / User Experience Polish

## Objective

Phase 13 improves the public command-line experience of the frozen release candidate without changing the underlying semantic capability.

The goal is:

```text
make the existing CLI easier to understand,
more consistent,
and safer to use
without expanding release scope
```

This phase focuses on:

```text
help text
command discoverability
argument clarity
error messages
output readability
JSON contract clarity
version reporting
diagnostics
examples
terminology consistency
```

This is not a semantic-engine phase.

Do not introduce new semantic capabilities, new routing behavior, or speculative agent-oriented optimizations.

---

# Baseline

Start from the Phase 12 release-qualified baseline.

Record:

```text
branch
commit hash
target version
Phase 12 result
known limitations
remaining release fixes
```

The semantic baseline remains:

```text
Phase 7d Skill
+
Phase 8a
+
Phase 8b
+
Phase 8c
```

Phase 10 routing changes remain excluded.

---

# Core Principle

Phase 13 should improve presentation, not behavior.

Prefer:

```text
same capability
same semantics
same result
clearer interface
```

Avoid:

```text
new capability
new inference
new implicit behavior
new command families
```

If a UX improvement requires a semantic redesign, defer it.

---

# Compatibility Constraint

Phase 12 established the current CLI contract.

Treat externally visible behavior as compatibility-sensitive.

Do not casually change:

```text
command names
argument meaning
exit codes
JSON field meaning
machine-readable structure
stdout/stderr responsibilities
```

Breaking changes are allowed only when:

```text
the existing behavior is clearly defective
and
the fix is necessary before the initial release
```

Document every externally visible change.

---

# Phase 13a — CLI Surface Review

Inventory all public commands and options.

For each command, evaluate:

```text
Is the name understandable?

Is its purpose obvious from --help?

Are required arguments obvious?

Are optional arguments understandable?

Are defaults visible?

Are mutually exclusive options clear?

Does terminology match related commands?

Is the expected output understandable?

Are common failure cases explained?
```

Create a table such as:

```text
Command
Purpose
Current UX issue
Proposed change
Compatibility impact
Decision
```

Do not change anything yet.

Review first, then apply only justified changes.

---

# Phase 13b — Top-Level Help

Review:

```text
ast-tool --help
```

The top-level help should answer quickly:

```text
What is ast-tool?

What kinds of things can it do?

What commands are available?

How do I get command-specific help?

What is the basic invocation shape?
```

Prefer concise help over exhaustive documentation.

A user should not need to read a long manual before running their first useful command.

Recommended structure:

```text
short description

usage

commands

global options

one or two examples

how to get more help
```

Avoid internal architecture terminology unless users need it.

---

# Phase 13c — Command Help

Review every public command:

```text
ast-tool <command> --help
```

Each command should clearly communicate:

```text
what it does

what the target argument means

whether the target can be:
    symbol
    FQN
    partial FQN
    file
    query

what repository/path context is required

what normal output contains

what JSON mode does

important ambiguity/not-found behavior
```

Do not duplicate entire reference documentation inside `--help`.

Help text should be optimized for terminal use.

---

# Phase 13d — Terminology Consistency

Audit user-facing terminology.

Identify cases where the CLI uses multiple names for the same concept.

Examples to review:

```text
symbol
target
query
name
FQN
qualified name

repository
workspace
root

reference
caller
callee
relationship

file
path
source
```

Choose consistent terms wherever possible.

Do not rename concepts solely for stylistic preference if doing so creates unnecessary compatibility changes.

Priority:

```text
clarity > stylistic uniformity
```

---

# Phase 13e — Argument UX

Review positional arguments and flags.

Look for:

```text
unclear positional ordering

required arguments that appear optional

flags with ambiguous names

boolean flags with surprising behavior

inconsistent flag spelling

duplicate concepts exposed differently across commands

arguments whose help text does not describe accepted forms
```

Prefer predictable patterns.

For example, related commands should ideally use the same argument naming conventions.

Do not redesign the parser unless necessary.

---

# Phase 13f — Error Messages

Review common user errors.

At minimum test:

```text
unknown command

unknown flag

missing required argument

invalid path

repository not found

unsupported source

symbol not found

ambiguous symbol

invalid output option

invalid command combination
```

Error messages should explain:

```text
what failed
why it failed when known
what the user can do next
```

Prefer:

```text
symbol not found: AuthToken::validate
```

over vague messages such as:

```text
operation failed
```

Where ambiguity exists, report enough information to help the user disambiguate.

Avoid exposing internal implementation details unless useful for diagnosis.

---

# Error Message Rules

A normal user error should not normally produce:

```text
stack trace
panic output
internal file paths
raw parser diagnostics
implementation-specific identifiers
```

unless verbose/debug mode explicitly requests diagnostics.

User-facing errors should be concise.

Diagnostic detail should remain available when needed.

---

# Phase 13g — Diagnostic Output

Define the intended distinction between:

```text
normal result
warning
error
debug/diagnostic information
```

Preserve the Phase 12 stdout/stderr contract.

In general:

```text
stdout
    command result / machine-readable output

stderr
    diagnostics / warnings / errors
```

Do not allow diagnostics to corrupt JSON output.

If verbose or debug modes already exist, verify that they are understandable.

Do not add an elaborate logging framework purely for release polish.

---

# Phase 13h — Human-Readable Output

Review normal text output for representative commands.

Evaluate:

```text
readability

unnecessary noise

duplicated information

unclear labels

path readability

qualified-name readability

empty-result presentation
```

Prefer compact output suitable for:

```text
humans
shell pipelines
coding agents
```

Do not turn normal command output into decorative UI.

The CLI should remain predictable and script-friendly.

---

# Phase 13i — Empty Results

Standardize how commands communicate valid empty results.

Examples:

```text
no callers

no references

no matching symbols
```

Distinguish:

```text
valid empty result
```

from:

```text
command failure
```

Do not accidentally treat a legitimate empty relationship as an internal error.

Make both human-readable and JSON behavior clear.

---

# Phase 13j — Ambiguous Results

Review commands that may encounter multiple valid matches.

The CLI should make ambiguity understandable.

Where currently supported, report:

```text
the query

that multiple matches exist

candidate identities or useful disambiguating information

how the user can retry with a more specific target
```

Do not silently choose a candidate unless that behavior is already part of the accepted semantic contract.

Preserve Phase 8 ambiguity guards.

---

# Phase 13k — JSON UX

Do not redesign the JSON schema without strong justification.

Review whether JSON consumers can understand:

```text
success result

empty result

ambiguous result

error result
```

Check naming consistency between commands.

If inconsistencies exist, classify them:

```text
release blocker
safe pre-release cleanup
post-release compatibility issue
```

Only make schema changes that are safe before the initial public contract is established.

After any JSON change:

```text
update contract tests
rerun JSON validation
document the change
```

---

# Phase 13l — Version UX

Verify that users can determine the installed version.

Preferred behavior:

```text
ast-tool --version
```

or the project's existing equivalent.

The output should be:

```text
short
stable
machine-readable enough for tooling
```

For example:

```text
ast-tool 0.1.0
```

If useful and already easily available, additional build metadata may be exposed through a separate verbose form.

Do not overload normal version output with development diagnostics.

---

# Phase 13m — Help Examples

Add a small number of high-value examples.

Prefer examples covering core workflows such as:

```text
search for a symbol

find references

find callers

find callees

request JSON output
```

Examples should use realistic but generic targets.

Keep examples copy-pasteable.

Do not add dozens of examples to `--help`.

Full examples belong in documentation.

---

# Phase 13n — First-Run Experience

Simulate a new user who knows only that the tool performs source-code semantic analysis.

Without reading internal project documentation:

```text
run --help

choose a command

run it on a repository

understand the result

recover from one incorrect invocation
```

Record friction points.

Prioritize issues where users cannot reasonably infer the correct next step.

Do not optimize every minor preference.

---

# Phase 13o — Shell Usability

Verify common command-line usage.

Examples:

```text
quoted arguments

paths containing spaces

redirecting output

piping text output where appropriate

capturing JSON output

checking exit status
```

Repeat key Windows quoting cases from Phase 12 if CLI text or argument handling changes.

Do not introduce shell-specific behavior unless explicitly required.

---

# Phase 13p — Agent Compatibility Smoke Test

Although this phase is user-facing, ast-tool is also intended for Coding Agent use.

Run a lightweight smoke check that the polished CLI did not make agent use worse.

Verify:

```text
existing Skill commands remain valid

command names remain discoverable

machine-readable output remains stable

error messages do not confuse automation

no new interactive prompt was introduced
```

Do not restart routing optimization.

The purpose is regression detection only.

---

# Phase 13q — Non-Interactive Requirement

The normal CLI should remain suitable for automation.

Avoid adding mandatory:

```text
interactive prompts

confirmation dialogs

terminal-only UI

color-dependent semantics

paging that blocks automation
```

If interactive functionality exists, automation should have a predictable non-interactive path.

Coding agents and scripts must be able to invoke commands unattended.

---

# Phase 13r — Color and Terminal Behavior

If colored output is used, ensure:

```text
meaning is not conveyed by color alone

redirected output remains usable

JSON never contains ANSI escape sequences

non-TTY environments behave correctly
```

Do not introduce color purely for cosmetic purposes if none exists.

---

# Phase 13s — Documentation Alignment Check

Phase 14 will perform full documentation work.

Phase 13 should only verify that current CLI behavior can be documented consistently.

Create a list of:

```text
commands requiring documentation

important flags

semantic terminology

known limitations

examples worth documenting

common troubleshooting cases
```

Do not write the entire documentation set here.

---

# Change Classification

For every proposed UX change, classify it as:

```text
A — clarity-only
    no behavioral compatibility impact

B — safe pre-release contract cleanup
    externally visible but clearly preferable before first release

C — breaking or risky
    defer unless release-critical
```

Prefer A.

Use B sparingly.

Avoid C.

---

# Release-Freeze Change Policy

Every Phase 13 code change should satisfy:

```text
What concrete user confusion or release-quality issue does this fix?
```

If there is no clear answer, do not make the change.

Do not perform opportunistic cleanup.

Avoid:

```text
large refactoring

parser redesign

command architecture redesign

semantic changes

dependency churn

performance optimization

code-style cleanup unrelated to UX
```

---

# Regression Requirements

After CLI changes, rerun relevant Phase 12 gates.

At minimum verify:

```text
build

CLI tests

exit codes

stdout/stderr

JSON parsing

path handling

help output

version output

core semantic smoke tests
```

If argument parsing changed, rerun cross-platform command-line tests.

If JSON changed, rerun all JSON contract tests.

---

# Expected Deliverables

## 1. CLI UX Audit

Produce a table:

```text
Area
Issue
Severity
Proposed action
Compatibility class
Final decision
```

Include both fixed and intentionally deferred issues.

---

## 2. Final Command Inventory

Record every public command with:

```text
purpose
usage
important options
output modes
```

This becomes input for Phase 14 documentation.

---

## 3. CLI Contract Change Log

For every externally visible change made in Phase 13:

```text
old behavior
new behavior
reason
compatibility impact
tests
```

If none were required, explicitly state that the CLI contract remained unchanged.

---

## 4. Error UX Matrix

Document representative errors:

```text
Scenario
Exit code
stdout
stderr
User guidance
Status
```

---

## 5. Help Verification

Capture or test:

```text
top-level --help

each public command --help

--version
```

Confirm that the output matches actual behavior.

---

## 6. Deferred UX Backlog

Record improvements that are useful but unnecessary for the release.

Examples:

```text
new aliases

additional formatting

advanced diagnostics

new convenience commands

interactive exploration

shell completion

additional output formats
```

Do not implement them during freeze.

---

## 7. Final Regression Result

Record:

```text
branch
commit
tests
CLI smoke result
JSON result
platform result
working tree status
```

---

## 8. Phase 13 Final Recommendation

End with one of:

```text
PASS — READY FOR DOCUMENTATION

PASS WITH DOCUMENTED UX LIMITATIONS

BLOCKED — CLI RELEASE ISSUE REMAINS
```

Aesthetic imperfections alone should not block release.

---

# Acceptance Criteria

Phase 13 is complete when:

```text
1. every public command has understandable help

2. top-level help clearly explains the tool and command surface

3. required/optional arguments are understandable

4. common user errors produce useful messages

5. valid empty results are distinguishable from failures

6. ambiguous results are understandable

7. stdout/stderr behavior remains automation-safe

8. JSON output remains valid and documented enough to consume

9. installed version can be identified

10. no mandatory interactive behavior blocks scripts or Coding Agents

11. terminology is reasonably consistent

12. all externally visible changes are recorded

13. relevant Phase 12 regression gates still pass

14. no unresolved CLI release blocker remains
```

---

# Out of Scope

Do not use Phase 13 for:

```text
semantic capability development

semantic routing optimization

agent prompt optimization

new command families

new language support

major architectural refactoring

performance optimization

packaging/distribution work

full documentation authoring
```

Those belong elsewhere.

---

# Final Principle

Phase 13 should answer:

```text
Can a new user understand and operate the existing product
without needing knowledge of its internal implementation?
```

The desired release CLI is:

```text
small
clear
predictable
scriptable
consistent
```

Do not polish the product by making it larger.

Polish it by making the existing behavior easier to understand.
