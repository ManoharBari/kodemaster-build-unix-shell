#!/bin/sh
#
# Runs the shell program
#
set -e

cd "$(dirname "$0")/.."

# Ensure compiled
if [ ! -f "./your_program" ]; then
    make >/dev/null 2>&1
fi

# Execute the shell
exec ./your_program "$@"
