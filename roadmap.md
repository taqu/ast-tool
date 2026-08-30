# AST Tool — Roadmap / Progress

**基準日: 2026-08-30**

## 1. 最終目標

AST Tool の最終的な構成は：

```text
Tree-sitter
    ↓
AST IR
    ↓
Semantic Layer
    ↓
Workspace Analysis
    ↓
Semantic Services
    ├── Search
    ├── Resolution
    ├── References
    ├── Callers / Callees
    ├── Semantic Diff
    └── Context Export
    ↓
AI Agent / CLI / IDE
```

設計原則：

* Tree-sitter は parsing に限定
* AST IR を stable intermediate representation とする
* Semantic Layer は AST IR のみに依存
* Workspace が複数ファイルの semantic information を集約
* Semantic Services は parser-independent
* 上位レイヤーから Tree-sitter を直接触らない
* language-specific processing は extractor に閉じ込める
* Semantic Provider による optional enrichment
* Agent は parser internals ではなく `ast-tool` を利用

---

# 2. Agent Evaluation

ここはかなり進んでいます。

## Evaluation test

YAML ベースの test framework を構築。

例：

```text
smoke-001.yaml
level1-xxx.yaml
level2-xxx.yaml
...
```

Task level を分けて、

```text
Level 1
    ↓
Level 2
    ↓
Level 3
    ↓
Level 4
    ↓
Level 5
```

と Agent の能力を段階的に評価する方針。

特に Level 2 以降では `ast-tool` の利用状況も評価対象。

---

## 実行結果の例

既に成功している test がある。

### smoke-001

```text
success
elapsed_seconds: 15.56
```

### level1-001

```text
success
elapsed_seconds: 13.78
tools:
  Grep
  Read
  Edit
```

### level2-008

```text
success
elapsed_seconds: 100.6

tools:
  Skill: 1
  Bash: 10
  Read: 5
  Edit: 5

ast_tool:
  callers: 4
  search: 2
  references: 1
```

### level2-004

```text
success
elapsed_seconds: 181.52

tools:
  Skill: 1
  Bash: 17
  Read: 5
  Glob: 1
  Grep: 1
  Edit: 6

ast_tool:
  search: 4
  callers: 8
  help: 2
  references: 1
  symbols: 1
```

つまり、**「Agent が task を成功させたか」だけでなく、「どの tool をどう使ったか」まで trace から分析できる段階**まで来ています。

---

# 3. 次のフェーズ：Tool-use Trace Analysis

ここが**次回のメインテーマ**です。

今までは主に：

```text
Task
 ↓
Agent
 ↓
Success / Failure
```

を見ていました。

次は：

```text
Task
 ↓
Agent
 ↓
Tool-use trace
 ↓
AST Tool usage
 ↓
Outcome
```

を調べます。

特に見るべきなのは：

### A. Agent が `ast-tool` を使ったタイミング

```text
Task start
    ↓
Grep
    ↓
Read
    ↓
ast-tool search
    ↓
ast-tool callers
    ↓
Edit
```

なのか、

```text
Task start
    ↓
ast-tool search
    ↓
ast-tool references
    ↓
Read
    ↓
Edit
```

なのか。

---

### B. `ast-tool` を使った結果、本当に tool-use が減ったか

比較したいのは：

```text
Without ast-tool
    ↓
Grep / Glob / Read / Bash
    ↓
many tool calls
```

vs.

```text
With ast-tool
    ↓
search / callers / references
    ↓
fewer exploratory calls
```

です。

特に、

* tool call count
* Read count
* Grep count
* Glob count
* Bash count
* AST Tool count
* total elapsed time
* input/output tokens

の関係を見る。

---

### C. `ast-tool` を使っているのに効率化していないケース

これも重要。

例えば：

```text
Grep
Read
Grep
Read
ast-tool search
Read
Grep
Edit
```

なら、

> AST Tool は使われたが、Agent の探索戦略そのものは変わっていない

可能性があります。

逆に：

```text
ast-tool search
ast-tool callers
Read
Edit
```

なら、かなり理想的。

---

### D. Task Level と tool-use の関係

特に：

```text
Level 1
Level 2
Level 3
Level 4
Level 5
```

で、

```text
difficulty
    ↓
AST Tool usage
    ↓
tool call efficiency
    ↓
success rate
```

がどう変化するかを見る。

---

# 4. AST Cache / Workspace Performance

現在こちらも大きな設計変更を進めています。

従来：

```text
workspace 全体
    ↓
AST を全部メモリに展開
    ↓
find/search/etc.
```

これが workspace 全体の AST 展開で bottleneck になっている。

---

## 新しい方針

```text
workspace
    ↓
path を streaming
    ↓
path ごとに AST を memory に展開
    ↓
find/search
    ↓
hit した AST を残す
```

つまり、workspace 全体を一度に AST 化して保持するのではなく、

```text
Path
 ↓
AST
 ↓
command
 ↓
必要な AST だけ保持
```

という方向。

DB cache は後段。

---

# 5. AST Binary Cache

その次の optimization として、

```text
AST
 ↓
binary serialization
 ↓
LZ4
 ↓
SQLite
```

という persistent cache を導入。

すでに **LZ4 と SQLite を使える状態**まで進んでいます。

Cache validation は：

```text
format_version
source_size
source_mtime
```

を fast path として利用。

一致しなければ：

```text
source hash
```

を確認。

hash が一致する場合は AST の再解析を避け、

