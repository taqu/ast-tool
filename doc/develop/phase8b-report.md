# Phase 8b — C++ Receiver-Type Member Relationship Resolution

## 1. Environment and revisions

Phase 8b ran on `features/phase8b` from accepted Phase 8a revision
`ede344d2cc1da7dc9e770910c2fefe110754a74e` on Windows, September 7, 2026 JST.
The implementation and evidence are working-tree changes on that revision.

The repository and installed `semantic-analysis` Skill files remained identical
at SHA-256 `96b07a6b89ae338f26d45fbfb31dd97d5b9c50efa39922103e8cb3e616807eaf`.
The recorded Phase 8a rebuilt binary was
`68d5154a86b6afb0142c3a12906a88b715accfb0a2f5a6fe63254d0de6ec16d1`.
The final Phase 8b agent and direct-replay binary was
`36904dd03fe0b586e9c9372f7fd83a56f99aa3983e4fb982504e233ce2f26ef7`.
The manifest checked this binary and both Skill copies around all ten final runs.

## 2. Phase 8a baseline verification

Phase 8a's target resolver, Skill, description, routing, command surface,
search, and find behavior were not modified. Its exact-FQN precedence, unique
FQN-suffix fallback, ambiguity/not-found handling, unqualified lookup, and
declaration/definition callable collapse remain on the same code path.

The compatibility tests compare partial and exact targets after receiver
resolution and pass. The final full Python evaluation passes 174 tests with two
skips; the final focused Phase 8a/8b set passes 25 tests.

## 3. Current member-resolution flow

Before Phase 8b, C++ calls followed this path:

```text
CallExpression
  → find_callee_identifier
  → retain only the member identifier text
  → IdentifierResolver(member name, call scope)
  → canonical callable or unresolved
  → callers / callees / references index
```

The AST retained `FieldExpression(receiver, member)`, including whether the
receiver was a simple identifier. Receiver identity and its declared type were
ignored after `find_callee_identifier`; unresolved results were dropped in each
of the three relationship commands.

Fields are workspace symbols whose `FieldDeclaration` AST retains the declared
type. Locals and parameters are omitted from workspace symbols, but their
`Declaration` and `ParameterDeclaration` nodes retain direct type identifiers,
pointer/reference declarators, and lexical scope IDs. This supported a narrow
AST-local extension without a general type system.

## 4. Typed receiver capability matrix

| Receiver form | Semantic receiver symbol | Direct static type | Canonical type | Member target | Class |
| --- | --- | --- | --- | --- | --- |
| object field | yes | field declaration | lexical exact lookup | if unique | A |
| pointer field | yes | field declaration | lexical exact lookup | if unique | A |
| local object | no | local declaration AST | lexical exact lookup | if unique | B |
| reference parameter | no | parameter AST | lexical exact lookup | if unique | B |
| pointer parameter/local | no | declaration AST | lexical exact lookup | if unique | B |
| implicit bare member | containing class scope | existing lexical path | existing resolver | existing behavior | A/existing |
| explicit `this->` | no declared receiver | unavailable to new rule | no | no | C |
| call/cast/chained expression | receiver retained | unavailable | no | no | C |

Class A uses existing semantic data. Class B is one bounded extension: inspect
only a directly written, visible declaration in the enclosing function and use
its explicit type. No assignment flow, deduction, alias expansion, or expression
inference occurs.

## 5. Authorized implementation scope

The implemented scope is ordinary non-template member calls with a simple
identifier receiver whose explicit declared type is available from:

- an enclosing class field;
- the nearest visible local declaration; or
- a function parameter declaration, including direct pointer/reference syntax.

The type must resolve by qualified identity or lexical namespace/class prefixes,
and the receiver class must declare exactly one callable with that member name.

## 6. Exact semantic change

`resolve_relationship_identifier` is a shared boundary used by callers,
callees, and references. For a member-name child of a call expression it:

