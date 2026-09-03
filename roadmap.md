## AST Tool — Roadmap / Progress Summary

### 目的

最適化対象は AST Tool 単体の sophistication ではなく、Coding Agent 全体の効率。

```text
Agent efficiency
├─ success rate ↑
├─ total tokens ↓
├─ latency ↓
├─ recovery cost ↓
└─ unnecessary exploration ↓
```

AST Tool call 数や failure rate は補助指標であり、それ自体を目的関数にはしない。

特に今回の評価から、

```text
AST Tool failure ↓
```

でも Agent が AST Tool を使わなくなって success rate が落ちれば失敗、と確認できた。

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
 └─ Current stable baseline

P6  Agent-facing Command Surface
 → CURRENT / NEXT EVALUATION

P7  Skill.md Compression

P8  Final Quantitative Evaluation

P9  Optional Semantic Research
```

---

# Phase 0 — Baseline / Trace Metrics

### 目的

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

### 状態

**Completed**

以降すべて同じ 41 evaluation tests を基本比較対象としている。

---

# Phase 1 — Skill.md Decision Tree

### 目的

Agent の、

```text
help
→ trial & error
→ repeated commands
```

を減らす。

Skill を説明文中心から decision tree 中心に変更。

基本方針：

```text
Find symbol       → search
Find callers      → callers
Find references   → references
Find callees      → callees
Find file symbols → symbols
Need AST structure → find
```

加えて、

```text
Do not retry unchanged failed commands.
Do not use --pretty by default.
Do not dump entire workspace.
Do not use --help for ordinary discovery.
```

などを明示。

### 結果

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

### 状態

**Accepted**

---

# Phase 2 — Output / JSON UX

### 目的

特に、

```text
--json --pretty
workspace-wide output
```

による token waste を減らす。

主な考え方：

```text
compact JSON by default
pretty only when explicitly requested
avoid unnecessary fields
avoid huge default output
preserve compatibility
```

### 結果

```text
tests                 41
success rate          90.24%

total tool calls      519
AST Tool calls         70
AST failures           36
failure rate         51.43%
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
total tool calls ↓
tokens           ↓ 約8%
elapsed          ↓ 約8%
grep             ↓
read             ↓
success rate     = 維持
```

### 状態

**Accepted**

当時の stable baseline。

---

# Phase 3 — Semantic Symbol Resolution

### 元の目的

C++ の、

```text
header declaration
cpp definition
```

を同一 logical semantic symbol として扱い、

```text
callers auth::AuthToken::validate .
```

などの false ambiguity を解消する。

### 結果

大幅 regression。

```text
tests                 41
successes             25
success rate          60.98%

total tool calls      452

AST Tool calls         15
AST failures            2
failure rate         13.33%

grep                   71

total tokens      253,785
avg tokens/test     6,189.9

elapsed            1347.23 sec
```

Phase 2 → Phase 3：

```text
success rate:
90.24% → 60.98%      ❌

total tokens:
162,628 → 253,785    ❌ +56%

AST Tool calls:
70 → 15              ⚠ usage collapse

grep:
18 → 71              ❌
```

表面上、

```text
AST failure rate
51.43% → 13.33%
```

まで下がったが、これは semantic tool が改善した証拠ではなかった。

Agent が AST Tool をほとんど使わなくなり、

```text
AST Tool ↓
grep fallback ↑
tokens ↑
success ↓
```

となった。

過去にも Semantic Resolver 改善を試みて不安定化した経緯あり。

### 判断

**DEFERRED**

現在の optimization track では declaration/definition unification を追わない。

重要な知見：

> Lightweight Tree-sitter semantic tool に C++ の完全な logical symbol identity を持たせるコストは、Agent-level benefit に対して高すぎる可能性がある。

---

# Phase 4 — Stable Semantic Symbol ID

元の案：

```text
search
  ↓
semantic symbol ID
  ↓
