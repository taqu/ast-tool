# Phase 12 — Release Quality / Compatibility

## Objective

Phase 12 validates that the frozen release baseline is stable, installable, predictable, and usable across the supported release environments.

This phase is about:

```text
correctness
stability
compatibility
failure behavior
installation
CLI contract
cross-platform reliability
```

This phase is **not** about adding semantic capability, improving routing, optimizing agent behavior, or redesigning the CLI.

The goal is to answer:

```text
Can the frozen baseline be safely distributed to users?
```

---

# Baseline

Use the Phase 11 frozen baseline.

Do not change the semantic baseline except to fix a confirmed release defect.

The expected baseline remains:

```text
Phase 7d Skill
+
Phase 8a
    unique FQN suffix relationship resolution
+
Phase 8b
    receiver-type member relationship resolution
+
Phase 8c
    callable declaration/definition body identity
```

Phase 10 routing experiments remain excluded.

Before beginning Phase 12, record:

```text
baseline commit
branch
target release version
working tree state
known release blockers from Phase 11
```

If the baseline changes during Phase 12 because of a release fix, record the new commit and the reason.

---

# Feature Freeze

The Phase 11 feature freeze remains active.

Do not add:

```text
new commands
new semantic behavior
new routing logic
new language support
new inference logic
new output modes
new agent optimization
new convenience features
```

A code change is allowed only if it addresses:

```text
release correctness
stability
compatibility
installation
packaging prerequisites
documented CLI contract
confirmed release blocker
```

If an issue can reasonably be documented as a known limitation, prefer documentation over expanding scope.

---

# Phase 12a — Clean Build Verification

Verify that the project builds successfully from a clean environment.

Do not rely on artifacts from previous development builds.

Test at minimum:

```text
fresh repository checkout

dependency/bootstrap procedure

clean build

test build

release build if distinct

binary execution after build
```

Record:

```text
platform
compiler/toolchain version
build command
result
warnings
produced artifact
artifact size
```

Warnings should be reviewed, but warnings alone are not release blockers unless they indicate correctness, compatibility, or packaging risk.

---

# Phase 12b — Full Regression Suite

Run the complete existing automated test suite against the frozen baseline.

Record:

```text
total tests
passed
failed
skipped
expected failures
unexpected failures
elapsed time
```

Classify every unexpected failure.

Do not casually modify tests to make the baseline pass.

For each failure, determine whether it is:

```text
product defect
platform defect
test defect
environment issue
known limitation
flaky test
```

A failing test may only be excluded from the release gate if the reason is understood and documented.

---

# Phase 12c — Core Semantic Smoke Tests

Run a compact set of release-level semantic smoke tests covering the accepted Phase 8 capabilities.

At minimum verify:

## Phase 8a

```text
exact FQN lookup

unique FQN suffix fallback

ambiguity remains ambiguous

not-found remains not-found
```

## Phase 8b

Verify representative member relationship resolution for:

```text
object field
pointer field
local object
local pointer
reference parameter
pointer parameter
```

Also verify the false-positive guard:

```text
same member name
on unrelated types
→ must not cross-link
```

## Phase 8c

Verify:

```text
free-function declaration → definition body

class method declaration → out-of-line definition

namespaced out-of-line definition

inline method
```

The purpose is not to re-evaluate Phase 8 performance.

The purpose is to detect release-time regression.

---

# Phase 12d — CLI Contract Validation

Validate every public CLI command recorded during Phase 11.

For each command, verify:

```text
valid invocation

required argument handling

optional argument handling

help behavior

normal stdout

normal stderr

exit code

JSON output if supported

invalid argument behavior

not-found behavior

ambiguous-result behavior
```

The CLI should behave consistently and predictably.

Do not redesign the command surface during this phase.

If a CLI inconsistency is non-critical and changing it would introduce compatibility risk, document it for later instead.

---

# Exit Code Validation

Explicitly test exit status behavior.

Verify at minimum:

```text
successful operation
    → success exit code

invalid command-line arguments
    → non-zero

missing repository / invalid path
    → non-zero

unsupported or invalid operation
    → non-zero where appropriate

internal fatal failure
    → non-zero
```

Do not require every semantic no-result case to be an error unless that is already the documented CLI contract.

Record the current contract rather than inventing a new one.

---

# stdout / stderr Validation

Check that machine-readable and human-readable output are not accidentally mixed.

Verify:

```text
normal command result
diagnostics
warnings
errors
JSON mode
```

For JSON output in particular:

```text
stdout must remain parseable
diagnostics must not corrupt JSON
error behavior must be deterministic
```

If diagnostics currently appear on stdout and would break machine consumption, treat this as a release-quality issue.

---

# Phase 12e — JSON Stability

For every command with JSON output:

```text
parse output with a real JSON parser
```

Do not validate JSON by visual inspection alone.

