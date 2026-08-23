# AST Tool — Progress & Roadmap

## 現在地

**AST / Semantic infrastructure、Semantic CLI、Agent-facing Skills、そして Agent Evaluation の評価セットと実行・統計基盤まで整った状態です。**

プロジェクトは現在、

```text
「Semantic capability を実装する」
        ↓
「Coding Agent が実際に使うか評価する」
        ↓
「評価結果を基に改善する」
```

というフェーズに移っています。

---

# 1. Architecture

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

## Design principles

* Tree-sitter は parsing に限定
* AST IR を安定した中間表現とする
* Semantic Layer は AST IR のみに依存
* Workspace が複数ファイルの semantic information を集約
* Semantic Services は parser-independent
* 上位層から Tree-sitter を直接利用しない
* 言語固有処理は extractor に閉じ込める
* Semantic Provider は optional enrichment
* Agent は parser internals ではなく `ast-tool` を利用する

**Status: 安定 / 基本設計完了 ✅**

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

ただし現在の Agent / Semantic workflow に対する優先度は低い。

**Priority: B〜低**

---

# 3. Semantic Extraction

**Status: 完了に近い / 基本機能成立 ✅**

実装済み：

* Qualified name
* Namespace / class hierarchy
* Symbol extraction
* Symbol index
* Scope model
* Scope builder
* Symbol → Scope association
* Lexical scope analysis
* Lexical name lookup

Semantic Layer の基盤は成立済み。

---

# 4. Workspace Analysis

## 完了 ✅

* Recursive workspace scan
* Multi-file parsing
* Workspace symbol table
* Include/import discovery
* Dependency graph
* Git ignore support
* File-level parallel analysis
* `get_physical_core_count()` による worker 数制御
* `analyze_workspace()` の共通利用

CLI ごとに Workspace 構築処理を重複しているわけではなく、

```cpp
Workspace ws = analyze_workspace(arguments.root_);
```

を中心とした共通構造になっている。

## 現在の性能課題

現状：

```text
Scan workspace
      ↓
Collect all file paths
      ↓
Parallel analysis
      ↓
Workspace
```

ファイル解析は並列化済みだが、workspace scan 時に file path を全件保持する。

将来的な改善案：

```text
Scanner
   ↓
File discovered
   ↓
Bounded work queue
   ↓
Worker pool
   ↓
Analysis result
   ↓
Workspace merge
```

目的：

* file path 全件保持を避ける
* parallelism を維持
* in-flight work を bounded にする
* peak memory を削減
* 大規模 repository で性能比較

**Status: Streaming 未実装 ☐**
**Priority: A**

---

# 5. Semantic Services

**Status: Complete ✅**

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

現時点では、

```text
Context Export
Semantic Diff
```

を独立した Agent-facing CLI command にする必要性は確認されていない。

---

# 6. Semantic CLI

**Status: Complete ✅**

現在の CLI：

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
 ├── children
 └── dump
```

## Semantic commands

```text
search
find
references
callers
callees
```

## Structural inspection

```text
symbols
outline
parent
children
```

## Advanced / low-level

```text
range
dump
```

`dump` は存在するが、通常の Agent workflow からは外す。

`range` も Agent workflow の中心にはしない。

---

# 7. CLI Help / Usability

**Status: 基本実装済み、Evaluation で検証可能 ✅**

例えば、

```text
ast-tool references --help
```

は利用可能。

実際の Level 2 evaluation では、

```json
"ast_tool": {
    "search": 4,
    "callers": 8,
    "help": 2,
    "references": 1,
    "symbols": 1
}
```

のように Agent が `help` を利用するケースも確認されている。

今後の焦点は、

> Help が存在するか

ではなく、

> Agent が Help を読んで正しい command を選択できるか

である。

**Status: Evaluation data を基に今後レビュー**

---

# 8. Agent-facing Skills

**Status: 整理完了 / 実際の評価対象として使用中 ✅**

現在：

```text
ast-tool/.claude/skills/
├── semantic-analysis/
│   └── SKILL.md
├── ast-inspection/
│   └── SKILL.md
└── api-review/
    └── SKILL.md
