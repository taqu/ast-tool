# Agent Evaluation Runner — Implementation Instructions

## 1. Objective

Implement an MVP Agent Evaluation Runner for `ast-tool`.

The purpose of this runner is to evaluate how effectively Claude Code uses `ast-tool` during realistic coding tasks.

The existing Claude Code log/statistics code should be reused and adapted rather than replaced.

The first implementation is intentionally **Claude Code-specific**. Do not build a generic multi-agent abstraction at this stage.

The runner must execute one evaluation task at a time, collect Claude Code usage statistics, validate the resulting repository state, and persist a machine-readable evaluation result.

---

## 2. Evaluation Model

The basic execution model is:

```text
Evaluation Task
      │
      ▼
Prepare Repository
      │
      ▼
Clear Claude Code Logs
      │
      ▼
Run Claude Code
      │
      ├── Claude Code session logs
      │
      └── Repository modifications
      │
      ▼
Collect Statistics
      │
      ├── Token usage
      ├── Tool usage
      ├── ast-tool command usage
      └── Execution time
      │
      ▼
Validate Repository
      │
      ├── Tests
      ├── Expected files
      └── Other task-specific checks
      │
      ▼
Persist Evaluation Result
```

One task execution must correspond to one independent evaluation record.

---

## 3. Scope

### In scope

Implement:

1. Evaluation task representation
2. Repository preparation/reset
3. Claude Code execution
4. Claude Code log cleanup
5. Claude Code JSONL parsing
6. Token usage aggregation
7. Tool usage aggregation
8. `ast-tool` command usage detection
9. Execution time measurement
10. Git diff collection
11. Task validation
12. JSONL result output
13. A CLI or script entry point for running evaluations
14. Basic error handling and logging

### Out of scope

Do NOT implement:

* A generic multi-agent framework
* Codex support
* Other coding-agent adapters
* A large task-generation framework
* Automatic benchmark generation
* Structured-output redesign for `ast-tool`
* Changes to `analyze_workspace()`
* Workspace streaming
* Changes to Semantic Services
* Changes to existing `ast-tool` commands
* A web dashboard
* Statistical visualization
* Large-scale parallel evaluation

The goal is a small, reliable MVP.

---

# 4. Existing Claude Code Statistics Code

The existing implementation provides the following functionality:

```python
CLAUDE_LOG_DIR = Path("~/.claude/projects").expanduser()

def clear_claude_logs():
    ...

def parse_session_logs():
    ...
```

Reuse this logic.

The existing parser already extracts:

```text
tokens:
  input
  output
  cache_read
  cache_creation

tools:
  tool name → invocation count
```

Preserve this behavior unless changes are necessary for reliable evaluation.

Do not rewrite the parser unnecessarily.

---

# 5. Task Definition

Introduce a small task definition format.

YAML is preferred.

Example:

```yaml
id: references-001

repository: repositories/basic-01

prompt: |
  Find all usages of User::save() and add logging
  to the call site in UserService.

validation:
  command: ./tests/run.sh

expected_files:
  - src/service/user_service.cpp
```

The exact schema may be adjusted to fit the existing project conventions.

At minimum, a task must contain:

```text
id
repository
prompt
validation
```

Optional fields may include:

```text
expected_files
expected_exit_code
working_directory
timeout
```

Keep the schema small.

Do not introduce unnecessary configuration complexity.

---

# 6. Repository Isolation

Each task must start from a clean repository state.

The runner must prevent changes from previous tasks from affecting subsequent tasks.

The preferred approach is:

```text
clean repository
      ↓
copy / checkout task repository
      ↓
run Claude Code
      ↓
collect results
      ↓
discard changes
```

If the existing project already has a suitable repository-reset mechanism, reuse it.

Do not assume that `git reset --hard` alone is sufficient if generated/untracked files can affect the next run.

Repository isolation must be deterministic.

---

# 7. Claude Code Execution

Implement a function conceptually equivalent to:

```python
run_claude(task) -> ExecutionResult
```

It should:

1. Prepare the environment
2. Set:

```python
CI=true
```

3. Clear the Claude Code logs
4. Record the start time
5. Launch Claude Code in headless/non-interactive mode
6. Provide the task prompt
7. Wait for completion
8. Record the end time
9. Capture the process exit code
10. Preserve stdout/stderr for debugging

The runner must support a configurable timeout.

A timeout must result in a failed evaluation rather than hanging indefinitely.

