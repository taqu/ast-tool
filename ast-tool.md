# Phase 15 — Packaging / Distribution

## Objective

Phase 15 prepares the release candidate for actual distribution.

The goal is:

```text id="7fmdfh"
make the frozen, documented release candidate
easy to obtain,
build,
verify,
and distribute
on the supported platforms
```

This phase focuses on:

```text id="kg9qj6"
release artifacts
source distribution
binary packaging where appropriate
release build reproducibility
checksums
version embedding
GitHub Release readiness
CI release workflow
dependency documentation
license / notice inclusion
```

This is not a feature-development phase.

Do not introduce new semantic behavior, routing changes, CLI redesign, or new language support.

---

# Baseline

Start from the Phase 14 documentation-complete release candidate.

Record:

```text id="7s20bl"
branch
commit hash
target release version
Phase 14 result
supported platforms
known limitations
```

The semantic and CLI baseline remain frozen.

Any implementation change made during Phase 15 must be required for packaging, distribution, installation, or release correctness.

---

# Supported Build Platforms

The initial release supports the following build environments.

## Windows

### Requirements

```text id="g24n5x"
MSVC 2022+
CMake 3.11+
```

The Windows build must work with the documented MSVC toolchain.

Do not assume MinGW, Clang-cl, or other toolchains are supported unless they are separately tested and documented.

---

## Linux

### Requirements

```text id="ixboay"
build-essential
CMake 3.11+
pkg-config
libre2-dev
libgit2-dev
```

The Linux build instructions must clearly state these dependencies.

Where Debian/Ubuntu package names are used, make that scope explicit.

Do not imply that the exact package names apply to every Linux distribution.

---

# Phase 15a — Packaging Strategy

Define the initial distribution model before implementing release automation.

Decide which of the following will be part of the first release:

```text id="glqlee"
source archive

Windows binary archive

Linux binary archive

GitHub Release artifacts

checksums

package-manager distribution
```

Prefer the smallest reliable release surface.

Do not add package-manager support merely because it may be useful later.

For the initial release, a valid strategy may be:

```text id="au0uv1"
source release
+
Windows release binary
+
Linux release binary
+
checksums
+
GitHub Release
```

if those binaries can be produced reliably.

If portable binary distribution is not practical on a platform, document source build as the supported path instead.

---

# Phase 15b — Release Build Configuration

Define and verify the canonical release build.

Use an explicit release configuration.

Conceptually:

```text id="42p2ba"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Adjust commands to match the actual project.

On multi-configuration generators such as Visual Studio, ensure the release configuration is explicitly selected.

Do not rely on a developer's previous CMake cache.

Release builds must be validated from a clean build directory.

---

# Phase 15c — Windows Release Build

Test the Windows release build from a clean checkout using:

```text id="vfqbf9"
MSVC 2022+
CMake 3.11+
```

Document the exact supported procedure.

Verify:

```text id="w5zf55"
CMake configure succeeds

Release build succeeds

expected executable is produced

binary starts successfully

--help works

--version works if supported

basic semantic smoke test passes

JSON smoke test passes
```

Record:

```text id="shafus"
Windows version
architecture
MSVC version
CMake version
generator
build command
artifact path
artifact size
```

Prefer x64 as the initial architecture if that is the tested release target.

Do not claim additional architectures unless verified.

---

# Phase 15d — Linux Release Build

Test the Linux release build from a clean environment.

Required packages:

```text id="qfw75o"
build-essential
cmake
pkg-config
libre2-dev
libgit2-dev
```

Document the minimum CMake requirement separately:

```text id="e1r20y"
CMake 3.11+
```

For Debian/Ubuntu-based environments, the prerequisite installation may resemble:

```text id="l74bhe"
sudo apt update
sudo apt install build-essential cmake pkg-config libre2-dev libgit2-dev
```

Verify the exact command in the target test environment before publishing it.

Then verify:

```text id="bsj87a"
CMake configure succeeds

Release build succeeds

expected executable is produced

binary starts successfully

--help works

--version works if supported

basic semantic smoke test passes

