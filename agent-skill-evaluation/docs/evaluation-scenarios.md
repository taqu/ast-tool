# Evaluation Scenarios

This document describes seven realistic evaluation scenarios for testing AI agent skills against the `agent-skill-evaluation` codebase. Each scenario specifies the user's natural-language request, the expected agent skill to invoke, and the type of analysis required.

---

## Scenario 1: Locate the definition of `Lexer::next`

**Task**
"Where is `Lexer::next` defined? Show me the function signature and the file and line number."

**Expected Skill**
`workspace-analysis`

**Expected Analysis**
The agent searches the workspace for the symbol `Lexer::next` by name or fully qualified name. It finds the declaration in `include/parser/lexer.hpp` (the `next()` member function of class `Lexer`) and the definition in `src/parser/lexer.cpp`. The agent reports both locations with file paths and line numbers, and may display the function signature `Token next()`.

---

## Scenario 2: Find every caller of `Scope::lookup`

**Task**
"List every place in the codebase that calls `Scope::lookup`. For each call site, show the file, line number, and surrounding context."

**Expected Skill**
`workspace-analysis`

**Expected Analysis**
The agent searches for references to `lookup` across all source files. It identifies call sites in `src/semantic/analyzer.cpp` (inside `visitCallExpr`, which calls `currentScope_->lookup(node.callee())`) and in `src/semantic/symbol.cpp` (recursive calls within the `lookup` overloads themselves). The agent presents each result with file path, line, and a snippet of the surrounding code.

---

## Scenario 3: Display the AST surrounding the `if` statement in parser.cpp

**Task**
"Show me the AST structure of the `parseIfStmt` function in `src/parser/parser.cpp`. I want to see the tree of nodes it creates."

**Expected Skill**
`ast-inspection`

**Expected Analysis**
The agent uses the AST inspection skill to dump the AST of `src/parser/parser.cpp`, then navigates to the `parseIfStmt` function node. It shows the function's children in tree form: the calls to `expect`, `advance`, the construction of the `IfStmt` node, and the child-appending loop. The agent highlights the `std::make_shared<ast::IfStmt>` call expression node and its position in the overall function body.

---

## Scenario 4: Export semantic context of the semantic module

**Task**
"Export a structured context package covering all files in the `src/semantic/` and `include/semantic/` directories, suitable for a code-review prompt."

**Expected Skill**
`context-export`

**Expected Analysis**
The agent assembles a multi-file context export containing:
- The AST outlines of `include/semantic/symbol.hpp`, `include/semantic/analyzer.hpp`, `src/semantic/symbol.cpp`, and `src/semantic/analyzer.cpp`.
- The symbol tables extracted from each file (classes, functions, enums, structs).
- Brief node details for key declarations such as `Scope::define`, `Scope::lookup`, `Analyzer::analyze`, and `Analyzer::visitFunctionDecl`.

The output is formatted as a coherent package with file boundaries and section headers, ready to be pasted into an LLM prompt.

---

## Scenario 5: Explain the class hierarchy under `Node`

**Task**
"Describe the class hierarchy rooted at `eval::ast::Node`. List every subclass and the virtual functions they override."

**Expected Skill**
`ast-inspection` or `semantic-analysis`

**Expected Analysis**
The agent inspects `include/ast/node.hpp` and identifies the abstract base class `Node` with its pure virtual `toString()` method, then enumerates the concrete subclasses: `TranslationUnit`, `FunctionDecl`, `VarDecl`, `IfStmt`, `WhileStmt`, `ReturnStmt`, `CallExpr`, `BinaryExpr`, `Literal`, and `Identifier`. For each subclass the agent lists the additional member functions (e.g., `FunctionDecl::name()`, `FunctionDecl::returnType()`, `FunctionDecl::params()`) and confirms that each overrides `toString()`. The agent may also note the `Visitor` class hierarchy (`PrintVisitor`, `CollectVisitor`) that operates on these nodes.

---

## Scenario 6: Review the public API of the eval library

**Task**
"Review the public API of the `eval` library. Are the interfaces consistent? Are there any missing accessors, confusing overloads, or naming inconsistencies?"

**Expected Skill**
`api-review`

**Expected Analysis**
The agent enumerates all public declarations across the four namespaces by inspecting the header files. It produces a report covering:
- **Consistency**: All node subclasses expose `toString()` and inherit `kind()`, `location()`, `children()`, `addChild()` from `Node`.
- **Naming**: `tokenKindName` and `nodeKindName` are free functions following a consistent naming pattern.
- **Overloads**: `Scope::lookup` provides both `Symbol*` and `const Symbol*` overloads, which is a correct and idiomatic pattern.
- **Template**: `Index::findSymbolIf<Predicate>` is the only template and is clearly documented by its parameter name.
- **Potential gaps**: `CollectVisitor` only collects `const Node*` pointers — a mutable variant could be useful. `Index::findSymbol` returns only the first match; a `findAllSymbols` variant might be expected.
- **Verdict**: The API is internally consistent; minor additions could improve ergonomics.

---

## Scenario 7: Analyze workspace dependency order

**Task**
"Which source files depend on which? Show the dependency graph for the `eval` library so I know the correct build and analysis order."

**Expected Skill**
`workspace-analysis`

**Expected Analysis**
The agent inspects `#include` directives across all source and header files and produces a dependency graph. The expected order from least to most dependent:

1. `include/ast/node.hpp` — no internal dependencies
2. `include/ast/visitor.hpp` — depends on `ast/node.hpp`
3. `include/parser/lexer.hpp` — no internal dependencies
4. `include/parser/parser.hpp` — depends on `parser/lexer.hpp` and `ast/node.hpp`
5. `include/semantic/symbol.hpp` — no internal dependencies
6. `include/semantic/analyzer.hpp` — depends on `semantic/symbol.hpp` and `ast/visitor.hpp`
7. `include/workspace/index.hpp` — depends on `semantic/symbol.hpp`
8. `src/ast/node.cpp` → `ast/node.hpp`
9. `src/ast/visitor.cpp` → `ast/visitor.hpp`
10. `src/parser/lexer.cpp` → `parser/lexer.hpp`
11. `src/parser/parser.cpp` → `parser/parser.hpp`
12. `src/semantic/symbol.cpp` → `semantic/symbol.hpp`
13. `src/semantic/analyzer.cpp` → `semantic/analyzer.hpp`, `ast/node.hpp`
14. `src/workspace/index.cpp` → `workspace/index.hpp`

The agent may express this as a topological order and note that the `Workspace` layer is the highest-level consumer of all other modules.
