# MaiaScript Native v0.1 — Grammar design and MaiaCC integration notes

**Status: candidate, NOT compiler-validated.** This document accompanies `MaiaScript_Native_v0_1_MaiaCC.ebnf`.

## Sources and evidence

- `MaiaScript.ebnf` (user-supplied): REx-style `<?TOKENS?>`, `::=`, lexical difference, character classes, `/* ws: definition */`, `$` end-of-input. Its expressions and syntax have been revised deliberately.
- `maia-shell(2).ebnf` (available reference): `EOF ::= $`, lookahead and token priority `A << B`.
- `MaiaScript_Native_Manual_Concept_v0.1.md` (conceptual manual): typed C-like functions, classes, imports, system APIs, LVGL-backed UI, native compilation.
- Public MaiaCC README (https://github.com/souzamonteiro/maiacc): `bin/tREx.sh --ebnf`, EBNF→XML→generated JavaScript parser. The public README does **not** establish that C++ generation is available in the inspected version.

## Design decisions

1. **Syntax vs runtime**: `import maia.ui;` and `ui.Window(...)` are ordinary qualified names/calls. The grammar should NOT hardcode LVGL widgets or MaiaRTOS APIs; module resolution and ABI belong to the compiler/linker.
2. **Functions**: `i32 main() { ... }` with explicit return type. No Rust `fn` or `let`.
3. **Classes**: fields, visibility, constructors, instance methods, optional `const` method. No inheritance, virtual dispatch or GC in v0.1.
4. **Expressions**: layered precedence; assignment right-associative; no exponent operator in v0.1. Lvalue validity is semantic, not lexical.
5. **Errors**: `Result<T,E>`, `Option<T>`, `?` option type; `?` error propagation operator deferred to avoid ambiguity with ternary. `defer` supported syntactically, semantics need definition.
6. **Pointer and array suffixes**: grammar allows `T*`, `T[]`, `T[123]`; safe access, ownership, stack layout, `unsafe` and bounds checks are compiler/runtime concerns.
7. **LVGL**: use `import maia.ui;`, `ui.Window`, `ui.Button` etc; actual API names are conceptual and must be matched to MaiaRTOS standalone application ABI.
8. **Assembler**: `asm("...")` is only a placeholder expression, not a complete inline assembly design. Recommend separate `.s` source and `extern` function declarations first.

## Known grammar issues requiring MaiaCC generator tests

- `NAME` vs keywords, token priorities, numeric suffixes, comment regex and `BLOCK_COMMENT` (especially `**/`) need empirical generator validation. Do not claim this grammar compiles until `tREx.sh` succeeds.
- C-like declaration/expression ambiguity: `Foo bar;` vs `foo(bar);` depends on type-name resolution. The current grammar may need tokenized TYPE_NAME or a two-stage semantic parser. This is a serious issue for deterministic parsers.
- `Type` with `*` and `[]` may overlap multiplication/indexing in ambiguous contexts. `TypeSuffix` is a syntax proposal, not proven conflict-free.
- `TypeAtom` `QualifiedName` overlaps generic `Result` and `Option`; keyword exclusion helps, but test with the actual MaiaCC parser.
- `SwitchArm ::= ... Statement*` requires checking that the parser correctly stops at `case`/`default` boundaries.
- `Postfix` can parse member chains; static type checking must validate methods, field access and calls.
- `ConstructorDecl ::= NAME (...)` conflicts with method syntax only in certain forms; check parser conflicts.
- `delete` as a primary expression and `asm` as expression are provisional. Better restrict them in v0.2.
- `STRING`/`CHAR` are ASCII-escape-only for now; UTF-8/Unicode literal semantics require a separate lexical decision.
- Grammar uses REx-like `<?TOKENS?>`, subtraction and `<<` consistent with supplied examples, but **C++ target generation remains unverified**.

## Required verification sequence

1. Run `bash bin/tREx.sh --ebnf --only-xml --to-xml /tmp/maiascript.xml MaiaScript_Native_v0_1_MaiaCC.ebnf` from the MaiaCC repository root.
2. Run the parser generation command supported by the installed MaiaCC version and capture errors/conflicts.
3. Inspect the generated lexer token priority and AST tree shape.
4. Add positive and negative fixtures and compare accepted syntax with manual examples.
5. Confirm how C++ output is actually generated: repository README currently documents JS generation, not C++ generation.
6. Only after grammar validation: implement semantic analysis, symbol table, typed IR, RV32 backend, ELF linker/loader and ABI.

## Positive fixture examples

```maiascript
import maia.console;
i32 main() { console.println("Hello"); return 0; }
```

```maiascript
class Counter {
    private i32 value = 0;
    public void increment() { value += 1; }
    public i32 getValue() const { return value; }
}
i32 main() { Counter c = Counter(); c.increment(); return c.getValue(); }
```

```maiascript
import maia.ui;
i32 main() {
    ui.Window win = ui.Window("Test", 420, 260);
    win.show();
    return app.run();
}
```

## Negative fixture examples

- `i32 main( { return 0; }` — malformed parameter list.
- `i32 main() { i32 x = ; }` — missing expression.
- `i32 main() { return 3 = 4; }` — **may parse**, must fail semantic lvalue check.
- `i32 main() { Missing x; }` — **may parse**, must fail unknown-type resolution.

## Deliberately deferred

- Native exceptions, `async/await`, lambdas, closures, interfaces, inheritance, templates, GC, reflection, operator overloads, direct LVGL handles, inline asm with constraints, and type-directed parsing of brace initializers.
- Explicit pointer ownership and error propagation operator `?` need a dedicated semantic design before being promised by the grammar.

## Important architecture constraint

A C++ parser running **inside MaiaRTOS** requires a freestanding-compatible generated runtime. MaiaCC's documented Node.js generation pipeline is a **host tool**; the resulting output cannot be assumed to run on bare metal. Validate the target backend and runtime dependencies independently.
