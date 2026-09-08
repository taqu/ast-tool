#!/bin/bash
EXE=/home/taqu/rc-test/ast-tool-0.1.0-source/bin/ast-tool

echo "=== invalid path ==="
"$EXE" search --name main /nonexistent_path 2>/dev/null
echo "exit=$?"

echo "=== empty result ==="
"$EXE" search --name nonexistent_xyz /mnt/d/Projects/Cpp/temp/ast-tool/evaluation/repositories/level1-store/src 2>/dev/null
echo "exit=$?"

echo "=== unknown command ==="
"$EXE" badcommand 2>/dev/null
echo "exit=$?"

echo "=== success ==="
"$EXE" search --name orders /mnt/d/Projects/Cpp/temp/ast-tool/evaluation/repositories/level1-store/src > /dev/null 2>&1
echo "exit=$?"