JSON smoke test passes
```

Record:

```text id="i1at1d"
distribution
distribution version
architecture
compiler version
CMake version
libre2 version where practical
libgit2 version where practical
artifact path
artifact size
```

---

# Phase 15e — Dependency Verification

Verify that documented dependencies match actual build behavior.

## Windows

Confirm whether the release requires only:

```text id="ivj7ix"
MSVC 2022+
CMake 3.11+
```

from the user's perspective.

Identify any additional runtime or build dependencies that are currently obtained through:

```text id="f30im2"
vendored sources
CMake FetchContent
submodules
prebuilt libraries
system installation
```

Do not leave hidden prerequisites undocumented.

---

## Linux

Confirm that:

```text id="n6lxdz"
build-essential
CMake 3.11+
pkg-config
libre2-dev
libgit2-dev
```

are sufficient for the intended build environment.

If additional required packages are discovered, classify whether they are:

```text id="7w6gsw"
real public prerequisite
CI-only requirement
test-only requirement
developer-only requirement
```

Only public build requirements belong in installation documentation.

---

# Phase 15f — Runtime Dependency Audit

Inspect the generated release binaries for runtime dependencies.

On Windows, identify whether the executable requires:

```text id="vv3uku"
MSVC runtime DLLs
additional project DLLs
third-party DLLs
```

On Linux, inspect dynamically linked libraries where practical.

Confirm that runtime dependency expectations match the intended distribution model.

A binary archive is not useful if required shared libraries are omitted or undocumented.

Do not attempt to force static linking unless there is a clear release requirement.

---

# Phase 15g — Artifact Layout

Define a simple, predictable archive layout.

For example:

```text id="n6urz5"
ast-tool-<version>-windows-x64/
    ast-tool.exe
    README.md
    LICENSE

ast-tool-<version>-linux-x64/
    ast-tool
    README.md
    LICENSE
```

Add other files only when useful.

Possible additions:

```text id="9trhd3"
CHANGELOG.md
NOTICE
examples/
```

Avoid shipping:

```text id="f3us3h"
build directories
object files
CMake cache
test output
evaluation data
internal research notes
temporary files
developer-only scripts
```

---

# Phase 15h — Artifact Naming

Use deterministic artifact names.

Recommended pattern:

```text id="t6wd1g"
ast-tool-<version>-windows-x64.zip

ast-tool-<version>-linux-x64.tar.gz

ast-tool-<version>-source.tar.gz
```

Adjust the project name and architecture suffixes to match the actual release.

Do not use ambiguous names such as:

```text id="bsu4l6"
release.zip
latest.zip
build.zip
```

The filename should identify:

```text id="5jeizt"
project
version
platform
architecture
```

where applicable.

---

# Phase 15i — Version Embedding

Ensure the built tool reports the intended release version.

Prefer:

```text id="9r31k0"
ast-tool --version
```

with output similar to:

```text id="16w0vi"
ast-tool 0.1.0
```

The version should come from one authoritative source where practical.

Avoid manually maintaining unrelated version strings in multiple files.

Verify that:

```text id="e1dwh9"
source version
CLI version
artifact filename
Git tag
GitHub Release version
```

all agree.

A release with mismatched version identifiers should not be published.

---

# Phase 15j — Source Archive

Prepare a clean source distribution.

The source archive should contain everything needed to build according to the documented requirements.

Verify that it includes:

```text id="31nfdd"
source code
CMake files
required vendored files
LICENSE
README
required generated/configuration inputs
```

Verify that it does not depend on untracked local files.

Test the source archive itself:

```text id="2m9kn1"
extract into clean directory
follow documented prerequisites
configure
build
run smoke test
```

Do not assume that a Git checkout test proves the source archive is complete.

---

# Phase 15k — Binary Archive Validation

If prebuilt binaries are distributed, test the exact packaged archive rather than only the build-tree executable.

For each binary artifact:

```text id="znkuaa"
download/copy archive

extract into clean location

run binary from extracted archive

run --help

run --version

run basic semantic query

run JSON query
```

This catches missing runtime files and packaging mistakes.

---

# Phase 15l — Checksums

Generate cryptographic checksums for public release artifacts.

Prefer:

```text id="zn2ya5"
SHA-256
```

Produce a checksum file such as:

```text id="mm6ur2"
SHA256SUMS
```

containing all distributed archives.

Verify the checksums after artifact generation.

The release workflow should make it clear which checksum corresponds to which file.

---

# Phase 15m — Release Reproducibility

The goal is not necessarily byte-for-byte reproducible builds.

The minimum requirement is **procedural reproducibility**:

```text id="6c21rd"
same source revision
+
documented toolchain
+
documented build procedure
→ valid equivalent release artifact
```

Record:

```text id="ege9nk"
source commit
toolchain versions
CMake version
build configuration
artifact generation procedure
```

If builds are not byte-identical, do not describe them as reproducible builds in the strict sense.

---

# Phase 15n — CI Release Workflow

If the project uses CI, create or verify a release workflow.

The workflow should ideally:

```text id="taqzgn"
checkout exact tag

configure clean release build

build

run release smoke tests

package artifacts

generate checksums

