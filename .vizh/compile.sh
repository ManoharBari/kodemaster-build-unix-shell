#!/bin/sh
#
# Compiles the C shell program
#
set -e

cd "$(dirname "$0")/.."

# Compile with gcc
make clean >/dev/null 2>&1 || true
make

echo "✓ Compiled successfully"
exit 0
