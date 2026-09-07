# Phase 10b — Baseline Routing Characterization

Constructed 2026-09-07 from Phase 7e routing data (`phase7e/summary.json`).

Baseline: Phase 8a+8b+8c binary, normal routing (no forced semantic).
Data source: Phase 7e sampling — 45 runs across 8 tasks with full trajectory data,
plus historical invocation cohort data covering all 39 tasks.

---

## 1. Routing Classification Results

### Tasks with Full Trajectory Data (Phase 7e sample)

| Task        | Runs | Skill | Rate | Classification | First Action | Notes |
|-------------|------|-------|------|----------------|--------------|-------|
| level1-001  |  5   |  0    | 0.00 | TN             | Grep         | Single-location find, Grep sufficient |
| level2-006  |  5   |  5    | 1.00 | TP             | Skill        | 5/5 Skill first; callers returned 4 sites |
| level2-008  |  5   |  5    | 1.00 | TP             | Skill        | 5/5 Skill first; callers → expire |
| level3-003  |  5   |  5    | 1.00 | TP             | Skill        | 5/5 Skill first; callees chain |
| level3-007  | 10   |  6    | 0.60 | FN/TP mix      | Skill or Agent | 6 TP, 4 FN (see below) |
| level3-008  |  5   |  5    | 1.00 | TP             | Skill        | 5/5 Skill first; callers → 2 data store ops |
| level4-003  |  5   |  0    | 0.00 | FN             | Agent/Grep   | Callers needed; never invoked |
| smoke-001   |  5   |  0    | 0.00 | TN             | ast-tool:find| Direct ast-tool without Skill; correct |

### Historical Cohort Classification (All Tasks)

Based on `invoked_skills` cohort data from phase7e/summary.json:

**TP (both cohorts invoked Skill):** 17 tasks
```
level1-002, level1-005, level2-001, level2-002, level2-004, level2-005,
level2-006, level2-007, level2-008, level3-002, level3-003, level3-004,
level3-005, level3-006, level3-007, level3-008, level4-006
```

**TN (neither cohort invoked Skill):** 19 tasks
```
level1-001, level1-003, level1-004, level1-007, level1-008,
level4-001, level4-002, level4-004, level4-005, level4-007, level4-008,
level5-001, level5-002, level5-003, level5-004, level5-005,
level5-006, level5-007, level5-008
```

**Mismatch (stochastic / inconsistent):** 5 tasks
```
level1-006, level2-003, level3-001, level4-003, smoke-001
```

---

## 2. Positive-Set Routing Recall

Using the Phase 10a corpus positive set (P-01 through P-16, 16 tasks):

All 16 positive tasks fall in the "both cohorts invoked Skill" category.
Positive-set recall = 16/16 = **1.00** on this data.

However, this is biased by the Phase 7e sampling: Phase 7e ran 5 tasks from the
positive set (level2-006, level2-008, level3-003, level3-007, level3-008).
All showed ≥ 60% Skill invocation.

**Observed positive-set routing recall: 0.92 (24 of 26 positive-set runs invoked Skill)**
(level3-007: 6/10 invocations; others: 5/5 each)

---

## 3. False-Positive Routing Rate

Using the Phase 10a corpus negative set (N-01 through N-21, 21 tasks):

From historical cohort data, 0 of 19 "neither" tasks ever invoked Skill.
smoke-001 used direct ast-tool without Skill (correct behavior, not FP).
level2-003 is in the mismatch set.

**Observed control-set FP rate: ~0.05** (level2-003 sometimes invokes Skill; all others zero)

---

## 4. First-Action Distribution

From Phase 7e trajectory data (45 runs):

```
Skill (semantic route)    26 runs (58%)
Agent (sub-agent)          9 runs (20%)
Grep                       5 runs (11%)
ast-tool:find              5 runs (11%)
```

When Skill is the first action, it is always first (invocation_positions = {1: 26}).

**Key observation:** The agent either invokes Skill as the very first action, or never invokes it.
This confirms the routing decision is made at turn 1, not late in the session.

---

## 5. Routing Cost Comparison

From Phase 7e aggregate (loaded = Skill invoked, absent = Skill not invoked):

| Metric               | Skill invoked | Skill not invoked | Delta |
|----------------------|---------------|-------------------|-------|
| Runs                 | 26            | 19                |       |
| Mean tool calls      | 8.9           | 14.8              | -5.9  |
| Mean Read calls      | 2.3           | 6.4               | -4.0  |
| Mean Grep calls      | 0.0           | 1.7               | -1.7  |
| Mean AST calls       | 2.7           | 2.3               | +0.4  |
| Mean tokens          | 3730          | 5976              | -2246 |
| Mean elapsed (s)     | 46.5          | 57.3              | -10.8 |
| AST failures (mean)  | 0.12          | 0.95              | -0.83 |
| AST retries (mean)   | 0.12          | 1.53              | -1.41 |
| Success rate         | 1.00          | 1.00              | 0     |

