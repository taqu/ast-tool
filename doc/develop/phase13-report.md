# Phase 13 Report — CLI / User Experience Polish

## Baseline Record

- Branch: `features/phase13`
- Starting commit: `d6ef1b4961e3db61a96c2199f77a125b62647fb6` ("Add phase12")
- Target release version: `v0.1.0`
- Phase 12 result: **PASS WITH KNOWN LIMITATIONS**
- Known limitations carried in: non-ASCII path support incomplete outside the active Windows code page (documented, out of scope here — behavioral, not presentational); `cmake --install` installs nothing (packaging, out of scope here); no automated Phase 8b regression test (test-infra, out of scope here)
- Remaining release fixes carried in from Phase 11/12: README stub, stale wiki, **no `--version`** (addressed this phase), no changelog, no CI, vendored-notices audit outstanding, Linux/macOS unqualified

Semantic baseline unchanged: Phase 7d Skill + Phase 8a + Phase 8b + Phase 8c. No semantic, routing, or Phase 10 behavior was touched.

## 1. CLI UX Audit

| Area | Issue | Severity | Proposed action | Compatibility class | Final decision |
|---|---|---|---|---|---|
| `symbols --json` output structure | **Produces syntactically invalid JSON** — wraps a comma-separated object list in `{...}` instead of `[...]`; verified to fail `json.load()` outright | **Critical** | Wrap in `[`/`]` like every other JSON command | A (fixes an outright defect, not a working contract) | **Fixed** |
| `symbols --json` field naming | `qualified_name` field, while `search`/`callers`/`callees`/`references` all use `fqn` for the identical concept | Medium (JSON consumer confusion, cross-command inconsistency) | Rename to `fqn` | B (externally visible, but pre-release and clearly preferable) | **Fixed** |
| `symbols --json` boolean fields | `static`/`constexpr`/`inline` emitted as quoted JSON strings (`"false"`) instead of native booleans (`false`) | Medium (non-idiomatic JSON, extra parsing burden on consumers) | Emit native JSON booleans | B (pre-release schema cleanup) | **Fixed** |
| `cache warm --help` / `cache status --help` | Both fall back to the generic `cache` overview page; dedicated, already-written detailed help text (`kHelpCacheWarm`, `kHelpCacheStatus`) existed in source but was never wired into the help-topic lookup for two-word subcommands | Medium (documented since Phase 11/12; real discoverability gap) | Route `cache <sub> --help` and `help cache <sub>` to the existing dedicated help text | A (clarity-only; no behavior change, `cache <sub>` itself is untouched) | **Fixed** |
| No `--version` | Users have no way to identify the installed build; flagged since Phase 11 as a required release fix | High (explicit Phase 13 acceptance criterion #9) | Add `--version`/`-v` at the top level, `ast-tool <version>` output | A (pure addition, no existing behavior changed; `-v` was previously "unknown command", now version — non-breaking since nothing depended on the old error) | **Fixed** |
| Top-level `--help` "Options" section | Didn't list `--version` (didn't exist) | Low | Add a line alongside `-h, --help` | A | **Fixed** (as part of the `--version` addition) |
| Per-command help structure/content (all 13 commands) | Reviewed for name clarity, purpose, argument clarity, defaults, ambiguity/not-found explanation, examples | — | — | — | **No change needed** — already NAME/SYNOPSIS/DESCRIPTION/ARGUMENTS/OPTIONS/OUTPUT/EXAMPLES structured, with disambiguation guidance and copy-pasteable examples in every command already |
| Terminology consistency (symbol/FQN/root/reference/caller/callee/file) | Audited across all 13 commands' help text | — | — | — | **No change needed** beyond the `fqn` JSON fix above — terminology was already uniform |
| Error messages (unknown command, missing arg, not found, ambiguous, invalid path, invalid flag combo) | Re-verified against the "what failed / why / what next" rule and the no-stack-trace rule | — | — | — | **No change needed** — already compliant; re-verified with the final binary |
| Empty vs. failure distinction | `note: no X found` (exit 0) vs. `error: ...` (exit 1); JSON empty array `[]` vs. structured error on stderr | — | — | — | **No change needed** — already correctly distinguished; `symbols --json`'s empty case now also correctly emits `[]` as a side effect of the array fix |
| Ambiguous results | Candidate list with kind/file/line + "supply a fully-qualified name" guidance | — | — | — | **No change needed** — Phase 8 ambiguity guards untouched, still fail closed |
| Diagnostic output / stdout-stderr separation | Re-verified byte-exact after all changes | — | — | — | **No change needed** |
| Color / interactive prompts | Source-wide search for ANSI escapes, `std::cin`, `scanf`, `getchar`, console-read APIs | — | — | — | **No change needed** — none exist anywhere in the codebase; nothing to fix for 13q/13r |
| Shell usability (piping, redirection, exit-status idioms) | Re-verified: `search \| wc -l`, `search \| grep`, `if cmd; then ... fi`, separate stdout/stderr capture | — | — | — | **No change needed** |
| Agent/Skill compatibility | Re-verified `search`/`references`/`callers`/`callees`/`find` (the Skill-documented commands) produce identical output and JSON after all changes | — | — | — | **No change needed** — none of these commands' code was touched |

