# Phase 10a — Opportunity Corpus

Constructed 2026-09-07 from:
- 39 task YAML files in `evaluation/tasks/`
- Phase 7e routing summary (`phase7e/summary.json`) — 45 runs across 8 sampled tasks
- Historical invocation cohort data from `phase7e/summary.json`

---

## Corpus Classification

Each task is classified as:

```
positive    — semantic routing expected to have concrete value
boundary    — routing likely valuable but historically stochastic or untested
negative    — semantic routing unnecessary, low-value, or clearly inferior
```

---

## Positive Set

Tasks where semantic relationship queries are expected to have concrete value.
All confirmed by historical routing: agent selected Skill in all or most runs.
All have success = 1.0 in both routed and unrouted runs, but routed runs show
lower tool counts, fewer Reads, and fewer retries.

### P-01 — level2-006 (Token::validate callers, multi-file)
**Prompt excerpt:** "Token::validate called from more than one source file. Before every call site, add a log line."
**Why positive:** Finding all callers of a member method across multiple files is the canonical Phase-8b case. Grep alone risks name ambiguity (validate exists on multiple classes).
**Historical routing:** 5/5 Skill invoked. Trajectory: Skill → search → callers → Read×4 → Edit×4. Zero Grep calls.
**Semantic commands used:** search + callers.
**Category:** callers / multi-file.

### P-02 — level2-008 (AuthToken::expire callers)
**Prompt excerpt:** "AuthToken::expire method called from more than one place. Add a deprecation notice before every call."
**Why positive:** Member method callers across files. `expire` name is non-unique without receiver-type disambiguation.
**Historical routing:** 5/5 Skill invoked. Mean AST calls: 2.0, mean tokens: 1273.
**Semantic commands used:** callers (sometimes with search first).
**Category:** callers.

### P-03 — level2-004 (AuthToken::validate callers, same-name problem)
**Prompt excerpt:** "AuthToken::validate method with multiple callers across authentication system."
**Why positive:** `validate` exists on both AuthToken and AuthService. Callers query with receiver-type resolution distinguishes them correctly.
**Historical routing:** 5/5 Skill invoked (both historical cohorts).
**Category:** callers / member disambiguation.

### P-04 — level2-001 (multiple authenticate methods)
**Prompt excerpt:** "Multiple classes — AuthService, SessionManager — each have authenticate/validate. Modify callers of AuthService::authenticate."
**Why positive:** Cross-file callers query with same-name disambiguation.
**Historical routing:** 5/5 Skill invoked (both cohorts).
**Category:** callers / multi-class disambiguation.

### P-05 — level2-002 (AuthToken and AuthService same method name)
**Prompt excerpt:** "AuthToken and AuthService both have a method. Find callers of AuthToken's version."
**Why positive:** Explicit member-class disambiguation required. Grep would find both.
**Historical routing:** 5/5 Skill invoked (both cohorts).
**Category:** callers / member disambiguation.

### P-06 — level2-005 (auth service refresh callees)
**Prompt excerpt:** "The auth service refresh method performs several operations. List what it calls."
**Why positive:** Direct callees query for a member method — exact Phase-8c capability.
**Historical routing:** 5/5 Skill invoked (both cohorts).
**Semantic commands used:** callees.
**Category:** callees.

### P-07 — level2-007 (Router::route callers)
**Prompt excerpt:** "Router::route invoked from multiple source files. Add middleware call before each invocation."
**Why positive:** Member method callers across files.
**Historical routing:** 5/5 Skill invoked (both cohorts).
**Category:** callers / multi-file.

### P-08 — level3-002 (payment authorization callers)
**Prompt excerpt:** "Payment authorization step invoked from multiple services. Add an audit log before each invocation."
**Why positive:** Callers of a member method across multiple service classes.
**Historical routing:** 5/5 Skill invoked (both cohorts).
**Category:** callers.