```

## semantic-analysis

対象：

```text
search
find
references
callers
callees
```

Semantic relationship / symbol navigation を担当。

## ast-inspection

対象：

```text
symbols
outline
parent
children
```

Structural inspection を担当。

## api-review

Semantic commands を組み合わせた高レベル workflow。

```text
Find symbol
      ↓
References
      ↓
Callers
      ↓
Callees
      ↓
Impact assessment
```

重要なのは、

> Skills は Agent に正しい semantic workflow を提示するための interface

として評価対象になっていること。

---

# 9. Agent Evaluation Infrastructure

**Status: 構築完了 ✅**

Agent Evaluation は `Claude Code` を対象として進めている。

基本フロー：

```text
Evaluation Task
      ↓
Claude Code
      ↓
Skills
      ↓
Tool selection
      ├── Grep
      ├── Read
      ├── Bash
      └── ast-tool
              ├── search
              ├── references
              ├── callers
              ├── callees
              └── help
      ↓
Code modification
      ↓
Validation
      ↓
JSONL result
      ↓
Statistics
```

取得している情報：

```json
{
  "task_id": "...",
  "success": true,
  "elapsed_seconds": 181.52,
  "tokens": {},
  "tools": {},
  "ast_tool": {},
  "workflow": [],
  "changed_files": [],
  "validation": {}
}
```

---

# 10. Evaluation Tasks

**Status: Level-based evaluation set 作成済み ✅**

現在、

```text
smoke
level1
level2
level3
level4
level5
```

という段階的な評価セットを構築済み。

概念的には、

```text
Smoke
  ↓
Basic symbol/file navigation

Level 1
  ↓
Simple local modification

Level 2
  ↓
Cross-file semantic navigation
  ↓
ast-tool が有利になり始める

Level 3
  ↓
Multiple files / relationships
  ↓
references / callers / search 等の利用価値

Level 4
  ↓
More complex impact analysis
  ↓
Semantic workflow が重要

Level 5
  ↓
High-complexity repository navigation
  ↓
Tool selection / semantic reasoning の評価
```

Level 1 の結果では主に、

```text
Grep
Read
Edit
```

で成功していた。

これは重要なベースラインである。

一方、Level 2 では、

```json
"ast_tool": {
    "callers": 4,
    "search": 2,
    "references": 1
}
```

あるいは、

```json
"ast_tool": {
    "search": 4,
    "callers": 8,
    "help": 2,
    "references": 1,
    "symbols": 1
}
```

のように、Agent が実際に `ast-tool` と Skills を利用するケースが確認された。

---

# 11. Evaluation Runner / Logging

**Status: 完了 / 再生成済みの評価スクリプトを含め基盤整備済み ✅**

既存の Claude Code の local JSONL log を利用して、

```text
~/.claude/projects
```

から、

* input tokens
* output tokens
* cache read tokens
* cache creation tokens
* tool usage

を集計する方式を利用。

ノイズ除去のため、

```python
clear_claude_logs()
```

でログを初期化し、

テストごとの実行結果を独立して取得できる構造。

さらに workflow から、

```text
Skill
Bash
Read
Edit
Grep
Glob
Agent
```

等の tool sequence を保存。

Bash 内で `ast-tool` が実行された場合は、

```json
"ast_tool": {
    "search": 4,
    "callers": 8,
    "references": 1
}
```

のように command 単位で抽出する。

---

# 12. Statistics / Result Analysis

**Status: 作成済み ✅**

テストランナーとは分離した統計スクリプトを作成。

基本構造：

```text
evaluation run
      ↓
results.jsonl
      ↓
statistics / analysis
      ↓
