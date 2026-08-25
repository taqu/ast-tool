# Add Detailed Tool Use Logging to the Evaluation Runner

## Goal

Add detailed tool-use tracing to the evaluation framework.

The current evaluation results contain aggregate information such as:

```text
tool usage counts
ast-tool command counts
workflow
token usage
elapsed time
validation result
```

This is useful for high-level comparison, but it is not sufficient to understand *why* a tool was called repeatedly.

For example, the current data may show:

```text
callers: 42 calls
```

but does not reveal:

```text
Which symbol was passed to callers?

What arguments were used?

What result was returned?

Did the agent call callers again with the same input?

Did the agent switch to another tool after seeing the result?

Was the output empty, unexpected, or ambiguous?
```

The goal of this change is to record individual tool invocations including:

```text
tool name
tool input
tool output
timestamp or sequence number
```

This logging should make it possible to analyze tool-call behavior for individual evaluation tasks.

The implementation does not need to enable detailed tracing for every evaluation run.

It should support running and tracing one or a small number of specified tests.

---

# 1. Keep `results.jsonl` as the Summary Result

Do not replace the existing `results.jsonl` format.

It should continue to contain the high-level evaluation result for each task.

For example:

```json
{
  "task_id": "level2-004",
  "success": true,
  "elapsed_seconds": 181.52,
  "tools": {
    "Bash": 17,
    "Read": 5,
    "Edit": 6
  },
  "ast_tool": {
    "search": 4,
    "callers": 8,
    "references": 1
  }
}
```

Detailed tool traces should be stored separately.

Do not embed all tool outputs into `results.jsonl`, because this could make the summary file unnecessarily large.

---

# 2. Add Per-Task Trace Files

For a traced task, create a separate file.

Recommended directory structure:

```text
results/
├── results.jsonl
│
└── traces/
    ├── level2-004.jsonl
    ├── level2-008.jsonl
    └── ...
```

Each line in the trace file should represent one event.

The primary event type is:

```text
tool_use
```

Example:

```json
{
  "event": "tool_use",
  "sequence": 12,
  "timestamp": "2026-08-26T12:34:56.123Z",

  "tool": "Bash",

  "input": {
    "command": "ast-tool callers greet"
  },

  "output": "...",

  "success": true
}
```

For an ast-tool invocation, the trace should preserve both the outer tool call and the semantic command if possible.

For example:

```json
{
  "event": "tool_use",
  "sequence": 7,

  "tool": "Bash",

  "ast_tool": {
    "command": "callers",
    "arguments": {
      "symbol": "greet"
    }
  },

  "input": {
    "command": "ast-tool callers greet"
  },

  "output": {
    "..."
  },

  "success": true
}
```

Adapt the exact schema to the actual runner and agent output.

The important requirement is that the original input and output remain available.

---

# 3. Prefer Structured JSONL Trace Data

Use JSONL rather than a human-formatted text log.

For example:

```text
level2-004.jsonl
```

```json
{"event":"task_start","task_id":"level2-004"}

{"event":"tool_use","sequence":1,"tool":"Read","input":{...},"output":{...}}

{"event":"tool_use","sequence":2,"tool":"Bash","input":{...},"output":{...}}

{"event":"tool_use","sequence":3,"tool":"Bash","input":{...},"output":{...}}

{"event":"task_end","success":true}
```

Reasons:

```text
Easy to parse later
Supports large outputs
Allows event-by-event analysis
Can be converted to CSV later
Allows repeated tool-call detection
```

Do not store the trace only as formatted terminal text.

---

# 4. Capture Tool Input Exactly

The trace must preserve the actual tool input as closely as possible.

Examples:

### Grep

```json
{
  "tool": "Grep",
  "input": {
    "pattern": "greet",
    "path": "src"
  }
}
```

### Read

```json
{
  "tool": "Read",
  "input": {
    "file_path": "src/main.cpp"
  }
}
```

### Bash

```json
{
  "tool": "Bash",
  "input": {
    "command": "ast-tool callers greet"
  }
}
```

Do not reduce the input to only a summary such as:

```text
tool = Bash
```

The arguments are required for later analysis.

---

# 5. Capture Tool Output

Capture the tool output associated with each tool invocation.

For example:

```json
{
  "tool": "Bash",
  "input": {
    "command": "ast-tool callers greet"
  },
  "output": {
    "stdout": "...",
    "stderr": "...",
    "exit_code": 0
  }
}
```

If the actual agent runtime provides the output as a string or another structure, preserve the original structure where practical.

Do not aggressively normalize away information that may be useful later.

The trace is intended for debugging and behavioral analysis.

---

# 6. Avoid Truncating Outputs by Default

Do not truncate tool outputs silently.

For the initial implementation, prefer preserving the complete output.

