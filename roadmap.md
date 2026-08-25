## 次回議論用：AST Tool Evaluation ロードマップと進捗まとめ

### 現在地

AST Tool の Agent Evaluation は、単なる「ast-tool を使ったかどうか」の確認から一歩進み、

> **ast-tool が Coding Agent の探索行動・成功率・トークン・実行時間にどのような影響を与えるか**

を比較・分析する段階に入っています。

現時点で特に重要なのは、

* `search` は既存の `Grep` をある程度代替している可能性
* `callers` / `callees` / semantic navigation は追加の探索を発生させている可能性
* 成功率の明確な改善はまだ確認できていない
* ast-tool 使用時に **実行時間が大幅に増加している**
* そのため、次は **高速化を優先して再開する**

という状態です。

---

# 1. これまでの進捗

## AST Tool の基本アーキテクチャ

設計方針は概ね固まっています。

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
    ↓
AI Agent / CLI / IDE
```

主要な設計原則：

* Tree-sitter は parsing に限定
* AST IR を安定した中間表現にする
* Semantic Layer は AST IR のみに依存
* Workspace が複数ファイルの semantic information を集約
* Semantic Services は parser-independent
* 上位層は Tree-sitter を直接利用しない
* language-specific な処理は extractor に閉じ込める
* Semantic Provider は optional enrichment
* Agent は parser internals ではなく `ast-tool` を利用する

提供する Semantic Services の中心は：

```text
search
resolution
references
callers
callees
semantic diff
context export
```

---

# 2. Agent Evaluation

Evaluation framework を作成し、複数レベルのテストを実行しています。

```text
smoke
level1
level2
level3
level4
level5
```

各タスクでは repository modification と validation を行い、結果を `results.jsonl` に保存する形です。

Evaluation runner については、

* task ごとの実行
* validation
* tool usage 集計
* ast-tool command usage 集計
* workflow 情報
* token usage
* elapsed time

などを記録する方向で進めています。

また、`results.jsonl` を利用して、

```text
成功済み
    → skip

失敗
    → retry

結果なし
    → run
```

とする resume / retry の改善も検討済みです。

さらに将来的な複数 Agent 比較を考慮し、

```text
(agent, task_id)
```

単位で結果を扱う設計を想定しています。

---

# 3. ast-tool の Command Usage

現時点の使用回数は次の通りです。

```text
command      total

callers       42
search        39
references    17
find          17
callees        9
symbols        2
outline        2
```

Level 別では特に、

```text
callers
    Level 2: 20
    Level 3: 10
    Level 1: 8
    Level 4: 4

search
    Level 2: 16
    Level 3: 14
```

が多く使われています。

---

# 4. ast-tool あり / なし比較の結果

## 成功率

Level 4 では、

```text
with ast-tool     4 validation failures
without ast-tool  4 validation failures
```

となりました。

現時点では、

> **ast-tool による明確な validation success の改善は確認できていない**

という結果です。

ただし、Agent が ast-tool を全く使っていないわけではなく、積極的に利用しています。

したがって問題は、

```text
ast-tool is not used
```

ではなく、

```text
ast-tool is used,
but the additional information is not yet converted
into better task execution.
```

という可能性があります。

---

# 5. `search` は Grep を代替している可能性

これは比較結果からかなり強く示唆されています。

全体：

```text
Grep

with ast-tool      16
without ast-tool   49

delta             -33
```

特に：

```text
Level 3

with ast-tool       0
without ast-tool   10
```

```text
Level 4

with ast-tool       1
without ast-tool   12
```

したがって、

```text
ast-tool search
    ↓
Grep の一部を代替
```

している可能性が高いです。

以前の、

> ast-tool は既存の探索に追加されるだけで置換していない

という仮説は修正する必要があります。

より正確には：

```text
search
    → existing text search を代替する傾向

callers / callees / references
    → additional semantic exploration を増やしている可能性
```

です。

---

# 6. Read / Edit はほぼ変化していない

全体では：

```text
Read

with ast-tool      294
without ast-tool   295
```

```text
Edit

with ast-tool       86
without ast-tool    84
```

ほぼ同じです。

つまり、

```text
探索方法は変化
```

している一方、

```text
最終的なコード確認
コード修正
```

の量は大きく減っていません。

これは自然な結果でもあります。

ast-tool があっても Agent は最終的にファイルを読んで修正する必要があります。

---

# 7. Bash が大幅に増加

ここは非常に重要です。

```text
Bash

with ast-tool      162
without ast-tool    57

delta             +105
```

特に：

```text
Level 1   +19
Level 2   +53
Level 3   +31
Level 4   +10
Level 5    -7
```

です。

Level 2 は特に顕著です。

```text
Bash average per task

with ast-tool      6.75
without ast-tool   0.12
```

これは、

```text
ast-tool
    ↓