## 2. Final Command Inventory

| Command | Purpose | Usage | Important options | Output modes |
|---|---|---|---|---|
| `outline` | Structural outline of a source file | `ast-tool outline <file>` | — | Plain text (indented tree) |
| `find` | Find AST nodes by type/text/position/ID | `ast-tool find [--type][--grammar][--text][--id][--line --column] <file>` | `--line`/`--column` must pair | Plain text |
| `range` | AST nodes within a line/column range | `ast-tool range [--start-line][--start-column][--end-line][--end-column] <file>` | All bounds optional, default 0 | Plain text |
| `parent` | Parent AST node of a given ID | `ast-tool parent --id <hex> <file>` | `--id` required | Plain text |
| `children` | Immediate child AST nodes | `ast-tool children --id <hex> <file>` | `--id` required | Plain text |
| `symbols` | List symbols in a source file | `ast-tool symbols [--json [--pretty]] <file>` | — | Plain text, JSON array, pretty JSON array |
| `search` | Search symbols across a workspace | `ast-tool search [--name][--fqn][--kind][--file][--name-regex][--fqn-regex][--file-regex][--json [--pretty]] <root>` | Exact/regex filters per field are mutually exclusive | Plain text, JSON array, pretty JSON array |
| `references` | Find references to a symbol | `ast-tool references [--json [--pretty]] <symbol> <root>` | Declaration site excluded by default | Plain text, JSON array, pretty JSON array |
| `callers` | Find direct callers of a function | `ast-tool callers [--json [--pretty]] <symbol> <root>` | Direct calls only, no transitive graph | Plain text, JSON array, pretty JSON array |
| `callees` | Find direct callees of a function | `ast-tool callees [--json [--pretty]] <symbol> <root>` | Direct calls only, no transitive graph | Plain text, JSON array, pretty JSON array |
| `cache warm` | Warm the persistent AST cache | `ast-tool cache warm [--verbose] [<root>]` | `--verbose`/`-v` prints statistics; `--background` (internal, used by `setup`) | Plain text (silent by default; stats with `--verbose`) |
| `cache status` | Show AST cache info for a workspace | `ast-tool cache status [<root>]` | — | Plain text |
| `setup` | Configure Claude Code / Codex session-start hooks | `ast-tool setup [--claude][--codex][--all][--global][--local][--dry-run][--remove]` | Idempotent; never removes unrelated hooks | Plain text |
| `dump` (hidden/internal) | Print every AST node depth-first | `ast-tool dump <file>` | — | Plain text |
| `--version` / `-v` (new this phase) | Print installed version | `ast-tool --version` | — | `ast-tool 0.1.0` |
| `--help` / `-h` / `help [command]` | Top-level or per-command help | `ast-tool --help`, `ast-tool <command> --help`, `ast-tool help <command>` | `help cache warm`/`help cache status` now resolve to dedicated pages | Plain text |

## 3. CLI Contract Change Log

