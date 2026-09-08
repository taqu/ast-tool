# Phase 11 — Release Freeze / Baseline Lock

Freeze date: 2026-09-08 (JST)  
Baseline branch: `features/phase10`  
Baseline commit: `d115fab0131a830daca0d8f25d8b4c2841da95a7` (`revert phase 10`)  
Release target: `v0.1.0`

## 1. Frozen baseline

The implementation baseline for the initial release is:

```text
accepted Phase 7d semantic-analysis Skill
+ Phase 8a unique FQN-suffix relationship resolution
+ Phase 8b receiver-type member relationship resolution
+ Phase 8c callable declaration/definition body identity
```

All four baseline commits are ancestors of the frozen commit. The accepted
`skills/semantic-analysis/SKILL.md` is present. The three Phase 10 Candidate 1
triggers (`all callers of`, `call sites of`, and `every call to`) are absent.
Commit `d115fab` is the explicit revert of that experiment; historical Phase 10
evaluation data remains in the repository and does not participate in runtime
behavior.

The source baseline itself is clean. At freeze time the working tree has two
pre-existing documentation edits, `ast-tool.md` and `roadmap.md`, which describe
the release-readiness work and do not alter the executable. This report is an
additional Phase 11 deliverable. No semantic or routing code was changed during
the freeze.

## 2. Baseline verification

An archive made directly from `HEAD` was configured and built in an isolated
temporary directory with CMake 4.2.0-rc2, Visual Studio 2022, MSVC 19.44, and
the Windows 10.0.26100 SDK. Both `ast-tool.exe` and `ast-tool-test.exe` built in
Release configuration. The build emitted existing MSVC/vendor warnings but no
errors.

The complete monolithic test executable was run both in the working tree and
from the clean commit archive. Both runs returned exit code 0 and printed
`ALL TESTS PASSED`.

| Regression measure | Working tree | Clean commit archive |
| --- | ---: | ---: |
| Reported PASS markers | 281 | 281 |
| Reported FAIL markers | 0 | 0 |
| Reported SKIP markers | 5 | 5 |
| Success rate for executed checks | 100% | 100% |
| Unexpected crashes | 0 | 0 |
| Unexpected output changes | 0 | 0 |

The five skips are understood: three git-ignore fixture setup checks report
that they could not initialize a temporary git repository, and two reference
fixtures document intentionally omitted local-variable/parameter symbols.
They are not new regressions. Expected error-path tests deliberately invoke
invalid commands and verify clean failures; no unexpected AST failures or
retries occurred.

The current suite directly retains the release-baseline behaviors:

- Phase 8a: unique suffix resolution tests for `references`, `callers`, and
  `callees` pass, including ambiguity and exact-match precedence guards.
- Phase 8b: typed object, pointer, field, local, and parameter receiver cases
  pass across all three relationship commands, with false-positive guards.
- Phase 8c: declaration/definition body-identity cases A–E, G, and the motivating
  `auth::AuthService::refresh` case pass.
- Phase 7d: the accepted Skill is present and unchanged by Phase 8–10 routing
  work except for the rejected Candidate 1 addition and its recorded revert.

The `experimental/` and `evaluation/` trees contain preserved evidence only.
They are not included in `SOURCES`, `HEADERS`, or `MAIN_SOURCES` and do not
affect the runtime targets.

## 3. Release scope

### Platforms

The qualified platform for this freeze is 64-bit Windows with Visual Studio
2022/MSVC and the bundled Windows libraries. The CMake project contains a Unix
path, but Linux and macOS were not built in this Phase 11 environment and are
not release-qualified claims yet.

### Languages

Parser and symbol-extractor implementations are present for Bash, C, C++, C#,
CSS, Go, HTML, Java, JavaScript, Python, Ruby, Rust, Scala, TypeScript, and TSX.
The clean Windows build links all 15 bundled grammars. Dedicated extractor
regression suites currently cover C, C++, Python, Rust, Go, Java, and
JavaScript. The other eight languages are supported by implementation but have
lighter release evidence and should be described as provisional until the
compatibility phase adds direct smoke coverage.

### Commands and output modes

The public command surface is `outline`, `find`, `range`, `parent`, `children`,
`symbols`, `search`, `references`, `callers`, `callees`, `cache`, and `setup`.
`dump` remains an internal/compatibility command: it is callable and has help,
but is intentionally hidden from top-level help.