Check:

```text
valid JSON syntax
stable top-level type
expected required fields
empty-result representation
error representation
escaping
Unicode content
paths containing spaces
```

Where practical, add automated contract tests for JSON output that is intended for external consumers.

Avoid unnecessary schema redesign.

The goal is to prevent accidental malformed or structurally inconsistent output.

---

# Phase 12f — Path and Repository Handling

Test repository and file-path behavior explicitly.

Include:

```text
absolute paths
relative paths
repository root
nested working directory
paths containing spaces
paths containing non-ASCII characters
missing paths
file path supplied where directory expected
directory path supplied where file expected
```

Also verify repository-root discovery if supported.

Do not assume Unix-style paths.

---

# Phase 12g — Windows Compatibility

The previously observed Windows path quoting issue must be explicitly investigated during release qualification.

Test the actual release CLI on Windows, or the closest supported Windows CI/runtime environment.

Include paths containing:

```text
spaces
parentheses
non-ASCII characters
nested directories
```

Test both:

```text
direct invocation

invocation through the expected shell / agent execution path
```

Determine whether the historical quoting failures originate from:

```text
ast-tool itself
shell invocation
test harness
agent-generated command syntax
```

Do not modify ast-tool to compensate for a harness-only problem unless the real public CLI is affected.

Classify the result as:

```text
FIXED / NO PRODUCT DEFECT

RELEASE BLOCKER

KNOWN ENVIRONMENT LIMITATION
```

with evidence.

---

# Phase 12h — Cross-Platform Verification

Test every platform intended to be claimed as supported.

Prefer a matrix such as:

```text
Linux
macOS
Windows
```

but only include platforms the project actually intends to support.

For each platform verify:

```text
build
tests
basic command invocation
repository detection
path handling
JSON output
exit codes
```

If a platform cannot be tested, do not silently claim it as fully supported.

Classify it appropriately, for example:

```text
tested
best-effort
unsupported
```

---

# Phase 12i — Installation Verification

Test installation from the user-facing installation method, not only from the development tree.

Examples may include:

```text
building from source
install script
package manager
downloaded binary
```

depending on what currently exists.

At minimum verify:

```text
installation succeeds

binary can be located and executed

--help works

version command works if supported

basic semantic command works

uninstallation or removal procedure is understood
```

Do not build a new packaging ecosystem in Phase 12.

Packaging work that does not yet exist belongs to the packaging/distribution phase.

For Phase 12, verify the installation paths that already exist or are required for the release candidate.

---

# Phase 12j — Clean Environment Smoke Test

Use an environment that does not contain the development repository's cached build state.

Perform a minimal user journey:

```text
obtain project / binary

install or build according to documentation

run --help

run version command if available

analyze a small repository

perform search

perform callers or references query

run JSON output

handle one expected error
```

The test should expose assumptions that only work in the developer environment.

---

# Phase 12k — Failure Behavior

Test malformed, incomplete, and unsupported inputs.

Examples:

```text
empty repository

repository with no supported source files

nonexistent repository

permission-denied path where practical

malformed source file

partially parseable source

unsupported language file

unknown symbol

ambiguous symbol

invalid command

missing command argument

invalid JSON/output flag combination
```

Expected behavior is:

```text
no crash

no panic

no repository modification

clear failure or empty-result behavior

appropriate exit code

machine-readable output remains valid where promised
```

Do not require semantic success for unsupported inputs.

Require graceful behavior.

---

# Phase 12l — Crash / Panic Audit

Search test logs and manual runs for:

```text
panic
fatal
assertion failure
uncaught exception
segmentation fault
stack trace
```

Any crash reachable through ordinary supported CLI input should be treated as a release blocker unless there is a strong documented reason otherwise.

For every crash:

```text
capture reproduction

minimize input if practical

identify root cause

add regression coverage

fix narrowly

rerun relevant release tests
```

Avoid opportunistic refactoring while fixing crashes.

---

# Phase 12m — Repository Safety

Verify that analysis commands do not unexpectedly modify user repositories.

Run representative commands and check:

```text
git status before

execute ast-tool commands

git status after
```

Normal read/analysis commands must not:

```text
rewrite source files

generate persistent files inside the repository unexpectedly

change permissions unexpectedly

modify project configuration
```

If caches or indexes are created, their location and lifecycle must be intentional and documented.

Unexpected repository modification is a release blocker.

---

# Phase 12n — Concurrency / Repeated Invocation Smoke Test

If the tool may reasonably be invoked repeatedly by agents or IDE integrations, run a lightweight repeated-use test.

Examples:

```text
same command repeated many times

different semantic commands in sequence

multiple repositories sequentially

parallel invocation if explicitly supported
```

Look for:

```text
state leakage

stale repository state

temporary-file collisions

crashes

corrupted output

significant resource growth
```

