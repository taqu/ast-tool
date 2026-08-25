# Evaluation Runner

Measures how effectively Claude Code uses `ast-tool` during coding tasks.

## Setup

```bash
cd evaluation
pip install pyyaml          # only dependency beyond stdlib
python setup_repos.py       # initializes git repos from fixtures/
```

`ast-tool` must be on `PATH` so Claude Code can invoke it during tasks.

## Run the smoke test

```bash
python run_eval.py tasks/smoke-001.yaml
```

Results are appended to `evaluation/results/results.jsonl`.

## Adding a new task

1. Create a fixture directory under `evaluation/fixtures/<name>/` with the repository source files
   and a `validate.py` (or other validation script) at its root.

2. Add a YAML task file under `evaluation/tasks/<id>.yaml`:

```yaml
id: my-task-001

# Path relative to evaluation/
repository: repositories/<name>

prompt: |
  <The exact instruction given to Claude Code>

validation:
  command: python validate.py   # run from the repository root

# Optional
expected_files:
  - src/some_file.cpp

timeout: 180   # seconds (default 300)
```

3. Run `python setup_repos.py` to (re-)initialize the repository from the fixture.

4. Run the task:

```bash
python run_eval.py tasks/my-task-001.yaml
```

## Result schema (JSONL)

```json
{
  "task_id": "smoke-001",
  "status": "success",
  "success": true,
  "timestamp": "2026-08-21T00:00:00Z",
  "repository_revision": "<sha>",
  "elapsed_seconds": 45.2,
  "process_exit_code": 0,
  "timed_out": false,
  "tokens": { "input": 0, "output": 0, "cache_read": 0, "cache_creation": 0 },
  "tools": { "Read": 3, "Bash": 2, "Edit": 1 },
  "ast_tool": { "find": 1, "references": 1 },
  "workflow": [
    { "tool": "Bash", "ast_tool_command": "find" },
    { "tool": "Bash", "ast_tool_command": "references" },
    { "tool": "Edit" }
  ],
  "changed_files": ["src/main.cpp"],
  "validation": { "success": true, "exit_code": 0, "stdout": "PASS: ...", "stderr": "" }
}
```

### Status values

| Status | Meaning |
|---|---|
| `success` | Agent finished, validation passed |
| `validation_failure` | Agent finished, but validation failed |
| `agent_process_failure` | Claude Code exited with a non-zero code |
| `agent_timeout` | Claude Code exceeded the timeout |
| `runner_failure` | Infrastructure error (missing repo, git failure, etc.) |

## Running a batch

```bash
python run_eval.py tasks/
```

Tasks execute sequentially. A failed task does not abort the batch.

###
```bash
 Usage:
  python run_eval.py tasks/ --task level2-004 --trace-tools
  python run_eval.py tasks/ --task level2-004 --task level2-008 --trace-tools
  --max-trace-output-bytes 50000
```
