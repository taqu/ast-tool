## 現在の全体像

最終目標は一貫して、**Coding Agent が必要なときに AST Tool の semantic capability を使い、少ない無駄で正しい編集まで到達すること**です。

評価軸は次のままです。

```text
1. correctness
2. semantic routing quality
3. AST failure / retry
4. manual fallback
5. tool count
6. token/context cost
7. latency
8. recovery cost
```

重要なのは、

```text
AST call を減らすこと自体
```

ではなく、

```text
targeted semantic query
→ small relevant context
→ edit / solution
```

へ近づけることです。

---

# ロードマップ

```text
P0  Baseline / trace metrics
    ✓ COMPLETE

P1  Skill.md decision tree
    ✓ COMPLETE

P2  Output / JSON UX
    ✓ COMPLETE

P3  Semantic resolver
    → historical item; later Phase 8 workで実質的に具体化

P4  Stable semantic symbol ID
    → DEFERRED

P5  Error recovery UX
    ✓ ACCEPTED
    → long-term behavioral baseline

P6  Agent-facing command surface
    ✗ REJECTED

P7  Skill / Agent Guidance Optimization
    ✓ COMPLETE

P8  Semantic Command Semantics Optimization
    ✓ COMPLETE
    → PROMOTED TO STABLE BASELINE

P9  Final Agent-Level Evaluation
    ✓ COMPLETE

P10 Semantic Routing Opportunity / Trigger Reliability
    → NEXT CANDIDATE
```

---

# Phase 5 — Stable behavioral baseline

Phase 5 が長く比較基準として機能しました。

41 tasks:

```text
success            37 / 41 = 90.24%
tools              518
AST calls           69
AST failures         9
AST retries          9
grep                15
glob                12
read               252
bash               120
edit                86
tokens          158,303
elapsed        2224.27 sec
```

ここからの教訓は、agent guidance と error recovery のバランスが比較的安定していたことです。

---

# Phase 6 — Command surface experiment

```text
Status: REJECTED
```

Correctness は維持できたものの、

```text
semantic usage ↓
grep/manual exploration ↑
tokens/recovery ↑
```

となりました。

重要な学び：

```text
Tool presentation changes agent routing.
```

単に CLI を簡略化すれば agent behavior が改善するわけではありません。

---

# Phase 7 — Skill / Agent Guidance Optimization

## 7a–7c

Skill の圧縮・semantic-preserving compression を検証。

結果として、

```text
too much compression
→ weaker trajectory
```

が確認されました。

## 7d — Selective backport

Phase 5 Skill に、根拠のある1ルールだけ追加：

```text
If a refined search already identifies the exact symbol or member needed,
do not add a redundant find solely to locate it;
use find when AST structure or node detail is required.
```

Controlled gate では改善。

```text
tools      -15
AST calls   -5
failures    -4
tokens      -9.2%
elapsed    -10.9%
```

最終判断：

```text
ACCEPT WITH CAVEATS
```

Phase 7d Skill body が以降の accepted Skill baseline になりました。

---

# Phase 7e — Skill invocation reliability

目的：

```text
semantic-analysis が呼ばれる / 呼ばれない
```

という routing 問題を分離。

主結果：

```text
semantic-analysis invocation
= always first action when invoked
```

つまり late invocation ではなく、

```text
first-decision router
```

として動いていることが判明。

また invocation は task-dependent で、一部 stochastic。

重要な発見：

```text
more invocation
!= automatically lower cost
```

同じ `level3-007` では semantic route は Read を減らしたものの、tools/tokens/time は増えました。

最終判断：

```text
INVOCATION PROBLEM CONFIRMED
NO SAFE FIX YET
```

ここで invocation rate 自体を最適化するのは保留。

---

# Phase 7f — Semantic Routing Value / Toolset Cost Audit

ここが Phase 8 の出発点になりました。

154 AST calls を分類：

```text
A necessary semantic query
B identity/resolution overhead
C relationship retry overhead
D redundant structural lookup
E context-fetch overhead
F recovery/error overhead
```

主要発見は3つ。

## 1. Relationship target resolution

```text
callers AuthToken::expire
→ not found
→ search expire
→ callers auth::AuthToken::expire
```

partial FQN が exact FQN としてしか扱われないため、3-call recovery が発生。

## 2. Receiver-type relationship gap

```text
token_.validate()
validator_.validate()
```

が存在しても、

```text
callers/references AuthToken::validate
```

が empty。

## 3. Declaration/body identity gap

target declaration は見つかるが、body-bearing definition に繋がらず `callees` が empty。

Phase 7f の結論：