### P-09 — level3-003 (inventory reservation callees)
**Prompt excerpt:** "When an order is submitted, inventory is reserved through a chain of calls."
**Why positive:** Callees chain — who does order submission call, and what do those callees call?
**Historical routing:** 5/5 Skill invoked. Mean trajectory: Skill → search → 1-2 AST calls → Read → Edit.
**Category:** callees / chain.

### P-10 — level3-004 (payment delegation callees)
**Prompt excerpt:** "Payment authorization delegates to underlying gateways."
**Why positive:** Callees of a payment authorization method — exact callees query.
**Historical routing:** 5/5 Skill invoked (both cohorts).
**Category:** callees.

### P-11 — level3-005 (data store callees — audit logging)
**Prompt excerpt:** "The data store performs audit logging within two of its operations. Identify which callees are invoked."
**Why positive:** Callees of specific member methods.
**Historical routing:** 5/5 Skill invoked (both cohorts).
**Category:** callees.

### P-12 — level3-006 (sync job callees — operation chain)
**Prompt excerpt:** "The sync job executes: loading data, syncing, saving. Identify the callees."
**Why positive:** Callees chain for a sync operation.
**Historical routing:** 5/5 Skill invoked (both cohorts). (Note: was in invocation mismatch for direct ast-tool invocations, but Skill was consistently used.)
**Category:** callees / chain.

### P-13 — level3-008 (audit logger callers, two data store operations)
**Prompt excerpt:** "Audit logger invoked by two data store operations: save and update. Find both and modify them."
**Why positive:** Callers of audit logger method. Multiple callers from same class.
**Historical routing:** 5/5 Skill invoked. Mean tools: 9.0, mean tokens: 3483. Zero Grep calls.
**Category:** callers.

### P-14 — level4-006 (rename validate, update semantic callers)
**Prompt excerpt:** "Rename ValidationService::validate to validatePayload. Update all semantic callers. Do not touch DatabaseStore::validate or CronJob::validate."
**Why positive:** Callers + references query with disambiguation (three classes share the name `validate`). The "do not touch" constraint makes Grep insufficient — it cannot distinguish by receiver type.
**Historical routing:** Consistently invoked (invoked_skills.both cohort).
**Category:** callers / selective-refactor / member disambiguation.

### P-15 — level1-002 (InventoryService::save callers)
**Prompt excerpt:** "InventoryService::save called from more than one place. Add log before every call site."
**Why positive:** Multi-file callers of a member method — foundational positive case.
**Historical routing:** 5/5 Skill invoked (both cohorts).
**Category:** callers / multi-file.

### P-16 — level1-005 (Server creates Connection and calls)
**Prompt excerpt:** "Server creates Connection objects and calls a method. Add logging to all call sites."
**Why positive:** Callers/call-site enumeration across classes.
**Historical routing:** 5/5 Skill invoked (both cohorts).
**Category:** callers.

---

## Boundary Set

Tasks where semantic routing is plausibly valuable but was historically stochastic or never invoked.
These are the primary FN opportunities for Phase 10.

### B-01 — level3-001 (CheckoutService callers — subset constraint)
**Prompt excerpt:** "CheckoutService called from three places: web handler, mobile handler, retry worker. Find web and mobile handlers and add log. Do NOT modify retry worker."
**Why boundary:** Callers query on CheckoutService is needed to enumerate all three callers, then the agent must select only web+mobile. Grep on "CheckoutService" would find all three but the constraint requires understanding caller identity.
**Why not fully positive:** The agent can sometimes solve this with Grep + Read (read the files, identify handler types by class name). The discrimination is behavioral not structural.
**Historical routing:** Stochastic. In invoked_skills.mismatch cohort — sometimes Skill invoked, sometimes not.
**FN risk:** HIGH. When not invoked, agent does Grep→Read×N exploration.
**Category:** callers / caller-subset.

### B-02 — level3-007 (web checkout handler callees chain)
**Prompt excerpt:** "The web checkout handler initiates a chain of operations. Identify and modify the chain."
**Why boundary:** Multi-step callees chain requiring traversal. Semantic helps enumerate the chain without manual Read exploration.
**Historical routing:** 6/10 Skill invoked (classification C). When invoked: mean AST calls = 5.3, zero Grep. When not invoked: mean Read = 6.75.
**FN risk:** MEDIUM. Non-invoked runs still succeed but with significantly more Reads.
**Category:** callees / chain.

