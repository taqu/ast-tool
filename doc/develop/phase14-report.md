# Phase 14 Report — Documentation

## Baseline Record

- Branch: `features/phase13` (task doc named this Phase 14; branch naming lagged behind — noted, not corrected, since branch renaming is outside documentation scope)
- Starting commit: `ee89994d1df71d40e700060b47ee44db181777c0` ("Fix missing fopen_s")
- Target release version: `v0.1.0`
- Phase 13 result: **PASS — READY FOR DOCUMENTATION**
- Final public command inventory (from Phase 13): `outline`, `find`, `range`, `parent`, `children`, `symbols`, `search`, `references`, `callers`, `callees`, `cache warm`, `cache status`, `setup`, plus hidden `dump` and (new in Phase 13) `--version`/`-v`
- Known limitations carried in: non-ASCII path support incomplete outside the active Windows code page; `cmake --install` produces no artifacts; Phase 8b has no automated regression test (capability itself verified correct); Linux/macOS not release-qualified (Windows-only tested)

## Implementation Changes Made During Phase 14

Two genuine defects were discovered while validating documentation examples against the real CLI, per the Change Policy ("release-quality defect → fix and rerun relevant Phase 12 gate"). Both were fixed narrowly, with the affected behavior fully revalidated:

1. **Repository-safety defect (found via Phase 14e/14v installation/example testing).** `open_workspace()` in `src/ast-workspace.cpp` unconditionally called `create_directories()` for the `.ast-tool` cache directory *before* checking whether the given workspace root even existed. Running any analysis command against a nonexistent, mistyped, or invalid path silently fabricated that path on disk (plus a cache database inside it) — even though the command itself reported failure. This directly contradicted Phase 12's own "no unexpected repository modification" finding, which had missed it because that testing only checked `git status` inside the fixture repos, not the CWD-level directory where the invalid-path tests actually landed. **Fixed**: cache-directory creation is now gated on `std::filesystem::is_directory()` for the given root. Verified: the original repro (`ast-tool search not_a_real_dir_xyz`) no longer creates anything; normal caching for real workspace roots is unaffected; full test suite rerun, `ALL TESTS PASSED`, 208/208, 0 regressions. Two stray directories left over from the original Phase 12 testing session were also found and removed from the repository root.

2. **Build defect — MSVC runtime-library flag conflict (found while rebuilding to test the fix above).** A from-scratch clean build failed with `LNK2038: RuntimeLibrary mismatch` for one translation unit. Root-caused via a verbose MSBuild compile-command dump to a longstanding, pre-existing conflict in `CMakeLists.txt`: `CMAKE_CXX_FLAGS_RELEASE`/`_DEBUG` hardcoded `/MD`/`/MDd`, directly conflicting with the target's `MSVC_RUNTIME_LIBRARY` property (`/MT`/`/MTd`) — a latent inconsistency that happened to resolve correctly for most files via MSVC's "last flag wins" behavior, until a from-scratch reconfigure exposed it non-deterministically for one file. This predates this session entirely (confirmed via `git blame`/diff — unrelated to any Phase 13 or Phase 14 change) and was not something Phase 12/13 exercised, since neither did a repeated fully-clean reconfigure. **Fixed**: removed the redundant, conflicting `/MD`/`/MDd` from the two `CMAKE_CXX_FLAGS_*` lines, leaving the target property as the single source of truth. Verified: repeated fully-clean `rm -rf build && cmake -B build && cmake --build` succeeds with 0 errors; full test suite rerun, `ALL TESTS PASSED`, 208/208.

3. **Minor `.gitignore` gap (found while cleaning up test debris).** The cache-directory ignore pattern (`/.ast-tool`) was anchored to the repository root only, so a cache directory created by analyzing any other part of the repository (e.g. `src/.ast-tool`, from documenting/testing `ast-tool search src/`) would show as untracked. Widened to `.ast-tool/` (unanchored, matches at any depth) — a zero-risk repository-hygiene fix, not a behavioral change.

All three are narrowly scoped, individually verified, and did not require re-running the full Phase 12 platform/CLI-contract gate beyond the full automated test suite (no CLI-visible behavior changed; the repository-safety fix changes an error-path side effect, not any documented output).

## 1. Documentation Inventory

