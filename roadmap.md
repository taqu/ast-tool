# AST Tool — Roadmap / Progress Summary

## 目的

最適化対象は AST Tool 単体ではなく、Coding Agent 全体の効率。

```text
Agent efficiency
├─ success rate ↑
├─ total tokens ↓
├─ latency ↓
├─ recovery cost ↓
└─ unnecessary exploration ↓
```

主要指標の優先順位：

```text
1. Success rate
2. Total tokens
3. Recovery / unnecessary exploration
4. Latency
5. Total tool calls
6. AST Tool local metrics
```

AST Tool call 数や failure rate 単独では評価しない。

---

# 現在のロードマップ

```text
P0  Baseline / Trace Metrics
 ✓ Completed

P1  Skill.md Decision Tree
 ✓ Completed

P2  Output / JSON UX
 ✓ Completed

P3  Semantic Resolver
 ✗ DEFERRED

P4  Stable Semantic Symbol ID
 ✗ DEFERRED

P5  Error Recovery UX
 ✓ ACCEPTED
 │
 └─ Current Stable Baseline

P6  Agent-facing Command Surface
 ✗ REJECTED

P7  Skill.md Compression
 → NEXT

P8  Final Quantitative Evaluation

P9  Optional Semantic Research
```

---

# Phase 0 — Baseline / Trace Metrics

## 目的

変更前後を同じ指標で比較可能にする。

収集対象：

```text
total tool calls
ast-tool calls
ast-tool failures
ast-tool retries
help calls

grep
glob
read
bash
edit

elapsed time

input tokens
output tokens
total tokens

AST Tool command sequence
recovery distance
failures by command
```

## 状態

**Completed**

同じ 41 evaluation tests を基本比較対象として使用。

---

# Phase 1 — Skill.md Decision Tree

## 目的

Agent の command selection を一本道にし、

```text
help
→ trial & error
→ repeated commands
```

を減らす。

基本 routing：

```text
Find symbol       → search
Find callers      → callers
Find references   → references
Find callees      → callees
Find file symbols → symbols
Need AST structure → find
```

追加ルール：

```text
Do not retry unchanged failed commands.
Do not use --pretty by default.
Do not dump entire workspace.
Do not use --help for ordinary discovery.
```

## 結果

```text
tests                 41
success rate          90.24%
total tool calls      553
AST Tool calls         67
AST failures           33
AST retries            22
avg recovery distance 1.90
total tokens      176,983
elapsed            2290.81 sec
```

## 状態

**Accepted**

---

# Phase 2 — Output / JSON UX

## 目的

特に、

```text
--json --pretty
workspace-wide output
```

による token waste を削減。

方針：

```text
compact JSON by default
pretty only when explicitly requested
avoid unnecessary fields
avoid huge default output
preserve compatibility
```

## 結果

```text
tests                 41
success rate          90.24%

total tool calls      519
AST Tool calls         70
AST failures           36
AST failure rate      51.43%
AST retries            23

grep                   18
read                  254

avg recovery distance 2.23
max recovery distance 5

total tokens      162,628
avg tokens/test     3,966.5
elapsed            2100.56 sec
```

Phase 1 比：

```text
success rate     = maintained
total tool calls ↓
tokens           ↓ ~8%
elapsed          ↓ ~8%
grep             ↓
read             ↓
```

## 状態

**Accepted**

---

# Phase 3 — Semantic Symbol Resolution

## 状態

**DEFERRED**

Semantic Resolver 強化により AST Tool usage が大幅に低下し、grep fallback・token 使用量・task failure が増加。

```text
success rate   90.24% → 60.98%
AST calls      70 → 15
grep           18 → 71
tokens         162,628 → 253,785
```

---

# Phase 4 — Stable Semantic Symbol ID

## 状態

**DEFERRED**

Reliable semantic identity が前提のため、Phase 3 とともに延期。

---

# Phase 5 — Error Recovery UX

## 目的

Semantic query を必ず成功させるのではなく、

```text
semantic query
    ↓
failure
    ↓
actionable error
    ↓
one useful next action
```

という安価で predictable な recovery path を作る。

Error message は、

```text
1. what failed
2. what is already known
3. cheapest reasonable next action
```

を compact に返す。

Semantic Resolver 自体は変更しない。

## 結果

