# Phase 8a — Unique FQN-Suffix Relationship Resolution

## 1. Environment and revision

Phase 8a ran on `features/phase8a` from committed revision
`ccc1fbb650bf058aa11602134d4e4fa1795cb98e` on Windows, September 7, 2026 JST.
The implementation and evidence are working-tree changes on that revision.

The accepted repository and installed `semantic-analysis` Skill files matched
SHA-256 `96b07a6b89ae338f26d45fbfb31dd97d5b9c50efa39922103e8cb3e616807eaf`
before and after every agent run. The previously installed baseline AST binary
was `d52918518f2b5dad59d0634a024e28bac7320880041dbfc49e65ce97c7f8029e`.
Agent validation used the workspace Release binary with SHA-256
`371978f1c95de9ce8b442af5455ca875dec51953e0973b9fce150de5efbe6387`.
The final source-equivalent rebuild has SHA-256
`68d5154a86b6afb0142c3a12906a88b715accfb0a2f5a6fe63254d0de6ec16d1`;
MSVC link-time code generation did not produce byte-identical rebuilds, so the
agent manifest identifies the exact evaluated executable.

## 2. Baseline verification

No Skill text, metadata, or registration changed. Agent prompts and validators
were unchanged. The controlled condition forced the accepted Skill as the first
action and prepended the workspace binary directory to the child process PATH.
All ten fresh runs invoked the Skill once and first.

The Phase 7f semantic baselines comprise six runs per target task. The observed
resolver recovery occurred twice in `level2-008` and twice in `level3-008`:

```text
callers AuthToken::expire       -> not found
search --name expire            -> auth::AuthToken::expire
callers auth::AuthToken::expire -> success
```

and equivalently for `DataStore::save`.

## 3. Existing resolver behavior

`callers`, `callees`, and `references` all call `cli::with_resolved_symbol`,
which loads the workspace and calls `cli::resolve_symbol_query`. Before Phase
8a, that resolver used this decision:

```text
query contains "::" -> exact FQN equality only
otherwise           -> unqualified symbol-name equality
```

It then collapsed a C++ out-of-line `Function` entry when the same FQN also had
a properly classified Method, Constructor, or Destructor entry. Zero candidates
produced the existing not-found error; multiple candidates produced the existing
ambiguity error. This common boundary is the only production behavior changed.

## 4. Implementation change

For a query containing `::`, `resolve_symbol_query` now:

1. collects exact FQN matches;
2. performs no fallback when at least one exact match exists;
3. otherwise collects FQNs ending with `"::" + query`;
4. runs the existing declaration/definition collapse on that result;
5. returns zero, one, or multiple candidates to the unchanged shared wrapper.

The explicit `::` prefix enforces a qualified-name boundary, so
`AuthToken::expire` cannot match `SomeAuthToken::expire`. Unqualified queries
remain on the original name-equality path. No command-specific resolver, new
command, output field, member receiver inference, or relationship-discovery
logic was added.

## 5. Resolver-level test matrix

Tests were written before the implementation and failed on the committed
resolver. The implemented resolver passes every focused case.

| Case | Workspace/query condition | Result |
| --- | --- | --- |
| A | exact `auth::AuthToken::expire` | exact candidate selected |
| B | unique suffix `AuthToken::expire` | canonical `auth::AuthToken::expire` selected |
| C | `auth::...` and `legacy::...` suffix candidates | both retained; ambiguity preserved |
| D | Method declaration plus same-FQN Function definition | one Method candidate |
| E | `MissingToken::expire` | zero candidates; not found preserved |
| F | exact `AuthToken::expire` plus `auth::AuthToken::expire` | exact candidate selected |
| Boundary | only `SomeAuthToken::expire` exists | no match |
| Unqualified | two symbols named `expire` | two candidates, as before |

The test additions also verify that the pre-existing canonicalization path is
reused after suffix collection rather than duplicated.

## 6. Relationship-command regression tests

Each relationship command has a namespaced fixture and compares a partial FQN
with the fully qualified form:

| Command | Partial query | Canonical target | Result |
| --- | --- | --- | --- |
| callers | `inner::suffixCalTarget` | `suffixcal::inner::suffixCalTarget` | outputs equal and nonempty |
| callees | `inner::suffixCeSource` | `suffixce::inner::suffixCeSource` | outputs equal and nonempty |
| references | `inner::suffixRefTarget` | `suffixref::inner::suffixRefTarget` | outputs equal and nonempty |

The Python integration guards exercise the built CLI and pass 7/7, including
the two Phase 7f replays, not-found behavior, and unchanged member receiver
behavior. The focused C++ resolver/command cases all pass in the monolithic
runner.

The monolithic runner has 15 existing Callees/context-export assertion failures
under both resolver versions on this checkout. A controlled rebuild with the
committed resolver produced those same 15 failures plus the eight expected new
suffix-test assertion failures; the Phase 8a build produced the same 15 and no
new non-Phase-8a failure. These existing failures prevent claiming a globally
clean C++ suite, but the comparison shows no added failure outside the isolated
tests.

## 7. Before/after recovery trajectories