```text
mtime / size
```

だけ更新する。

hash も異なる場合：

```text
parse
 ↓
serialize
 ↓
LZ4
 ↓
SQLite
```

---

# 6. Cache Warming の並列化

現在の `warm_cache` は sequential loop。

```text
for path:
    stat
    metadata lookup
    hash
    parse
    store
```

これを既存の `BlockingQueue` ベースの workspace analysis と同じモデルに変更する方針。

## 最終構成

```text
Directory Scanner
        ↓
BlockingQueue<Path>
        ↓
 ┌──────┼──────┬──────┐
 ↓      ↓      ↓      ↓
Worker Worker Worker Worker
 │      │      │      │
 │ stat
 │ metadata lookup
 │ hash
 │ parse
 │ serialize
 │ LZ4
 │
 └──────┬──────┴──────┘
        ↓
BlockingQueue<CacheWriteRequest>
        ↓
Single SQLite Writer
        ↓
batched transaction
```

既存の：

```cpp
BlockingQueue<T>
```

を再利用する。

新しい thread pool / task scheduler は作らない。

---

## SQLite

worker は DB read を行うが、write は直接行わない。

```text
Workers
    ↓
read-only SQLite connection
```

↓

```text
CacheWriteRequest Queue
```

↓

```text
Single Writer
    ↓
SQLite transaction
```

という構造。

これにより AST parsing / serialization / LZ4 compression は並列化しつつ、DB write は単純化。

---

# 7. Background Cache Warming

さらに SessionStart から cache warming を開始する方針。

狙い：

```text
SessionStart
    ↓
background cache warm
    ↓
Agent immediately starts working
```

tool use が実際に始まるまでに cache warming を先行させる。

以前の Session ですでに workspace が scan 済みなら、

```text
existing cache
    ↓
差分確認
    ↓
何もなければ即終了
```

となる。

---

# 8. Multiple Process Protection

Claude Code / Codex が同じ workspace で同時に SessionStart する可能性があるため、

```text
Claude
   ↓
cache warm

Codex
   ↓
cache warm
```

となっても二重 warming しない。

`WorkspaceWarmLock` を導入する方針。

設計：

```text
Windows
    → Named Mutex

Linux/macOS
    → OS-level file lock
```

重要なのは lockfile の「存在」を見るのではなく、**OS の lock state** を使うこと。

```text
Process A
    ↓
acquire lock
    ↓
warm


Process B
    ↓
try acquire
    ↓
failed
    ↓
exit 0
```

Crash 時にも stale lock が残って永久に warming できなくなる問題を避ける。

---

# 9. `ast-tool setup`

次に追加する CLI。

```text
ast-tool setup
```

目的：

```text
Claude Code SessionStart
        ↓
ast-tool cache warm --background
```

および：

```text
Codex SessionStart
        ↓
ast-tool cache warm --background
```

を自動設定。

要求事項：

* idempotent
* existing hooks を壊さない
* duplicate hook を作らない
* ast-tool の hook を識別できる
* configuration を atomic に更新
* workspace path を setup 時に固定しない
* background execution
* Claude / Codex 両対応
* 将来的に remove/update 可能な構造

---

# 10. 現在の全体ロードマップ

```text
[Completed / Mostly Completed]

AST IR
  ↓
Semantic Layer
  ↓
Workspace Analysis
  ↓
Semantic Services
  ↓
Agent Evaluation Framework
  ↓
Level 1 / Level 2 tests
  ↓
AST Tool usage trace collection
```

↓

```text
[Current]

Tool-use Trace Analysis
  ↓
Agent がどう ast-tool を使うか調査
  ↓
tool call efficiency
  ↓
AST Tool usage pattern
  ↓
成功率 / latency / token usage との相関
```

↓

```text
[In Progress]

Workspace AST memory bottleneck
  ↓
Path streaming
  ↓
Per-path AST processing
  ↓
必要な AST のみ保持
```

↓

```text
[Next]

Binary AST Cache
  ↓
SQLite
  ↓
LZ4
  ↓
metadata / hash validation
```

↓

```text
[Next]

Parallel Cache Warming
  ↓
existing BlockingQueue
  ↓
parallel workers
  ↓
single SQLite writer
```

↓

```text
[Next]

Background Cache Warming
  ↓
WorkspaceWarmLock
  ↓
SessionStart
```

↓

```text
[Next]

ast-tool setup
  ↓
Claude Code SessionStart
  +
Codex SessionStart
```

---

# 11. 次回の開始地点

次回は **実装より先に tool-use trace を調査する**。

見る対象は特に：

```text
1. task
2. agent success/failure
3. total elapsed time
4. total tokens
5. tool call sequence
6. ast-tool invocation
7. ast-tool command
8. Grep / Glob / Read / Bash との関係
9. ast-tool invocation → subsequent tool calls
10. task level
```

そして最終的には、

```text
                    ┌─ success
                    │
Task ─→ Trace ─→ AST Tool usage
                    │
                    ├─ tool calls
                    ├─ latency
                    └─ tokens
```

という形で、**「AST Tool が Agent の探索行動を本当に改善しているか」**を定量的に見られる状態にするのが次の大きなマイルストーンです。

### 現時点での一番重要な問い

> **AST Tool を使ったこと自体ではなく、AST Tool を使うことで Agent の tool-use trajectory がどう変わったか？**

ここを trace から掘るのが、次の議論の中心です。
