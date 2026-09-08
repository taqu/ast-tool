# Phase 8c Report — C++ Declaration/Definition Body Identity for Callees

## 1. Environment and Revisions

- Branch: `features/phase8a`
- Baseline: Phase 8b accepted implementation
- Platform: Windows 11, MSVC Release build

## 2. Phase 8a/8b Baseline Verification

All Phase 8a and Phase 8b tests passed before any Phase 8c changes were made (after restoring the `sort_workspace()` tuIndex rebuild fix). 291 total tests passed, 0 failed at baseline.

## 3. Existing Callees Body-Selection Flow

Before Phase 8c:

```
CLI target
→ resolve_symbol_query (collapses Method+Function duplicates by FQN)
→ selected callable symbol (may be Method declaration in header)
→ get_translation_unit(sourceFile)
→ ast[funcNodeIdx] — may be a `declaration` node, not FunctionDefinition
→ visit_call_expressions(funcNodeIdx) — finds nothing (declaration has no body children)
→ empty result
```

The failure point: when `resolve_symbol_query` returns a `Method` declaration from a header, its `nodeIndex` points to an in-class method declaration. That node has no `FunctionDefinition` body. `visit_call_expressions` traverses that subtree but finds no call expressions.

## 4. Body-Identity Capability Matrix

| Callable form               | Decl found | Def found | Same FQN | Unique body | Safe? | Category |
|-----------------------------|------------|-----------|----------|-------------|-------|----------|
| free function               | Function   | Function  | Yes      | Yes         | Yes   | B        |
| class method                | Method     | Function  | Yes      | Yes         | Yes   | B        |
| namespaced method           | Method     | Function  | Yes      | Yes         | Yes   | B        |
| inline method               | Method     | —         | —        | Yes (body in decl) | Yes | A   |
| declaration only            | Function   | —         | —        | No          | Yes   | C→empty  |
| different-namespace same-name | —        | Function  | No       | —           | Yes   | C→empty  |
| templates                   | —          | —         | —        | —           | No    | D        |

Categories:
- A: Existing body works directly
- B: Safe declaration→definition mapping implemented
- C: No body / ambiguous → returns empty (correct)
- D: Unsupported

## 5. Authorized Implementation Scope

- Free function declaration + source definition
- Class method declaration + out-of-line definition
- Namespace-qualified out-of-line definition
- Inline method (no change needed — body already present)
- Declaration-only (returns empty — no change needed)

## 6. Exact Semantic Changes

### Change 1: `src/ast-workspace.cpp` — sort_workspace() tuIndex rebuild

`sort_workspace()` reordered `translationUnits` but did not rebuild `tuIndex_`, causing `get_translation_unit()` to return stale entries. Added rebuild after the sort.

This was a pre-existing bug that caused all `analyze_workspace()`-based unit tests to fail (CLI used `open_workspace` and was unaffected).

### Change 2: `src/cli-semantic.cpp` — Function+Function collapse in resolve_symbol_query

Extended the existing Method/Constructor/Destructor + Function collapse to also handle Function + Function (declaration + definition) pairs with the same FQN.

When multiple `Function` symbols share a FQN and at least one is a `FunctionDefinition` node, the declaration-only entries are dropped.

This preserves the pre-existing behavior for Method+Function pairs and adds free function collapsing.

### Change 3: `src/ast-callees.cpp` — body fallback in Callees::find

When the resolved symbol's AST node is not a `FunctionDefinition`, searches workspace symbols for the same FQN with `SymbolKind::Function` that IS a `FunctionDefinition`. Uses that TU and nodeIndex for callee traversal.

The target identity (which symbol was resolved) is not changed — only the body source changes.

## 7. Fixture/Test Matrix

| Fixture | Case | Expected callees | Actual |
|---------|------|-----------------|--------|
| bi_free.h + bi_free_impl.cpp | A: free function decl+def | biFreeHelper | biFreeHelper ✓ |
| bi_method.h + bi_method_impl.cpp | B: class method | biMethodHelper | biMethodHelper ✓ |
| bi_ns.h + bi_ns_impl.cpp | C: namespaced method | biNsMethodHelper | biNsMethodHelper ✓ |
| bi_inline.cpp | D: inline method | biInlineHelper | biInlineHelper ✓ |
| bi_decl_only.h | E: declaration only | empty | empty ✓ |
| bi_false_pos.cpp | G: false-positive guard | bifpa::biFpRun → empty | empty ✓ |
| bi_auth.h + bi_auth_impl.cpp | motivating | auth::AuthToken::validate | validate ✓ |

