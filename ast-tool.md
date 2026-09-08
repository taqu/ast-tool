# Phase 14 — Documentation

## Objective

Phase 14 prepares the user-facing documentation required for the initial release.

The goal is:

```text
document the existing release candidate accurately,
clearly,
and with enough practical guidance
for a new user to install and use it successfully
```

This phase focuses on:

```text
README
installation
quick start
command reference
examples
supported capabilities
supported platforms
known limitations
troubleshooting
Coding Agent integration guidance
release-facing architecture overview
```

This is not a feature-development phase.

Do not modify semantic behavior, routing behavior, or the public CLI merely to make documentation easier to write.

The documentation must describe the product that actually exists.

---

# Baseline

Use the Phase 13 release candidate as the documentation baseline.

Record:

```text
branch
commit hash
target release version
Phase 13 result
final public command inventory
known limitations
supported platforms
```

If implementation changes occur during Phase 14 due to a genuine documentation-discovered release defect, document the change and revalidate the affected behavior.

Do not silently document behavior that has not been implemented or tested.

---

# Core Principle

Documentation is part of the release contract.

Every user-facing statement should satisfy:

```text
Is this true for the current release candidate?
```

Prefer:

```text
precise documented capability
```

over:

```text
broad aspirational capability
```

Do not advertise theoretical or partially implemented functionality.

---

# Source of Truth

Use the outputs from previous release phases as the primary source of truth.

In particular:

```text
Phase 11
    release scope
    frozen baseline
    known limitations
    deferred/rejected work

Phase 12
    tested platforms
    installation behavior
    failure behavior
    CLI contract validation
    compatibility findings

Phase 13
    final command inventory
    final help behavior
    error UX
    externally visible CLI changes
```

When documentation conflicts with actual tested behavior:

```text
actual tested behavior wins
```

Update the documentation.

Do not change the product merely to preserve outdated documentation unless the mismatch is itself a release defect.

---

# Phase 14a — Documentation Inventory

First inspect the repository for existing documentation.

Inventory at least:

```text
README
docs/
examples/
CONTRIBUTING
CHANGELOG
LICENSE
installation notes
CLI reference
architecture notes
agent integration documentation
```

For every existing document, classify it as:

```text
KEEP
UPDATE
REPLACE
MOVE
REMOVE
INTERNAL ONLY
```

Do not immediately rewrite everything.

Preserve useful existing material when it remains accurate.

---

# Phase 14b — README

The README should be the main entry point for a new user.

It should answer, in approximately this order:

```text
What is ast-tool?

Why would I use it?

What does it support?

How do I install it?

How do I run my first useful command?

What are the main commands?

Where can I find detailed documentation?

What are the important limitations?
```

Keep the README practical.

Avoid turning it into an implementation design document.

---

## Recommended README Structure

Use a structure similar to:

```text
# ast-tool

short description

## Features

## Installation

## Quick Start

## Common Commands

## JSON / Automation Usage

## Coding Agent Usage

## Supported Languages / Platforms

## Known Limitations

## Documentation

## License
```

Adjust the exact structure to the project.

Do not add empty sections merely to match this template.

---

# Phase 14c — Project Description

Write a concise product description.

The description should explain the actual role of the tool.

Conceptually:

```text
ast-tool provides structural and semantic source-code queries
through a command-line interface.

It is intended for users, scripts, IDE/tooling integration,
and Coding Agents that need targeted code relationships
without manually reconstructing them from text search.
```

Adapt this to the actual supported scope.

Avoid marketing claims such as:

```text
understands any codebase

complete semantic analysis

compiler-level accuracy

perfect cross-language analysis
```

unless those claims are demonstrably true.

---

# Phase 14d — Feature List

Document only accepted release capabilities.

At minimum consider documenting:

```text
symbol search

structural lookup

references

callers

callees

supported semantic resolution

machine-readable JSON output

repository/workspace analysis
```

If other public commands exist, include them.

