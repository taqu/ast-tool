import os
import subprocess
import sys
import time
import json
from re import sub
import shutil
import glob
from pathlib import Path
from collections import defaultdict
from socket import timeout
from datasets import load_dataset
from git import Repo

cache_dir = Path("cache").resolve()
output_preds_path = Path("predictions.jsonl").resolve()
if os.path.exists(output_preds_path):
    with open(output_preds_path, "r+") as f:
        f.truncate(0)

CLAUDE_LOG_DIR = Path("~/.claude/projects").expanduser()

env_config = os.environ.copy()
env_config["CI"] = "true"                  # CI/CD環境（ヘッドレス）として認識させる

def clear_claude_logs():
    """Claude Code のローカルログを完全にクリアしてノイズを排除する"""
    if CLAUDE_LOG_DIR.exists():
        shutil.rmtree(CLAUDE_LOG_DIR)
    CLAUDE_LOG_DIR.mkdir(parents=True, exist_ok=True)

def parse_session_logs():
    """このセッション（直前の1インスタンス）で消費されたトークンとツール回数を集計"""
    jsonl_files = glob.glob(str(CLAUDE_LOG_DIR / "**/*.jsonl"), recursive=True)
    
    stats = {
        "tokens": {"input": 0, "output": 0, "cache_read": 0, "cache_creation": 0},
        "tools": defaultdict(int)
    }
    
    for file_path in jsonl_files:
        try:
            with open(file_path, "r", encoding="utf-8") as f:
                for line in f:
                    if not line.strip(): 
                        continue
                    data = json.loads(line)
                    message = data.get("message") or None
                    if not message:
                        continue
                    
                    # 1. トークン消費量の集計
                    usage = message.get("usage") or message.get("token_usage") or None
                    if usage:
                        stats["tokens"]["input"] += usage.get("input_tokens", 0)
                        stats["tokens"]["output"] += usage.get("output_tokens", 0)
                        stats["tokens"]["cache_read"] += usage.get("cache_read_tokens", 0)
                        stats["tokens"]["cache_creation"] += usage.get("cache_creation_tokens", 0)
                    
                    # 2. ツール呼び出しの集計
                    contents = message.get("content") or None
                    if contents:
                        for content in contents:
                            if not isinstance(content, dict):
                                continue
                            content_type = content.get("type") or None
                            if content_type and content_type == "tool_use":
                                tool_name = content.get("name") or None
                                if tool_name: 
                                    stats["tools"][tool_name] += 1
        except Exception as e:
            print(e)
            continue
            
    return stats

