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

---

## Experiment Results

Evaluation date: 2026-09-08

Agent: Claude Code

Runs: 5 per task, 20 total

Candidate Skill SHA-256: `3863d939f93f64696feece9db29b5e497cc13d44ffad55ec7abbb1170990eb04`

The repository and installed copies of `semantic-analysis/SKILL.md` had the same
candidate hash before the experiment. The existing task prompts, repositories,
semantic implementation, agent configuration, and evaluation runner were unchanged.

### Routing and Correctness

| Task | Expected | Baseline Skill | Candidate Skill | Success | Classification |
|---|---|---:|---:|---:|---|
| level4-003 | FN -> TP | 0/5 | 0/5 | 5/5 | FN unchanged |
| level2-008 | TP stable | 5/5 | 5/5 | 5/5 | TP stable |
| level1-001 | TN stable | 0/5 | 0/5 | 5/5 | TN stable |
| level4-004 | TN stable | 0/5 | 0/5 | 5/5 | TN stable |

Candidate routing matrix for this cohort:

```text
                   Semantic useful?
                   Yes              No
Skill invoked      TP: 5 runs       FP: 0 runs
Skill not invoked  FN: 5 runs       TN: 10 runs
```

The target-task routing recall remained `0/5 = 0.00`; the control-set
false-positive rate remained `0/10 = 0.00`. Overall correctness was `20/20 = 1.00`.

### Per-Task Metrics

| Task | First action distribution | Mean tools | AST calls | AST failures | Mean Read | Mean Grep | Mean tokens | Mean elapsed (s) |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| level4-003 | Grep 4, Bash 1 | 29.0 | 0 | 0 | 16.4 | 3.8 | 11089.8 | 66.86 |
| level2-008 | Skill 5 | 4.4 | 6 | 0 | 1.2 | 0.0 | 1271.4 | 32.69 |
| level1-001 | Grep 5 | 4.8 | 0 | 0 | 1.0 | 2.8 | 1103.4 | 18.83 |
| level4-004 | Bash 3, Glob 1, Grep 1 | 22.2 | 0 | 0 | 11.4 | 0.6 | 10873.0 | 66.29 |

There were no AST failures, retries, or help calls. On level2-008, the Skill was
always the first action and its `callers` result directly identified the logout and
refresh call sites used by the edit. On level4-003, the added vocabulary never caused
Skill routing; all five runs used manual discovery and averaged 16.4 Reads and 3.8
Greps.

The current Skill validator reports the pre-existing `triggers` and `languages`
frontmatter keys as unsupported. Candidate 1 did not introduce those keys, but it
changed only values inside `triggers`. This is consistent with the evaluated runtime
not using that list for discovery, although the behavioral traces alone do not prove
that mechanism. Testing the supported `description` field would be a separate
one-variable candidate.

Raw results and traces are in `evaluation/phase10d/candidate1/r1` through `r5`.

## Recommendation

**REJECT**

Candidate 1 fails the primary acceptance criterion: level4-003 required at least
3/5 Skill invocations but remained at 0/5, identical to baseline. The candidate did
not introduce false positives or correctness regressions, but it provided no routing
benefit. The three added triggers were therefore reverted from the repository and
installed Skill, restoring the pre-candidate trigger list.