Feature descriptions should explain practical value rather than internal implementation.

For example, prefer:

```text
Find callers of a function or method across the repository.
```

over:

```text
Uses receiver-type member relationship resolution.
```

Internal terms may appear in architecture documentation, not as the primary user explanation.

---

# Phase 14e — Installation Documentation

Document the actual supported installation path.

Instructions must be reproducible from a clean environment.

Include:

```text
prerequisites

required toolchain/runtime versions where relevant

installation/build command

where the binary is produced or installed

how to verify installation
```

The verification step should use the real CLI, for example:

```text
ast-tool --version
ast-tool --help
```

Do not document packaging methods that do not yet exist.

If the initial release supports only building from source, say so clearly.

Packaging/distribution mechanisms belong to Phase 15.

---

# Phase 14f — Quick Start

Create a minimal path from installation to first useful result.

A good quick start should require only a few commands.

Include representative workflows such as:

```text
search for a symbol

inspect a symbol

find references

find callers or callees
```

Use a small realistic example.

Keep the quick start copy-pasteable.

Do not require the user to understand the architecture first.

---

# Phase 14g — Command Reference

Create or update documentation for every public command identified in Phase 13.

For each command document:

```text
purpose

usage

arguments

important options

accepted target forms

output

JSON behavior if supported

empty-result behavior

ambiguity behavior

common errors

example
```

Do not manually duplicate every line of `--help` unless useful.

The command reference should complement the CLI help by providing context and examples.

---

# Phase 14h — Examples

Provide a compact set of representative examples.

Prefer workflows such as:

```text
find a symbol

find all references to a symbol

find callers of a function

find callees of a function

query a class method

use a qualified name

use JSON output

run against a repository from a nested directory
```

Use exact syntax supported by the current CLI.

Test every documented command example.

A documentation example that does not run is a release defect in the documentation.

---

# Phase 14i — JSON / Automation Documentation

Document machine-readable usage explicitly.

Explain:

```text
how to enable JSON output

what appears on stdout

what appears on stderr

how empty results are represented

how failures affect exit status

whether field stability is part of the release contract
```

Provide at least one valid JSON example where appropriate.

Do not invent a formal schema unless one actually exists.

If the JSON API is intentionally pre-1.0 and subject to change, state that clearly.

---

# Phase 14j — Shell / Script Usage

Provide enough information for users to use ast-tool safely in automation.

Document relevant behavior such as:

```text
exit codes

stdout/stderr separation

quoting symbol names

quoting paths

paths containing spaces

non-interactive behavior
```

If platform-specific quoting differs, document the tested form.

Do not document shell behavior that was not validated in Phase 12.

---

# Phase 14k — Coding Agent Integration

Create a concise section describing how Coding Agents should use the tool.

The goal is not to reproduce the internal evaluation Skill.

Explain the recommended usage model:

```text
use targeted semantic queries when relationship discovery is needed

prefer small semantic results over broad repository reads

use callers / callees / references where appropriate

fall back to ordinary inspection when semantic analysis is unnecessary
```

If the repository includes a Skill file or agent integration example, explain where it is and how it can be used.

Do not claim that agents will always invoke semantic tooling correctly.

Do not document rejected Phase 10 routing experiments as supported behavior.

---

# Phase 14l — Supported Languages

Document the actual supported language set.

For each language, if relevant, distinguish between:

```text
parsing support

structural extraction support

semantic relationship support
```

Only make distinctions that are useful and supported by evidence.

Avoid implying that every command has identical capability across all languages unless verified.

If support is currently focused on one language, state that plainly.

---

# Phase 14m — Supported Platforms

Use the Phase 12 compatibility matrix.

Classify platforms using the actual test status, for example:

```text
Supported and tested

Best effort

Not supported
```

Do not list an operating system as fully supported merely because the code can theoretically compile there.

If architecture restrictions exist, document them.

---

# Phase 14n — Known Limitations

