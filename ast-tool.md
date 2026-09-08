# Phase 16 — Release Candidate Validation

## Objective

Phase 16 validates the actual Release Candidate artifacts produced by Phase 15 in disposable or clean environments.

The goal is not to improve the product.

The goal is to answer:

```text
Can another user obtain the Release Candidate,
follow the public documentation,
and successfully use the released artifacts
without relying on the development environment?
```

Phase 16 is an end-to-end release rehearsal.

Test the **actual packaged artifacts**, not the developer build tree.

---

# Core Rule

Treat the Release Candidate as if it had already been publicly released.

Do not:

```text
use files from the development build directory

use undocumented environment variables

use locally installed project-specific dependencies unless documented

patch the RC artifact manually

modify semantic behavior

add new features

change routing behavior

perform opportunistic cleanup
```

If a defect is found:

```text
record
→ classify
→ fix in source if release-blocking
→ rebuild a new RC
→ restart affected validation
```

Do not silently repair the installed RC environment and continue as though the artifact were correct.

---

# Validation Environments

Use two independent environments.

## Linux RC Environment

Use:

```text
wsl -d debian2 -u taqu
```

This environment is disposable for Phase 16 purposes.

It may be modified freely.

You may:

```text
install/remove packages

delete previous ast-tool installations

delete repositories

modify shell configuration

create temporary users/files

clear caches

overwrite test directories

reinstall dependencies
```

Do not rely on pre-existing ast-tool development state inside this WSL instance.

Before RC validation, remove or isolate any previous ast-tool installation that could affect the test.

---

## Windows RC Environment

Preferred environment:

```text
Windows Sandbox
```

Use Windows Sandbox as the disposable Windows RC validation environment whenever available.

The Sandbox should represent a fresh Windows user environment.

Closing the Sandbox after a validation run should discard its state.

Do not use the host development environment as evidence of clean Windows RC compatibility.

---

# Windows Sandbox Requirements

Before using Windows Sandbox, verify that it is available and enabled.

If Windows Sandbox cannot be used because of host edition, virtualization, policy, or other environment restrictions:

```text
do not silently substitute the developer machine
```

Instead classify Windows clean-environment validation as:

```text
NOT FULLY VALIDATED
```

A host-based fallback may still be used for diagnostic testing, but it is not equivalent to disposable RC validation.

---

# Windows Sandbox Mapping Policy

Where practical, expose RC input artifacts from the host using a read-only mapped folder.

Conceptual structure:

```text
Host:
    rc-input/
        ast-tool-<version>-windows-x64.zip
        SHA256SUMS
        README.md

Sandbox:
    mapped read-only input
```

Perform extraction and testing inside a separate writable Sandbox directory.

Do not run directly from a writable host-shared development directory.

The goal is:

```text
immutable RC input
→ clean Sandbox extraction
→ validation
```

---

# Optional Windows Sandbox Configuration

A `.wsb` configuration may be created for repeatable RC validation.

Prefer settings that:

```text
map only the RC input directory

make the mapping read-only where possible

enable networking only when required

avoid mapping the project source tree

avoid exposing unrelated host directories
```

Do not map the entire developer workspace into the Sandbox.

If internet access is needed to install MSVC/CMake or other documented prerequisites, networking may be enabled.

Record this requirement.

---

# Phase 16a — RC Identity Verification

Before testing either platform, record the exact RC.

Include:

```text
version

Git tag or candidate tag

source commit

artifact filenames

artifact sizes

SHA-256 hashes
```

Verify checksums before installation/extraction.

Example outcome:

```text
RC version:
    v0.1.0-rc1

Source:
    <commit>

Artifacts:
    ast-tool-0.1.0-rc1-windows-x64.zip
    ast-tool-0.1.0-rc1-linux-x64.tar.gz
    ast-tool-0.1.0-rc1-source.tar.gz
    SHA256SUMS
```

Do not validate an artifact whose provenance is unclear.

---

# Phase 16b — Documentation-Only Rule

During end-to-end validation, use only public release documentation.

The tester may use:

```text
README

installation instructions

command reference

troubleshooting documentation

release notes
```

Do not rely on:

```text
internal phase documents

development notes

evaluation logs

knowledge of implementation internals

unpublished build commands
```

If documentation is insufficient to complete a normal operation, record a documentation defect.

---

# Phase 16c — Linux Clean-State Preparation

Inside:

```text
wsl -d debian2 -u taqu
```

inspect the initial environment.

Record:

```text
Debian version

architecture

compiler state

CMake version if installed

pkg-config state

RE2/libgit2 package state

existing ast-tool executable if any
```

Then remove or neutralize previous ast-tool-specific state.