```text
EXISTING COMMAND SEMANTICS SHOULD BE IMPROVED
```

新 command ではなく既存 semantics を直すべき、と判断。

---

# Phase 8 — Semantic Command Semantics Optimization

Phase 8 は完了し、現在の stable semantic baseline です。

---

## Phase 8a — Unique FQN-Suffix Relationship Resolution

変更：

```text
exact FQN first
→ if no exact match
→ unique "::" + query suffix fallback
→ preserve ambiguity
```

対象：

```text
callers
callees
references
```

結果：

```text
3-call recovery
→ 1 successful relationship call
```

exact precedence / ambiguity / not-found / unqualified behavior は維持。

最終判断：

```text
ACCEPT WITH CAVEATS
```

---

## Phase 8b — Receiver-Type Member Relationship Resolution

Phase 8 の中で、**agent-level ROI が最も強く確認された改善**です。

対応した receiver：

```text
object field
pointer field
local object
local pointer
reference parameter
pointer parameter
```

原則：

```text
receiver identifier
→ explicit declared/static type
→ canonical class
→ unique member
```

以下は意図的に未対応：

```text
auto / decltype
complex receivers
templates
inheritance / virtual dispatch
overloads
explicit this->
```

False-positive guard:

```text
same member name on unrelated types
→ cross-linkしない
```

結果：

```text
callers auth::AuthToken::validate
Before: empty
After: exact callers
```

callers / references / callees に generalize。

最終判断：

```text
ACCEPT WITH CAVEATS
```

ただし caveats は未完成というより intentional scope boundary。

---

## Phase 8c — Declaration/Definition Body Identity for Callees

問題：

```text
callees AuthService::refresh
→ Method declaration selected
→ no body
→ empty
```

改善：

```text
canonical callable identity
→ same-FQN body-bearing definition
→ traverse definition body
```

対応：

```text
free function decl + definition
class method + out-of-line definition
namespaced out-of-line definition
inline method
```

Regression tests:

```text
291 tests
0 failures
```

Motivating case：

```text
Before:
callees auth::AuthService::refresh
→ empty

After:
→ auth::AuthToken::expire
→ auth::TokenCache::invalidate
→ auth::AuthToken::refresh
```

最終判断：

```text
ACCEPT
```

残る edge caveat：

```text
ODR-violating multiple body definitions
→ first matching body currently wins
```

これは unsupported edge case として記録。

---

# Phase 9 — Final Evaluation

Phase 8 を stable baseline に昇格できるかを検証しました。

---

## Phase 9a — Controlled Semantic Capability Evaluation

Phase 8 capability set を controlled routing で評価。

主結果：

```text
8a:
partial-FQN failure/retry eliminated

8b:
empty callers → exact populated callers

8c:
empty callees → exact populated callees

false positives:
0

guards:
stable
```

結果：

```text
ACCEPT PHASE 8 CAPABILITY SET
```

ただし Arm A の一部が historical proxy だったため、fresh symmetric A/B が必要になりました。

---

## Phase 9b.1 — Fresh Controlled Confirmation

Fresh A/B、forced semantic routing。

```text
84 agent runs
84/84 semantic-analysis first
0 binary drift
0 Skill drift
```

Whole cohort:

```text
tools        -2.02 / run
AST calls    -0.57
tokens       -1533
elapsed      -6.25 sec
```

Phase 8b が特に強い：

```text
Arm A empty relationship fallback:
14/15

Arm B:
0/15
```

8c も agent-level で：

```text
Arm A false-empty:
5/5

Arm B:
0/5
```

結論：

```text
CONFIRM PHASE 8
```

---

## Phase 9b.2 — Final Normal-Routing Evaluation

Forced routing を外し、実際の agent behavior を評価。

```text
41-task full suite
+ selected repeated cohort
= 194 runs
```

Whole-suite Stage 1：

```text
                Arm A       Arm B

success         92.7%       90.2%
tools           10.78        9.39
AST calls        0.39        0.61
AST failures     0.073       0.049
AST retries      0.122       0.049
Read             5.098       3.707
tokens        6020.4      5737.0
elapsed         30.44       26.26
```

Success差は `level4-005` の inherent flakiness で説明され、5-repeat では両 Arm 20%。

したがって Phase 8 attributable correctness regression はなし。

### Phase 8b under normal routing

最も重要な結果：

```text
Arm A:
affected relationship invoked → false empty 9/9

Arm B:
affected relationship invoked → false empty 0/9
```

さらに same-loaded task では：

```text
level2-004
level4-006
```

が最大の Arm B 改善。

おおよそ：