Publish a concise and practical known-limitations section.

Include only limitations relevant to users.

Examples may include:

```text
advanced receiver/type inference limitations

auto / decltype receiver cases

templates

inheritance / virtual dispatch

overload-sensitive cases

complex receiver expressions

specific declaration/definition relationship gaps
```

Verify current behavior before including each item.

Explain limitations in user-facing language.

For example, prefer:

```text
Some calls through complex C++ expressions may not be resolved.
```

over internal implementation terminology.

Where useful, explain the fallback:

```text
use search or inspect the relevant source manually
```

---

# Phase 14o — Unsupported Behavior

Make intentional unsupported scope clear enough that users do not confuse it with bugs.

Where relevant, distinguish:

```text
unsupported

best-effort

known defect
```

Do not promise behavior merely because it works in a simple example.

---

# Phase 14p — Troubleshooting

Create a focused troubleshooting section based on observed failures from Phase 12 and Phase 13.

Include real problems users are likely to encounter, such as:

```text
repository not detected

symbol not found

ambiguous symbol

no supported source files

path quoting problems

malformed source

JSON parsing problems caused by incorrect invocation

unsupported semantic case
```

For each issue, provide:

```text
symptom

likely cause

recommended action
```

Do not create a huge speculative FAQ.

Use evidence from actual release testing.

---

# Phase 14q — Error Interpretation

Document important distinctions users need to understand.

Examples:

```text
no result
vs.
command failure

ambiguous result
vs.
not found

unsupported semantic case
vs.
internal error
```

Keep terminology consistent with the CLI.

---

# Phase 14r — Architecture Overview

Provide a small architecture overview for users and contributors who need context.

Keep it high level.

Conceptually:

```text
source files
→ parser
→ AST IR
→ semantic/workspace analysis
→ CLI semantic services
```

Explain important design boundaries only if they help users understand capability or limitations.

Do not copy internal phase-history documentation into the public architecture section.

---

# Phase 14s — Internal vs Public Documentation

Review existing design and evaluation documents.

Do not expose internal experimental detail as if it were public product documentation.

Examples of material that may remain internal:

```text
Phase-by-phase optimization history

agent token experiments

routing candidate evaluations

rejected semantic experiments

raw evaluation logs
```

Keep them if they are useful for development.

Clearly separate:

```text
user documentation
```

from:

```text
development / research documentation
```

---

# Phase 14t — Version and Compatibility Language

Because this is an initial release, document compatibility expectations carefully.

If the release is pre-1.0, consider language such as:

```text
The CLI and machine-readable output may evolve between minor releases.
```

only if that reflects the intended policy.

Do not promise long-term API stability unless the project is ready to maintain it.

Align this wording with the versioning decision from Phase 11.

---

# Phase 14u — License and Notices

Verify that public documentation references the correct license.

Check:

```text
LICENSE exists

README license section is accurate

third-party notices are referenced if required

copyright/project naming is consistent
```

Do not invent license terms.

If licensing information is incomplete, classify it as a release issue.

---

# Phase 14v — Documentation Validation

Every command shown in public documentation must be tested.

Create an automated or manual documentation verification pass.

At minimum verify:

```text
installation commands

--help / --version examples

quick-start commands

command-reference examples

JSON examples

path examples
```

Where possible, convert important examples into smoke tests.

Do not trust copied historical command syntax.

---

# Phase 14w — Fresh-User Documentation Test

Perform a documentation-only usability test.

Use a clean environment or simulate one.

The evaluator should follow only the public documentation.

Do not rely on undocumented project knowledge.

Attempt:

```text
install/build

verify installation

run first query

run a relationship query

run JSON output

recover from one common error
```

Record every point where the documentation is:

```text
missing

incorrect

ambiguous

outdated

dependent on undocumented assumptions
```

Fix documentation defects before closing Phase 14.

---

# Phase 14x — Documentation Consistency Audit

