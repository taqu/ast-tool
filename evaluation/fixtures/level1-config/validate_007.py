#!/usr/bin/env python3
"""Task level1-007: log line added to ConfigManager::validate, not SchemaValidator::validate."""
import sys
from pathlib import Path

def main():
    config_file = Path("src/config/config_manager.cpp")
    schema_file = Path("src/config/schema_validator.cpp")

    if not config_file.exists():
        print(f"FAIL: {config_file} not found")
        sys.exit(1)

    config_text = config_file.read_text(encoding="utf-8")
    if "[ConfigManager] validate called" not in config_text:
        print("FAIL: log line '[ConfigManager] validate called' not found in config_manager.cpp")
        sys.exit(1)

    schema_text = schema_file.read_text(encoding="utf-8")
    if "[ConfigManager] validate called" in schema_text:
        print("FAIL: log line was incorrectly added to schema_validator.cpp")
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
