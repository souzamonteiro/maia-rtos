# MaiaASM grammar — implementation notes (candidate v0.1)

## Scope

The accompanying `MaiaASM_RISCV_MaiaCC_v0_1.ebnf` is a **candidate** in the
MaiaCC/REx dialect observed in the MaiaScript and MaiaOS shell grammars.
It has **not** been run through MaiaCC and may need changes to accommodate
parser-generator conflict resolution, lexer token priority, and the C++ backend.

This grammar intentionally accepts any mnemonic `NAME` and directive `.NAME`.
The opcode table and directive table validate the actual names and operand
signatures. Thus `bogus a0, a1` is syntactically valid but semantically invalid.

## Design

- Physical newlines are significant; only horizontal whitespace is skipped.
- Labels and instructions can appear on the same line.
- Labels use symbolic names. Numeric local labels `1:` and `1f`/`1b` are deferred.
- The lexer recognizes numbers, strings, identifiers, comments and punctuation.
- Memory forms `imm(base)` are recognized; `0(a0)` and `(a0)` differ:
  `(a0)` is parsed as a parenthesized expression, so the operand classifier
  must also accept this as an atomic zero-offset memory operand.
- `fence iorw, iorw` is parsed as names, then validated semantically.
- `amoadd.w.aq` is a single identifier, checked by opcode metadata.
- `%hi`, `%lo`, `%pcrel_hi`, `%pcrel_lo` are relocation expressions.
- `.option`, `.attribute`, `.cfi_*`, `.macro`, `.if`, `.rept`, `.irp`,
  and other GAS features are not fully specified in this initial grammar.
- GNU syntax permits multiple assembly statements per line separated by `;`.
  Not supported in this first version.
- The source must end in newline or lexer preprocessor must append one.
- `.section .rodata` uses a dot token followed by NAME, so directive argument
  parsing should be checked against actual MaiaCC behavior.
- `NAME` contains `.` after its first character, supporting `fadd.s` and
  `.Llocal` only if dot-leading names are added with a dedicated lexical rule.

## Validation stages

1. Generate lexer/parser C++ using the actual MaiaCC command and confirm every
   positive script is accepted and every negative script rejected.
2. Confirm tokens and parse tree for `.text`, `label:`, `lw a0, 4(sp)`,
   `amoadd.w.aq`, `%pcrel_hi(symbol)` and comments.
3. Resolve symbol names and expressions, validate register classes and operand
   kinds against the ISA table, and enforce immediate ranges/alignment.
4. Assemble against LLVM/GNU binutils and compare emitted bytes/relocations.
5. Test ELF32 output and execution on target/simulator.

## Required emitter responsibilities

Two-pass or multi-pass symbol resolution; section layout; instruction encodings;
RV32I/M/A/F/C and vendor extensions selected by target; PC-relative branch
and jump ranges; compressed instruction selection; relocations; ELF32 attributes;
ABI; alignment; and diagnostics. The grammar does not implement these.

## Suggested evolution

For an assembler intended to support all GNU assembly constructs, introduce
lexer states or a dedicated preprocessor for macro bodies and conditional
assembly. If MaiaCC has difficulties with the operand/Expr ambiguity, use a
minimal line parser that emits a raw operand-token list, then parse operands
with an independent expression parser. This retains MaiaCC for the high-level
syntax while keeping the ISA logic data-driven.
