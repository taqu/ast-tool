# Phase 7f — Semantic Routing Value and Toolset Cost Audit

## 1. Environment and baseline

The audit ran on `features/phase7f` at revision
`9b88d4a0b838e678ad0c2bcdd8465ddaafc99278` on Windows, September 6, 2026
JST. The accepted repository and installed Phase 7d `semantic-analysis` Skill
both had SHA-256
`96b07a6b89ae338f26d45fbfb31dd97d5b9c50efa39922103e8cb3e616807eaf`.
The installed AST executable had SHA-256
`d52918518f2b5dad59d0634a024e28bac7320880041dbfc49e65ce97c7f8029e`.
The hashes still matched after the audit. No Skill, command behavior, fixture, or
evaluation result was changed.

This is an observational reuse of 59 preserved runs: one forced semantic Phase
7d run and one historical no-Skill run for each selected task, plus all applicable
natural Phase 7e repetitions. No new agent run was justified: every task already
had both routing conditions, `level3-007` had 7 semantic and 5 manual runs, and
the two variable relationship tasks had 6 semantic runs each. Single-pair tasks
remain weak evidence and are labeled accordingly. Historical and controlled
runs are descriptive pairs because routing was not randomized.

## 2. Probe cohort and conditions

The 12-task cohort covers direct and ambiguous lookup, structural lookup,
callers, callees, references, multi-level relationships, distributed workflow,
API evolution, broad modification, and recovery.

| Task | Principal probe | Semantic runs | Manual/direct runs | Evidence |
| --- | --- | ---: | ---: | --- |
| level1-006 | ambiguous direct lookup | 1 | 1 | weak |
| level2-001 | structural lookup | 1 | 1 | weak |
| level2-004 | callers/references | 1 | 1 | weak alone; part of repeated limitation |
| level2-005 | callees | 1 | 1 | weak alone; part of repeated limitation |
| level2-006 | references/callers | 6 | 1 | moderate |
| level2-008 | ambiguous relationship target | 6 | 1 | moderate |
| level3-005 | multi-level callers | 1 | 1 | weak |
| level3-007 | distributed workflow | 7 | 5 | strong |
| level3-008 | indirect callers | 6 | 1 | moderate |
| level4-003 | API evolution | 1 | 6 | moderate but unbalanced |
| level4-006 | broad modification | 1 | 1 | inconclusive; both routes failed validation |
| smoke-001 | prescribed invalid forms/recovery | 1 | 6 | recovery control, not a routing estimate |

Raw measurements are separate from interpretation in
[`evaluation/phase7f/tables.md`](evaluation/phase7f/tables.md). The complete run
and per-call data are documented in
[`evaluation/phase7f/README.md`](evaluation/phase7f/README.md).

## 3. Task-level semantic value

All successful paired tasks had zero success delta. Semantic value therefore
comes from precision and exploration cost, while the elapsed penalty is material
in most cases.

| Task | Tool delta | Token delta | Elapsed delta | Read delta | Class | Reasoning |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| level1-006 | +1.00 | +501 | +16.88 | 0.00 | E | Two searches resolved ambiguity, but one manual lookup reached the same correct change more cheaply. |
| level2-001 | +1.00 | +669 | +22.56 | 0.00 | D | Search plus structural find was precise, but only replaced two Greps and did not reduce Reads. |
| level2-004 | +3.00 | +3,028 | +30.39 | 0.00 | E | Both relationship results were empty; the route paid semantic cost without useful relationship information. |
| level2-005 | +4.00 | +316 | +37.98 | +1.00 | E | Two empty callee calls led to find and source inspection; manual exploration was cheaper. |
| level2-006 | +2.00 | -1,240 | +19.56 | 0.00 | B | Six semantic runs consistently removed Grep and reduced tokens, with a two-call and latency cost. |
| level2-008 | +0.17 | -303 | +17.70 | -1.00 | B | Relationship queries reduced Reads and Greps; two of six runs paid short-name recovery overhead. |
| level3-005 | 0.00 | +77 | +16.38 | 0.00 | B | Exact callers replaced three Greps at almost equal tool and token cost, but one pair is weak evidence. |
| level3-007 | +0.17 | +3,129 | +16.25 | -3.91 | D | Strong same-task evidence: semantic routing reduced Reads but used 5.43 AST calls per run and more tokens/time with unchanged correctness. |
| level3-008 | -15.00 | -3,526 | -10.66 | -17.83 | A | Six semantic routes sharply reduced broad reading, tools, tokens, and time; two runs still paid name-resolution recovery. |
| level4-003 | -8.50 | -6,578 | +22.95 | -8.67 | B | Semantic relationships greatly reduced broad exploration and tokens, while AST latency made the route slower. |
| level4-006 | -6.00 | +48 | -15.02 | -5.00 | F | Both single runs failed validation and the relationship commands returned no information. |
| smoke-001 | -6.50 | -3,289 | -51.76 | -1.83 | F | The prompt prescribes obsolete/invalid forms; direct-AST recovery dominates and does not estimate ordinary routing value. |