```text
tests                      41
successes                  37
success rate              90.24%

total tool calls           518
avg tool calls/test       12.63

AST Tool calls              69
AST failures                 9
AST failure rate          13.04%
AST retries                  9

avg recovery distance      1.44
max recovery distance         2

grep                         15
read                        252
glob                         12

total tokens            158,303
avg tokens/test           3,861

elapsed                 2224.27 sec
```

Phase 2 比：

```text
success rate
90.24% → 90.24%        maintained

AST calls
70 → 69                stable

AST failures
36 → 9                 -75%

AST retries
23 → 9                 -61%

avg recovery distance
2.23 → 1.44

max recovery distance
5 → 2

grep
18 → 15

total tokens
162,628 → 158,303      -2.7%

elapsed
2100.56 → 2224.27      +5.9%
```

Semantic commands も大きく改善。

```text
Phase 2:
callers     21 / 21 failures
callees      9 / 9
references   6 / 5

Phase 5:
callers     14 / 3 failures
callees      3 / 1
references   6 / 1
```

## 状態

**ACCEPTED**

現在の **Stable Baseline**。

---

# Current Stable Baseline — Phase 5

今後の比較基準：

```text
tests                      41
success rate             90.24%

total tool calls           518
avg tool calls/test       12.63

AST Tool calls              69
AST failures                 9
AST failure rate          13.04%
AST retries                  9

avg recovery distance      1.44
max recovery distance         2

grep                         15
read                        252
glob                         12

total tokens            158,303
avg tokens/test           3,861

elapsed                 2224.27 sec
```

Phase 5 が現在の正式 baseline。

---

# Phase 6 — Agent-facing Command Surface

## 目的

既存 CLI を壊さず、

```text
small obvious agent-facing surface
+
full backwards-compatible CLI
```

を作る。

変更対象：

```text
help grouping
ordering
category headings
discoverability
```

非対象：

```text
command deletion
command rename
syntax change
semantic resolution change
Symbol ID
Skill.md change
JSON redesign
Phase 5 error UX change
```

想定 command classification：

```text
Primary:
  search
  callers
  references
  callees
  find
  symbols

Secondary:
  outline

Debug / Low-level:
  parent
  children
  range

Infrastructure:
  cache
  setup
```

## Clean re-evaluation

clang++ を更新して環境由来の validation failure を除去した結果：

```text
tests                      41
successes                  37
failures                    4
success rate              90.24%

total tool calls           411
avg tool calls/test       10.02

AST Tool calls              22
AST failures                 5
AST failure rate          22.73%
AST retries                  8
AST help calls               5

grep                         64
read                        181
edit                         81
bash                         73

avg recovery distance      2.60
max recovery distance         5

total tokens            241,130
avg tokens/test           5,881.2

elapsed                 1104.85 sec
```

Phase 5 比：

```text
success rate
90.24% → 90.24%          maintained

AST Tool calls
69 → 22                  -68%

grep
15 → 64                  +327%

avg recovery distance
1.44 → 2.60              worse

max recovery distance
2 → 5                    worse

total tokens
158,303 → 241,130        +52%

avg tokens/test
3,861 → 5,881            +52%
```

Per-command usage：

```text
Phase 5       Phase 6

search       30 → 5
callers      14 → 4
references    6 → 7
find         12 → 4
symbols       1 → 1
```

特に `search` / `callers` / `find` が大幅に減少。

Agent は correctness を維持しているものの、

```text
AST semantic path ↓
grep/manual exploration ↑
context/token cost ↑
recovery cost ↑
```

となっている。

以前疑った Skill.md と CLI help の直接的な矛盾については、解析上は強い証拠なし。

Phase 6 の help はほとんどの trajectory で参照されておらず、Skill.md と Primary command の対応にも明示的な矛盾は確認されなかった。

したがって Phase 6 の問題は、

```text
helpを読んで混乱した
```

と断定するより、

```text
Agent が AST Tool を
default semantic/context-reduction path として
選択しなくなった
```

という agent-level behavior regression と捉える。

## 状態

**REJECTED**

Correctness は baseline を維持したが、AST usage collapse、grep fallback、token +52%、recovery 悪化のため不採用。

Phase 5 に rollback。

---

# Phase 7 — Skill.md Compression

## 状態

**NEXT**

Phase 5 に rollback した状態から開始する。

目的：

```text
Skill tokens ↓
Agent decision complexity ↓
```

ただし Phase 6 の結果を踏まえ、routing behavior を変えないことを最優先とする。

基本 routing は固定：

