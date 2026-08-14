from asyncio.windows_events import NULL
from pathlib import Path
import glob
from collections import defaultdict
import json

CLAUDE_LOG_DIR = Path("~/.claude/projects").expanduser()

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
    efficiency_stats = parse_session_logs()
    print(f"\n[Metrics] Execution Summary:")
    print(f"  - Total Input Tokens:  {efficiency_stats['tokens']['input']:,}")
    print(f"  - Total Output Tokens: {efficiency_stats['tokens']['output']:,}")
    print(f"  - Tool Calls:          {dict(efficiency_stats['tools'])}\n")


if __name__ == "__main__":
    main()

