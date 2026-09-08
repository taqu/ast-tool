#!/bin/bash
# Package Linux binary RC artifact and verify it

set -e

VERSION=0.1.0
BUILT_BIN=/home/taqu/rc-test/ast-tool-0.1.0-source/bin/ast-tool
REPO_ROOT=/mnt/d/Projects/Cpp/temp/ast-tool
ARTIFACTS=$REPO_ROOT/release-artifacts
NAME="ast-tool-${VERSION}-linux-x64"

echo "==> Staging $NAME..."
rm -rf "/tmp/$NAME"
mkdir -p "/tmp/$NAME"
cp "$BUILT_BIN"          "/tmp/$NAME/ast-tool"
cp "$REPO_ROOT/README.md"    "/tmp/$NAME/README.md"
cp "$REPO_ROOT/LICENSE"      "/tmp/$NAME/LICENSE"
cp "$REPO_ROOT/CHANGELOG.md" "/tmp/$NAME/CHANGELOG.md"
cp "$REPO_ROOT/NOTICE"       "/tmp/$NAME/NOTICE"

echo "==> Creating archive..."
mkdir -p "$ARTIFACTS"
tar -czf "$ARTIFACTS/${NAME}.tar.gz" -C /tmp "$NAME"
echo "size=$(du -sh "$ARTIFACTS/${NAME}.tar.gz" | cut -f1)"

echo "==> Checksum..."
sha256sum "$ARTIFACTS/${NAME}.tar.gz" >> "$ARTIFACTS/SHA256SUMS"
echo "SHA256SUMS updated"

echo "==> Post-packaging binary verification..."
TMPVERIFY=$(mktemp -d)
tar -xzf "$ARTIFACTS/${NAME}.tar.gz" -C "$TMPVERIFY"

echo "--- --version ---"
"$TMPVERIFY/$NAME/ast-tool" --version
echo "exit=$?"

echo "--- --help (first 5 lines) ---"
"$TMPVERIFY/$NAME/ast-tool" --help 2>&1 | head -5
echo "exit=$?"

echo "--- semantic smoke ---"
"$TMPVERIFY/$NAME/ast-tool" search --name orders /mnt/d/Projects/Cpp/temp/ast-tool/evaluation/repositories/level1-store/src 2>&1
echo "exit=$?"

echo "--- JSON smoke ---"
"$TMPVERIFY/$NAME/ast-tool" callers store::OrderService::save /mnt/d/Projects/Cpp/temp/ast-tool/evaluation/repositories/level1-store/src --json 2>&1
echo "exit=$?"

rm -rf "$TMPVERIFY" "/tmp/$NAME"
echo "==> Linux binary packaging DONE"
