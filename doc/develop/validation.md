# Release Candidate Validation Notes

## Purpose

The Release Candidate should be validated using the actual packaged release artifacts in isolated environments before the final release.

The purpose of this validation is to confirm that:

```text
release artifacts
+
public documentation
+
documented prerequisites
```

are sufficient for a new user to install, build, and use the tool without relying on the development environment.

This is a release validation procedure, not a feature-development phase.

---

## General Rules

Validate the actual Release Candidate artifacts.

Do not validate only the executable from the development build tree.

During validation:

```text
- use packaged RC artifacts
- use public documentation
- avoid development-only environment state
- avoid undocumented dependencies
- verify artifact checksums
- record the exact source revision and RC version
```

Do not introduce semantic, routing, or CLI improvements during RC validation.

If a release-blocking defect is found:

```text
record defect
→ fix source
→ rebuild artifacts
→ create a new RC
→ repeat affected validation
```

An existing RC artifact must not be modified in place.

---

# Validation Environments

Use isolated or disposable environments for both supported platforms.

## Linux

Use a disposable Debian-based Linux environment.

The environment may be modified freely during validation.

It is acceptable to:

```text
install/remove packages
remove previous ast-tool installations
delete temporary repositories
clear ast-tool-specific state
rebuild from scratch
```

The important requirement is that the validation does not rely on an existing ast-tool development installation.

---

## Windows

Prefer an isolated disposable Windows environment.

A Windows Sandbox or equivalent clean virtualized environment is suitable.

The environment should not rely on:

```text
the development source tree
developer-specific PATH entries
previous ast-tool installations
local build artifacts
undocumented environment variables
```

If clean Windows isolation is unavailable, host-based testing may be used for diagnostics, but it should be reported separately from clean-environment RC validation.

---

# Release Candidate Identity

Before testing, record:

```text
RC version
source commit
release tag or candidate tag
artifact filenames
artifact sizes
SHA-256 checksums
```

Verify the checksum of every artifact being tested.

The exact artifact set being validated must remain immutable.

Example:

```text
v0.1.0-rc1
    source archive
    Windows x64 archive
    Linux x64 archive
    checksum manifest
```

Any source or packaging change requires a new candidate identifier:

```text
rc1 → rc2
```

---

# Linux Requirements

Validate the documented Linux prerequisites:

```text
build-essential
CMake 3.11+
pkg-config
libre2-dev
libgit2-dev
```

Confirm that these requirements are sufficient for the documented build procedure.

If another package is required for an ordinary source build, determine whether it is:

```text
public build dependency
test-only dependency
CI-only dependency
developer-only dependency
```

Only public build dependencies should be added to the user-facing requirements.

---

# Windows Requirements

Validate the documented Windows prerequisites:

```text
MSVC 2022+
CMake 3.11+
```

Confirm that the documented MSVC installation contains all components required for the build.

If a specific Visual Studio workload or component is required, identify and document it.

Do not assume that components installed on a developer workstation exist in a clean Windows environment.

---

# Binary Artifact Validation

For each distributed binary artifact:

```text
1. start with an isolated environment

2. obtain the packaged RC artifact

3. verify SHA-256

4. extract into a new directory

5. run:
       ast-tool --version

6. run:
       ast-tool --help

7. perform basic semantic queries

8. test JSON output

9. test representative errors
```

Verify that the packaged executable does not depend on:

```text
the source checkout
the development build tree
developer-specific DLL/library paths
unpublished resource files
```

---

# Source Archive Validation

Validate the source archive separately from the Git checkout.

Procedure:

```text
extract source archive

install documented prerequisites

configure from a clean build directory

build Release configuration

run the resulting executable

perform a basic smoke test
```

Verify:

```text
source archive is complete

CMake configuration succeeds

Release build succeeds

no untracked development file is required

generated executable reports the correct version
```

A successful build directly from the development Git checkout does not substitute for this test.

---

# Documentation-Only Validation

During the main RC walkthrough, use only public release documentation.

Allowed sources include:

```text
README
installation instructions
CLI documentation
examples
troubleshooting documentation
release notes
```

Do not rely on:

```text
internal evaluation notes
phase reports
development scripts
implementation knowledge
undocumented commands
```

If undocumented knowledge is required to complete an ordinary workflow, record a documentation defect.

---

# New-User Walkthrough

Perform the following workflow on each supported platform:

```text
1. obtain the RC artifact

2. verify and extract it

3. install or build using public documentation

4. run --version

5. run --help

6. analyze a small repository

7. search for a known symbol

8. query references

9. query callers

10. query callees

11. request JSON output

12. intentionally run one invalid command

13. recover using the CLI help/error information
```

The walkthrough should succeed without internal project knowledge.

---

# Semantic Smoke Tests

RC validation is not another semantic research benchmark.

Only verify that the accepted release capabilities remain functional.

Test representative cases for:

```text
search
references
callers
callees
```

Also include representative regression checks for the stable semantic baseline:

```text
unique partial-FQN relationship resolution

receiver-type member relationships

declaration/definition body resolution
```

Do not rerun extensive historical evaluation unless a regression is discovered.

---

# JSON Validation

For representative commands:

```text
capture stdout
parse it using a real JSON parser
```

Verify:

```text
normal result

empty result

representative failure behavior

paths containing spaces
```

Diagnostics must not corrupt JSON output.

---

# Exit-Code Validation

Verify exit status from the actual packaged executable.

Include:

```text
successful command

unknown command

missing required argument

invalid repository/path

representative command failure
```

Record actual exit codes.

Do not rely solely on unit-test expectations.

---

# Path Validation

Test repositories and artifact locations containing:

```text
spaces
parentheses
non-ASCII characters
nested directories
```

