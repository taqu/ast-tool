# Phase 10d — Candidate 1: Trigger Vocabulary Addition

## Experiment Design

**Variable tested:** SKILL.md trigger list (single variable).

All other variables held constant:
- Repository and fixtures unchanged
- Task prompts unchanged
- Semantic implementation unchanged (Phase 8a+8b+8c)
- Skill body unchanged
- Agent configuration unchanged

---

## Exact Change

Add to `skills/semantic-analysis/SKILL.md` triggers:

```yaml
  - "call sites of"
  - "all callers of"
  - "every call to"
```

**Rationale per trigger:**

- `"call sites of"` — matches level4-003 prompt: "update call sites of authorize".
  "Call sites" is the imperative-modification formulation of "callers of".
  
- `"all callers of"` — matches tasks that say "update all callers of X" or
  "find all callers of X". Slight variant of existing "callers of" trigger.
  
- `"every call to"` — matches level1-002 (already TP) and similar phrasing.
  Harmless addition for existing positive set; may help edge cases.

**Triggers NOT added (deferred):**

- `"chain of calls"` / `"call chain"` — Candidate 2; separate experiment.
- `"update all callers"` — too broad without "of"; could match general update instructions.

---

## Hypothesis

Adding "call sites of" should cause the agent to invoke Skill on level4-003-type tasks
where the prompt uses directive phrasing with "call sites" language.

Expected outcome:
```
level4-003: FN → TP (Skill invoked; callers query returns authorize call sites)
level1-001: TN unchanged (no "call sites of" in prompt)
level4-004: TN unchanged (prompt says "Update CheckoutService::checkout", not "call sites of")
```

---

## Test Cohort

| Task       | Expected | Baseline | Post-Change |
|------------|----------|----------|-------------|
| level4-003 | FN → TP  | 0/5      | TBD         |
| level2-008 | TP stable| 5/5      | TBD         |
| level1-001 | TN stable| 0/5      | TBD         |
| level4-004 | TN stable| 0/5      | TBD         |

Minimum 5 runs per task.

---

## Acceptance Criteria

Accept this change if:
1. level4-003 Skill invocation rate ≥ 3/5 (improvement from 0)
2. level4-003 success rate = 1.0 (no correctness regression)
3. level1-001 Skill invocation rate = 0/5 (no FP)
4. level4-004 Skill invocation rate = 0/5 (no FP on explicit-chain task)
5. No increase in AST failures on any positive task

---

## Rejection Criteria

Reject if:
- Skill invoked on level1-001 or level4-004
- level4-003 semantic result is ignored (agent uses Skill output incorrectly)
- AST failures increase on any task
- Positive set success rate decreases