All commands provide plain-text output. `symbols`, `search`, `references`,
`callers`, and `callees` additionally provide compact JSON and `--pretty` JSON.
Semantic error paths also produce structured errors when JSON is requested.
Workspace commands recursively scan supported extensions, sort their results,
use the SQLite AST cache, and honor git-ignore behavior when available.

### Supported semantic behavior

- Per-file symbol extraction with symbol kind, FQN, access/storage metadata,
  and content-dependent AST node ID.
- Workspace symbol search using exact/sub-string filters and RE2 regex filters.
- Direct references, callers, and callees for resolvable symbols.
- Exact FQN, unqualified-name, and unique boundary-safe FQN-suffix target
  resolution. Ambiguous matches fail closed with candidates.
- Receiver-constrained member resolution for directly typed object/pointer
  fields, locals, reference parameters, and pointer parameters.
- Callable declaration/definition canonicalization and out-of-line body lookup
  for ordinary free functions and methods.

Relationship results are direct, not transitive. Empty relationship results are
successful; unresolved, ambiguous, unsupported-kind, invalid-query, and empty
workspace cases fail cleanly.

## 4. CLI contract inventory

Global `-h`/`--help` and `help [command]` print help to stdout and exit 0.
Successful commands print their documented result to stdout and exit 0.
Argument, parse, resolution, and operational errors print diagnostics to stderr
and exit 1, except JSON-aware semantic commands render structured error output
when JSON mode is selected.

| Command | Required arguments | Optional arguments / modes | Normal stdout | Common failures |
| --- | --- | --- | --- | --- |
| `outline` | `FILE` | help | indented named AST | missing/unparseable file |
| `find` | `FILE` | `--type`, `--grammar`, `--text`, `--id`, paired `--line`/`--column` | matching AST nodes | bad option/value, invalid filter pairing, unparseable file |
| `range` | `FILE` | start/end line and column bounds | intersecting AST nodes | bad option/value, unparseable file |
| `parent` | `--id ID FILE` | help | parent AST node | bad/missing ID, node not found, unparseable file |
| `children` | `--id ID FILE` | help | immediate child AST nodes | bad/missing ID, node not found, unparseable file |
| `symbols` | `FILE` | `--json`, `--pretty` | symbols | bad option, missing/unparseable file |
| `search` | `ROOT` | name/FQN/kind/file exact or regex filters, `--limit`, JSON modes | sorted workspace symbols | conflicting/invalid filters, empty workspace |
| `references` | `SYMBOL ROOT` | JSON modes | sorted direct reference sites | not found, ambiguous symbol, empty workspace |
| `callers` | `SYMBOL ROOT` | JSON modes | sorted direct call sites and caller FQNs | relationship-ineligible kind, not found/ambiguous, empty workspace |
| `callees` | `SYMBOL ROOT` | JSON modes | sorted direct callees from target body | relationship-ineligible kind, not found/ambiguous, empty workspace |
| `cache warm` | none | `ROOT`, `--verbose`, internal `--background` | warm summary/progress | invalid root, cache/worker failure |
| `cache status` | none | `ROOT` | cache status | invalid root/cache access failure |
| `setup` | none | agent target, global/local, dry-run, remove flags | planned/applied hook changes | invalid flag combination, config read/write failure |

Known contract/documentation mismatches are classified below rather than fixed
during the freeze: the wiki omits newer semantic/cache/setup commands, cache
subcommand help currently falls back to the generic cache page, and the CLI has
no version option.

## 5. Known limitations

### Intentional unsupported behavior

- No `auto`, `decltype`, alias-expansion, or other inferred receiver typing.
- No chained, return-value, call, cast, subscript, or other complex receiver
  expression resolution.
- No explicit `this->` receiver typing.
- No inheritance-aware lookup, virtual dispatch expansion, function-pointer
  calls, transitive relationship traversal, or template/dependent lookup.
- No overload-sensitive member resolution; ambiguous overloads fail closed.
- No cross-language callable-definition identity.
- ODR-violating multiple definitions are unsupported.

### Known edge cases

- Local variables and parameters are not uniformly emitted as workspace
  symbols across languages, although directly typed receivers can still be
  used by the receiver resolver where supported.
- Callee-side identity is FQN-based and may retain gaps for uncommon callable
  forms such as constructor/destructor or language-specific definitions.
- AST node IDs are stable only for unchanged file content; they are not stable
  semantic symbol IDs.
- Eight implemented languages do not yet have dedicated extractor regression
  suites, so semantic completeness varies by language grammar/extractor.

### Low-priority defects

- Three git-ignore tests skip when their temporary git repository cannot be
  initialized in the current environment.