publish or stage artifacts
```

Prefer release automation over undocumented manual steps.

However, do not introduce a large CI redesign solely for the initial release.

The workflow should remain understandable and debuggable.

---

# Phase 15o — Tag-Triggered Release Behavior

If Git tags trigger packaging, define the expected tag pattern.

For example:

```text id="v5uzwy"
v0.1.0
```

Ensure that arbitrary branches or commits do not accidentally create public releases.

A release workflow should distinguish:

```text id="ub1n2p"
normal CI
release candidate
final tagged release
```

where applicable.

---

# Phase 15p — GitHub Release Preparation

Prepare the release process for a GitHub Release or equivalent release hosting system.

The release should contain:

```text id="vnr89b"
release version

release notes

supported platform information

installation/build summary

known limitations

binary/source artifacts

checksums
```

Do not publish during Phase 15 unless the project explicitly intends Phase 15 to perform the final release.

The preferred outcome is a fully validated, publishable artifact set for the later release-candidate phase.

---

# Phase 15q — License / Notice Packaging

Verify that distributed artifacts satisfy license requirements.

At minimum:

```text id="7qizq1"
include project LICENSE where appropriate

include required third-party notices

do not omit required attribution files
```

Inspect dependencies such as:

```text id="f4y2e4"
RE2
libgit2
other bundled dependencies
```

for redistribution requirements.

Do not guess licensing obligations.

If redistribution requirements are unclear, classify the issue as a release blocker until resolved.

---

# Phase 15r — Debug Artifact Exclusion

Verify that public archives do not unintentionally contain:

```text id="w75owr"
debug logs
core dumps
temporary files
test repositories
evaluation logs
credentials
environment files
local paths
developer configuration
```

Also inspect source archives for unintended generated files.

Treat accidental credential inclusion as a critical release blocker.

---

# Phase 15s — Symbol / Debug File Policy

Decide whether debugging symbols are distributed.

Possible policy:

```text id="9bnu2w"
normal release archive
    stripped/release binary where appropriate

debug symbols
    separate artifact if needed
```

Do not make this more complex than necessary.

For an initial release, it is acceptable to distribute only the normal release executable unless debugging symbols are operationally useful.

---

# Phase 15t — Installation Documentation Alignment

Update Phase 14 installation documentation to reflect the actual packaged release.

The documented requirements must include:

## Windows

```text id="itj23z"
Requirements:
- MSVC 2022+
- CMake 3.11+
```

## Linux

```text id="wrnalo"
Requirements:
- build-essential
- CMake 3.11+
- pkg-config
- libre2-dev
- libgit2-dev
```

Ensure all commands in the release documentation match the tested packaging workflow.

Do not leave development-only installation steps as the primary public path when a release artifact now exists.

---

# Phase 15u — Unsupported Distribution Methods

Do not implement additional distribution systems unless already planned and justified.

Examples to defer by default:

```text id="xf1qxg"
Homebrew

Scoop

winget

Chocolatey

APT repository

RPM repository

Snap

Flatpak

Docker image

Conan package

vcpkg port
```

These can be post-release improvements.

The first release should prioritize reliable artifacts over broad distribution coverage.

---

# Phase 15v — Package-Manager Decision Record

Even if package-manager distribution is deferred, record the decision.

For example:

```text id="kbal96"
Homebrew
    POST-RELEASE

Scoop
    POST-RELEASE

winget
    POST-RELEASE
```

This prevents package-manager work from expanding Phase 15 unexpectedly.

---

# Phase 15w — Release Artifact Smoke Matrix

Test every artifact intended for distribution.

Create a matrix such as:

```text id="01jj6y"
Artifact
Platform
Clean extraction
Starts
--help
--version
Semantic smoke
JSON smoke
Status
```

An artifact that has not been tested after packaging is not release-qualified.

---

# Phase 15x — Release Security Sanity Check

Perform a narrow release-focused security sanity check.

Verify:

```text id="o7irhs"
no secrets in artifacts

no private keys

no access tokens

no unexpected credentials

no internal absolute paths where avoidable

no writable executable content from temporary directories

downloads/dependencies are obtained through expected sources
```

Do not turn Phase 15 into a broad security-audit project.

Focus on packaging and distribution risks.

---

# Phase 15y — Final Artifact Rebuild

Before closing Phase 15, rebuild release artifacts from the final source commit.

Do not reuse artifacts produced before the final packaging changes.

The final process should be:

```text id="taedg3"
clean checkout

exact release commit

clean configure

release build

tests / smoke tests

package

checksum

artifact verification
```

Record the exact commit and generated artifact names.

---

# Release Blocker Policy

Treat the following as release blockers:

```text id="wvaskq"
documented clean build fails

required dependency is missing from documentation

release archive cannot execute

required runtime library is missing

wrong version embedded in binary

artifact/version/tag mismatch

malformed archive