| File | Audience | Purpose | Status | Action taken |
|---|---|---|---|---|
| `README.md` | Public/new users | Project landing page | Was a 2-line stub | **REPLACE** — full rewrite per Phase 14b structure |
| `wiki/Home.md` | Public/new users | Wiki landing page, doc index | Stale quick-start (missing `references`/`callers`/`callees`/`--version`) | **UPDATE** — quick start refreshed |
| `wiki/Getting-Started.md` | Public/new users | Build, install, first commands | Stale build command (`CMAKE_BUILD_TYPE` for a multi-config VS generator), stale `--help` output block (missing 5 commands, wrong tagline/usage line, `dump` shown as top-level), no relationship-command examples | **UPDATE** — build instructions, verification output, and first-commands section corrected and extended |
| `wiki/CLI.md` | Public/contributors | Full CLI reference | Missing `references`/`callers`/`callees`/`cache`/`setup` entirely from the command tables and output-format sections; stale invocation syntax; no error/ambiguity documentation | **UPDATE** — command tables, output formats, JSON section, error/ambiguity section, and workflow examples added |
| `wiki/AI-Integration.md` | Coding agents / integrators | Agent usage guidance | Described `callers`/`callees` as "library API" only (not CLI commands); missing `references` entirely; **wrong path for `SKILL.md`** (claimed repository root; actual location is `skills/semantic-analysis/SKILL.md`) | **UPDATE** — command selection table, semantic-vs-AST section, limitations, and all `SKILL.md` path references corrected |
| `wiki/Architecture.md` | Contributors | Internal layer design | Accurate — describes internal C++ types/headers, not the CLI surface, so unaffected by the CLI-list staleness found elsewhere | **KEEP** |
| `wiki/API-Reference.md` | Contributors | C++ header/API map | Accurate, matches current header set including `ast-references.h`/`ast-callers.h`/`ast-callees.h` | **KEEP** |
| `wiki/Extension-Guide.md` | Contributors | Adding a language/service | Accurate, process-oriented, not CLI-surface-dependent | **KEEP** |
| `wiki/Contributing.md` | Contributors | Style, workflow | Accurate | **KEEP** |
| `LICENSE` | Public | MIT license | Present, accurate | **KEEP** |
| `CLAUDE.md` | Contributors/agents | Build command note | Accurate (matches the corrected build command now also in the wiki) | **KEEP** |
| `doc/build.md` | Contributors | Windows/Linux build prerequisites | Present, accurate as prerequisites; Linux is prerequisites-only, not release-qualified | **KEEP** (Linux qualification status clarified in README instead) |
| `summary.md`, `roadmap.md`, `analysis.md` | Project maintainers | Raw evaluation data, internal roadmap planning notes (in Japanese), phase-history analysis | Internal development record, not user-facing | **INTERNAL ONLY** — left as-is, not linked from public docs |
| `phase7d2-report.md` through `phase13-report.md` (all phase reports), `phase10a-report.md` | Project maintainers | Phase-by-phase evaluation/release-qualification history | Internal development record | **INTERNAL ONLY** — left as-is, not linked from public docs |
| `evaluation/`, `experimental/` | Project maintainers | Evaluation harness, fixtures, raw run data | Internal, referenced by phase reports | **INTERNAL ONLY** — not linked from public docs (README's Quick Start borrows one small fixture from `evaluation/repositories/` as a demo target, without documenting the evaluation methodology itself) |
| `CONTRIBUTING`, `CHANGELOG` (top-level) | Public | — | Do not exist at top level (`Contributing.md` exists in the wiki instead; no changelog anywhere) | **Not created** — out of scope for this phase (carried forward from Phase 11 as a release fix, not a documentation-completeness blocker; see Phase 11/12 issue lists) |

## 2. Updated README

`README.md` was rewritten from a 2-line stub to a full release-ready landing page: what ast-tool is, features, installation (source-build only, with the correct multi-config build command and the `cmake --install` caveat), quick start, common-commands table, JSON/automation contract, Coding Agent usage (with the corrected `SKILL.md` path), supported languages/platforms, known limitations, and links to the wiki. Every command shown was executed against the built binary and its output verified (§9).

## 3. Installation / Quick Start

Verified from a fresh build in this session (§ Implementation Changes):

```sh
cmake -B build -DCMAKE_INSTALL_PREFIX="D:\Programs\ast-tool"
cmake --build build --config Release
ast-tool --version   # -> ast-tool 0.1.0
ast-tool --help      # -> matches the documented command list exactly
```

The Quick Start originally drafted against this repository's own `src/` directory, but `src/` bundles a vendored SQLite amalgamation file and took over a minute to cold-parse on first run — a bad first impression and not representative of a typical project. **Documentation was corrected before publishing**, not left as an untested claim: the Quick Start and README examples now target `evaluation/repositories/level1-store`, a small bundled example project (0.37s cold-cache run), with a note that a first analysis of a large workspace builds a persistent cache and is fast thereafter.

## 4. Command Reference

`wiki/CLI.md` now covers the complete 13-command public surface (plus hidden `dump`) established in Phase 13, including the newly-added `--version`/`-v`, the `cache warm`/`cache status` two-word subcommand help routing fixed in Phase 13, output formats for every command (including the ones missing before: `references`, `callers`, `callees`), the JSON contract (array-on-success, structured-error-on-stderr, `[]` for empty), and a dedicated errors/ambiguity/not-found section that didn't exist before.

## 5. Examples

Every example shown in `README.md` and the updated wiki pages was executed against the freshly-built `bin/ast-tool.exe` in this session, not copied from memory or historical docs:

- `--version`, `--help` (top-level and `cache warm --help`/`cache status --help`/`help cache warm`/`help cache status`)
- `outline`, `symbols` (single-file)
- `search --name`, `search --kind`, `search --fqn-regex`
- `references`, `callers`, `callees` (plain text and `--json`)
- The ambiguous-symbol error path and its fully-qualified-name recovery (documented end-to-end fresh-user walkthrough, §9)
- `cache status`

No documented example failed. Two were corrected *because* they were tested (the `src/`-as-demo-target choice, and the `SKILL.md` path).

## 6. Support Matrix

| | Status | Evidence |
|---|---|---|
| Windows (64-bit) | **Supported and tested** | Phase 12 clean build + full test suite + CLI contract validation; this phase's fresh clean rebuild |
| Linux | **Best effort** — build prerequisites documented (`doc/build.md`), a compatibility shim (`fopen_s`) exists in source, but never built or tested in any release-qualification phase | Not claimed as supported in README |
| macOS | **Not supported** — untested, no toolchain evidence | Not claimed anywhere |
| Languages — parsing/structural (`outline`/`find`/`dump`/etc.) | Bash, C, C++, C#, CSS, Go, HTML, Java, JavaScript, Python, Ruby, Rust, Scala, TypeScript, TSX | Phase 11 grammar inventory |
| Languages — semantic (`symbols`/`search`/`references`/`callers`/`callees`) with dedicated regression coverage | C, C++, Python, Rust, Go, Java, JavaScript | Phase 11 test-suite inventory, re-confirmed this phase (test suite section headers) |
| Languages — semantic, provisional (extraction machinery present, lighter test evidence) | Bash, C#, CSS, HTML, Ruby, Scala, TypeScript, TSX | Phase 11 |

This matches Phase 12's evidence exactly — no platform or language claim was broadened beyond what was actually tested.

## 7. Known Limitations

Published in `README.md` (Known Limitations section) and cross-referenced in `wiki/AI-Integration.md`'s agent-facing limitations list:

- No inferred receiver typing (`auto`, `decltype`, alias expansion, `this->`, chained/cast/subscript receiver expressions).
- No inheritance-aware lookup, virtual dispatch, function-pointer calls, or transitive relationship traversal.
- No overload-sensitive member resolution — ambiguous overloads fail closed.
- A callee with its own declaration/definition split can, in a narrow case, be omitted from a `callees` result even when the calling function's body was found correctly.
- Workspace/file/cache paths must stay within the active Windows code page (a confirmed Phase 12 defect, documented rather than silently worked around).
- Local variables/parameters are not uniformly exposed as workspace symbols across all languages.
- No `cmake --install` support (documented as a known gap, with the working manual-copy alternative given instead).

Every item is phrased in user-facing terms with a stated workaround where one exists, per Phase 14n's guidance — none are internal implementation jargon carried over verbatim from the phase reports.

## 8. Troubleshooting Guide

Folded into `wiki/CLI.md`'s new "Errors, Not-Found, and Ambiguity" section and `README.md`'s command-reference prose, covering the real failure modes observed across Phase 12/13 testing:

| Symptom | Likely cause | Recommended action |
|---|---|---|
| `error: symbol not found: <fqn>` | Wrong namespace, typo, or the symbol genuinely doesn't exist | Run the suggested `search --name <name> <root>` to find the correct FQN |
| `error: ambiguous symbol: <name>` | Multiple declarations share that unqualified name | Read the candidate list; retry with the printed fully-qualified name |
| `error: workspace at '<path>' is empty or could not be analyzed` | Path doesn't exist, isn't a directory, or has no supported source files | Verify the path; pass a directory, not a file, for `search`/`references`/`callers`/`callees` |
| Empty result, exit 0 | The symbol genuinely has no callers/callees/references — this is a valid answer | No action needed; this is not a failure |
| Slow first run on a large workspace | Cold cache — every file is being parsed for the first time | Expected; subsequent runs against the same root reuse the persistent cache and are fast |
| `error: unexpected exception: No mapping for the Unicode character...` | Workspace/file path contains a character outside the active Windows code page | Known limitation (documented) — use a path within your system's code page |
| JSON parse failure in a script | Almost always the script reading the wrong stream — errors go to stderr, only stdout is JSON | Redirect/capture stdout and stderr separately |

## 9. Documentation Validation Report

- **Commands tested**: `--version`, `--help` (top-level, per-command ×13, `cache warm`/`cache status` two-word form), `outline`, `symbols`, `search` (`--name`, `--kind`, `--fqn-regex`), `references`, `callers`, `callees` (plain and `--json`), `cache status`.
- **Examples tested**: every command example that appears in `README.md` and the updated wiki pages, executed against the session's freshly-built binary.
- **Clean-environment walkthrough result**: full sequence — verify install → outline → symbols → search → references → callers → callees → deliberate ambiguous-symbol error → recovery with a fully-qualified name — completed successfully with no friction points and no step requiring undocumented project knowledge.
- **Documentation defects found**: (1) stale `Getting-Started.md` build command and `--help` output block; (2) `CLI.md` missing 5 of 13 commands; (3) `AI-Integration.md` describing `callers`/`callees` as library-only and omitting `references`; (4) **wrong `SKILL.md` path** in 3 places in `AI-Integration.md` (claimed repository root; actual path is `skills/semantic-analysis/SKILL.md`); (5) README's originally-drafted Quick Start pointed at a slow, unrepresentative directory (this repo's own `src/`).
- **Documentation defects fixed**: all five, above, plus the two implementation-level defects found in the process (§ Implementation Changes) and the `.gitignore` gap.
- **Remaining limitations**: `CONTRIBUTING`/`CHANGELOG` files at the repository top level, `cmake --install`, non-ASCII path support, and Phase 8b's missing automated regression test remain open — all previously classified as non-blocking release fixes or documented limitations, not documentation-completeness issues, and are called out explicitly rather than hidden.