1. extracts the simple receiver identifier;
2. prefers the nearest visible local or parameter declaration;
3. otherwise requires a field owned by the enclosing class;
4. reads the declaration's explicit type identifier;
5. resolves that type from inner to outer lexical prefixes, then global scope;
6. confirms the class body declares exactly one callable with the member name;
7. collects only the exact `TypeFQN::member` callable and applies the existing
   Method-versus-out-of-line-Function duplicate collapse;
8. returns unresolved for missing or ambiguous information.

Ordinary non-member identifiers still call `IdentifierResolver` unchanged.
There is no global member-name fallback, first match, source-order preference,
or arbitrary namespace suffix match.

## 7. Fixture/test matrix

Fixtures were written before production changes. The Phase 8a binary failed the
four initial positive field/cross-command assertions while the safe-negative
cases remained unresolved. The final matrix covers field objects, field
pointers, local objects, reference parameters, pointer parameters, unrelated
types, same names across namespaces, local/field shadowing, sibling lexical
scopes, unknown expressions, and overloads.

The normalized matrix contains 16 command cases. Missing relationships: zero.
Unexpected/forbidden relationships: zero. The live integration suite passes all
focused tests.

## 8. Supported receiver categories

Supported receivers are directly declared object fields, pointer fields, local
objects, local pointers, reference parameters, and pointer parameters. Locals
and parameters must be visible at the call and must have a direct type identifier.
Nearest lexical scope wins, so a local correctly shadows a same-named field and
a declaration in a sibling block is ignored.

## 9. Unsupported receiver categories

The resolver intentionally leaves these unresolved:

- return-value, call, cast, subscript, chained, and other complex receivers;
- explicit `this->` receiver typing added by this phase;
- `auto`, `decltype`, aliases requiring expansion, and inferred types;
- inherited-member lookup and virtual target expansion;
- templates and dependent lookup;
- overloaded member names, even when argument count appears suggestive.

Bare implicit member calls retain the prior lexical resolver behavior.

## 10. Cross-command results

`callers typed::Validator::validate` returns the five supported fixture callers:
object field, pointer field, local object, reference parameter, and pointer
parameter. `references` returns exactly their five call sites. `callees` returns
`typed::Validator::validate` for each of those enclosing functions. The
unrelated `typed::OtherValidator::validate` and the left/right namespace types
remain separated.

## 11. False-positive guards

All negative guards pass:

- unrelated receiver types with the same method name do not cross-link;
- `left::Validator` and `right::Validator` stay separate;
- a local shadows a same-named field using its own type;
- an out-of-scope sibling declaration is ignored;
- `makeValidator().validate()` is not guessed;
- an overloaded `validate` remains unresolved.

Observed false-positive count is zero across the focused fixture matrix and the
two motivating repositories.

## 12. Before/after motivating relationships

| Command/target | Phase 7f | Phase 8b | Missing | Unexpected |
| --- | --- | --- | ---: | ---: |
| callers `auth::AuthToken::validate` | empty | four exact callers | 0 | 0 |
| references `auth::AuthToken::validate` | empty | four exact sites | 0 | 0 |
| callers `service::ValidationService::validate` | empty | one exact caller | 0 | 0 |
| references `service::ValidationService::validate` | empty | one exact site | 0 | 0 |

Every after-query was repeated five times with identical output. No
`AuthService::validate`, `DatabaseStore::validate`, or `CronJob::validate` site
entered the target sets.

`callees auth::AuthService::refresh` still selects a header declaration without
a body and returns empty. That is a declaration/definition body-identity issue,
not receiver typing, and remains outside Phase 8b.

## 13. Targeted repeated agent results

Five final-binary semantic runs per task invoked the unchanged Skill first.

