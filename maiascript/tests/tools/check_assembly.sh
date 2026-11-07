#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${CLANG:=clang}"
for src in pairs/*/main.S; do
  out="${src%.S}.o"
  "$CLANG" --target=riscv32 -march=rv32im -mabi=ilp32 -c "$src" -o "$out"
  echo "OK $src"
done
