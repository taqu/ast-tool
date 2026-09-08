# Phase 10c — First-Decision False-Negative Analysis

Constructed 2026-09-07 from Phase 7e trajectory data and task prompt review.

---

## Overview

Phase 10b established two confirmed FN patterns:

1. **level4-003**: 0/5 Skill invocations despite clear callers/references opportunity.
2. **level3-007**: 4/10 Skill non-invocations on a multi-step callees chain task.

And one stochastic boundary case:

3. **level3-001**: Sometimes FN, sometimes TP.

All FN cases show the same structural feature: **the routing failure occurs at turn 1**.
When Skill is not invoked first, it is never invoked at all.

---

## FN Case A — level4-003 (Confirmed, 0/5 runs)

### Task Prompt (abridged)
```
Evolve PaymentGateway::charge to accept a transaction descriptor string parameter.
Change PaymentService::authorize signature. Ensure all definitions are updated.
Update call sites of authorize within checkout/order system:
  - Inside OrderService::submit, pass "user_checkout".
  - Inside AdminOrderService::submit, pass "admin_bulk".
BillingGateway::charge must remain unchanged.
```

### Why Semantic Route Was Not Selected

**Root cause: Directive phrasing with explicit symbol names and explicit call site enumeration.**

The prompt contains all of the following:
- Named targets: `PaymentGateway::charge`, `PaymentService::authorize`
- Explicit call sites: `OrderService::submit`, `AdminOrderService::submit`
- Explicit values to pass at each call site

The agent sees a multi-step editing task with a complete specification. The WHAT and WHERE
are both given. The agent does not need to discover call sites; it is told where they are.

**Trajectory (run 1 of 5 — representative):**
```
Agent → Glob → Grep("PaymentGateway") → Grep("authorize") → Read(payment_gateway.h)
→ Read(payment_service.h) → Read(order_service.cpp) → Read(admin_order_service.cpp)
→ Edit×4 → Read(payment_gateway.cpp) → Edit
```

The agent finds all files by Grep and reads them directly. It correctly skips
BillingGateway by reading billing_gateway.h and recognizing it doesn't match.

**Why callers would have helped:**
The callers query for `PaymentService::authorize` would have returned:
`OrderService::submit`, `AdminOrderService::submit` (and nothing else).
The agent would have had zero BillingGateway false-positive risk and fewer Reads.
Instead, it Grep'd broadly and read files unnecessarily.

**Trigger gap analysis:**

Current triggers that should have matched this task:
```
"callers of"    → NOT present in prompt
"who calls"     → NOT present in prompt
"find references" → NOT present in prompt
"find usages"   → NOT present in prompt
```

The prompt says "update call sites of authorize" — this is imperative, not interrogative.
It is a modification instruction, not a discovery instruction. The trigger vocabulary
does not include "call sites of" or "update all callers" as discovery cues.

**First-decision cause:** Trigger mismatch. The agent's intent is to edit, not discover.
The semantic value (safe enumeration of callers, disambiguation from BillingGateway) is
invisible from the directive framing.

---

## FN Case B — level3-007 (Stochastic, 4/10 non-invocations)

### Task Prompt (abridged)
```
The web checkout handler initiates a chain of operations: checkout service, order service,
payment authorization. Identify the callees at each step and add trace logging.
```

### Why Semantic Route Was Sometimes Not Selected

**Root cause: Competition from Agent sub-agent tool; callees chain not always phrased as discovery.**

**Trajectory when FN (4 runs):**
```
Agent(sub-agent) → Bash → Read×4-9 → Edit×3
```
or:
```
Agent → Glob → Bash → Grep → Read×N → Edit×3
```

**Trajectory when TP (6 runs):**
```
Skill → ast-tool:search × 4 → [Glob|Read|find] → Read → Edit×3
```

The FN runs delegate to an Agent sub-agent as their first action. The sub-agent does
file exploration without access to the semantic-analysis Skill. Once inside the
sub-agent context, the Skill is unavailable for the discovery phase.

**Two distinct FN patterns:**
1. **Agent-first pattern**: Agent spawned before Skill considered. Sub-agent explores with
   Read×6-9 and produces edit plan. Main agent then edits.
2. **Glob-first pattern**: Agent does direct file exploration, reasons from file structure,
   identifies checkout chain by reading sequential files.

**First-decision cause:** Agent sub-tool competes with Skill as a first-action choice for
"complex multi-step exploration" tasks. When the task involves a "chain" of operations,
the agent may perceive it as exploration work → Agent sub-tool rather than
relationship discovery → Skill.

**Trigger gap:** The phrase "chain of operations" or "initiates a chain" does not map to
any current trigger. "callees of" or "what does X call" would trigger Skill, but the
prompt describes the outcome structure (chain), not the discovery intent.

---

## FN Case C — level3-001 (Stochastic, inconsistent)

### Task Prompt (abridged)
```
CheckoutService is called from three places: a web checkout handler, a mobile checkout
handler, and a retry worker. Find web and mobile handlers and add log line. Do not modify
the retry worker.
```

### Why Semantic Route Was Sometimes Not Selected

**Root cause: Explicit caller enumeration in prompt reduces discovery intent signal.**