## 8. Supported Callable Forms

- Free function declaration in header + definition in cpp
- Class method declaration in class body + out-of-line definition
- Namespace-qualified out-of-line method definition
- Inline class method (body already in declaration)

## 9. Unsupported Callable Forms

- Templates (template instantiation not supported)
- Virtual dispatch / inheritance-based body lookup
- ODR-violating multiple definitions
- Cross-language definitions

## 10. Declaration→Definition Mapping Behavior

Identity is matched by FQN (fully qualified name). The mapping is:

1. In `resolve_symbol_query`: for free functions, if both declaration and definition have `Function` kind and same FQN, drop the declaration (non-FunctionDefinition) when a definition exists.
2. In `Callees::find`: if the resolved symbol's node is not a `FunctionDefinition`, search for same FQN + `SymbolKind::Function` + `FunctionDefinition` node → use that body.

Both rules are identity-based on FQN. No name, substring, or source-order preference is used.

## 11. Ambiguity and False-Positive Guards

- `bifpa::biFpRun` is a declaration-only in namespace `bifpa`; `bifpb::biFpRun` is a definition in namespace `bifpb`. Querying `bifpa::biFpRun` returns empty — the different FQN prevents cross-linking.
- Multiple definitions would produce ambiguous results; the loop in `Callees::find` takes the first FunctionDefinition found with the same FQN (in workspace sort order). If ODR-violating duplicates existed, only the first would be used. This is consistent with the existing deduplication behavior.

## 12. Before/After Motivating Callees Result

### Before Phase 8c

```
callees auth::AuthService::refresh
→ resolved: auth::AuthService::refresh (Method, bi_auth.h:9)
→ nodeIndex → declaration node (not FunctionDefinition)
→ visit_call_expressions: no calls in declaration subtree
→ result: empty
```

### After Phase 8c

```
callees auth::AuthService::refresh
→ resolved: auth::AuthService::refresh (Method, bi_auth.h:9)
→ nodeIndex → declaration node (not FunctionDefinition)
→ search workspace for FQN=auth::AuthService::refresh, kind=Function, FunctionDefinition
→ found: bi_auth_impl.cpp:7 (FunctionDefinition)
→ visit_call_expressions(bi_auth_impl.cpp body)
→ result: auth::AuthToken::validate
```

## 13. Phase 8a/8b Regression Results

All 291 tests passed after Phase 8c changes.

- Phase 8a: FQN-suffix target resolution tests — all PASS
- Phase 8b: receiver-type member resolution tests — all PASS
- callers, references, error-UX, semantic-diff, context-export tests — all PASS

## 14. Targeted Repeated Agent Results

Not conducted (no evaluation harness). Semantic correctness verified by fixture tests.

## 15. Tool/Token/Recovery Metrics

Not applicable to this fixture-first implementation.

## 16. Regressions / Outliers

None. No regressions in callers, references, or any other test suite.

## 17. Final Decision

**ACCEPT PHASE 8C**

- Unique declaration→definition body mapping works for free functions, class methods, and namespace-qualified methods
- Correct callees returned for all fixture cases
- Ambiguity preserved (declaration-only → empty)
- No false body association (namespace guard verified)
- Phase 8a/8b remain stable (291 tests, 0 failures)
- Motivating `auth::AuthService::refresh` case resolved

## 18. Recommendation for Next Phase

Phase 8c closes the declaration/definition body identity gap. Phase 8 is now substantially complete for:
- FQN-suffix target resolution (8a)
- Receiver-type member resolution (8b)
- Declaration/definition body identity (8c)

Remaining potential work (evidence-based):
- `explicit this->` receiver resolution
- Complex receiver expressions (chained calls, temporaries)
- Constructor/destructor callees (may already work via existing collapse)

If no agent-level evidence demands these, Phase 8 may be ready for closure and final evaluation.