callers --id
references --id
callees --id
```

しかし stable Symbol ID は、

```text
reliable semantic identity
```

が前提。

Phase 3 が deferred になったため Phase 4 も延期。

### 状態

**DEFERRED**

現時点では以下を追加しない。

```text
callers --id
references --id
callees --id
```

---

# Phase 5 — Error Recovery UX

### 方針転換

Phase 3 の失敗を受けて、

```text
semantic query を必ず成功させる
```

ではなく、

```text
semantic query が失敗しても
Agent が安く正しい recovery をできるようにする
```

へ変更。

目標 trajectory：

```text
semantic command
    ↓
failure
    ↓
actionable error
    ↓
one useful next action
```

悪い trajectory：

```text
failure
→ help
→ find
→ search
→ grep
→ read
→ retry
→ ...
```

### 改善対象

```text
ambiguous symbol
symbol not found
no result
invalid query
unknown option
invalid arguments
unsupported query
```

Error message は、

```text
1. what failed
2. what is already known
3. cheapest reasonable next action
```

を compact に返す。

Semantic Resolver 自体は変更しない。

### 結果

```text
tests                 41
successes             37
success rate          90.24%

total tool calls      518
AST Tool calls         69

AST failures            9
failure rate         13.04%

AST retries             9

avg recovery distance 1.44
max recovery distance 2

grep                   15
read                  252
glob                   12

total tokens      158,303
avg tokens/test     3,861.0

elapsed            2224.27 sec
```

Phase 2 → Phase 5：

```text
success rate
90.24% → 90.24%        ✅

AST calls
70 → 69                ✅ stable

AST failures
36 → 9                 ✅ -75%

AST retries
23 → 9                 ✅ -61%

avg recovery distance
2.23 → 1.44            ✅

max recovery distance
5 → 2                  ✅

grep
18 → 15                ✅

total tokens
162,628 → 158,303      ✅ -2.7%

elapsed
2100.56 → 2224.27      ⚠ +5.9%
```

Phase 3 と違い、

```text
AST Tool usage ≈ stable
```

なので failure rate 改善は usage collapse によるものではない。

特に semantic commands：

```text
Phase 2:
callers     21 calls / 21 failures
callees      9 / 9 failures
references   6 / 5 failures

Phase 5:
callers     14 / 3 failures
callees      3 / 1 failure
references   6 / 1 failure
```

trajectory の揺らぎがあるので直接の同一母集団比較ではないが、全体 recovery metrics と合わせて見ると明確な改善。

### 状態

**ACCEPTED**

現在の **stable baseline**。

---

# 現在の Stable Baseline

今後の比較基準は Phase 2 ではなく **Phase 5**。

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

---

# Phase 6 — Agent-facing Command Surface

### 状態

**Current / implementation instructions prepared**

目的は command を削除することではない。

```text
small obvious agent-facing surface
+
full backwards-compatible CLI
```

を作る。

### 現在の分類案

#### Primary Agent Commands

```text
search
callers
references
callees
find
symbols
```

#### Secondary

```text
outline
```

#### Debug / Low-level

```text
parent
children
range
```

#### Infrastructure

```text
cache
setup
```

### `find` について

元々 Support 扱いを検討していたが、Phase 5 では、

```text
find = 12 calls
```

とかなり使われている。

そのため architectural purity より実測を優先し、Primary 側に残す。

### Phase 6 の重要制約

やらないこと：

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

主に、

```text
help grouping
ordering
category headings
discoverability
```

だけを改善する。

Phase 5 の trajectory を壊さないことが最優先。

---

# Phase 6 Acceptance Criteria

Phase 5 と比較して、

```text
success rate ≈ 90.24% or higher

AST usage does not collapse

AST failures ≈ stable or better
AST retries ≈ stable or better
recovery distance ≈ stable or better

tokens ≈ stable or lower