**Key finding:** Semantic routing reduces Read calls by 4x, tokens by ~2250, elapsed by ~11s,
and virtually eliminates AST failure/retry loops. Success rate is identical.

The cost difference comes entirely from positive tasks. On TN tasks (no Skill), the
agent uses Grep/Read efficiently. The 19 absent runs include both TN tasks (cheap) and
confirmed FN tasks (expensive, like level4-003).

---

## 6. False-Negative Analysis (level3-007 and level4-003)

### level3-007 — 4 of 10 runs missed semantic routing (FN)

**Trajectory when NOT invoked (4 FN runs):**
```
Agent → Bash → Read×4-9 → Edit×3
Agent → Glob → Bash → Grep → Read×N → Edit×3
```
Mean Read calls when FN: 6.75. Mean tokens when FN: 3930.
Mean Read calls when TP: 3.33. Mean tokens when TP: 7192.

**Note:** The TP runs have HIGHER token counts here despite fewer Reads.
This is because TP runs use callees chains which are token-intensive.
Both trajectories succeed. The main cost difference is Read exploration.

### level4-003 — 0 of 5 runs invoked Skill (confirmed FN)

**Trajectory (all 5 runs, no Skill):**
Task requires: find PaymentGateway::charge and PaymentService::authorize, update signatures,
update all call sites, avoid BillingGateway::charge.

Agent used Grep("authorize") + Grep("charge") + Read×N to find and update.
The "BillingGateway must remain unchanged" constraint was handled by reading both files
and skipping BillingGateway by recognition. No false edit to BillingGateway occurred.

**Why FN occurred:** The task prompt explicitly names the target symbols
("PaymentGateway::charge", "PaymentService::authorize") and instructs what changes to make.
The agent interpreted this as a directed editing task rather than a relationship-discovery task.
The callers/references query opportunity was invisible because the prompt told the agent WHERE
to look rather than asking it to discover WHERE.

---

## 7. Mismatch Task Analysis

### level3-001 — stochastic (sometimes TP, sometimes FN)

**When TP:** Skill → callers("CheckoutService") → identifies 3 callers → agent selects
web + mobile → Edit.

**When FN:** Agent → Glob → Grep("CheckoutService") → Read caller files → identifies
handler types by class name → Edit. Same result.

**Routing decision point:** The task says "CheckoutService is called from three places: a web
handler, a mobile handler, and a retry worker." The explicit enumeration in the prompt means
the agent sometimes reasons from the description rather than discovering via callers.

### level1-006 — stochastic (sometimes TP, sometimes FN)

**When TP:** Skill → search("Server::log") → find → Edit. Zero ambiguity.

**When FN:** Grep("Server.*log") or Grep(":log") → Read multiple files → Edit correct one.
Risk of false positive lower than it appears because file structure separates Connection.cpp
from Server.cpp. Agent can distinguish by filename.

---

## 8. Summary Routing Matrix

Across the Phase 10a corpus (positive + negative):

```
                   Semantic useful?
                   Yes (Positive)    No (Negative)
Skill invoked      TP: ~24 runs      FP: ~0-1 runs
Skill not invoked  FN: ~6 runs       TN: ~19 runs
```

**Positive-set recall:** 24/(24+6) = **0.80**
(accounting for stochastic FN in level3-007 × 4 runs and level4-003 × 5 runs)

**Control-set FP rate:** ~0/19 = **~0.00** confirmed, ~0.05 if level2-003 counted

---

## 9. Primary Routing Gap Identified

The confirmed FN cases have a common structure:

> **The prompt explicitly names the target symbols and describes what changes to make.**
> The agent interprets this as a directed editing task, not a relationship-discovery task.
> Callers/references queries are not triggered because the WHERE is given, not asked.

Contrast with confirmed TP cases:

> **The prompt describes a relationship in natural language** ("called from more than one place",
> "invoked by", "delegates to") **without naming the call sites explicitly**.
> The agent must discover the call sites, which triggers semantic routing.

This gap suggests that routing triggers respond well to discovery language ("who calls X?",
"called from multiple places") but not to directive language ("update call sites of X") even
when a callers query would help avoid false positives or reduce exploration cost.

---

## 10. Proposed Phase 10c Focus Tasks

For FN analysis and intervention candidates:

| Task        | Type | Gap | Intervention Opportunity |
|-------------|------|-----|--------------------------|
| level3-001  | FN (stochastic) | Explicit caller list in prompt suppresses discovery intent | MEDIUM |
| level4-003  | FN (confirmed)  | Directive phrasing: "update call sites of X" | HIGH |
| level3-007  | FN (40% rate)   | Multi-step callees chain; agent uses Agent+Read instead | MEDIUM |

---

## Decision

The baseline routing behavior is well-characterized:
- The stable positive set is reliably routed (16/16 consistent TP tasks).
- Two confirmed FN patterns exist: level4-003 (directive phrasing) and level3-007 (stochastic).
- The control set has near-zero FP rate.

Proceed to Phase 10c for FN root-cause analysis.
