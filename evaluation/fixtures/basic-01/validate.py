#!/usr/bin/env python3
"""Validation script: checks that a logging statement was added before greet() calls."""
import sys
from pathlib import Path

main_cpp = Path("src/main.cpp").read_text(encoding="utf-8")

if "Calling greet" in main_cpp:
    print("PASS: 'Calling greet' found in src/main.cpp")
    sys.exit(0)
else:
    print("FAIL: 'Calling greet' not found in src/main.cpp")
    sys.exit(1)