| # | Old behavior | New behavior | Reason | Compatibility impact | Tests |
|---|---|---|---|---|---|
| 1 | `symbols --json` emitted `{obj},{obj},...}` — **not valid JSON**, fails any real JSON parser | Emits a valid JSON array `[obj,obj,...]`, matching every other JSON-capable command | Confirmed defect; no working contract existed to preserve | None — the old output could not be consumed by a JSON parser in the first place, so there is no working integration to break | Verified with Python's `json.load()` on compact and pretty output, including the empty-result case (`[]`); full automated suite rerun, 0 regressions |
| 2 | `symbols --json` field `qualified_name` | Field renamed to `fqn` | Consistency with `search`/`references`/`callers`/`callees`, which all already use `fqn`; safe now, before the public JSON contract is established | Any *hypothetical* consumer of the old (invalid) JSON would need updating regardless, since it couldn't previously be parsed | Same as above |
| 3 | `symbols --json` booleans (`static`/`constexpr`/`inline`) as quoted strings `"false"` | Native JSON booleans `false`/`true` | Idiomatic JSON; same reasoning as #2 | Same as #2 | Same as above |
| 4 | `ast-tool cache warm --help`, `ast-tool cache status --help`, `ast-tool help cache warm`, `ast-tool help cache status` all showed the generic `cache` overview | Each shows its own dedicated, detailed help page | Fixes a documented, pre-existing discoverability gap (Phase 11/12) using help text that already existed but wasn't wired up | `ast-tool cache --help` (no subcommand) is **unchanged**; `cache warm`/`cache status` themselves (non-`--help` invocation) are unchanged; only the `--help` output for the two-word form changed | Manual verification of all 4 invocation forms + `cache --help`/top-level listing unchanged; full automated suite rerun, 0 regressions |
| 5 | `ast-tool --version` / `ast-tool -v` → "unknown command", exit 1 | Prints `ast-tool 0.1.0`, exit 0 | Explicit Phase 13 requirement (acceptance criterion #9); flagged as needed since Phase 11 | Purely additive — nothing could have depended on the old "unknown command" error for this specific invocation as documented behavior | Manual verification of `--version` and `-v`; confirmed `cache warm -v` (verbose) is unaffected since it's parsed in a different scope; full automated suite rerun, 0 regressions |
| 6 | Top-level `--help` "Options" listed only `-h, --help` | Also lists `-v, --version` | Discoverability for the new flag | Additive | Visual verification |

**No other externally visible behavior changed.** All semantic commands (`search`, `find`, `references`, `callers`, `callees`, `symbols` plain-text mode, `outline`, `range`, `parent`, `children`, `cache warm`/`status` non-help behavior, `setup`, `dump`) are byte-for-byte unchanged — none of their source files were modified.

## 4. Error UX Matrix

| Scenario | Exit code | stdout | stderr | User guidance | Status |
|---|---|---|---|---|---|
| Unknown command | 1 | empty | `error: unknown command or missing required arguments` + `Run 'ast-tool help'...` | Yes | Unchanged, verified |
| Unknown flag (e.g. inside `cache warm`) | 0 (silently ignored) | normal command output | empty | None — flag is silently dropped | **Pre-existing, unchanged; noted below** |
| Missing required argument | 1 | empty | `error: missing FILE` / `missing required arguments: <symbol> <root>` + usage line | Yes | Unchanged, verified |
| Invalid path (nonexistent file) | 1 | empty | `error: could not parse file` + usage line | Partial (doesn't say *why* unparseable — missing vs. malformed) | Unchanged, verified |
| Repository/workspace not found | 1 | empty | `error: workspace at '<path>' is empty or could not be analyzed` + `next: verify the workspace root path` | Yes | Unchanged, verified |
| Unsupported/unparseable source | 0 (symbols/outline) | empty result / tree-sitter error-recovery nodes | empty | Implicit (graceful degradation, no explicit message) | Unchanged, verified |
| Symbol not found | 1 | empty | `error: symbol not found: <fqn>` + `next: search <name> <root>` | Yes | Unchanged, verified |
| Ambiguous symbol | 1 | empty | `error: ambiguous symbol: <name>` + full candidate list (kind, FQN, file, line) + `next: retry with a fully-qualified name` | Yes, detailed | Unchanged, verified |
| Invalid output option (`outline --json`, JSON-unsupported command) | 1 | empty | rejection, no partial output | Implicit | Unchanged, verified |
| Invalid command combination | 1 (varies) | empty | clear usage-style message | Yes | Unchanged, verified |
| `symbols --json` parse | — | **now valid JSON** | — | — | **Fixed this phase** |
| `cache warm/status --help` | 0 | dedicated help text | empty | — | **Fixed this phase** |

**Noted, not changed:** unknown flags passed to `cache warm`/`cache status` (e.g. a typo'd `--verbse`) are silently ignored rather than rejected — the command still runs using defaults. This is pre-existing behavior, outside a single command's help/JSON concerns, and changing argument-rejection semantics is exactly the kind of parser-behavior change Phase 13's "do not redesign the parser unless necessary" guidance says to leave alone. Recorded in the Deferred Backlog (§6) rather than fixed here.

## 5. Help Verification

All verified against the rebuilt binary (post-fixes):

- **Top-level `--help`**: matches actual behavior — command list matches the 13 real commands (dump correctly hidden), both `--help`/`-h` and now `--version`/`-v` are listed and work as described.
- **All 13 public commands' `--help`** (`outline`, `find`, `range`, `parent`, `children`, `symbols`, `search`, `references`, `callers`, `callees`, `cache`, `setup`, plus hidden `dump`): captured in full, cross-checked against actual command behavior for every documented output field — all match. `symbols --help` updated to say "JSON array" (was "JSON object") and `fqn` (was `qualified_name`), now matching real output.
- **`cache warm --help` / `cache status --help` / `help cache warm` / `help cache status`**: now show their own dedicated pages (previously fell back to generic `cache`); content verified against actual `cache warm`/`cache status` behavior (arguments, defaults, `--verbose` output fields).
- **`--version` / `-v`**: prints `ast-tool 0.1.0`, exit 0; verified `cache warm -v` (verbose) is unaffected.

## 6. Deferred UX Backlog

Recorded but intentionally **not** implemented during this freeze:

- Unknown-flag rejection for `cache warm`/`cache status` (currently silently ignored) — a parser-behavior change, deferred per "do not redesign the parser unless necessary."
- Shell completion (bash/zsh/PowerShell) — new feature, out of scope.
- Additional output formats beyond plain text / JSON / pretty JSON — out of scope.
- `--verbose`/diagnostic modes for commands other than `cache warm` — not requested, no identified user confusion to fix.
- New command aliases or shorthand — not requested.
- Interactive exploration mode — explicitly out of scope (non-interactive requirement).
- More specific error messages distinguishing "file not found" vs. "file exists but unparseable" (both currently report similar text) — a minor clarity nicety, not a source of real confusion given the usage context (the path is always echoed), deferred as low-value.
- README/wiki/changelog/CI/vendored-notices/version-embedding-in-build-metadata — carried forward from Phase 11/12, belongs to Phase 14 (documentation) or packaging work, not CLI UX.
- Non-ASCII path support beyond the active code page, and `cmake --install` producing no artifacts — both are Phase 12 findings; they are behavioral/build-system issues, not presentation, and explicitly out of this phase's scope.

## 7. Final Regression Result

- Branch: `features/phase13`
- Working commit at end of phase: still `d6ef1b4961e3db61a96c2199f77a125b62647fb6` as the last *committed* revision — this phase's changes are in the working tree, uncommitted (see below)
- Files changed: `include/ast-tool.h`, `src/ast-tool.cpp`, `src/help.cpp`, `src/symbols.cpp`
- Tests: full `ast-tool-test.exe` suite rerun after **every** change in this phase (4 separate rebuild+retest cycles) — **`ALL TESTS PASSED`, 208/208, 0 FAIL, 0 regressions, every time**
- CLI smoke result: PASS — all 13 commands' `--help`, all documented error scenarios, `--version`, `cache <sub> --help` in all 4 forms, all re-verified against the final binary
- JSON result: PASS — all 7 JSON-output combinations (`search` compact/pretty, `references`, `callers`, `callees`, `symbols` compact/pretty) parse cleanly with a real JSON parser, including the empty-result case
- Platform result: Windows only (consistent with Phase 11/12 scope; no other environment available)
- Working tree status: `ast-tool.md` (task file) + the 4 source files above modified; nothing committed (per this session's standing practice of never committing without an explicit request — flagged for the user to review and commit)

## 8. Phase 13 Final Recommendation

### **PASS — READY FOR DOCUMENTATION**

Justification against the acceptance criteria:

1. Every public command has understandable help — **yes**, including the two subcommands that previously fell back to a generic page.
2. Top-level help clearly explains the tool and command surface — **yes**, unchanged content, now also lists `--version`.
3. Required/optional arguments are understandable — **yes**, reviewed across all 13 commands, no changes needed.
4. Common user errors produce useful messages — **yes**, verified against the full Error UX Matrix.
5. Valid empty results are distinguishable from failures — **yes**, and `symbols --json`'s empty case is now also correctly `[]` rather than broken output.
6. Ambiguous results are understandable — **yes**, unchanged, guards intact.
7. stdout/stderr behavior remains automation-safe — **yes**, re-verified byte-exact.
8. JSON output remains valid and documented enough to consume — **yes, and materially improved**: a previously-broken JSON command (`symbols --json`) is now valid and consistent with the rest of the CLI.
9. Installed version can be identified — **yes, newly implemented** (`ast-tool --version` / `-v`).
10. No mandatory interactive behavior blocks scripts or Coding Agents — **yes**, confirmed no interactive code paths exist anywhere.
11. Terminology is reasonably consistent — **yes**, one inconsistency found and fixed (`qualified_name`→`fqn`).
12. All externally visible changes are recorded — **yes**, §3.
13. Relevant Phase 12 regression gates still pass — **yes**, full suite rerun after every change, 0 regressions throughout.
14. No unresolved CLI release blocker remains — **yes** — if anything, this phase *removed* a real one (`symbols --json`'s invalid output), which arguably should have gated Phase 12 had it been caught there.

The most significant outcome of this phase was catching and fixing a genuine, previously-undetected **invalid JSON output defect** in `symbols --json` — exactly the class of issue Phase 13's JSON UX review (13k) exists to catch. Combined with the `--version` addition and the `cache <sub> --help` discoverability fix, the CLI is now measurably more consistent and correct than the Phase 12 baseline, with zero regressions and zero scope creep beyond what each fix concretely justified.