| Target | Historical occurrences | Before | After direct replay | Answer equivalence |
| --- | ---: | --- | --- | --- |
| `AuthToken::expire` | 2 | failed callers → search → exact callers | one partial-FQN callers call | exact two-caller set |
| `DataStore::save` | 2 | failed callers → search → exact callers | one partial-FQN callers call | exact one-caller set |

Each partial and exact form was replayed five times. All 20 invocations returned
success, and every partial result exactly equaled its corresponding exact-FQN
result. The causal command cost changed from three AST calls, one failure, and
one retry to one successful AST call.

## 8. Targeted repeated agent results

Five controlled semantic runs per task all passed their unchanged validators.

| Task | After trajectories | Failures | Retries | Success |
| --- | --- | ---: | ---: | ---: |
| level2-008 | callers ×3; search→callers ×2 | 0 | 0 | 5/5 |
| level3-008 | search→callers ×1; callers→callers ×2; search→search→callers ×1; search→callers→callers ×1 | 0 | 0 | 5/5 |

No run used `callers→search→callers`. Searches that remain precede the first
relationship command and reflect the agent's chosen identity/workflow
decomposition, not recovery from rejected partial qualification. Some Bash
events combine two caller invocations; raw data therefore records both the
established AST-bearing tool-event metric and executable invocation count.

## 9. Aggregate metrics

Deltas below are Phase 8a means minus Phase 7f semantic means. Routing was
controlled but model sampling was not, so agent-level differences remain
descriptive rather than randomized causal estimates.

| Task | Tools | Traced AST | Failures | Retries | Searches | Tokens | Elapsed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| level2-008 | -0.77 | -0.77 | -0.33 | -0.33 | -0.43 | -42.57 | -4.59 s |
| level3-008 | -0.40 | +0.07 | -0.33 | -0.33 | -0.20 | -445.00 | +0.99 s |
| equal-task aggregate | -0.58 | -0.35 | -0.33 | -0.33 | -0.32 | -243.78 | -1.80 s |

Correctness stayed at 100%. Help, Grep, and Glob remained zero. The primary
causal metrics improved cleanly: target-resolution failures and retries fell to
zero, and the exact recovery pattern disappeared. Total tools, tokens, and the
equal-task elapsed mean improved directionally. `level3-008` traced AST calls
and elapsed time were effectively flat because runs varied in whether they
searched for audit logging and whether two caller commands shared one Bash
event.

Raw run measurements and direct outputs are in
`evaluation/phase8a/measurements.json`; readable raw tables are in
`evaluation/phase8a/tables.md`.

## 10. Exact-match precedence assessment

Exact matches are collected first, and suffix fallback is conditional on the
exact set being empty. The exact-plus-suffix test selects
`AuthToken::expire`, not `auth::AuthToken::expire`. Existing exact Phase 7f
replays return the same relationship sets as partial queries. Exact precedence
is preserved.

## 11. Ambiguity assessment

The suffix fallback returns every boundary-valid suffix candidate. The shared
wrapper retains its existing multiple-candidate error behavior. A workspace
containing `auth::AuthToken::expire` and `legacy::AuthToken::expire` yields two
candidates and does not choose by order. Arbitrary substring, distance, and
first-match behavior are absent.

## 12. Declaration/definition canonicalization assessment

Suffix candidates flow through the existing callable duplicate collapse. A
Method declaration plus a Function-classified out-of-line definition with the
same canonical FQN resolves to the Method alone. The exact path uses the same
collapse. Phase 8a introduces no second canonicalization rule.

## 13. Regressions and outliers

- All ten agent validations passed, with no timeout or process failure.
- All partial/exact direct answer comparisons passed.
- The full Python evaluation suite passed: 158 passed, 2 skipped.
- The controlled C++ before/after comparison added no unrelated failure; its 15
  shared Callees/context-export failures remain a repository limitation.
- `level3-008` elapsed time increased by 0.99 seconds and traced AST events by
  0.07 per run. Its failures and retries still fell to zero, and tokens fell by
  445; this is treated as sampling/strategy variance.
- `callers AuthToken::validate` still resolves the target but returns no
  callers, confirming that receiver-type relationship discovery did not change.

## 14. Final decision

**ACCEPT WITH CAVEATS.** Unique FQN-suffix resolution is deterministic, bounded,
shared across all three relationship commands, and preserves exact precedence,
ambiguity, not-found behavior, unqualified lookup, and callable duplicate
collapse. The four previously observed recoveries are eliminated, relationship
answers are unchanged, and agent-level failures/retries fall to zero.

The caveats are the small stochastic cohort, nearly flat `level3-008` time/AST
cost, non-reproducible binary hashes across MSVC LTCG rebuilds, and the 15
existing monolithic-runner relationship assertions that remain failing under
both resolver versions.

## 15. Recommendation for Phase 8b

Phase 8a is stable enough to open a separate Phase 8b investigation, without
automatically implementing it. The receiver-type limitation remains live and
important: `token_.validate(...)` and `validator_.validate(...)` are still not
reported as callers/references, and Phase 7f observed related empty results
across three tasks. Phase 8b should begin with typed receiver-resolution fixtures
and distinguish fields, locals, references, overloads, and unresolved receivers
before changing relationship discovery. It must use a separate baseline and
must not alter the accepted Phase 8a target resolver.
