#! /bin/sh
#
# run.sh — compile and run the stat-penetrates-jail reproducer.
#
#   sh run.sh
#
# Defaults to probing $HOME (which the sandbox does not allow).
# Override with:
#   FORBIDDEN=/some/dir        the path to probe
#   LLJAIL=/path/to/lljail     the sandbox executable
#   CC=clang                   the compiler to use

set -e
cd "$(dirname "$0")"

FORBIDDEN="${FORBIDDEN:-$HOME}"
export LLJAIL
CC="${CC:-cc}"

"${CC}" -O0 -o statjail statjail.c

echo "### 1. bare run — control (no jail)"
./statjail "${FORBIDDEN}"

echo
echo "### 2. under ./sandbox — bare-bones lljail ('${FORBIDDEN}' not allowed)"
sh ./sandbox ./statjail "${FORBIDDEN}"
