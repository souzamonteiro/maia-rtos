#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHELL_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ROOT_DIR="$(cd "$SHELL_DIR/.." && pwd)"
TREX="$ROOT_DIR/tools/maiacc/bin/tREx.sh"

if [[ ! -x "$TREX" ]]; then
    chmod +x "$TREX"
fi

GRAMMAR_DIR="$SHELL_DIR/grammar"
GEN_DIR="$SHELL_DIR/generated"

mkdir -p "$GEN_DIR"

echo "=== Generating MaiaShell Parsers using MaiaCC ==="

echo "[1/5] maia-shell.ebnf -> maia-shell-parser.hpp"
bash "$TREX" --ebnf --language cpp98 --to-xml "$GEN_DIR/maia-shell.xml" \
    "$GRAMMAR_DIR/maia-shell.ebnf" "$GEN_DIR/maia-shell-parser.hpp"

echo "[2/5] maia-shell-expr.ebnf -> shell-expr-parser.hpp"
bash "$TREX" --ebnf --language cpp98 --to-xml "$GEN_DIR/shell-expr.xml" \
    "$GRAMMAR_DIR/maia-shell-expr.ebnf" "$GEN_DIR/shell-expr-parser.hpp"

echo "[3/5] maia-shell-test.ebnf -> shell-test-parser.hpp"
bash "$TREX" --ebnf --language cpp98 --to-xml "$GEN_DIR/shell-test.xml" \
    "$GRAMMAR_DIR/maia-shell-test.ebnf" "$GEN_DIR/shell-test-parser.hpp"

echo "[4/5] maia-shell-word.ebnf -> shell-word-parser.hpp"
bash "$TREX" --ebnf --language cpp98 --to-xml "$GEN_DIR/shell-word.xml" \
    "$GRAMMAR_DIR/maia-shell-word.ebnf" "$GEN_DIR/shell-word-parser.hpp"

echo "[5/5] maia-shell-return.ebnf -> shell-return-parser.hpp"
bash "$TREX" --ebnf --language cpp98 --to-xml "$GEN_DIR/shell-return.xml" \
    "$GRAMMAR_DIR/maia-shell-return.ebnf" "$GEN_DIR/shell-return-parser.hpp"

echo "=== All MaiaShell parsers generated successfully! ==="