---

# 8. Claude Code Log Parsing

After Claude Code exits:

```python
stats = parse_session_logs()
```

The parser must collect:

### Token usage

```text
input
output
cache_read
cache_creation
```

### Tool usage

Count every `tool_use` entry.

For example:

```text
Read
Grep
Bash
Edit
Write
```

The exact tool names should come from the logs rather than from a hard-coded list.

---

# 9. Detecting ast-tool Usage

The evaluation must distinguish ordinary Claude Code tool usage from `ast-tool` usage.

For example:

```text
Bash
  ast-tool find ...
  ast-tool references ...
  ast-tool callers ...
```

should be recorded separately.

The result should expose something conceptually like:

```json
{
  "tools": {
    "Read": 5,
    "Grep": 2,
    "Bash": 4
  },
  "ast_tool": {
    "find": 1,
    "references": 2,
    "callers": 1,
    "callees": 0
  }
}
```

The implementation must account for the fact that `ast-tool` may be invoked through a shell command such as `Bash`.

Do not assume that `ast-tool` itself always appears as a Claude Code tool name.

If the existing Claude Code log format contains the shell command as part of the tool input, inspect that input and extract the command.

---

# 10. Preserve the Workflow

The runner should retain enough information to reconstruct the high-level tool workflow.

For example:

```text
Read
Grep
Bash(ast-tool find)
Bash(ast-tool references)
Read
Edit
```

At minimum, store:

```text
tool name
ast-tool command, if detected
```

in execution order.

Do not attempt sophisticated semantic classification yet.

A simple ordered event list is sufficient.

Example:

```json
{
  "workflow": [
    {"tool": "Read"},
    {"tool": "Grep"},
    {"tool": "Bash", "ast_tool_command": "find"},
    {"tool": "Bash", "ast_tool_command": "references"},
    {"tool": "Read"},
    {"tool": "Edit"}
  ]
}
```

This will be useful for later analysis.

---

# 11. Execution Metrics

Record at least:

```text
elapsed_seconds
process_exit_code
timed_out
```

Also record token statistics and tool statistics.

Do not attempt to derive a monetary cost unless the required pricing information is already available in the project.

---

# 12. Git Diff Collection

After Claude Code finishes, collect the repository changes.

At minimum record:

```text
changed files
diff
```

Prefer obtaining the diff from Git rather than relying only on Claude Code logs.

The evaluation result should make it possible to inspect:

```text
What did the agent change?
```

after the run.

Do not automatically discard the diff before it has been persisted.

---

# 13. Validation

Task correctness must be evaluated independently from Claude Code logs.

Implement a validator conceptually equivalent to:

```python
validate_task(task, repository) -> ValidationResult
```

The primary MVP validation mechanism is a task-provided command:

```yaml
validation:
  command: ./tests/run.sh
```

Execute it after Claude Code completes.

Record:

```text
validation exit code
validation stdout
validation stderr
validation success
```

A task is successful only when its validation succeeds, unless the task explicitly defines another validation rule.

The validator must not inspect Claude Code tool usage when determining correctness.

This separation is important:

```text
Agent behavior
    ↓
Claude Code logs

Task correctness
    ↓
Repository validation
```

---

# 14. Expected Files

Support optional expected-file validation.

Example:

```yaml
expected_files:
  - src/service/user_service.cpp
```

If specified, verify that those files were modified.

This should be an additional validation signal, not a replacement for the task's actual tests.

Do not require expected files for every task.

---

# 15. Evaluation Result

Persist one JSON object per task execution.

JSONL is preferred.

Example:

```json
{
  "task_id": "references-001",
  "success": true,
  "elapsed_seconds": 18.2,
  "process_exit_code": 0,
  "timed_out": false,
  "tokens": {
    "input": 12345,
    "output": 1832,
    "cache_read": 5000,
    "cache_creation": 1000
  },
  "tools": {
    "Read": 5,
    "Grep": 2,
    "Bash": 4,
    "Edit": 1
  },
  "ast_tool": {
    "find": 1,
    "references": 2,
    "callers": 0,
    "callees": 0
  },
  "workflow": [
    {"tool": "Read"},
    {"tool": "Bash", "ast_tool_command": "find"},
    {"tool": "Bash", "ast_tool_command": "references"},
    {"tool": "Edit"}
  ],
  "changed_files": [
    "src/service/user_service.cpp"
  ],
  "validation": {
    "success": true,
    "exit_code": 0
  }
}
```

