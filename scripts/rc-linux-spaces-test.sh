#!/bin/bash
# RC validation: path-with-spaces test on Linux

EXE=/home/taqu/rc-test/ast-tool-0.1.0-source/bin/ast-tool
SRC=/mnt/d/Projects/Cpp/temp/ast-tool/evaluation/repositories/level1-store/src

mkdir -p "/tmp/with spaces/src"
cp -r "$SRC/." "/tmp/with spaces/src/"

echo "=== path without spaces ==="
"$EXE" search --name orders "$SRC" 2>&1
echo "exit=$?"

echo "=== path with spaces ==="
"$EXE" search --name orders "/tmp/with spaces/src" 2>&1
echo "exit=$?"

rm -rf "/tmp/with spaces"