```text
Find symbol       → search
Find callers      → callers
Find references   → references
Find callees      → callees
Find file symbols → symbols
Need AST structure → find
```

Compression では、

```text
routing semantics
command recommendation
fallback policy
```

を変更しない。

削減候補は、

```text
redundant explanation
obsolete workaround
CLI / error UX 側ですでに担保された説明
repeated examples
```

のみ。

## 推奨する進め方

一度に大きく圧縮せず、

```text
Phase 7a
Conservative Skill.md Compression
    ↓
evaluation
    ↓
Phase 7b
Further Compression
```

と段階化する。

Phase 7a の目標は大幅な token reduction ではなく、

```text
Phase 5 behavior preserved
+
Skill.md smaller
```

とする。

Acceptance criteria の中心：

```text
success rate ≈ 90.24% or higher
AST usage ≈ Phase 5
grep fallback does not spike
recovery distance ≈ Phase 5
total tokens <= Phase 5, ideally
```

特に、

```text
search usage
30 → collapse
```

のような routing regression が起きないことを確認する。

---

# Phase 8 — Final Quantitative Evaluation

Phase 7 が安定した後に実施。

正式比較：

```text
Phase 1
  ↓
Phase 2
  ↓
Phase 5
  ↓
Phase 7
```

Phase 3 / Phase 6 は experimental failure として別扱い。

見るべき主要指標：

```text
success rate
total tokens
AST Tool usage
grep/read fallback
AST failures
AST retries
recovery distance
elapsed time
total tool calls
per-command usage
help calls
```

最終的には、

```text
baseline → final
```

で Agent-level efficiency がどこまで改善したかを評価する。

---

# Phase 9 — Optional Semantic Research

Error UX / Skill / command-routing 改善が plateau した場合のみ再検討。

候補：

```text
exact qualified lookup
resolver ranking
specific narrow C++ cases
same-file pairing
```

General declaration/definition unification のような広い semantic resolver 改修には戻らない。

---

# 現時点での重要な設計上の学び

## 1. Semantic sophistication ≠ Agent quality

AST Tool 自体の semantic metric が改善しても、

```text
AST usage ↓
grep fallback ↑
tokens ↑
success ↓
```

なら Agent-level では regression。

---

## 2. Failure をゼロにする必要はない

Phase 5 から、

```text
moderately capable semantic tool
+
good actionable recovery
```

でも高い task success rate を維持できることが確認できた。

---

## 3. Coding Agent には一本道が重要

理想：

```text
search
  ↓
semantic query
  ↓
success
```

または、

```text
semantic query
  ↓
actionable failure
  ↓
one cheap recovery
```

複雑な semantic machinery より predictable trajectory を優先する。

---

## 4. AST Tool は context compression mechanism としても重要

Phase 6 では tool call 数自体は減ったにもかかわらず、

```text
total tool calls
518 → 411

total tokens
158k → 241k
```

となった。

Semantic query による targeted result を使わず、

```text
grep
→ read
→ manual reasoning
```

へ寄ると、一回あたりの探索コストが大きくなる可能性が高い。

したがって AST Tool の価値は単なる navigation speed ではなく、

```text
relevant context を小さく取り出す
```

ことにもある。

---

## 5. Correctness と Efficiency は分けて評価する

Phase 6 clean run：

```text
Correctness
  success rate = baseline

Efficiency
  AST usage ↓
  grep ↑
  tokens +52%
  recovery worse
```

Task が成功しただけでは optimization success としない。

---

# Evaluation 周りの補足

以前 trace に出ていた、

```text
"2": 1
```

は trace parser が、

```text
which ast-tool 2>/dev/null
```

の `2` を subcommand と誤認したものと確認済み。

実際の AST Tool command ではなく、Phase 6 regression の原因ではない。

今後 parser 側で別途修正可能。

---

# 次回の議論の開始地点

次回は、

```text
Phase 5 に rollback 済み
        ↓
Phase 7a — Conservative Skill.md Compression
```

から開始する。

最初に決めるべきこと：

```text
1. Phase 5 Skill.md のどの記述を必須 routing contract として固定するか

2. どの説明・workaround・example を安全に削除できるか

3. Phase 7a の target size / compression 方針

4. Phase 7a acceptance criteria
```

Phase 7 の原則：

```text
Do not optimize routing yet.

Preserve Phase 5 routing behavior.
Reduce instruction volume only.
```

Phase 7a が Phase 5 performance を維持できた場合のみ、追加圧縮または Phase 8 に進む。
