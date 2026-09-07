# Phase 10 — Semantic Routing Opportunity / Trigger Reliability

Evaluation date: 2026-09-07  
Branch: features/phase8a  
Semantic baseline: Phase 8a + 8b + 8c  
Routing baseline data: Phase 7e (45 runs, 8 sampled tasks + historical cohorts)

---

## Phase 10a — Opportunity Corpus

Full corpus: `evaluation/phase10a/corpus.md`

### Corpus Summary

| Class         | Tasks | Count |
|---------------|-------|-------|
| Positive      | P-01 through P-16 | 16 |
| Boundary (FN opportunity) | B-01 through B-04 | 4 |
| Negative/Control | N-01 through N-21 | 21 |
| **Total** | | **41** |

**Positive set selection criteria:** Tasks requiring callers, callees, or references
discovery across files where Grep is ambiguous due to same-name methods on different classes.
All 16 positive tasks were consistently routed to Skill in historical data.

**Negative set criteria:** Tasks with single-location edits, explicitly named target
symbols, or bug investigation patterns where Read+Grep is the correct approach.

---

## Phase 10b — Baseline Routing Characterization

Full report: `evaluation/phase10b/baseline.md`

### Routing Matrix (baseline, normal routing)

```
                   Semantic useful?
                   Yes (Positive)    No (Negative)
Skill invoked      TP: ~24 runs      FP: ~0-1 runs
Skill not invoked  FN: ~6 runs       TN: ~19 runs
```

### Key Metrics

| Metric | Value |
|--------|-------|
| Positive-set routing recall | 0.80 (accounting for stochastic level3-007 + confirmed level4-003) |
| Confirmed positive-set (consistent) recall | 1.00 (16/16 positive tasks always or usually routed) |
| Control-set FP rate | ~0.00 (0/19 negative tasks invoked Skill) |
| Skill-invoked: mean tools | 8.9 |
| Skill-absent: mean tools | 14.8 |
| Skill-invoked: mean tokens | 3730 |
| Skill-absent: mean tokens | 5976 |
| Skill-invoked: AST failures (mean) | 0.12 |
| Skill-absent: AST failures (mean) | 0.95 |

**First-action observation:** When Skill is invoked, it is always the first action (position 1 in 100% of cases). The routing decision is made at turn 1 and never recovered later.

---

## Phase 10c — False-Negative Analysis

Full analysis: `evaluation/phase10c/fn_analysis.md`

### Confirmed FN Cases

**level4-003 (0/5 runs routed) — highest priority FN**
- Task: Evolve PaymentGateway::charge, update call sites of PaymentService::authorize
- Trigger gap: "call sites of" is an imperative modification phrase; no current trigger matches
- Agent trajectory: Grep("authorize") + Read×N → Edit correctly but inefficiently
- Root cause: Directive phrasing with explicit symbol names; no discovery language

**level3-007 (4/10 runs not routed) — stochastic FN**
- Task: Web checkout handler initiates chain of operations
- Trigger gap: "chain of operations" not a callers/callees trigger
- Agent trajectory (FN): Agent sub-tool → Bash → Read×6 → Edit
- Root cause: Multi-step task perceived as exploration → Agent sub-tool selected first

### Cross-Case Pattern

```
FN prompt pattern:    "update call sites of X" / "X called from Y, Z, W" (explicit)
TP prompt pattern:    "X is called from multiple places" / "X is invoked by"
```

The routing trigger fires on discovery language, not on imperative modification language.
Even when a callers query would help, if the prompt says WHERE rather than asking WHAT, the trigger is not activated.

---

## Phase 10d — Candidate 1: Trigger Vocabulary Addition

Experiment design: `evaluation/phase10d/candidate1_trigger_vocabulary.md`

### Change Applied

Added to `skills/semantic-analysis/SKILL.md` triggers:

```yaml
- "all callers of"
- "call sites of"
- "every call to"
```

**Rationale:** "call sites of" directly matches level4-003's missed routing opportunity.
"all callers of" is a natural variation of the existing "callers of" trigger that captures
imperative modification phrasing. "every call to" matches multi-call-site edit tasks.

**One variable only:** Only the trigger list was changed. The Skill description, body,
semantic implementation, and agent configuration are unchanged.

### Hypothesis

- level4-003: FN → TP (routing recall improves)
- level1-001: TN stable (no "call sites of" in prompt)
- level4-004: TN stable (uses explicit function names, not "call sites of")

### Test Cohort (pending runs)

| Task       | Type | Baseline | Post-Change |
|------------|------|----------|-------------|
| level4-003 | FN   | 0/5      | TBD (5 runs needed) |
| level2-008 | TP   | 5/5      | TBD (regression check) |
| level1-001 | TN   | 0/5      | TBD (FP guard) |
| level4-004 | TN   | 0/5      | TBD (FP guard) |

---

## Current Status

| Phase | Status |
|-------|--------|
| 10a Corpus | COMPLETE |
| 10b Baseline | COMPLETE (using historical data) |
| 10c FN Analysis | COMPLETE |
| 10d Candidate 1 applied | APPLIED to SKILL.md |
| 10d Agent runs | PENDING |
| Final Recommendation | PENDING (awaiting run results) |

---

## Acceptance Criteria (Candidate 1)

Accept if:
1. level4-003 routing recall ≥ 3/5 (improvement from 0/5)
2. Success rate = 1.0 on level4-003
3. level1-001 and level4-004 remain at 0/5 Skill invocations
4. No AST failure increase on positive tasks

Reject if:
- Skill invokes on single-location tasks (level1-001)
- Skill invokes on explicit-chain tasks (level4-004)
- level4-003 semantic result is not used correctly

---

## Notes

1. The stable positive set (P-01 through P-16) shows baseline routing is already reliable
   for the canonical Phase-8b cases. Phase 10 improvement is at the margin.

2. The confirmed FN (level4-003) is a real efficiency loss: the agent reads more files,
   makes more Grep calls, and risks BillingGateway false positives. But success rate was
   1.0 even without semantic routing — the FN cost is efficiency, not correctness.

3. Candidate 1 is conservative: "call sites of" is specific to call-relationship language
   and does not appear in bug investigation or single-location edit prompts.