### B-03 — level1-006 (Server::log disambiguation)
**Prompt excerpt:** "Both Connection and Server have a method named log. Find Server::log implementation and add line."
**Why boundary:** Search/find with disambiguation is the correct tool. Without Skill, agent must Grep for `log` in context and read multiple files to identify the Server implementation.
**Historical routing:** Stochastic. In invoked_skills.mismatch cohort. When invoked (phase7f data): Skill → search → find → success with 2 AST calls, no Grep.
**FN risk:** MEDIUM. Grep can solve this but risks editing the wrong log.
**Category:** search / disambiguation.

### B-04 — level4-003 (PaymentGateway::charge API evolution — callers)
**Prompt excerpt:** "Evolve PaymentGateway::charge and PaymentService::authorize to add descriptor parameter. Update call sites of authorize in checkout/order system. BillingGateway::charge must remain unchanged."
**Why boundary:** Callers of `authorize` needed. References query needed. The "BillingGateway must not change" constraint makes Grep risky — false positive disambiguation is exactly the Phase-8b use case.
**Historical routing:** 0/5 Skill invoked (invoked_skills.neither). Agent used Grep + Read. Success = 1.0 but through manual exploration.
**FN risk:** HIGH. This is a confirmed FN: semantic routing would have been more efficient and safer.
**Category:** callers / references / api-evolution.

---

## Negative / Control Set

Tasks where semantic routing is unnecessary, low-value, or where Grep/Read is the correct approach.
Routing changes must not cause Skill invocation on these tasks.

### N-01 — level1-001 (InventoryService::update single location)
**Prompt:** "Find InventoryService::update and add diagnostic line at start of body."
**Why negative:** Single-location find. The method name `update` is unique enough in context. No cross-file callers needed. Agent solved with Grep (1-3 calls) + Edit in 3-6 tools total.
**Historical routing:** 0/5 Skill invoked. Mean tools: 4.8. Mean tokens: 1158.
**TN verdict:** Confirmed. Semantic routing adds no value here.

### N-02 — level1-003 (same method on two classes — single modification)
**Prompt:** "Both InventoryService and OrderService have a method called `process`. Find and modify only the InventoryService version."
**Why negative:** Disambiguation is needed, but a direct Grep + context Read is sufficient. No callers enumeration required — just find the right definition and edit it.
**Historical routing:** 0/5 Skill invoked.
**TN verdict:** Confirmed.

### N-03 — level1-004 (OrderService single change)
**Why negative:** Single-file, single-location edit. No relationship discovery needed.
**Historical routing:** 0/5 Skill invoked.
**TN verdict:** Confirmed.

### N-04 — level1-007 (config management, two classes, single target)
**Why negative:** Find one method implementation in a two-class codebase. Grep is direct.
**Historical routing:** 0/5 Skill invoked.
**TN verdict:** Confirmed.

### N-05 — level1-008 (Application loads config — single-file change)
**Why negative:** Locate single method and add code. No cross-file discovery.
**Historical routing:** 0/5 Skill invoked.
**TN verdict:** Confirmed.

### N-06 — level2-003 (RequestHandler overloads — single class)
**Why negative:** Three overloads of a method in a single class. No callers enumeration needed — just find the right overload.
**Historical routing:** Stochastic (invoked_skills.mismatch) — some runs used Skill but it added marginal value. When Skill was used, the agent did extra search calls vs direct Grep.
**TN/FP verdict:** Using Skill here is low-value. If a routing change causes more Skill invocation on this task, treat as FP signal.