Do not introduce concurrency support if it does not already exist.

Only test concurrency if the CLI contract or expected integrations permit it.

---

# Phase 12o — Performance Sanity Check

This is not a performance optimization phase.

Record only enough performance data to detect severe regression.

Use a small representative set and capture:

```text
startup latency
simple search latency
relationship-query latency
peak or approximate memory usage if easily available
binary size
```

Compare with recent accepted behavior where useful.

Do not reject the release because a metric is slightly worse.

Investigate only large unexplained regressions.

---

# Phase 12p — Dependency and Runtime Audit

Identify runtime dependencies required by the release.

Record:

```text
runtime libraries
external executables
language runtimes
parser/runtime assets
configuration files
environment variables
```

Verify that missing required dependencies fail clearly.

Avoid dependency upgrades unless required for:

```text
security
build compatibility
release correctness
distribution
```

Do not perform routine version churn during freeze.

---

# Phase 12q — Release Blocker Triage

Every issue discovered during Phase 12 must be classified as:

```text
RELEASE BLOCKER

RELEASE FIX

KNOWN LIMITATION

POST-RELEASE IMPROVEMENT

TEST / HARNESS ISSUE
```

Use the Phase 11 blocker definition.

A release blocker generally includes:

```text
crash in supported normal use

incorrect destructive behavior

broken installation

broken supported platform

invalid machine-readable output

incorrect exit behavior that breaks automation

common semantic correctness failure within documented support

major CLI contract violation
```

A marginal efficiency problem is not a release blocker.

---

# Fix Policy

For confirmed release defects:

```text
make the smallest reasonable fix

add or update regression coverage

rerun the directly affected tests

rerun the relevant platform/CLI gate

run the full suite before closing Phase 12
```

Do not bundle unrelated fixes.

Every release fix should have a clear mapping:

```text
observed defect
→ minimal change
→ verification
```

If fixing an issue requires substantial semantic redesign, reconsider whether the behavior should instead be documented as unsupported for this release.

---

# Final Regression Run

After all accepted release fixes are complete, perform a final clean verification.

At minimum:

```text
clean build

full test suite

core semantic smoke suite

CLI contract smoke suite

JSON validation

failure-behavior smoke suite

supported-platform smoke tests

repository-safety check
```

Record the final commit.

The final result must correspond to an identifiable source revision.

---

# Expected Deliverables

## 1. Environment Matrix

Produce a table containing:

```text
OS
version
architecture
compiler/toolchain
build result
test result
CLI smoke result
status
```

---

## 2. Regression Report

Include:

```text
tests run
passed
failed
skipped
known failures
unexpected failures
```

---

## 3. CLI Compatibility Report

Cover:

```text
public commands
exit codes
stdout/stderr
JSON
invalid input behavior
path behavior
```

---

## 4. Failure-Behavior Report

Record representative cases and whether each produced:

```text
expected result
clean error
unexpected failure
crash
```

---

## 5. Release Issue List

Use a table such as:

```text
Issue
Category
Platform
Severity
Release blocker?
Resolution
Verification
```

---

## 6. Final Frozen Commit

Record:

```text
branch
commit hash
target version
working tree status
```

---

## 7. Phase 12 Final Recommendation

End with one of:

```text
PASS — READY FOR CLI / UX RELEASE POLISH

PASS WITH KNOWN LIMITATIONS

BLOCKED — RELEASE FIXES REQUIRED
```

Do not declare PASS while confirmed release blockers remain unresolved.

---

# Acceptance Criteria

Phase 12 is complete when:

```text
1. clean builds succeed on every claimed supported environment

2. the full test suite has no unexplained release-critical failures

3. accepted Phase 8 semantic behavior passes smoke regression

4. public CLI behavior is understood and tested

5. machine-readable output is syntactically valid and predictable

6. exit-code behavior is suitable for automation

7. supported path/repository cases behave correctly

8. Windows quoting/path behavior has been explicitly classified

9. ordinary invalid input fails gracefully without crashing

10. analysis operations do not unexpectedly modify repositories

11. installation/basic execution works through the intended release path

12. all discovered issues are classified

13. no unresolved release blocker remains for a PASS result

14. the final tested commit is recorded
```

---

# Out of Scope

Do not use Phase 12 for:

```text
semantic capability expansion

routing improvement

agent trajectory optimization

token reduction work

new command design

large CLI redesign

new language support

major refactoring

general performance optimization

documentation rewrite

distribution ecosystem expansion
```

These are separate concerns.

---

# Final Principle

Phase 12 should answer:

```text
Does the frozen product behave reliably as a real command-line tool,
outside the development and evaluation environment?
```

The preferred outcome is not:

```text
more capable
```

but:

```text
predictable
stable
installable
safe
compatible
```

If a defect is found, fix only what is necessary to make the existing release scope trustworthy.