Examples:

```text
old ast-tool binary

old source checkout used for testing

old test repositories

AST Tool-specific environment variables

old temporary indexes/caches if applicable
```

Do not unnecessarily erase unrelated user data.

The WSL environment may be dirty globally, but the ast-tool RC test itself must start from a known state.

---

# Phase 16d — Linux Requirements Installation

Using the public documentation, verify the Linux requirements:

```text
build-essential
CMake 3.11+
pkg-config
libre2-dev
libgit2-dev
```

For Debian-based testing, install using the documented package procedure.

Confirm:

```text
compiler available

cmake >= 3.11

pkg-config available

RE2 development package available

libgit2 development package available
```

If installation requires an undocumented package, record a release issue.

---

# Phase 16e — Linux Binary RC Test

If a Linux binary artifact is part of the release, test the exact archive.

Procedure:

```text
copy/download RC artifact

verify SHA-256

extract into a new directory

do not modify extracted binary

run --version

run --help
```

Verify:

```text
binary launches

version matches RC

help works

no missing shared-library error

no unexpected environment dependency
```

---

# Phase 16f — Linux Source RC Test

Test the source archive independently of the Git repository.

Procedure:

```text
extract source archive into a clean directory

follow README installation/build instructions exactly

configure

build Release configuration

run resulting binary
```

Verify:

```text
source archive is complete

CMake configure succeeds

build succeeds

no untracked developer file is required

resulting binary reports correct version
```

A successful build from the Git checkout does not substitute for this test.

---

# Phase 16g — Windows Clean Environment Preparation

Start a fresh Windows Sandbox session.

Record:

```text
Windows version

architecture

Sandbox availability

initial MSVC availability

initial CMake availability
```

The first run should reveal whether the published prerequisites are sufficient.

Do not assume Visual Studio development components from the host are available inside the Sandbox.

Host-installed applications are not evidence of Sandbox availability.

---

# Phase 16h — Windows Prerequisite Verification

The documented Windows requirements are:

```text
MSVC 2022+
CMake 3.11+
```

Validate the actual installation procedure required to obtain them in the clean environment.

Confirm that the installed MSVC workload contains everything needed by ast-tool.

If the documentation merely says:

```text
MSVC 2022+
```

but a specific Visual Studio workload or component is required, identify it.

Examples that may need explicit verification:

```text
Desktop development with C++

MSVC v143 toolset

Windows SDK

CMake tools
```

Do not add these requirements to documentation unless the build actually depends on them.

The purpose of this test is to discover hidden Windows prerequisites.

---

# Phase 16i — Windows Binary RC Test

If a Windows prebuilt binary is distributed:

```text
start fresh Sandbox

copy/map RC archive

verify checksum where tooling permits

extract into Sandbox-local directory

run ast-tool.exe --version

run ast-tool.exe --help
```

Verify:

```text
binary starts

version matches RC

required DLLs are available

no development-tree dependency exists

no PATH assumption exists
```

Do not install the project's source tree simply to make the binary work.

---

# Phase 16j — Windows Source RC Test

If source build is part of the supported release path, test the source archive separately.

From a fresh or appropriately reset Sandbox:

```text
install documented prerequisites

extract source archive

open appropriate MSVC build environment if required

configure with CMake

build Release configuration

run resulting ast-tool.exe
```

Verify:

```text
clean configure

clean build

correct release executable

correct --version

no undocumented dependency
```

---

# Phase 16k — Windows Path / Quoting Regression

Explicitly repeat the historical Windows path/quoting cases.

Create RC test locations containing:

```text
spaces

parentheses

non-ASCII characters

nested directories
```

Examples:

```text
C:\Users\WDAGUtilityAccount\Desktop\AST Tool RC\

C:\Users\WDAGUtilityAccount\Desktop\test (1)\

C:\Users\WDAGUtilityAccount\Desktop\日本語\repository\
```

Run representative commands against repositories in these locations.

Distinguish failures caused by:

```text
ast-tool argument parsing

PowerShell quoting

cmd.exe quoting

test harness

Coding Agent command generation
```

Do not classify a harness-only quoting problem as an ast-tool product defect.

---

# Phase 16l — First-Run User Journey

On each supported platform, perform the same minimal new-user journey.

Using only public documentation:

```text
1. obtain the RC

2. verify/extract it

3. install/build if required

4. run --version

5. run --help

6. analyze a small repository

7. search for a known symbol

8. run references

9. run callers

10. run callees

11. request JSON output

12. intentionally invoke one invalid command

13. recover using the documented error/help behavior
```

Record friction or undocumented assumptions.

---

# Phase 16m — Representative Repository Set

Use at least three repository classes if practical:

