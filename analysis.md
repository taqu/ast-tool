# Phase 7c — Semantic-Preserving `SKILL.md` Compression

## Recommendation

**ACCEPT FOR CONTROLLED EVALUATION**

The Phase 7c candidate was rebuilt from the exact Phase 5 `semantic-analysis/SKILL.md` at commit `e561505`, not from Phase 7a or Phase 7b. It compresses wording and documentation while retaining the Phase 5 decision boundaries for command selection, query scope, semantic ambiguity, error recovery, retry limits, grep fallback, and output cost.

This recommendation accepts the text as a candidate only. Body equivalence and the normal agent-level result remain unproven until the required controlled and 41-task evaluations run; Phase 5 remains the stable baseline.

## Textual Diff Summary

The Phase 5 frontmatter is unchanged. The body was transformed locally:

- The command table, selection flowchart, “When to Use,” and tool-selection repetitions became one direct routing table.
- Repeated command examples were reduced to representative targeted forms.
- Command syntax and behaviorally important flags remain, while exhaustive ordinary examples and repeated output descriptions were removed.
- Error reference, ambiguity recovery, recommended workflow, common mistakes, and best practices were merged without removing their distinct conditions.
- Explanatory rationale was removed where an explicit behavioral rule already determines the next action.
- Output-format prose became explicit plain-text/JSON/pretty and workspace-scope cost boundaries.

No routing command was generalized away. Phase 7c explicitly maps symbol/declaration → `search`, known-file AST node → `find`, usages → `references`, direct incoming calls → `callers`, direct outgoing calls → `callees`, and known-file inventory → `symbols`.

## Rule Classification Summary

| Category | Preserved Phase 5 content | Compression treatment |
|---|---|---|
| Routing contract | All direct command mappings | Consolidated into one table |
| Behavioral disambiguation | `search` vs `find`; references vs callers/callees; FQNs; directory roots; direct vs transitive calls; C++ declaration/definition ambiguity | Retained and shortened |
| Recovery contract | Diagnostic-directed correction; no unchanged retry; two-attempt ceiling; restricted help; ambiguity-specific fallback | Merged into recovery sections |
| Output/cost boundary | Semantic-first preference; scoped queries; plain text default; JSON only for processing; no pretty by default; cache broad JSON search | Retained explicitly |
| Behavioral examples | Targeted syntax and caller recovery sequence | Reduced to examples that clarify boundaries |
| Non-behavioral explanation | Purpose repetition, routine syntax demonstrations, repeated rationale, repeated output prose | Removed or subsumed |

## Rule-Equivalence Map

| Phase 5 rule | Category | Phase 7c equivalent | Transformation |
|---|---|---|---|
| Unknown declaration or symbol pattern uses `search` | Routing | Symbol/declaration across workspace → `search`; unknown file and filter boundary retained | Shortened |
| Known-file node lookup uses `find` | Routing/disambiguation | Text/type/ID/position in known file → `find` | Shortened |
| Cross-workspace `search` differs from file-scoped `find` | Disambiguation | Explicit table boundary plus syntax section | Merged |
| Usages use `references` | Routing | Every semantic usage → `references` | Shortened |
| Call sites to target use `callers` | Routing | Direct incoming calls → `callers` | Shortened |
| Calls inside target use `callees` | Routing | Direct outgoing calls → `callees` | Shortened |
| File symbol inventory uses `symbols` | Routing | Known-file symbols → `symbols` | Made explicit |
| Search filters are ANDed and can narrow kind/FQN/path | Disambiguation/cost | Same filter semantics and early-scoping rule | Shortened |
| `references` excludes declaration and empty success is valid | Disambiguation | Both conditions retained | Merged |
| Call targets must be callable; indirect/virtual calls are excluded | Disambiguation | Callable-target and direct-resolution limits retained | Shortened |
| `callers` is not transitive | Disambiguation | Direct-only rule; recurse only when transitive graph is requested | Shortened |
| Relationship commands require directory root | Recovery/scope | Directory requirement plus exact file-root diagnostic correction | Merged |
| FQN vs unqualified matching | Disambiguation | `::` matches FQNs; otherwise unqualified names | Retained |
| Flags are command-specific | Recovery | `find`, `search`, and ID flag boundaries remain explicit | Condensed |
| Not-found requires `search`, corrected FQN, one retry | Recovery | Same diagnostic, action, and retry boundary | Condensed |
| Never retry unchanged failure | Recovery | Same absolute rule | Shortened |
| Retry must meaningfully address the error | Recovery | FQN/root/command correction required; cosmetic variations rejected | Consolidated |
| Avoid routine help and syntax trial-and-error | Recovery/cost | Help only when syntax is absent and diagnostic insufficient | Clarified |
| Stop after two failures on same semantic command | Recovery | Same ceiling | Retained |
| Different namespaces require exact FQN | Ambiguity | Candidate search/selection and more-qualified FQN retry | Shortened |
| Same-FQN C++ declaration/definition cannot be fixed with more qualifiers | Ambiguity | Same exception and lack of relationship-command filters retained | Shortened |
| Narrow C++ ambiguity to implementation root when directories differ | Ambiguity/recovery | Same ordered action | Retained |
| If same directory or still ambiguous, use scoped Grep | Fallback | Same fallback condition | Retained |
| Same unqualified names require search and exact candidate FQN | Ambiguity | Same action | Shortened |
| Do not use grep for semantic declarations/callers | Cost/routing | Direct semantic route explicitly preferred; false textual matches named | Consolidated |
| Grep is for comments, strings, docs/config, or unresolved semantic fallback | Fallback | Same allowed cases and delayed fallback | Consolidated |
| Plain output preferred; JSON only for processing; pretty is costly | Output/cost | Same preference ordering and conditions | Shortened |
| Cache large JSON search rather than rerun | Output/cost | Same rule | Shortened |
| Scope queries and avoid workspace dumps | Output/cost | Narrow filters/root and targeted reads required | Consolidated |
| Ordinary examples, repeated purpose, and output-schema restatements | Non-behavioral | No separate equivalent needed | Deleted |

