# Add Antigravity CLI Support to the Agent Evaluation Framework

## Goal

Extend the existing agent evaluation framework so that it supports **Antigravity CLI** in addition to the current **Claude Code** runner.

The goal is **not** to create a separate evaluation system for Antigravity.

Instead, refactor the evaluation framework so that multiple coding agents can run the **same evaluation tasks**, use the **same repositories**, execute the **same validation commands**, and produce results in a common format.

The initial supported agents should be:

```text
Claude Code
Antigravity CLI
```

The architecture should make it easy to add more coding agents later.

---

# 1. Current Situation

The current evaluation infrastructure was originally built around Claude Code.

The existing flow is conceptually:

```text
Evaluation Task
      ↓
Claude Code
      ↓
Skills / Tools / ast-tool
      ↓
Code Modification
      ↓
Validation
      ↓
JSONL Result
      ↓
Statistics / Analysis
```

The framework already collects information such as:

```json
{
  "task_id": "...",
  "success": true,
  "elapsed_seconds": 181.52,
  "tokens": {},
  "tools": {},
  "ast_tool": {},
  "workflow": [],
  "changed_files": [],
  "validation": {}
}
```

The framework also analyzes agent-specific logs, especially Claude Code JSONL logs.

The new design must preserve the existing Claude Code functionality while adding Antigravity CLI support.

---

# 2. Primary Design Goal

Separate the evaluation framework into two layers:

```text
                Evaluation Core
                      │
                      ▼
            Common Task / Result Model
                      │
          ┌───────────┴───────────┐
          ▼                       ▼
    Claude Code Runner      Antigravity Runner
          │                       │
          ▼                       ▼
    Claude-specific logs    Antigravity-specific logs
          │                       │
          └───────────┬───────────┘
                      ▼
              Normalized Result
                      │
                      ▼
            Common Statistics
```

The evaluation core must not depend directly on Claude Code.

Agent-specific behavior should be isolated behind a small adapter or runner interface.

---

# 3. Do Not Break Existing Behavior

This is a compatibility-sensitive refactor.

The following existing functionality must continue to work:

```text
Claude Code evaluation execution
Task loading
Repository preparation
Validation
Timeout handling
Changed file detection
Claude Code log parsing
Token statistics
Tool statistics
ast-tool command extraction
Workflow extraction
JSONL result generation
Existing statistics scripts
```

Do not rewrite the entire evaluation framework unless necessary.

Prefer incremental refactoring.

---

# 4. Introduce an Agent Runner Abstraction

Create a common abstraction for running a coding agent.

For example:

```python
class AgentRunner:
    def run(
        self,
        task,
        workspace,
        timeout,
    ) -> AgentRunResult:
        ...
```

The exact API may differ depending on the existing codebase.

The important requirement is that the evaluation core should be able to do something conceptually similar to:

```python
runner = create_runner(agent_name)

result = runner.run(
    task=task,
    workspace=workspace,
    timeout=task.timeout,
)
```

The evaluation core should not need to know:

```text
How Claude Code is invoked
How Antigravity CLI is invoked
How agent logs are located
How agent-specific output is parsed
```

Those details belong inside the corresponding runner or adapter.

---

# 5. Suggested Runner Structure

A structure similar to the following is recommended:

```text
evaluation/
├── core/
│   ├── task.py
│   ├── result.py
│   ├── validation.py
│   └── runner.py
│
├── agents/
│   ├── base.py
│   ├── claude_code.py
│   └── antigravity.py
│
├── logging/
│   ├── claude_logs.py
│   └── antigravity_logs.py
│
└── run_evaluation.py
```

Do not force this exact directory layout if the current repository structure already has a better organization.

The important architectural boundary is:

```text
Evaluation Core
        ↓
Agent Runner Interface
        ↓
Agent-specific implementation
```

---

# 6. Add Agent Selection

The evaluation runner should support explicit agent selection.

For example:

```bash
python run_evaluation.py --agent claude
```

and:

```bash
python run_evaluation.py --agent antigravity
```

It should also be possible to run multiple agents in the future.

For example, the architecture should not prevent something like:

```bash
python run_evaluation.py --agent claude,antigravity
```

This multi-agent mode does not need to be implemented unless it is simple and fits naturally into the current codebase.

The initial requirement is:

```text
One evaluation task set
+
Selectable coding agent
```

---

# 7. Antigravity CLI Integration

Implement a dedicated runner for Antigravity CLI.

The runner should be responsible for:

```text
Preparing the working directory
Constructing the prompt
Invoking Antigravity CLI
Passing the task instructions
Handling timeout
Capturing stdout
Capturing stderr
Recording exit status
Returning normalized execution information
```

Before implementing the integration, inspect how Antigravity CLI is expected to be invoked in the local environment.

Do not hard-code assumptions about undocumented command-line syntax.

Use the actual installed CLI behavior and its help output where available.

The implementation should clearly isolate the command construction, for example:

```python
class AntigravityRunner(AgentRunner):
    def build_command(self, ...):
        ...
```

This will make future CLI changes easier to handle.

---

# 8. Normalize Agent Results

Different coding agents may expose different metadata.

For example:

Claude Code may provide:

```text
Input tokens
Output tokens
Cache read tokens
Cache creation tokens
Tool usage
Tool sequence
```

Antigravity CLI may provide a different set of information.

Do not force fake or estimated values.

Instead, use a normalized result structure where unavailable information can remain empty or null.

For example:

```json
{
  "agent": "antigravity",
  "task_id": "level2-004",

  "success": true,
  "elapsed_seconds": 123.45,

  "tokens": {
    "input": null,
    "output": null,
    "cache_read": null,
    "cache_creation": null
  },

  "tools": {},

  "ast_tool": {},

  "workflow": [],

  "changed_files": [],

  "validation": {}
}
```

If Antigravity exposes additional useful metadata, it may be stored in an agent-specific section.

For example:

```json
{
  "agent": "antigravity",

  "agent_metadata": {
    "..."
  }
}
```

However, the common fields used by the statistics system should remain consistent.

---

# 9. ast-tool Usage Detection

The evaluation framework already tracks commands such as:

```text
search
find
references
callers
callees
symbols
help
```

This behavior should continue for Claude Code.

For Antigravity, detect `ast-tool` usage from whatever execution logs or captured command information are actually available.

The desired normalized output is:

```json
"ast_tool": {
  "search": 4,
  "callers": 2,
  "references": 1
}
```

The implementation should not assume that Antigravity logs have the same structure as Claude Code logs.

Create a separate extraction path if necessary.

Conceptually:

```text
Claude logs
    ↓
Claude ast-tool parser
    ↓
Normalized ast_tool counts

Antigravity logs
    ↓
Antigravity ast-tool parser
    ↓
Normalized ast_tool counts
```

---

# 10. Workflow Collection

The existing evaluation framework records tool or workflow sequences.

For example:

```text
Skill
↓
ast-tool search
↓
ast-tool callers
↓
Read
↓
Edit
```

Preserve this capability for Claude Code.

For Antigravity, collect workflow information only if reliable data is available.

Possible normalized entries could look like:

```json
[
  "Bash",
  "ast-tool search",
  "Read",
  "Edit"
]
```

If Antigravity does not expose reliable tool-level information, do not fabricate a workflow.

In that case:

```json
"workflow": []
```

is acceptable.

---

# 11. Statistics Compatibility

The existing statistics scripts should continue to work.

Update them so that results can be grouped by agent.

For example:

```text
Claude Code
├── success rate
├── average time
├── token usage
├── ast-tool adoption
└── command usage

Antigravity
├── success rate
├── average time
├── token usage, if available
├── ast-tool adoption
└── command usage
```

Support analysis such as:

```text
Success rate by agent
Success rate by level and agent
Average execution time by agent
ast-tool adoption by agent
ast-tool command usage by agent
Failure rate by agent
```

The existing global statistics should remain available.

For example:

```text
Overall
  ↓
By Agent
  ↓
By Level
  ↓
By Agent × Level
```

---

# 12. Result Schema Evolution

Add an explicit agent identifier to every new result.

For example:

```json
{
  "agent": "claude",
  "task_id": "level2-004",
  ...
}
```

and:

```json
{
  "agent": "antigravity",
  "task_id": "level2-004",
  ...
}
```

Maintain backward compatibility with existing result files if practical.

For old results that do not contain an `agent` field, treat them as:

```text
claude
```

if they were generated by the existing Claude Code runner.

Avoid unnecessary schema redesign.

This task is about multi-agent support, not a complete result format redesign.

---

# 13. Agent-Specific Log Isolation

Do not assume that all agents store logs in:

```text
~/.claude/projects
```

Claude Code log handling should remain where it is.

Antigravity-specific log discovery and cleanup must be isolated.

For example:

```python
class AgentRunner:
    def clear_logs(self):
        ...

    def collect_logs(self):
        ...
```

Or equivalent functionality.

The evaluation runner should conceptually perform:

```text
Prepare agent environment
        ↓
Clear relevant logs
        ↓
Run agent
        ↓
Collect execution metadata
        ↓
Normalize result
```

Only clear logs that are known to belong to the evaluation session.

Do not delete unrelated user data.

This is especially important when introducing support for additional coding agents.

---

# 14. Preserve Evaluation Fairness

Claude Code and Antigravity should receive equivalent task conditions.

For the same task:

```text
Same repository
Same initial repository state
Same task prompt
Same timeout
Same validation command
Same success criteria
```

Do not create agent-specific task variants unless explicitly required later.

The purpose is comparative evaluation.

---

# 15. Failure Handling

The runner should distinguish between:

```text
Agent execution failure
Timeout
Validation failure
Runner error
```

For example:

```json
{
  "success": false,

  "failure_type": "validation_failure",

  "validation": {
    "success": false,
    "exit_code": 1
  }
}
```

