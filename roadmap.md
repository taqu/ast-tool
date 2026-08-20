# AST Tool — Progress & Roadmap

## 1. Architecture

```text
Tree-sitter
     │
     ▼
   AST IR
     │
     ▼
Semantic Layer
     │
     ▼
Workspace Analysis
     │
     ▼
Semantic Services
     │
     ├── Search
     ├── Resolution
     ├── References
     ├── Callers / Callees
     ├── Semantic Diff
     └── Context Export
     │
     ▼
AI Agent / CLI / IDE
```

### Design principles

* Tree-sitter は parsing に限定
* AST IR が安定した中間表現
* Semantic Layer は AST IR のみに依存
* Workspace が複数ファイルの semantic information を集約
* Semantic Services は parser-independent
* 上位層から Tree-sitter を直接利用しない
* 言語固有処理は extractor に閉じ込める
* Semantic Provider は optional enrichment
* Agent は parser internals ではなく `ast-tool` を利用する

---

# 2. AST Foundation

**Status: ほぼ完了 ✅**

完了：

```text
Parser integration
AST IR
dump
symbols
outline
range
parent
children
```

残り：

```text
Tree-sitter query integration
```

ただし、現在の Agent / Semantic workflow に対する優先度は低い。

---

# 3. Semantic Extraction

**Status: ほぼ完了 ✅**

完了：

* Qualified name
* Namespace / class hierarchy
* Symbol extraction
* Symbol index
* Scope model
* Scope builder
* Symbol → Scope association
* Lexical scope analysis
* Lexical name lookup

Semantic Layer の基本機能は成立済み。

---

# 4. Workspace Analysis

**Status: Parallelized / Streaming が未完**

### 完了 ✅

* Recursive workspace scan
* Multi-file parsing
* Workspace symbol table
* Include/import discovery
* Dependency graph
* Git ignore support
* File-level parallel analysis
* `get_physical_core_count()` による worker 数制御
* `analyze_workspace()` の共通利用

現在、過剰な worker を無制限に生成する構造ではない。

### 現在の課題

`analyze_workspace()` 内では、ファイル解析そのものは並列化されているが、**workspace scan 時に全 file path を先に集める構造が残っている**。

概念的には：

```text
Current

Scan workspace
      ↓
Collect all file paths
      ↓
Parallel analysis
      ↓
Workspace
```

これを将来的に：

```text
Target

Scanner
   ↓
File discovered
   ↓
Bounded work
   ↓
Worker pool
   ↓
Analysis result
   ↓
Workspace merge
```

へ移行する。

### 次の技術テーマ

**Streaming / bounded-memory workspace analysis**

特に、

* file path の全件保持を避ける
* parallelism を維持する
* in-flight work を bounded にする
* peak memory を測定する
* 大規模 repository で性能を比較する

がポイント。

---

# 5. Semantic Services

**Status: 完了 ✅**

実装済み：

```text
Search
Resolution
References
Callers
Callees
Semantic Diff
Context Export
```

`SemanticContext` や `SemanticDiff` は内部 Service / API として存在する。

重要な判断：

> **存在する Semantic Service をすべて CLI command にする必要はない。**

特に `Context Export` / `Semantic Diff` は現時点では独立 CLI にする必要性は低い。

---

# 6. Semantic CLI

**Status: 完了 ✅**

現在の Agent-facing command set：

```text
ast-tool
 ├── symbols
 ├── outline
 ├── range
 ├── search
 ├── find
 ├── references
 ├── callers
 ├── callees
 ├── parent
 └── children
```

### Semantic commands

```text
search
find
references
callers
callees
```

### Structural inspection

```text
symbols
outline
parent
children
```

### Advanced / low-level

```text
range
dump
```

`dump` は CLI には存在するが、Agent-facing Skill の通常 workflow からは外す方針。

`range` も通常 workflow の中心にはしない。

---

# 7. CLI Common Handling

**Status: 完了 ✅**

`references` / `callers` / `callees` などで、

```cpp
Workspace ws = analyze_workspace(arguments.root_);
```

を個別に実装するのではなく、共通の処理として利用している。

したがって、

> CLIごとの Workspace construction の重複

は現在の課題ではない。

残っている問題は **共通化ではなく `analyze_workspace()` 自体の streaming / memory characteristics**。

---

# 8. CLI Help

**Status: 実装済み / 今後レビュー**

command-specific help は既に存在する。

例えば：

```text
ast-tool references --help
```

も利用可能。

したがって、次に必要なのは Help 機能そのものの実装ではなく、

> **Agent が help だけで適切な command を選択できるか**

という usability review。

特に、

```text
search
find
references
callers
callees
```

の責務が明確かを確認する。

---

# 9. Agent-facing Skills

**Status: 整理完了 ✅**

現在：

```text
skills
├── semantic-analysis
├── ast-inspection
└── api-review
```

### semantic-analysis

```text
search
find
references
callers
callees
```

Semantic relationship / symbol navigation を担当。

### ast-inspection

```text
symbols
outline
parent
children
```

Structural inspection を担当。

### api-review

Semantic commands を組み合わせた高レベル workflow。

