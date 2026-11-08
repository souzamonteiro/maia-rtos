#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
MAIAASM="../../assembler/maiaasm"

for src in pairs/*/main.S; do
  out="${src%.S}.bin"
  if [ -x "$MAIAASM" ]; then
    "$MAIAASM" "$src" -o "$out"
    echo "OK (maiaasm) $src"
  elif command -v riscv32-esp-elf-as >/dev/null 2>&1; then
    riscv32-esp-elf-as -march=rv32im -mabi=ilp32 "$src" -o "${src%.S}.o"
    echo "OK (riscv32-esp-elf-as) $src"
  else
    echo "Skipping $src (no assembler found)"
  fi
done