The exact schema may be simplified if necessary, but the information above should be preserved.

---

# 16. Batch Execution

Support running multiple tasks sequentially.

Conceptually:

```bash
python run_eval.py tasks/
```

should execute:

```text
task-001
task-002
task-003
...
```

Each task must run in an isolated repository state.

Do not parallelize task execution in the MVP.

Sequential execution makes debugging and Claude Code log isolation much simpler.

---

# 17. Failure Handling

The runner must distinguish at least:

```text
agent_process_failure
agent_timeout
validation_failure
runner_failure
success
```

For example:

```json
{
  "task_id": "callers-001",
  "status": "validation_failure"
}
```

A failed task must still produce an evaluation record whenever possible.

One failed task must not prevent the remaining batch from running.

Exceptions should be captured and reported clearly.

---

# 18. Reproducibility

Each evaluation result should contain enough metadata to identify the execution environment.

Record where practical:

```text
task_id
timestamp
repository revision
Claude Code command/version if available
ast-tool revision if available
```

Do not add fragile environment detection solely for the sake of metadata.

Use information already available from the runner or repository.

---

# 19. Recommended Project Structure

Adapt this to the existing repository rather than forcing an unrelated architecture.

A possible structure is:

```text
evaluation/
├── tasks/
│   ├── references-001.yaml
│   ├── callers-001.yaml
│   └── ...
│
├── repositories/
│   ├── basic-01/
│   └── ...
│
├── runner.py
├── claude.py
├── logs.py
├── validator.py
└── results/
```

The existing project structure takes precedence.

Do not create unnecessary packages merely to match this example.

---

# 20. Testing the Evaluation Runner

Before creating the full benchmark, create a very small smoke-test evaluation.

For example:

```text
1 repository
1 task
1 Claude Code invocation
```

Verify:

1. Claude Code starts successfully
2. Logs are isolated
3. Tokens are collected
4. Tool calls are collected
5. `ast-tool` commands are detected
6. Git diff is captured
7. Validation runs
8. JSONL result is produced
9. The runner exits cleanly

Only after this works should batch execution be tested.

---

# 21. Do Not Build the Benchmark Yet

This implementation task is about the **evaluation infrastructure**, not the final evaluation dataset.

Do not spend significant effort creating:

```text
20–30 tasks
5–8 repositories
large synthetic repositories
real-world repositories
```

Those will be added after the runner works.

The immediate goal is:

```text
One task
   ↓
Claude Code
   ↓
Statistics + workflow + diff
   ↓
Validation
   ↓
JSONL result
```

Once this pipeline is reliable, the benchmark data can be developed independently.

---

# 22. Design Principle

Keep the architecture simple:

```text
Task Definition
      │
      ▼
Evaluation Runner
      │
      ├── Claude Code
      ├── Log Parser
      ├── Git
      └── Validator
      │
      ▼
Evaluation Result
```

Do not let evaluation-specific logic leak into `ast-tool`.

The existing `ast-tool` implementation should remain unchanged unless a concrete integration problem is discovered.

---

# 23. Acceptance Criteria

The implementation is complete when all of the following are true:

* [ ] A task can be defined independently of the runner.
* [ ] A clean repository can be prepared for a task.
* [ ] Claude Code can be launched automatically in headless mode.
* [ ] Claude Code logs are isolated per task.
* [ ] Input/output/cache token usage is collected.
* [ ] Claude Code tool usage is collected.
* [ ] `ast-tool` commands invoked through shell tools can be detected.
* [ ] The ordered tool workflow is preserved.
* [ ] Execution time and process status are recorded.
* [ ] Git changes are collected.
* [ ] Task-specific validation can be executed.
* [ ] Validation results are recorded separately from agent behavior.
* [ ] One JSONL record is produced per task.
* [ ] A failed task does not terminate the entire batch.
* [ ] A single-task smoke test passes end-to-end.
* [ ] No changes to Semantic Services or `ast-tool` behavior are required.
* [ ] No generic multi-agent abstraction is introduced.

## Final Deliverable

Provide:

1. The implemented evaluation runner.
2. A minimal example task.
3. A minimal example repository or test fixture.
4. A command showing how to run the smoke test.
5. An example JSONL evaluation result.
6. Brief documentation describing how to add a new evaluation task.

Do not implement the full benchmark dataset in this change.
