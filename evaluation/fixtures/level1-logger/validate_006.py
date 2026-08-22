#!/usr/bin/env python3
"""Task level1-006: diagnostic added to Server::log, not Connection::log."""
import sys
from pathlib import Path

def main():
    server_file = Path("src/net/server.cpp")
    conn_file = Path("src/net/connection.cpp")

    if not server_file.exists():
        print(f"FAIL: {server_file} not found")
        sys.exit(1)

    server_text = server_file.read_text(encoding="utf-8")
    if "[Server] " not in server_text:
        print("FAIL: '[Server] ' not found in server.cpp")
        sys.exit(1)

    conn_text = conn_file.read_text(encoding="utf-8")
    if "[Server] " in conn_text:
        print("FAIL: '[Server] ' was incorrectly added to connection.cpp")
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