semantic result
    ↓
additional verification / investigation
    ↓
Bash
```

のような行動が発生している可能性があります。

ただし現時点では、Bash が何のために増えているかはまだ確定していません。

そのため、詳細な tool-use trace を取る方針になっています。

---

# 8. Glob も増加

```text
Glob

with ast-tool      34
without ast-tool   12

delta             +22
```

特に Level 4 / Level 5 で増えています。

```text
Level 4   +7
Level 5   +9
```

これも、

```text
semantic navigation
    ↓
related file discovery
    ↓
Glob
```

のような追加探索が発生している可能性があります。

---

# 9. Workflow Length

workflow length は全体的に ast-tool 使用時の方が長くなっています。

```text
level     with_ast    without_ast    delta

overall      15.24       12.59       +2.66
smoke        16.00       14.00       +2.00
level1        7.62        4.38       +3.25
level2       12.50        5.88       +6.62
level3        9.38        8.25       +1.12
level4       21.38       19.25       +2.12
level5       25.25       25.00       +0.25
```

特に Level 2：

```text
+6.62 tool / workflow steps per task
```

です。

しかも Level 2 の Bash 増加も：

```text
+6.62 per task
```

となっています。

この一致は非常に興味深く、

> **Level 2 では ast-tool 使用後に追加 Bash investigation がほぼそのまま workflow 増加につながっている**

可能性があります。

これは次の trace analysis で確認する価値があります。

---

# 10. 現在の重要な仮説

現時点では ast-tool の各 command を同じものとして扱わない方がよさそうです。

現在の仮説：

```text
ast-tool
│
├── search
│     └── Grep を代替する傾向
│
├── find / references
│     └── navigation を補助
│
└── callers / callees
      └── semantic exploration を追加
            ↓
          workflow ↑
          Bash ↑
          Glob ↑
          token ↑
          elapsed time ↑
```

特に `callers` は最多使用 command です。

```text
callers: 42
search:  39
```

ここで新しい仮説が出ています。

> **`callers` が Agent の想定と異なる結果を返し、Agent が何度も呼び直したり、別の方法で検証しているのではないか？**

ただし、現在の aggregate data だけでは、

```text
callers(foo)
→ callers(foo)
```

なのか、

```text
callers(foo)
→ callers(bar)
→ callers(baz)
```

なのか区別できません。

そのため詳細 trace が必要です。

---

# 11. Tool Use Trace の次の作業

`run_eval.py` または専用スクリプトに、詳細な tool use logging を追加する予定です。

推奨方針：

```text
results.jsonl
    → compact summary

traces/
    level2-004.jsonl
    level2-008.jsonl
    ...
```

各 tool invocation について：

```text
sequence
tool name
input
output
success / failure
timestamp / duration
```

を保存します。

ast-tool が Bash 経由で実行される場合は、可能なら：

```text
raw command
ast-tool command
arguments
```

も記録します。

例えば：

```text
sequence: 7

tool:
    Bash

input:
    ast-tool callers greet

output:
    ...

ast_tool:
    command: callers
    arguments: ...
```

---

# 12. Trace で調べること

最初に少数のテストを指定して実行します。

例えば：

```text
level2-004
level2-008
```

など。

分析したいパターン：

### 同一 query の繰り返し

```text
callers(foo)
→ callers(foo)
→ callers(foo)
```

### Command transition

```text
callers
→ callers
```

```text
callers
→ Bash
```

```text
callers
→ Read
```

```text
callers
→ references
```

### Empty / unexpected result の再試行

```text
callers(foo)
→ unexpected result

callers(foo)
→ same query
```

### 正常な semantic traversal

```text
callers(foo)
→ callers(bar)
→ callers(baz)
```

これらを区別することが目的です。

---

# 13. 次回の最優先事項：高速化

## ここから再開する

Tool-use trace の仕組みは並行して進めますが、次回の主題は **高速化**です。

理由：

> **ast-tool を使うと実行時間が倍近くになっている。**

したがって、次のフェーズではまず、

```text
Why is ast-tool slow?
```

を調べる必要があります。

分析の優先順位は次の通り。

---

## Phase 1 — Performance Baseline

まず各 ast-tool command の性能を測定する。

対象：

```text
search
find
references
callers
callees
symbols
outline
```

各 command について：

```text
wall-clock time
number of files
repository size
number of results
```

を取得する。

最初に知りたいのは：

```text
Agent execution time increase
```

が、

```text
A. ast-tool command 自体が遅い
```

のか、

```text
B. ast-tool は速いが、Agent が追加探索するため全体が遅い
```

のかです。

これは最優先で切り分ける。

---

# 14. Phase 2 — Command-Level Profiling

特に最多使用の：

```text
callers
search
references
```

を重点的に調査する。

例えば `callers` について：

```text
callers
    ↓