The prompt names all three callers by type (web, mobile, retry). The agent knows how
many callers exist and what they are. The only open question is WHERE the web and mobile
handler files are located.

**When TP:** Agent reasons "I need to find the callers of CheckoutService" → Skill → callers.
**When FN:** Agent reasons "I know there are 3 callers; I need to find the web and mobile
ones" → Grep("CheckoutService") → read files → identify by class name → Edit.

Both approaches succeed. The Grep approach succeeds because the codebase is small enough
that `grep CheckoutService` returns a tractable list, and the web/mobile distinction is
readable from the class names in the caller files.

**First-decision cause:** The explicit caller description in the prompt short-circuits the
"need to discover callers" trigger. The agent already has the answer to "who calls this?"
embedded in the problem statement. The routing trigger fires on the question, not the answer.

---

## Cross-Case Pattern Summary

All three FN cases share the same structural feature:

```
PROMPT PATTERN:                         ROUTING OUTCOME:
"X is called by Y, Z, W"               FN — callers enumerated, no discovery needed
"Update call sites of X: at Y, at Z"   FN — directive, no discovery verb
"X initiates a chain of operations"    FN (sometimes) — chain is described, not queried
```

vs. confirmed TP cases:

```
PROMPT PATTERN:                         ROUTING OUTCOME:
"X is called from more than one place" TP — discovery intent is explicit
"X is invoked by multiple services"    TP — "invoked by" triggers discovery
"What does X call?"                    TP — direct callees query
"Audit logger invoked by two stores"   TP — two unspecified places, discovery needed
```

**Core finding:** The routing decision is primarily driven by whether the prompt contains
an open-ended discovery phrase vs. a closed-ended directive or explicit enumeration.

Current triggers fire on: "who calls", "callers of", "callees of", "find references",
"find usages", "called from multiple places" (implicit from phrasing).

Current triggers miss: "update call sites of", "chain of operations", "called from three
places: [names enumerated]".

---

## First-Action Distribution for FN Runs

From Phase 7e trajectory data:

| First Action in FN Run | Count | Pattern |
|------------------------|-------|---------|
| Agent (sub-agent)      | 7     | Sub-agent takes over exploration |
| Grep                   | 3     | Direct text search |
| Glob                   | 2     | File enumeration |

**No FN run used ast-tool directly without Skill** (as smoke-001 does).
FN runs diverged via Agent or Grep, not via direct ast-tool use.

---

## Implication for Phase 10d

The FN root causes fall into two categories:

**Category 1: Trigger vocabulary gap**
Phrases like "update call sites of X" or "update all callers" are imperative modifications
that imply a callers query but do not use discovery language.

Candidate intervention: Add trigger phrase that matches imperative "update/modify call
sites of X" or "all callers" language.

**Category 2: Agent sub-tool competition**
For multi-step chain tasks, the Agent sub-tool is selected before Skill consideration.

Candidate intervention: Add callees-chain language ("initiates a chain", "what does X
call") to triggers. Lower-cost intervention than sub-tool suppression.

---

## Phase 10d Candidate Interventions

Ranked by estimated impact and safety:

### Candidate 1 — Add "call sites" and "update callers" triggers (Category 1)

**Change:** Add triggers to SKILL.md:
```yaml
triggers:
  ...
  - "call sites of"
  - "update all callers"
  - "all callers of"
  - "every call to"
```

**Hypothesis:** Matches level4-003 and level4-005 task language.
**Risk:** Could broaden to tasks like N-07 where "add to standard path" may now match.
**Estimated FN improvement:** HIGH for B-04 (level4-003).
**Estimated FP risk:** LOW-MEDIUM (imperative editing tasks are distinguishable).

### Candidate 2 — Add callees chain language (Category 2)

**Change:** Add triggers:
```yaml
triggers:
  ...
  - "chain of calls"
  - "what does X call"
  - "what operations does"
```

**Hypothesis:** Matches level3-007 stochastic case.
**Risk:** Low — these phrases are specific to callees-chain tasks.
**Estimated FN improvement:** LOW-MEDIUM (level3-007 is already 60% routed; improvement
is at margin).

### Candidate 3 — Description refinement (routing-guidance strengthening)

**Change:** Enhance the SKILL.md description to include imperative relationship tasks:
```
description: ... use when a task requires discovering or enumerating callers, callees,
or references — including when updating call sites, propagating changes to callers, or
tracing call chains across files.
```

**Hypothesis:** Description-level guidance affects LLM routing when triggers are ambiguous.
**Risk:** Lower precision — broadens applicability statement.

---

## Prioritization for Phase 10d

Test Candidate 1 first (single trigger-vocabulary variable).

If Candidate 1 reduces level4-003 FN without increasing FP on N-01 through N-05,
accept and measure.

Do not combine Candidates 1 and 2 in the same initial experiment.

---

## Rejection Pre-conditions

If the following occur, reject any candidate:

- Skill is invoked on level1-001 (single-location find) → FP
- Skill is invoked on level4-004 or level4-005 (explicit chain, no discovery) → FP
- Skill invocation rate on negative set increases by > 1 task → likely FP regression
- Positive-set success rate decreases → correctness regression
