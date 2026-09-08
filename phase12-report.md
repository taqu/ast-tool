# Phase 12 Report — Release Quality / Compatibility

## Baseline Record (before Phase 12 work)

- Branch: `features/phase12`
- Baseline commit: `8962b195c1cb8218917db6a33bd48f53aa707927` ("add phase 11") — source-identical to the Phase 11 frozen baseline `d115fab0131a830daca0d8f25d8b4c2841da95a7`; the only diff between the two is documentation (`ast-tool.md`, `roadmap.md`, `phase11-report.md`)
- Target release version: `v0.1.0`
- Working tree at start: clean except `ast-tool.md` (this task file)
- Known release blockers from Phase 11: **none confirmed**. Open RELEASE FIX items carried in: README stub, stale wiki, no `--version`, no changelog, no CI, vendored-notices audit outstanding. KNOWN LIMITATION carried in: Linux/macOS not qualified (Windows-only claim).

## 1. Environment Matrix

| OS | Version | Arch | Compiler/Toolchain | Build result | Test result | CLI smoke result | Status |
|---|---|---|---|---|---|---|---|
| Windows | 10.0.26200 (SDK 10.0.26100.0) | x64 | MSVC 19.44.35224.0 (VS 2022 17.14), CMake 4.2.0 | PASS (0 errors, 40 pre-existing vendor warnings) | PASS — `ALL TESTS PASSED` | PASS (see §12d–12k) | **Tested** |
| Linux | — | — | — | not attempted | not attempted | not attempted | **Not tested** (no environment available) |
| macOS | — | — | — | not attempted | not attempted | not attempted | **Not tested** (no environment available) |

Consistent with Phase 11's own scope statement: only 64-bit Windows is qualified for this release. Linux/macOS remain an explicit **KNOWN LIMITATION**, not a claimed-but-unverified platform.

## 2. Regression Report

### 12a — Clean Build Verification

