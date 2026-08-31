## 推奨 Phase 構成

```text
Phase 0
Baseline / Trace Metrics
        ↓
Phase 1
Skill.md 改善
        ↓
Phase 2
CLI Output / JSON UX 改善
        ↓
Phase 3
Symbol Resolution 改善
        ↓
Phase 4
Symbol ID-based API
        ↓
Phase 5
Error Recovery UX
        ↓
Phase 6
Agent-facing Command Surface の整理
        ↓
Phase 7
Trace Analysis / Effect Measurement
```

---

# Phase 0 — Baseline を固定

### 目的

実装前の状態を測定して、改善効果を比較できるようにする。

### やること

既存 trace から最低限、

```text
total tool calls
ast-tool calls
ast-tool failures
ast-tool retries
help calls
grep calls
glob calls
read calls
bash calls

elapsed time
input tokens
output tokens
total tokens
```

を集計できるようにする。

さらに、

```text
ast-tool command sequence
```

を保存。

例えば：

```text
callers
search
callers
find
find
search
references
find
help
...
```

### 完了条件

同じ evaluation test を複数回実行して、

```text
baseline.json
```

のような形で比較可能。

### Coding Agent への指示

> Do not modify ast-tool behavior in this phase.
> Add or improve trace analysis only. Establish baseline metrics for tool calls, ast-tool failures, retries, help usage, token usage, and elapsed time.

---

# Phase 1 — Skill.md を Decision Tree 化

これは比較的安全なので最初の実装対象にします。

### 目的

Agent が `help → trial & error` に入るのを減らす。

### Skill.md の構成

今までの説明中心ではなく、

```text
## Semantic Task Decision Tree

Need to find a symbol?
→ search

Need callers?
→ callers

Need references?
→ references

Need callees?
→ callees

Need symbols in a specific file?
→ symbols

Need AST structure?
→ find

Need command syntax clarification?
→ help
```

という形式。

さらに、

```text
Do not:
- retry the same failed command without changing the input
- use --pretty by default
- dump the entire workspace
- use find for semantic resolution
- call --help for normal command discovery
```

を明記。

### 完了条件

Skill の変更だけで evaluation を再実行。

比較：

```text
help calls ↓
repeated command ↓
ast-tool calls ↑/↓
total calls ↓
tokens ↓
```

### 重要

**この Phase では CLI や semantic implementation を変更しない。**

Skill の効果だけを測る。

---

# Phase 2 — Output / JSON UX

次は Agent が大量の output を要求する問題を潰します。

### 目的

```text
--json --pretty
```

による token waste を減らす。

### 方針

デフォルト JSON は compact。

```bash
ast-tool search --json ...
```

↓

```json
{"id":"819318E9","kind":"function","name":"validate","fqn":"auth::AuthToken::validate","file":"src/auth/auth_token.cpp","line":7}
```

`--pretty` は明示指定時のみ pretty print。

### さらに

Agent が semantic task で必要とする情報を優先。

例えば不要な、

```text
access
static
constexpr
inline
```

などを常時出さない。

ただし、**既存 CLI/API の互換性を壊さない**ようにする。

### 完了条件

同一 task について、

```text
plain
json
json --pretty
```

の output token 数を比較。

特に、

```text
search --json --pretty .
```

のような workspace-wide query に上限・compact化が効いているか確認。

---

# Phase 3 — Semantic Symbol Resolution 改善

ここが**最重要の実装 Phase**です。

### 問題

```text
auth::AuthToken::validate
```

について、

```text
header declaration
cpp definition
```

が別 symbol として ambiguity を起こしている。

### 目的

semantic layer で、

```text
declaration
definition
      ↓
same logical symbol
```

として扱えるようにする。

### 実装対象

まず C++ に限定してよいです。

```text
Function declaration
Function definition

Method declaration
Method definition
```

の identity / resolution を確認。

### 重要な設計

ここでは CLI をいじる前に、

```text
AST IR
 ↓
Semantic Layer
 ↓
Symbol identity
 ↓
Resolver
```

を正しくする。

### 完了条件

例えば、

```bash
ast-tool callers auth::AuthToken::validate .
```

が ambiguity にならず、正しい caller を返す。

同様に、

```bash
ast-tool references auth::AuthToken::validate .
```

も検証。

### Regression tests

最低限：

```text
declaration + definition
overload
namespace
class method
multiple translation units
```

を入れる。

---

# Phase 4 — Stable Symbol ID API

Phase 3 と非常に相性がいいですが、**別 Phase にした方が Agent 実装は安全**です。

### 目的

名前による resolution が失敗しても、

```text
search
 ↓
symbol ID
 ↓
callers --id
```

という deterministic な workflow を可能にする。

### CLI

例えば：

```bash
ast-tool callers --id 819318E9 .
ast-tool references --id 819318E9 .
ast-tool callees --id 819318E9 .
```

を検討。

### ポイント

ID は、

```text
AST node ID
```

ではなく、

```text
semantic symbol ID
```

として扱う。

つまり declaration / definition が同じ symbol なら同じ ID。

### 理想的 trajectory

```text
search validate
        ↓
id=819318E9
        ↓
callers --id 819318E9
```

### 完了条件

Agent が名前解決を何度も試さずに semantic query を完了できる。

---

# Phase 5 — Actionable Error Messages

Phase 3/4 の機能を使って、Agent の recovery cost を下げます。

