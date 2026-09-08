# Phase 17 Report — Initial Public Release

## Release record

| Field | Value |
|---|---|
| Version | `v0.1.0` |
| Release date | 2026-09-09 (JST) |
| Source commit | `2418a9701cf540ec0d11f2cbbbebe30ccb3693a3` |
| Git tag | `v0.1.0` |
| Publication | [GitHub Release](https://github.com/taqu/ast-tool/releases/tag/v0.1.0) |
| Promotion policy | Exact Phase 16 artifact promotion; no rebuild |
| Phase 16 result | PASS WITH KNOWN LIMITATIONS — RC APPROVED |

The source archive's Git pax header records commit `2418a9701cf540ec0d11f2cbbbebe30ccb3693a3`. The public tag points directly to that commit. Later repository commits contain validation and operational documentation and are not part of the released source tree.

## Final artifact manifest

| Filename | Platform | Architecture | Type | Size (bytes) | SHA-256 |
|---|---|---|---|---:|---|
| `ast-tool-0.1.0-windows-x64.zip` | Windows 10/11 | x64 | Binary archive | 4,452,883 | `f1d6f707ef9c75617c25f0cecc09578d067442fc4148a31954a62635704081d5` |
| `ast-tool-0.1.0-linux-x64.tar.gz` | Linux | x64 | Binary archive | 17,851,877 | `d108d33f27610fb1d91f7af60b0e91f4df52db8b6e4c9fe4373b96cc73e53bf3` |
| `ast-tool-0.1.0-source.tar.gz` | Source | — | Source archive | 65,562,776 | `f579661920c17b6e76699351ea2db13020f5e28bd05631d89d9e569f068c28f4` |
| `SHA256SUMS` | All | — | Checksum manifest | 293 | `1c297052093cd2c1f98ddb92e6671a5cb072e695f9cc5e6ff8af5b78abceb5b0` |

Immediately before publication, all three artifact hashes matched `release-artifacts/SHA256SUMS`. After publication, all four downloaded files were byte-identical to the approved local files, and the downloaded manifest verified every artifact successfully.

## Supported platforms and requirements

- Windows 10/11 x64 is tested and supported. Source builds require MSVC 2022 or newer and CMake 3.11 or newer.
- Linux x64 is best effort. Source and binary smoke validation passed on Debian 13 x86_64, but Linux did not receive the full Windows qualification cycle. Source builds require `build-essential`, CMake 3.11 or newer, `pkg-config`, `libre2-dev`, and `libgit2-dev`.
- macOS, ARM64, and MinGW are not supported in v0.1.0.

## Known limitations

- Windows paths containing characters outside the active Windows code page are unsupported.
- Semantic analysis is syntax-driven rather than a full compiler/type checker; advanced language constructs can produce conservative, ambiguous, or incomplete relationships.
- Cache invalidation uses modification time and file size.
- CLI and JSON formats may evolve before 1.0.
- Windows clean-environment validation used a host fallback because Windows Sandbox was unavailable.

## Publication and post-publication verification

| Check | Result |
|---|---|
| Release page publicly accessible | PASS |
| Release title/tag/version agree | PASS — `ast-tool v0.1.0` / `v0.1.0` / `ast-tool 0.1.0` |
| Tag target | PASS — `2418a9701cf540ec0d11f2cbbbebe30ccb3693a3` |
| Expected assets present | PASS — four of four |
| Public downloads complete | PASS |
| Downloaded checksums | PASS |
| Published/local artifact identity | PASS — byte-identical |
| Release-note required sections | PASS |
| Tagged README link | PASS — HTTP 200 |
| Public Windows archive smoke | PASS — `--version`, `--help`, search, callers, JSON |
| Public source archive configure/build | PASS — Visual Studio 2022 Release build |
| Public source-built `--version` | PASS — `ast-tool 0.1.0` |
| License and notices | PASS — `LICENSE`, `NOTICE`, `README.md`, and `CHANGELOG.md` included in every archive; dependency notices included in the source archive |

The published tag and artifacts are immutable. Any correction requiring changed binaries or source will use a new version.

## Post-release backlog

Deferred work, with no priority implied:

- Complete Unicode path handling on Windows.
- Expand semantic handling for overloads, templates, macros, aliases, virtual dispatch, and generated code.
- Evaluate stronger cache invalidation.
- Run additional clean-environment and Linux-distribution qualification.
- Evaluate additional platforms and package-manager distribution based on user demand.
- Consider performance work, new CLI features, and IDE integrations based on measured usage.

## Final recommendation

**RELEASE COMPLETE WITH KNOWN LIMITATIONS**

The public source revision, tag, documented version, published artifacts, and verified checksums agree. No critical release defect is known.
