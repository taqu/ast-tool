---
name: context-export references
description: Quick-reference card for combining ast-tool commands into context packages
---

# Context Export References

## Command Summary

| Command           | Input scope     | Output content                        |
|-------------------|-----------------|---------------------------------------|
| `dump <file>`     | single file     | All AST nodes (depth-first)           |
| `outline <file>`  | single file     | Named nodes, indented hierarchy       |
| `symbols <file>`  | single file     | Semantic declarations + locations     |
| `search <dir>`    | entire directory| Semantic declarations across all files|
| `find <file>`     | single file     | Nodes matching type/text/position/ID  |
| `range <file>`    | single file     | Nodes in a source range               |
| `parent <file>`   | single file     | Parent of a given node ID             |
| `children <file>` | single file     | Children of a given node ID           |

## Recommended Context Bundles

### Minimal (one file, human review)
```
outline <file>
symbols <file>
```

### Standard (one file, LLM review)
```
outline --json <file>
symbols --json <file>
```
Merge arrays; pass as `{"outline": [...], "symbols": [...]}`.

### Module (multiple files, JSON)
```
search --file "<subdir>" --json <root>
```

### Full workspace index
```
search --json <root>  →  .nav-index.json
```

## Output Format Reference

| Flag       | Effect                          |
|------------|---------------------------------|
| (none)     | Plain text, one item per line   |
| `--json`   | Compact JSON array              |
| `--pretty` | Indented JSON array             |

## jq Snippets for Context Assembly

```bash
# Merge outline and symbols JSON into one object
jq -n \
  --slurpfile outline outline.json \
  --slurpfile symbols symbols.json \
  '{outline: $outline[0], symbols: $symbols[0]}'

# Filter symbols to public API (functions + classes)
jq '[.[] | select(.kind == "function" or .kind == "class")]' symbols.json

# Sort by file then line
jq 'sort_by(.file, .line)' workspace.json
```
