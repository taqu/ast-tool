import os
import json
import subprocess
from pathlib import Path
from datasets import load_dataset
from git import Repo

cache_dir = Path("cache").resolve()
output_preds_path = Path("predictions.jsonl").resolve()

def main():
    print("Loading SWE-bench Multilingual dataset...")
    dataset = load_dataset("SWE-bench/SWE-bench_Multilingual", split="test", cache_dir=cache_dir)
    
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
        
        local_dir = os.path.abspath(os.path.join("workspace", f"{instance_id}"))
        github_url = f"https://github.com/{repo_name}.git"
        
        # リポジトリ準備
        if not os.path.exists(local_dir):
            try:
                repo = Repo.clone_from(github_url, local_dir)
            except Exception as e:
                print(f"Clone failed: {e}")
                continue
        else:
            repo = Repo(local_dir)
            
        try:
            repo.git.checkout(base_commit)
        except Exception as e:
            print(f"Checkout failed: {e}")
            continue
        
        # 問題文の保存
        instruction_file = os.path.join(local_dir, "SWE_PROBLEM_STATEMENT.md")
        with open(instruction_file, "w", encoding="utf-8") as f:
            f.write(problem_statement)
            
        continue
        # --- Claude Code 完全自動実行（対策B + アルファ） ---
        claude_instruction = (
            "Read 'SWE_PROBLEM_STATEMENT.md' to understand the issue. "
            "Locate the bug, modify the source code to fix it, and ensure all relevant tests pass. "
            "Once fixed, exit automatically without asking."
        )
        
        env_config = os.environ.copy()
        env_config["CLAUDE_CODE_TRUST_ALL"] = "1"  # Workspace Trustの確認をスキップ
        env_config["CI"] = "true"                  # ヘッドレス環境として明示
        
        try:
            # 1つのリポジトリがドツボにハマった時のためにタイムアウト（例: 15分=900秒）を設定
            subprocess.run(
                [
                    "claude", 
                    "-c", claude_instruction,
                    "--dangerously-skip-permissions"
                ], 
                cwd=local_dir, 
                env=env_config, 
                check=True,
                stdin=subprocess.DEVNULL,
                timeout=900 
            )
            
            # --- 実行完了後、Git差分（パッチ）を抽出して保存 ---
            patch_diff = repo.git.diff()
            
            if patch_diff.strip():
                prediction_entry = {
                    "instance_id": instance_id,
                    "model_patch": patch_diff,
                    "model_name_or_path": "claude-code-agent"
                }
                
                # SWE-bench公式フォーマット(JSONL)で追記保存
                with open(output_preds_path, "a", encoding="utf-8") as f:
                    f.write(json.dumps(prediction_entry) + "\n")
                print(f"[Success] Extracted patch for {instance_id}")
            else:
                print(f"[Info] Claude finished but no code changes were made.")
                
        except subprocess.TimeoutExpired:
            print(f"[Timeout] Claude exceeded time limit on {instance_id}. Skipping...")
            continue
        except subprocess.CalledProcessError as e:
            print(f"[Warning] Claude returned error code: {e}. Skipping...")
            continue
        except KeyboardInterrupt:
            print("\nInterrupted by user. Exiting...")
            break

    # --- 全リポジトリ巡回後、最後に公式評価ハーネスを叩いて自動集計 ---
    if output_preds_path.exists():
        print("\n=============================================")
        print("All repos processed. Starting SWE-bench Evaluation...")
        print("=============================================\n")
        
        eval_command = [
            "python", "-m", "swebench.harness.run_evaluation",
            "--dataset_name", "SWE-bench/SWE-bench_Multilingual",
            "--predictions_path", str(output_preds_path),
            "--run_id", "claude_code_eval_run"
        ]
        # 評価を実行（内部でDockerが立ち上がりテストが回ります）
        subprocess.run(eval_command)
    else:
        print("\nNo patches were generated. Evaluation skipped.")

if __name__ == "__main__":
    main()