### 現状

```text
error: symbol is ambiguous
```

だけでは Agent が次に何をすべきか分からず、

```text
find
search
help
symbols
```

へ迷走する。

### 改善

例えば：

```text
error: ambiguous symbol 'auth::AuthToken::validate'

Candidates:

1. function
   src/auth/auth_token.cpp:7
   id: 819318E9

2. method
   src/auth/auth_token.h:12
   id: 819318E9

The declaration and definition resolve to the same semantic symbol.

Try:

  ast-tool callers --id 819318E9 .
```

のようにする。

### さらに

unknown option：

```text
unknown option '--foo'

Available options:
  --json
  --pretty
  --id

Example:
  ast-tool callers --id SYMBOL_ID PATH
```

など。

### 完了条件

失敗 → 次の tool call までの距離を測定。

目標：

```text
error
 ↓
correct retry
```

に近づける。

---

# Phase 6 — Agent-facing Command Surface の整理

ここで初めて subcommand の剪定を行います。

いきなり command を削除するのではなく、

```text
Core
Support
Debug/Internal
Infrastructure
```

に分類。

## Core

```text
search
callers
references
callees
symbols
```

## Support

```text
find
outline
```

## Debug / Low-level

```text
parent
children
range
```

## Infrastructure

```text
cache
setup
```

### 重要

**削除ではなく discoverability を下げる**方を第一候補にします。

なぜなら、

```text
parent
children
range
```

は Agent には不要でも、人間の debugging には便利だからです。

CLI compatibility を壊さない方がいい。

---

# Phase 7 — Trace Analysis / Quantitative Evaluation

最後に、ここまでの変更を全部まとめて測定します。

比較対象：

```text
Baseline
    ↓
Skill improved
    ↓
Output improved
    ↓
Resolver improved
    ↓
Symbol ID
    ↓
Error UX
```

各段階で、

| Metric             | 見るもの                      |
| ------------------ | ------------------------- |
| Total tool calls   | 探索全体の長さ                   |
| AST Tool calls     | 利用量                       |
| AST Tool failures  | API usability             |
| AST Tool retries   | recovery cost             |
| Help calls         | Skill/API discoverability |
| Grep calls         | semantic tool の代替利用       |
| Read calls         | 探索コスト                     |
| JSON output tokens | output efficiency         |
| Input tokens       | context cost              |
| Output tokens      | generation cost           |
| Total tokens       | 最重要                       |
| Elapsed time       | latency                   |
| Success rate       | correctness               |

を見る。

---

# 特に重要な「成功」の定義

今回のプロジェクトでは、

```text
AST Tool calls が増えた
```

だけでは成功とは言えません。

例えば、

```text
Before

26 tool calls
210k tokens
200 sec
```

↓

```text
After

30 tool calls
180k tokens
190 sec
```

なら、**AST Tool の call 数は増えたのに改善**です。

逆に、

```text
26 calls
 ↓
18 calls
```

になっても、

```text
tokens ↑
latency ↑
success rate ↓
```

なら失敗。

なので最終的な目的関数は、

```text
                ┌─ success rate ↑
                ├─ total tokens ↓
Agent efficiency├─ latency ↓
                ├─ recovery cost ↓
                └─ unnecessary exploration ↓
```

です。

---

# Coding Agent に渡す単位としては

僕ならさらに、各 Phase を以下のフォーマットにします。

```text
Phase N
  Goal
  Scope
  Non-goals
  Implementation
  Tests
  Evaluation
  Acceptance Criteria
```

そして**1回の Coding Agent 実行では1 Phaseだけ**渡します。

特に、

```text
Phase 1 Skill.md
Phase 2 JSON
Phase 3 Resolver
Phase 4 Symbol ID
```

は分離した方がいいです。

Resolver と CLI を同時に触らせると、問題が発生したときに、

> Skill が悪いのか、CLI が悪いのか、Semantic Layer が悪いのか

分からなくなるからです。

---

## 最終的な実装順

かなり端的にすると、

```text
P0  Trace baseline
 │
 ├─ measure only
 │
P1  Skill.md
 │
 ├─ decision tree
 ├─ discourage help
 ├─ discourage pretty
 │
P2  Output UX
 │
 ├─ compact JSON
 ├─ reduce unnecessary fields
 └─ prevent huge default output
 │
P3  Semantic Resolver
 │
 ├─ declaration/definition identity
 └─ fix callers/references ambiguity
 │
P4  Symbol ID
 │
 └─ callers/references/callees --id
 │
P5  Error UX
 │
 └─ actionable recovery
 │
P6  Command Surface
 │
 ├─ Core
 ├─ Support
 └─ Debug/Infrastructure
 │
P7  Evaluation
 │
 └─ prove total tokens / trajectory improved
```

**この順番がかなり重要**です。

特に `subcommand の削除` は後回しにして、まず **「Agent が自然に使う `search → symbol ID → callers` の一本道を作る**」のが良いと思います。

そうすると最終的には Skill.md もかなり短くできます。

```text
Find a symbol       → search
Find callers        → callers
Find references     → references
Find callees        → callees
Need exact symbol   → symbols
Need AST structure  → find

Prefer symbol IDs when available.
Use compact output.
Do not use --help unless necessary.
```

くらいまで圧縮でき、**Skill のトークン削減と Agent の trajectory 短縮を同時に達成**できます。