summary
CSV
```

取得対象：

## Global

```text
total tasks
success / failure
timeout
elapsed time
token usage
tool usage
ast-tool usage
```

## Per Level

```text
tasks
success rate
average time
median time
average token usage
```

## ast-tool

```text
command usage
search
find
references
callers
callees
symbols
help
...
```

さらに今後重要になる指標：

```text
ast-tool adoption rate
```

つまり、

```text
Tasks using ast-tool
/
Total tasks
```

である。

---

# 13. 次に見るべき Evaluation Metrics

評価セット自体はできたため、次は**実データの分析フェーズ**。

特に見るべきなのは以下。

## A. Success Rate by Level

```text
Level
  ↓
Success rate
```

例：

```text
Smoke   100%
Level 1 100%
Level 2  90%
Level 3  85%
Level 4  70%
Level 5  50%
```

難易度設計が適切なら、ある程度の難易度上昇に伴う成功率変化が見える。

---

## B. ast-tool Adoption by Level

```text
Level
  ↓
Tasks that used ast-tool
```

例：

```text
Smoke    0%
Level 1  0%
Level 2 60%
Level 3 80%
Level 4 90%
Level 5 95%
```

これは、

> Task difficulty が上がるほど semantic tooling が自然に使われるか

を見る重要な指標。

---

## C. Command Usage

```text
search
find
references
callers
callees
symbols
help
```

について、

```text
どの command が実際に使われたか
```

を見る。

例えば、

```text
callers     32
search      24
references  18
callees      3
find         0
```

となった場合、

```text
callees
find
```

の discoverability や Skill 上の説明を疑うことができる。

---

## D. Workflow Analysis

単なる回数ではなく、

```text
Skill
  ↓
ast-tool search
  ↓
ast-tool callers
  ↓
Read
  ↓
Edit
```

のような workflow が成立しているかを見る。

理想的には、

```text
Skill selection
      ↓
Semantic discovery
      ↓
Target narrowing
      ↓
Read
      ↓
Edit
```

という流れ。

逆に、

```text
Grep
Grep
Read
Read
Read
Read
...
```

だけで完結している場合、

* Task が簡単すぎる
* ast-tool の優位性がない
* Skill が選ばれていない
* Command UX が弱い

などを疑う。

---

# 14. Structured Output

**Status: 後回し ☐**

現時点では、

```text
JSON
--format json
machine-readable schema
output versioning
```

などの大規模 redesign は行わない。

理由：

> まず Agent Evaluation の実測で、本当に output format が問題になるか確認する。

ただし、

```text
明らかな CLI output inconsistency
```

が見つかった場合は通常の polish として修正してよい。

**Priority: B**

---

# 15. Workspace Streaming / Performance

**Status: 次の独立した技術テーマ ☐**

Evaluation と並行して進められる。

現在：

```text
parallel analysis       ✅
bounded worker count    ✅
streaming               ☐
benchmark               ☐
memory optimization     ☐
```

Streaming 化前後で比較したい指標：

```text
file count
total analysis time
peak memory
CPU utilization
worker count
queue depth
in-flight work
throughput
```

比較対象：

```text
Before
all file paths retained

vs.

After
streamed / bounded pipeline
```

---

# 16. Future Workspace Features

Streaming / benchmark の結果を見てから検討。

## Incremental Analysis

```text
File changed
      ↓
Re-analyze file
      ↓
Update Workspace
```

## Filesystem Watch

```text
Filesystem
      ↓
Watcher
      ↓
