# Phase 3
```
command	total	level1	level2	level3	level4	level5	smoke
2	1	0	0	0	1	0	0
callers	4	0	1	2	1	0	0
find	3	0	0	0	0	0	3
help	2	0	0	0	0	0	2
references	2	0	0	0	1	0	1
search	3	0	1	1	1	0	0
```

```json
{
  "tests": 41,
  "successes": 25,
  "failures": 16,
  "success_rate": 0.6098,
  "total_tool_calls": 452,
  "average_tool_calls_per_test": 11.02,
  "ast_tool_calls": 15,
  "ast_tool_failures": 2,
  "ast_tool_failure_rate": 0.1333,
  "ast_tool_help_calls": 1,
  "ast_tool_retries": 2,
  "bash_calls": 72,
  "read_calls": 165,
  "edit_calls": 81,
  "grep_calls": 71,
  "glob_calls": 16,
  "total_elapsed_seconds": 1347.23,
  "average_elapsed_seconds": 32.86,
  "ast_tool_commands": {
    "search": 3,
    "callers": 4,
    "2": 1,
    "references": 2,
    "find": 3,
    "help": 2
  },
  "ast_tool_failures_by_command": {
    "find": 2
  },
  "all_ast_tool_recovery_distances": [
    2,
    1
  ],
  "average_ast_tool_recovery_distance": 1.5,
  "max_ast_tool_recovery_distance": 2,
  "total_input_tokens": 1550,
  "total_output_tokens": 252235,
  "total_tokens": 253785,
  "average_tokens_per_test": 6189.9
}
```

# Phase 6
```json
{
  "tests": 41,
  "successes": 25,
  "failures": 16,
  "success_rate": 0.6098,
  "total_tool_calls": 444,
  "average_tool_calls_per_test": 10.83,
  "ast_tool_calls": 15,
  "ast_tool_failures": 4,
  "ast_tool_failure_rate": 0.2667,
  "ast_tool_help_calls": 5,
  "ast_tool_retries": 4,
  "bash_calls": 88,
  "read_calls": 188,
  "edit_calls": 81,
  "grep_calls": 73,
  "glob_calls": 7,
  "total_elapsed_seconds": 1326.58,
  "average_elapsed_seconds": 32.36,
  "ast_tool_commands": {
    "search": 4,
    "callers": 2,
    "references": 5,
    "2": 1,
    "find": 3
  },
  "ast_tool_failures_by_command": {
    "find": 1,
    "references": 3
  },
  "average_ast_tool_recovery_distance": 2.25,
  "max_ast_tool_recovery_distance": 5,
  "total_input_tokens": 1498,
  "total_output_tokens": 250744,
  "total_tokens": 252242,
  "average_tokens_per_test": 6152.2
}
```

# Phase 6 revised
```json
{
  "tests": 41,
  "successes": 37,
  "failures": 4,
  "success_rate": 0.9024,
  "total_tool_calls": 411,
  "average_tool_calls_per_test": 10.02,
  "ast_tool_calls": 22,
  "ast_tool_failures": 5,
  "ast_tool_failure_rate": 0.2273,
  "ast_tool_help_calls": 5,
  "ast_tool_retries": 8,
  "bash_calls": 73,
  "read_calls": 181,
  "edit_calls": 81,
  "grep_calls": 64,
  "glob_calls": 6,
  "total_elapsed_seconds": 1104.85,
  "average_elapsed_seconds": 26.95,
  "ast_tool_commands": {
    "search": 5,
    "callers": 4,
    "references": 7,
    "find": 4,
    "symbols": 1,
    "top_level": 1
  },
  "ast_tool_failures_by_command": {
    "search": 1,
    "find": 1,
    "references": 2,
    "callers": 1
  },
  "all_ast_tool_recovery_distances": [
    1,
    2,
    5,
    2,
    3
  ],
  "average_ast_tool_recovery_distance": 2.6,
  "max_ast_tool_recovery_distance": 5,
  "total_input_tokens": 1364,
  "total_output_tokens": 239766,
  "total_tokens": 241130,
  "average_tokens_per_test": 5881.2
}
```