Built from a fresh `git worktree` (isolated from the dev tree's cached build state) at commit `8962b19`, using the documented command (`cmake -B build -DCMAKE_INSTALL_PREFIX=...`) plus `cmake --build build --config Release`.

| Item | Result |
|---|---|
| Configure | Clean, 8.9s, no errors |
| Build (Release) | **0 errors**, 40 warnings |
| Test build (`ast-tool-test`) | Built successfully alongside |
| Binary execution after build | `ast-tool.exe --help` → exit 0 |
| Artifact | `bin/ast-tool.exe`, 4,235,264 bytes (~4.14 MB), plus 15 bundled tree-sitter grammar DLLs (~29 MB total `bin/`) |

Warning breakdown (all pre-existing, none correctness/compatibility/packaging-relevant, matching Phase 11's own note): C4200 ×26 (sqlite3.c vendor code, zero-length array extension), C4996 ×6 (`getenv` deprecation notice), C4559 ×4 (mimalloc's `operator new`/`new[]` override redefinition). **No warning indicates a release risk.**

### 12b — Full Regression Suite

Ran the complete `ast-tool-test.exe` from the clean-build artifact, invoked with `cwd` at the project root (the binary resolves its fixtures via relative paths like `data/test00.cpp`).

| Metric | Result |
|---|---|
| Result marker | `ALL TESTS PASSED` |
| Exit code | 0 |
| Elapsed | 7.4s |
| `[PASS]` section markers | 208 |
| `[FAIL]` section markers | **0** |
| Individual `SKIP` assertions | 2 (documented, see below) |

The 2 skips are the same intentionally-omitted local-variable/parameter-symbol cases Phase 11 recorded (`lpTmp`/`lpN` — the extractor doesn't emit locals/parameters as workspace symbols by design). **Notably, the 3 git-ignore fixture skips Phase 11 reported (its temp-git-repo setup failed in that run's environment) do not occur here — all 3 git-ignore tests (`basic`, `nested dir`, `file pattern`) and the `non-git workspace` guard all PASS in this environment**, meaning this run has strictly *more* executed coverage than Phase 11's, with the same zero-failure result.

**Procedural note (not a product defect):** an initial run from `test/bin/` (the directory containing the copied DLLs) produced cascading `could not parse` failures and a segfault. Root-caused to my own invocation from the wrong working directory — the test binary's fixtures are referenced by paths relative to the project root, not its own directory. Re-run from the correct `cwd` produced the clean result above. Documented here per the "no unexplained failures" discipline, not carried forward as a finding.

## 3. Core Semantic Smoke Tests (12c)

### Phase 8a (exact FQN / unique-suffix / ambiguity / not-found)
Covered by the automated suite: `resolver: unique suffix works for references/callers/callees` — **PASS** (all three relationship commands). Directly re-verified against the clean binary:
- Exact FQN lookup: `callers auth::AuthToken::validate` → 4 correct callers.
- Not-found: `callers auth::NoSuchThing::foo` → `error: symbol not found`, exit 1.
- Ambiguity: `callers update` (4 same-named methods across unrelated classes) → `error: ambiguous symbol`, candidate list, exit 1 — fails closed, no guess.

### Phase 8b (receiver-type member relationship resolution)
**Finding: no automated regression test exists for this capability.** `test/ast-member-receiver/` contains only a fixture file (`workspace/typed_receivers.cpp`) with no accompanying `.cpp`/`.h` test file, and it is referenced by neither `test/main.cpp` nor `test/CMakeLists.txt` — it is not compiled or run by `ast-tool-test.exe`. This means a future regression in receiver-type resolution would **not** be caught by `ALL TESTS PASSED`.

Given the gap, I ran the required 12c checks directly against the clean binary using the existing (but unwired) fixture, covering every case `ast-tool.md` lists:

| Case | Query | Result |
|---|---|---|
| Object field | `callers typed::Validator::validate` | Correctly includes `fieldObject`, `fieldPointer`, `localObject`, `referenceParameter`, `pointerParameter`; **excludes** `unrelatedField` (different type) and `unresolvedExpression` (return-value receiver) |
| Pointer field | (same query above) | Included, see row above |
| Local object | (same query above) | Included |
| Reference parameter | (same query above) | Included |
| Pointer parameter | (same query above) | Included |
| False-positive guard — unrelated type, same method name | `callers left::Validator::validate` / `right::Validator::validate` | Each returns only its own namespace's caller — no cross-linking |
| False-positive guard — variable-name shadowing across types | `callers shadow_guard::Validator::validate` | Returns only `fieldCall`; excludes `localWithFieldName` (same variable name `receiver`, different type `OtherValidator`) |
| False-positive guard — sibling-scope shadowing | `callers scope_guard::Validator::validate` | Correctly empty (`note: no callers found`, exit 0) — only the sibling-scoped `OtherValidator receiver` calls `validate()` |

**All 8 cases pass with zero false positives or false negatives.** The capability itself is confirmed correct on the clean binary; the gap is purely in automated regression coverage (see Release Issue List, §5).

### Phase 8c (declaration/definition body identity)
Covered by the automated suite and passing: `bi/A` (free function decl+def), `bi/B` (class method out-of-line), `bi/C` (namespace-qualified out-of-line), `bi/D` (inline method), `bi/E` (declaration-only stays empty), `bi/G` (false-positive guard, different namespace), and `bi/motivating: auth::AuthService::refresh callee found` — the exact case from the Phase 9b.1/9b.2 evaluation. **All PASS.**

## 4. CLI Compatibility Report (12d, Exit Codes, stdout/stderr)

All 13 commands (`outline`, `find`, `range`, `parent`, `children`, `symbols`, `search`, `references`, `callers`, `callees`, `cache`, `setup`, plus hidden `dump`) tested for `--help`: **13/13 exit 0, help text to stdout, nothing to stderr.**

| Behavior | Verified result |
|---|---|
| No arguments | Prints top-level help to stdout, exit 0 |
| `--version` / `-v` | Not implemented — falls through to "unknown command", exit 1 (matches Phase 11's documented gap, not a new issue) |
| Unknown command | `error: unknown command...`, exit 1, message to stderr |
| Missing required argument | Clear `error: missing FILE` / `missing required arguments: <symbol> <root>` + usage line, exit 1 |
| Nonexistent file | `error: could not parse file`, exit 1 |
| Nonexistent/empty workspace root | `error: workspace at '<path>' is empty or could not be analyzed`, exit 1 |
| Valid relationship query | Correct sorted results to stdout, exit 0 |
| Unknown symbol | `error: symbol not found: <fqn>` + `next:` suggestion, exit 1 |
| Ambiguous symbol | `error: ambiguous symbol` + full candidate list + `next:` suggestion, exit 1 — fails closed |
| `--pretty` without `--json` | Silently implies `--json` (documented behavior per the Skill doc: "`--pretty`, implies `--json`") — not a contract violation |
| `outline --json` (JSON-unsupported command) | Cleanly rejected, exit 1, no partial/garbled output |
| File path where directory expected | `error: workspace at '<file>' is empty or could not be analyzed`, exit 1, no crash |
| Directory path where file expected | `error: could not parse file`, exit 1, no crash |

**stdout/stderr separation is clean everywhere tested**: successful results always go to stdout only; diagnostics/errors always go to stderr only (verified byte-for-byte, not just visually) — including in `--json` mode, where an error path emits **nothing to stdout** and a structured JSON error object to **stderr**, so a JSON consumer reading stdout never sees mixed or malformed content.

## 5. Failure-Behavior Report (12k, 12l)

| Case | Result |
|---|---|
| Empty repository | Clean `workspace... is empty or could not be analyzed`, exit 1 |
| Repository with no supported source files | Same clean error, exit 1 |
| Nonexistent repository | Same clean error, exit 1 |
| Malformed/unparseable source file (`symbols`) | Exit 0, empty result (no crash) |
| Malformed/unparseable source file (`outline`) | Exit 0, tree-sitter error-recovery output (`unknown` nodes), graceful degradation |
| Unsupported/unrecognized file extension | Exit 0, empty result, no crash |
| Unknown command | Exit 1, clean message |
| Missing command argument | Exit 1, clean usage message |
| Repeated sequential invocations (20×, same workspace) | 0 failures |
| Different commands in sequence | 0 failures, no state leakage |
| 5 parallel invocations against the same workspace/cache DB | All 5 produced identical, correct, uncorrupted output — no crashes, no corruption (SQLite WAL mode + busy_timeout, as configured in `ast-cache-db.cpp`, does its job) |
| Deliberately mangled path (mimicking the historical Phase 9b.1/9b.2 artifact, e.g. `D:MyDocumentsProjects...`) | Clean `workspace... is empty or could not be analyzed`, exit 1 — **no crash, no silent wrong-answer** |

**Crash/Panic Audit (12l): zero crashes, panics, or unhandled exceptions found across all of the above, the full automated suite, and the CLI contract sweep** — with one exception, described next.

**Non-ASCII path defect (found during 12f/12g path testing, see §7 for full detail and fix):** a workspace/cache path containing a character outside the process's active Windows code page (e.g. `é` on this Japanese-locale machine) fails with `error: unexpected exception: No mapping for the Unicode character exists in the target multi-byte code page.` This is caught (not a raw crash/segfault — the process exits cleanly with code 1), but it is a genuine correctness failure, not a clean "not found" response. Root-caused and partially fixed; see §7.

## 6. Path, Repository, and Windows Compatibility (12f, 12g)

| Case | Result |
|---|---|
| Absolute path | Works |
| Relative path (including `.`) | Works |
| Nested working directory (3+ levels deep) | Works |
| Path containing spaces | Works (`search`/`callers` on `"path with spaces"` — correct results) |
| Path containing non-ASCII characters **within** the active code page | Works (pure-Japanese path succeeds on this CP932 system) |
| Path containing non-ASCII characters **outside** the active code page | **Fails** — see below |
| Missing path | Clean error, exit 1 |
| File path where directory expected / vice versa | Clean errors, exit 1, no crash |
| git-ignore honoring, repository-root discovery via libgit2 | Covered and passing in the automated suite (`git ignore: basic/nested/file pattern`, `non-git workspace`) |

### Windows path-quoting artifact — root-caused

The malformed-path pattern observed repeatedly in Phase 9b.1/9b.2 evaluation traces (e.g. `D:MyDocumentsProjectscppast-toolevaluationrepositorieslevel3-pipeline` — colons and backslashes silently dropped) was explicitly re-investigated here. Feeding that exact mangled string directly to the clean-build CLI produces a clean `workspace at '...' is empty or could not be analyzed` error (exit 1) — the same clean rejection any malformed path gets. Feeding the correctly-formed equivalent path succeeds.

**Classification: FIXED / NO PRODUCT DEFECT.** The malformation happens before `ast-tool` ever receives the argument — it originates in agent-generated command syntax / the calling shell's string handling, not in `ast-tool`'s own argument or path parsing. `ast-tool` behaves correctly and safely (clean error, no crash) when given either a well-formed or a malformed path.

### Non-ASCII path defect — root-caused, partially fixed

**Reproduction:** `ast-tool search <path-containing-a-character-outside-the-active-Windows-code-page>` (e.g. `café` on a CP932 system, or conversely a CP932-only character on a Western code page) fails with `error: unexpected exception: No mapping for the Unicode character exists in the target multi-byte code page.` A pure-ASCII or in-codepage-only path is unaffected.

**Root cause:** although command-line arguments are correctly decoded from UTF-16 to UTF-8 at entry (`wmain` + `WideCharToMultiByte(CP_UTF8, ...)` in `main.cpp`), several downstream call sites convert `std::filesystem::path` back to a narrow string via `.string()` — which on Windows uses the **process's ANSI code page**, not UTF-8 — before handing that narrow string to a codepage-sensitive API (`sqlite3_open`, `fopen_s`). Any character outside the active code page then fails to round-trip.

Two confirmed call sites in `src/ast-cache-db.cpp` (`ASTCacheDatabase::open` and `open_readonly`) were fixed by switching to `.u8string()` + `reinterpret_cast<const char*>` (matching an already-established correct pattern used elsewhere in the codebase, `ast-workspace.cpp`'s `IgnoreMatcher` constructor). This is a 2-line, isolated, backward-compatible change (UTF-8 and ANSI are byte-identical for ASCII content, so no existing passing test is affected) — verified by a full test-suite re-run after the change: **`ALL TESTS PASSED`, 208/208, 0 regressions.**

**This fix does not fully resolve the user-visible defect.** The same failure still reproduces for `search`/`symbols`/`outline` on a non-ASCII path, because the actual **source-file reading** path (`fopen_s` calls in `ast-workspace.cpp:243`, `ast-tool.cpp:480`, `cache-warm.cpp:140`, and `read_file()` in `setup.cpp`) has the identical pattern — `fopen_s`'s narrow-string overload is itself always ANSI-code-page-bound on Windows regardless of the byte content passed to it, so simply passing correctly-encoded UTF-8 bytes into it is not sufficient; these call sites need to switch to the wide-character (`_wfopen_s`) family or an equivalent UTF-8-locale approach.

**Decision:** fixing the remaining call sites touches the core file-reading path used by every command and several files (`ast-workspace.cpp`, `ast-tool.cpp`, `cache-warm.cpp`, `setup.cpp`) — this is a materially larger, riskier change than the "smallest reasonable fix" bar for a release-qualification pass, and doing it under time pressure without dedicated new regression coverage would violate the phase's own "avoid opportunistic refactoring" / "do not bundle unrelated fixes" guidance. Per the Fix Policy's explicit escape hatch ("if fixing an issue requires substantial ... redesign, reconsider whether the behavior should instead be documented as unsupported for this release"), **this is documented as a KNOWN LIMITATION for v0.1.0** rather than fixed further in this pass: *workspace, file, and cache paths must use characters representable in the active Windows code page; paths mixing scripts outside the user's system locale are not currently supported.* The cache-db half-fix is kept (real, harmless, verified improvement) and the remaining scope is recommended as a dedicated near-term follow-up (see §7 issue list) — this is very likely reachable in real usage (any project path containing characters outside the user's OS-locale code page), and should be prioritized before any claim of general Unicode-path support.

## 7. Cross-Platform, Installation, Dependency Audit (12h, 12i, 12j, 12p)

**Cross-platform (12h):** only Windows was tested (no Linux/macOS environment available in this session) — classified **tested: Windows; not tested: Linux/macOS**, consistent with and not expanding Phase 11's own claimed scope.

**Installation (12i):** `cmake --install build --config Release --prefix <dir>` **exits 0 but installs nothing** — `CMakeLists.txt` contains **zero `install()` rules**, so `CMAKE_INSTALL_PREFIX` (documented in `CLAUDE.md`) has no effect despite appearing to succeed. This is misleading (a silent no-op reported as success) rather than destructive. The actual working installation method today is **manually copying the `bin/` directory** (the exe plus its 15 tree-sitter grammar DLLs) — verified directly: copied to `D:\Programs\ast-tool-manual-install`, ran `--help` and a `search` against an absolute path to a workspace elsewhere on disk, both succeeded (exit 0, correct results). This also serves as the **clean-environment smoke test (12j)**: `--help`, `search`, and error handling all work correctly when run from a location with no development repository context, using absolute paths.

**Dependency/runtime audit (12p):** CRT is statically linked (`MSVC_RUNTIME_LIBRARY = MultiThreaded`, i.e. `/MT`) — **no Visual C++ Redistributable is required**. Runtime dependencies are the 15 bundled tree-sitter grammar DLLs (must remain alongside the exe; confirmed by relative-path loading working correctly from a relocated copy) plus standard Windows system DLLs. Environment variables used: `CLAUDE_HOME`, `USERPROFILE`, `HOMEDRIVE`, `HOME` (all only by the `setup` command, for locating agent config directories — not used by ordinary analysis commands). Config files: `~/.claude/*`, `~/.codex/*` (only touched by `setup`). No dependency version changes were made.

**Repository safety (12m):** ran `search`, `callers`, `references`, `callees`, `symbols`, and `cache warm` against a real git-tracked fixture repo; `git status`/`git rev-parse HEAD` identical before and after. The only filesystem change was the pre-existing, intentional `.ast-tool/` cache directory (untracked, gitignorable) — no tracked file was modified, no permissions changed. **No unexpected repository modification.**

**Performance sanity (12o), no severe-regression baseline existed to compare against — recorded as the new reference point:**

| Operation | Mean latency (5 runs, small fixture) |
|---|---|
| `--help` | ~0.147s |
| `search --name <x>` | ~0.161s |
| `callers <fqn>` | ~0.186s (one outlier at 0.281s) |
| Binary size | 4,235,264 bytes |
| `bin/` total (exe + 15 grammar DLLs) | ~29 MB |

Nothing here suggests a severe regression; these numbers are dominated by process startup, not analysis work, on this small fixture.

## 8. Release Issue List

| Issue | Category | Platform | Severity | Release blocker? | Resolution | Verification |
|---|---|---|---|---|---|---|
| Non-ASCII path support incomplete outside the active code page | Product defect (Windows narrow-string/code-page handling) | Windows | High (real correctness failure, but scoped to mixed-locale paths) | **No — documented as KNOWN LIMITATION for v0.1.0** | Partial fix applied (`ast-cache-db.cpp`, 2 lines); remaining scope (`ast-workspace.cpp`, `ast-tool.cpp`, `cache-warm.cpp`, `setup.cpp` narrow `fopen_s`/`.string()` call sites) documented as a prioritized near-term follow-up, not fixed this phase | Repro'd, root-caused, partial fix verified via full test-suite rerun (0 regressions) |
| Windows path-quoting artifact from Phase 9b.1/9b.2 | Investigated | Windows | N/A | **No — FIXED / NO PRODUCT DEFECT** | None needed | Confirmed originates in agent-generated command syntax, not `ast-tool` |
| Phase 8b (receiver-type resolution) has no automated regression test | Test/harness gap | All | Medium | No (capability itself verified correct via direct smoke testing) | Wire `test/ast-member-receiver/workspace/typed_receivers.cpp` into `test/main.cpp`/`CMakeLists.txt` as a proper regression suite | Direct CLI smoke test, 8/8 cases pass on clean binary (this phase) |
| `cmake --install` silently installs nothing | Release/packaging defect | Windows (likely all) | Medium (misleading success, not destructive) | No for this freeze | Add minimal `install()` rules, or document "copy `bin/`" as the release install procedure | Reproduced; manual-copy alternative verified working |
| `cache warm --help` / `cache status --help` show generic cache help | Known, pre-existing | All | Low | No | Unchanged from Phase 11 | Re-confirmed present, unchanged |
| README stub / stale wiki / no `--version` / no changelog / no CI / vendored-notices audit | Carried forward from Phase 11 | All | Low–Medium | No for baseline freeze | Unchanged — Phase 11's classification stands | Not re-litigated this phase |
| Linux/macOS not qualified | Known limitation | Linux/macOS | N/A | No, if only Windows is claimed | Unchanged from Phase 11 | No environment available this phase either |

## 9. Final Frozen Commit

- Branch: `features/phase12`
- Baseline commit going in: `8962b195c1cb8218917db6a33bd48f53aa707927`
- **Working tree at end of Phase 12: contains one uncommitted release fix** — `src/ast-cache-db.cpp` (the `.u8string()` fix, §6) — verified via a full clean rebuild and full test-suite rerun (`ALL TESTS PASSED`, 208/208, 0 regressions) but **not yet committed**, per this session's standing practice of never committing without an explicit request. `ast-tool.md` (this task file) is also modified, as in every prior phase.
- **Recommendation:** commit `src/ast-cache-db.cpp` as a scoped release fix (e.g. "fix: use UTF-8 path encoding for AST cache database open, avoiding ANSI code-page failures") before tagging `v0.1.0`, and record the resulting commit hash as the actual final frozen revision — this report's clean-build and full-suite verification already covers that exact code.

## 10. Phase 12 Final Recommendation

### **PASS WITH KNOWN LIMITATIONS**

Justification against the acceptance criteria:

1. Clean build succeeds on the one claimed-supported environment (Windows) — **yes**, 0 errors.
2. Full test suite has no unexplained release-critical failures — **yes**, `ALL TESTS PASSED`, 208/208, 2 documented skips.
3. Accepted Phase 8 semantic behavior passes smoke regression — **yes** for 8a/8c (automated); **yes** for 8b (direct smoke, no automated coverage — flagged as a follow-up, not a regression).
4. Public CLI behavior is understood and tested — **yes**, all 13 commands, exit codes, stdout/stderr separation.
5. Machine-readable output is syntactically valid and predictable — **yes**, verified with a real JSON parser on success, empty-result, and error-path output.
6. Exit-code behavior is suitable for automation — **yes**.
7. Supported path/repository cases behave correctly — **yes for ASCII/in-codepage paths; no for out-of-codepage non-ASCII paths** (documented limitation, not silently ignored).
8. Windows quoting/path behavior explicitly classified — **yes**: quoting artifact = no product defect; non-ASCII path = confirmed defect, partially fixed, remainder documented as a known limitation.
9. Ordinary invalid input fails gracefully without crashing — **yes**, exhaustively tested, zero crashes found.
10. Analysis operations do not unexpectedly modify repositories — **yes**, verified before/after.
11. Installation/basic execution works through an intended release path — **yes, via manual `bin/` copy**; the CMake `install()` path does not currently work and should not be advertised until fixed.
12. All discovered issues are classified — **yes**, §8.
13. No unresolved release blocker remains for a PASS result — **yes**, nothing here was classified as a hard blocker; the non-ASCII path issue is real but scoped and documented rather than silently accepted.
14. Final tested commit recorded — **yes**, §9, with one pending uncommitted fix flagged for the user to commit.

The frozen baseline is stable, correct on its accepted Phase 8 capabilities, safe against crashes and repository damage, and behaves predictably as a CLI — with two explicitly documented gaps (non-ASCII path support outside the active code page, and the non-functional `cmake --install`) that should be prioritized for the next release-quality pass rather than blocking this one.