- Clean MSVC builds contain existing compiler/vendor warnings.
- `cache warm --help` and `cache status --help` show generic cache help rather
  than the detailed pages promised by that help text.

None of these limitations violates the narrowly documented Windows v0.1.0
baseline contract.

## 6. Issue classification and blockers

| Issue | Classification | Blocker? | Required before public release? | Verification |
| --- | --- | --- | --- | --- |
| Semantic baseline regression | RELEASE BLOCKER if observed | No; none observed | Yes | Clean Release build and full suite |
| README is only a two-line stub | RELEASE FIX | No for baseline freeze | Yes | Fresh-user documentation review |
| Wiki CLI/Getting Started content is stale | RELEASE FIX | No for baseline freeze | Yes | Compare docs with built-in help and clean install |
| No version source of truth or `--version` output | RELEASE FIX | No for baseline freeze | Yes | `ast-tool --version` returns `0.1.0` after Phase 13/15 work |
| No changelog/release-notes source | RELEASE FIX | No for baseline freeze | Yes | Release notes present for RC |
| No project CI/release workflow | POST-RELEASE IMPROVEMENT for freeze; packaging work | No | Required only by chosen distribution process | CI matrix/release dry run |
| Linux/macOS not qualified | KNOWN LIMITATION | No when only Windows is claimed | Only before claiming those platforms | Native clean builds and suites |
| Vendored dependency notices not assembled | RELEASE FIX / distribution review | No for baseline freeze | Yes before distributing archives | License inventory reviewed and included |
| Advanced receiver/call semantics | KNOWN LIMITATION | No | No | Documented fail-closed fixtures |
| Stable semantic symbol ID | POST-RELEASE IMPROVEMENT | No | No | Deferred design work |

No confirmed release blockers were found during Phase 11. The listed release
fixes are gates for later release-quality, documentation, and packaging phases;
they do not make the implementation baseline unsafe to freeze.

## 7. Version and branch policy

- Initial release version: `0.1.0`; tag format: `v0.1.0`.
- Phase 11 version source of truth: this frozen-baseline record. A single build
  metadata source and `ast-tool --version` are release fixes for the CLI/
  packaging phase; no competing version declaration exists today.
- Cut `release/v0.1.0` from the frozen baseline after the Phase 11 documentation
  is committed. Only correctness, stability, compatibility, packaging,
  documentation, and distribution fixes may enter that branch.
- Continue post-release feature development from `main`. If a separate branch
  is judged unnecessary, the same release-only rule applies to the tagged
  stabilization commits on `main`.
- Every proposed release change must answer whether it is required for release
  correctness, stability, compatibility, packaging, documentation, or
  distribution. Otherwise it is deferred.

## 8. Artifact inventory

| Artifact | State at freeze | Disposition |
| --- | --- | --- |
| README | Present but inadequate | Complete in documentation phase |
| Project LICENSE | Present (MIT) | Keep |
| Changelog/release notes | Missing | Add before RC |
| Installation/usage docs | Present in wiki but stale | Reconcile with CLI and clean build |
| Supported platforms/languages | Partially documented | Publish the qualified scope above |
| Known limitations | Recorded here | Promote to release docs |
| Version information | Decision recorded; not embedded | Add one source of truth and CLI output |
| CI configuration | No project workflow found | Add if required for qualification/distribution |
| Release workflow/checksums | Missing | Packaging phase |
| Third-party notices | Individual inventory not assembled | Audit and include before distribution |

## 9. Rejected and deferred work

- Phase 6 agent-facing command surface: **REJECTED**.
- Phase 10 Candidate 1 trigger vocabulary: **REJECTED AND REVERTED**.
- Stable semantic symbol ID: **DEFERRED**.
- Additional semantic commands, receiver inference, relationship semantics, and
  resolution strategies: **DEFERRED**.
- Further semantic routing optimization: **DEFERRED**.
- Unrelated refactors, dependency upgrades, cleanup, and performance work:
  **DEFERRED UNDER FEATURE FREEZE**.

Phase 10 is complete with the final result **NO SAFE ROUTING IMPROVEMENT FOUND**.
Baseline routing remains unchanged.

## 10. Release readiness

The exact implementation being prepared is commit
`d115fab0131a830daca0d8f25d8b4c2841da95a7`, plus release-only documentation
and qualification work that does not broaden semantics. Scope, exclusions,
limitations, version target, and change policy are now unambiguous.

**BASELINE LOCKED — READY FOR RELEASE QUALIFICATION**