symbol lookup
    ↓
workspace / file traversal
    ↓
reference search
    ↓
semantic resolution
    ↓
result formatting
```

のどこに時間が使われているかを測定する。

可能なら内部 timing を追加する。

例：

```text
total:                820 ms

symbol lookup:         20 ms
workspace loading:    400 ms
reference collection: 300 ms
resolution:            50 ms
formatting:            10 ms
```

---

# 15. 高速化の優先仮説

次回は以下を順番に確認する。

## Hypothesis A — Workspace / Index の再構築

最も疑わしい候補。

もし command ごとに：

```text
workspace load
parse
semantic extraction
```

を繰り返しているなら、

```text
callers
search
references
```

を連続して実行すると非常に遅くなります。

改善候補：

```text
long-lived workspace
persistent index
in-memory cache
incremental analysis
```

---

## Hypothesis B — 同じ symbol / file の再解析

例えば：

```text
callers(foo)
references(foo)
callees(foo)
```

で同じ symbol resolution や file parsing を繰り返している可能性。

改善候補：

```text
symbol cache
resolution cache
reference cache
parsed AST cache
```

---

## Hypothesis C — `callers` / `callees` の実装コスト

`callers` が最多利用されているため、個別最適化の価値が高い可能性があります。

特に：

```text
callers
```

が毎回 workspace 全体を scan しているなら優先的に改善する。

候補：

```text
reverse reference index
symbol → references index
caller cache
workspace-level dependency graph
```

---

## Hypothesis D — CLI startup overhead

Agent が毎回：

```bash
ast-tool callers ...
```

のように CLI process を起動している場合、

```text
process startup
workspace initialization
configuration loading
index loading
```

が毎回発生している可能性があります。

改善候補：

```text
persistent daemon
server mode
JSON-RPC
long-lived process
```

ただし、まず profiling で startup cost が支配的か確認する。

---

# 16. 次回の推奨作業順序

### Step 1

代表的な repository と query を使い、

```text
search
references
callers
callees
```

の単体実行時間を測定。

### Step 2

同じ process / workspace 内で連続実行した場合と、

```text
ast-tool callers ...
ast-tool references ...
ast-tool search ...
```

のような個別 CLI invocation を比較。

### Step 3

内部 profiling を追加し、時間の内訳を取得。

### Step 4

最大の bottleneck を一つ選ぶ。

### Step 5

その bottleneck だけを最初に最適化。

### Step 6

再度 Agent Evaluation を実行。

比較：

```text
success rate
elapsed time
tokens
workflow length
Grep usage
Bash usage
ast-tool usage
```

---

# 17. 高速化後に再評価するポイント

高速化後も単に、

```text
ast-tool command time
```

だけを見るのではなく、

```text
End-to-end evaluation time
```

を見る。

最終的に重要なのは：

```text
ast-tool is fast
```

ではなく、

```text
Agent completes the task faster
```

です。

成功パターンの理想形は：

```text
Without ast-tool

Grep
→ Read
→ Grep
→ Read
→ Edit
```

が、

```text
With optimized ast-tool

search
→ callers / references
→ Read
→ Edit
```

となり、

```text
Grep ↓
Bash ↓
Workflow ↓
Elapsed time ↓
Token usage ↓ or neutral
Success ↑ or unchanged
```

に近づくことです。

---

# 18. 次回議論のスタート地点

次回はこの状態から開始する。

## Current priority

```text
Tool-use analysis
    → continue in parallel

Performance
    → highest priority
```

最初の問いは：

> **ast-tool 使用時に実行時間が倍近くになる原因は、ast-tool 自体の処理速度なのか、それとも Agent の追加 exploration behavior なのか？**

この切り分けから始める。

その後：

```text
1. Baseline benchmark

2. Command-level profiling

3. Identify bottleneck

4. Optimize

5. Re-run selected evaluations

6. Compare end-to-end results

7. Use detailed tool traces to understand
   whether callers / callees cause repeated or unnecessary exploration
```

という順序で進める。

---

## 一言でいうと現在地

```text
Phase 1
Architecture
    ✓

Phase 2
Evaluation framework
    ✓

Phase 3
Task generation / Level evaluation
    ✓

Phase 4
With vs Without ast-tool comparison
    ✓

Phase 5
Behavior analysis
    → in progress
    → detailed tool traces next

Phase 6
Performance optimization
    → NEXT / highest priority

Phase 7
Re-evaluation after optimization
    → pending
```

**次回は「なぜ遅いのか」を定量的に分解して、最大のボトルネックから高速化するところから再開。**
その間、詳細な tool-use trace を使って、特に `callers` が「有益な semantic traversal」なのか「期待と違う結果による再試行・検証ループ」なのかを確認する、という進め方がよさそうです。
