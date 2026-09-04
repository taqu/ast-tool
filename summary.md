# Phase 7b Summary

## Size

```text
Phase 7a: 145 lines, 6,183 characters, 6,195 bytes, ~1,546 tokens
Phase 7b:  68 lines, 3,235 characters, 3,239 bytes,   ~809 tokens
Reduction: 47.7% approximate tokens
```

## Evaluation

```json
{
  "tests": 41,
  "successes": 37,
  "failures": 4,
  "success_rate": 0.9024,
  "total_tool_calls": 421,
  "average_tool_calls_per_test": 10.27,
  "ast_tool_calls": 28,
  "ast_tool_failures": 7,
  "ast_tool_failure_rate": 0.25,
  "ast_tool_help_calls": 4,
  "ast_tool_retries": 5,
  "bash_calls": 83,
  "read_calls": 170,
  "edit_calls": 80,
  "grep_calls": 73,
  "glob_calls": 10,
  "total_elapsed_seconds": 1165.06,
  "average_elapsed_seconds": 28.42,
  "ast_tool_commands": {
    "search": 10,
    "callers": 6,
    "references": 9,
    "find": 2,
    "help": 1
  },
  "ast_tool_failures_by_command": {
    "find": 1,
    "references": 4,
    "help": 1,
    "callers": 1
  },
  "all_ast_tool_recovery_distances": [3, 1],
  "average_ast_tool_recovery_distance": 2.0,
  "max_ast_tool_recovery_distance": 3,
  "total_input_tokens": 1378,
  "total_output_tokens": 238890,
  "total_tokens": 240268,
  "average_tokens_per_test": 5860.2
}
```

## Invocation and Decision

`semantic-analysis` loaded in four Phase 7b tests. The two tests with skill invocation in both available Phase 7a and Phase 7b traces preserved targeted routing. Three of four Phase 7b failures did not load the skill; the fourth used targeted semantic queries and failed compilation in an unrelated broken baseline header. A preserved Phase 7a replay fails the same four tasks.

```text
Recommendation: ACCEPT
```
