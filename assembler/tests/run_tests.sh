#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$DIR/.."
MAIAASM="$ROOT/maiaasm"
GNU_AS="/Users/roberto/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin/riscv32-esp-elf-as"
GNU_OBJCOPY="/Users/roberto/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin/riscv32-esp-elf-objcopy"

echo "=========================================================="
echo "   MaiaASM vs GNU RISC-V Assembler Differential Suite     "
echo "=========================================================="

# 1. Executa suite interna do Disassembler e Roundtrip
echo ""
echo "[1/2] Running Unit Disassembler & Roundtrip Tests..."
"$ROOT/test_disasm"
"$ROOT/test_roundtrip"

# 2. Executa suite diferencial contra o GNU as
echo ""
echo "[2/2] Running Differential Binary Verification against GNU as..."
TESTS=("01_basic.s" "02_labels.s" "05_expression.s" "06_fpu_atomic.s")
PASSED=0
TOTAL=${#TESTS[@]}

for t in "${TESTS[@]}"; do
    SRC="$DIR/positive/$t"
    GCC_O="/tmp/gcc_$t.o"
    GCC_BIN="/tmp/gcc_$t.bin"
    MAIA_BIN="/tmp/maia_$t.bin"

    "$GNU_AS" -march=rv32imaf -mabi=ilp32f "$SRC" -o "$GCC_O"
    "$GNU_OBJCOPY" -O binary --only-section=.text "$GCC_O" "$GCC_BIN"
    "$MAIAASM" "$SRC" -o "$MAIA_BIN" > /dev/null

    if cmp -s "$GCC_BIN" "$MAIA_BIN"; then
        echo "  [PASS] $t matches GNU as byte-for-byte!"
        PASSED=$((PASSED + 1))
    else
        echo "  [FAIL] $t differs from GNU as!"
    fi

    rm -f "$GCC_O" "$GCC_BIN" "$MAIA_BIN"
done

echo ""
echo "=========================================================="
echo "Result: $PASSED / $TOTAL differential tests passed byte-for-byte!"
echo "=========================================================="