If output size is a concern, support an explicit option such as:

```text
--max-trace-output-bytes
```

For example:

```bash
python run_eval.py \
    --task level2-004 \
    --trace-tools \
    --max-trace-output-bytes 50000
```

If truncation occurs, record it explicitly:

```json
{
  "output_truncated": true,
  "original_output_bytes": 183421
}
```

Do not make a truncated output look complete.

---

# 7. Record a Sequence Number

Every event within a task trace should have a monotonically increasing sequence number.

For example:

```text
1  task_start
2  Skill
3  Bash
4  Read
5  Bash
6  Bash
7  Edit
8  validation
9  task_end
```

The sequence number is important because it allows later analysis of patterns such as:

```text
callers
→ callers
```

or:

```text
callers
→ Bash
→ callers
```

Do not rely only on timestamps for ordering.

---

# 8. Record Timing Where Available

If practical, record:

```text
start timestamp
end timestamp
duration
```

for each tool call.

For example:

```json
{
  "event": "tool_use",
  "sequence": 12,

  "started_at": "2026-08-26T12:34:56.100Z",
  "ended_at": "2026-08-26T12:34:57.420Z",

  "duration_seconds": 1.32
}
```

This is useful for determining whether ast-tool improves navigation latency or adds expensive investigation steps.

Timing is optional if the underlying agent stream does not provide enough information.

Do not block the implementation on precise timing support.

---

# 9. Add Task-Level Trace Selection

The evaluation runner should support tracing only selected tasks.

Recommended examples:

```bash
python run_eval.py \
    --task level2-004 \
    --trace-tools
```

Support specifying multiple tasks if convenient:

```bash
python run_eval.py \
    --task level2-004 \
    --task level2-008 \
    --trace-tools
```

Alternatively, support:

```bash
--tasks level2-004,level2-008
```

Follow the existing CLI style.

The important requirement is that it must be easy to trace one task without rerunning the entire evaluation suite.

---

# 10. Optional: Separate Trace Script

If modifying `run_eval.py` significantly would make it complicated, it is acceptable to implement a separate script.

For example:

```text
trace_eval.py
```

Usage:

```bash
python trace_eval.py \
    --task level2-004 \
    --output results/traces/level2-004.jsonl
```

The trace script should reuse as much of the existing evaluation execution logic as possible.

Do not duplicate the entire evaluation runner if shared functions can be extracted.

A possible structure is:

```text
eval_runner.py
    load_task()
    run_task()
    validate_task()

run_eval.py
    normal evaluation mode

trace_eval.py
    detailed tool tracing mode
```

This separation is acceptable and may be cleaner.

---

# 11. Capture Raw Agent Events When Possible

If the coding agent runtime already exposes a stream of structured events, prefer recording those events before converting them into aggregate statistics.

For example, if the runtime produces:

```text
assistant_message
tool_use
tool_result
assistant_message
```

record enough information to reconstruct the sequence.

For example:

```json
{
  "event": "tool_use",
  "sequence": 4,
  "tool_use_id": "toolu_123",
  "tool": "Bash",
  "input": {
    "command": "ast-tool callers greet"
  }
}
```

followed by:

```json
{
  "event": "tool_result",
  "sequence": 5,
  "tool_use_id": "toolu_123",
  "output": {
    "stdout": "...",
    "stderr": ""
  }
}
```

It is also acceptable to combine them into a single event if that better matches the existing runtime.

The key requirement is that the input can be associated with the correct output.

---

# 12. Record ast-tool Metadata When Detectable

The trace system should detect ast-tool commands when possible.

For example:

```text
ast-tool callers greet
```

should ideally be represented as:

```json
{
  "tool": "Bash",

  "ast_tool": {
    "detected": true,
    "command": "callers",
    "raw_command": "ast-tool callers greet"
  }
}
```

If parsing the arguments is reliable:

```json
{
  "ast_tool": {
    "detected": true,
    "command": "callers",
    "arguments": {
      "symbol": "greet"
    }
  }
}
```

Do not attempt fragile parsing that could corrupt the original command.

Always preserve:

```text
raw_command
```

even if structured argument extraction is implemented.

---

# 13. Add a Trace Index

When detailed tracing is enabled, optionally create an index file.

For example:

```text
results/traces/index.jsonl
```

Example:

```json
{
  "task_id": "level2-004",
  "trace_file": "level2-004.jsonl",
  "success": true,
  "tool_calls": 27,
  "ast_tool_calls": 8
}
```

This is optional but useful when many traces exist.

---

# 14. Include Task Start and End Metadata

Each trace should include task-level metadata.

Example:

```json
{
  "event": "task_start",
  "task_id": "level2-004",
  "repository": "repositories/basic-01",
  "agent": "claude",
  "started_at": "..."
}
```