Relationship-heavy, multi-level tasks benefit most when the relationship result
is populated. Direct and structural lookup tasks in these small fixtures do not
show agent-level savings. Distributed multi-target lookup is semantically useful
but costly: the agent asks several legitimate identity questions and search
latency accumulates.

## 4. Per-call annotation and command assessment

Every one of 154 AST calls records command, target, effective success,
information gained, reason, next action, A-F classification, and hindsight
assessment. “Effective success” treats CLI errors and usage output as failures
even where a shell event exited successfully. The deterministic annotations were
reviewed against trajectories; they are evidence-guided judgments rather than
independent ground truth.

The totals are A necessary 54, B identity/resolution 28, C relationship retry or
empty-result overhead 7, D redundant structural lookup 0, E context bridge 16,
and F recovery/error handling 49. Direct-AST `smoke-001` runs contribute 42 of
the 49 F calls. Semantic routes alone contain A 51, B 28, C 7, E 8, and F 7.

| Command | Useful cases | Repeated overhead | Frequency/cost | Assessment |
| --- | --- | --- | --- | --- |
| search | Exact identity and ambiguity resolution | refinement and recovery before relationships | 56 calls, 563.18 s; 27 A, 28 B | Unique and central, but the largest latency source. |
| find | AST structure and node detail | source-context bridge before Read | 33 calls, 196.18 s; 3 A, 16 E, 14 F | Useful structurally; no remaining clearly redundant location-only find was observed. |
| callers | Direct relationship discovery | under-qualified retry and empty member-call results | 27 calls, 147.05 s; 20 A, 3 C, 4 F | High value when populated; target and member resolution are the main limits. |
| callees | Intended outgoing relationship discovery | repeated empty result for a body with member calls | 2 calls, 8.83 s; both C | Represented too narrowly to judge generally; current member resolution failed this case. |
| references | Direct reference discovery | invalid smoke syntax and empty member-call results | 30 calls, 129.60 s; 3 A, 2 C, 25 F | Ordinary evidence is sparse; smoke failures must not drive product conclusions. |
| symbols | Workspace structural inventory | none established | 1 A call, 3.91 s | Insufficient evidence beyond one useful smoke call. |

## 5. Repeated patterns and command boundaries

### Relationship target resolution

Four semantic trajectories across two tasks used this exact recovery:

```text
callers short-qualified-name (not found)
-> search unqualified member
-> callers full FQN (success)
```

They occurred twice for `AuthToken::expire` and twice for `DataStore::save`.
The 12 calls took 64.59 seconds; failed calls plus resolution searches accounted
for 47.27 seconds. Search uniquely supplied the namespace prefix, but each input
already contained a class and member and was unambiguous by suffix in its
workspace.

Source inspection explains the boundary. `resolve_symbol_query` treats every
query containing `::` as an exact FQN and never attempts a unique suffix match.
The shared wrapper applies this resolver to `callers`, `callees`, and
`references`. This is agent protocol overhead caused by existing command
semantics.

### Search followed by find

There were four sequences: one in `level2-001` and three in `level3-007`.
Manual review found no D call. The first located an implementation after search
returned several declarations; the others asked for implementation structure in
specific files. The Phase 7d guidance appears to have removed simple
location-only duplication. Removing or merging `find` is not supported.

### Declaration versus definition

Of 56 search calls, 39 outputs showed declaration-only evidence, 5 contained
both declaration and definition, 3 showed a definition only, and 9 provided no
role evidence. Search reports kind, FQN, file, line, and column, but does not
explicitly label declaration/definition or connect them as one canonical
identity. This may explain some implementation rediscovery, but only four
search-to-find sequences occurred and each was defensible. Explicit role or
canonical-identity metadata remains a lower-priority hypothesis.