Windows path/quoting behavior should receive particular attention.

If a failure occurs, distinguish between:

```text
ast-tool behavior

shell quoting

test harness behavior

external agent command construction
```

Do not classify a harness-only quoting issue as an ast-tool release defect.

---

# Repository Safety

Use disposable repositories for validation.

Before analysis:

```text
record repository state
```

After analysis:

```text
verify repository state
```

Where Git is available:

```text
git status --short
```

can be used before and after.

Read-only semantic analysis must not unexpectedly modify source-controlled files.

Unexpected repository modification is a release blocker.

---

# Repeated Invocation

Perform a lightweight repeated-use test.

For example:

```text
search      × 10
references  × 10
callers     × 10

mixed semantic command sequence
```

Look for:

```text
crashes
state leakage
stale results
temporary-file collisions
output corruption
```

This is a stability smoke test, not a performance benchmark.

---

# Runtime Dependency Check

Verify that packaged binaries have the expected runtime dependencies.

For Windows, inspect whether additional DLLs or runtime components are required.

For Linux, verify that expected shared-library dependencies are available through the documented environment.

A binary package must not depend on an undocumented development-machine library.

---

# Fresh-Environment Recheck

After the initial validation, perform a small second pass from a newly reset or recreated environment.

Repeat at minimum:

```text
artifact extraction
--version
--help
one semantic query
one JSON query
```

The purpose is to catch hidden state dependencies introduced during the first validation session.

---

# Known Limitations

Spot-check important documented limitations.

Do not attempt to fix them during RC validation unless they violate the documented release contract.

A limitation is acceptable when:

```text
behavior is understood
failure is safe
documentation is accurate
normal supported usage remains reliable
```

---

# Issue Classification

Every issue discovered during RC validation should be classified as:

```text
RC BLOCKER

RELEASE FIX

DOCUMENTATION FIX

KNOWN LIMITATION

ENVIRONMENT / HARNESS ISSUE

POST-RELEASE
```

Do not automatically fix every observed issue.

---

# RC Blockers

Examples include:

```text
packaged binary does not start

source archive cannot build using documented requirements

required runtime dependency is missing

embedded version is incorrect

artifact checksum does not match

ordinary supported command crashes

JSON output is malformed

supported path fails because of product behavior

analysis unexpectedly modifies the repository

public documentation cannot get a new user to the first successful query
```

---

# Non-Blockers

Examples include:

```text
minor documentation wording

cosmetic help inconsistency

unsupported advanced semantic edge case

small performance variation

package-manager feature request

agent routing inefficiency

test-harness-only problem
```

---

# RC Fix Procedure

When an RC blocker is confirmed:

```text
1. capture reproduction

2. reproduce from source

3. implement the smallest necessary fix

4. add regression coverage where appropriate

5. rerun the relevant release-quality gate

6. rebuild all affected artifacts

7. generate new checksums

8. create a new RC identifier

9. repeat affected RC validation
```

Never replace an already-tested RC artifact while retaining the same candidate identifier.

---

# Final Clean Pass

When no known RC blocker remains, validate one exact immutable artifact set.

Perform:

```text
Linux isolated-environment validation

Windows isolated-environment validation

checksum verification

binary/source installation validation

new-user walkthrough

semantic smoke tests

JSON smoke tests

exit-code smoke tests

path smoke tests

repository-safety test
```

Do not modify source code during the final clean pass.

---

# Validation Matrix

Record results independently for each platform.

```text
Test                         Linux       Windows
---------------------------------------------------
Artifact checksum
Artifact extraction
Version
Help
Source build
Binary startup
Search
References
Callers
Callees
JSON
Exit codes
Paths with spaces
Non-ASCII paths
Repository safety
Fresh-environment recheck
```

Use:

```text
PASS
FAIL
N/A
NOT VALIDATED
```

Do not hide platform-specific gaps behind an aggregate result.

---

# Validation Record

For each platform, retain:

```text
OS/version
architecture
toolchain versions
artifact tested
checksum result
source-build result
binary result
new-user walkthrough result
semantic smoke result
JSON result
exit-code result
path result
repository-safety result
issues discovered
```

Do not include:

```text
local machine names
local usernames
private filesystem paths
local WSL distribution names
developer-specific directory layouts
```

Local environment identifiers are implementation details of the validation process and should not appear in the public release record.

---

# Final Recommendation

End the validation note with one of:

```text
PASS — RC APPROVED FOR FINAL RELEASE

PASS WITH KNOWN LIMITATIONS — RC APPROVED

BLOCKED — NEW RC REQUIRED

NOT FULLY VALIDATED
```

---

# Acceptance Criteria

The RC is approved when:

```text
1. one exact immutable artifact set has been validated

2. artifact checksums are correct

3. Linux validation succeeds in an isolated Debian-based environment

4. documented Linux prerequisites are sufficient

5. Windows validation succeeds in an isolated Windows environment

6. documented Windows prerequisites are sufficient

7. source distribution builds independently of the development checkout

8. packaged binaries work independently of the development environment

9. --help and --version work

10. a new-user workflow succeeds using only public documentation

11. search/references/callers/callees smoke tests pass

12. stable semantic behavior shows no release-time regression

13. JSON output is valid

14. exit codes behave as documented

15. important path/quoting cases have been validated

16. analysis does not unexpectedly modify repositories

17. no hidden environment-state dependency remains

18. all discovered issues have been classified

19. no unresolved RC blocker remains

20. the exact approved RC artifacts remain unchanged
```

---

# Final Principle

Release Candidate validation should answer:

```text
If these exact files were published today,
could a new user successfully use them
with only the documented requirements and documentation?
```

The RC is approved because the **release artifacts themselves** work.

It is not approved merely because the development repository works.
