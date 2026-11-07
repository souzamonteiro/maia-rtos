# MaiaASM — Referência de implementação RISC-V para MaiaRTOS
**Edição 0.1 · 7 novembro 2026 · Escopo: RV32I + M + A + F + Zicsr + Zifencei, com roteiro C/Zb/Zc e extensões Espressif.**
> **Limite de cobertura:** esta edição é uma especificação operacional completa do núcleo RV32I/M/A/F e das instruções Zicsr/Zifencei listadas abaixo, mas **não** é ainda o catálogo exaustivo de todas as extensões RISC-V existentes nem dos encodings proprietários XespV/XespLoop. Para C, Zb, Zc e instruções proprietárias, a tabela exata deve ser importada dos arquivos oficiais versionados; nunca gerar opcodes por adivinhação.
## 1. Perfil do alvo
ESP32-P4: RV32IMAFC, extensões Zb e Zc (incluindo Zcb, Zcmp, Zcmt conforme revisão), XespV e XespLoop. Não presumir extensão D. Selecionar `-march` conforme revisão real, binário do kernel e ABI efetiva. O assembler deve aceitar `.option arch`, rejeitar instruções de extensões desabilitadas e emitir atributos ELF coerentes. Separar modo usuário de instruções privilegiadas.
## 2. Fontes normativas
- https://docs.riscv.org/reference/isa/v20260120/index.html
- https://github.com/riscv/riscv-opcodes — arquivos `extensions/`, `arg_lut.csv`, `csrs.csv`, `csrs32.csv`, gerador de `instr_dict.json`, máscaras e matches.
- https://documentation.espressif.com/esp32-p4_datasheet_en.html
- https://documentation.espressif.com/esp32-p4_technical_reference_manual_en.pdf
- https://github.com/riscv-non-isa/riscv-elf-psabi-doc — ABI ELF, relocations, convenção de chamada.
## 3. Formatos de instrução (32 bits)
Bit numbering é LSB=0; palavras de instrução emitidas em little-endian. Campos:
```text
R: funct7[31:25] rs2[24:20] rs1[19:15] funct3[14:12] rd[11:7] opcode[6:0]
I: imm[11:0][31:20] rs1[19:15] funct3[14:12] rd[11:7] opcode[6:0]
S: imm[11:5][31:25] rs2[24:20] rs1[19:15] funct3[14:12] imm[4:0][11:7] opcode[6:0]
B: imm[12][31] imm[10:5][30:25] rs2 rs1 funct3 imm[4:1][11:8] imm[11][7] opcode
U: imm[31:12][31:12] rd[11:7] opcode[6:0]
J: imm[20][31] imm[10:1][30:21] imm[11][20] imm[19:12][19:12] rd opcode
```
Offsets B/J são relativos ao endereço da instrução, assinados e múltiplos de 2. Não confundir PC relativo com PC+4. A compressão altera a largura e o alcance do deslocamento; desabilitar relaxação inicialmente.
### 3.1 Construtores de palavra
```cpp
uint32_t R(unsigned f7,unsigned rs2,unsigned rs1,unsigned f3,unsigned rd,unsigned op) { return (f7<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op; }
uint32_t I(int32_t imm,unsigned rs1,unsigned f3,unsigned rd,unsigned op) { return ((uint32_t(imm)&0xfff)<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op; }
uint32_t S(int32_t imm,unsigned rs2,unsigned rs1,unsigned f3,unsigned op) { unsigned u=uint32_t(imm)&0xfff; return ((u>>5)<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|((u&31)<<7)|op; }
uint32_t B(int32_t imm,unsigned rs2,unsigned rs1,unsigned f3,unsigned op) { unsigned u=uint32_t(imm)&0x1fff; return (((u>>12)&1)<<31)|(((u>>5)&63)<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|(((u>>1)&15)<<8)|(((u>>11)&1)<<7)|op; }
uint32_t U(uint32_t imm20,unsigned rd,unsigned op) { return ((imm20&0xfffff)<<12)|(rd<<7)|op; }
uint32_t J(int32_t imm,unsigned rd,unsigned op) { unsigned u=uint32_t(imm)&0x1fffff; return (((u>>20)&1)<<31)|(((u>>1)&1023)<<21)|(((u>>11)&1)<<20)|(((u>>12)&255)<<12)|(rd<<7)|op; }
```
Validar limites **antes** de mascarar: I/S 12-bit signed; B signed 13-bit com bit0=0; J signed 21-bit com bit0=0; shifts RV32 0..31; CSR 12-bit unsigned. `lui`/`auipc` recebem campo de 20 bits (convenção do assembler), não um valor arbitrário sem normalização.
## 4. Catálogo RV32I — opcode principal, funct3, funct7
`—` indica campo variável ou não aplicável. A coluna `match` é a palavra com operandos zero; para máscaras precisas, consulte a seção 6. `SYSTEM` especiais têm match completo.
| Mnemônico | Formato | opcode | funct3 | funct7/imm12 | Operandos |
|---|---|---|---|---|---|
| lui | U | 0x37 | — | — | rd, imm20 |
| auipc | U | 0x17 | — | — | rd, imm20 |
| jal | J | 0x6f | — | — | rd, target |
| jalr | I | 0x67 | 0 | — | rd, imm(rs1) |
| beq | B | 0x63 | 0 | — | rs1, rs2, target |
| bne | B | 0x63 | 1 | — | rs1, rs2, target |
| blt | B | 0x63 | 4 | — | rs1, rs2, target |
| bge | B | 0x63 | 5 | — | rs1, rs2, target |
| bltu | B | 0x63 | 6 | — | rs1, rs2, target |
| bgeu | B | 0x63 | 7 | — | rs1, rs2, target |
| lb | I | 0x03 | 0 | — | rd, imm(rs1) |
| lh | I | 0x03 | 1 | — | rd, imm(rs1) |
| lw | I | 0x03 | 2 | — | rd, imm(rs1) |
| lbu | I | 0x03 | 4 | — | rd, imm(rs1) |
| lhu | I | 0x03 | 5 | — | rd, imm(rs1) |
| sb | S | 0x23 | 0 | — | rs2, imm(rs1) |
| sh | S | 0x23 | 1 | — | rs2, imm(rs1) |
| sw | S | 0x23 | 2 | — | rs2, imm(rs1) |
| addi | I | 0x13 | 0 | — | rd, rs1, imm |
| slti | I | 0x13 | 2 | — | rd, rs1, imm |
| sltiu | I | 0x13 | 3 | — | rd, rs1, imm |
| xori | I | 0x13 | 4 | — | rd, rs1, imm |
| ori | I | 0x13 | 6 | — | rd, rs1, imm |
| andi | I | 0x13 | 7 | — | rd, rs1, imm |
| slli | I-shamt | 0x13 | 1 | 0x00 | rd, rs1, shamt |
| srli | I-shamt | 0x13 | 5 | 0x00 | rd, rs1, shamt |
| srai | I-shamt | 0x13 | 5 | 0x20 | rd, rs1, shamt |
| add | R | 0x33 | 0 | 0x00 | rd, rs1, rs2 |
| sub | R | 0x33 | 0 | 0x20 | rd, rs1, rs2 |
| sll | R | 0x33 | 1 | 0x00 | rd, rs1, rs2 |
| slt | R | 0x33 | 2 | 0x00 | rd, rs1, rs2 |
| sltu | R | 0x33 | 3 | 0x00 | rd, rs1, rs2 |
| xor | R | 0x33 | 4 | 0x00 | rd, rs1, rs2 |
| srl | R | 0x33 | 5 | 0x00 | rd, rs1, rs2 |
| sra | R | 0x33 | 5 | 0x20 | rd, rs1, rs2 |
| or | R | 0x33 | 6 | 0x00 | rd, rs1, rs2 |
| and | R | 0x33 | 7 | 0x00 | rd, rs1, rs2 |
| fence | I-special | 0x0f | 0 | fm/pred/succ | pred, succ |
| ecall | I-fixed | 0x73 | 0 | imm12=0x000 | — |
| ebreak | I-fixed | 0x73 | 0 | imm12=0x001 | — |
### 4.1 Extensão M
| Mnemônico | Formato | opcode | funct3 | funct7 | Operandos |
|---|---|---|---|---|---|
| mul | R | 0x33 | 0 | 0x01 | rd, rs1, rs2 |
| mulh | R | 0x33 | 1 | 0x01 | rd, rs1, rs2 |
| mulhsu | R | 0x33 | 2 | 0x01 | rd, rs1, rs2 |
| mulhu | R | 0x33 | 3 | 0x01 | rd, rs1, rs2 |
| div | R | 0x33 | 4 | 0x01 | rd, rs1, rs2 |
| divu | R | 0x33 | 5 | 0x01 | rd, rs1, rs2 |
| rem | R | 0x33 | 6 | 0x01 | rd, rs1, rs2 |
| remu | R | 0x33 | 7 | 0x01 | rd, rs1, rs2 |
### 4.2 Extensão A (RV32)
Opcode `0x2f`, funct3=`010`; bits [31:27]=funct5, [26]=aq, [25]=rl. `lr.w` exige rs2=0. `aq`/`rl` são sufixos `.aq`, `.rl`, `.aqrl`.
| Mnemônico | funct5 | Operandos |
|---|---|---|
| lr.w | 0x02 | rd, (rs1) |
| sc.w | 0x03 | rd, rs2, (rs1) |
| amoswap.w | 0x01 | rd, rs2, (rs1) |
| amoadd.w | 0x00 | rd, rs2, (rs1) |
| amoxor.w | 0x04 | rd, rs2, (rs1) |
| amoand.w | 0x0c | rd, rs2, (rs1) |
| amoor.w | 0x08 | rd, rs2, (rs1) |
| amomin.w | 0x10 | rd, rs2, (rs1) |
| amomax.w | 0x14 | rd, rs2, (rs1) |
| amominu.w | 0x18 | rd, rs2, (rs1) |
| amomaxu.w | 0x1c | rd, rs2, (rs1) |
### 4.3 Extensão Zicsr e Zifencei
| Mnemônico | opcode | funct3 | Operandos |
|---|---|---|---|
| csrrw | 0x73 | 1 | rd, csr, rs1 |
| csrrs | 0x73 | 2 | rd, csr, rs1 |
| csrrc | 0x73 | 3 | rd, csr, rs1 |
| csrrwi | 0x73 | 5 | rd, csr, zimm |
| csrrsi | 0x73 | 6 | rd, csr, zimm |
| csrrci | 0x73 | 7 | rd, csr, zimm |
| fence.i | 0x0f | 1 | — |
`fence.i` é necessário para coerência instruções/dados após geração de código, **se** a plataforma implementar Zifencei e conforme protocolo de cache do ESP32-P4; não presumir que uma instrução isolada substitua manutenção de cache específica do SoC.
### 4.4 Extensão F (single precision)
Registros `f0..f31`. Instruções com `rm` usam rounding mode [14:12] (000 RNE, 001 RTZ, 010 RDN, 011 RUP, 100 RMM, 111 DYN; 101/110 reservados). `rs2` é campo fixo em operações unárias e conversões; `fmt` nos fused ops é 00 para S.
| Mnemônico | opcode | funct7 (ou fmt) | rs2 fixo | Operandos |
|---|---|---|---|---|
| flw | 0x07 | — | — | fd, imm(rs1); funct3=010 |
| fsw | 0x27 | — | — | fs2, imm(rs1); funct3=010 |
| fmadd.s | 0x43 | fmt=00 | — | fd, fs1, fs2, fs3, rm |
| fmsub.s | 0x47 | fmt=00 | — | fd, fs1, fs2, fs3, rm |
| fnmsub.s | 0x4b | fmt=00 | — | fd, fs1, fs2, fs3, rm |
| fnmadd.s | 0x4f | fmt=00 | — | fd, fs1, fs2, fs3, rm |
| fadd.s | 0x53 | 0x00 | — | fd, fs1, fs2, rm |
| fsub.s | 0x53 | 0x04 | — | fd, fs1, fs2, rm |
| fmul.s | 0x53 | 0x08 | — | fd, fs1, fs2, rm |
| fdiv.s | 0x53 | 0x0c | — | fd, fs1, fs2, rm |
| fsqrt.s | 0x53 | 0x2c | 0 | fd, fs1, rm |
| fsgnj.s | 0x53 | 0x10 | — | fd, fs1, fs2; funct3=000 |
| fsgnjn.s | 0x53 | 0x10 | — | fd, fs1, fs2; funct3=001 |
| fsgnjx.s | 0x53 | 0x10 | — | fd, fs1, fs2; funct3=010 |
| fmin.s | 0x53 | 0x14 | — | fd, fs1, fs2; funct3=000 |
| fmax.s | 0x53 | 0x14 | — | fd, fs1, fs2; funct3=001 |
| fcvt.w.s | 0x53 | 0x60 | 0 | rd, fs1, rm |
| fcvt.wu.s | 0x53 | 0x60 | 1 | rd, fs1, rm |
| fmv.x.w | 0x53 | 0x70 | 0 | rd, fs1; funct3=000 |
| fclass.s | 0x53 | 0x70 | 0 | rd, fs1; funct3=001 |
| feq.s | 0x53 | 0x50 | — | rd, fs1, fs2; funct3=010 |
| flt.s | 0x53 | 0x50 | — | rd, fs1, fs2; funct3=001 |
| fle.s | 0x53 | 0x50 | — | rd, fs1, fs2; funct3=000 |
| fcvt.s.w | 0x53 | 0x68 | 0 | fd, rs1, rm |
| fcvt.s.wu | 0x53 | 0x68 | 1 | fd, rs1, rm |
| fmv.w.x | 0x53 | 0x78 | 0 | fd, rs1; funct3=000 |
## 5. Pseudo-instruções (não têm opcode próprio)
| Pseudo | Expansão básica |
|---|---|
| `nop` | `addi x0,x0,0` |
| `mv rd,rs` | `addi rd,rs,0` |
| `not rd,rs` | `xori rd,rs,-1` |
| `neg rd,rs` | `sub rd,x0,rs` |
| `seqz rd,rs` | `sltiu rd,rs,1` |
| `snez rd,rs` | `sltu rd,x0,rs` |
| `j label` | `jal x0,label` |
| `jr rs` | `jalr x0,0(rs)` |
| `ret` | `jalr x0,0(ra)` |
| `call sym` | `auipc ra,%pcrel_hi(sym); jalr ra,%pcrel_lo(sym)(ra) [relocation pair]` |
| `tail sym` | `auipc t1,%pcrel_hi(sym); jalr x0,%pcrel_lo(sym)(t1)` |
| `li rd,imm` | `addi se 12-bit signed; senão lui + addi (cuidar arredondamento hi20)` |
| `la rd,sym` | `sequência dependente de PIC/medlow/medany e relocations` |
## 6. Máscara e match: representação para lexer/parser/emitter
Para R: `mask = 0xfe00707f`, `match = (funct7<<25)|(funct3<<12)|opcode`. Para I ordinária: `mask = 0x0000707f`; para shift-immediate RV32: `mask = 0xfe00707f`; para load/store/branch: `mask = 0x0000707f`; para U/J: `mask = 0x0000007f`. Para AMO: máscara base `0xf800707f`, mais restrição rs2=0 em LR; aq/rl ficam variáveis. Para F, cada operação tem máscara dependente de campos fixos (rs2, funct3, rm), portanto gerar do catálogo oficial em vez de aplicar máscara genérica. Para ECALL `match=0x00000073`, EBREAK `match=0x00100073`, `mask=0xffffffff`. Para FENCE.I `match=0x0000100f`, `mask=0xffffffff` na forma canônica.
**Recomendação:** tabela gerada automaticamente com `mnemonic`, `isa_extension`, `length`, `mask`, `match`, `format`, `operands`, `immediate_kind`, `constraints`, `reloc_kind`, `privilege`. O parser identifica formas de operandos; o emitter seleciona descriptor + campos e valida ISA.
## 7. Lexer e parser para MaiaCC
Tokens: identificador (`[A-Za-z_.$][A-Za-z0-9_.$]*` com cautela para nomes locais `1f/1b`), registradores inteiros x0..x31 + aliases ABI, registradores flutuantes f0..f31 + aliases, inteiro decimal/hex/binário com sinal unário, string com escapes, diretivas `.text` etc., operadores `+ - * / % << >> & | ^ ~`, vírgula, `:`, `(`, `)`, newline, comentário `#` e `//` se adotado. **Não** tornar mnemônicos palavras reservadas globais: reconhecer na posição inicial de instrução, pois nomes podem aparecer como símbolos.
```ebnf
Source          ::= Line* EOF
Line            ::= Label* (Directive | Instruction)? LineEnd
Label           ::= Symbol ":"
Directive       ::= DirectiveName OperandList?
Instruction     ::= Mnemonic OperandList?
OperandList     ::= Operand ("," Operand)*
Operand         ::= MemoryOperand | Expression | Register | RelocExpression
MemoryOperand   ::= Expression? "(" Register ")"
RelocExpression ::= "%" Symbol "(" Expression ")"
Expression      ::= ... ; /* precedence-aware AST, no evaluation in lexer */
```
Esta EBNF é **conceitual**; adaptar ao dialeto REx/MaiaCC (`<?TOKENS?>`, prioridades e whitespace) após inspecionar gramáticas compiladas no repositório. Diferenciar `addi a0,a1,4` de `lw a0,4(sp)` no nível sintático/semântico. Linhas são significativas; preservar newline e permitir `;` separador se desejado.
## 8. Diretivas mínimas
`.text`, `.data`, `.rodata`, `.bss`, `.section`, `.globl`/`.global`, `.local`, `.weak`, `.type`, `.size`, `.align`, `.p2align`, `.balign`, `.byte`, `.half`, `.word`, `.zero`, `.space`, `.ascii`, `.asciz`, `.string`, `.equ`, `.set`, `.option rvc/norvc`, `.option arch`, `.attribute`, `.file`, `.ident`. Definir explicitamente se `.align` significa potência de dois ou alinhamento em bytes; preferir `.p2align`/`.balign` para evitar ambiguidade. `.org` somente com restrições e sem retrocesso. Não implementar macros/condicionais inicialmente.
## 9. Símbolos, duas passagens e relaxação
Passagem 1: tokenização, AST, tabelas de seções, labels, offsets, expressões constantes; emitir placeholders e fixups. Passagem 2: resolver símbolos locais e gerar relocations para símbolos externos/relativos. Uma vez habilitada a compressão C e relaxação de branch/call, tamanhos mudam: preferir inicialmente **sem relaxação** e algoritmo de layout determinístico; implementar relaxação iterativa posteriormente.
## 10. ELF32 RISC-V e relocations
ELF little-endian, `e_machine=EM_RISCV (243)`, `ELFCLASS32`, `ET_REL` para `.o`, `ET_EXEC`/`ET_DYN` somente após definir loader. Seções `.text/.data/.rodata/.bss/.symtab/.strtab/.shstrtab/.rela.text` etc. Usar RELA quando aplicável; adicionar `R_RISCV_32`, `R_RISCV_BRANCH`, `R_RISCV_JAL`, `R_RISCV_CALL`, `R_RISCV_CALL_PLT`, `R_RISCV_PCREL_HI20`, `R_RISCV_PCREL_LO12_I`, `R_RISCV_PCREL_LO12_S`, `R_RISCV_HI20`, `R_RISCV_LO12_I`, `R_RISCV_LO12_S`, `R_RISCV_RVC_BRANCH`, `R_RISCV_RVC_JUMP`, `R_RISCV_RELAX` somente com regras oficiais psABI. `PCREL_LO12` referencia label da instrução `AUIPC`, não diretamente o símbolo alvo; é fonte comum de bugs. Para executáveis em RAM, cuidar da coerência I-cache/D-cache e permissões de execução.
## 11. ABI do MaiaRTOS
Convenção RV32 ILP32/ILP32F conforme FPU e compatibilidade do kernel; registradores a0-a7 argumentos/retornos, ra retorno, sp alinhamento ABI (16 bytes em ABI padrão), s0-s11 callee-saved, t0-t6 caller-saved. Verificar atributos ELF `Tag_RISCV_arch`, flags RVC/float ABI. Não invocar APIs nativas por endereços fixos; usar tabela versionada de símbolos/syscalls e resolver no loader. Se usar user mode/PMP, definir permissões e transição de privilégio.
## 12. Validação diferencial e vetores de teste
```asm
.text
.globl main
main:
  addi a0, zero, 10
  addi a1, zero, 20
  add a0, a0, a1
  ret
```
Bytes esperados little-endian: `13 05 a0 00` (addi a0,x0,10); `93 05 40 01` (addi a1,x0,20); `33 05 b5 00` (add a0,a0,a1); `67 80 00 00` (ret). Confirmar em GNU `riscv32-unknown-elf-as/objdump` ou LLVM MC. Testar extremos dos imediatos, sinais, alinhamento, branch PC-relative, registradores inválidos, CSR inválido, extensões desligadas, ELF symbol/reloc e round-trip assembler→disassembler. Para extensão C, adicionar testes de 16 bits e endereço PC+2.
## 13. Extensões que exigem catálogo adicional antes de declarar cobertura completa
- **C (RVC):** quadrantes 00/01/10, CI/CIW/CL/CS/CA/CB/CJ/CR/CSS; inclui c.addi4spn, c.lw, c.sw, c.addi, c.jal (RV32), c.li, c.addi16sp, c.lui, c.srli, c.srai, c.andi, c.sub, c.xor, c.or, c.and, c.j, c.beqz, c.bnez, c.slli, c.lwsp, c.jr, c.mv, c.ebreak, c.jalr, c.add, c.swsp. Encodings 16-bit exatos devem vir do `riscv-opcodes` oficial.
- **Zb:** distinguir Zba/Zbb/Zbc/Zbs, e confirmar subconjunto real ESP32-P4; não presumir todas as subextensões.
- **Zc:** Zcb/Zcmp/Zcmt e interações com C, registradores e stack frames; tabela oficial obrigatória.
- **XespV/XespLoop:** instruções customizadas Espressif, consultar TRM específico da revisão do chip; não reutilizar opcodes reservados sem confirmação.
- **Privilegiadas:** `mret`, `wfi`, `sfence.vma` (quando aplicável), `csrs` e controles M-mode; separar suporte de assembler de autorização de execução. Não supor S-mode no ESP32-P4.
- **D, RV64, V, B extras:** fora do alvo inicial. A arquitetura da tabela deve permitir importação futura, sem contaminar RV32IMAFC.
## 14. Checklist de aceitação
- [ ] Geração de `instruction_table.json` a partir de snapshot versionado de riscv-opcodes, com hash/commit.
- [ ] Tabela completa C/Zb/Zc com máscaras, matches, restrições e teste diferencial.
- [ ] Parser MaiaCC C++ compilado freestanding ou com runtime mínimo compatível.
- [ ] Conformidade RV32I/M/A/F/Zicsr/Zifencei com binutils/LLVM.
- [ ] ELF32 `.o` com símbolos, relocations e ABI validado por `readelf -a` e `objdump -dr`.
- [ ] Linker/loader do MaiaRTOS com API versionada, cache sync e rollback de falhas.
- [ ] Teste no dispositivo sem FreeRTOS/Arduino e sem dependência oculta de bibliotecas de runtime.

---
**Status:** documento de engenharia e núcleo de encoding verificável; extensões específicas pendentes de importação automatizada da fonte oficial. Nunca tratar esta edição como catálogo exaustivo da ISA inteira.