## 10. Final Documentation Commit

- Branch: `features/phase13`
- Working commit at start: `ee89994d1df71d40e700060b47ee44db181777c0`
- Files changed this phase: `.gitignore`, `CMakeLists.txt`, `README.md`, `src/ast-workspace.cpp`, `wiki/AI-Integration.md`, `wiki/CLI.md`, `wiki/Getting-Started.md`, `wiki/Home.md`
- Working tree status at end of phase: all of the above modified, plus `ast-tool.md` (this task file) as in every prior phase; nothing committed — per this session's standing practice of never committing without an explicit request, flagged here for the user to review and commit.
- Final verification on this working tree: fresh clean build (0 errors), full automated test suite (`ALL TESTS PASSED`, 208/208, 0 regressions), `--version`/`--help` correct, full fresh-user walkthrough successful.

## 11. Phase 14 Final Recommendation

### **PASS — READY FOR PACKAGING / DISTRIBUTION**

Justification against the acceptance criteria:

1. README accurately describes the current release candidate — **yes**, rewritten from a 2-line stub and validated command-by-command against the real binary.
2. Installation instructions work from a clean environment — **yes**, verified via a fresh `rm -rf build && cmake -B build && cmake --build` in this session (which also surfaced and fixed the pre-existing MSVC runtime-library build defect — installation documentation is now *more* trustworthy than before this phase, having actually forced a from-scratch verification).
3. Quick-start instructions produce a useful result — **yes**, full walkthrough completed with no friction; the original draft's slow/unrepresentative demo target was caught and corrected before publishing.
4. Every public command is documented — **yes**, `CLI.md`'s command tables now match the Phase 13 inventory exactly.
5. All documented CLI examples have been validated — **yes**, every one executed against the real binary this session.
6. JSON/automation behavior is documented — **yes**, including the pre-1.0 stability caveat.
7. Supported languages stated accurately — **yes**, matches Phase 11's evidence, with the parsing/semantic/provisional distinction preserved.
8. Supported platforms match Phase 12 evidence — **yes**, Windows tested, Linux best-effort, macOS unsupported — no claim broadened.
9. Known limitations clearly documented — **yes**, in user-facing language with workarounds where they exist.
10. Common release-relevant errors have troubleshooting guidance — **yes**, §8.
11. Version and compatibility language matches Phase 11 policy — **yes**, `0.1.0`, pre-1.0 CLI/JSON-stability caveat included.
12. License information present and accurate — **yes**, unchanged MIT license, correctly linked.
13. Stale/rejected behavior removed from public documentation — **yes**: no Phase 10 trigger-experiment references, no pre-Phase-8 behavior claims, no incorrect `SKILL.md` path, no missing-command gaps remain in any public-facing file.
14. A fresh user can follow the documentation without internal project knowledge — **yes**, demonstrated by the full walkthrough in §9.
15. No documentation-critical release issue remains — **yes** — and two release-quality code defects that documentation work surfaced (repository-safety side effect, MSVC build-flag conflict) were fixed and verified rather than merely noted.