Workspace update
```

## Persistent Cache

必要性が確認された場合のみ導入。

現時点では必須ではない。

---

# 17. Future Semantic Extensions

Core architecture と Agent Evaluation が安定してから検討。

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

ただし現時点では、

> **追加しない。**

まず既存の、

```text
search
find
references
callers
callees
```

が Agent にとって十分有効か確認する。

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
      ├── Parallelization ──────────── ✅
      ├── Worker limit ─────────────── ✅
      └── Streaming ────────────────── ☐
      │
      ▼
Semantic Services
      │
      └─────────────────────────────── ✅
      │
      ▼
Semantic CLI
      │
      └─────────────────────────────── ✅
      │
      ▼
Agent-facing Skills
      │
      └─────────────────────────────── ✅
      │
      ▼
Evaluation Infrastructure
      │
      ├── Claude Code runner ───────── ✅
      ├── Task validation ──────────── ✅
      ├── Tool / token logging ─────── ✅
      └── JSONL results ────────────── ✅
      │
      ▼
Evaluation Dataset
      │
      ├── Smoke ────────────────────── ✅
      ├── Level 1 ──────────────────── ✅
      ├── Level 2 ──────────────────── ✅
      ├── Level 3 ──────────────────── ✅
      ├── Level 4 ──────────────────── ✅
      └── Level 5 ──────────────────── ✅
      │
      ▼
Statistics / Analysis
      │
      └─────────────────────────────── ✅
      │
      ▼
▶ Evaluation Run & Result Analysis ◀
      │
      ├── Success rate
      ├── Level difficulty validation
      ├── ast-tool adoption
      ├── Command usage
      ├── Workflow analysis
      └── Failure analysis
      │
      ▼
CLI / Skill Refinement
      │
      ├── Help / examples
      ├── Skill wording
      ├── Command discoverability
      └── Output consistency
      │
      ▼
Workspace Optimization
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

| Priority | Item                                     | Status                   |
| -------- | ---------------------------------------- | ------------------------ |
| **S**    | AST / Semantic infrastructure            | ✅ Complete               |
| **S**    | Semantic Services                        | ✅ Complete               |
| **S**    | Semantic CLI                             | ✅ Complete               |
| **S**    | Agent-facing Skills                      | ✅ Complete               |
| **S**    | Evaluation runner / logging              | ✅ Complete               |
| **S**    | Evaluation dataset (Smoke–Level 5)       | ✅ Complete               |
| **S**    | Statistics / result analysis             | ✅ Complete               |
| **A**    | **Full evaluation run**                  | **Next**                 |
| **A**    | Level difficulty validation              | Next                     |
| **A**    | ast-tool adoption analysis               | Next                     |
| **A**    | Command / workflow analysis              | Next                     |
| **A**    | Failure analysis                         | Next                     |
| **A**    | CLI / Skill refinement based on evidence | After analysis           |
| **A**    | Workspace streaming                      | Parallel technical topic |
| **A**    | Memory / performance benchmark           | Streamingと併行 / 後         |
| **B**    | Structured output consistency            | Evaluation後              |
| **B**    | Tree-sitter Query                        | 未完だが低優先                  |
| **C**    | Incremental analysis                     | Future                   |
| **C**    | Filesystem watch                         | Future                   |
| **C**    | Persistent cache                         | Future                   |
| **C**    | External semantic providers              | Future                   |
| **C**    | Additional semantic services             | Future                   |

---

# 現在地を一言でいうと

**「ast-tool を作るフェーズ」はほぼ終わり、現在は「ast-tool が Coding Agent の実作業で本当に使われ、役に立つことを測定するフェーズ」に入っています。**

次の中心テーマは明確です。

```text
Evaluation Dataset
        ↓
Full Evaluation Run
        ↓
Statistics
        ↓
Failure / Workflow Analysis
        ↓
CLI / Skill Improvement
```

そして並行する技術テーマとして、

```text
analyze_workspace()
        ↓
Streaming / bounded-memory design
        ↓
Benchmark
        ↓
Memory optimization
```

があります。

したがって次の議論では、まず **Smoke〜Level 5 を実行した結果をどう評価するか、特に「ast-tool を使ったか」ではなく「どの難易度・どの種類の問題で、どの Semantic command が Agent の成功に寄与したか」を分析すること**が、最も自然な次のステップです。