```text
small repository

medium representative repository

realistic repository
```

The purpose is not another semantic benchmark.

The purpose is to ensure the packaged RC behaves normally outside artificial smoke fixtures.

Use repositories that are safe to modify/delete in the disposable environments.

---

# Phase 16n — Semantic Release Smoke Test

Verify only the stable release capabilities.

Include representative checks for:

```text
search

references

callers

callees
```

Also verify representative Phase 8 behavior:

```text
unique partial-FQN relationship resolution

receiver-type member relationship

declaration/definition body resolution
```

Do not rerun the entire Phase 8/9 research evaluation unless a regression appears.

---

# Phase 16o — JSON End-to-End Test

For representative commands:

```text
run JSON mode

capture stdout

parse stdout with a real JSON parser

verify stderr does not corrupt stdout
```

Test:

```text
normal result

empty result

not-found/error case where supported

path containing spaces
```

The packaged RC must preserve the Phase 12/13 JSON contract.

---

# Phase 16p — Exit Code End-to-End Test

From an actual shell, verify:

```text
successful command

unknown command

missing argument

invalid repository/path

fatal failure where safely reproducible
```

Record the exact exit code.

Do not infer exit behavior from unit tests alone.

---

# Phase 16q — Installation Isolation Test

Check whether the RC relies on accidental machine state.

Examples:

```text
developer PATH entries

previous ast-tool installation

CMAKE_PREFIX_PATH

custom LIBRARY_PATH

custom INCLUDE paths

Git checkout-relative resources

user-specific config files
```

The test should still work after ast-tool-specific environment customizations are removed.

---

# Phase 16r — Repository Safety Test

Before representative analysis:

```text
record repository state
```

After queries:

```text
verify repository state again
```

Read-only analysis must not unexpectedly modify source-controlled files.

In disposable repositories, use:

```text
git status --short
```

before and after where applicable.

Unexpected source modification is a release blocker.

---

# Phase 16s — Repeat Invocation Test

Run representative queries repeatedly from the packaged RC.

For example:

```text
search × 10

references × 10

callers × 10

mixed command sequence
```

Look for:

```text
crashes

state leakage

stale results

temporary file problems

output corruption
```

This is a smoke test, not a performance benchmark.

---

# Phase 16t — Restart / Fresh Environment Test

Linux:

Start a new WSL shell and verify that the installed/built RC still behaves as documented.

Windows:

Close Windows Sandbox completely.

Start a new Sandbox.

Repeat a small subset of the installation/extraction and smoke process.

The Windows restart test is particularly useful because a new Sandbox instance should not contain state from the previous validation run.

---

# Phase 16u — RC Artifact Independence

Verify that no operation requires access to:

```text
original development repository

developer build directory

Phase evaluation repositories

unpublished scripts

local dependency caches that users would not have
```

If such access is required, the RC is incomplete.

---

# Phase 16v — Release Notes Verification

Read the RC release notes as a user.

Verify that they accurately state:

```text
version

supported platforms

requirements

installation path

major capabilities

known limitations
```

Do not include internal phase history unless intentionally part of the release notes.

---

# Phase 16w — Known Limitation Confirmation

Spot-check important documented limitations.

The objective is not to fix them.

Verify that documentation does not imply support that does not exist.

A known limitation is acceptable when:

```text
behavior is understood

failure is safe

documentation is accurate

normal supported use remains reliable
```

---

# Phase 16x — RC Defect Classification

Every issue discovered during RC testing must be classified as:

```text
RC BLOCKER

RELEASE FIX

DOCUMENTATION FIX

KNOWN LIMITATION

ENVIRONMENT / HARNESS ISSUE

POST-RELEASE
```

Do not automatically fix every issue.

---

# RC Blockers

Examples:

```text
packaged binary does not start

source archive cannot build using documented requirements

missing runtime dependency

wrong embedded version

checksum mismatch

ordinary command crashes

JSON output is invalid

supported Windows path fails because of product defect

supported platform cannot complete installation

analysis unexpectedly modifies repository

documentation cannot get a clean user to first successful query
```

---

# Non-Blockers

Examples:

```text
minor wording issue

cosmetic help inconsistency

unsupported advanced semantic edge case

small performance variation

additional package-manager request

agent-routing inefficiency

test harness-specific quoting problem
```

Use judgment when a documentation issue prevents basic installation or operation; such an issue may still block the release.

---

# Phase 16y — RC Fix Procedure

If an RC blocker is found:

```text
1. record reproduction

2. reproduce against source revision

3. identify minimum fix

4. implement fix in source

5. add regression test where appropriate

6. rerun relevant Phase 12/13/14/15 gate

7. create new RC artifact
```