def main():
    # 1. SWE-bench Lite のデータセットをロード
    print("Loading SWE-bench Multilingual dataset...")
    dataset = load_dataset("SWE-bench/SWE-bench_Multilingual", split="test", cache_dir=cache_dir)
    predictions = []
    seen_repos = set()
    
    for entry in dataset:
        repo_name = entry["repo"]
        
        if repo_name in seen_repos:
            continue
            
        seen_repos.add(repo_name)
        
        instance_id = entry["instance_id"]
        base_commit = entry["base_commit"]
        problem_statement = entry["problem_statement"]
        
        print(f"\n=============================================")
        print(f"Processing: {repo_name} ({instance_id})")
        print(f"=============================================\n")
        
        # 3. リポジトリのクローン設定
        github_url = f"https://github.com/{repo_name}.git"
        local_dir = os.path.abspath(os.path.join("workspace", f"{instance_id}"))
        
        if not os.path.exists(local_dir):
            print(f"Cloning {github_url}...")
            try:
                repo = Repo.clone_from(github_url, local_dir)
            except Exception as e:
                print(f"Failed to clone: {e}")
                continue
        else:
            print(f"Directory {local_dir} already exists.")
            repo = Repo(local_dir)
            
        # 4. ベースコミットにチェックアウト
        try:
            repo.git.checkout(base_commit)
        except Exception as e:
            print(f"Failed to checkout {base_commit}: {e}")
            continue
        
        # 5. 問題文を保存
        instruction_file = os.path.join(local_dir, "SWE_PROBLEM_STATEMENT.md")
        with open(instruction_file, "w", encoding="utf-8") as f:
            f.write(f"# Problem Statement for {instance_id}\n\n")
            f.write(problem_statement)

        # 6. 実行直前に前回のログを完全削除
        clear_claude_logs()

        # 7. Claude Code を【非対話・自律モード】で実行する
        claude_instruction = (
            "Read 'SWE_PROBLEM_STATEMENT.md' to understand the issue. Locate the bug, modify the source code to fix it, and ensure all relevant tests pass. Once the fix is completed and verified, exit automatically without waiting for my input."
        )
        
        print(f"Launching Claude Code in autonomous mode...")
        command = ["claude", "-c", claude_instruction, "--dangerously-skip-permissions"]
        timeout = 9000

        current_env = os.environ.copy()
        current_env.update(env_config)
        current_env["PYTHONUNBUFFERED"] = "1" 

        # Unix/Windowsでの共通プロセス起動設定
        # stdoutとstderrを統合し、テキストモード、行バッファリングを有効化
        popen_kwargs = {
            "args": command,
            "stdout": subprocess.PIPE,
            "stderr": subprocess.STDOUT,
            "cwd": local_dir,
            "env": current_env,
            "text": True,
            "bufsize": 1, 
        }

        # Unix系かつptyモジュールが利用可能な場合のみ、TTYをエミュレートする
        # (Claude Codeに標準出力を強制的に「端末」として認識させるため)
        if os.name != 'nt':
            try:
                import pty
                master, slave = pty.openpty()
                popen_kwargs["stdout"] = slave
                popen_kwargs["stderr"] = slave
                popen_kwargs["text"] = False  # pty経由はバイナリで扱う
            except ImportError:
                master = None
        else:
            master = None

        try:
            process = subprocess.Popen(**popen_kwargs)
    
            # ptyを使用した場合、子プロセス側(slave)のFDは親プロセス側では閉じる
            if master is not None:
                os.close(slave)

            start_time = time.time()

            # リアルタイムで出力を読み取って標準出力へ流すループ
            while True:
                # タイムアウト監視
                if time.time() - start_time > timeout:
                    process.terminate()
                    raise subprocess.TimeoutExpired(command, timeout)

                # プロセスの終了確認
                retcode = process.poll()
        
                # 出力の読み込み
                if master is not None:
                    # Unix + PTY モード
                    import select
                    r, _, _ = select.select([master], [], [], 0.1)
                    if master in r:
                        try:
                            data = os.read(master, 4096)
                            if not data:
                                if retcode is not None: break
                            else:
                                sys.stdout.buffer.write(data)
                                sys.stdout.flush()
                        except OSError:
                            # 子プロセスが終了すると、ptyの読み込みはOSErrorを起こす
                            if retcode is not None: break
                else:
                    # Windows または PTY無しのUnixモード
                    # non-blockingな読み込みをシミュレート（readlineはブロックするためpollと併用）
                    line = process.stdout.readline()
                    if not line:
                        if retcode is not None: break
                    else:
                        sys.stdout.write(line)
                        sys.stdout.flush()
                
                # CPUハイジャック防止の短いスリープ
                time.sleep(0.01)

            # 最終的な終了コードの確認
            exit_code = process.returncode
            if exit_code != 0:
                raise subprocess.CalledProcessError(exit_code, command)

            # --- パッチ（差分）の抽出 ---
            patch_diff = Repo(local_dir).git.diff()
            efficiency_stats = parse_session_logs()
            prediction_entry = {
                "instance_id": entry["instance_id"],
                "model_patch": patch_diff,
                "model_name_or_path": "claude-code-agent",
                "efficiency_metrics": {
                    "tokens": efficiency_stats["tokens"],
                    "tools": dict(efficiency_stats["tools"])
                }
            }
            predictions.append(prediction_entry)
        
            # JSONLへ保存
            with open(output_preds_path, "a", encoding="utf-8") as f:
                f.write(json.dumps(prediction_entry) + "\n")

            print(f"\n[Metrics] {instance_id} Execution Summary:")
            print(f"  - Total Input Tokens:  {efficiency_stats['tokens']['input']:,}")
            print(f"  - Total Output Tokens: {efficiency_stats['tokens']['output']:,}")
            print(f"  - Tool Calls:          {dict(efficiency_stats['tools'])}\n")
            break
        except FileNotFoundError:
            print("Error: 'claude' command not found. Please install Claude Code globally.")
            break
        except subprocess.CalledProcessError as e:
            print(f"\n[Warning] Claude execution failed or returned an error code: {e}\n")
            # 1つのエラーで全体を止めず、次のリポジトリへ進む
            continue
        except KeyboardInterrupt:
            print("\nAutomation interrupted by user. Exiting...")
            break
        except Exception as e:
            print(f"Error executing command: {e}")
            raise
        finally:
            if master is not None:
                try:
                    os.close(master)
                except OSError:
                    pass

if __name__ == "__main__":
    main()