```text
Find
  ↓
References
  ↓
Callers
  ↓
Callees
  ↓
Impact assessment
```

以前存在した、

```text
context-export
workspace-analysis
```

は独立 Skill として整理済み。

---

# 10. Agent Evaluation

**Status: 次の段階**

ここから重要になるのが、実際の coding agent に使わせること。

評価対象：

```text
Agent
  ↓
Skill selection
  ↓
ast-tool command selection
  ↓
Command execution
  ↓
Result interpretation
  ↓
Code change
```

確認したいこと：

* Agent は適切な Skill を選べるか
* `search` と `find` を区別できるか
* `references` / `callers` / `callees` を区別できるか
* 構造確認と semantic analysis を使い分けられるか
* Help を適切に利用できるか
* 不要な `dump` / grep 等に逃げないか
* Semantic information を実際の変更判断に利用できるか

ここから得られる実測結果を、CLI / Skills の改善材料にする。

---

# 11. Structured Output

**Status: 後回し**

現時点では大規模な output redesign はしない。

例えば、

```text
JSON
--format json
machine-readable schema
output versioning
```

などはまだ不要。

まず Agent evaluation を行い、

> 出力形式の違いが実際に Agent の判断を妨げる

という問題が確認されてから検討する。

ただし、明らかな不整合は通常の CLI polish の範囲で修正してよい。

---

# 12. Performance / Memory Benchmark

**Status: 次の技術テーマ**

Streaming 化の前後で、

```text
Repository
    ↓
analyze_workspace()
    ↓
metrics
```

を比較できるようにする。

測定候補：

* file count
* total analysis time
* peak memory
* CPU utilization
* worker count
* queue / in-flight work
* throughput

特に重要なのは、

```text
Before
all paths retained

vs.

After
streamed / bounded
```

の peak memory 比較。

---

# 13. Future Workspace Features

Streaming / benchmark が固まった後に検討。

### Incremental Analysis

```text
File changed
    ↓
Re-analyze file
    ↓
Update Workspace
```

### Filesystem Watch

```text
Filesystem
    ↓
Watcher
    ↓
Workspace update
```

### Persistent Cache

必要性が確認できた場合のみ。

現時点では必須ではない。

---

# 14. Future Semantic Extensions

Core architecture が安定した後に検討。

候補：

```text
Definition
Dependencies
Dependents
Inheritance
Overrides
Type hierarchy
Diagnostics
Documentation
```

ただし、**今は追加しない**。

まず既存 Semantic Services を Agent が実際に使えることを確認する。

---

# 現在のロードマップ

```text
AST Foundation
      │
      ▼
Semantic Extraction
      │
      ▼
Workspace Analysis
      │
      ├── Parallelization ──────── ✅
      ├── Worker limit ─────────── ✅
      └── Streaming ────────────── → Next
      │
      ▼
Semantic Services
      │
      └─────────────────────────── ✅
      │
      ▼
Semantic CLI
      │
      └─────────────────────────── ✅
      │
      ▼
Agent-facing Skills
      │
      └─────────────────────────── ✅
      │
      ▼
Agent Evaluation
      │
      ├── Command selection
      ├── Result interpretation
      └── Real coding tasks
      │
      ▼
CLI / Skill refinement
      │
      ├── Help / examples
      └── Structured output
      │
      ▼
Workspace optimization
      │
      ├── Streaming
      ├── Benchmark
      └── Memory optimization
      │
      ▼
Future
      ├── Incremental analysis
      ├── Filesystem watch
      ├── Persistent cache
      └── Semantic extensions
```

---

# 現在の優先順位

| Priority | Item                                     | Status               |
| -------- | ---------------------------------------- | -------------------- |
| **S**    | Semantic Services                        | ✅ Complete           |
| **S**    | `references` / `callers` / `callees` CLI | ✅ Complete           |
| **S**    | Common Workspace handling                | ✅ Complete           |
| **S**    | Workspace parallel analysis              | ✅ Complete           |
| **S**    | Worker count control                     | ✅ Complete           |
| **A**    | Agent evaluation                         | **Next**             |
| **A**    | Help / examples usability review         | Next                 |
| **A**    | Workspace streaming                      | Next technical topic |
| **A**    | Memory / performance benchmark           | Streamingと併行/後       |
| **B**    | Structured output consistency            | Evaluation後          |
| **B**    | Tree-sitter Query                        | 未完だが優先度低             |
| **C**    | Incremental analysis                     | Future               |
| **C**    | Filesystem watch                         | Future               |
| **C**    | Persistent cache                         | Future               |
| **C**    | External semantic providers              | Future               |

## 現在地を一言でいうと

**AST / Semantic infrastructure はほぼ完成し、Semantic CLI と Agent-facing Skills まで整った。**

現在は、

```text
「機能を増やす」
        ↓
「Agentに実際に使わせて評価する」
```

への切り替え地点です。

その一方で Workspace 側には、

```text
parallel analysis       ✅
bounded worker count    ✅
streaming               ☐
```

という明確な性能改善テーマが残っています。

したがって次の議論では、**「Agent Evaluation を先に進めるか」「`analyze_workspace()` の streaming を先に詰めるか」**の2本を軸に考えるのがよい状態です。