Increment the RC identifier:

```text
rc1 → rc2
```

Do not silently replace an artifact while keeping the same RC identifier.

---

# RC Immutability Rule

Once an RC artifact has been generated and tested:

```text
never modify that artifact in place
```

Any source or packaging change requires a new RC.

For example:

```text
v0.1.0-rc1
    immutable

fix applied

v0.1.0-rc2
    new artifacts
    new checksums
```

This keeps test evidence traceable.

---

# Phase 16z — Final Clean RC Pass

After all blockers are resolved, perform one final pass against a single immutable RC set.

Do not make code changes during this pass.

Run:

```text
Linux disposable-environment validation

Windows Sandbox validation

artifact checksum verification

installation/build verification

first-run journey

semantic smoke

JSON smoke

exit-code smoke

repository-safety smoke
```

The exact artifact set that passes this gate becomes the release candidate eligible for final release.

---

# Linux Validation Report

Record:

```text
WSL distribution:
    debian2

User:
    taqu

Debian version

architecture

requirements installed

artifact tested

checksum result

source build result

binary result

first-run journey

semantic smoke

JSON smoke

exit-code smoke

repository safety

issues
```

---

# Windows Validation Report

Record:

```text
environment:
    Windows Sandbox

Windows version

architecture

MSVC version

CMake version

artifact tested

checksum result

source build result if applicable

binary result

path/quoting tests

first-run journey

semantic smoke

JSON smoke

exit-code smoke

repository safety

issues
```

If Windows Sandbox was unavailable, state that explicitly.

Do not label a host-only test as equivalent to Sandbox validation.

---

# Cross-Platform Result Matrix

Produce:

```text
Test                     Linux     Windows
------------------------------------------------
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
Fresh-environment rerun
```

Use:

```text
PASS
FAIL
N/A
NOT VALIDATED
```

Do not hide platform-specific gaps behind one aggregate status.

---

# Expected Deliverables

## 1. RC Identity Record

```text
version
tag
commit
artifacts
SHA-256
```

## 2. Linux RC Validation Report

Use the disposable `debian2` WSL environment.

## 3. Windows RC Validation Report

Prefer Windows Sandbox.

## 4. Documentation-Only Walkthrough Report

Record every place where undocumented knowledge was required.

## 5. Cross-Platform Matrix

Report all relevant checks independently.

## 6. RC Issue List

Use:

```text
Issue
Platform
Classification
Severity
Reproduction
Resolution
Retest status
```

## 7. Final RC Commit and Artifact Manifest

Record the exact source and artifact set that passed validation.

## 8. Phase 16 Final Recommendation

End with one of:

```text
PASS — RC APPROVED FOR FINAL RELEASE

PASS WITH KNOWN LIMITATIONS — RC APPROVED

BLOCKED — NEW RC REQUIRED

NOT FULLY VALIDATED
```

---

# Acceptance Criteria

Phase 16 is complete when:

```text
1. one exact immutable RC artifact set has been identified

2. SHA-256 verification succeeds

3. Linux RC validation succeeds in:
       wsl -d debian2 -u taqu

4. Linux documented requirements are sufficient:
       build-essential
       CMake 3.11+
       pkg-config
       libre2-dev
       libgit2-dev

5. Windows RC is tested in Windows Sandbox when available

6. Windows documented requirements are sufficient:
       MSVC 2022+
       CMake 3.11+

7. source archives build independently of the Git checkout

8. packaged binaries start independently of the development environment

9. --help and --version work

10. first-run workflow succeeds using only public documentation

11. search/references/callers/callees smoke tests succeed

12. stable Phase 8 behavior has no release-time regression

13. JSON output parses correctly end to end

14. exit codes behave as documented

15. Windows path/quoting behavior is explicitly validated

16. analysis does not unexpectedly modify repositories

17. restarting/refreshing the disposable environment does not reveal hidden state dependencies

18. every discovered issue is classified

19. no unresolved RC blocker remains

20. the exact RC that passed validation is preserved unchanged
```

---

# Out of Scope

Do not use Phase 16 for:

```text
new features

semantic improvements

routing experiments

agent optimization

CLI redesign

new packaging systems

performance optimization

new platform support

refactoring

dependency modernization
```

If a useful improvement is discovered but is not required for release correctness:

```text
POST-RELEASE
```

---

# Final Principle

Phase 16 should answer:

```text
If we published these exact files today,
would a new Windows or Linux user be able to use them successfully?
```

The RC is not approved because:

```text
the source tree works
```

It is approved only when:

```text
the actual release artifacts
+
the actual public documentation
+
the declared prerequisites
```

work together in clean or disposable environments.