### Find followed by Read

Sixteen find calls served as context bridges, eight on semantic routes and eight
in direct-AST smoke routes. Reads were needed for editable source and surrounding
implementation context. Returning larger source excerpts could merely transfer
that context cost into AST output. The evidence does not justify changing find
output.

### Repeated relationship calls

The two consecutive `callees` calls in `level2-005` repeated the same target and
both returned empty. Other ordinary consecutive relationship calls queried
different targets (`save` then `purge`, or `charge` then `authorize`) and were
correct task decomposition. Repeated smoke calls mainly recovered from
prescribed invalid syntax.

## 6. Semantic API limitations

The narrow target-resolution limitation is directly isolated in
`src/cli-semantic.cpp`: a partially qualified query cannot match a unique
namespace-qualified FQN. It causes a visible error, then a reliable three-call
recovery.

A broader limitation also repeats across three tasks. Fully qualified
`callers`/`references` for `auth::AuthToken::validate` and
`service::ValidationService::validate` return no results, while the fixture
source contains `token_.validate(...)` at three sites and
`validator_.validate(...)` at one site. `callees auth::AuthService::refresh`
also returns empty although its body contains member calls. `Callees::find`
passes the member identifier text to `IdentifierResolver` and drops unresolved
calls; the observed behavior is consistent with failure to infer the receiver
field's type. That implementation diagnosis is an inference from source and
live reproduction. The user-visible failure is established.

This second limitation is more damaging because an empty result looks
successful and can hide relationships. It spans callers, references, and
callees, but fixing C++ member-call resolution is broader and riskier than the
target resolver change. It should receive its own fixture-level design and
experiment.

## 7. Candidate improvements ranked by evidence

1. **Let relationship commands resolve a uniquely matching FQN suffix.** Four
   recoveries across two tasks support this. Try exact FQN first; if absent,
   suffix-match `::query`, apply the existing declaration/definition collapse,
   and accept only one canonical symbol. Preserve ambiguity when multiple
   namespaces match. Evidence is moderate and the change is narrow.
2. **Resolve member calls through receiver types.** Empty relationships repeat
   across three tasks and live fixtures. The value is potentially higher, but
   the implementation surface and overload/type risks are larger. Evidence is
   moderate across tasks; causal agent-level evidence remains thin.
3. **Expose declaration/definition roles and canonical identity in search.** The
   output is declaration-heavy, but downstream extra calls were uncommon and
   defensible. Evidence is weak.

## 8. Rejected hypotheses

- Add a combined search-and-relationship command: the existing relationship
  commands can absorb safe identity resolution without adding a command.
- Add a multi-symbol batch command for `level3-007`: its three targets are
  separate semantic questions, and the evidence shows cost rather than a missing
  operation.
- Remove or discourage find: all four remaining search-to-find cases requested
  implementation structure or resolved declaration/definition placement.
- Embed source bodies in find by default: immediate Reads usually obtained
  editable surrounding context; larger default output has no demonstrated net
  saving.
- Change Skill text or routing metadata: Phase 7f observed tool boundaries and
  retained the accepted baseline; no evidence isolates a routing-text defect.

## 9. Decision and next experiment

**EXISTING COMMAND SEMANTICS SHOULD BE IMPROVED.** Semantic routing clearly
helps relationship-heavy tasks when results are populated, but the shared exact
FQN rule repeatedly forces a recoverable three-call protocol. No new command is
justified.

The next experiment should change exactly one behavior: add unique FQN-suffix
fallback to the existing shared relationship target resolver. First add tests
for exact-match precedence, one unique suffix, two ambiguous namespace suffixes,
and declaration/definition duplicates. Then replay the four observed recovery
traces, run relationship-command guards, and repeat `level2-008` and
`level3-008` under semantic routing. The causal success criterion is one
relationship call per target with unchanged answers and ambiguity behavior,
followed by lower AST-call and elapsed cost at the agent level. Member receiver
resolution should remain unchanged during that experiment.

Audit reproduction and regression validation passed: `python
evaluation/phase7f_analyze.py` regenerated the datasets, and `python -m pytest
evaluation --basetemp=evaluation/.pytest-phase7f` completed with 149 passed and
2 skipped. `git diff --check` also passed.