source archive cannot rebuild

checksum mismatch

license/notice requirement unresolved

supported platform artifact fails smoke test

release workflow publishes incorrect artifacts

secret or private data included in release package
```

Do not block the release for:

```text id="tev75p"
missing package-manager integration

absence of installer GUI

lack of automatic update mechanism

lack of Docker package

lack of additional architectures

minor archive-size optimization

cosmetic artifact-layout preferences
```

unless they are explicitly part of the release scope.

---

# Fix Policy

When a packaging issue is discovered:

```text id="fuyuk8"
identify packaging defect

make smallest necessary fix

rebuild from clean state

repackage

rerun artifact smoke tests
```

Do not patch an already-generated archive manually and call it final.

The release artifact must be reproducible from the source revision and documented build process.

---

# Expected Deliverables

## 1. Distribution Plan

Document:

```text id="0plq8w"
release artifact types

supported platforms

supported architectures

source vs binary distribution

deferred package managers
```

---

## 2. Build Requirement Matrix

Include at minimum:

```text id="9pmrkr"
Platform | Requirements
Windows  | MSVC 2022+, CMake 3.11+
Linux    | build-essential, CMake 3.11+, pkg-config, libre2-dev, libgit2-dev
```

Add only requirements verified by the build.

---

## 3. Release Build Report

For each supported platform record:

```text id="gmhbxw"
OS
architecture
toolchain
CMake version
build command
result
artifact
```

---

## 4. Release Artifact Manifest

Produce a manifest such as:

```text id="1oxgz4"
Filename
Platform
Architecture
Type
Size
SHA-256
Status
```

---

## 5. Runtime Dependency Report

Document required runtime libraries or state that no additional shipped runtime files are required, if verified.

---

## 6. Source Archive Verification

Record:

```text id="vgosqp"
archive name
source commit
clean extraction result
clean configure result
clean build result
smoke test result
```

---

## 7. Binary Archive Verification

For every distributed binary archive record:

```text id="kwl7hu"
archive
target platform
extraction
startup
--help
--version
semantic smoke
JSON smoke
result
```

---

## 8. CI / Release Workflow Report

Record:

```text id="2jrrlo"
workflow file

trigger

build matrix

packaging steps

checksum generation

publication/staging behavior
```

If release creation remains manual, document the exact manual procedure instead.

---

## 9. License / Notice Verification

Record:

```text id="5oalg6"
project license included

third-party notices checked

redistribution concerns

unresolved issues
```

---

## 10. Final Packaging Commit

Record:

```text id="16e3rb"
branch
commit hash
target version
artifact names
working tree status
```

---

## 11. Phase 15 Final Recommendation

End with one of:

```text id="u3egv0"
PASS — READY FOR RELEASE CANDIDATE

PASS WITH DOCUMENTED DISTRIBUTION LIMITATIONS

BLOCKED — PACKAGING / DISTRIBUTION ISSUE REMAINS
```

Do not declare PASS while a planned release artifact is known to be broken or incomplete.

---

# Acceptance Criteria

Phase 15 is complete when:

```text id="soii6a"
1. supported release platforms are explicitly defined

2. Windows requirements are documented and verified:
       MSVC 2022+
       CMake 3.11+

3. Linux requirements are documented and verified:
       build-essential
       CMake 3.11+
       pkg-config
       libre2-dev
       libgit2-dev

4. clean release builds succeed for every claimed supported platform

5. every intended release artifact can be generated from the frozen source revision

6. every packaged binary artifact passes post-packaging smoke tests

7. source distribution can be rebuilt from a clean extraction

8. version information is consistent across source, binary, artifact names, and release metadata

9. SHA-256 checksums are generated and verified

10. required runtime dependencies are understood

11. required license / notice material is included

12. no development-only or sensitive files are present in public artifacts

13. release automation or the manual release procedure is documented and repeatable

14. Phase 14 installation documentation matches the real distribution process

15. no unresolved packaging/distribution release blocker remains
```

---

# Out of Scope

Do not use Phase 15 for:

```text id="9srekh"
semantic capability changes

routing optimization

CLI redesign

documentation restructuring unrelated to packaging

new language support

general performance optimization

major dependency upgrades

new package-manager ecosystems

automatic update mechanisms

installer GUI development
```

Defer those unless required by the defined initial release scope.

---

# Final Principle

Phase 15 should answer:

```text id="l0shrv"
Can a release artifact be produced from the frozen source,
given to another person,
and successfully built or executed
using only the documented requirements?
```

The release process should be:

```text id="v9zlqq"
clean
repeatable
traceable
minimal
verifiable
```

The goal is not to support every possible distribution channel.

The goal is to produce a small set of release artifacts that can be trusted.