## Size Comparison

| Measure | Phase 5 | Phase 7c | Reduction |
|---|---:|---:|---:|
| Lines | 355* | 135 | 62.0% |
| Characters | 13,053* | 8,745 | 33.0% |
| UTF-8 bytes | 13,111* | 8,759 | 33.2% |
| Words | 1,950 | 1,302 | 33.2% |
| Approximate tokens (`characters / 4`) | 3,263 | 2,186 | 33.0% |

\*The Phase 5 measurements were streamed through PowerShell and include its newline representation; token and percentage comparisons are approximate. The Phase 7c token size is near the requested 2,200–2,600 guideline and was not forced lower.

## Semantic Review

For each behaviorally meaningful Phase 5 rule, the candidate still states:

- what command to choose and under which request condition;
- command-specific scope and prohibited flags;
- how semantic relationships differ;
- how to react to not-found, ambiguity, invalid root, and empty success;
- what constitutes a meaningful retry and when retries stop;
- when help and Grep are—and are not—allowed;
- how the special C++ same-FQN declaration/definition case changes recovery;
- how to constrain result and context cost.

No uncertain behavioral deletion was accepted. The additional `symbols` route required by the Phase 7c protected contract is explicit even though the Phase 5 command table did not document it as fully as the other commands.

## Validation

The skill-creator validator parses the file but reports `triggers` and `languages` as unsupported frontmatter keys. These keys are inherited verbatim from Phase 5 and are part of its discovery contract, so removing them would violate the instruction to preserve the baseline metadata. This is a validator-schema mismatch rather than a new Phase 7c defect.

`git diff --check` reports no whitespace errors. No AST Tool implementation, CLI behavior, evaluation task, harness, or metrics logic was changed.

## Evaluation Status

The required controlled comparison has not been run in this compression step. A valid controlled run must force `semantic-analysis` to load before exploration for both Phase 5 and Phase 7c; the existing evaluation runner has no option for that control, and its normal runs load the globally installed skill stochastically. Running a normal 41-task evaluation first would not answer body equivalence and would violate the required evaluation order.

A representative controlled set should cover at least `search`, `callers`, `references`, `callees`, `find`, symbol ambiguity, failure recovery, and grep fallback across approximately 15–20 tasks. Only after that paired cohort preserves targeted trajectories should the unchanged 41-task evaluation run.

## Separate Conclusions

### A. Body Semantic Equivalence

**Textually supported, behaviorally pending.** The rule inventory and equivalence map find no weakened Phase 5 decision boundary, but controlled Skill-loaded trajectories are still required to demonstrate equivalence.

### B. Full Agent-Level Behavior

**Pending.** No Phase 7c 41-task result exists yet, and the rejected Phase 7b run must not be reused as evidence about this new Phase 5-derived body.

## Final Recommendation

**ACCEPT FOR CONTROLLED EVALUATION**, not as the new stable baseline. The candidate achieves meaningful conservative compression with explicit rule equivalence. Final `ACCEPT`, `ACCEPT WITH CAVEATS`, `REVISE`, or `REJECT` must wait for the controlled comparison and, if it passes, the normal full evaluation.