At the end:

```json
{
  "event": "task_end",
  "task_id": "level2-004",
  "success": true,
  "elapsed_seconds": 181.52,
  "validation_success": true
}
```

This allows a trace file to be analyzed independently without needing to join against `results.jsonl`.

---

# 15. Desired Future Analysis

The trace format should support future analysis such as:

## Repeated identical calls

```text
callers(foo)
→ callers(foo)
```

Count:

```text
same tool
+
same normalized input
```

---

## Command transitions

Build transitions such as:

```text
search
→ callers

callers
→ callers

callers
→ Bash

callers
→ Read

callers
→ references
```

---

## Output-driven retries

Detect patterns such as:

```text
callers(foo)
→ output empty

callers(foo)
→ output empty
```

or:

```text
callers(foo)
→ error

callers(foo)
→ modified argument

callers(foo)
```

---

## Investigation loops

For example:

```text
callers
→ Read
→ callers
→ Bash
→ callers
```

---

## Same-target repetition

For example:

```text
callers(foo) × 5
```

versus:

```text
callers(foo)
callers(bar)
callers(baz)
```

These represent different behaviors and should be distinguishable.

---

# 16. Suggested Trace Schema

Use a schema conceptually similar to:

```json
{
  "event": "tool_call",

  "sequence": 7,

  "tool": {
    "name": "Bash"
  },

  "input": {
    "command": "ast-tool callers greet"
  },

  "output": {
    "stdout": "...",
    "stderr": "",
    "exit_code": 0
  },

  "ast_tool": {
    "detected": true,
    "command": "callers",
    "raw_command": "ast-tool callers greet"
  },

  "success": true,

  "duration_seconds": 0.42
}
```

Do not require every field to exist.

For example, some tools may not have:

```text
stdout
stderr
exit_code
```

The schema should support heterogeneous tools.

---

# 17. Error Handling

Tool failures must also be logged.

For example:

```json
{
  "event": "tool_call",
  "sequence": 14,

  "tool": {
    "name": "Bash"
  },

  "input": {
    "command": "ast-tool callers unknown_symbol"
  },

  "output": {
    "stdout": "",
    "stderr": "symbol not found",
    "exit_code": 1
  },

  "success": false
}
```

Do not omit failed tool calls.

They may be particularly important when analyzing repeated invocations.

---

# 18. Do Not Change Evaluation Behavior

Detailed logging must be observational.

It should not:

```text
change the prompt
change agent instructions
change timeout behavior
change tool availability
change validation behavior
```

The same task with and without tracing should execute under the same evaluation conditions, except for the additional logging overhead.

---

# 19. Tests

Add tests for at least:

### Tool input/output recording

Verify:

```text
tool input
+
tool output
```

are both written.

### Sequence ordering

Verify:

```text
sequence 1
sequence 2
sequence 3
```

is preserved.

### Failed tool call

Verify failed calls are logged.

### ast-tool detection

Verify:

```text
ast-tool callers greet
```

is detected as:

```text
command = callers
```

while preserving the raw command.

### Trace disabled

Verify normal evaluation does not generate detailed trace files unless tracing is requested.

### Single task selection

Verify:

```bash
--task level2-004
```

does not run unrelated tasks.

---

# 20. Acceptance Criteria

The implementation is complete when:

* [ ] A single evaluation task can be selected and executed.
* [ ] Detailed tracing can be enabled explicitly.
* [ ] Each tool invocation records its tool name.
* [ ] Tool input is recorded.
* [ ] Tool output is recorded.
* [ ] Failed tool calls are recorded.
* [ ] Events have deterministic sequence numbers.
* [ ] ast-tool commands are detected when possible.
* [ ] The original raw command/input is always preserved.
* [ ] `results.jsonl` remains a compact summary file.
* [ ] Detailed traces are stored separately per task.
* [ ] Normal evaluation behavior remains unchanged when tracing is disabled.
* [ ] The trace format can support repeated-call and command-transition analysis later.
* [ ] Unit tests cover the core trace functionality.

---

# Final Design Principle

The purpose is not simply to produce more logs.

The trace should allow us to reconstruct this kind of sequence:

```text
1. search("greet")
   → found declaration and references

2. callers("greet")
   → unexpected result

3. Bash(...)
   → inspect files manually

4. callers("greet")
   → same query again

5. references("greet")
   → different result

6. Read(src/main.cpp)

7. Edit(src/main.cpp)
```

The resulting data should make it possible to distinguish:

```text
Normal semantic traversal
```

from:

```text
Repeated identical queries
```

from:

```text
Manual verification after an unexpected result
```

from:

```text
Tool errors or symbol resolution failures
```

This is the primary objective of detailed tool-use tracing.