```text
tools    -4.4 ～ -5.2
tokens   -3.4k ～ -4.7k
elapsed  -18s ～ -27s
```

### Final decision

```text
PROMOTE PHASE 8 TO STABLE BASELINE
```

Phase 9 は完了。

---

# 現在の Stable Baseline

現在の正式な baseline は：

```text
Phase 7d Skill
+
Phase 8a
    unique FQN suffix resolution
+
Phase 8b
    receiver-type member relationships
+
Phase 8c
    callable body identity
```

つまり、**pre-Phase-8 に戻る理由はありません**。

---

# 現在見えている最大の問題

Phase 9b.2 で、semantic capability 自体より大きな bottleneck が明確になりました。

Normal routing の `semantic-analysis` invocation：

```text
weighted corpus:
14.4%

raw full-suite:
roughly 5–12%
```

大多数：

```text
manual Grep/Glob/Read
or
direct AST without Skill
```

です。

つまり、

```text
semantic capability is useful when selected
```

は証明された一方で、

```text
it is often not selected
```

が現在の主要課題です。

---

# 次の Phase 10

次は semantic capability をさらに増やすより、

```text
Phase 10
Semantic Routing Opportunity / Trigger Reliability
```

を推奨。

ただし目的は：

```text
increase invocation rate
```

ではありません。

目標は：

```text
increase appropriate semantic routing
where semantic routing has demonstrated value
without increasing low-value invocation
```

です。

---

# Phase 10 の評価思想

Phase 8b の成功パターンを positive evidence の中心にするのがよいです。

コード baseline は：

```text
Phase 8a + 8b + 8c
```

のまま。

ただし routing experiment の positive set は **Phase-8b-like tasks** を中心にします。

例：

```text
who calls X?
where is X referenced?
which callers need modification?
cross-file member relationship
member-method relationship discovery
```

これらは、

```text
Skill invoked
→ useful semantic result
→ fewer fallbacks / Reads / tokens / time
```

が実証済み。

---

# Routing を classification problem として扱う

単純な invocation rate ではなく：

```text
                     Semantic route useful?
                     Yes          No

Invoked              TP           FP
Not invoked          FN           TN
```

として扱う。

主目標：

```text
reduce FN
without materially increasing FP
```

つまり、

```text
missed valuable semantic route
```

を減らす。

---

# Phase 10 の候補実験

最初から Skill body をいじるのではなく、まず：

```text
1. positive opportunity set
2. negative/control set
3. current routing classification
4. first-decision divergence
```

を測る。

その後、一変数ずつ：

```text
Skill description
trigger phrasing
discovery metadata
system-level routing hint
```

などを試す。

評価：

```text
appropriate invocation
correctness
semantic precision
tools
tokens
elapsed
manual fallback
```

---

# 今は優先しないもの

## Callee-side declaration/definition identity

Phase 9b.1 で、

```text
repo_.update(u)
→ UserRepository::update
```

が `callees` に出ない residual gap を発見。

ただし normal routing ではほぼ未露出。

したがって：

```text
real semantic gap
but not current top priority
```

将来 repeated evidence が増えたら Phase 10/11 candidate。

## Windows path quoting

194 runs 中2件。

```text
real harness issue
low urgency
```

## `--help` overuse

forced routing では一度見えたが、normal routing では再現せず。

```text
not currently actionable
```

---

# 現在のロードマップ

```text
P0  Baseline / tracing
    ✓

P1  Skill decision tree
    ✓

P2  Output / JSON UX
    ✓

P3/P4
    deferred / superseded by later evidence-driven work

P5  Recovery UX
    ✓

P6  Agent-facing command surface
    ✗ rejected

P7  Skill / Agent Guidance Optimization
    ✓ complete

    7d stable Skill body
    7e invocation behavior characterized
    7f semantic cost audit

P8  Semantic Command Semantics Optimization
    ✓ complete
    ✓ promoted stable

    8a FQN suffix resolution
    8b receiver-type member relationships
    8c callable body identity

P9  Final Agent-Level Evaluation
    ✓ complete

    9a controlled validation
    9b.1 fresh symmetric confirmation
    9b.2 normal-routing final evaluation

P10 Semantic Routing Opportunity / Trigger Reliability
    → NEXT
```

## 次の議論での出発点

一文で言えば：

> **Semantic capability is now good enough to be the stable baseline. The next bottleneck is not semantic correctness but selecting that capability on the tasks where it has proven agent-level value.**

そして Phase 10 の設計は、**Phase 8b の「semantic route を使えば明確に勝つ」タスク群を positive set として始める**のが最も evidence-driven です。
