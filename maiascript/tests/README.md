# MaiaScript Native → RISC-V: syntax and differential tests (draft v0.1)

**Status:** test corpus and illustrative assembly references, **not** a working MaiaScript compiler. Grammar source: `MaiaScript_Native_v0_1_MaiaCC.ebnf` (included in parent working directory). All RISC-V examples target **RV32IM/ILP32** and are independent of a MaiaRTOS syscall ABI. C++ parser generation and acceptance of the grammar must be tested with the actual MaiaCC checkout.

## Layout

- `syntax/positive/*.ms`: 25 intended-valid syntax fixtures.
- `syntax/negative/*.ms`: 8 intended-invalid syntax fixtures.
- `manifest.json`: fixture-to-rule coverage map (intent, not verified coverage).
- `pairs/*/main.ms`: 8 self-contained MaiaScript programs.
- `pairs/*/main.S`: corresponding handwritten RISC-V programs.
- `pairs/*/expected.json`: expected `a0` at return.
- `tools/check_assembly.sh`: assembles all examples with clang's RISC-V integrated assembler.
- `tools/check_suite.py`: static fixture sanity and optional assembly checks.

## Suggested acceptance gates

1. **G0** grammar parsed by MaiaCC, C++ lexer/parser generated, built on host and target (if supported).
2. **G1** all intended-positive inputs accepted; all negative inputs rejected (no semantic checks in this gate).
3. **G2** parse-tree/AST assertions: operator precedence, associativity, scopes, declarations, nested calls, pointer syntax.
4. **G3** semantic-negative suite (to add): unknown types, duplicate symbols, invalid lvalues, wrong argument count, return type mismatch, unsafe violations.
5. **G4** RISC-V backend: assemble, link, execute, assert `a0`; compare generated code's behavior against reference and a host interpreter.
6. **G5** device: ELF loader, executable memory/cache synchronization, API ABI, LVGL ownership and application lifecycle; test on actual Tab5.

## Important known gaps

- The grammar has **known potential ambiguities**, including `Type NAME` vs expression statements and `Result`/`Option` handling. The fixtures are expectations, not proof of acceptance.
- No claim of full rule/branch coverage: lexical error tests, all `TypeSuffix` alternatives, deeply nested expression precedence, error recovery, class constructors, and GUI behavior need further tests.
- The LVGL example checks **only syntax**. API names and ABI are conceptual, not verified against running MaiaRTOS.
- `new`/`delete` semantics, ownership, exceptions, and memory protection are not established.
- The included local `maia-rtos/README.md` describes an architectural bootstrap, not a functioning kernel. The user's newer standalone-app changes may not be reflected in that snapshot. Do not infer runtime features from this suite.

## Assembly compilation

```sh
bash tools/check_assembly.sh
```

These are standalone functions with an ordinary `main` entry. They are not boot images or complete MaiaRTOS executables; linker/startup/ELF details are separate.

## Recommended assembler architecture

`source -> lexer/parser (MaiaCC C++ OR small handwritten lexer/parser) -> normalized Assembly IR -> symbol table -> layout/relaxation -> relocation -> instruction encoder -> ELF32 writer`.

Use MaiaCC if the generated C++ runtime is freestanding-compatible, incremental memory overhead is acceptable, error recovery is adequate, and the syntax is expressible without contortions. Handwrite a small assembly line parser if MaiaCC cannot handle register aliases, nested relocation expressions, pseudo-ops, and section directives cleanly. **Regardless, handwrite/test the encoder, relocation engine, symbol resolution, and ELF writer**; these are not grammar responsibilities.

### Recommended instruction IR

```cpp
struct AsmInstruction {
  OpcodeId opcode; // table-driven, includes ISA extension gate
  Operand operands[4];
  unsigned operand_count;
  SourceSpan location;
};
```

Keep `.text`, `.data`, `.bss`, `.align`, `.globl`, `.word`, `.byte`, `.asciz`, `.equ`, `.section`, and `%hi/%lo/%pcrel_hi/%pcrel_lo` as separately validated directives/expressions. Do not encode a pseudo-instruction until its expansion and relocations are known.

### Distinguish testing domains

- **Syntax tests**: compare parse success, diagnostics and AST.
- **Assembler tests**: compare object bytes and relocations with GNU/LLVM reference assemblers.
- **End-to-end tests**: execute linked code under emulator and actual hardware; compare return values and side effects.
- **GUI tests**: integration with MaiaRTOS desktop/standalone app lifecycle, not just parsing of `ui.Window` calls.