Search public documentation for stale terminology and old behavior.

Check for references to:

```text
rejected commands

old argument forms

removed flags

pre-Phase-8 behavior

Phase 10 trigger experiments

obsolete installation paths

unsupported platforms
```

Also verify consistency of:

```text
project name

binary name

version

command spelling

option spelling

repository terminology

JSON terminology
```

---

# Writing Style

Public documentation should be:

```text
concise

technical

practical

specific

copy-pasteable
```

Avoid:

```text
marketing-heavy language

unverified claims

long internal design explanations

historical experiment narratives

unnecessary jargon
```

Prefer examples over abstract explanation where practical.

---

# Change Policy

Documentation work may reveal a product problem.

If that happens, classify it before changing code.

Use:

```text
documentation defect
    → fix documentation

minor CLI clarity defect
    → consider narrowly scoped Phase 13-style fix

release-quality defect
    → fix and rerun relevant Phase 12 gate

new capability request
    → defer
```

Do not allow documentation work to reopen feature development.

---

# Required Documentation Set

At minimum, the release should have:

```text
README

installation instructions

quick start

public command reference

examples

supported language/platform information

known limitations

troubleshooting guidance

license information
```

These may live in one document or several files depending on project size.

Do not create unnecessary documentation hierarchy for a small tool.

---

# Expected Deliverables

## 1. Documentation Inventory

Produce:

```text
File
Audience
Purpose
Status
Action taken
```

---

## 2. Updated README

The README should be release-ready and serve as the primary project landing page.

---

## 3. Installation / Quick Start

Provide tested instructions from a clean environment.

---

## 4. Command Reference

Cover the complete public CLI surface established in Phase 13.

---

## 5. Examples

Provide tested, practical examples for major workflows.

---

## 6. Support Matrix

Document:

```text
languages

operating systems

architectures if relevant

support level
```

---

## 7. Known Limitations

Produce a release-facing limitations list.

---

## 8. Troubleshooting Guide

Cover recurring real-world failures found during release qualification.

---

## 9. Documentation Validation Report

Record:

```text
examples tested

commands tested

clean-environment walkthrough result

documentation defects found

documentation defects fixed

remaining limitations
```

---

## 10. Final Documentation Commit

Record:

```text
branch
commit hash
target version
working tree status
```

---

## 11. Phase 14 Final Recommendation

End with one of:

```text
PASS — READY FOR PACKAGING / DISTRIBUTION

PASS WITH DOCUMENTED LIMITATIONS

BLOCKED — RELEASE DOCUMENTATION INCOMPLETE
```

Do not block on cosmetic documentation imperfections.

Block only when a user cannot reasonably install, understand, or operate the supported release from the provided documentation.

---

# Acceptance Criteria

Phase 14 is complete when:

```text
1. README accurately describes the current release candidate

2. installation instructions work from a clean environment

3. quick-start instructions produce a useful result

4. every public command is documented

5. all documented CLI examples have been validated

6. JSON / automation behavior is documented where supported

7. supported languages are stated accurately

8. supported platforms match Phase 12 evidence

9. known limitations are clearly documented

10. common release-relevant errors have troubleshooting guidance

11. version and compatibility language matches Phase 11 policy

12. license information is present and accurate

13. stale or rejected behavior has been removed from public documentation

14. a fresh user can follow the documentation without internal project knowledge

15. no documentation-critical release issue remains
```

---

# Out of Scope

Do not use Phase 14 for:

```text
semantic capability changes

routing optimization

CLI redesign

new commands

performance work

dependency upgrades

new platform support

new packaging mechanisms

release automation

feature roadmap implementation
```

Those belong elsewhere.

---

# Final Principle

Phase 14 should answer:

```text
Can someone who has never worked on this project
understand what it does,
install it,
use its supported capabilities,
and understand its limitations
using only the public documentation?
```

The documentation should describe:

```text
the product we are releasing
```

not:

```text
the product we might build later
```
