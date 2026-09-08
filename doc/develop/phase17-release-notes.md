# ast-tool v0.1.0

Initial public release of ast-tool, a command-line tool for structural source-code inspection and semantic relationship analysis.

## Highlights

- Inspect syntax trees with `outline`, `find`, `range`, `parent`, and `children`.
- Discover symbols with `symbols` and `search`.
- Trace semantic relationships with `references`, `callers`, and `callees`.
- Produce machine-readable JSON output for scripting and agent integrations.
- Maintain and warm a persistent workspace cache.
- Analyze C, C++, C#, CSS, Go, HTML, Java, JavaScript, Python, Ruby, Rust, Scala, TSX, and TypeScript source files.

## Supported platforms

- Windows 10/11 x64: tested and supported.
- Linux x64: best effort; source and binary smoke validation passed on Debian 13 (x86_64), but Linux did not receive the full Windows release-qualification cycle.

macOS, ARM64, and MinGW are not supported in this release.

## Requirements

Windows source builds require Visual Studio 2022 or newer with the MSVC C++ workload and CMake 3.11 or newer.

Linux source builds require `build-essential`, CMake 3.11 or newer, `pkg-config`, `libre2-dev`, and `libgit2-dev`. The Linux binary requires compatible RE2 and libgit2 runtime libraries; see the included README for distro-specific guidance.

## Installation

Download the archive for your platform, verify it with `SHA256SUMS`, extract it, and add the extracted directory to `PATH`. Source build and install instructions are in the included README.

## Known limitations

- On Windows, workspace and file paths must use characters representable in the active Windows code page. Mixed-script paths outside the system locale are not supported.
- Semantic analysis is syntax-driven and is not a full compiler/type checker; overloads, templates, macros, aliases, virtual dispatch, and generated code can yield conservative, ambiguous, or incomplete relationships.
- Cache invalidation is based on modification time and file size.
- CLI and JSON formats may evolve during the pre-1.0 series.

## Artifacts

- `ast-tool-0.1.0-windows-x64.zip`
- `ast-tool-0.1.0-linux-x64.tar.gz`
- `ast-tool-0.1.0-source.tar.gz`
- `SHA256SUMS`

## SHA-256

```text
f1d6f707ef9c75617c25f0cecc09578d067442fc4148a31954a62635704081d5  ast-tool-0.1.0-windows-x64.zip
d108d33f27610fb1d91f7af60b0e91f4df52db8b6e4c9fe4373b96cc73e53bf3  ast-tool-0.1.0-linux-x64.tar.gz
f579661920c17b6e76699351ea2db13020f5e28bd05631d89d9e569f068c28f4  ast-tool-0.1.0-source.tar.gz
```

Documentation is included in every archive and is also available in the repository README and wiki.
