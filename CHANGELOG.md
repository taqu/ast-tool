# Changelog

## [0.1.0] — 2026-09-08

Initial release.

### Features

- Structural commands: `outline`, `find`, `range`, `parent`, `children`
- Semantic commands: `symbols`, `search`, `references`, `callers`, `callees`
- Cache management: `cache warm`, `cache status`
- Setup: `setup`
- JSON output via `--json` flag on all commands
- `--version` / `-v` flag
- 15 supported languages: Bash, C, C++, C#, CSS, Go, HTML, Java, JavaScript,
  Python, Ruby, Rust, Scala, TypeScript, TSX
- Persistent workspace cache (SQLite, LZ4-compressed) for fast repeated queries
- Windows x64 binary release (MSVC 2022, statically linked MSVC runtime)

### Known Limitations

- Workspace and file paths must stay within the active Windows code page
- No inferred receiver typing (auto, decltype, alias expansion)
- No inheritance-aware lookup or virtual dispatch resolution
- No transitive relationship traversal
- `cmake --install` not supported; copy the `bin/` directory manually
- Linux build is best-effort (prerequisites documented; not release-qualified)
- macOS is not supported

[0.1.0]: https://github.com/taqu/ast-tool/releases/tag/v0.1.0