| Task | Phase 7f trajectory | Phase 8b trajectories | Relationship usefulness | Raw validation |
| --- | --- | --- | --- | ---: |
| level2-004 | search→callers(empty)→references(empty) | search→callers ×5 | all four files identified directly | 5/5 |
| level4-006 | search→callers(empty)→references(empty) | search→callers ×2; search→callers→search ×2; search→callers→references ×1 | sole caller identified; exact three edits in 5/5 | 0/5* |

`*` The committed `ApiHandler` header lacks the two fields used by its unchanged
implementation. The validator therefore fails whole-project compilation in the
baseline and every Phase 8b run. Raw sessions show exactly the intended header,
definition, and caller edits in all five runs, with unrelated methods untouched.

## 14. Tool, token, and recovery metrics

For `level2-004`, mean tools fell 12→10, AST calls 3→2, tokens
6,276→5,543.8, and elapsed time 60.47→45.18 seconds. The now-unneeded references
fallback disappeared in all five runs. Reads stayed at three and correctness
stayed 100%.

For `level4-006`, mean tools rose 9→10.2, AST calls fell 3→2.6, tokens rose
2,957→3,504, and elapsed rose 51.82→54.17 seconds. All runs used the useful
populated callers answer; extra searches/references reflect strategy variance.

Across both equally weighted tasks, tools fell 0.40, AST calls fell 0.70,
references fell 0.90, tokens fell 92.6, and elapsed fell 6.47 seconds. AST
failures, retries, help, Find, and Grep were zero throughout; recovery mean/max
are therefore not applicable. These agent deltas are descriptive because the
historical baseline has one run per task.

## 15. Overload and ambiguity assessment

The receiver type may be known while member identity remains ambiguous. Phase
8b counts declarations with the requested member name in the resolved class;
anything other than one fails closed. It does not inspect argument types or
claim overload resolution. Multiple canonical type identities, duplicate
definitions, missing declarations, and conflicting same-scope receiver
declarations also remain unresolved.

## 16. Regressions and outliers

- The full Python evaluation passes 174 tests with two skips; the final focused
  Phase 8a/8b set passes 25 tests.
- Six already-correct relationship queries were replayed five times each and
  returned stable exact cardinality, including nested direct callees and the
  out-of-line local-object caller that exposed an early regression.
- The monolithic C++ runner remains globally red and cache/order sensitive.
  Across three clean-cache runs, Phase 8a produced 11–16 failures and Phase 8b
  produced 14–15. Some assertion membership changed between builds and runs;
  the corresponding current CLI queries pass repeatedly. The raw comparison is
  retained rather than presenting it as a clean suite.
- The first agent cohort preceded the final local/scope guards. It is preserved
  under `agent-pilot` and excluded from final aggregates.
- `level4-006` has a pre-existing validator fixture compilation defect described
  above.

## 17. Final decision

**ACCEPT WITH CAVEATS.** Receiver-constrained resolution is bounded and shared,
all four requested ordinary receiver categories resolve from explicit existing
AST information, cross-command results agree, overloads and unknown receivers
fail closed, and the motivating caller/reference sets move from empty to exact
with zero observed false positives. Agent-level usefulness is clear in both
tasks, with one unchanged validator reaching 5/5 and the other producing the
correct edit set 5/5 despite its independent compile defect.

The caveats are direct-declaration-only coverage, no overload or complex
receiver support, a one-run historical baseline per task, and the existing
unstable monolithic C++ runner.

## 18. Recommendation for the next Phase 8 step

Do not expand receiver inference automatically. The repeated remaining failure
supports a separate investigation of declaration/definition body identity for
`callees`: a selected method declaration should map to its same-FQN out-of-line
definition before body traversal. That phase should begin with focused body
identity fixtures and keep overloads, explicit `this`, and complex receiver
expressions out of scope until repeated task evidence justifies them.

Raw measurements are in `evaluation/phase8b/measurements.json`, readable tables
in `evaluation/phase8b/tables.md`, and the final run manifest and traces under
`evaluation/phase8b/agent`.