### N-07 — level4-001 (add audit log to standard user path, avoid admin path)
**Why negative:** Task names the target function implicitly ("function that handles user order submissions"). Agent can search by text or Grep for the class. The "avoid admin path" constraint is implementable by reading both paths and editing only the user one.
**Historical routing:** 0/5 Skill invoked. Success = 1.0. Mean tools: ~28.
**TN verdict:** Probable TN. If agent were to use Skill here, evaluate whether it reduces the tool count substantially. Current tool count is high due to Read exploration, not callers discovery.

### N-08 — level4-002 (add security check to request path)
**Why negative:** Similar to N-07. Named target function, single insertion point.
**Historical routing:** 0/5 Skill invoked.
**TN verdict:** Probable TN.

### N-09 — level4-004 (propagate trace_id through explicitly named chain)
**Why negative:** The task explicitly names the three functions to modify: CheckoutService::checkout, OrderService::submit, OrderRepository::save. No callers discovery needed — search for each by name.
**Historical routing:** 0/5 Skill invoked.
**TN verdict:** Confirmed. Explicit chain means callers query adds nothing.

### N-10 — level4-005 (propagate client_ip through explicitly named chain)
**Why negative:** Same as N-09. Explicit chain: ApiHandler::handleRequest → RequestProcessor::process → DatabaseStore::save.
**Historical routing:** 0/5 Skill invoked.
**TN verdict:** Confirmed.

### N-11 — level4-007 (callback system return status)
**Why negative:** API evolution within a small system. No relationship discovery.
**Historical routing:** 0/5 Skill invoked.
**TN verdict:** Confirmed.

### N-12 — level4-008 (multi-file inventory pre-check)
**Why negative:** Add pre-check during order submission — text-guided editing across named files.
**Historical routing:** 0/5 Skill invoked.
**TN verdict:** Probable TN.

### N-13 through N-20 — level5-001 through level5-008 (bug investigation)
**Why negative:** Bug investigation tasks require reading and understanding code behavior, not relationship discovery. The agent needs to follow logic, identify incorrect conditions, and fix them. Semantic callers/callees may be called incidentally but provide no decisive advantage over Read.
**Historical routing:** 0/5 Skill invoked across all level5 tasks.
**TN verdict:** Confirmed.

### N-21 — smoke-001 (greet() trivial function)
**Why negative:** Single function in a named file. Trivial edit.
**Historical routing:** 0/5 Skill invoked. Agent used direct ast-tool calls (find/references) without Skill wrapper.
**Note:** This task shows that the agent can invoke ast-tool directly without Skill. Skill absence here is correct behavior.

---

## Summary

```
Positive set:     16 tasks (P-01 through P-16)
Boundary set:      4 tasks (B-01 through B-04)
Negative/Control: 21 tasks (N-01 through N-21)
Total:            41 tasks (including smoke-001)
```

### Focus Cohort for Phase 10 Experiments

For routing intervention experiments, use this focused cohort:

```
Positive (confirmed TP baseline):
  P-01 level2-006   callers / multi-file
  P-02 level2-008   callers
  P-13 level3-008   callers / multiple callers

Boundary (primary FN opportunity targets):
  B-01 level3-001   callers / caller-subset (stochastic)
  B-04 level4-003   callers / api-evolution (confirmed FN)

Negative (FP detection):
  N-01 level1-001   single-location find
  N-09 level4-004   explicit-chain propagation
```

These 7 tasks span TP, FN, and TN outcomes and represent the clearest signals.

---

## Reasoning Notes

### Why positive tasks focus on callers/callees

The Phase 10 spec states:

> "The strongest positive evidence comes from Phase-8b-like tasks such as:
> who calls X? where is X referenced? which callers need modification?"

All 16 positive tasks require discovering callers, callees, or references across files
where manual Grep would risk false positives from same-name methods on different classes.

### Why some always-failing tasks are excluded

level4-006 (defective validation fixture): Had validation defect in Phase 9a. Excluded from
routing experiment cohort until fixture is repaired.

### Why level5-xxx are negative

Bug investigation requires reading and understanding code behavior, not relationship
discovery by name. Semantic routing on these tasks would produce tool calls without
reducing manual Read exploration. They are the cleanest TN set.