Or:

```json
{
  "success": false,

  "failure_type": "timeout"
}
```

Use the existing result conventions where possible.

Do not introduce a completely separate failure model unless required.

---

# 16. Testing

Add tests or manual verification for at least the following.

## Claude Code regression

Verify that:

```text
Existing Claude Code evaluation still runs
Existing result generation still works
Existing log parsing still works
Existing statistics still work
```

## Antigravity smoke test

Create or run a minimal evaluation task and verify:

```text
Antigravity CLI is invoked successfully
The task prompt is delivered
Repository changes are detected
Validation runs
A normalized JSON result is produced
The result contains:
    agent = antigravity
```

## Statistics

Verify that mixed results such as:

```text
claude
claude
antigravity
antigravity
```

can be analyzed without errors.

Verify grouping by:

```text
agent
level
agent × level
```

---

# 17. Recommended Implementation Order

Implement in this order.

### Step 1

Inspect the existing evaluation runner.

Identify:

```text
Task loading
Agent invocation
Repository setup
Timeout handling
Validation
Result generation
Claude log parsing
Statistics dependencies
```

Do not start by rewriting the framework.

---

### Step 2

Extract the existing Claude Code-specific execution logic behind an agent runner abstraction.

The behavior should remain unchanged.

At this point:

```text
Old architecture
        ↓
Claude-specific evaluation code

New architecture
        ↓
Evaluation Core
        ↓
ClaudeCodeRunner
```

Both should produce equivalent results.

---

### Step 3

Add:

```text
AntigravityRunner
```

Implement only the minimum functionality required to:

```text
Run task
Capture execution
Detect timeout
Run validation
Generate normalized result
```

---

### Step 4

Investigate Antigravity logging and metadata.

Add support for:

```text
ast-tool usage detection
workflow extraction
token statistics
tool statistics
```

only where reliable information is actually available.

---

### Step 5

Update the statistics layer.

Add:

```text
agent
agent × level
```

grouping.

Do not remove existing reports.

---

### Step 6

Run regression tests.

Compare:

```text
Existing Claude result
vs
Refactored Claude result
```

Ensure that the refactor did not silently lose metrics.

Then run Antigravity on the same smoke and Level 1 or Level 2 tasks.

---

# 18. Acceptance Criteria

The implementation is complete when all of the following are true.

### Core

* [ ] Evaluation core no longer directly depends on Claude Code execution logic.
* [ ] Claude Code execution is implemented through an agent-specific runner or adapter.
* [ ] Antigravity CLI execution is implemented through a separate runner or adapter.
* [ ] Agent selection is supported.

### Claude Compatibility

* [ ] Existing Claude Code evaluations still run.
* [ ] Existing Claude log parsing still works.
* [ ] Existing token statistics remain available.
* [ ] Existing tool statistics remain available.
* [ ] Existing ast-tool usage detection remains available.
* [ ] Existing workflow collection remains available.

### Antigravity

* [ ] Antigravity CLI can execute evaluation tasks.
* [ ] Task prompts are passed correctly.
* [ ] Timeout handling works.
* [ ] Validation runs after execution.
* [ ] Changed files are detected.
* [ ] Results contain `agent: "antigravity"`.
* [ ] Available Antigravity metadata is normalized into the common result format.
* [ ] ast-tool usage is collected when reliable command information is available.

### Statistics

* [ ] Existing statistics continue to work.
* [ ] Results can be grouped by agent.
* [ ] Success rate by agent is available.
* [ ] Success rate by agent and level is available.
* [ ] ast-tool adoption by agent is available.
* [ ] ast-tool command usage by agent is available.

### Safety

* [ ] Agent-specific log cleanup does not delete unrelated user data.
* [ ] Missing agent-specific metrics are represented as unavailable rather than fabricated.

---

# 19. Non-Goals

Do not implement the following unless they are necessary for the Antigravity integration:

```text
Complete redesign of the evaluation framework
New evaluation task format
New validation system
Structured output redesign
Streaming workspace analysis
New ast-tool semantic commands
Incremental workspace analysis
Filesystem watching
Persistent caching
New semantic services
```

The scope of this task is:

```text
Existing Evaluation Framework
        +
Agent Runner Abstraction
        +
Antigravity CLI Support
        +
Common Result Format
        +
Agent-aware Statistics
```

Keep the implementation focused.

---

# Final Principle

The final architecture should allow the evaluation system to answer:

```text
For the same task set,
under the same repository and validation conditions,

how do different coding agents perform,
and when do they actually benefit from ast-tool?
```

The evaluation framework should therefore separate:

```text
Task Definition
        ↓
Evaluation Core
        ↓
Agent Adapter
        ↓
Agent Execution
        ↓
Normalized Result
        ↓
Common Analysis
```

Claude Code is the first implementation.

Antigravity CLI is the second.

Future coding agents should be addable without modifying the core evaluation logic.
