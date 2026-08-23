import os
import re
import sys
import time
import shutil
import json
import subprocess
from pathlib import Path
from collections import defaultdict
from agents.base import AgentRunner, AgentRunResult

def _find_agy_exe() -> str:
    candidates = ["agy", "agy.exe"] if sys.platform == "win32" else ["agy"]
    for name in candidates:
        if shutil.which(name):
            return name
    return "agy"

class AntigravityRunner(AgentRunner):
    def build_command(self, prompt: str, log_file: Path) -> list[str]:
        exe = _find_agy_exe()
        return [
            exe,
            "--dangerously-skip-permissions",
            "--log-file",
            str(log_file),
            "--print",
            prompt,
        ]

    def run(self, prompt: str, repo_path: Path, timeout: int) -> AgentRunResult:
        # Create a unique log file name in the repo path
        log_file = repo_path / f".agy_log_{int(time.time() * 1000)}.txt"
        
        cmd = self.build_command(prompt, log_file)
        
        env = os.environ.copy()
        env["PYTHONUNBUFFERED"] = "1"
        
        start = time.time()
        timed_out = False
        stdout_data = ""
        stderr_data = ""
        exit_code = -1
        
        try:
            proc = subprocess.Popen(
                cmd,
                cwd=str(repo_path),
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            try:
                stdout_data, stderr_data = proc.communicate(timeout=timeout)
                exit_code = proc.returncode
            except subprocess.TimeoutExpired:
                proc.kill()
                stdout_data, stderr_data = proc.communicate()
                timed_out = True
                exit_code = -1
        except FileNotFoundError:
            stderr_data = (
                f"Error: '{cmd[0]}' not found on path."
            )
            exit_code = -2

        elapsed = round(time.time() - start, 2)
        
        # Parse the log file to get the conversation ID
        conv_id = None
        if log_file.exists():
            try:
                log_content = log_file.read_text(encoding="utf-8")
                m = re.search(r"Created conversation ([a-f0-9\-]+)", log_content)
                if m:
                    conv_id = m.group(1)
            except Exception as e:
                print(f"[antigravity] Warning: failed to parse log file {log_file}: {e}")
            
            # Clean up log_file
            try:
                log_file.unlink()
            except Exception as e:
                print(f"[antigravity] Warning: failed to delete temp log file {log_file}: {e}")

        # Locate and parse the transcripts if conv_id is found
        tokens = {"input": None, "output": None, "cache_read": None, "cache_creation": None}
        tools = {}
        ast_tool = {}
        workflow = []
        
        if conv_id:
            brain_dir = Path("~/.gemini/antigravity-cli/brain").expanduser()
            transcript_path = brain_dir / conv_id / ".system_generated" / "logs" / "transcript.jsonl"
            
            if transcript_path.exists():
                try:
                    tools_counts = defaultdict(int)
                    ast_tool_counts = defaultdict(int)
                    ast_tool_re = re.compile(r'\bast-tool\s+(\w+)')
                    
                    with open(transcript_path, "r", encoding="utf-8") as f:
                        for line in f:
                            if not line.strip():
                                continue
                            data = json.loads(line)
                            
                            # Parse model responses for tool calls
                            if data.get("source") == "MODEL" and data.get("type") == "PLANNER_RESPONSE":
                                tool_calls = data.get("tool_calls") or []
                                for tc in tool_calls:
                                    tool_name = tc.get("name")
                                    if not tool_name:
                                        continue
                                    
                                    tools_counts[tool_name] += 1
                                    
                                    ast_cmd = None
                                    if tool_name == "run_command":
                                        args = tc.get("args") or {}
                                        cmd_line = args.get("CommandLine", "") if isinstance(args, dict) else str(args)
                                        m = ast_tool_re.search(cmd_line)
                                        if m:
                                            ast_cmd = m.group(1)
                                            ast_tool_counts[ast_cmd] += 1
                                            
                                    event = {"tool": tool_name}
                                    if ast_cmd:
                                        event["ast_tool_command"] = ast_cmd
                                    workflow.append(event)
                    
                    tools = dict(tools_counts)
                    ast_tool = dict(ast_tool_counts)
                except Exception as e:
                    print(f"[antigravity] Warning: failed to parse transcript {transcript_path}: {e}")
            
            # Clean up the conversation folder under brain/
            conv_dir = brain_dir / conv_id
            if conv_dir.exists() and conv_dir.is_dir():
                def remove_readonly(func, path, excinfo):
                    import stat
                    os.chmod(path, stat.S_IWRITE)
                    func(path)
                try:
                    shutil.rmtree(conv_dir, onexc=remove_readonly)
                except Exception as e:
                    print(f"[antigravity] Warning: failed to clean up conversation directory {conv_dir}: {e}")

        return AgentRunResult(
            exit_code=exit_code,
            elapsed_seconds=elapsed,
            timed_out=timed_out,
            stdout=stdout_data,
            stderr=stderr_data,
            tokens=tokens,
            tools=tools,
            ast_tool=ast_tool,
            workflow=workflow,
        )