grep/read fallback does not spike
```

であれば採用候補。

Phase 6 は大きな token 改善がなくてもよい。

Command surface が整理され、

```text
Phase 5 performance preserved
```

なら成功とみなす。

---

# Phase 7 — Skill.md Compression

Phase 6 の command surface が安定した後に実施。

最終的な Skill はかなり短くしたい。

目標イメージ：

```text
Find a symbol        → search
Find callers         → callers
Find references      → references
Find callees         → callees
Inspect structure    → find
Inspect file symbols → symbols

If a semantic query fails,
follow the recovery guidance.

Use compact output.
Do not retry unchanged failed commands.
Do not use --help unless necessary.
```

目的：

```text
Skill tokens ↓
Agent decision complexity ↓
```

Phase 1 で Skill を改善した後、CLI/Error UX 側が進化したため、Skill に workaround を大量に持たせる必要が減っている。

---

# Phase 8 — Final Quantitative Evaluation

比較：

```text
Phase 1
  ↓
Phase 2
  ↓
Phase 5
  ↓
Phase 6
  ↓
Phase 7
```

特に Phase 3 は experimental failure として別扱い。

主要指標の優先順位：

```text
1. Success rate
2. Total tokens
3. Recovery / unnecessary exploration
4. Latency
5. Total tool calls
6. AST Tool local metrics
```

AST Tool failure rate 単独では評価しない。

---

# Phase 9 — Optional Semantic Research Track

Semantic Resolver は完全に捨てたわけではない。

ただし main optimization track から分離する。

再検討条件：

```text
1. Error UX / command surface / Skill improvements が plateau
2. Semantic failures が依然 task failure の主要因
3. narrow かつ低リスクな改善案が見つかる
```

再開するとしても、

```text
general declaration/definition unification
```

をいきなり再実装しない。

例えば：

```text
exact qualified lookup
resolver ranking
specific narrow C++ cases
same-file pairing
```

のように局所実験する。

---

# 現時点での設計上の重要な学び

### 1. Semantic sophistication ≠ Agent quality

Phase 3 で確認。

```text
local metric improvement
≠
end-to-end improvement
```

---

### 2. Failure をゼロにする必要はない

Phase 5 が重要な証拠。

```text
moderately capable semantic tool
+
good actionable recovery
```

でも高い task success rate を維持できる。

---

### 3. Coding Agent には一本道が重要

理想は、

```text
search
  ↓
semantic query
  ↓
success

or

semantic query
  ↓
actionable failure
  ↓
one cheap recovery
```

複雑な semantic machinery より predictable trajectory を優先する。

---

### 4. Command classification は実測を優先

例：

```text
find
```

は設計上は lower-level に見えるが、Phase 5 で頻繁に利用されている。

したがって、

```text
architecture says support command
```

より、

```text
evaluation says useful command
```

を優先する。

---

### 5. Stable baseline を常に明示する

現在：

```text
Phase 5 = stable baseline
```

Phase 3 の、

```text
13.33% AST failure rate
```

は正式 baseline として使わない。

AST usage collapse を伴うため比較基準として misleading。

---

# Evaluation 周りの未解決事項

trace に継続して、

```text
"2": 1
```

という command classification が現れている。

Phase 2、Phase 3、Phase 5 などで観測。

候補：

```text
trace parser bug
stderr redirect "2>"
positional argument misclassification
malformed invocation
```

Phase 6 本体には混ぜず、評価ツール側の小さい investigation として切り離すのがよい。

---

# 次の議論の開始地点

次回は基本的に、

```text
Phase 6 の実装結果
```

から再開する。

見るべき比較は：

```text
Phase 5 → Phase 6
```

特に：

```text
success rate
total tokens
AST Tool usage
AST Tool failures
AST retries
recovery distance
grep/read fallback
per-command usage
help calls
```

Phase 6 が安定していれば、

```text
Phase 7 — Skill.md Compression
```

へ進む。

Phase 6 で regression が出た場合は command surface をさらに削る方向ではなく、**Phase 5 の discoverability に戻して最小限の grouping だけ残す**のが基本方針です。
