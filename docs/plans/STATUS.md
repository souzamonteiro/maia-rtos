# Project status

Date: 2026-11-26

Progress checklist (owner-facing, Portuguese):
[CHECKLIST_P4_SEM_FREERTOS.md](CHECKLIST_P4_SEM_FREERTOS.md).

## F9: primeira execução nativa no core 1 — 2026-11-26

Um probe separado do produto iniciou o HP core 1 sem FreeRTOS ou runtime
ESP-IDF. O worker, mailbox e pilha ficam em SRAM interna; somente o core 0
escreve no console. O core 1 mantém interrupções desabilitadas, usa uma entrada
de exceção própria e não chama scheduler, libc, TLS, drivers, flash ou PSRAM.
Após 1.024 trocas request/ack com payload e resultado conferidos, confirma o
estacionamento. Há limites de polling e falhas explícitas, sem retry após a
partida. A API é apenas diagnóstica, na fase de boot com o secundário ainda
pertencendo à inicialização da ROM; não permite assumir um core já em uso.

No Tab5 rev-v1.3, duas partidas registraram `hart=1`, `sp=0x4ff00510`,
`rounds=1024`, estágio final 5 e nenhum fatal. O runner confirmou
`probe_pass=true` e `restore_verified=true`, com o hash original
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Capturas e relatório:
[partida do core 1](../../../maia-rtos-recovery/native-tab5-core1-startup-20261126-02/).
Uma primeira tentativa parou sem escritas por uma precondição excessiva sobre
clock/reset; a condição foi alinhada ao protocolo do fabricante e coberta no
teste host. Essa tentativa também restaurou o app original.

`core1_start_test`, `native_probe_gate` e `f0_flash_runner_test` passaram (3/3).
O novo probe e o probe F0 anterior passaram na validação estrutural das imagens.
O binário de produto continua idêntico, SHA-256
`b0337655064a6a6271cdb12a2fed73dac25446317f9facb6dbe25fc6ddb2a7ee`.
Não há ainda IPI, scheduler SMP, TLS/traps por núcleo, qualificação de caches
do core 1 ou concorrência dos drivers. Nenhuma melhoria na abertura de Files
é atribuída a este probe. Próximo passo: IPI isolado e estado de interrupções
por núcleo, antes de integrar a partida ao produto.

## Leitura ELF por offsets e probe SciCalc — 2026-11-24

O caminho `runelf <arquivo.mobj>` agora mantém o arquivo aberto e fornece ao
loader um leitor por offsets, evitando uma cópia integral do ELF no heap. A
API de carregamento em memória continua disponível pelo mesmo núcleo. Os
testes host do loader passaram com ASan/UBSan (3/3), os testes da CLI passaram
(14/14), e a imagem P4 atual foi empacotada pelo alvo `maia_m0_image` e validada
(5 segmentos).

No Tab5, o arquivo existente `/sd/apps/scicalc.mobj` (57.256 bytes) foi aberto
pelo loader, que alocou 36.096 bytes e registrou a entrada em
`maia_module_main`. A captura permaneceu sem marcadores fatais por 20 s; a
shell foi recuperada após o reset. O runner confirmou `probe_pass=true` e
`restore_verified=true`, com o SHA-256 original
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Isso comprova o caminho de leitura/dispatch, mas não confirma que a interface
SciCalc desenhou ou aceitou interação.

Foi implementado um adaptador para os imports legados `host_app_quit`,
`host_app_run`, `draw_window`, `draw_text`, `draw_button`, `draw_rect`,
`draw_line`, `host_bind_key`, `host_bind_button` e `powf`, com gráficos
encaminhados pelo lease do serviço LCD e entrada pelo host nativo. Os testes
host focados com ASan/UBSan passaram (6/6), e a imagem P4 foi reconstruída e
validada. No novo probe USB, SciCalc voltou a chegar à entrada e permaneceu
sem marcador fatal por 20 s; o log interativo não contém avisos de imports não
resolvidos. O runner confirmou `probe_pass=true` e `restore_verified=true`;
o firmware original e o app instalado foram preservados.

A aceitação visual e de interação ainda estava pendente nessa etapa; o resultado
posterior está registrado abaixo.
Evidências:
[probe USB inicial](../../../maia-rtos-recovery/native-tab5-scicalc-streaming-observe-20261124/),
[probe com compatibilidade legada](../../../maia-rtos-recovery/native-tab5-scicalc-legacy-compat-20261124/).

## ABI RV32F e SciCalc nativa — 2026-11-24

A disassembly do módulo marcou o ELF como single-float (`Flags=0x2`) e mostrou
que a execução parava no primeiro `fmv.w.x`/`fsw` de `g_last_val`: o produto
estava compilado RV32IMAC/ILP32, sem contexto FP, e `_start` deixava `FS=Off`.
O alvo de produto Tab5 e suas bibliotecas estáticas nativas agora usam
RV32IMAFC/ILP32F; o startup habilita `FS=Initial` com MIE ainda desabilitado.
Build e validação da imagem passaram, com 5 segmentos.

Depois disso, a SciCalc chegou ao loop de eventos; a primeira leitura de toque
pode retornar indisponível até existir um snapshot válido. O adaptador legado
agora registra essa condição uma vez, preserva o estado anterior e continua o
app, em vez de encerrá-lo antes da primeira amostra.

No Tab5, o dono confirmou título/controles visíveis e interação por toque e
teclado. O log registrou `2+3*4 = 14` e retorno normal código 0 após o dono
encerrar o app. O runner marcou `probe_pass=false` porque a saída ocorreu antes
dos 60 s solicitados; não houve marcador fatal e `restore_verified=true`. O
firmware original foi restaurado com SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Testes host focados do scheduler/loader/compatibilidade passaram (5/5), e o
teste de regressão para toque indisponível passou isoladamente (1/1).

Evidências:
[probe de toque e SciCalc](../../../maia-rtos-recovery/native-tab5-scicalc-rv32f-integration-followup-20261124/),
[probe de teclado](../../../maia-rtos-recovery/native-tab5-scicalc-rv32f-keyboard-20261124/).

## Ponte gráfica MNM para o owner LCD — 2026-11-24

O build de produto P4 agora publica a API de display MNM. `put_pixel` e
`fill_rect` mantêm a orientação lógica 1280x720; o primeiro desenho do lote
solicita ao serviço LCD um lease do backbuffer, que é inicializado copiando o
quadro frontal atual. `sync` envia a publicação pelo owner e espera a
apresentação. `get_framebuffer` permanece NULL para não expor um ponteiro que
escaparia do lease. A exclusividade é contada por escopos de execução aninhados:
o loader fecha lotes e libera aquisições esquecidas quando cada módulo retorna,
sem permitir que um app filho solte a exclusividade do desktop pai. Ao voltar
ao terminal, todas as linhas são marcadas para redesenho.

O owner continua sendo o único responsável por adquirir/publicar os buffers e
serializa o lease com atualizações do terminal. O DMA só lê o framebuffer
frontal enquanto o backbuffer é semeado; o caminho existente de write-back
publica as alterações antes da troca. O custo de copiar e sincronizar o quadro
completo ainda não foi medido no Tab5. `sync` continua sendo `void`; falhas são
registradas no console, enquanto as primitivas retornam erro. `gui_kit` ignora
os retornos das primitivas, portanto a confirmação visual continua necessária.

Validação host: `lcd_output_test`, `native_display_api_test` e
`native_shell_display_test` passaram (3/3); firmware `maia_m0.elf` P4/XIP
compilado. No Tab5, o smoke USB publicou os dois lotes, o módulo retornou zero,
a shell respondeu e o diretório de teste foi removido. O runner restaurou a
aplicação anterior com verificação (`probe_pass=true`,
`restore_verified=true`). O usuário confirmou visualmente o quadro e sua
preservação após a atualização parcial; desktop e SciCalc continuam pendentes.
[Arquitetura](../shell/MaiaRTOS_Native_Execution_Architecture.md#54-native-host-abi-v1).

Foi adicionada uma fixture MNM de hardware que desenha fundo e quatro
marcadores coloridos, publica, aguarda 8 s, altera um retângulo central,
publica novamente e aguarda mais 8 s. O runner
[run_mnm_graphics_display_probe_usb.py](../../tests/hardware/run_mnm_graphics_display_probe_usb.py)
usa o runner F0 existente para fazer backup, gravar e restaurar a aplicação
via USB. A fixture foi compilada como ELF32/RISC-V relocável e empacotada; seu
ELF usa apenas tipos de relocação tratados pelo loader. O smoke foi executado
duas vezes no Tab5; na segunda, cada quadro permaneceu 8 s para observação. Nos
dois casos, os marcadores dos dois lotes apareceram, o módulo retornou zero, a
shell respondeu e o diretório temporário foi removido. O runner confirmou
`probe_pass=true` e `restore_verified=true`; o backup da aplicação original teve SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Os logs comprovam o caminho eletrônico da API; na repetição ampliada, o usuário
confirmou visualmente o quadro e a preservação após a atualização parcial.
Evidências:
[primeira execução](../../../maia-rtos-recovery/native-tab5-mnm-graphics-display-probe-20261124/)
e [repetição com janela visual ampliada](../../../maia-rtos-recovery/native-tab5-mnm-graphics-display-probe-20261124-repeat/).

## Regressão host MaiaScript/SciCalc — 2026-11-23

A árvore de trabalho atual passou 33/33 casos de compilador → MaiaASM →
emulador RV32, além do teste de lifetime e recuperação de erros do parser.
O caso `32_scicalc_input` usa os handlers da aplicação SciCalc real e verificou
`2+3*4=14`, `2.5^2=6.25` e erro de domínio para divisão por zero. As chamadas
gráficas são simuladas: o teste não executa nem valida visualmente a UI, o
desktop ou a calculadora no firmware nativo.

Os executáveis foram compilados fora do repositório, ligando os testes à
biblioteca `maia_shell_host_tools` da árvore de trabalho. O teste de compilador
precisa ser executado a partir de `maiascript/` para resolver os imports; fora
desse diretório houve falhas de localização, não falhas de execução dos casos.
Os resultados abrangem alterações anteriores ainda não commitadas e não
certificam todos os programas ou instruções RV32.

## Proteção da capacidade gráfica MNM — 2026-11-23

A shell nativa anunciava `get_framebuffer` em `0x48100000`, embora a janela
PSRAM qualificada use `0x4B800000` e o serviço LCD detenha os dois buffers.
As primitivas de desenho eram nulas e a aquisição exclusiva retornava sucesso.
Para não expor uma capacidade inexistente ou um endereço não qualificado, a
shell agora anuncia `host->display == NULL` e informa a limitação no boot;
entrada de toque e teclado permanece registrada. O wrapper importado
`fill_rect` retorna `-1` quando não há implementação. O perfil ESP possui
adaptador próprio e não foi alterado.

O host reproduziu as duas falhas anteriores e passou 5/5 regressões focadas;
os três testes da proteção/loader também passaram em Release e ASan/UBSan.
Um snapshot isolado com submódulos fixados passou os mesmos 5 testes em
Release com `-UNDEBUG` e gerou imagem P4 válida de 847.088 bytes, não gravada.
O `lcd_output_test` preexistente não compila no Release padrão com `-DNDEBUG`:
suas asserções removidas deixam parâmetros/variáveis sem uso sob `-Werror`.
No Tab5, um módulo confirmou a ausência do display, a presença das interfaces
de entrada e o erro de `fill_rect`; retornou zero, a shell respondeu após sua
execução e o diretório de teste foi removido. O runner restaurou a aplicação
anterior com escrita verificada. A imagem contém alterações locais anteriores,
inclusive a sincronização de cache; a flash inteira não foi comparada.
[Evidências](../../../maia-rtos-recovery/native-tab5-mnm-graphics-gate-20261123/).

Isto **não implementa** os gráficos nativos: apps que ignoram a ausência da
capacidade ainda podem executar sem UI visível. A integração com o proprietário
do LCD, o desktop e a SciCalc continuam pendentes.

## Escrita MNM da shell — 2026-11-23

O callback `write_fd` retornava zero mesmo depois de emitir os dados. Agora,
para stdout e stderr, retorna a quantidade de bytes entregue ao callback ou
erro em entradas inválidas, ausência de callback e falha observável do buffer
de redirecionamento/pipeline. A saída ainda é textual: NUL embutido é
rejeitado e o callback `void` não confirma transmissão física. O
[contrato atual](../shell/MaiaRTOS_Native_Execution_Architecture.md#54-native-host-abi-v1)
documenta os limites e não declara suporte binário.

No host, 5/5 testes focados passaram; o teste de escrita também passou em
Release e sob ASan/UBSan. Foram verificados, entre outros casos, o sucesso com
65.535 bytes no staging, a falha com 65.536, pipeline e a preservação do
arquivo em falha de redirecionamento. O build nativo P4 passou.
Um snapshot isolado do índice com submódulos nas revisões fixadas passou os
mesmos 5 testes em Release e gerou uma imagem P4 válida de 847.200 bytes; essa
imagem não foi gravada e não inclui a sincronização de cache ainda local.

No Tab5, um módulo verificou os retornos e erros e escreveu 138 bytes em
stdout e texto em stderr; a execução terminou em zero. Outra execução com
stdout redirecionado confirmou stderr na saída e o conteúdo exato e EOF do
arquivo. O diretório temporário foi removido, e o runner restaurou a aplicação
anterior com escrita verificada. A imagem testada inclui alterações anteriores
ainda não commitadas, incluindo sincronização do cache; não houve comparação
integral da flash. Evidências:
[teste de escrita MNM](../../../maia-rtos-recovery/native-tab5-mnm-write-20261123/).
Desktop, SciCalc e integração gráfica no firmware nativo permanecem pendentes.

## Contexto MNM aninhado — 2026-11-23

O loader agora ativa o host dos wrappers somente durante a entrada do módulo e
restaura o host anterior no retorno. Uma validação malsucedida não substitui
esse contexto. O build nativo com TLS usa um ponteiro por tarefa; o fallback
sem TLS e o perfil ESP mantêm o ponteiro global com restauração em aninhamento
síncrono. A API ainda exige que invocações concorrentes do loader sejam
serializadas e não suporta saída não local de um módulo.

No host, os testes de contexto passaram com e sem TLS, incluindo ELF inválido
e aninhamento com hosts distintos; a variante TLS também intercalou duas
threads. O aninhamento host chama diretamente o helper interno de entrada;
a falha de validação percorre o loader público. Os dois testes passaram com
ASan/UBSan e, junto com o teste de espera MNM, em Release com asserções
ativas. O build P4 passou e seu ELF identifica `s_active_host` como TLS.
Um snapshot isolado das alterações preparadas, com submódulos nas revisões
fixadas, passou 5/5 testes host e empacotou uma imagem P4 de 847.184 bytes;
essa imagem não foi gravada.

No Tab5, um módulo C pai executou `runelf` aninhado, confirmou o retorno 42 do
filho, exercitou a rejeição de um ELF de um byte e verificou os serviços de
tempo antes e depois; retornou 77. O módulo foi conferido por leitura de volta,
os arquivos e o diretório de teste foram removidos, e o runner restaurou a
aplicação anterior com hash verificado. A primeira tentativa não obteve
resposta após reabrir a conexão; a tentativa seguinte manteve a mesma conexão
serial e confirmou que o diretório anterior não existia. Capturas e imagens:
`../../../maia-rtos-recovery/native-tab5-mnm-context-retry-20261123/`.
O smoke físico usa as mesmas funções de host nos dois níveis e não comprova
isolamento por tarefa, segurança concorrente do loader ou reentrância geral
da shell. A imagem testada inclui alterações anteriores ainda não commitadas,
inclusive a sincronização do cache de instruções; desktop e integração gráfica
continuam pendentes.

## Espera MNM sobre o scheduler nativo — 2026-11-23

O callback `sleep_ms` fornecido pela shell aos módulos retornava imediatamente
no build nativo. Agora converte milissegundos para `timespec` e chama o
`nanosleep` da Newlib, que usa a espera por ticks do MaiaRTOS; o caminho
`ESP_PLATFORM` permanece inalterado. A chamada retorna o erro da Newlib ao
consumidor MNM. Espera zero continua retornando imediatamente, sem yield;
esperas positivas requerem contexto de tarefa e o limite nativo é
`INT32_MAX` ms. O wrapper MaiaScript `delay_ms` tem retorno `void` e, portanto,
não expõe esse erro ao programa.

No host, `shell_mnm_sleep_test` exercitou o callback real entregue por
`runelf`, incluindo 0, 1, 1500 e `UINT32_MAX` ms e falha simulada;
`newlib_syscalls_test` verificou a conversão para ticks, zero sem yield,
ausência de tarefa e o limite. `shell_expr_control_test` também passou.
`maia_m0_image` compilou e passou na validação da imagem.
Um snapshot isolado das alterações preparadas para commit, com os submódulos
nas revisões fixadas, também passou nos três testes host e no build P4; essa
imagem isolada não foi gravada.

No Tab5, o probe `tests/hardware/maia_mnm_sleep_probe.ms` compilou para MNM e
mediu 309 ms no relógio monotônico para `delay_ms(300)`. O diretório temporário
no SD foi criado apenas se inexistente e removido ao final; os metadados FAT
podem ter mudado. O runner restaurou a aplicação anterior e verificou o hash
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Capturas em `../../../maia-rtos-recovery/native-tab5-mnm-sleep-20261123/`.
Uma única medição não define garantia de latência. O desktop e a integração
segura das primitivas gráficas MNM continuam pendentes.

## Restored baseline and initial audit

- Functional baseline: `bf044d3`, restored Tab5 desktop, full shell and MaiaScript
  apps. Owner confirmed the SciCalc/Alarm visual fix. Firmware binary SHA-256
  remains `21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd`.
- **AUDITED / NONCOMPLIANT WITH R-002:** the working image contains FreeRTOS
  startup/scheduler and uses ESP-IDF drivers/runtime. Project native kernel
  sources are absent from this image. LVGL/vendor panels are compiled but their
  code is discarded from the final image. No competing native scheduler was found.
- **HOST-TESTED:** MaiaScript 32/32 and the panel raster test pass. Fresh full host
  build fails on shell contract mismatch; current native target fails on missing
  `riscv_asm.hpp`. These are baseline gaps, not fixed in this audit.
- Temporary checkout: fresh host tests 8/8 and native console build pass. Its
  prior hardware evidence is limited to cooperative console operation; timer
  interrupts, display and SMP remain unresolved. It is not the active firmware.
- No firmware/source/configuration changes, serial access or flash in this audit.
  Working artifacts were copied to the sibling recovery directory.

Evidence, source inventory, exact limitations and staged migration plan:
[runtime dependency audit](../audits/freertos-dependencies-2026-11-10.md).
Next small implementation: reconcile shell contract tests without changing the
current ABI, and establish dependency checks per build target.

## First incremental experiment — 2026-11-10

- Added opt-in `MAIA_TAB5_SINGLECORE_DRIVERS`, isolated config/build and guards
  protecting the default configuration. Both profiles build; host isolation and
  panel tests pass. The default configuration remains two-core and unchanged.
- **HARDWARE-TESTED, PARTIAL:** the one-core experiment passes desktop app,
  scientific calculation, timer/cancellation and background-job tests. ELF no
  longer defines secondary-core startup, cross-core tick or IPC-call symbols.
  It still contains the FreeRTOS scheduler and is not compliant with R-002.
- **FAILED / NOT PROMOTED:** initial heavy compiler work starved idle and
  triggered its watchdog. With experimental compiler priority lowered, a small
  program exposed a C++/pthread mutex fault. The same source also asserts in
  `xQueueSemaphoreTake` on the original preserved firmware. Root cause remains
  open; no claim that removing SMP caused or fixed the compiler fault.
- Restored the preserved `bf044d3` application, cleaned test files and verified
  SciCalc `2+3*4=14` and return to desktop. Bootloader/partitions were untouched.
- Next small task: isolate that compiler/ownership fault, then continue native
  scheduling/driver-service migration behind independent acceptance checks.

Exact source changes, version comparison, results, hashes and logs:
[first migration experiment](../audits/tab5-singlecore-stage-1-2026-11-10.md).

## Compiler CST ownership fix — 2026-11-10

- Removed shared control blocks/mutexes from private temporary CST nodes; one
  storage owner releases them after AST conversion. Public AST/Parser ABI,
  grammar/generated parser, codegen and module ABI remain unchanged.
- **HOST-TESTED:** 32/32 compiler regressions plus AST lifetime, large/repeated
  parse and negative syntax/recovery tests PASS. ASan/UBSan PASS. Both firmware
  profiles build; already-versioned host compiler binaries rebuilt.
- **HARDWARE-TESTED, LIMITED:** the formerly failing 32-addition source now
  compiles on the one-core Tab5 profile, validates and returns 32. Background
  jobs and SciCalc/Alarm regressions PASS. Seven CST shared-control-block ELF
  symbols before the fix, zero afterwards. Runtime/AST locks still remain.
- Original application restored after tests. This does not promote the
  one-core profile or certify large compilation, allocation failures, native
  preemption or SMP. Continue with a verified native scheduling target.

Evidence: [CST fix and exact scope](../audits/maiascript-cst-ownership-2026-11-10.md).

## Host contract build restored

- **HOST-TESTED:** root CMake build and all 5/5 CTest entries pass. ABI/kernel
  adapter tests also pass 2/2 with ASan/UBSan; C/C++98 validation-header check
  passes. The default Tab5 profile builds; no device flash in this step.
- Existing native v1 struct/values unchanged. Console adapter and fixtures use
  current names; a separate helper validates prefix/version/callbacks. The
  full C++ shell test links real host tools and retains all ten assertions.
- Parser/lexer/EBNF and command implementations unchanged. These tests do not
  certify real VFS/async I/O, native preemption or SMP. The full freestanding
  shell remains incomplete. Next: isolated native timer/scheduler diagnostic.

Details and exact commands: [host contract repair](../audits/shell-host-contracts.md).

## Historical milestone claims

The table below is retained as history. It does not certify the current working
image as independent, preemptive under the native kernel, or SMP-capable under
the native kernel. Current evidence above takes precedence.

| Area | Status | Evidence |
|---|---|---|
| Architecture documents | DESIGN | Repository documents |
| M0-A / N0 ESP32-P4 & Tab5 hardware/memory audit | HARDWARE-TESTED | `docs/audits/esp32p4-execution-memory.md`, `docs/hardware/EVIDENCE.md`, `docs/hardware/OPEN_QUESTIONS.md` |
| N1 MaiaShell boundary & reentrancy audit | DESIGN | `docs/audits/shell-boundary.md` (inspected against `maiaos/apps/shell`) |
| Host build/test scaffold (`maia_contract_test`, `esp32p4_regs_test`, `kernel_mvp_test`) | HOST-TESTED | CMake/CTest (`100% tests passed out of 4` in `build_host`, plus C++98 header checks) |
| N2 MaiaShellHost v1 contract & Interactive Native Shell (`maia_shell_mvp.c`) | HARDWARE-TESTED | Host CTest (`tests/host/shell_host_contract_test.c`, `tests/host/kernel_mvp_test.c`) + live interactive session on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M0 ESP32-P4 bare-metal boot (`start.S`, `soc_early_init.c`, `linker.ld`) | HARDWARE-TESTED | Booted directly in `L2MEM_LOW` (`0x4FF00000`) on M5Stack Tab5 (`ESP32-P4 rev v1.3`) with zero FreeRTOS/ESP-IDF/Arduino runtime (`docs/hardware/EVIDENCE.md`) |
| M0 Console driver (`soc_console.c`: USB-Serial/JTAG + UART0) | HARDWARE-TESTED | Bidirectional TX/RX verified on M5Stack Tab5 `/dev/cu.usbmodem1301` (`docs/hardware/EVIDENCE.md`) |
| M1 RISC-V Trap Vector & Timebase (`trap_context.S`, `soc_timer.c`) | HARDWARE-TESTED | 64-byte aligned CLIC `_maia_trap_entry`, `ebreak` exception trap (`mcause=0x30000003`), `ecall` trap (`mcause=0x3800000B`), CLIC `mcause` `MPP`/`MPIE`/`MPIL` save/restore, and microsecond hardware timebase verified on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M2 Multitasking Scheduler (`kernel/ksched.c`) | HARDWARE-TESTED | Priority + round-robin task scheduler (`idle`, `telemetry`, `maia_shell`), tick sleep/wakeup (`maia_task_delay_ticks`), and task introspection (`ps`, `ticks`, `uptime`) verified on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M4 Deterministic Region Memory Allocator (`kernel/kmem.c`) | HARDWARE-TESTED | 16-byte aligned region allocator with free-block coalescing (`mem`, `alloc 256`) verified on host CTest and on M5Stack Tab5 `L2MEM` (`0x4FF076A8`) (`docs/hardware/EVIDENCE.md`) |
| M5 Synchronization & IPC (`kernel/ksync.c`) | HARDWARE-TESTED | Mutex (`maia_mutex_t`), counting semaphore (`maia_sem_t`), and bounded message queue (`maia_queue_t`) verified across `telemetry` and `maia_shell` tasks and `qtest` on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M3 SMP (Dual-Core HP Core 0 + Core 1) | NOT IMPLEMENTED | Core 1 parked safely in `start.S`; bring-up tracked in `UQ-006` |
| PSRAM external heap region | NOT IMPLEMENTED | Internal 32 KiB `L2MEM` pool active; Octal PSRAM MSPI init tracked in `UQ-007` |
| Board display driver (720x1280 MIPI-DSI ST7121 with LEDC backlight) & physical keyboard (A164 @ 0x6D) | HARDWARE-TESTED | Clean-room MIPI-DSI 800Mbps D-PHY + ST7121 28-cmd vendor DCS init + LEDC 5000Hz 12-bit PWM channel 1 backlight + PI4IOE 0x43/0x44 push-pull reset sequencing + DW_GDMA continuous 10-block ring in L2MEM + A164 STM32 keyboard String mode driver verified on Tab5 (`docs/hardware/EVIDENCE.md`) |
| Permanent Standalone Flash Boot (0x10000) | HARDWARE-TESTED | ESP-IDF 2nd-stage bootloader dual-segment Flash MMU mapping (`0x40000000`/`0x40010000`) and clean-room `esp_app_desc_t` (`0xABCD5432`) at Segment 0 (`0x4FF00000`), booting standalone directly from SPI Flash without host computer (`docs/hardware/EVIDENCE.md`) |
| MaiaScript Native Compiler (RV32IMAF ILP32F) & MaiaASM | IMPLEMENTED / HARDWARE-TESTED | Zero-GC AOT/JIT native compiler with 30/30 regression tests passing (Sections 01-32: Basics, OO, UI, Canvas, LVGL, Wi-Fi, Storage, Audio, I2C), EBNF-driven via MaiaCC (`make parser`), zero FreeRTOS dependencies, and live GUI execution verified on Tab5 hardware (`t_gui.mobj`) |
| Board touch/audio/SD drivers | NOT IMPLEMENTED | MicroSD pinout verified (GPIO39-44); driver tracked in `UQ-008` (M6) |
| MaiaWASMVM integration | OUT OF SCOPE for core | Optional user-space workload per `README.md` |

Do not change statuses without attached evidence.

## Scoped update — 2026-11-10

MaiaScript SciCalc/Alarm panel newlines: HOST-TESTED pixel positions and
panel bounds; updated firmware built/flashed and app operation
HARDWARE-TESTED over serial. Physical display appearance still requires
visual confirmation. See [test results and scope](../audits/maiascript-panel-text.md).
This update concerns the user's restored working firmware baseline and
does not revalidate the historical kernel/milestone claims above.

## Native scheduler diagnostic — partial hardware result

- Added isolated RAM acceptance probe and native dependency gate; host 6/6 PASS.
- Corrected verified SYSTIMER base (old address was I2S0) and off-by-one IRQ route.
- Hardware progresses from zero to one real timer interrupt/task switch, but
  repeated delivery fails with an enabled pending alarm. **M1/M2 NOT ACCEPTED.**
- No flash writes; no parser or desktop changes. Native FP/SMP remain unverified.
- Evidence/logs/reproduction and next CLIC diagnosis:
  [native probe report](../audits/native-scheduler-probe.md).

## CLIC fresh-context return — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** corrected fresh task INT bit; four RAM runs
  each deliver 250 real timer IRQs/ticks and 251 task switches, both busy tasks
  progressing, zero ecalls/exceptions, active MIL restored to zero.
- Supersedes prior one-IRQ stall for the corrected frame. Host 6/6 PASS, kernel
  ASan/UBSan PASS; old frame rejected by negative host/hardware regressions.
- No foreign source/runtime imported; no flash writes or parser changes.
- Full M2 context ABI/FP and SMP remain unverified; native desktop not ported.
- [Research, exact changes, captures and next test](../audits/native-clic-return.md).

## Live integer context — 2026-11-11

- Assembly register sentinel test reproduced missing tp preservation after three
  real IRQs. Save/restore tp in existing frame word 124 fixes that failure.
- **HARDWARE-TESTED, LIMITED:** corrected test reaches 250 IRQs/251 switches
  with all checked integer registers intact, both tasks progressing and no
  ecalls/exceptions. Host 6/6 and sanitized kernel PASS.
- No new runtime dependency, frame-size/public ABI or parser changes. FP pending.
- [Exact workload, failure and scope](../audits/native-integer-context.md).

## RV32F context — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** independently authored 272-byte RV32F frame
  preserves f0–f31/FCSR and zero-initializes fresh state. Final two FP runs and
  integer regression pass 250 real IRQs/251 switches, reg_errors0, no ecalls/faults.
- Negative old-context test fails on f0; isolated FCSR-zero mutant fails on
  rounding/flags. Real ILP32F C calls return 5.0 correctly. Host 7/7 and sanitized
  integer/FP frame fixtures 2/2 PASS; native runtime guard PASS.
- C and ASM ISA/ABI flags aligned; fresh builds required to clear old caches.
- No flash/parser/vendor-runtime changes; mixed FS-Off/on, priority/yield/delay,
  fault policy and full native desktop integration remain pending.
- [Exact evidence, layout, risks and commands](../audits/native-fp-context.md).

## Mixed scheduling — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** FS-Off high-priority task yields ready and
  sleeps two ticks while two FP tasks retain live sentinels across ecalls/IRQs.
- Reproduced saved-ecall INT=0 priority leak; normalize task return flags while
  passing raw cause separately. Corrected workload passes250 realIRQs plus
  explicit ecalls,124 sleeps,125 priority checks and374 FS-Off checks.
- Host7/7 and sanitized frame2/2 PASS. No flash/vendor/parser changes.
- [Evidence and limits](../audits/native-mixed-scheduling.md). Next: fatal traps.

## Native fatal-trap policy — 2026-11-11

- Reproduced illegal16 being skipped with FAULT_RESUMED on previous handler.
- Unknown exceptions/breakpoints/IRQs now retain raw diagnostics and halt; only
  valid enabled M-mode task ecalls advance PC. No guessed instruction width.
- Fatal output uses bounded USB polling and bypasses display/normal console,
  heap and scheduler. RAM record is reset-volatile; no persistent writes.
- Host 9/9, sanitizer C fixtures 3/3, C++98 headers pass. Controlled illegal16/32
  and compressed-breakpoint tests compare cause/PC against ELF metadata.
- [Full actual evidence and limits](../audits/native-fatal-traps.md).
- Hardware final: all3 controlled fatal injections PASS; post-policy mixed
  regression PASS (250 IRQs,18142 ecalls,124 sleeps). Desktop/shell/SciCalc
  restored and verified after RAM reset. No flash/SD writes.

## Scheduler atomicity — 2026-11-11

- Local IRQ nesting and coherent task/tick/stat snapshots added. READY published
  after construction; delay/exit commit within saved-context traps. EXITED slot
  reuse keeps registered count stable; wraparound deadlines use modular arithmetic.
- **HARDWARE-TESTED, LIMITED:** lifecycle/mixed profile250 IRQs, 125 creates/returns,
 125 nesting checks, 4 slots, live integer/FP sentinels intact.
- Host10/10, sanitized atomic/integer/FP fixtures3/3 PASS; negative mutants rejected.
- No flash/parser/vendor runtime changes. Next: no-ready idle fallback.
- [Scope, tests and limits](../audits/native-scheduler-atomicity.md).

## Internal idle — 2026-11-11

- Replaced task revival on no-ready with a private static idle context/WFI;
 16 public slots unchanged, idle current ID=-1, coherent diagnostics added.
- **HARDWARE-TESTED, LIMITED:** all-sleeping/all-exited workload and empty boot
  each pass 150 real IRQs, idle MIE on/FS off and correct terminal task states.
- Host 11/11, sanitized fixtures 4/4 PASS; revival mutant rejected.
- No flash/vendor/parser changes. Next ordered step: stack bounds/guards.
- [Exact results and instrumentation limits](../audits/native-idle.md).

## Stack protection — 2026-11-11

- Pre-save SP/frame/guard checks use mscratch metadata; complete saved contexts
  are checked before selection/restore. Reserved/overlapping stacks rejected.
- Dedicated 4096-byte IRQ and 2048-byte emergency buffers; task minimum304/ 448
  bytes for integer/RV32F. Raw diagnostics append stack reason/measured handler SP.
- **HARDWARE-TESTED, LIMITED:** seven deliberate fault profiles stop at exact
  ELF PC with emergency=1; bad-SP cases keep source guard intact, including SP=0.
- Host 13/13, ASan/UBSan stack/kernel/atomic/idle fixtures 6/6 PASS; header mutants
  rejected after strengthening in-range misalignment regression.
- [Exact evidence and limits](../audits/native-stack-protection.md). No isolation,
  NMI/double-fault, native desktop or SMP completion claim.

Final regressions PASS: lifecycle/FP and integer 250 real IRQs each; both idle
profiles 150 each; actual C IRQ-stack checks all pass. Three ordinary fatal
profiles preserve raw cause/exact ELF PC. Preserved flash shell, SciCalc
`2+3*4=14` and desktop verified after reset. Logs and exact recovery artifacts:
`../maia-rtos-recovery/native-stack-protection-20261111/` (SHA256SUMS).

## Future minimal POSIX layer — 2026-11-11

- **DESIGN ONLY:** owner requested a future optional POSIX subset with minimal
  changes to whatever native system is complete when implementation begins.
- [Plan and acceptance criteria](POSIX_MINIMAL.md): fresh API/libc gap audit,
  public-API adapters, small separate native extensions, explicit feature matrix,
  single-process/multithread model and regression with POSIX enabled/disabled.
- PSE51 is scope guidance, not a compliance claim. No runtime/header/build/parser
  changes, hardware access or execution tests; document links/diff checked.
- Current native-driver migration remains the next implementation task.

## Native driver communication queues — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** native queue send/recv/count now serialize full
  payload/index transactions with nested local IRQ exclusion; no SMP guarantee.
- Item size is capped at 256 bytes to bound copy work; initialization rejects
  size/address overflow. Struct layout and function signatures remain unchanged.
- Host 14/14 and ASan/UBSan queue/integer/FP fixtures 3/3 PASS; negative local
  mutants rejected. Tab5 RAM workload: 253 real IRQs, 688 intact ordered packets
  from two task producers and one IRQ producer, zero errors. Initial workload
  failed an unjustified per-producer progress floor; adjusted workload/logs are
  documented in the [driver queue report](../audits/native-driver-queues.md).
- Preserved flash shell/SciCalc/desktop verified after reset. No foreign source,
  parser, module ABI, flash or legacy runtime removal in this unit.
- Next: serial I/O ownership/frontier and queue-backed service integration.
  Blocking waits, mutex/semaphore concurrency and full driver ports remain open.

## Native serial input service — 2026-11-11

- **HARDWARE-TESTED, LIMITED (USB):** raw USB/UART RX separated from board/display;
  permanent boot claim prevents legacy reader from stealing service input.
- Optional service: fixed owner, 64-byte queue, bounded 16-byte poll; FULL avoids
  another FIFO read. No implicit task/heap, blocking wait or RX IRQ wakeup.
- Host 15/15, sanitizer service/queue 2/2, C++98 headers and four negative mutants
  PASS. Final real USB sequence 0..255 passes with FULL=1, 317 real IRQs,
  errors=0 and auxiliary progress. Initial test failed auxiliary-progress criterion;
  consumer now sleeps while empty. [Exact evidence](../audits/native-serial-input.md).
- Service-off queue regression passes 253 IRQs/688 packets. Preserved flash
  shell/SciCalc/desktop restored; no SDK runtime, parser, module ABI or flash edit.
- Limits: owner must remain alive until reset, single-core, UART physical test
  pending, paced traffic only; desktop firmware still depends on FreeRTOS.
- Next: serial TX ownership/separation, keeping blocking waits and other driver
  migrations in separate increments. POSIX remains DESIGN only.

## Native serial output service — 2026-11-11

- **HARDWARE-TESTED, LIMITED (USB):** raw USB/UART TX separated from display;
  optional sole-owner service serializes whole records from multiple producers.
- 1..64-byte records, queue 8 plus one active record, 16 byte attempts/poll.
  FULL rejects whole enqueue; BUSY retains active offset. No heap or spin waits.
  Legacy TX/display mirroring is suppressed after claim; fatal sink stays separate.
- Host 16/16, sanitizer TX/RX/queue 3/3, C++98 headers and five mutants PASS.
  Final Tab5 capture: 80 records/3840 bytes, 260 IRQs, FULL=344, BUSY=239, errors=0.
  RX regression passes all 256 bytes/305 IRQs. [Evidence](../audits/native-serial-output.md).
- Flash shell/SciCalc/desktop preserved and verified. No foreign runtime, parser,
  module ABI or flash/SD change. Complete desktop still depends on FreeRTOS.
- Limits: single-core polling, permanent owner until reset, FIFO acceptance is
  not host ACK, UART physical/reconnect pending. Next: native atomic worker
  waits/wakeups and synchronization audit; POSIX remains DESIGN only.

## Native task notification waits — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** sticky per-task notify + blocking wait via labeled
  ECALL; check/block atomic after context save, first timeout/post completion wins.
  Late/second post retained; result installed once after frame validation.
- Single-core, no payload/count/heap, timeout0 EMPTY, finite TIMEOUT, explicit
  FOREVER. IRQ wait returns PERM; notify is ISR-safe without implicit yield.
- Host 18/18, sanitizer integer/FP notification + atomic/idle 4/4, C++98 headers
  and five mutants PASS. Actual Tab5: 40 rounds, 282 waits, 80 timeouts, 750 real
  IRQs, 520 blocked checks, errors=0; FP/lifecycle/fatal regressions also PASS.
- [Exact evidence and limits](../audits/native-task-notifications.md). Original
  flash shell/SciCalc/desktop restored; no parser, display configuration or flash
  edits. This does not fix blocking semaphores/mutexes, SMP or the complete runtime.
- Next: notification-driven TX worker on empty queue; display/PSRAM/cache/DMA
  remain separate prerequisites and risks. POSIX remains DESIGN only.

## Notification-driven serial TX worker — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** accepted TX records notify owner after publication;
  owner-only wait blocks indefinitely only with no queued/active bytes. Stale
  hints are consumed then predicate rechecked; FIFO BUSY retains a finite retry.
- Host 18/18, sanitizer TX + integer/FP notifications 3/3, C++98 header and five
  mutants PASS. Native: 80 intact records/3840 bytes, 302 IRQs, quiet head20/tail21
  with polls/run_ticks stable, and real IRQ write wakes TX; errors=0.
- Old polling TX profile remains compatible (80 records/260 IRQs PASS).
  [Exact evidence](../audits/native-tx-worker-notifications.md). Flash desktop,
  shell/SciCalc restored. No kernel/trap/parser/display configuration/flash edit.
- Limits: single-core static owner, shared mailbox must be coordinated, no
  TX-ready IRQ/reconnect/producer-space wait. Complete desktop still uses FreeRTOS.
- Next: audit/preserve the working display initialization before native driver
  changes; PSRAM/cache/DMA/power/panel variants remain separate hardware risks.

## Display baseline audit — 2026-11-11

Read [display baseline audit](../audits/display-baseline-audit.md) and its pinned
manifest. Documentation/evidence only: no native driver or firmware change.
Preserved flash boot selects ST7121 via touch firmware1; later ST7123 log is
generic. Active config is 70 MHz DPI/965 Mbps lane, not native 80/960. All30
ST7121 table entries match, but extra delays differ. PSRAM startup/ownership,
cache/DMA, safe reset, error propagation and memory reservations remain open.
Native Cache_WriteBack_All address/signature/0x30 match pinned ROM references;
policy/safety still need validation. Configured legacy L2 cache line is128B.
Retained panel drivers are required; discarded LVGL must not be grouped with them.
Recovery: `../maia-rtos-recovery/display-baseline-audit-20261111/`. Known baseline
artifacts rehashed/copied; current build/bin is a different experimental image.
Boot captured and maia.py shell/SciCalc14/desktop reverified. Native display
NOT RUN; complete desktop remains FreeRTOS-dependent, POSIX remains DESIGN.
Next: bounded system-I2C identification API/probe, read-only touch firmware
transaction, explicit timeout/missing-device tests; no LDO/reset/DMA yet.

## Native boot-only touch identification — 2026-11-11

Read [native touch transport/evidence](../audits/native-touch-identify.md). New
independent GPIO register-read engine has explicit NACK/TIMEOUT/BUSY, full high
hold after stretching, bounded waits and output commit after successful STOP.
Boot-exclusive core0/IRQs-off SoC backend uses SYSTIMER, not presumed CPU MHz;
caller must ensure pre-scheduler pin/timer ownership. No runtime locking/teardown.
Tab5 status0/4 permits read-only firmware ID; other states return BUSY, unknown
firmware is preserved without panel fallback. No production driver is replaced.
Host19/19, sanitizer fixture, C++98 headers, six negative mutants and invalid
profile rejection PASS. Target warm-reset RAM probe reads status4/firmware1
16 times plus recovery read, actual0x56 NACK, sampled-SCL injection TIMEOUT,
errors0. Initial too-strict Normal-only gate failures are retained. Physical
stuck-wire/frozen-timer/cold-panel tests NOT RUN. No power/reset/DMA/flash/SD writes.
Legacy shell/SciCalc14/desktop verified after final reset. Artifacts/manifest:
`../maia-rtos-recovery/native-touch-identify-20261111/`. User's local Tab5 PDF
and Sitronix protocol now pin the inspected pages; see EVIDENCE. Full firmware
still uses FreeRTOS; POSIX DESIGN. Next: checked bounded expander-register access
and safe bit-preserving reset/power contract, separately from memory/display DMA.

## Native expander configuration port — 2026-11-11

Owner permits open-source driver adaptation/porting (M5Stack, ESP-IDF, owner
code and portable drivers found in FreeRTOS); zero underlying FreeRTOS remains
mandatory. AGENTS.md supersedes its prior blanket copying prohibition, requiring
license/attribution and native dependencies. Read [expander port](../audits/native-expander-port.md).
M5Unified configuration/bit-update model ported to C over bounded native I2C;
MIT notice/source revision retained. New read8 repeated START/write8 transport,
restricted config register set, mask-preserving update, no-op and full readback
VERIFY error. Exclusive pre-scheduler ownership only; no retries/rollback/locks.
Host20/20, two sanitizer fixtures, C++98 header and five mutants PASS. Actual
RAM probe: banks43/44 accept identical OUT05 writes, five config bytes unchanged
over16 rounds each; touch16 reads firmware1, actual absent-write NACK, errors0.
Changed reset/power bits are NOT tested. Original flash shell/SciCalc14/desktop
verified after reset. Artifacts: `../maia-rtos-recovery/native-expander-port-20261111/`.
No flash/SD write; full desktop still depends on FreeRTOS. Next: checked board
reset sequence with safe LCD input/pull-up release and explicit partial-failure
contract, separately from LDO/DSI/PSRAM/DMA. POSIX remains DESIGN.

## Native LCD/touch reset — 2026-11-11

Read [native panel reset port](../audits/native-panel-reset.md). ST712x-only checked
bank43 P4/P5 sequence ports functional BSP100/100ms waits, cross-checked with
M5Stack input/pull-up LCD release. Apache-2.0 Espressif notice/license retained;
all dependencies are native, no FreeRTOS/SDK runtime. Snapshot, masked updates,
full verification and report last completed stage; error stops, no compensating
writes/retry/rollback. Caller exclusively owns boot bus/pins, stable supplies,
GPIO23 input and inactive display/DMA. Bounded pauses use SYSTIMER, not CPU MHz.
Host21/21, sanitizer fixture, C++98, six mutants, profile-negative PASS. Target
RAM reset reaches stage9, preserved1, elapsed244643us; touch status0/firmware1
16 reads plus recovery, errors0. Both-bank snapshots preserve unrelated config.
No voltage/waveform/pixels/cold-power/SMP/runtime claim. Preserved flash shell,
SciCalc14/desktop verified after reset; no flash/SD write. Exact artifacts/logs:
`../maia-rtos-recovery/native-panel-reset-20261111/`. Legacy full desktop remains
FreeRTOS-based and POSIX DESIGN. Next: audit/port MIPI PHY LDO acquisition with
calibration/ownership/error contracts before DSI clocks or framebuffer/DMA.

## Native VO3 calibration planning — 2026-11-12

Read [VO3 calibration audit/port](../audits/native-ldo3-calibration.md). Pure native
C port of SDK voltage conversion and read-only ROM-shadow/PMU snapshot; Apache
notice retained. No regulator acquisition/enable/write yet. Scope chip1.3, block
schema0/minor>=1; disabled/unknown revision flags and uncorrectable block1 reject.
SDK sign-magnitude/zero defaults,128-candidate first-minimum order, int64 math,
modeled-voltage/default diagnostics. Snapshot boot/core0/IRQs-off only.
Host22/22, sanitizer/C++98/six mutants PASS; exact SDK differential matches all
1048576 trim triples. Actual block0.3 raw55/33/33 ->1030/-4/989 ->dref13/mul3,
modeled2504637uV/error4637uV; sixteen snapshots unchanged, errors0. Current PMU
is already enabled/SW-selected with those codes: inherited state, NOT native
power-init proof. Old fixed code matches this chip but is not a generic port.
Artifacts: `../maia-rtos-recovery/native-ldo3-calibration-20261112/`. Builds retain
20261111 start-date suffix. Next: boot-exclusive acquire/lifetime/error policy,
adopt matching active rail, reject conflicts, masked write/verify for off rail.
No voltage/settling/cold-enable/display parity proof; full firmware still uses
FreeRTOS, POSIX remains DESIGN.

Preserved flash shell/SciCalc14/desktop verified after reset with paced keys;
initial unpaced3s capture failed to collect the result and is retained in the
VO3 report. No firmware fix or flash/SD write was made for this retest.

## Native VO3 acquisition — 2026-11-12

Read [VO3 acquisition/evidence](../audits/native-ldo3-acquire.md). Boot-only fixed
reservation until reset, stable calibration snapshots, active matching adoption
without writes; active mismatch BUSY. Off path guards DSI/CSI clocks, preserves
other fields, verifies analog/source/enable in order. Failure after programming
starts is sticky FAILED until reset; no rollback/retry/release. SDK runtime and
locks are not imported; Apache notice retained. Host23/23, sanitizer/C++98/six
mutants PASS. Actual target: stage5/adopted1/changed0/preserved1/duplicate_busy1.
Important finding: MIPI clock enables persist through ROM load_ram (initial
words0x0C0D5678/0x2B3). First overly broad guard rejected adoption; final policy
permits write-free adoption only, preserving all guards for power writes. No
regulator power write occurred on target. Off programming is HOST-TESTED; cycle
candidate compiled, physical run NOT RUN due active-clock precondition. Never
assume RAM entry quiesces display/DMA or just clear clocks to pass the guard.
Paced shell/SciCalc14/desktop verified after reset; no flash/SD writes. Artifacts:
`../maia-rtos-recovery/native-ldo3-acquire-20261112/`. Next: joint audit/port of
ordered DSI/CSI/DMA quiesce before hardware off->on test and DSI bring-up. Full
legacy desktop still uses FreeRTOS; native cold display remains pending.

The earlier panel-reset probe did not establish consumer/DMA quiescence. Its
report is now qualified and its diagnostic rejects active MIPI clocks before
reset; guarded probe compile passes, physical rerun remains pending quiesce.

## Read-only inherited MIPI/DW-GDMA state — 2026-11-12

Read [MIPI state audit](../audits/native-mipi-state.md). New isolated RAM diagnostic observes only
clock-enabled blocks, rechecks full clock words, never follows descriptors or
writes MIPI/DMA/power/reset. Host24/24, ASan/UBSan, three mutation negatives and
two mixed-profile rejection checks PASS; strict native runtime gate PASS.
Actual Tab5 eight samples PASS: clocksE6DFB7FF/0C0D5678/2B3 stable; validity0D
(DSI, CSI bridge, DW-GDMA; CSI host skipped). DSI bridge/DPI enable0, host power0/
PHY reset0; DMA global/channel/abort0 and all four channel FIFO/address/status0.
Active clocks are not evidence of enabled DMA in this observed state. This is
not an atomic snapshot, AXI outstanding-traffic proof or ordered shutdown test;
CSI host fields are unmeasured, not verified idle. Desktop/shell/SciCalc14 restored
via maia.py after reset; no flash/SD write. Full artifacts/reference hashes:
`../maia-rtos-recovery/native-mipi-state-20261112/`. Next: bounded native shutdown
contract covering reset/AXI completion, shared channels and PHY state before
clock gating/power tests. Full desktop still uses FreeRTOS; physical VO3 cycle,
guarded reset rerun, native cold display and POSIX implementation remain pending.

## Bounded native VDMA stop — 2026-11-12

Read [native VDMA stop](../audits/native-vdma-stop.md). Original native C whole-controller boot
reservation: require both existing bus clocks/exclusive all-channel ownership,
reject abort/reset/inconsistent states, issue channel disable WE strobes only,
clear global EN preserving other bits, bounded total poll budget and checked
completion. TRM v0.4 CFG0 p349 readback stays1 until activity terminates; checking
channel bits alone is insufficient. No IRQ-clear/abort/reset/clock/power/memory
writes. FIFO residue reported, no lossless-transfer guarantee. Failure in stop
phase is sticky until reset; success makes duplicate BUSY. Pending IRQ flags
retained; handle before native IRQ enable. No ISR/task/SMP/runtime ownership.
Host25/25, ASan/UBSan/C++98, four mutants and two invalid CMake profiles PASS.
Target empty-controller arm/stop PASS: CFG0->1 verified, native stop->0 in one
poll, stage3/armed1/adopted0/changed1/requested0/preserved1/duplicate_busy1, no DMA
transfer started. Active-channel stop remains HOST-TESTED; PHY/MIPI clocks and
VO3/panel-reset guards untouched. Paced shell/SciCalc14/desktop verified after
reset, no flash/SD write. Artifacts: `../maia-rtos-recovery/native-vdma-stop-20261112/`.
Next: reserved DMA-visible SRAM/cache ownership and actual active-transfer stop
fixture, then separate PHY/bridge stop before clock gating/power tests. Full
legacy desktop still uses FreeRTOS; POSIX DESIGN and native cold display pending.

## Native SRAM transfer and active VDMA stop — 2026-11-12

Read [active DMA fixture/evidence](../audits/native-vdma-transfer.md). Opt-in diagnostic reserves66048B
NOLOAD outside BSS/stack/ROM; checks actual L1D line64, invokes pinned silicon ROM
range writeback/invalidate, then uses only CPU NC alias8FF/physical DMA4FF.
No SDK headers/runtime/FreeRTOS; explicit ROM entries documented. Channel0 M2M
single-block8-bit/master1: actual full32768-byte copy, then active-channel stop
(initialCFG1/CHEN1/requested1) stage3/two polls/finalglobal0. Correct partial1152B,
zero tail31616B, source/guards intact and eight later full scans stable; sampled
MIPI/clocks/other channels unchanged. This supersedes HOST-TESTED-only active
stop for this SRAM workload, not peripheral/display/PSRAM/SMP/DMA IRQ behavior.
Host26/26, sanitizer, four compiled mutants and two invalid profiles PASS. Initial
RO/reserved INTSIGNAL readback refusal fixed with documented writable mask;
first post-fix run lost early banner, correctly rejected by capture validator.
Final complete capture PASS; no relaxed checks. Prior empty-controller binary
remains byte-identical after CMake changes. Paced shell/SciCalc14/desktop restored
and no flash/SD write. Artifacts: `../maia-rtos-recovery/native-vdma-transfer-20261112/`.
Next: DSI/CSI PHY/bridge shutdown under explicit failure/ownership policy before
clock gating/power tests. VO3 and panel-reset guards untouched; full desktop still
uses FreeRTOS, native cold display pending, POSIX DESIGN.

## Native boot MIPI reset parking — 2026-11-12

Read [MIPI reset parking](../audits/native-mipi-park.md). New native boot-exclusive API requires
VDMA global/channel/abort/reset/FIFO0, already-shutdown DSI/CSI hosts/analog PHYs,
DPI0 and DSI FIFO0. Three MIPI/both DMA bus clocks +PHY config clocks must already
be enabled. Four masked/verified bridge/digital reset steps, preserved other bits,
fresh guards/no-op skipping and sticky failure until reset. No analog/clock/power/
DMA/IRQ-status writes. Live states reject; generic live-stream stop is not ported.
TRM38.7.2 p1974 also requires shared ISP reset for CSI stop; do not skip ownership
or treat this boot-only API as complete live teardown/power-cycle permission.
Host27/27, sanitizer/C++98/four mutants/two invalid profiles PASS. Target RAM
probe enables only CSI host bus SYS11 (0C0D5678->0C0D5E78) for inspection, arms
both bridge EN and two digital reset-release bits while hosts/analog remain low.
Native stage4/attemptmaskF/four control words0/preserved1/duplicate_busy1 PASS;
clocks (prepared baseline) and sampled DMA/channel/other fields unchanged.
DSI PHY clock-lane/forcePLL clearing host-tested only. Runner host-ready byte
precedes setup/capture. Shell/SciCalc14/desktop restored after reset, no flash/SD
write. Artifacts: `../maia-rtos-recovery/native-mipi-park-20261112/`.
Next: shared ISP/boot consumer prerequisites, checked reset-state clock gating;
then VO3/reset hardware gates and native display/PSRAM. Full desktop still uses
FreeRTOS; live-stream shutdown and POSIX implementation remain pending.

## Checked native MIPI clock gating — 2026-11-12

Read [MIPI gate evidence](../audits/native-mipi-gate.md). Gate API requires internal native park
claim, inactive VDMA and reset-state MIPI. Initial clock-off-only ISP guard
refused02E82000; actual inspection found CNTL40002442/CAM4 (CSI input disabled,
DVP disabled/held reset). Final guard accepts only that disconnected input mode
when clocked, or skips unreadable clock-off/held-reset ISP; enabled/other sources
reject. No ISP clock enabling or fabricated idle signal. Hold ISP/CSI host/CSI
bridge/DSI bridge resets, then clear PHY93 and SYS1C00, checking full readbacks/
preserved bits and fresh RCC/VDMA. No MIPI/ISP MMIO after reset; DMA clocks stay on.
Failure after guards sticky until reset, no rollback/retry; duplicate BUSY.
Target RAM PASS: reset40->3C000040, PHY2B3->220, SYS0C0D5E78->0C0D4278;
stage5/attempt7/preserved1/duplicate_busy1. ISP clock remains on, module held reset.
Host28/28, sanitizer/C++98/four mutants and invalid profile PASS. Preserved flash
shell/SciCalc14/desktop verified after reset; no flash/SD/LDO/analog-power writes.
Artifacts: `../maia-rtos-recovery/native-mipi-gate-20261112/`. This resolves only
boot-reset clock gating; not live ISP/CSI/video stop, electrical settling or an
unconditional power-cycle permission. Next: compose guarded VO3 off/acquire test
with native park/gate, retaining supply/discharge/stabilization limitations.
Full desktop still FreeRTOS-based; native cold display/PSRAM/POSIX pending.

## Guarded native VO3 off/acquire — 2026-11-12

Read [VO3 cycle audit](../audits/native-vo3-cycle.md). Opt-in RAM diagnostic composes native park/gate
with calibrated VO3 off request and unchanged native acquire. Boot CSI preparation
now checks idle DMA/disconnected or reset-held ISP, asserts CSI resets, enables
only its access/config clocks, releases CSI resets, then reads shutdown defaults.
Actual inherited RESET38000040 ->20000040 retains ISP reset; warm RAM/reset is
not a cold state. Early bus-clock refusal and CSI access-fault candidates are
preserved; neither reached VO3 writes. VO3 profile installs native trap vector.
Final target PASS: held reset3C000040/gated SYS0C0D4278/PHY220,
CTRL40200180 ->40200080 ->40200180, ANA D5800000/dref13/mul3 retained.
Native acquire verified5/non-adopted/changed,16 stable samples per phase, ten
other LDO words and clock/reset/VDMA guards preserved, duplicate BUSY. Host quiet
windows20.946/21.124ms are not physical voltage/settling proof; analog/source
programming was a no-op because values matched. Host30/30, two sanitizer fixtures,
four compiled negative mutants and invalid profile PASS. Shell/SciCalc14/desktop
restored, no flash/SD writes; artifacts `../maia-rtos-recovery/native-vo3-cycle-20261112/`.
No public API/default firmware change or new foreign runtime dependency. Full
desktop still FreeRTOS-based. Next: checked native DSI clock/PHY prerequisites;
cold panel/display/PSRAM, electrical characterization/live stream shutdown and
POSIX implementation remain pending.

## Native DSI configuration access — 2026-11-12

Read [native DSI access](../audits/native-dsi-access.md). New boot-exclusive SoC API requires all MIPI
clocks gated, shared resets held, overrides clear, inactive clocked VDMA and
matching active calibrated VO3. Enables only DSI SYS12/PHY config0, then releases
only DSI bridge RCC reset26, with fresh full guards/readbacks. No DSI MMIO before
access/config clocks and reset release; no ISP/CSI MMIO. Target RAM stage5/attempt7
PASS: SYS0C0D4278->0C0D5278, PHY220->221, RESET3C000040->38000040; VO3 unchanged.
VERSION3134312A, POWER/PHY_RSTZ/BRIDGE/FIFO0, DPI19D0(enable0), STATUS1528 observed,
not treated as PHY-ready proof. CSI/ISP resets held; reference/DPI clocks remain
gated. No PLL/source/test-port/analog/lane/video/panel/DMA/PSRAM writes.
Sticky failure/no rollback, duplicate BUSY without I/O; no runtime ownership or
release implementation. Host31/31, sanitizer, C++98, four compiled mutants and
invalid profile PASS. Shell/SciCalc14/desktop recovery verified; no flash/SD writes.
Recovery `../maia-rtos-recovery/native-dsi-access-20261112/`. Default production
firmware unchanged and still FreeRTOS-based. Next: verified native DSI reference
source/provider/frequency, then PHY PLL/lane sequence; cold display/PSRAM/electrical
settling and POSIX implementation remain pending. No parser changes.

## Native nominal20MHz DSI PHY reference — 2026-11-13

Read [DSI reference](../audits/native-dsi-reference.md). Existing native DSI module now requires internal
successful access claim, quiet DSI/VDMA, calibrated VO3, CSI/ISP reset-held state,
LP XTAL40/SPLL480 gates and shared REF20 divider24. Never reprograms shared divider
or root PLL/LP power. Enables provider if needed, gates config around mux change
(the mux feeds both config/reference), restores config, then enables reference.
No gated DSI MMIO; fresh full guards/readbacks, skipped matching writes, sticky
failure/no rollback, duplicate BUSY without I/O. Not a runtime clock manager.
Actual Tab5 RAM PASS stage7/mask10: divider58170503/LP1FFCFFFD/provider115/source
38862600 retained, PHY221->223 one actual reference-gate write;16 stable samples
and supplied VO3/eFuse preserved. Model20MHz, no physical measurement/PLL-ready
claim. Provider enable/nonzero mux switch branches HOST-TESTED only. Host32/32,
sanitizer ASan/UBSan, C++98, four compiled mutants/invalid profile PASS. Original
shell/SciCalc14/desktop restored, no flash/SD writes. Recovery:
`../maia-rtos-recovery/native-dsi-reference-20261113/`. Default firmware still
FreeRTOS-based. Next: native PHY PLL/test-port configuration with bounded failure,
then analog/lane lock/stop waits; no cold display/PSRAM/POSIX implementation yet.

## Native two-lane960Mbps DSI PHY — 2026-11-13

Read [native PHY](../audits/native-dsi-phy.md). New native PHY API requires internal reference claim,
quiet inactive DMA/bridge/DPI/host/PHY, no HS request, lock initially low and fresh
clock/reset/provider/divider/supply guards. Adapted pinned SDK init/test-port byte
sequence, Apache-2.0 attributed in dsi_phy_program.h; no foreign runtime imported.
Fixed M48/N1/hsfreq1A =>nominal960Mbps (working BSP requests965, SDK rounds960).
Masked checked lane/host/analog/reset/clock/forcePLL and five byte writes, RO test
output excluded from writable readback. Caller budget1..100000 observes lockbit0
and all lane-stop94; sticky failure/timeout until reset, no rollback. PHY can drive
LP idle, no packet/HS/video/panel command. Actual Tab5 RAM stage11 PASS:40 stores
(lanes already1), five bytes, seven observations, STATUS15BD/power1/PHY_RSTZF;
16 stable samples/guards preserved/duplicate BUSY. Not measured bitrate/timing.
Host33/33/sanitizer/C++98/four compiled mutants/invalid profile PASS;41 dropped-
store tests distinguish37 real failures from4 harmless identical clock-low stores.
Shell/SciCalc14/desktop restored; no flash/SD writes. Recovery
`../maia-rtos-recovery/native-dsi-phy-20261113/`. Default desktop still FreeRTOS.
Next: native command-mode/escape+timeout clock configuration and bounded LP FIFO
transfers, then panel DCS initialization; video/PSRAM/full native desktop pending.

## Native DSI command-mode host configuration — 2026-11-13

Read [command configuration](../audits/native-dsi-command.md). Native boot API requires internal PHY claim,
locked/stopped2lanes/quiet VDMA/bridge/DPI, stable clock/supply and generic FIFOs
empty/not busy.14 masked checked steps: mode1, timeoutdiv12/escapediv6, SDK PHY
switch/read/stop timings, CRC/ECC/HS-EoTp. Hardware timeout counts0 by setup policy;
no transaction/unbounded wait introduced. Packet-specific LP/BTA/read handling pending.
Fresh full table/root/PMU/PHY/FIFO guards, no-op skipping, sticky failure/duplicate
BUSY without I/O. Attributed Apache-2.0 table port; no foreign runtime headers/API.
Actual Tab5 RAM stage16/mask181E PASS: six writes, CLKMGR0C06/DATA00320068/
CLK002E0080/PCK19/MAXRD1770/PHY_IF3F01, MODE already1. STATUS15BD/FIFO00050015
(low7=15),16 later stable samples/guards preserved. Modeled20/10MHz, not measured.
No header/payload/HS/panel/video/DMA/PSRAM writes. Host34/34/sanitizer/C++98/four
compiled mutants/invalid profile PASS. Original shell/SciCalc14/desktop restored,
no flash/SD writes. Recovery `../maia-rtos-recovery/native-dsi-command-20261113/`.
Default desktop still FreeRTOS. Next: packet-specific LP/VC settings and bounded
short writes, distinguishing host completion from panel ACK; then panel DCS/video/
framebuffer/PSRAM. No full native desktop or POSIX implementation claimed.

## Bounded native LP DCS short writes — 2026-11-13

Read [LP short transport](../audits/native-dsi-short.md). Native boot API requires internal command claim,
stable cached14 config/root/supply/PHY, empty generic FIFOs and ACK/TE/BTA off.
DCS0/1, VC0..3/byte validation, LP bit16/17 masked, one GEN_HDR publication (no
readback/payload), finite budget1..100000 observations, allow lane-stop activity
while PLL stays locked. Read/OR error0mask1FFFFF/error1mask81FFF; reads may consume
latches, no clear/mask writes. Sticky failure/timeout, no rollback/retry after
possible publication. Success allows next exclusive call; not a runtime queue.
Clock guard before LP readback fixed under host fault injection before target load.
Actual Tab516 VC0 NOPs PASS: stage5/header05/attempted1, each2polls/busy witnessed;
mode0->10000 once, final STATUS15BD/FIFO50015/errors0, prior config/root/supply
preserved. panel_ack0: host drain/stop, not panel reception proof. Fast/unobservable
publication limits explicit; DCS1/nonzeroVC host-tested only. Host35/35/sanitizer/
C++98/five compiled mutants/invalid profile PASS. Shell/SciCalc14/desktop restored,
no flash/SD writes. Recovery `../maia-rtos-recovery/native-dsi-short-20261113/`.
New packing helper Apache-2.0 attributed, no foreign runtime or parser changes.
Default desktop still FreeRTOS. Next: bounded LP long payload writes for vendor
panel startup, then panel-specific reset/DCS integration; ACK/read/video/PSRAM/
full native desktop and POSIX implementation remain pending.

## Bounded native LP DCS long framing — 2026-11-14

Read [long transport](../audits/native-dsi-long.md). New boot API stages1..64 payload bytes (command
included) then exact WC/DCS39 header.64 is policy, not verified FIFO depth. Bytewise
unaligned/end-safe packing and zero high tail bytes; no SDK overread/unbounded wait.
Aggregate budget covers space/header/drain, no reset per word. Partial payload
failure poisons BOTH long/short paths until reset, no flush/retry/header on partial
WC. Shared guards renamed, short semantics otherwise preserved. Attributed Apache
packing helper, no runtime imports. First target capture exposed internal buffer
pending while front FIFO empty (10015). Preserved initial image/log; strengthened
long initial/final maskF007F==50015, command30003==10001, room80008, then retested.
Final Tab54 long VC0/NOP00/WC1 PASS: stage6/header139/1word/1byte,4polls/2space/busy
witness each, pre10015->final50015/STATUS15BD/errors0, LP10000->90000 once; short
NOP followup PASS. Full prior config/root/supply retained, panel_ack0. Multiword/
lengths2..64/nonzeroVC host-only; no panel acceptance or initialization claim.
Host36/36/sanitizer/C++98/seven compiled mutants/invalid profile PASS. Shell/SciCalc
14/desktop restored, no flash/SD writes. Recovery
`../maia-rtos-recovery/native-dsi-long-20261114/` (build paths13 reflect start date).
Default desktop still FreeRTOS. Next: bounded read/BTA for panel response/identity,
then verified reset/DCS startup; video/PSRAM/full native desktop/POSIX pending.

## Native bounded RDDID read: reproducible PHY refusal — 2026-11-14

Read [read/BTA audit](../audits/native-dsi-read.md). Fixed VC0 DCS04/MRPS3 native API, bounded aggregate
poll budget, one RX-word maximum, error/guard checks, sticky shared failure and
success-only restoration of packet/RXVC/LP controls. No SDK/FreeRTOS runtime/header
imports; Apache-2.0 attributed packet definitions. HOST-TESTED successful path;
actual target successful read NOT DEMONSTRATED. Two RAM loads reproduced IO at
stage6/headers3/polls6, FIFO50005 (data available), PHY15BD, ST0=00100000
(DPHY_ERRORS_4). Rejected BEFORE pop, words0/restored0; zero ID fields mean unread.
No masking/clearing/automatic retry. Both preceding native short/long runs pass.
37/37 host tests/sanitizers/C++98/seven mutants/invalid profile PASS; runtime gate
clean. Exact error+RX-ready and capture false-success regressions included.
Both shell/SciCalc14/desktop recoveries verified; no flash/SD/default/parser changes.
Recovery `../maia-rtos-recovery/native-dsi-read-20261114/`. Working desktop remains
FreeRTOS-based. Next: resolve P4-specific error semantics and BTA/turnaround cause;
require error-free panel responses before panel-specific startup/video/PSRAM.
Do not ignore error20 based on SDK omissions or a different controller's manual.

## Native RDDID response after escape divider correction — 2026-11-14

Read [escape/BTA correction](../audits/native-dsi-bta-escape.md). The prior ede1698 refusal is historical;
successful bounded response is now HARDWARE-TESTED, LIMITED. Fixed CLKMGR0C06->
0C07 follows official Espressif cebb3dd4d8704a5aa1c2b3a23c4c4b9b26e71194 rounded
18MHz-target divider policy for960Mbps: byte120MHz/7 = modeled17.142857MHz.
Report derives integer17142857Hz from checked divider. No relaxed error masks,
BTA/read guards or invented turnaround registers; exact electrical cause unmeasured.
Two FINAL RAM loads each pass4 RDDID responses: stage10/headers3/oneword00FBA080,
low bytes80 A0 FB/polls7/errors0/PHY15BD/FIFO50015, mode90000 restored, mask5,
stable1/preserved1. Model/exact response length NOT established. Preceding native
16short/4long/followup and all clock/supply checks pass. Intermediate successful
runs with stale frequency report are preserved separately. Host37/37, four
ASan+UBSan fixtures/C++98/two targeted mutants PASS; native runtime gate clean.
Shell/SciCalc14/desktop recovered, no flash/SD/foreign-runtime/parser changes.
Recovery `../maia-rtos-recovery/native-dsi-bta-escape-20261114/`. Default desktop
still FreeRTOS. Next: guarded native panel reset integration and variant-verified
bounded DCS startup, then video/framebuffer/PSRAM; no native desktop/POSIX claim.

## Quiet native panel reset before DSI responses — 2026-11-14

Read [integrated reset](../audits/native-dsi-reset.md). New opt-in DSI_RESET profile reuses existing
native I2C/expander/board reset after proven MIPI gate+VO3, before DSI open. Exact
RCC/ISP/VDMA/supply guards, supported firmware1/3, verified GPIO23 input, both-bank
preservation and touch recovery; any error halts, no rollback/retry. Private boot
diagnostic only, no new production/public driver/parser changes. Actual two final
RAM loads PASS: sys0C0D4278/phy220/reset3C000040, stage9/reset~244ms, FW1/status0,
banks/clocks/supply/GPIO preserved, then four stable00FBA080 error-free RDDID words
each and prior native DSI/short/long checks. Host38/38, two ASan+UBSan fixtures/four
compiled mutants/invalid profile PASS, native runtime gate clean. Shell/SciCalc14/
desktop restored, no flash/SD writes. Recovery
`../maia-rtos-recovery/native-dsi-reset-20261114/`. Earlier standalone reset capture
remains qualified; new gate evidence applies to this integrated profile only.
Important: preserved BSP maps observed touch FW1 to ST7121, FW3 to ST7123. Do not
reverse this or guess from DSI raw word. Next: selected ST7121 command-table port
with bounded delays/failure policy; video/framebuffer/PSRAM/full native desktop/
POSIX pending. Default full desktop still FreeRTOS-based.

## Selected ST7121 command program and generic FIFO space fix — 2026-11-14

Read [program audit](../audits/native-st7121-program.md). New native board API executes exact883e9a6 software
reset+vendor table:31 packets/461bytes/requested1150ms, max61byte packet, firmware1
only. Portable checked callbacks, no heap/MMIO/foreign runtime; single lifetime
claim, success duplicate BUSY/failure sticky FAILED, no rollback/retry. Apache-2.0
table attribution retained; no DPI init/final duplicate29 imported. First target
attempt stopped A3/40bytes at stage3 after8 staged bytes/FIFO90015/internal-full
but generic-front not full. Corrected only payload-space predicate per pinnedSDK
LL: use front fullbit3, not internalbit19. Both initial/final buffer completion,
errors/guards/budget/partial-sticky policy retained. Refusal images/logs preserved.
Two final RAM loads pass31 packets including A7/61bytes(stage6/23polls), full waits,
then4 stable00FBA080 words each/stage10/errors0/modeB0000 restored/mask5. Host39/39,
two ASan+UBSan fixtures/C++98/five runtime mutants/invalid profile/table comparison
PASS; native runtime gate clean. Original shell/SciCalc14/desktop restored, no
flash/SD/parser changes. Recovery `../maia-rtos-recovery/native-st7121-20261114/`.
Next bounded panel-state readback before DPI/video/framebuffer/PSRAM. Program
completion does NOT prove vendor registers/sleep/display state/pixels. Default
complete desktop still FreeRTOS-based; POSIX remains DESIGN.

## Native panel power-state readback — 2026-11-14

Read [power-state audit](../audits/native-panel-power.md). New fixed VC0 RDDPM0A/MRPS1 API shares the existing
private bounded read state machine with RDDID04/MRPS3; guards/errors/sticky claim,
one RX pop and success-only host restoration unchanged. Low byte only is power
payload; upper word/id[1..2] are not requested data. MRPS1 persists until next
request. No arbitrary read API/foreign runtime/parser/board-table change.
Two final RAM loads each read4 stable08 before and4 stable9C after unchanged
ST7121 program. Every power read stage10/headerA06/headers3/polls6/oneword/errors0,
PHY15BD/FIFO50015, mode90000 orB0000 restored/mask5. Standard DCS bit4 SleepOut
andbit2 DisplayOn transition0->1; stronger than command drain, still no pixels.
Primary pinnedSDK0A and Linuxv6.12 bit facts, corroborating Sitronix ST7121P V1.0
2024/01 p103; P suffix not established for this device. No Linux code imported.
Host39/39/ASan+UBSan/C++98/five runtime mutants/invalid profile PASS. Capture tests
reject already-on baseline/missing transition and allow unrelated bit changes.
Runtime gate clean; prior native program/reset/read checks pass. Shell/SciCalc14/
desktop recovered, no flash/SD writes. Recovery
`../maia-rtos-recovery/native-panel-power-20261114/`. Next native DPI format/clock/
timing with DMA inactive and matched panel format, then video/framebuffer/PSRAM.
Complete installed desktop still FreeRTOS-based; no pixels/POSIX claim.

## Inactive native DPI format/clock-route/timing stage — 2026-11-14

Read [DPI staging audit](../audits/native-dpi-stage.md). New generic SoC timing API stages19 words,
RGB565/VC0/burst+LP, nominalPLL240/div3=80MHz for existingPHY960. Geometry fromBSP
fixture720x1280/H2,40,40/V20,24,200; no resolution in generic kernel. Working BSP
requests70MHz but pinned integer divider produces80; report modeled only, gateoff.
Native exact RMW/readback/root/supply/VDMA/command/PHY/FIFO guards; frameACK/pattern
mustoff. No bridge update/output/video/HS/DMA/framebuffer/IRQ/provider/PLL writes.
Partial failure poisons stage+transfers until reboot; duplicate BUSY without I/O;
post-stage command reads work. Host40/40/ASan+UBSan/C++98/five runtime mutants/
invalid profile PASS, including20 dropped stores, clock-loss readback protection,
no-op profile and invalid geometry. Native runtime gate clean.
Two final RAM loads PASS stage22/maskFFF30(14stores), route223->223,19 words checked,
MODE1/bridge0/DPI19D0/clock223 gateoff/update0/DMAquiet1/preserved1/duplicateBUSY1;
four post-stage power words9C each/stage10/errors0/restored controls. Nonmatching
route-write branch HOST-TESTED only; live timing latch/actual frequency not proven.
Original shell/SciCalc14/desktop restored; no flash/SD/parser/default changes.
Recovery `../maia-rtos-recovery/native-dpi-stage-20261114/`. Next panel-format
verification and native clock source/provider/enable with output inactive, then
latch/video/framebuffer/PSRAM. No pixels/full native desktop/POSIX; installed
working desktop still FreeRTOS-based.

## Reported RGB565 selection and inactive DPI clock enable — 2026-11-14

Read [format/clock audit](../audits/native-dpi-clock.md). Fixed color0C/MRPS1 read shares native guarded
transport; private staging cache and new clock API require actual stage claim,
exact19 words/root/supply/PHY/mode, bridge/output/DMA inactive. Enable REF240 bit30
atHP+28 (preserve shared20MHz divider), then DPIgate7 atHP+3C, no source retune/
PLL/latch/video/HS/DMA/IRQ writes. Duplicate BUSY/no I/O; partial failure poisons
all transfers until reboot. LP preflights accept clock-on only with private claim,
correct240/div3 route/provider; external enable and provider/route/gate drift reject.
First format read00 was refused before writes; documented initial00 and M5GFX
28d47949 COLMOD55 policy support explicit selection, not accepting00 as16-bit.
Two final RAM loads: color00->05 after one3A55 (header553A15), then clock223->2A3/
provider58170503 unchanged/mask2/stage4, MODE1/bridge0/DPI19D0/update0/DMAquiet1.
Four post-clock color05 and power9C reads each/errors0/restored LP controls.
Nominal80MHz only, no measurement; provider-off/skip-format branches host-only.
Host40/40/two ASan+UBSan fixtures/C++98/six runtime mutants/invalid profile PASS;
runtime gate clean. Shell/SciCalc14/desktop restored, no flash/SD/parser/table edits.
Recovery `../maia-rtos-recovery/native-dpi-clock-20261114/`, includes initial refusal.
DBI16-bit readback is not actual wire/pixel proof; supplier ST7121P suffix unproven
and its datatype table lists24-bit only. Working BSP/M5GFX use565; next guarded
latch/native test pattern (noDMA) to prove pixels/compatibility before framebuffer/
PSRAM. No full native desktop/POSIX; installed complete desktop still FreeRTOS.

## Native host color bars visible; bounded return to commands — 2026-11-14

Read [pattern audit](../audits/native-dpi-pattern.md). Native boot-only pattern start/stop requires private
stage+clock ownership, full cached configuration/root/supply/PHY guards, shadow0,
bridge/output/DMA inactive. Checked SDK vertical VPG mode (owner observed horizontal bars), automatic HS request,
video mode; exclusive transport BUSY during active pattern. Stop command/LP,
bounded stopped/empty TX wait, exact VPG restore; partial failure sticky/reboot.
First attempt had error-free controls but owner saw black. Final diagnostic adds
private saved/restored GPIO22 DC HIGH lighting (Tab5 PDF p12 G22/LEDA, pinned GPIO
fields); owner confirmed stable bars. Final native RAM SHA22fa5cedf860437a2f1ec529
1119654cecab8008a6d0d452e3f8c1a84e4a91b2. VID1BF02/MODE0/LP3 for5 samples,
PHY1529/15B9/errors0; stop VIDBF02/MODE1/LP0 in1 poll, GPIO restored,4 power9C
responses/errors0/controls restored. Bridge/update/output/DMA stayed inactive.
Host41/41, two ASan+UBSan fixtures, C++98, six rejected runtime mutants and invalid
profile PASS; native runtime gate clean. No flash/SD/parser/table/timing changes.
Recovery `../maia-rtos-recovery/native-dpi-pattern-20261114/`. Installed full desktop
still FreeRTOS; restored shell/SciCalc14/desktop. Host pattern bypasses bridge, so
no DMA/framebuffer/PSRAM or full native desktop proof. Next guarded bridge/DPI latch
with output/DMA off, then owned SRAM framebuffer; POSIX remains DESIGN.

Orientation clarification: the owner observed HORIZONTAL stable bars. Production
`platforms/tab5/main/hal/hal_esp32.cpp:104` uses `LV_DISPLAY_ROTATION_90`, software
rotation: native panel720x1280, logical desktop1280x720. The host VPG bypasses LVGL
rotation. Preserve desktop and input-coordinate orientation when implementing
native framebuffer presentation; verify direction with an asymmetric pattern.
See [correction](../audits/native-dpi-pattern-orientation.md).

## Native inactive bridge enable and update publication — 2026-11-14

Read [bridge audit](../audits/native-dpi-bridge.md). New native boot/core0/exclusive API requires real
stage+clock claims and cached19/root/supply/PHY/LP-command guards, stopped/empty TX,
bridge/output/DMA/VPG/shadow/frameACK/IRQ enables off. Checked bridge EN0->1,
then literal1 to DPI_UPDATE+44. Register is WT, no ACK: no read/poll/RMW/retry;
latch_verified0, dropped publication explicitly unobservable in host fixture.
Private expected bridge bit permits LP traffic only after checked native enable;
external adoption/loss refuses. Original reference-acquisition bridge-off guard
preserved. Duplicate BUSY/no I/O; partial failure sticky/reboot, VPG refused once
bridge owned. Final native RAM load PASS stage4/mask3/EN1/DPI19D0/MODE1/LP0/
DMAquiet1/IRQ_RAW0/errors0 and four power9C words/errors0/restored controls. Earlier
candidate also passed, images/captures separate. Final SHA213a32b2288b58ebe4ea8363
d267a3b38c5d4cfbfc15a29ee5ac6999b476e10d. Host41/41/ASan+UBSan/C++98/six rejected
runtime mutants/invalid profile PASS; native runtime gate clean. Shell/SciCalc14/
desktop restored; no flash/SD/parser/table/timing/production rotation changes.
Recovery `../maia-rtos-recovery/native-dpi-bridge-20261114/`. No live latch or
DMA/framebuffer pixels claim. Next owned SRAM DMA/bridge flow/descriptor contract
and inactive staging before pixels; full frame1,843,200bytes cannot fit the tested
RAM-image window. PSRAM and full native desktop pending; installed complete desktop
still FreeRTOS. Logical1280x720/productionLV_DISPLAY_ROTATION_90 and owner's
horizontal visible-pattern clarification retained. POSIX remains DESIGN.

## Owned SRAM tile and finite DMA descriptors, no engine activation — 2026-11-15

Read [tile/list audit](../audits/native-dpi-dma-tile.md). New private SoC pure plan/encoder validates bounded
RGB565 geometry, disjoint/aligned diagnostic SRAM capacities and item indices.
Source8rows/11,520bytes reused by160x64-byte LLIs, finite tail0/LAST only tail,
source/list master1, DSI destination50105000/master0/fixed,64-bit/item counts.
Host-only independent pinned SDK types/map oracle confirms constants. No target
SDK headers/runtime/allocator imported. Opt-in22,272byte NOLOAD reservation outside
BSS/stack, ROM cache WB/invalidate only owned range, then CPU-only NC access.
One RAM load: reserve4FF0FC80..4FF15380, source4FF0FD00/list4FF12B00/cacheline64;
source hashE18C2E45/listF8CAF0B9 independently match, guards/preserved/inert1.
All160 items read back,3 samples correct; four power9C reads/errors0/restored.
No DMA register programming or LLP publication/channel/video/pixel enable. Thus
HARDWARE-TESTED memory construction only, not descriptor fetch/pixels/PSRAM.
Host42/42/ASan+UBSan/C++98/six rejected mutants/invalid profile/oracle PASS; runtime
gate clean. Image SHA dfc11fe6b6012e7411e3cfaac2175c80383ee53ca307ab9ccc18d5a85d69f57b.
Shell/SciCalc14/desktop restored; no flash/SD/parser/table/timing/rotation changes.
Recovery `../maia-rtos-recovery/native-dpi-dma-tile-20261115/`. Next checked inactive
channel/list/handshake/bridge flow staging, then bounded transfer/stop/pixels.
Full installed desktop still FreeRTOS; native diagnostic independent. Keep owner's
horizontal-bar observation and logical1280x720/software90 rotation contract; POSIX
DESIGN. SDK marker-helper comment ambiguity remains for future actual fetch gate.

## Inactive single-block channel0 and DSI flow staging — 2026-11-15

Read [DMA stage audit](../audits/native-dpi-dma-stage.md). New boot-only native solid-source API requires
actual bridge/stage/clock claims and full idle/root/config guards. Bridge frame
count field4bits cannot directly encode160; prior tile/list remains CPU-only and
unused. Stage contiguous single frame230400x64-bit, fixed8-byte source and fixed
DSI50105000, no LLP, DMAC flow/HW DSI0 handshake, priority1/outstanding5,2. Source
single-item/no ARLEN override, destination256items/AWLENfield15. Sixteen masked
words checked, reserved bits preserved; no enable/IRQ/WT/status-clear writes.
First optional status-address read50081148 trapped before configuration stores;
all four optional status-fetch addresses now excluded/writeback disabled. AWLEN16
read15; strict refusal retained, final explicit15 verified. Refusal images/logs
preserved; one failed diagnostic rebuild repeated prior image, not new evidence.
Final RAM PASS stage18/maskD975/source4FF10E80/CTL1F8000/CFG1A020001/globalCFG0/
channels0/DPIoff, hashes unchanged and four power9C replies/errors0. Host42/42,
ASan+UBSan/C++98/seven rejected mutants/invalid profile/SDK oracle PASS; runtime
gate clean. Final SHA20fe0dcdba0dc61ee1fbaf34163fc039a45a6788d408ff021795696a11d19287.
Shell/SciCalc14/desktop restored; no flash/SD/parser/table/timing/rotation changes.
Recovery `../maia-rtos-recovery/native-dpi-dma-stage-20261115/`. No transfer/pixels
proof yet. Next bounded solid single-frame activation/completion/stop and visual
color with temporary light; do not enable160-LLI plan. Optional-register fault and
AWLEN normalization root causes open. Native full desktop/PSRAM/rotation-input
acceptance pending; prior bars observed horizontal; POSIX DESIGN.

## One finite DMA frame completed/recovered; visual red unconfirmed — 2026-11-15

Read [frame audit](../audits/native-dpi-dma-frame.md). New boot-only API consumes private solid-stage claim:
globalCFG1(no IRQ), channel101, MODE0/LP3/output1/WT, bounded completion/drain and
guarded cleanup channel100/global0/output0/WT/MODE1/LP0. Transfer/recovery results
separate; errors retained; lost ownership can prevent cleanup. Single use/BUSY,
partial failure sticky. Active guards allow live progress; source immutable8bytes.
Initial frame DMA-done/FIFOempty but raw count383FF was refused against38400.
Separate SRAM copies verify actual8/64/256/32768bytes with rawN-1 for8/64-bit data.
Expose raw/normalized separately: only done+natural-disable+expectedraw confirms
38400items. Qualification first HOST FAIL was an unnecessary done-bit requirement
(M2M target status0 with exact bytes); corrected byte-proof evaluator revalidated
the exact rows, preserving original raw FAIL. No display done requirement relaxed.
Final frame PASS: stage9/maskFFF/active1/done1/raw383FF/items38400/status12/errors0,
967 completion/2 recovery polls/13550us sample, all enables0/commands restored,
GPIO restored/hashes unchanged/four power9C replies. Visible red NOT CONFIRMED.
Host42/42/ASan+UBSan/C++98/six rejected mutants/invalid profile PASS; runtime gates
clean. Final SHAde826aeeb87f82a3a43d2e3e17bf4856b0d470525ffaebae56083924e2cc2e03.
Shell/SciCalc14/desktop restored; no flash/SD/parser/table/timing/rotation changes.
Recovery `../maia-rtos-recovery/native-dpi-dma-frame-20261115/`. Bridge raw0 is at
existing discard threshold413, not an all-pixel quality proof. Next bounded visual
refresh/color/underrun qualification, then framebuffer/PSRAM. Native full desktop
pending; installed complete desktop FreeRTOS; prior bars horizontal; POSIX DESIGN.

## Bounded native DMA refresh: stable red confirmed — 2026-11-15

Read [refresh audit](../audits/native-dpi-dma-refresh.md). Boot-only exclusive immutable-source batch1..400,
per-frame completion/recovery and fresh idle guards before status-clear/rearm;
only completion/disabled mask4000001B cleared, failures sticky/no retry. Old
standalone frame stays one-shot. RAM opt-in400 frames after explicit user readiness:
HOST PASS, 399 rearms/436369 polls/92160000 items/6109505us sample, last active/done1,
raw383FF/items38400/status12/errors0/FIFO0, recovered and four power9C replies.
User: “Sim, vermelho estável”. Previous short test had no valid visual observation
because user was not in front. Source/list hashes unchanged, light restored;
reset shell/SciCalc14/desktop PASS. Host42/42, sanitizers/C++98/five rejected
mutants/invalid profile/runtime gate PASS. SHA48ff157bbb7e68df5a2b65c32468d829371628ae251ae750c0df864b0d561650.
Recovery `../maia-rtos-recovery/native-dpi-dma-refresh-20261115/`. No flash/SD writes,
foreign runtime/parser/timing/rotation change. Diagnostic IRQs off; bounded batch
is not a production stream. Threshold413 still limits underrun interpretation.
Next owned finite SRAM pixel tile/source-increment spatial-color qualification;
PSRAM/full native desktop/rotation-input still pending, installed desktop original.

## Finite SRAM descriptor fetch/copy qualified — 2026-11-15

Read [list audit](../audits/native-vdma-list.md). New isolated VDMA_LIST probe: two finite64-byte LLIs,
first RGB565720x8 tile source increment11520bytes, second fixed8-byte8410 source
21248bytes; destination32768 bytes. Initial refusal16832bytes and descriptor
changes preserved; sampling proves8correct+24A5 per32bytes with automatic ARLEN.
Explicit second-node ARLEN_EN1/field0 gives exact32768bytes; not a universal AXI
encoding claim. LLI writeback clears VALID, writes per-block raw counts/status
(first59F/40000000, tailA5F/C0000000) despite SSTAT/DSTAT fetch disabled. New
contract requires writable exclusive descriptors, exact expected writeback; no
arbitrary mutation mask. Guards/source/unrelated peripherals retained; global/
channels/FIFO0, natural stop1, polls4170, runner PASS. Host43/43+ASan/UBSan PASS,
runtime gate clean. SHA c5a06dd084445025f0e314024b8cff93447da9e34ae33b09994dde2750affcc7.
Reset shell/SciCalc14/desktop PASS; no flash/SD/parser/panel/rotation changes.
Recovery `../maia-rtos-recovery/native-vdma-list-20261115/`. No display pixels
proven in this unit. Prior solid stage's automatic ARLEN does not prove claimed
8-byte footprint: red observation holds, containing tile was owned/red, but force/
qualify explicit source length before future fixed-source reuse. Next <=15-block
finite spatial display diagnostic with source increment and LLI writeback handling;
no160-LLI activation. PSRAM/full native desktop/production mapping still pending.

## Five-node native display chain; four stable bands confirmed — 2026-11-15

Read [spatial audit](../audits/native-dpi-dma-spatial.md). New opt-in private boot diagnostic, actual source tile
and writable five-LLI list, no160-LLI activation. Source increment for8 native rows,
then four solid bands312/320/320/320 rows; count5 fits bridge. Explicit ARLEN_EN1/
field0, source64-bit, finite LAST/terminal0, all exact LLI writeback checked. Existing
contiguous-frame completion contract unchanged; spatial path accepts only all five
writeback completion records/final DMA-done plus natural disable/rawE0FF/drain.
400 individually completed/recovered frames,399 checked rearms/414370 polls/
92160000 items/6109911us sample; stage18/frame9/maskFFF/status12/errors0/FIFOs0.
All80 descriptor words/source hashE18C2E45/unused tail/guards retained as specified.
Output/global/channel/command/LP/light restored; four9C replies; runner PASS.
Fresh readiness then one RAM upload. User confirmed four large stable bands but
DID NOT identify narrow strip: no complete incremental-pixel visual acceptance.
Initial capture45 command rejected before serial; actual capture30, same image.
Host44/44, ASan/UBSan, C++98/four rejected runtime mutants/invalid profile/runtime
gate PASS. SHA b2ae7a91e8f42a93283d39349cd639ca58df4b76db58a6f810b415f3e83a7648.
Reset shell/SciCalc14/desktop PASS. Recovery
`../maia-rtos-recovery/native-dpi-spatial-20261115/`. No flash/SD/parser/panel timing/
rotation change. Diagnostic boot IRQs off; not production streaming/desktop.
Next make incremental-source marker thicker/central within owned SRAM bounds,
then fresh visual readiness/test. Threshold413/WT no-ACK/counter-doc limitations
remain. PSRAM/full native framebuffer/desktop/production rotation-input pending;
installed desktop still original runtime. Prior solid API auto-ARLEN footprint
limitation remains documented in SRAM-list audit; new spatial path forces length.

## Central32-row native tile: stable aligned segments confirmed — 2026-11-15

Read [central audit](../audits/native-dpi-dma-center.md). Separate opt-in CENTER profile/reservation46848bytes,
source46080/list320used+64unused; legacy eight-row profile remains available.
Five nodes: fixed red320rows, green304, incrementing tile32rows at native624–655,
blue304, yellow320. Full frame unchanged1843200bytes, native720x1280/rotation90
untouched. Guarded source span grows; actual descriptor/cleanup protocol unchanged.
RAM PASS:400frames/399rearms/414268polls/92160000items/6109902us sample, last rawE0FF/
status12/errors0/FIFOs0, all80 LLI words exact, source hash4B6A57C5/tail/guards kept,
GPIO/output/global/channel/command/LP restored, four power9C replies. Buffer:
4FF12580..4FF1DC80; source4FF12600/list4FF1DA80;16KiB stack ends4FF21C80<4FF28000.
Host46/46+ASan/UBSan/central C++98/four rejected runtime mutants/invalid profile/
runtime gate PASS; prior actual eight-row capture still validates as legacy only.
SHA834ce950de16f5a39b6aca27bf0c7c136ee7d6f554621b00549bff1e16e02435.
Fresh readiness, ONE RAM upload. User initially selected visual fault/other, then
said small colored pieces at center; after expected32x180segment explanation,
confirmed “Sim, estáveis e alinhados no centro”. Preserve original answer and
clarification, not a bug fix/retest. Accept qualitative central marker, no all-pixel/
tear-free/full framebuffer acceptance. Reset shell/SciCalc14/desktop PASS.
Recovery `../maia-rtos-recovery/native-dpi-center-20261115/`, original digests intact.
No flash/SD/parser/panel timing/rotation changes. Next primary-source audit of native
PSRAM initialization/cache/DMA ownership and full-framebuffer memory budget.
Native production streaming/desktop and rotation-input acceptance still pending;
installed desktop original runtime. WT no-ACK/threshold413/counter-doc/legacy
solid auto-ARLEN footprint limitations remain; new spatial path constrains length.

## LCD/touch priority; native HEX PSRAM state observed — 2026-11-15

Owner prioritizes finishing native LCD+touch, remaining drivers, then MaiaScript.
See [delivery plan](LCD_TOUCH_NATIVE.md) and [PSRAM audit](../audits/native-psram-state.md). Correct earlier Octal shorthand:
working P4 SDK config/actual boot use HEX/X16/AP vendor0D/gen4/density7/32MB;
not S3 Octal driver. SDK startup reference: MPLL, VO2, dedicatedMSPI2/3, mode regs,
DQS/DLL/tuning, cache/MMU; these dependencies must be ported, not linked.
New boot-only read-only snapshot/isolated probe, nine roots/27 controller words,
conservative gated policy and fresh RCC comparison before every read; no array/
cache/MMU/power/clock writes. Host47/47+sanitizers+C++98/four runtime mutants/SDK
36-address oracle/invalid profile PASS. Actual result0/54samples stable: RCC
E6DFB7FF/D00C/38000040/norst0, VO2 40200180/76800000, bothCMD0/CLK30103;
MSPI0 AXI_REQ_EN0/read-write cmd0. Access is NOT memory readiness/init proof.
SHA a059fe491269d2cec18e4402a2edb45f441dcf32dd30f16d47744c7929bc29e1,
RAM4FF00000/6608bytes/entry4FF00100. Reset shell/SciCalc14/desktop PASS;
no flash/SD/parser/display changes. Recovery
`../maia-rtos-recovery/native-psram-state-20261115/`. Next bounded HEX MR identity/
restore, native power/clock/mode/MMU/cache and owned memory before framebuffer.
Touch coordinates/events/runtime bus ownership still pending. Native full desktop
not integrated; preserved complete installed firmware still original runtime.

## Native HEX mode-register identity and restore — 2026-11-15

Read [identity audit](../audits/native-psram-identity.md). New boot-only direct PSRAM command-port read, no SPI
ROM helpers (reference helper has unbounded wait/mutable callback). Source facts
SDKf5c3654 HEX s_get_psram_mode_reg + P4 registers. Inherited powered/timed device,
AXI off; temp OPI_DTR/4040/cmd16/address32/dummy12/CS1/receive poison, bounded poll,
guarded ten-word restore; no device mode/array/clock/LDO/MMU write. Failure sticky,
no active cleanup/abort/retry. Actual one RAM run: MR0/1=8D30, MR2/3=A0DF, MR4=20,
MR8=4B twice; vendorD/density7/32MiB/device3(gen4)/KGD6/X16 confirmed. Every read
stage5/result0/setup+restore3FF/polls1/busy0/idle/restored1; busy edge not sampled,
response proof is poison replacement and repeatable device fields. Roots/27 words
unchanged after all eight reads, runner PASS. Host48/48 + ASan/UBSan/C++98/six
runtime mutants/invalid profile/SDK mask oracle PASS. SHA
95cec29d4b5fbd6ff0b635c2bf420a9eafc37be8e3c6509ae0f0c3421f8d8471,
RAM10448bytes/entry4FF00100. Reset shell/SciCalc14/desktop PASS, no flash/SD writes.
Recovery `../maia-rtos-recovery/native-psram-identity-20261115/`. No native cold
PSRAM/array/capability/mapping/framebuffer claim. Next checked native power/clock/
mode startup + owned memory, then cache/MMU/display. LCD+touch remain priority,
other drivers then MaiaScript; delivery checklist updated without claiming cold init.

## Native warm PSRAM clock qualification and VO2 guard fix — 2026-11-15

Read [clock audit](../audits/native-psram-clock.md). Boot/core0/IRQs-off private native start/stop switches
warm HEX from MPLL/core-div1/bus-div4 to XTAL/core-div1/bus-div2, nominal20MHz
with ROM-reported40MHz XTAL. Core gated before mux; no controller accesses while
gated. Idle/AXIoff/calibrated1900-field VO2 preconditions, checked writes/roots/
controller ownership, sticky failures; exact stop restores original profile.
VO2 old identity guard corrected: XPDbit8, ownerbit7. Earlier actual40200180
had both so old observed responses remain valid. Host49/49+ASan/UBSan/C++98,
six clock mutants+old-mode-bit7 mutant/invalid profile/SDK oracle PASS. First
capture lost initial line: strict FAIL preserved, no reconstruction. Final adds
bounded host-readyC; actual eight mode reads8D30/A0DF/20/4B twice, start/stop
stage5/mask1F and nine roots/27 words restored; runner PASS. SHA
561588ba9a1301294c8c252f2c218224bea52f88fd1462ec1dc3daa5940114e2,
RAM13152bytes/entry4FF00100. Reset shell/SciCalc14/desktop PASS; no flash/SD/parser
writes. Recovery `../maia-rtos-recovery/native-psram-clock-20261115/`. No physical
frequency/voltage/clock-edge measurement or cold-init/array/MMU/framebuffer
claim. Next native VO2/mode setup and reserved array test, then cache/full LCD.
Touch events/runtime bus ownership still pending; installed full desktop original.

## Ported PSRAM mode + actual array data verified — 2026-11-15

[Array audit](../audits/native-psram-array.md) groups native port of SDKf5c3654 DQS FIFO read/write algorithm
(Apache-2.0), checked mode switch, data patterns and exact data/mode/clock restore.
Boot/core0/IRQs-off/all internal buffers/exclusive PSRAM. Initial raw-X16 offsets
failed random data2060/4096; samples0..127 match expected1024..1151. Primary AP
Memory rev1.2 p11 CA10 ignored/X16 word addresses explains alias. No guessed
address remap. Adapt SDK8-line mode: exactMR8 4B->0B readback, byte aperture with
privateclaim, then checked0B->4B. Raw X16 byte transfers are now refused.
Final one RAM upload SHA830fd7d2a29e102a7a2a522aa9ad340851b4ecddf94f74a5e8288f15d56d32a3,
18320bytes/entry4FF00100. Eleven4KiB transfers/704chunks/45056bytes, four exact
patterns4096/4096 (76EFDDC5/34E76DC5/0F3A4DC5/E4B8E234), saved bytes hashD3D6E224
restored/all4096matched/guards kept. Mode andD00C/30103/30103 clocks restored,
nine roots/27 words equal original; runnerPASS, oldcapturesFAIL retained. Host50/50,
ASan+UBSan/C++98/six runtime mutants/invalid profile/SDK oraclePASS. Reset shell/
SciCalc14/desktopPASS; original hashes/no flash/SD/parser writes. Recovery
`../maia-rtos-recovery/native-psram-array-20261115/`, two refusal sibling dirs.
No cold startup/ordinary mapped PSRAM/full capacity/full framebuffer/continuous
LCD or touch claim. Next adapt controller AXI/cache/MMU startup/mapping, then
framebuffer. Owner requests grouping functional deliveries/autonomous electronic
tests, existing licensed driver reuse; AGENTS and four-gate delivery plan updated.
Installed complete desktop remains original runtime; native port imports none.

## AXI exclusion guard corrected before mapped-memory activation — 2026-11-15

[AXI correction](../audits/native-psram-axi.md): previous CTRL1+0Cbit0 guard was clock mode, not AXI.
Use actual CACHE_FCTRL5008E03C AXI_REQ_EN0/CLOSE_AXI_INF_EN1 shared helper in mode,
clock and array initial/ownership checks; never read while core gated. Caller
boot exclusivity remains required. Historical AXI-off inference corrected, actual
previous data observations retained. Host50/50/sanitizers/mutants/C++98/SDKoffset
oraclePASS; actual whole X8 array/data/mode/clock test repeated with AXI C0000000
before/after, four exact4KiB patterns, full recovery and runnerPASS. SHA
4d96fb7a71a3b79ba2f20e5d4c1dc8a055a1212da255545d03edd71b9a5567a2,
18592bytes/entry4FF00100. Reset shell/SciCalc14/desktopPASS. No flash/SD/parser
writes or mapped-memory activation. Recovery `../maia-rtos-recovery/native-psram-axi-20261115/`.
Next controller/cache/MMU mapping port, then framebuffer; no cold/LCD/touch completion.

## Native 2MiB mapped PSRAM data qualified — 2026-11-15

[Window audit](../audits/native-psram-window.md): native Apache-2.0 port of SDK HEX
controller/pad/MMU/cache algorithms, no foreign runtime/header. Actual one RAM
upload23c512c93d72fc111a417829246eb73b3b6dc511e733bbaeebbcf10a408253cf/24304bytes:
CPU524288/524288words over2MiB, hash2E4E04FE; post-sync direct SPI4096/4096bytes
confirms device visibility. MMU32entries/pads/controller/AXI/mode4B/clocks restored;
first4KiB saved data restored, rest2MiB intentionally volatile/overwritten. Global
ROM cache/flash-MMU bootstrap owned until reset, not undone by window stop. Host
51/51, ASan/UBSan/six rejected mutants/C++98/SDK oracle/invalid profile/runtime gate
PASS; additional host-only negatives rerun after target, native image unchanged.
Reset shell/SciCalc14/desktopPASS; no flash/SD/parser writes. Recovery
`../maia-rtos-recovery/native-psram-window-20261115/`. CPU window fits one full
RGB565frame; DMA/throughput/cold startup/production concurrency still unqualified.
Continue clock/burst/DMA/full-image integration autonomously, then continuous LCD/
touch. Existing full installed desktop still original runtime.

## Native PSRAM DMA burst/address qualification — 2026-11-15

[DMA audit](../audits/native-psram-dma.md): one native RAM upload686f26dead6cee2033c6cec32250670e27d837a86e2fd273275ee89acd6d84dd/27152bytes.
All9x4096bytes PSRAM->SRAM matched, guards kept, offsets0/65408(pagecross)/1FF000,
explicit source1/4/16items andARLENfields0/3/15,64bit. All raw511/naturalstop/
errors0,12configwords restored and MIPI/otherchannels preserved. Nominal20MHz
4KiB samples1179/373/170–171us; insufficientfor currentDPIrate, not sustained
displaythroughput. Full2MiB CPU/cache/direct/mode/clock recovery alsoPASS. Host
52/52/sanitizers/SDKoracle/C++98/three rejected mutants/invalidprofile/runtime
gatePASS. Reset shell/SciCalc14/desktopPASS, no flash/SD/parserwrites. Recovery
`../maia-rtos-recovery/native-psram-dma-20261115/`. Next operationalclock adaptation
(bounded MPLLdivisorread/no sharedPLL retuning), data/DMA then fullframePSRAM->DSI.
No nativePLLchange/fullLCD/touch/coldstartup acceptance yet. Existing fullfirmware
remains originalruntime; native tests use none. Continue internally under existing
authorization; coordinate owner presence only for prepared visual/touch checks.

## Native200MHz PSRAM/DMA qualification — 2026-11-16

[Operational-clock audit](../audits/native-psram-fast.md): native bounded analogMPLLread
(two99 =>400MHz, powered/calibrated), no PLLretuning; checked200MHz clock switch,
20pad/DQS setup, **both MSPI0+180/+190 DLLbit5** enable/exactrestore. Four-phase
readonly DMA search all9/9each, maskF/selected0; no zero-pass fallback. Final
CPU524288words/2MiB hash2E4E04FE,9x4KiB DMA allbytes/guards correct, directSPI
read/saved4KiB/mode4B/clocks/AXI/MMU/config restored.45 DMAcopies/184320matchedbytes
including training;16-item4KiB samples22–23us vs20MHz170–171us, not sustainedDSI
rate guarantee. Final85168103973a0ca39828eb8ee955289e11f1b8ece72139ab72370fab3c845af1/33056bytes.
Four refused earlier images/captures retained (DQS order, burstbits, samples, phase
mask0 beforeDLL enable), no acceptance relaxation. Host53/53/sanitizers/six rejected
mutants/SDKoracle/C++98/runtimegatePASS. Desktop shell/SciCalc14/exit0 restored.
No flash/SD/parser/rotation changes. Recovery
`../maia-rtos-recovery/native-psram-fast200-20261116/`.100MHz HOST-only. Stillwarm
boot-exclusive, not coldstartup/31point delayline/temp-margin/SMP acceptance.
Next fullframePSRAM->DSI grouped integration/visual test; then continuousLCD/touch.
Continue under existing authorization; no per-substep approval stops. Installed
full desktop still originalruntime; native tests import none.

## Full native framebuffer visible and accepted — 2026-11-16

[Framebuffer audit](../audits/native-framebuffer.md): actual1843200B RGB565 source
4B800080 in native200MHz PSRAM, single-block sourceincrement/MSIZE16/ARLEN15 to
DSI, native720x1280/logical1280x720/software90. Private source claim/cache handoff
freezes window lifecycle until actualDMA drain/MMU/guard release; per-frameSAR
rewind only while idle. No LLP or relaxed legacy solid-source bounds.
Same image68a8dfc0f8d9c9cb8bd3c2cdad7635a2d2c610bb703c07d129bc65c587e97d8f/79488bytes:
electronic4frames/65383us; fresh-ready visual1000frames/230400000items/999rearms/
15282355us, raw383FF/status12/allerrors0/recovered1/released1. HashE3D71BEF/all
guards unchanged; preliminary9x4KiB PSRAMDMA exact; memory/mode4B/clocks/MMU
restored, fourRDDPM9C replies. Owner confirmed complete/stable/correct orientation
when asked about numbered red/green/blue/yellow corners. Qualitative acceptance,
not all-pixel/tear-free proof. Host56/56/ASan/UBSan/four mutants/oracle/C++98/runtime
gatePASS. Electronic initialhostFAIL was wrongwholeDPI0 predicate; correct exact
inherited19D0/enable0 comparison passes sameUART; original log retained. Visual
strictPASS. Reset shell/SciCalc14/desktop verified both runs; no flash/SD/parser
changes. Recovery `../maia-rtos-recovery/native-framebuffer-20261116/`.
Next continuous nativeLCD tasks/IRQ/source lifetime/updates/recovery, then touch.
Still boot-exclusive finite diagnostic, not native complete desktop/coldstartup/
SMP or31point margin acceptance. Full installed firmware remains originalruntime;
new native code imports none. Keep grouped work/autonomous electronic tests.

## Scheduled native LCD with actual IRQs — 2026-11-16

[Service audit](../audits/native-lcd-stream.md): native owner task, split-phase framebuffer DMA, actual
DW-GDMA source24 routing to proven-free CLIC18 (17 already enabled, preserved),
native notification and100tick deadline. Target99frames/99realIRQs/99posts,
1517timerIRQs,1414heartbeat iterations and CPU-load progress; active-stop request,
actual drain/source release/exact route restoration, no timeout/spurious/error.
Hash/guards/9mirrorDMAcopies/restored4KiB/mode/clocks/MMU/fourRDDPM9C PASS.
Native/strict host PASS; no new visual claim. Image
f62e30839ff5ac3b7ba7ad63d5f0927fd9c4569701b1414ead30659db00e8c38/87008bytes.
Host60/60, ASan/UBSan/four rejected mutants/C++98/pinned SDK oracle/runtimegate
PASS. Two earlier safe init refusals preserved; fixed-line busy condition resolved
by bounded free-line selection. Recovery `../maia-rtos-recovery/native-lcd-stream-20261116/`.
Reset shell/SciCalc14/exit0/desktop verified. No flash/SD/parser changes.
HARDWARE-TESTED, PARTIAL: single warm session/core0/immutable buffer; no native
complete desktop, image updates/reinitialization/coldboot/SMP/margin acceptance.
Next grouped unit: safe source publication/reuse and LCD lifecycle, then native
touch contacts/rotation/service integration. Continue under existing authorization;
coordinate presence only for actual visual/touch observations. Installed complete
firmware remains original runtime; new native code imports none.

## Native LCD updates and warm restart — 2026-11-16

[Lifecycle audit](../audits/native-lcd-lifecycle.md): one queued native owner callback edits only after actual
DMA completion; CPU edit lease freezes PSRAM lifecycle, cache publication returns
ownership to DMA. Clean owner-thread restart reclaims the warm source/IRQ route;
failed or uncertain state cannot restart. No foreign OS/runtime/headers.
Electronic three sessions x24frames, six complete image edits,72realIRQs;
visual three sessions x253frames,759realIRQs, six updates, correct hashes, all
active stops/releases/restoration with no errors/timeouts. Owner confirmed the
whole picture alternated correctly and stayed stable. Final harness makes the
duplicate-request test atomic; final electronic regression again72frames/IRQs
PASS. Final image3fda28fd6228a4bc40cd58527aa48dde2c66d4489c8ba6307c8089b16cd06f48/89792bytes.
Host61/61, four ASan/UBSan targets, six rejected mutants/C++98/runtimegate/pinned
SDK oracle PASS. Shell/SciCalc14/moduleexit0/desktop verified after both final resets.
Recovery `../maia-rtos-recovery/native-lcd-lifecycle-20261116/`. No flash/SD/parser
changes. HARDWARE-TESTED, LIMITED: warm fixed single-buffer/core0, trusted bounded
callbacks and exclusive PSRAM/cache clients; no cold startup/SMP/margin, brightness
lifecycle, target fault injection or complete native desktop acceptance.
Next grouped delivery: native touch contacts/coordinate rotation/input service
with LCD updates; preserve full installed original-runtime fallback. Continue
under existing authorization; coordinate presence only for actual touch/visual
observations, no per-substep approval stops.

## Native touch service / electronic checkpoint — 2026-11-16

[Touch input audit](../audits/native-touch-input.md): adapted Apache-2.0 Espressif ST7123 read/capability
algorithms to bounded native block-I2C/controller/service, with complete10slot
reads,90degree checked coordinates and latest-state snapshots. No-data holds
contact state; complete empty report releases; errors/stale polling invalidate,
never synthesize UP. Native core0 bus try-ownership and short serialized timer
capture allow task use with wire phases interruptible. No foreign runtime/headers.
Host64/64, three ASan/UBSan targets, six rejected mutants and C++98 headers PASS.
Native runtime gate project+libgcc PASS. Two electronic RAM runs: firmware1,
maxX720/maxY1280/10slots,295polls/101LCDframes with realIRQ, errors/contentions0,
no fresh coordinate reports. Final image2482663ab6063b6ee76e97477858e3f5795efda43139c34194d15ae180f77fda/93040bytes.
Source hash/guards/memory/clocks/route/panel replies restored. Final desktop
shell/SciCalc14/exit0 verification in recovery evidence. No flash/SD/parser writes.
HOST-TESTED contacts, HARDWARE-TESTED LIMITED idle transport; physical contacts,
70byte reports under preemption, visible cursor/rotation/release are NOT RUN.
Prepared20second corner1/2/3/4 tap+lift and center drag test awaits owner readiness
(requested asynchronously); do not launch it until fresh confirmation. Next action
is this grouped hardware acceptance, fixing native layers if it exposes defects.
Recovery `../maia-rtos-recovery/native-touch-input-electronic-20261116/`. Full
installed desktop remains original-runtime fallback. Not production touch/cold
startup/SMP/multi-finger/lossless-queue/full native desktop acceptance.

## Physical native touch + LCD accepted — 2026-11-16

[Physical checkpoint](../audits/native-touch-input.md): fresh owner readiness, same93040byte final image
2482663ab6063b6ee76e97477858e3f5795efda43139c34194d15ae180f77fda loaded in RAM.
1148polls/826fresh70byte reports/1974native bus transactions, ten downs/ten ups/
96moves and corner maskF, errors/contentions0.827LCD publications/presentations,
1211actualDMAframes/IRQs/posts, zero timeout/spurious/controller errors.20048timer
IRQs/17044heartbeats/CPU-load progress; all four other tasks exit before teardown.
Active stop/source release/exactroute/hash/guards/memory/panelRDDPM restoration
PASS. Native/strict host PASS. Owner confirms cross follows correct finger position
and disappears on release, with no image fault. Host64/64 regressions PASS with
actual physical capture and negative gesture/IRQ/presentation checks. No native
implementation/image changes, flash/SD/parser writes; original desktop restored.
Recovery `../maia-rtos-recovery/native-touch-lcd-physical-20261116/`.
This supersedes prior pending physical readiness/contact/70byte-under-preemption
notes. HARDWARE-TESTED LIMITED warm single-finger native LCD/touch interaction;
not coldboot/multi-finger/SMP/lossless-event/physical-fault/shared-extra-client or
complete native desktop acceptance. Keep original-runtime full firmware fallback.
Next grouped work: consolidate native board/display/input lifecycle and remaining
production gates, then remaining drivers; preserve MaiaScript EBNF/MaiaCC rule.

## Native input/bus lifecycle — 2026-11-16

[Lifecycle audit](../audits/native-input-lifecycle.md): owner-thread touch stop/revalidate/restart, no stale
contact reuse; task bus close/reopen with closed reads refused and no pin/clock
reset. Three cycles plus final shutdown pass alongside102nativeDMAframes/IRQs;
296polls/302bus calls/errors0, three poll/closed-read/sequence guards,1569timerIRQs/
1460heartbeats and CPU-load progress. Source/route/memory/panel replies restored,
original shell/SciCalc14/exit0/desktop verified. Host65/65, three ASan/UBSan targets,
eight rejected mutants/C++98/native-runtime-gate PASS. Initial host FAIL preserved:
intentional invalid snapshot sentinel was treated as contact corruption; corrected
explicit lifecycle evaluator passes the same UART prefix. Native image unchanged
7204dc1076dfc79d12326a95a1b6f6108a8f943e9173084ec4037a2d7011bf90/94896bytes.
Recovery `../maia-rtos-recovery/native-input-lifecycle-20261116/`. No flash/SD/parser
writes. HARDWARE-TESTED LIMITED warm lifecycle, not cold startup/power-down/SMP or
full native desktop. Prior physical touch acceptance retained, not rerun here.
Audit inventories remaining inherited prerequisites: PSRAM supply/device modes,
already calibrated MPLL, fixed boot cache/reservation. Next grouped work is native
PSRAM device/clock cold-start ownership/acceptance from pinned vendor algorithms;
never relax warm guards to simulate cold support. Keep full fallback and continue
under existing authorization, coordinating presence only for actual visual tests.

## Native MPLL power/configuration/calibration — 2026-11-16

[Initializer audit](../audits/native-mpll-initialize.md): native boot-only MPLL circuit power cycle, DHREF/RSTB/
divider400MHz programming and observed END0->END1, with PSRAM on genuineXTAL20;
LDO2 supply untouched, other MPLL/F25/F50 clients quiescent. No foreign runtime.
Actual before/after3=50->70,div99,cal34C, four analog writes/29polls/routing restored.
Then nativePSRAM200MHz/fullimage/guards/9byte-exact4KiB DMA cases/102LCDframes and
three input lifecycle cycles pass, errors0. Final desktop shell/SciCalc14/exit0
restored. Two prior END0 refusals retained; no acceptance guard relaxed. Image
f27fcb3fae416f5ca07a0b68625047a260e128a017c4d78cd34d159b47973da1/97776bytes.
Host67/67, initializer ASan+UBSan/five rejected mutants/C++98/official-header oracle/
six pinned sources/runtimegate PASS. MPLL-only diagnostic task stacks6KiB fit
unchanged low-SRAM boundary; context/stack checks pass. Recovery
`../maia-rtos-recovery/native-mpll-initialize-20261116/`. No flash/SD/parser writes.
HARDWARE-TESTED LIMITED powered-domain native PLL startup; PSRAM supply/device
modes still inherited, not cold board/full32MiB/margins/SMP/native desktop acceptance.
Next grouped work: native MR0/MR4/MR8 timing/interface configuration and supply/
device startup from primary documented defaults; preserve warm guards/fallback.

## Native PSRAM MR profile programming — 2026-11-16

[Profile audit](../audits/native-psram-profile.md): native MR0 read/fixed latency/drive,MR4 write latency and
MR8 burst/interface programming from pinned Apache2 vendor algorithm, with
original-profile ownership/readback/restoration. Native command/FIFO guards,
no SDK/FreeRTOS runtime. Unexpected restore mode changes now poison the shared
profile so legacy array/X8 entry cannot bypass refusal. Host differing-mode entry
and exact restore/ignored writes/timeout/unowned fields/clock guards pass.
Actual powered device already8D30/20/4B; three programming attempts/readback and
restore pass, not a cold/default-mode transition claim. Subsequent MPLL native
power/cal400,PSRAM200/fullimage/guards/9x4KiBDMA/102LCDframes/three input restart
cycles/errors0 and desktop shell/SciCalc14/exit0 pass. Final image
57927950a3d722e5295b3a47a6e8b682a490cba8adb2f61612f872359515f1c3/99168bytes.
Host68/68, array/profile ASan+UBSan/five rejected mutants/C++98/vendor struct oracle/
pinned source/runtimegate PASS. Both initial and final successful image evidence
preserved in `../maia-rtos-recovery/native-psram-profile-20261116/`. No flash/SD/
parser changes, no new visual claim. HARDWARE-TESTED LIMITED powered mode setup;
actual differing-default startup HOST-only. Next native LDO2 supply/reset/power-on
entry, then exercise mode initialization against actual defaults. Keep private
warm guards/fallback; no cold/full32MiB/margins/SMP/native desktop acceptance yet.

## Native PSRAM VO2 supply configuration — 2026-11-16

[Supply audit](../audits/native-psram-supply.md): native fixed1900mV/eFuse configuration,
strict no-write adoption or gated-consumer disabled-supply programming, unrelated
fields preserved, checked writes and sticky failure. Apache2 vendor adaptation;
no foreign runtime/headers. Actual powered Tab5 adoption result0/stage4/trim7/5/
writes0/control40200180/analog76800000 unchanged. Native MPLL cal400,mode profile,
PSRAM200/fullimage/guards/9x4KiBDMA/102LCDframes/threeinputcycles/errors0 pass;
295polls/301bus calls/1569timerIRQs. Saved4KiB/modes/clocks/MMU/IRQ routes and
panel replies restored; shell/SciCalc14/exit0/desktop verified after RAM reset.
Image119322f50a2443dfcadabc0fdc2e2e36c078683c6565f27c01ffbdd715db1138/100304bytes.
Host70/70, actual supply ASan+UBSan/five non-equivalent mutants rejected/original
PMU-eFuse field oracle/C++98/source pins/native runtime gate PASS. Initial harness
redundant-mutant expectation failure preserved; whole-snapshot check still caught
the omitted immediate readback. Recovery `../maia-rtos-recovery/native-psram-supply-20261116/`.
No flash/SD/parser changes. HARDWARE-TESTED LIMITED powered adoption; off-supply
programming HOST-only, no rail measurement/discharge/settling/device default/cold
claim. Next grouped work: primary documented supply/device power-on timing and
native PSRAM+MPLL consumer sequencing, then physical different/default-mode entry.
Keep warm guards/fallback; full32MiB/margins/SMP/native desktop still open.

## Native PSRAM/MPLL consumer parking — 2026-11-16

[Park audit](../audits/native-psram-park.md): native boot XTAL20 -> core/modulePLL/
MPLL output/circuit off -> circuit/output/modulePLL/XTAL core restore, checked
writes/unchanged fields/no gated reads/sticky failure. VO2 stays on, not cold.
Actual PERI C00C->000C->C00C,RF09000000->08000000->09000000,output1FFCFFFD->
0FFCFFFD->1FFCFFFD. Then nativeMPLL END0->1/CAL024C->034C/400MHz,mode profile,
PSRAM200/fullimage/guards/9x4KiBDMA/102LCDframes/threeinputcycles/errors0 pass.
295polls/301bus calls/1569timerIRQs, saved4KiB/modes/clocks/MMU/IRQ routes restored.
Desktop shell/SciCalc14/exit0/desktop PASS. Image
5c5e46958229260d1d01ab316a02e23417fde572cde3ded616e683f1653023b2/101072bytes.
Host72/72, actualclock/park ASan+UBSan -Os/five rejected mutants/C++98/sixsource
pins/runtimegate PASS. Initial416byte SRAM overflow refused; clock -Os then exposed
unresolved compiler-generated memcpy, also link-refused. Bounded explicit snapshot
copy removes libc dependency. No boundary/stack reduction:160KiB window/16KiB boot
stack, final BSS4FF23FA0/stacktop4FF27FA0, task guards pass. Failed preparations
contain explicitly marked stale previous-image files NOT RUN; successful final
image/source/capture in `../maia-rtos-recovery/native-psram-park-20261116/`.
No flash/SD/parser changes/new visual claims. New PDF research: owner's Tab5.pdf
is17-page product export; linked official5-sheet schematic visually verifies
LDO2->U1 pins72/59/67 and capacitors, separate flashLDO1. Primary SDK documents
consumer-stop order but no fitted AP32MiB HEX/SIP discharge/initialization timing.
Do not substitute generic Octal timing or claim MPLL power-off resets PSRAM.
Next: establish applicable fitted-memory supply/reset specification, then qualify
native device entry after genuine supply loss. Power-off API remains absent;
full32MiB/margins/SMP/shared consumers/native desktop still open. Keep fallback.

## PSRAM boot-state observation preparation — 2026-11-16

[Boot observation](../audits/native-psram-boot-state.md): new runner
--psram-boot-state requests installed esptool4.12 before=no_reset, only for isolated
existing state-only RAM image. Refuses active/fatal profiles before serial; normal
profiles retain default_reset. Logs operator power provenance, never infers cold
initialization from snapshots. Native firmware/MMIO unchanged; early startup still
configures UART/watchdogs/USB reset control, so PSRAM-read-only, not wholeSoC-readonly.
Host73/73, actual CLI fake-image/connection isolation, installed esptool no-reset
success/five failedsync attempts/default-reset behavior and image bounds PASS.
Fresh state-only image6704bytes/SHAaff430345ea1781429ed2541c7dcd40bd3a735dcbe9270a0d62cbc1f2f3960b7,
native runtimegate project+libgcc PASS. Prepared image/ELFs/map/config/source in
`../maia-rtos-recovery/native-psram-boot-state-prepared-20261116/`, checks/docs in
`../maia-rtos-recovery/native-psram-boot-state-20261116/`.
HOST-TESTED/PREPARED, actual no-reset target capture NOT RUN. Operator physical
power-off/all sources removed/atleast5s/directROMdownload preparation requested
via pending async question; wait for explicit report before serial access. Do not
claim absence of visible desktop proves no application code ran. Manufacturer5s
note concerns board/IMU; not assumed PSRAM discharge timing. Applicable AP HEX/SIP
power timing remains unresolved; no guessed VO2 pulse or supply-off API added.
Next after operator confirmation: exact preserved image/hash, --psram-boot-state
--capture-seconds8, retain raw/provenance, compare with warm roots, reset and verify
fallback shell/SciCalc14/exit0/desktop. Never silently fall back to resetting
connection on sync failure. No flash/SD/parser/EBNF changes, no cold acceptance.

## Armed PSRAM ROM-entry snapshot — 2026-11-16

[Actual boot snapshot and transport audit](../audits/native-psram-boot-armed.md)
supersedes pending manual preparation. First no_reset connection failed/noimage;
operator then reported Desktop visível. Failure preserved. New host connector
opens preconfigured DTR/RTS false on macOS, passes open object to pinned esptool
4.12, optional held wait0..120s/no reset callback/re-enumeration/cleanup. Other
modes default_reset unchanged. This removes implicit asserted opens, not measured
proof of physical reset-free pins or cause of first failure. Source/CLI tests pass.
Armed capture synced promptly, no new manual request; no button action pending.
Actual unchanged state-only6704byte imageaff430345ea1781429ed2541c7dcd40bd3a735dcbe9270a0d62cbc1f2f3960b7,
result0/54samples/nine roots+27controllerwords before=after. SYS E6DF97AF,
PERI C00C(XTAL/core+PLL gates enabled),reset100,VO2control40200000(XPD8/SW7clear),
analogA0000000(DREF10/MUL0/VDET0),bothbusdiv30103; eFuse7B054313/3A184DEA/error0
stilldedicated1900mVtrim7/5. UnlikewarmVO240200180/76800000/MPLLmuxD00C, this is
an unconfigured/off-enable supply entry. No mode/array reads, no memory-ready or
factory/default/cold physical-provenance claim. Earlier desktop visibility and
unconfirmed power history prevent uninterrupted power-off-to-ROM assertion.
Native warmguards shouldcontinue refusing this entry. Next grouped work: native
boot entry from these observed roots, gateconsumers/configure+enable VO2 with
nonzero eFuse pair, establish MPLL prerequisites, then modes/data/DMA/LCD. No
invented rail power-off pulse/timing; warm and boot ownership remain distinct.
Host75/75 plus unchanged native runtimegate PASS. Final runnerreset immediate
shell check blank; explicit esptool standard reset/chip_id/hard_reset restored
fallbackboot. Short postbootchecks missed startup; retained all failures. Same
idleport held10s before existingcheck passes shell/SciCalc14/exit0/desktop.
Transient approval BadRequest blocked first recovery invocation, same reviewed
retry succeeded; no bypass. No commanded flash/SD writes; fallback existingboot
SDinitialization stillruns. Nativecode/parser/EBNF unchanged. All source/image/
rawsuccess+failedcaptures/recovery/GIF/pins/checks in
`../maia-rtos-recovery/native-psram-boot-armed-20261116/`.
HARDWARE-TESTED LIMITED observation only. Cold init/full32MiB/margins/SMP/shared
clients/native desktop stillopen. Keepfallback. No repeated button request needed.

## Native supply-to-mode boot implementation — 2026-11-16

Owner explicitly says proceed with manual power/button test NOT RUN; no further
chat-timed preparation requests. Recorded in AGENTS.md. [Native boot audit](../audits/native-psram-native-boot.md).
HOST-TESTED/NATIVE-BUILT, new target boot and full pipeline NOT RUN. No device
connection/reset/flash/SD operation this unit. Prior raw snapshots/physical warm
LCD/touch evidence retained with their original limits, not promoted to new boot.
New maia_esp32p4_psram_boot_initialize200 composes private boot entry from VO2-off,
XTAL/bus-div4/idle closed AXI/accessibleregisters: gateconsumers, dedicatedtrim
VO2enable, LPPERI analogmaster clock/heldreset release/160Msource, MPLLpower/output,
XTAL20div2, native400Mcal END0->1 then MR0/MR4/MR8 target200/X16. No poweroffpulse,
resetprotocol/settlingguess; readabledevice/widerROMCPU/cache prerequisites remain.
Boot-lifetime owner cannot restore off-supply/oldfirmwareclocks. Warm guards retained;
only privateboot permits unknowninitialMPLLdivider/readableX8mode. Fastclockblocked
beforecal;park invalidatesready. Stickyphasefailure/no blindrollback/checkedroots.
Frame diagnostic/CMake/runner explicit --psram-native-boot path feeds existing
PSRAM200/window/DMA/LCD/input pipeline; native baseline retained onteardown,
no originaloff-state restoration. Host future-envelope fixture explicitlysynthetic,
not UART evidence. SourceAPI, integrationcode, header/contracts, tests, docs updated.
Host77/77 PASS, actualboot/array-profile/MPLL ASan+UBSan -Os, six rejectedmutants,
C++98/sevenprimarysourcepins PASS. Both warm/nativeboot builds runtimegate project+
libgcc PASS. Initial -Os libc-helper linkrefusal fixed via statictables/bounded
transfer clear; warm32byte overflow fixed withMPLL -Os, no stack/boundary change.
New coldprofile99840byte image45a2589c643d1118519a0330e4c8ad26ed04aa654524bf634571bf2e61bd187c;
BSS4FF23B20/stacktop4FF27B20, warm4FF23E20/4FF27E20. validate-onlybounds PASS,
no serialaccess. Full artifacts/source/config/commands/checks/failures in
`../maia-rtos-recovery/native-psram-native-boot-20261116/`.
Manualtiming and newboot physicalacceptance explicitly NOT RUN; proceed remaining
memory/driver work without treating that as developmentblocker. Electricalmargins,
32MiBcapacity, fullyindependent CPU/cachestartup,SMP/sharedclients/native desktop
and otherdrivers remainopen. No shell/MaiaScript parser/EBNF/MaiaCC changes.

## F0 kickoff — 2026-11-17

- Corrected the `maia_m0.elf` include directories to match its actual shell,
  assembler, MaiaScript and utility-header dependencies. This removes the
  earlier missing `riscv_asm.hpp` diagnostic, but the native target still does
  not build: the shell pulls hosted C++ library facilities (`<vector>`,
  `<memory>`/`std::shared_ptr`) into a `-ffreestanding` target. This is a
  runtime integration blocker; no stubs or compatibility layer were added.
- Removed the linker comment's unsupported claim that the current RAM/probe
  layout is compatible with second-stage flash boot. The existing app descriptor
  is placed in internal memory; it is not evidence of the required DROM image
  layout.
- Added C-only `maia_f0_flash_probe.elf` and optional
  `maia_f0_flash_probe_image` target, separate from the shell/runtime. The
  cross-build and esptool 4.12.0 image packaging pass. The candidate has two
  shared D/I mappings, a DROM descriptor at `0x40010020`, entry at
  `0x40000020`, a reserved address-zero `PADDING` record, and valid checksum
  and validation hash. Its validator checks descriptor, entry, mapping count
  and 64 KiB MMU alignment; malformed descriptor, extra-mapping and alignment
  cases fail.
- Pinned ESP-IDF v5.4.2 source confirms the P4 shared D/I window and loader
  requirement for exactly two mapped segments; address-zero padding is
  reserved and not loaded. This plus the offline image check is not a device
  boot test. Project partition configuration is known, but physical flash
  contents and actual bootloader compatibility remain open under UQ-016.
- The probe ELF has no undefined symbols and no FreeRTOS/ESP-IDF runtime
  symbols. This audit applies only to the minimal probe, not the full product.
- Validation: host configure/build/CTest 77/77 PASS. Native probe image target
  PASS. The full `maia_m0.elf` target still FAILS at hosted libstdc++ headers
  (`<vector>`, `<memory>`/`std::shared_ptr`) under `-ffreestanding`; no stubs
  or compatibility layer were added. Existing recovery image
  `maia_sched_probe_ram.bin` remains a RAM-load image, not a flash-app result.
- Device test (2026-11-17, `/dev/cu.usbmodem1301`, ESP32-P4 v1.3):
  `tests/hardware/run_f0_flash_probe.py` read back 3 MiB from `0x10000`; the
  2,093,696-byte app validated and is byte-identical to `bf044d3`
  `m5stack_tab5.bin`. The probe (66,192 bytes, SHA-256
  `c29163763556f391edd59277af7992c27d84b2db09cb1d47f1f90b5c8e3ca3a6`) was
  written with esptool verification. After RTS reset the installed v5.4.2
  bootloader listed the partition table (factory `0x10000`, 10 MiB), mapped
  segments `0x40010020` / `0x40000020`, skipped the padding block and the probe
  printed `MAIA_F0_FLASH_PROBE`. The backup was rewritten and verified; the
  restored firmware booted the desktop and `notes.mobj`, confirmed with
  `maia.py info`. Logs and backup are in
  `maia-rtos-recovery/native-f0-flash-probe-20261117-215248/`.
- Checklist: "native image started by the second-stage bootloader" and the
  verified write/restore procedure are PASS for the probe. Direct XIP of real
  product code, product linker, heaps, cold PSRAM, services and cold 10× boot
  remain open.
- Flash-mapped kernel/task probe (2026-11-17): built
  `maia_f0_kernel_psram_probe_image` (83,376 bytes; SHA-256
  `c37955c11762740940c7c4f046f783572d417c8eb40fd5984deea444bcdc3600`).
  ELF places scheduler, timer, task workers and entry in IROM `0x40000020`;
  descriptor/constants in DROM `0x40010020`; BSS/boot stack at
  `0x4ff00000..0x4ff08650`, within the bounded 160 KiB SRAM window. No undefined
  symbols or FreeRTOS/ESP-IDF runtime symbols.
- Device capture: `MAIA_F0_PSRAM_STATE result=0 roots_valid=1
  controller_samples=54`, then 250 real timer IRQs/ticks, 251 switches,
  both workers progressed, zero register/IRQ-stack errors and
  `MAIA_NATIVE_PREEMPT_PASS`. The snapshot is read-only controller observation,
  not PSRAM initialization, capacity verification or array test. Backup was
  verified/restored and `maia.py info` confirmed the desktop and notes app.
  Logs/backup: `maia-rtos-recovery/native-f0-kernel-psram-flash-20261117-220134/`.
- This closes the probe-level XIP/kernel/two-task/timer acceptance only.
  Product linker, data heaps, cold PSRAM startup and PSRAM memory use remain open.
- Multi-region heap groundwork (2026-11-18): `maia_mem_init` retains the
  existing reset/first-pool API; new `maia_mem_add_region` accepts up to four
  distinct, non-overlapping writable pools. Allocation scans each pool,
  coalescing/freeing stays within its owning region, and stats aggregate all
  regions. Added bounds/overflow/overlap/capacity and cross-region tests;
  host CTest 78/78, allocator ASan/UBSan and RV32 freestanding compile PASS.
  Allocator calls remain externally serialized. `boot_main.c` still registers
  only its 32 KiB SRAM pool; no PSRAM memory is added until cold init and the
  product CPU mapping are validated.
- Device heap test (2026-11-18): flash image `maia_f0_kernel_psram_probe_image`
  (85,008 bytes; SHA-256
  `72198537e34c141beb6c934686381d0ac5da3c9e6d0fa44d317a9ca79553e908`)
  initialized two disjoint SRAM pools at `0x4ff046c0..0x4ff056c0`, verified a
  64-byte-aligned allocation and a large allocation from pool 2, freed both and
  verified aggregate free bytes equal total bytes (`MAIA_F0_SRAM_HEAP_PASS`).
  Then PSRAM controller snapshot result0/54 samples and scheduler acceptance
  passed (250 IRQ/ticks, 251 switches). Original app restored; `maia.py info`
  confirmed desktop/notes. Evidence: `maia-rtos-recovery/native-f0-multiheap-flash-20261118-080619/`.
  This tests multiple SRAM pools only; PSRAM heap startup/registration remains
  pending.
- Flash-XIP linker baseline (2026-11-18): `boards/m5stack/tab5/flash_product.ld`
  now lays out IROM/DROM, the app descriptor and writable image state in the
  validated 160 KiB low-SRAM window. It exports the qualified 2 MiB PSRAM window
  as absolute runtime symbols only; no loadable section or heap is placed there.
  The scheduler/heap probe image is 80,080 bytes (SHA-256
  `3aa5179dd6a5c8e95daee485571353edf7cfa2f70bd10b6bd22ee34709deeff4`), with
  BSS ending and the 16 KiB stack beginning at `0x4ff056c0`, stack top
  `0x4ff096c0`, and PSRAM symbols `0x4b800000..0x4ba00000`.
  Structural image validation passed; device boot printed
  `MAIA_F0_SRAM_HEAP_PASS` and `MAIA_NATIVE_PREEMPT_PASS` (250 timer IRQs,
  251 switches). Original app restoration was verified by esptool and the
  captured boot reached the desktop/wallpaper. Evidence:
  `maia-rtos-recovery/native-f0-product-linker-20261118/flash-probe.bin`
  (exact image), with report, backup and boot captures in the same directory.
  This was a warm reset, not cold PSRAM initialization; the linker is a
  flash-XIP baseline, not the completed product layout.
- PSRAM startup closure placed in internal SRAM (2026-11-18):
  `flash_product.ld` collects the selected boot/clock/supply/MPLL/array/mode/
  snapshot/window modules and constants in `.maia_psram_iram` at `0x4ff00000`.
  Linker assertions pin the critical entry points, and `NOCROSSREFS_TO` rejects
  relocations to XIP text/rodata. Readelf confirms distinct RX and RW segments
  with no RWX mapping; disassembly shows the remaining indirect cache calls
  target ROM. The 79,600-byte image passed esptool structure validation and
  warm-reset device boot, including SRAM heap and scheduler markers; the
  original app was verified restored and the desktop/Notes app loaded.
  CTest 78/78 passed. Exact image and logs:
  `maia-rtos-recovery/native-f0-psram-iram-20261118-084629/`.
  This proves image loading, not execution of PSRAM startup, cold PSRAM
  initialization, array access, full-product linker integration or heap use.
- Kernel mutex atomicity for the C/C++ runtime (2026-11-18):
  `maia_mutex_trylock` and `maia_mutex_unlock` now mask local interrupts around
  their ownership/count transition and restore the caller's prior MIE state on
  every outcome. Host tests cover success, contention, wrong-owner release,
  recursive acquisition, and both initially enabled and disabled IRQ states;
  RV32 cross-compilation/disassembly confirms the `mstatus` mask/restore
  instructions. Full host CTest passed 78/78.
- Newlib lock backend: `runtime/newlib_locks.c` implements the retargetable
  callbacks over `maia_mutex`, including recursive locks and dynamic lock
  allocation from the Maia heap. Boot now initializes this backend after the
  heap and halts with an explicit diagnostic if setup fails. Host tests cover
  contention, recursion, yielding, lock close/reclaim, and IRQ-state
  preservation; CTest passes 79/79. The backend and updated `boot_main.c`
  compile for the product RV32 ABI. A relocatable link against the installed
  `libc.a` confirms the default no-op `libc_a-lock.o` is not extracted because
  Maia supplies both the callbacks and Newlib's static lock symbols. The
  isolated Tab5 runtime probe now exercises contended `std::mutex` lock/unlock;
  the canonical product XIP target also linked with this backend on 20/11;
  device boot acceptance remains pending.
- Newlib heap bridge: `runtime/newlib_heap.c` routes global and reentrant
  `malloc/free/calloc/realloc` calls to the Maia allocator, serializes access
  with Newlib's recursive malloc lock, and reports allocation failures through
  each reentrancy object's `errno`. The kernel allocator now exposes a checked
  allocation-size query for realloc/usable-size support. Host tests cover
  zeroing, alignment-sized capacity, data preservation, OOM behavior and
  freeing; CTest passes 80/80. The allocator and shell C++ translation unit
  compile for RV32. The isolated Tab5 probe exercised `calloc/realloc/free`;
  the canonical product XIP target linked with the heap bridge on 20/11.
  Device boot acceptance remains pending. Shell-local `malloc/free` shims were
  removed so C++ uses the same runtime heap.
- Per-task Newlib reentrancy: `runtime/newlib_reent.c` now provides
  `__getreent()` with isolated state for each Maia task and a separate
  boot/idle slot. The scheduler reclaims Newlib state when a task entry returns.
  Host tests cover slot isolation, stable pointers and reclaim/reinitialization;
  RV32 compilation passes. The isolated Tab5 runtime probe confirmed distinct
  task `errno`/`__getreent()` state in two workers over four warm-reset boots.
  The canonical product XIP target also linked and packaged with this runtime
  on 20/11. Slot reuse/reinitialization after task-ID recycling and product
  device boot remain untested.
- Gthread mutex bridge: `runtime/gthread.c` exports pthread mutex
  init/destroy/lock/trylock/unlock over Maia mutexes, supports default
  attributes and lazily registers static mutexes by address, and reports
  unsupported attributes/capacity and ownership errors. Host tests cover
  static/dynamic mutexes, contention/yield, self-lock, invalid unlock and
  destroy-while-locked. The RV32 object exports the expected pthread symbols.
  The isolated device probe exercised contended `std::mutex`, task-specific
  keys and destructors on task return; the canonical product XIP target linked
  and packaged on 20/11. `std::thread`, ELF TLS and product device boot remain
  pending.
- Full-product flash-XIP image preflight (2026-11-18): the complete
  `maia_m0.elf` links when `MAIA_USE_FLASH_PRODUCT_LINKER=ON`; the explicit
  non-XIP SRAM-linker configuration still overflows. The initial `esptool elf2image` attempt exposed two
  linker-script defects: the NOLOAD DROM reservation created an overlapping
  LOAD segment, and uncollected `.gcc_except_table.*` sections produced too
  many image segments. `flash_product.ld` now excludes the reservation from
  program headers and collects exception tables in `.flash.rodata`. The new
  optional `maia_m0_image` CMake target packages the ELF and runs
  `f0_flash_image_test.py`. The initial 768,528-byte image passes esptool
  checksum/hash checks and structural validation: five image segments, entry
  at `0x40000020`, first DROM/app descriptor at `0x400A0020`, exactly two
  flash-mapped segments, and SRAM load segments inside
  `0x4FF00000..0x4FF28000`. `readelf` confirms separate IROM/DROM LOAD
  addresses and no RWX segment.   This was an offline preflight image; subsequent display and boot-path fixes and
  the controlled device test are recorded below. At that time, the flash-XIP
  linker option was experimental and off by default; it was later promoted to
  the default with native LCD, as recorded below.
- Full-product XIP device smoke and recovery (2026-11-18): the first controlled
  boot showed the legacy display probe reading `0x48100000` before native PSRAM
  startup, causing a load-access fatal in `maia_display_init`. The product target
  now explicitly uses the existing internal-SRAM band fallback while native
  PSRAM startup is pending; it does not touch the uninitialized PSRAM window.
  The next boot exposed an intentional `ebreak` in `boot_main.c`, incompatible
  with the kernel's fail-closed unknown-breakpoint policy. The forced breakpoint
  was removed; scheduler timer and ECALL paths now exercise the trap handler.
  The final 766,672-byte image
  (SHA-256 `575253a5a85c7e9f1532c194dd3bad40f2391a4a1e110fe13e4ed58d7f03e1f8`)
  passed esptool packaging/structure checks and booted from the factory app
  offset. UART evidence shows three tasks, 205 timer IRQs, 21 context switches,
  shell `alloc`/`qtest` self-tests passing, and the `msh>` prompt. The runner
  backed up the original 2,093,696-byte app, verified both flash writes by
  esptool hash, restored it, and captured the original desktop boot. Evidence:
  [runner log](../../../maia-rtos-recovery/native-full-product-xip-scheduler-20261118/run.log),
  [native boot](../../../maia-rtos-recovery/native-full-product-xip-scheduler-20261118/probe-boot-1.log),
  [restore boot](../../../maia-rtos-recovery/native-full-product-xip-scheduler-20261118/restore-boot.log),
  and [report](../../../maia-rtos-recovery/native-full-product-xip-scheduler-20261118/report.json).
  This was one warm reset, not a cold PSRAM startup or visual acceptance. The
  framebuffer used SRAM fallback; PSRAM heap registration, cold initialization,
  repeated boots and product screen validation remain open.
- Product display visual observation (2026-11-18): the owner reported a stable,
  uniform cyan screen with no text during the first 60-second SRAM-fallback
  observation. The shell still reached `msh>` and passed `qtest`, but this is a
  product display failure, not visual acceptance. The first runner stopped
  after starting restoration without a completion record; the cause is not
  established. An independent `verify_flash` reported a digest mismatch. The
  preserved backup matched the known SHA-256, was rewritten with esptool hash
  verification, and the owner confirmed the original desktop returned.
  Evidence: [first run](../../../maia-rtos-recovery/native-product-visual-sram-20261118/run.log),
  [verification](../../../maia-rtos-recovery/native-product-visual-sram-20261118/restore-verification.log),
  and [manual restoration](../../../maia-rtos-recovery/native-product-visual-sram-20261118/restore-manual.log).
  The flash runner now protects the restoration phase from catchable interrupt,
  termination and hangup signals, checks that the backup covers all sectors
  affected by the probe, requires the restored image hash confirmation, records
  the restoration outcome and rejects fatal traps even after the marker. Host
  tests cover these contracts; no software can guarantee restoration after
  forced termination or loss of power/USB.
  A second 60-second run used the same image and reached `msh>` without a
  recorded fatal trap; automatic restoration was hash-verified and the captured
  original boot launched the desktop. The follow-up visual confirmation was
  cancelled, so the repeated image's appearance is not claimed. Evidence:
  [repeat report](../../../maia-rtos-recovery/native-product-visual-sram-repeat-20261118/report.json),
  [repeat boot](../../../maia-rtos-recovery/native-product-visual-sram-repeat-20261118/probe-boot-1.log),
  and [repeat restoration](../../../maia-rtos-recovery/native-product-visual-sram-repeat-20261118/restore-boot.log).
  No cold-boot or native PSRAM initialization/heap test was performed.

## Product display diagnosis and LCD/touch regression — 2026-11-18

A third product-image run (same SHA-256 `575253a5…`, 120 s observation) reached
`msh>` without fatal; the owner saw a black screen followed by cyan. Restoration
was hash-verified (`maia-rtos-recovery/native-product-visual-sram-observado-20261118`).

Diagnosis at that point: the product boot (`boards/m5stack/tab5/boot_main.c`) calls the
legacy terminal `maia_display_init` in `soc/espressif/esp32p4/soc_display.c`.
It never used the validated native LCD/touch path (`dsi_lcd_async_impl.h`,
`services/touch_input.c`, `drivers/input/st712x.c`). Since `c7591be`, no
change touched those drivers; the only probe change is code behind F0 `#ifdef`s.
The exact cause was not established then; the later shell experiment below
showed that the partial video path, not the approved LCD service, produced the
failed output.

Regression: the validated `--touch-lcd --framebuffer-visual` RAM probe was rebuilt from
HEAD `7a9f24f` plus local changes and run on the Tab5 (warm PSRAM, no flash write).
`MAIA_NATIVE_TOUCH_LCD_PASS`: 843 reports, 10 downs/10 ups/82 moves, corners
`0xF`, 0 bus/touch errors, 844 presented frames, no spurious LCD IRQs. The owner
confirmed the cross tracked the finger and disappeared on release; the desktop returned.
Evidence: `maia-rtos-recovery/native-touch-lcd-recheck-20261118` (`SHA256SUMS`).

Next: integrate the native LCD/touch services into the product boot. A full
720x1280 RGB565 framebuffer requires native cold PSRAM startup first.

## Candidate product DSI video-mode correction — 2026-11-18

The ST7121 product branch in `soc_display.c` programmed DSI host video-mode
word `0xFF02`, while the native DPI path that passed the LCD/touch visual test
programs `0xBF02`. The extra `0x4000` bit is also rejected by the native stage's
`0x14000` guard. The 30-entry ST7121 DCS table itself matches the validated
native table, so it was not changed.

Both product panel branches and the native staging path now use the shared
`MAIA_TAB5_DSI_VIDEO_MODE_CFG` (`0xBF02`) constant. A host `_Static_assert`
pins this value and `dsi_dpi_test` passes. The full XIP product target builds
and its packaged image passes structural validation (5 segments, entry
`0x40000020`, app descriptor `0x400A0020`); image SHA-256:
`108262989dc70b454ac1b8a192797807eb15ffa773875ee6e93aa6cc7dc27e23`.
The preserved candidate is
[maia_m0.bin](../../../maia-rtos-recovery/native-product-display-mode-candidate-20261118/maia_m0.bin).

This correction was not sufficient. The candidate was flashed with the
confirmed backup/restore runner; it reached `msh>` without a fatal trap, but
the owner observed the same cyan screen. The runner verified restoration of
the original firmware (`restore_verified=true`). Captures and report are in
[native-product-display-mode-observation-20261118](../../../maia-rtos-recovery/native-product-display-mode-observation-20261118/).

Comparing that outcome with the passing native visual probe exposed additional
known differences, not yet proven as the cause: the native sequence explicitly
reads COLMOD, writes DCS `0x3A 0x55` when needed, verifies four stable `0x05`
readbacks, and only then enables the DPI clock (`0x223` to `0x2A3`). The product
does not issue COLMOD and enables `0x2A3` before DSI command/panel setup. It also
uses the legacy circular SRAM-band DMA path rather than the native LCD stream.
The failed visual test disproves `0xBF02` alone as a fix; no further visual
claim is made. The product still uses the SRAM fallback; native cold PSRAM
startup and full LCD/touch service integration remain separate work.

Further direct source comparison found a more fundamental panel-sequence gap:
the product's inline table plus software reset sends 29 commands, omitting the
ST7121 `SLPOUT (0x11)` and `DISPON (0x29)` commands and their 85 ms/805 ms
waits. The passing shared program has 31 steps and 5 ms spacing. The legacy
sender also returned `void`, so its callers could continue after FIFO timeout
or `GEN_CMD_WR_ERR`. The current unflashed source change now calls the shared 31-step program for
detected ST7121 panels, propagates bounded DSI send errors, sends COLMOD
`0x3A 0x55`, and stages the DPI clock after timing/color setup. Host
`st7121_program_test` and `dsi_dpi_test` pass; the XIP `maia_m0.elf` builds and
the packaged image passes structural validation (767,584 bytes, 5 segments,
entry `0x40000020`, app descriptor `0x400A0020`). The image is at
`/tmp/maia-rtos-display-xip-check/maia_m0.bin`; it has not been flashed.
It does not yet read back COLMOD or replace the legacy SRAM-band DMA path, and
the product still bypasses the guarded native DSI API sequence. This is not a
claim that the full native display procedure has been ported; do not request
another visual observation until those differences are addressed.

## Native PSRAM product-startup attempt — 2026-11-18

Rebuilt the isolated native PSRAM boot plus framebuffer/LCD/touch RAM probe from
the current tree (99,856-byte ESP image, SHA-256
`bac487ed97e925944e8c7a5c9077f7bf3b839ee98235890e73a24c73b0b58ff5`).
The image is entirely loaded at `0x4ff00000`, entry `0x4ff00100`; it does not
write flash. Five focused host tests passed: PSRAM boot, native boot capture and
connection gates, LCD output, and touch input.

On-device DSI/MIPI setup and `DPI_FB_CACHE` passed. The strict native PSRAM
initializer then returned `MAIA_PSRAM_BUSY` (`-5`), stage0, boot writes0; the
framebuffer/DMA/LCD stream did not run. The runner reset to the preserved flash
application; the owner confirmed the original desktop returned.

A separate read-only snapshot after a standard reset recorded54 stable
controller samples and VO2 at `0x501151d0 = 0x40200180` (enable bit `0x100`
set), contrary to the required ROM-entry state. The snapshot is reset-state
evidence only, not a physical cold-power observation. Since the failed probe
did not print its call-time snapshot, the VO2 mismatch is consistent with, but
not proven to be the sole cause of, its BUSY result.

No product startup or driver code was changed. Do not weaken the native boot
guard or call cache bootstrap from the current XIP boot: the qualified pipeline
has so far run entirely from SRAM, while cache/MMU boot is a global ownership
change. Next work must establish a safe SRAM-to-XIP cache/MMU handoff and a
valid ROM-entry state before integrating PSRAM-backed LCD/touch services.
Evidence, build configurations, exact images and UART captures:
`maia-rtos-recovery/native-psram-product-startup-refusal-20261118/`.

### Owner-observed repeat of the PSRAM refusal — 2026-11-18

At the owner's request, repeated the exact native boot image (SHA-256
`bac487ed97e925944e8c7a5c9077f7bf3b839ee98235890e73a24c73b0b58ff5`)
with `--psram-native-boot --capture-seconds 30`, without flash writes.
Cache bootstrap passed; PSRAM boot again returned BUSY (-5), stage0, writes0.
The runner returned exit1 and reset to the preserved flash application.
The owner reported only a black screen followed by the desktop and confirmed
that the original desktop returned normally. No framebuffer/touch drawing was
reached in this run. This is separate from the successful warm LCD/touch test
showing bars and a cross tracking the finger.

The cyan screen in the product experiment does not by itself identify a cause;
display configuration/initialization faults remain possible. The earlier
observation that product boot uses a different display path is not a proven
explanation of the cyan color.

Repeat captures: `maia-rtos-recovery/native-psram-boot-repeat-20261118-1524/`.

## Integração do setup DSI nativo no produto — 2026-11-18

O caminho de produto para ST7121 agora exige firmware touch `1` lido com
sucesso; os demais painéis/firmwares preservam o caminho legado. Para esse
painel, a inicialização usa as APIs guardadas de CSI/MIPI park e gate, aquisição
VO3 calibrada, reset físico do painel com o transporte I2C nativo já testado,
abertura/configuração DSI, referência de 20 MHz, PHY de 960 Mbps, modo de
comando e o programa ST7121 compartilhado de 31 etapas. Em seguida, confirma
leituras estáveis de ID/power mode, programa e relê COLMOD RGB565, configura
timing DPI, habilita o clock e prepara a bridge. A detecção do firmware exato
foi exposta pela board IO e é reconfirmada no barramento antes do reset, para não
aplicar a sequência fw1 por suposição.

O caminho preserva o DMA circular em bandas SRAM existente; não integra o DMA
nativo nem a inicialização PSRAM a frio. A preparação DSI/MIPI pode parar com
erro explícito nas guardas, sem tentativa de rollback de estados de hardware
parcialmente escritos.

Validação: build do ELF XIP e empacotamento estrutural passaram (795,536 bytes,
5 segmentos, entrada `0x40000020`, descriptor `0x400B0020`); nove testes
focados de DSI, ST7121, MIPI, VO3, I2C e reset de painel passaram (9/9);
`git diff --check` passou. O candidato
`/tmp/maia-rtos-display-xip-check/maia_m0.bin` (SHA-256
`1bba29e0bbb08079f79039c263113e157203f67b152c52ae75437c840310179a`) foi
gravado no dispositivo em teste controlado. Um boot alcançou o marcador
`[DISPLAY] Initialized MIPI-DSI` sem fatal; a janela de observação visual durou
60 s. O runner confirmou a restauração do app original por hash e capturou o
boot restaurado. Registro: `../maia-rtos-recovery/native-product-display-native-dsi-20261118/`.
O dono observou tela ciano durante a janela do candidato com setup DSI nativo;
portanto, essa alteração isolada não corrigiu o sintoma visual. O log do boot
mostra GDMA habilitado e `BRG flow_st=0 raw_buf=0`. O rótulo `raw_buf` é
enganoso: o valor impresso vem do registro de interrupção bruta da bridge
(`+0x58`), não do nível do FIFO (`+0x14`). Esses valores pontuais não comprovam
falha nem sucesso na transferência de pixels. O refresh do produto ainda
usa o anel circular legado em SRAM, enquanto o caminho DMA nativo/framebuffer
validado permanece separado e não integrado. A comparação encontrou diferenças
concretas nos descritores/configuração: o caminho legado usa CTL0 `0x001E1B41`,
CTL1 `0x801080C0` e CFG1 `0x0A060001`; a lista espacial nativa validada usa
CTL0 `0x001C1B41` para fonte incremental, CTL1 `0x800F8040` e CFG1
`0x0A020001`. São diferenças a investigar, não causa confirmada. A lista
espacial finita é outro teste: não é o serviço de framebuffer que mostrou
barras e cruz. Copiar seus controles para o anel circular não reproduziu esse
serviço nem comprovou o funcionamento do anel.
O novo candidato aplica esses controles apenas ao perfil ST7121/fw1, preserva
os demais perfis, verifica que DMA/bridge estão drenados e recompõe os campos
de writeback antes de rearmar. Build XIP e empacotamento passaram (795,920 bytes,
5 segmentos, entrada `0x40000020`, descriptor `0x400B0020`); SHA-256
`fa55e0d75182cd886dd8e891f9daa525a96410163015583f1d4bc839b5af47b5`.
Teste controlado no dispositivo: um boot alcançou o marcador de display sem
fatal; o log confirmou CFG1 `0x0A020001`, GDMA ativo e `BRG flow_st=0
raw_buf=0`. O dono observou tela preta, então esse ajuste também não resolveu o
display; o backlight estava aceso. O runner confirmou a restauração do app original por hash. Captura:
`../maia-rtos-recovery/native-product-display-dma-config-20261118/`. Investigar
status/FIFO/writeback dos descritores e a diferença entre a lista finita nativa
validada e o anel circular de dez blocos. O próximo candidato remove, apenas no
perfil ST7121/fw1, os 5 ms entre habilitar DMA e alternar para vídeo; a sequência
nativa validada alterna imediatamente após iniciar o DMA. Hipótese ainda não
confirmada. O candidato XIP resultante foi empacotado e validado (795,920 bytes,
5 segmentos, entrada `0x40000020`, descriptor `0x400B0020`); SHA-256
`efa40624f4fdfb892650f31afd82cb6d24f3e85f275be601475f80ec0ebe1b39`.
O candidato foi gravado e alcançou o marcador de display; o dono relatou tela
ciano já quando começou a janela de observação. O runner foi interrompido e
não produziu relatório de restauração. O backup foi restaurado manualmente;
esptool verificou o hash, uma leitura independente dos 2 MiB foi idêntica ao
backup (`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`)
e o boot serial restaurado chegou ao `msh>` e ao carregamento de `desktop.mobj`.
Evidências: [teste interrompido](../../../maia-rtos-recovery/native-product-display-dma-order-20261118/run.log),
[leitura de verificação](../../../maia-rtos-recovery/native-product-display-dma-order-20261118/manual-restore-readback.log)
e [boot restaurado](../../../maia-rtos-recovery/native-product-display-dma-order-20261118/manual-restore-boot.log).
O CTest não está configurado no diretório XIP usado para build.

### Diferença entre as barras/cruz e o candidato XIP

- O [teste visual aprovado](../../tests/hardware/native_touch_lcd_probe.h)
  executou em RAM, sem gravar a flash, com PSRAM em estado morno. Ele desenhou
  um framebuffer RGB565 completo (720×1280, 1.843.200 bytes), incluindo barras
  e marcadores de toque.
- O teste usou o [serviço LCD nativo](../../services/lcd_output.c), DMA de um
  framebuffer por vez com conclusão por IRQ, e o serviço touch nativo que
  solicita os desenhos. A captura registrou 1.209 quadros DMA, 843 relatórios
  touch e zero erros. Ao terminar, o runner reiniciou o firmware preservado;
  o desktop visto em seguida não era o produto XIP candidato.
- Naquele candidato XIP inicial, o produto ainda usava o terminal legado
  ([código não-XIP](../../soc/espressif/esp32p4/soc_display.c)) com três bandas
  SRAM e uma lista DMA circular; não iniciava os serviços LCD/touch nem usava o
  framebuffer do teste. O teste shell posterior, registrado abaixo, corrigiu
  essa diferença e confirmou a imagem no device.

Restam touch no produto, desktop e inicialização de PSRAM a frio. O setup DSI ou
um marcador serial isolado não validam a imagem apresentada; o shell posterior
foi também conferido visualmente pelo dono.

### Vídeo nativo do produto: promovido a padrão XIP (18/11)

O build XIP do produto usa agora o serviço LCD nativo por padrão. O caminho de
bandas SRAM/lista circular continua apenas na configuração explicitamente
não-XIP; não é incluído no alvo de produto padrão.

- Depois do setup nativo do ST7121, o [display do produto](../../soc/espressif/esp32p4/soc_display.c)
  no build XIP é o [driver nativo de terminal](../../soc/espressif/esp32p4/soc_display_product.c)
  e não executa a ponte/DMA legada. Repete a sequência PSRAM do teste aprovado:
  `cache_adopt` → `snapshot` → `xtal20_start` → `x8_mode(0)` →
  `fast_start(200)` → `window_start`, cada passo com `[DISPLAY] NATIVE_LCD step=`.
  Em falha, informa e não usa vídeo alternativo.
- `maia_esp32p4_psram_cache_adopt` ([janela PSRAM](../../soc/espressif/esp32p4/soc_psram_window.c))
  é a única diferença: no XIP o bootloader já ligou os caches que executam a
  imagem, então só consulta o modo do cache, sem reinicializá-lo pela ROM.
- O framebuffer, a DMA e a IRQ são os do [serviço LCD](../../services/lcd_output.c).
  `putc` só altera a grade de texto; uma tarefa pede `request_update` e o desenho
  das linhas alteradas ocorre na tarefa dona do LCD. Touch ainda não entra.
- Build: imagem de 806.720 bytes, 5 segmentos, entrada `0x40000020`; código PSRAM
  em SRAM (16 KB), pilha termina em `0x4FF20640`. Testes de host
  `psram_window`, `lcd_output`, `dw_dma_irq`, `dsi_lcd_async` e
  `dsi_framebuffer` passaram, com casos novos para `cache_adopt`.
- O CMake agora seleciona `soc_display_product.c`, `lcd_output.c` e o dono da
  IRQ DMA automaticamente quando `MAIA_USE_FLASH_PRODUCT_LINKER=ON`, opção que
  é ligada por padrão. A opção temporária `MAIA_PRODUCT_NATIVE_LCD` foi removida.
  O linker também exige que `maia_esp32p4_psram_cache_adopt` fique na closure SRAM.
- Resultado no device: todos os passos `NATIVE_LCD` retornaram OK, o shell
  chegou a `msh>` e executou `ls`/`ps`. O dono viu o cabeçalho e o prompt em fundo
  preto e executou comandos como `ps`. Imagem SHA `f9e6c308…6709`; firmware
  original restaurado e conferido (`08cee753…6abc06`). Evidências em
  `maia-rtos-recovery/native-product-shell-lcd-service-20261118/`.
- Conclusão limitada: o serviço LCD nativo produz imagem no produto XIP. Os
  candidatos anteriores com vídeo parcial falharam; os testes não estabeleceram
  a causa exata da tela ciano nem excluem problemas no painel ou na sequência.
- Após promoção: a imagem foi reconstruída pelo build padrão, sem definir uma
  opção extra de display. A validação estrutural passou (5 segmentos,
  entry `0x40000020`); no device, o shell apareceu e executou `ls`. Imagem SHA
  `30df29cb…e9602`; o runner restaurou o backup e confirmou o SHA
  `08cee753…6abc06`. Evidências em
  `maia-rtos-recovery/native-product-shell-lcd-default-20261118/`.
- O dono confirmou piscada lenta da tela inteira, incluindo cabeçalho e fundo,
  e depois lembrou que o teste das barras também piscava. A confirmação anterior
  comprovou imagem e resposta do touch, não estabilidade visual. O renderer do
  shell só redesenha linhas alteradas; barras e shell compartilham o serviço LCD.
  Ele inicia e finaliza uma transferência DMA por quadro, restaura os estados
  DPI/DSI na conclusão e inicia o quadro seguinte. Isso é uma hipótese de
  investigação, não uma causa comprovada: medir intervalos e transições entre
  quadros, inclusive sem alterações, e comparar com o firmware preservado.
  Touch e desktop no produto continuam fora deste escopo.

### Comparação do reinício entre quadros (18/11)

- O serviço nativo repete `maia_lcd_async_start` e `maia_lcd_async_finish`
  para cada quadro. O início ativa vídeo/clock HS e saída DPI; após o DMA,
  `frame_cleanup` desabilita o DMA, restaura a saída DPI desativada e retorna
  o host ao modo de comando, antes do próximo quadro. Isso reinicia o caminho
  de vídeo, não a alimentação nem o reset físico do painel.
- Na referência ESP-IDF preservada (revisão `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`),
  `dpi_panel_init` habilita vídeo e saída DPI; `dma_trans_done_cb` rearma o
  descritor e o canal DMA sem desligar vídeo/DPI por quadro. A diferença é
  consistente com a hipótese do dono, mas ainda não prova que causa a piscada.
- Próximo experimento: medir a duração das transições e desenhar um fluxo nativo
  sustentado, com ownership de framebuffer/cache e tratamento de erros
  preservados; validar imagem estável no device. Não remover os guards nem
  copiar dependências de runtime da ESP-IDF. Nenhuma alteração de código ou
  novo teste físico foi feito nesta comparação.

### Candidato de refresh sustentado (18/11)

- O serviço agora rearma o DMA no callback de conclusão: confirma DMA-done,
  contagem bruta `0x383FF`, erros e estado do host/bridge; limpa apenas o status
  GDMA já qualificado, reposiciona o SAR e reinicia canal/IRQ. DPI, modo de vídeo
  e LP clock permanecem ativos nos quadros sem atualização.
- Uma solicitação de desenho ou parada pede pausa no limite do próximo quadro.
  Só depois de DMA/FIFOs drenarem o owner pode editar/publicar o framebuffer; a
  retomada religa DMA sem repetir a transição DSI/DPI. O desligamento normal
  continua usando a recuperação/restauração guardada existente. O buffer único
  nunca é escrito enquanto o DMA o lê.
- Testes host `lcd_output_test` e `dsi_lcd_async_test` passam com cobertura de
  rearmamento repetido, pausa para edição, retomada, parada, drain, timeout e
  falha de status/underrun e preservação dos estados. Suíte completa: 84/84.
- Build XIP/linker e empacotamento passam: 801200bytes,5 segmentos,
  entry`0x40000020`; imagem SHA256
  `d0a222b1ce0782a2a585cb3465657c16ab2f307347534b23d94775b13550d603`.
- Boot eletrônico controlado no Tab5 chegou a `msh>` com todos os passos
  `NATIVE_LCD` em OK e sem marcador fatal. O runner restaurou o app original e
  verificou novamente o hash `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
- Observação visual do dono por 60 s: a tela permaneceu estável em repouso e
  piscou a cada tecla pressionada. Isso valida visualmente o refresh contínuo
  estático neste teste, mas a atualização acionada pela entrada continua com
  interrupção perceptível. A observação não determina a causa; medir a cadência
  e eliminar a piscada durante atualizações seguem pendentes. Não há garantia
  tear-free durante atualizações nem medição de WCET.

### Candidato de atualização com double buffering (19/11)

- Implementado um segundo framebuffer na janela virtual de PSRAM, ampliada para
  4 MiB. O DMA lê o buffer frontal enquanto o owner redesenha o quadro completo
  no traseiro; a origem do DMA é trocada somente na IRQ de fim de quadro.
  Publicar o traseiro inclui write-back de cache antes de ele poder ser lido.
- O serviço só conta uma atualização como apresentada depois de um quadro
  completo do novo buffer. A parada ainda ocorre em limite de quadro e mantém o
  drain; os caminhos de terminal, toque e probe passam a desenhar quadros
  completos, sem escrever no buffer que o DMA está lendo.
- Validação host: build completo e 84/84 testes CTest passam. Verificação de
  sintaxe Pylance dos dois scripts Python alterados também passou.
- Build XIP e empacotamento passam. Imagem: 801.488 bytes, cinco segmentos,
  entry `0x40000020`, descriptor `0x400B0020`; teste estrutural de imagem PASS.
  Arquivo `/tmp/maia-product-xip-stream-build/maia_m0.bin`, SHA-256
  `5dbc5b10e7efdb958991c2621048ffb9ab8f38abc69d2b84c2f5aafcb06e92dd`.
- Primeira gravação: a captura de 8 s terminou após `window_start`, sem prompt
  nem fatal; o firmware original foi restaurado com hash verificado. O dono viu
  tela preta e depois o desktop. Isso não comprovou travamento.
- Repetição da mesma imagem com captura de inicialização de 30 s: chegou a
  `msh>`, sem fatal; janela visual de 60 s concluída e restauração verificada.
  O dono confirmou que não pisca ao teclar, mas relatou faixa suja de
  aproximadamente 5 pixels na borda inferior durante a entrada.
  Evidência: [relatório](../../../maia-rtos-recovery/native-product-double-buffer-repeat-20261119/report.json)
  e [boot capturado](../../../maia-rtos-recovery/native-product-double-buffer-repeat-20261119/probe-boot-1.log).
  Cadência/WCET, boot frio e certificação tear-free continuam pendentes.
- Comparação direta: o BSP original configura `num_fbs = 1`, e a referência
  ESP-IDF permite um ou mais buffers (zero seleciona um). Seu `draw_bitmap`
  copia/sincroniza os pixels sem parar o refresh; a IRQ rearma o DMA. Portanto,
  double buffering não é necessário para evitar piscada no driver original.
  A pausa explícita para edição do nosso candidato de buffer único era uma
  diferença de implementação, não um requisito imposto pelo painel.

### Correção candidata da margem inferior do terminal (19/11)

- O renderer preenche 22 linhas de 32 pixels (704 pixels lógicos), deixando os
  16 pixels restantes sem desenho no backbuffer. O primeiro framebuffer era
  zerado no boot; o segundo não tinha essa região preenchida.
- Os dois caminhos de terminal agora preenchem essa margem em preto a cada
  desenho, sem alterar DMA, cache, clocks ou inicialização do display.
- Novo `product_terminal_test` usa o renderer real, dois buffers com sentinelas,
  quatro atualizações e verificação de todos os pixels, margem preta e limites.
  Falhou antes da correção e passou depois. Suíte host: 85/85.
- Nova imagem XIP: 801.536 bytes, cinco segmentos; validação estrutural PASS.
  SHA-256 `e619152d3b4e5b26e234244a1cc19bf248a147dee008d19f7dbc5ba5a1194ecb`.
  Gravada em teste controlado: chegou ao shell, sem marcador fatal, com
  observação de 60 s após o prompt. O dono confirmou: “Sem piscada e sem faixa
  suja”. O firmware original foi restaurado com hash verificado
  `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
  Evidências: [relatório](../../../maia-rtos-recovery/native-product-terminal-margin-20261119/report.json)
  e [boot capturado](../../../maia-rtos-recovery/native-product-terminal-margin-20261119/probe-boot-1.log).
  Aceitação visual limitada a este teste morno; medição de cadência/WCET, boot
  frio, touch integrado e desktop completo permanecem pendentes.

### Touch nativo no produto: candidato eletrônico (19/11)

- O produto XIP inicializa o serviço ST7121 já testado no probe, após o reset
  e a inicialização do LCD. O barramento preparado para reset é adotado pelo
  serviço I²C; acessos do teclado no I²C de sistema passam pelo mesmo dono,
  incluindo leitura de blocos e escrita de registradores de 8 bits. O teclado
  no barramento externo permanece no caminho existente. O touch é uma task
  própria, com polling a cada 5 ticks e snapshot com rotação de 90 graus.
- O terminal redesenha o quadro completo no backbuffer e sobrepõe uma cruz
  branca em cada contato válido. Falhas invalidam o snapshot e removem o
  marcador sem inventar uma soltura; não há fila de eventos nem desktop
  integrado. Controladores e dimensões diferentes do ST7121 firmware 1 com
  raster 720×1280 são recusados.
- Suíte host: 86/86. Os testes cobrem handoff, falhas de inicialização,
  invalidação, arbitragem entre chamadas de 8 e 16 bits, leitura de 1 a 70
  bytes com START repetido, falhas em cada pausa, e marcador nos limites dos
  dois buffers. O teclado conectado ao I²C de sistema ainda não foi exercitado
  no hardware; no Tab5 testado ele está no barramento externo.
- Imagem XIP validada: 804.144 bytes, cinco segmentos, entry `0x40000020`,
  SHA-256 `6950909917ec8371d22a49749b35bedfda7593721c8f8dc18743dc9f9b6a4485`.
  Boot morno chegou a `msh>` com `[TOUCH] Native ST7121 service ready`,
  task `touch` com nove trocas de contexto e sem erro de polling ou fatal
  na captura. Isso não comprova contatos físicos.
  [Boot capturado](../../../maia-rtos-recovery/native-product-touch-electronic-20261119/probe-boot-1.log);
  [relatório](../../../maia-rtos-recovery/native-product-touch-electronic-20261119/report.json).
  O firmware original foi restaurado com verificação de hash
  `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`
  e boot até o desktop no log.
- A validação física do produto foi iniciada após esse boot eletrônico.
  Boot frio, SMP, teclado no I²C de sistema e desktop completo continuam fora
  desta evidência.

**Teste físico no produto, 19/11:** a mesma imagem iniciou com o serviço touch
pronto, chegou a `msh>` e permaneceu em observação por 60 s. O dono confirmou
que a cruz acompanhou o dedo inclusive nos cantos, desapareceu ao soltar, o
teclado funcionou e a tela ficou sem piscada ou faixa suja. A captura contém
os comandos `ls` e `ps`; neste último a task touch registra 2368 trocas de
contexto. Não foram registrados erros de polling ou fatal; o runner não mede
quantidade de contatos, latência ou cadência de quadros.

O firmware original foi restaurado com verificação do hash
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`;
o log registra a inicialização do desktop original.
[Relatório](../../../maia-rtos-recovery/native-product-touch-physical-20261119/report.json),
[boot do candidato](../../../maia-rtos-recovery/native-product-touch-physical-20261119/probe-boot-1.log)
e [boot restaurado](../../../maia-rtos-recovery/native-product-touch-physical-20261119/restore-boot.log).
HARDWARE-TESTED limitado ao reset morno, ST7121 fw1 e teclado externo deste
Tab5; boot frio, SMP, teclado no I²C de sistema, eventos sem perda e desktop
nativo completo permanecem pendentes.

## Probe nativo de cold boot PSRAM — 2026-11-19

Corrigi a ordem do diagnóstico para executar `PSRAM_BOOT` antes de
`DPI_FB_CACHE`; o teste host aceita a ordem correta e rejeita a inversa. O
probe RAM foi recompilado com o linker de RAM, passou o gate de símbolos e a
validação offline; a suíte host completa passou 86/86.

No Tab5, a captura automática de reset morno passou MIPI/VO3, DSI,
ST7121/power e bridge DPI, mas a inicialização nativa retornou
`MAIA_PSRAM_BUSY` (`-5`), estágio0, zero escritas. Não houve marcador de cache
nem execução de memória, DMA, stream LCD ou touch. O runner retornou
`FAIL / NOT DEMONSTRATED` e acionou reset para o aplicativo em flash. Nenhuma
imagem foi gravada na flash. Isso confirma a recusa segura no estado morno,
não uma partida fria bem-sucedida; falta snapshot dos registradores no instante
da chamada e nenhum ciclo físico de energia foi feito.

Imagem final padrão (97.760 bytes, SHA-256
`903234a96dbbb00a6458490e399c3fb52850786f016e45f820e0ba9cb67a0ecc`) e
[captura/ELF/mapa/configuração](../../../maia-rtos-recovery/native-psram-native-boot-order-20261119/default-product-option/).
Continuar preservando as guards; aceitação de cold boot e pipeline completo
PSRAM/LCD/touch permanecem pendentes.

## Diagnóstico dos guards de entrada da PSRAM — 2026-11-20

O relatório do boot agora preserva o snapshot usado na avaliação dos guards,
a palavra AXI, a amostra MPLL condicional e um bitmask nomeado das recusas. O
probe imprime a linha `PSRAM_BOOT_ENTRY`; os caminhos inválidos, fora de
contexto e duplicados continuam sem MMIO. A suíte host passou 86/86, incluindo
testes positivos e negativos de todos os guards de entrada, claims de clock/X8,
duplicação, falha anterior e MPLL ocupado. O probe RV32 passou o gate de
símbolos e a validação de imagem RAM.

Na captura automática de reset morno do Tab5, o boot retornou
`MAIA_PSRAM_BUSY` (`-5`), estágio0, zero escritas. O bitmask `0x500` identifica
`VO2_ENABLED` (`0x100`) e `DIVIDER_NOT_XTAL` (`0x400`); a amostra estável foi
VO2 `0x40200180`, root1 `0x0000D00C`, máscara do divisor `0x00FF3000`. AXI,
controllers e eFuse passaram seus guards. O runner acionou reset para o
firmware em flash, sem gravar flash. Nenhum ciclo físico de energia foi feito e
não houve execução de cache, memória PSRAM, DMA, LCD ou touch. A causa da
recusa morna está identificada, mas cold boot e o pipeline integrado continuam
pendentes.

[Imagem e captura UART](../../../maia-rtos-recovery/native-psram-entry-diagnostics-20261120/):
99.008 bytes, SHA-256
`67ccae80de64d55fc5ec9340ac71d4248c2f78045700271e3f735b36178e06ac`;
captura SHA-256
`f2dcd2494f89227e6c96486534cd17bf10b1eaf56b1ad624c7694ee198391607`.

## Smoke do runtime Newlib/gthread no Tab5 — 2026-11-20

O alvo isolado `maia_f1_runtime_probe_image` validou no P4 `write`, `read` sem
dados (`EAGAIN`), relógio monotônico, `nanosleep`, heap, `errno`/`__getreent`
por tarefa, `std::mutex` com contenção, `pthread_key_t` e destrutores no retorno
de duas tarefas. Houve quatro boots por reset morno (um inicial e três
repetições), todos com `MAIA_F1_RUNTIME_PASS`; a restauração do app original
foi verificada por hash, e o boot restaurado chegou ao `msh>`/`desktop.mobj`.
Os três testes host focados passaram (3/3); os alvos de imagem F1 e F0 passaram
empacotamento/validação estrutural. A varredura de símbolos do probe não
encontrou FreeRTOS.

Escopo limitado: o caminho de leitura com dados não foi exercitado no device;
naquele smoke isolado, `thread_local`/ELF TLS, construtores/guards C++, reuso
da slot `reent` após reciclagem de ID e `std::thread` não foram validados.
Construtores/guards foram cobertos depois, como registrado abaixo. A tentativa
exploratória de incluir o smoke translation unit no ELF do produto atingiu a
asserção de residência SRAM de `maia_esp32p4_psram_cache_adopt`, sem
enfraquecer o guard;
o alvo canônico do produto foi linkado e empacotado com sucesso em 20/11
(detalhes abaixo). Nenhuma validação de boot do produto, display, touch ou
PSRAM decorre deste smoke.

Imagem, capturas, backup e relatório:
`../../maia-rtos-recovery/native-f1-runtime-probe-repeat-20261120/`.
Detalhes em [EVIDENCE.md](../hardware/EVIDENCE.md).

## Link XIP canônico do produto com runtime F1 — 2026-11-20

`cmake --build /tmp/maia-rtos-f1-runtime --target maia_m0_image -j4` passou:
`maia_m0.elf` linkou com Newlib/gthread e o alvo empacotou/validou a imagem
ESP32-P4. Binário de 804.144 bytes, entry `0x40000020`, cinco segmentos,
descriptor `0x400B0020`, checksum/hash válidos; SHA-256
`e9615cf79c2f1c910bbc5581896b8bd7f4db832a2f223081554850e44ca97f3d`.
O ELF contém `__getreent`, `_read`, `_write`, `clock_gettime`, `nanosleep` e
`pthread_mutex_lock`; `maia_esp32p4_psram_cache_adopt` está em `0x4ff03b1c`,
dentro da SRAM interna. A auditoria de símbolos não encontrou nomes das
famílias comuns FreeRTOS (`xTask`, `vTask`, `uxTask`, `xQueue`, `vQueue`,
`xSemaphore`, `vSemaphore`, `xTimer`, `vTimer`, `pvPortMalloc`, `vPortMalloc`,
`xPortStartScheduler`, `portYIELD`, `heap_2/3/4`) nem símbolos de runtime
`esp_timer`, `esp_event`, `esp_log`, `esp_system`, `esp_heap_caps`, `esp_cache`,
`esp_task_wdt`, `esp_pthread`, `esp_newlib` ou `esp_pm`. `nm -u` encontrou zero
símbolos indefinidos; o mapa de link não contém caminhos de archives
FreeRTOS/ESP-IDF correspondentes. ELF SHA-256:
`5148207b3c81bd298885735ccb92c200c99cc2c4271d035b451bceb6f76dcb89`;
mapa SHA-256:
`84f9785aa61199acaeceec772ef88ef14ba84622507c7a3b11b317caf6c310b8`.
Isso conclui a auditoria de nomes/caminhos, mas não a revisão ampla para excluir
código vendor renomeado ou embutido; o item F10 permanece aberto.

A imagem, ELF auditado, mapa e relatório estão em
`../../maia-rtos-recovery/native-product-xip-link-20261120/`. Não foi gravada
nem inicializada no device durante a validação de link; o boot controlado
posterior está registrado abaixo. A tentativa anterior que injetava o smoke
probe-only no ELF do produto falhou nessa asserção; o build canônico atual
passa sem alterar o guard. O boot controlado com backup e restauração verificada
está descrito abaixo; a observação visual e a interação touch foram confirmadas
em execuções posteriores. Cold boot/PSRAM continuam pendentes.

## Boot morno do produto XIP no Tab5 — 2026-11-20

O runner `run_f0_flash_probe.py` fez backup e validação da app original,
gravou a imagem canônica no offset `0x10000`, capturou um boot pelo marcador
`msh>` e restaurou a app com hash verificado. Resultado: `probe_pass=true`,
`restore_verified=true`, marker presente e `fatal=false`; SHA-256 da app
restaurada `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.

No boot morno, `cache_adopt`, snapshot, `xtal20_start`, `x8_mode`,
`fast_start`, `window_start` e `window_sync` reportaram resultado zero. Os
serviços nativos LCD/framebuffer PSRAM e touch ST7121 anunciaram inicialização;
as três tarefas do scheduler iniciaram, o self-test de mutex/semaphore/queue
passou e o shell chegou a `msh>`. A captura não contém fatal nem stack fault.
O boot restaurado alcançou a app original, carregou `desktop.mobj` e aplicou
`wallpaper.raw`.

Este resultado supersede a afirmação anterior de que o caminho XIP/cache/MMU
só havia sido executado no probe SRAM, mas vale apenas para um reset morno e
para o caminho canônico do produto. O probe RAM-only continua recusando seu
snapshot específico pelos guards `0x500`; nenhum guard foi alterado. Não houve
toque, boot a frio nem validação de conteúdo de framebuffer neste primeiro
boot. Capturas, report e backup:
`../../maia-rtos-recovery/native-product-xip-smoke-20261120-1218/`.

Uma segunda execução manteve o mesmo produto ativo por 60 s após `msh>`.
O dono confirmou tela correta e estável, sem ciano, piscada ou artefatos
visíveis; captura serial sem fatal, `probe_pass=true` e restauração verificada.
Não houve interação touch nessa segunda execução nem boot a frio. Evidências:
`../../maia-rtos-recovery/native-product-xip-visual-20261120-1220/`.

Na terceira execução de 60 s, o dono confirmou visualmente a cruz branca
seguindo o dedo e desaparecendo ao soltar. UART confirmou serviço ST7121
iniciado, prompt `msh>` e ausência de fatal; o runner restaurou novamente o
app original com hash verificado. Não houve boot a frio. Evidências:
`../../maia-rtos-recovery/native-product-xip-touch-20261120-1233/`.

## Inicialização C++ estática e guards no F1 — 2026-11-20

Corrigido `maia_gthread_once`: o Newlib real inicializa `pthread_once_t` com
`{1, 0}`, incompatível com a interpretação anterior, que podia esperar
eternamente. O backend agora reconhece esse ABI e objetos zero-inicializados,
com estado intermediário `2` durante o inicializador. O teste host
`gthread_test` passou com os dois formatos e verifica execução única.

`start.S` executa `__libc_init_array` depois de `maia_soc_early_init` e antes
de `maia_boot_main`; os três linkers retêm `.preinit_array`/`.init_array`.
O probe F1 validou construtor global e guarda de variável estática local sob
contenção entre duas tarefas. Quatro boots mornos passaram. O build canônico
`maia_m0_image` passou e confirmou entradas reais do shell/libstdc++ em
`.init_array`, zero símbolos indefinidos e imagem estruturalmente válida
(806.592 bytes, 5 segmentos, entry `0x40000020`, descriptor `0x400B0020`).
A imagem SHA-256 é
`3cb55a05b31776e31fffbe54b78aa6a9eab86166830094b7657583751f3281ac`.

Um boot morno do produto no Tab5 chegou a `msh>` sem fatal. O runner restaurou
o app original e confirmou o SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Relatório, logs e backups estão em
`../../maia-rtos-recovery/native-product-xip-cxx-init-20261120/`; resultados
repetidos do probe estão em
`../../maia-rtos-recovery/native-f1-cxx-init-repeat-20261120/`.
Isso valida o caminho morno, não cold boot. Permanecem pendentes: TLS de
compilador (`thread_local`/ELF TLS) e `std::thread`.

## Leitura serial positiva no F1 — 2026-11-20

O probe agora verifica primeiro `read(STDIN_FILENO)` vazio com `EAGAIN`,
publica `MAIA_F1_RUNTIME_INPUT_READY` e recebe quatro bytes (`Maia`) por USB
Serial/JTAG, conferindo conteúdo e quantidade antes de emitir PASS. O runner
aceita envio opcional via `--input-marker`/`--input-hex`, sem alterar o caminho
padrão; falha explicitamente se o marcador faltar ou houver escrita curta.
`f0_flash_runner_test` e `gthread_test` passaram (2/2).

A imagem de 108.640 bytes passou validação estrutural; quatro boots mornos no
Tab5 receberam o payload e emitiram `MAIA_F1_RUNTIME_PASS`, sem fatal. O runner
restaurou o app original e confirmou o SHA-256 já registrado acima. Logs e
relatório: `../../maia-rtos-recovery/native-f1-positive-read-20261120/`.
Leitura positiva por UART0 e teclado não foi testada. F1 ainda tem pendentes
TLS de compilador e `std::thread`.

## Reciclagem do slot Newlib `reent` no F1 — 2026-11-20

Após o retorno dos dois workers, o probe espera dois ticks e cria uma tarefa
substituta. Ela precisa receber o ID reciclado do primeiro worker, o mesmo
endereço de `__getreent()` com `_errno == 0` e `pthread_getspecific()` limpo.
Isso cobre a reutilização real do slot EXITED e a limpeza feita no trampoline.

A imagem de 108.944 bytes passou validação estrutural; quatro boots mornos no
Tab5 emitiram `MAIA_F1_RUNTIME_PASS` sem fatal e repetiram a leitura USB
positiva. O app original foi restaurado com hash verificado. Os testes host
`newlib_reent_test`, `gthread_test` e `f0_flash_runner_test` passaram (3/3).
Relatório e capturas: `../../maia-rtos-recovery/native-f1-reent-reuse-20261120/`.
Isso valida uma reutilização; estresse repetido de slots, TLS de compilador e
`std::thread` continuam pendentes.

## TLS estático de compilador por tarefa no F1 — 2026-11-20

Implementado TLS ELF estático com modelo local-exec. Os linkers delimitam
`.tdata`/`.tbss` e publicam `PT_TLS`; o contexto RISC-V salva/restaura `tp`,
e cada tarefa recebe uma cópia inicializada e alinhada. Boot, idle e tarefas
possuem blocos independentes; o bloco é zerado e liberado ao terminar uma
tarefa. O probe testa TLS inicializado e zero-inicializado, isolamento entre
workers durante preempção e reset após reciclar o ID. `tls_runtime_test`
verifica cópia do template, zero-fill, alinhamento, isolamento e reuso.

O probe de 110.048 bytes passou validação estrutural; quatro boots mornos no
Tab5 emitiram `MAIA_F1_RUNTIME_PASS` sem fatal e repetiram a leitura USB
positiva. A imagem foi restaurada após o teste e o SHA-256 do app original foi
confirmado. A suíte host completa passou (87/87). Relatório, backups e logs
estão no [diretório de evidências](../../../maia-rtos-recovery/native-f1-compiler-tls-20261120/);
o ELF, a imagem e o mapa dessa execução não foram preservados.

O suporte atual cobre TLS estático local-exec com variáveis triviais e
inicialização constante; TLS dinâmico e construtores/destrutores não triviais
não estão implementados. `std::thread`, cold boot e estresse de alto volume
de reciclagem de slots seguem pendentes. A tentativa de linkar a configuração
alternativa sem XIP não passou: `.text` excedeu `L2MEM_LOW` em 692.752 bytes;
esse layout não foi validado e requer uma redução de imagem independente.

## TLS com alinhamento máximo e boot do produto — 2026-11-20

O probe agora contém uma variável `thread_local` alinhada a 64 bytes e confere
seu endereço, valor inicial, isolamento e reutilização. O ELF resultante tem
`PT_TLS` com alinhamento de 64 bytes e a variável no offset `0x40`. Quatro boots
mornos passaram; os testes host focados TLS, gthread e scheduler passaram
(4/4). A imagem de 110.160 bytes, ELF, mapa e logs estão em
[evidências do probe](../../../maia-rtos-recovery/native-f1-tls-align64-20261120/).

O produto XIP com TLS integrado passou por um boot morno até `msh>`, com
`qtest PASS` e serviços LCD/touch anunciados; não houve nova conferência visual.
O app original foi restaurado com hash verificado após ambos os testes.
Imagem, ELF, mapa e logs: [evidências do produto](../../../maia-rtos-recovery/native-product-xip-tls-20261120/).

## F1 suporte básico a `std::thread` — 2026-11-20

Implementados `pthread_create`, `pthread_join`, `pthread_detach`, identidade
estável independente do ID reciclável do scheduler, atributos básicos de stack
e retorno/`pthread_exit`. Registros joinable sobrevivem à saída até o `join`;
threads detached removem o registro ao sair. A stack própria só é liberada no
hook de conclusão, depois do estado `EXITED`, no caminho de trap que já usa a
stack de IRQ. O runtime usa exclusão por IRQ local e, portanto, permanece
limitado ao scheduler single-core atual; isto não é uma implementação POSIX
completa nem evidência de SMP.

Host: os testes focados `gthread_test`, `tls_runtime_test` e
`pthread_runtime_test` passaram (3/3), incluindo task-ID reutilizado antes do
join do primeiro pthread. A suíte host completa passou (88/88). Os alvos
`maia_f1_runtime_probe_image` e `maia_m0_image` compilaram/empacotaram.
No Tab5, o probe passou em dois boots mornos com criação, ID, join e detach,
incluindo 16 criações sequenciais alternando `join`/`detach` em cada boot e a
entrada serial esperada; o produto XIP chegou a `msh>` com `qtest PASS` e
serviços LCD/touch anunciados. Ambos os runners restauraram o app original e
verificaram seu SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Capturas e binários:
`../../../maia-rtos-recovery/native-f1-std-thread-churn-20261120/`
e `../../../maia-rtos-recovery/native-product-xip-pthread-20261120/`.

O teste do produto foi eletrônico: não houve nova observação visual/touch nem
validação de cold boot. TLS dinâmico, inicializadores/destrutores não triviais
de `thread_local`, SMP e APIs POSIX além do subconjunto exercitado permanecem
fora deste resultado. O ciclo sequencial é uma validação limitada, não um
estresse prolongado/de alto volume nem prova de sincronização multicore.

## F2 leitura SDHC — base portátil HOST-TESTED — 2026-11-20

Adicionada uma biblioteca separada para decodificar a geometria CSD v2 de
cartões SDHC/SDXC e validar o endereço de uma leitura de 512 bytes antes de
delegá-la a um transporte `CMD17`. O transporte deve conferir resposta R1,
comprimento e CRC, aplicar seus requisitos de DMA/cache e informar falhas;
eventuais dados parciais em caso de erro devem ser descartados. SDSC, SDUC e
outros formatos não são suportados por esta etapa. Não há escrita, formatação,
montagem ou inicialização de cartão nesta biblioteca.

`sd_read_test` cobre campos CSD entre palavras, capacidade até 2^32 blocos,
último bloco válido, endereços fora do limite, OCR/CSD incompatíveis, argumentos
inválidos e propagação de erros do transporte simulado. No macOS/AppleClang
21, CTest completo passou 89/89 e o teste focado passou com ASan/UBSan; o
cabeçalho compilou em C++98. A biblioteca também compilou com GCC
RISC-V Espressif 14.2; `maia_m0_image` continuou compilando. A biblioteca é
um alvo independente e ainda não está ligada ao produto nem a um controlador.

**Hardware NOT RUN:** esta etapa não acessou o Tab5, o microSD nem o pendrive.
O log anterior da restauração do firmware funcional mostra sincronização de
arquivos para `/sd` no boot: antes de um futuro teste físico, resolver como
evitar essa escrita automática ou preservar os dados independentemente.
O próximo passo é levantar e portar a alimentação, pinagem, clock e transporte
SDMMC nativos, com comandos e testes de leitura delimitados. Ver
[evidências](../hardware/EVIDENCE.md) e
[questões de hardware](../hardware/OPEN_QUESTIONS.md).

## F2 observação SDMMC no Tab5 — 2026-11-20

Uma sonda independente, carregada em RAM, observou duas vezes o estado de VO4,
os pinos e o controlador SDMMC depois da entrada automática no ROM loader.
Ambos os boots mornos passaram, com oito amostras estáveis cada. VO4 estava
habilitado e em bypass, mas isso não mede a tensão física. O clock do cartão
estava desabilitado e a função dos pinos não era a função SDMMC nativa: o
transporte precisará configurar esses recursos em vez de presumir herança do
firmware original. Esta sonda não enviou comandos SD nem leu setores.

A sonda não gravou na flash; após os testes, o firmware original montou `/sd`,
sincronizou arquivos e carregou o desktop e o wallpaper. O proprietário
confirmou que o cartão é para testes; não foi formatado. A suíte host passou
90/90, incluindo os testes negativos do diagnóstico e de seu validador;
os dois testes SD também passaram com ASan/UBSan. Capturas e imagem estão em
`maia-rtos-recovery/native-sdmmc-state-20261120/`; detalhes e limites em
[evidências](../hardware/EVIDENCE.md).

## F2 diagnóstico SDMMC CMD0/CMD8 — 2026-11-20

Uma sonda exclusiva de boot, carregada em RAM e separada do produto, configurou
o slot 0 em 1 bit, com clock nominal de 400 kHz. Em dois boots mornos, o
controlador concluiu `CMD0` e a resposta de `CMD8` foi `0x1aa`, sem erro
reportado pelo controlador. `CMD0` não possui resposta; `CMD8` sozinho não
identifica nem certifica um cartão SDHC. A primeira captura, anterior ao
handshake host–sonda, tinha o cabeçalho truncado e foi corretamente rejeitada
pelo validador, apesar da resposta observada. O handshake foi adicionado e
validado nas duas capturas posteriores.

O diagnóstico exige propriedade exclusiva do controlador, pinos, VO4 e
SYSTIMER UNIT0, além de core 0, interrupções desabilitadas e outro core parado.
Ele verifica VO4 já habilitado e em bypass, mas não mede tensão nem inicializa
a alimentação. A presença do cartão é forçada; não há detecção física.
Não houve leitura ou escrita de setores pela sonda, nem gravação de flash.
Após os testes, o firmware original montou `/sd`, sincronizou arquivos e
carregou o desktop/wallpaper; não houve nova comparação do hash da flash.

A suíte host passou 92/92; os três testes SD passaram com ASan/UBSan.
Referências, comandos, hashes e capturas estão em
[evidências](../hardware/EVIDENCE.md) e
`maia-rtos-recovery/native-sdmmc-cmd8-20261120/`.
Naquele checkpoint, `ACMD41`, identificação/CSD, seleção e leitura `CMD17`
permaneciam pendentes; o resultado da etapa seguinte está registrado abaixo.
DMA/coerência, escrita, FatFs, VFS e boot frio continuam pendentes.

## F2 diagnóstico SDHC e bloqueio PIO — 2026-11-20

Uma sonda somente de diagnóstico, carregada em RAM, avançou a inicialização
do cartão no Tab5: `ACMD41` retornou OCR `0xc0ff8000`, CID/RCA/CSD foram lidos,
RCA `0xaaaa` foi atribuído, a capacidade CSD calculada foi `124735488` blocos
e `CMD7`/`CMD13` confirmaram o estado selecionado/pronto. O `CMD17` do bloco 0
respondeu R1 `0x900`; `RINTSTS=0x0c` indicou `CMD_DONE|DATA_OVER`, sem bits de
erro do controlador.

A leitura PIO não foi validada. A instrumentação observou um FIFO inicialmente
com uma posição preenchida; após a primeira e a 128.ª leitura, o status ainda
mostrava contagem 1 e `FIFO_EMPTY=0`. Os 128 acessos a `BUFFIFO` retornaram
zeroes; após `DATA_OVER`, `STATUS=0x01008900` indicava contagem 128 e FIFO não
vazio. O bloco capturado não contém o marcador `0x55aa`; por isso não houve
`PASS`, aceitação dos dados nem comparação independente. Uma tentativa com
fences explícitas de I/O ao redor dos acessos não alterou a captura e foi
removida do código. A causa do FIFO/leitura ainda precisa ser identificada.

As capturas instrumentadas estão em
`maia-rtos-recovery/native-sdmmc-block0-20261120/probe-block0-retry2.log`,
`probe-block0-retry3.log` e `probe-block0-retry4.log`; a primeira captura
rejeitada por handshake permanece preservada. O runner carregou somente em
RAM e não emitiu comandos de escrita ao cartão ou à flash. Após cada execução
o runner resetou para o firmware original, que pode sincronizar arquivos em
`/sd`; isso não é evidência de escrita pela sonda.

Os três testes focados (`tab5_sdmmc_boot_test`, `sdmmc_cmd8_runner_test` e
`sdmmc_block0_runner_test`) e a suíte host completa (93/93) passaram; o teste C
SD também passou com ASan/UBSan (LeakSanitizer indisponível nesta plataforma).
O probe cruzado compilou, passou o gate do runtime nativo e foi validado na
janela L2MEM_LOW. Uma tentativa RAM-only baixou `RX_WMARK` de 511 para 0 e
habilitou somente RXDR no `INTMASK`; RXDR foi observado, mas 128 leituras ainda
retornaram zero e o `FIFO_COUNT` continuou inconsistente. A tentativa terminou
em erro de I/O, sem aceitar o setor. Outra tentativa passou a reconhecer RXDR
como W1C e capturou `CTRL=0`, `BMOD=0`, `TCBCNT=0x200` e `TBBCNT=0`; o status
imediatamente após o acknowledgement ainda continha RXDR e os dados/FIFO
permaneceram iguais. O detalhe e as capturas estão em
[evidências](../hardware/EVIDENCE.md).

Uma nova tentativa isolada aguardou `DATA_OVER` antes de qualquer leitura,
mantendo `RX_WMARK=511` e `INTMASK=0`. Mesmo assim, 128 leituras de 32 bits
retornaram zero e `FIFO_COUNT` permaneceu em 128 (`STATUS=0x01008900`) antes e
depois da drenagem. A disassembly confirma o acesso `lw` ao endereço
`0x50083200`; portanto, ler antes de `DATA_OVER` e um erro óbvio de endereço/
geração do acesso não explicam o bloqueio. O setor continua não validado.
Captura e binários preservados em
`maia-rtos-recovery/native-sdmmc-block0-dto-drain-20261120/`.

O TRM também exige configurar `CARDTHRCTL` para transferência de dados. A
sonda passou a limpar o enable e o threshold de leitura; o valor lido antes e
depois já era zero, e a transferência permaneceu idêntica (128 palavras zero,
contagem 128). Portanto, esse campo foi coberto conforme a especificação, mas
não explica o bloqueio. Captura em
`maia-rtos-recovery/native-sdmmc-block0-cardthrctl-20261120/`.

Permanecem pendentes a causa do acesso/count do FIFO PIO, a comparação
independente do bloco recebido por DMA, integração ao produto, escrita,
FatFs/VFS e power/cold boot. A leitura em múltiplas LBAs está registrada abaixo.
Ver também
[questões abertas](../hardware/OPEN_QUESTIONS.md).

## F2 — Sonda de leitura IDMAC — 2026-11-20

O caminho PIO permaneceu sem dados e não foi usado para declarar sucesso. Uma
sonda isolada agora usa o IDMAC interno, com descritor/buffer em reserva `NOLOAD`
fora de BSS/stack, alinhamento de 128 bytes, sincronização L1D pela ROM e acesso
à CPU pelo alias não-cacheável. Testes host cobrem a preparação do descritor,
conclusão, erros de DMA, timeout e falha de ownership; o runner valida estrutura,
capacidade conhecida e trailer MBR.

O probe/API agora aceita um LBA SDHC, rejeitando endereço acima de 32 bits e
fora da capacidade CSD antes de selecionar o cartão. No Tab5, leituras RAM-only
passaram em LBA 0 (duas vezes), LBA 1 e LBA 32768 (início da partição no MBR,
duas vezes). LBA 1 retornou um bloco de zeros; LBA 32768 retornou um setor de
boot FAT32 com `EB 58 90` e trailer `55aa`. O MBR e o BPB concordaram no início
e no tamanho da partição, e as duas capturas de LBA 32768 foram idênticas
(SHA-256 `ebdfb90905827cbc8e0cd6f8b851c51552e7915ff0e27810834fe03b6d92978a`).
Os testes host cobrem argumento, payloads, limite de capacidade e falha
sticky; o runner valida LBA, geometria e captura. Build RISC-V passou o gate
nativo/L2MEM_LOW e a imagem para LBA 32768 passou `--validate-only`. Imagens e
capturas estão em `maia-rtos-recovery/native-sdmmc-lba1-20261120/` e
`maia-rtos-recovery/native-sdmmc-lba32768-20261120/`.

Isso valida repetibilidade e coerência de leitura em múltiplos LBAs, não uma
comparação com leitor independente nem o driver de armazenamento. Ainda faltam
comparação externa, cobertura mais ampla, recuperação de erro/remoção,
integração ao MaiaRTOS e validação de power/cold boot. Nenhuma escrita no
cartão ou flash, formatação, FatFs ou VFS foi executada. Consulte
[evidências](../hardware/EVIDENCE.md) e
[questões abertas](../hardware/OPEN_QUESTIONS.md).

## F2 — Contadores IDMAC e backup FAT32 — 2026-11-20

A sonda passou a rejeitar uma transferência se os contadores do cartão
`TCBCNT` ou do DMA `TBBCNT` não forem exatamente 512, e só copia o bloco ao
relatório após o desligamento bem-sucedido do DMA. Os testes host cobrem 0,
511 e 513 bytes em cada contador, limpeza e falha sticky; o runner também
exige os dois contadores. No Tab5, o setor de backup FAT32 em LBA 32774,
indicado pelo BPB, coincidiu byte a byte com o setor principal em LBA 32768.
Uma nova leitura de LBA 0 também coincidiu com a captura anterior; ambas
passaram com `TCBCNT=TBBCNT=0x200`. Os builds e a validação RAM passaram.
Evidências em `maia-rtos-recovery/native-sdmmc-fat32-backup-20261120/` e
`maia-rtos-recovery/native-sdmmc-counts-lba0-20261120/`.

Os dois setores redundantes foram lidos pelo mesmo caminho, portanto não são
uma comparação com leitor independente. As leituras consecutivas no mesmo
boot estão qualificadas na etapa F3; falhas/remoção e integração continuam
pendentes. As sondas não escreveram ou formataram o cartão nem gravaram flash;
após cada reset o firmware legado pode sincronizar arquivos em `/sd`.

## F3 — Leituras CMD17 consecutivas — 2026-11-20

A sonda agora inicializa o cartão uma vez e lê LBA 32768 e LBA 32774 na mesma
execução. O teste físico no Tab5 passou; as duas transferências concluíram por
IDMAC, cada uma validada com `TCBCNT=TBBCNT=512`, e os buffers de 512 bytes
foram idênticos (SHA-256
`ebdfb90905827cbc8e0cd6f8b851c51552e7915ff0e27810834fe03b6d92978a`). O
runner schema 6 valida os dois LBAs e blocos e exige igualdade neste caso FAT32.
Testes host C e Python, build RISC-V, gate nativo/L2MEM_LOW e `--validate-only`
passaram.

Evidências e binários estão em
`maia-rtos-recovery/native-sdmmc-consecutive-20261120/`. O teste usou
`load_ram`, sem comandos de escrita no cartão/flash ou formatação; após a
captura o runner reiniciou o firmware preservado, cujo boot legado pode
sincronizar `/sd`. A referência externa de software está qualificada a seguir;
continuam pendentes cobertura mais ampla, erros/remoção e integração ao produto.
Escrita, FatFs e VFS continuam fora do escopo validado.

## F2 — Referência externa de software SDMMC — 2026-11-21

O exemplo do usuário em `../m5stack-tab5-examples/src/MicroSD/MicroSD.ino`
serviu de referência para um novo `MicroSDReference`, separado do MaiaRTOS.
Ele usa Arduino M5Stack 3.3.10/ESP-IDF v5.5.4 e `sdmmc_read_sectors`, com
slot 0 configurado em 4 bits/20 MHz e LDO4, sem montar FatFs, abrir arquivos,
escrever setores ou formatar o cartão. O firmware auxiliar foi compilado e
executado no Tab5, sem retirar o cartão ou teclado.

Os LBAs 0, 1, 32768 e 32774 coincidiram byte a byte com as capturas nativas:
2048 bytes comparados, capacidade de 124735488 setores de 512 bytes. A
validação verificou quantidade/ordem/offsets dos registros e três payloads
distintos, não apenas a igualdade dos setores FAT32 redundantes. Isso é
referência independente de software no mesmo hardware, não teste com outro
controlador ou validação de erro/remoção.

Antes da gravação autorizada, duas leituras completas da flash de 16 MiB
coincidiram. Após o teste, a flash foi integralmente restaurada e relida:
igualdade de todos os bytes com o backup antes do reboot. O reset RTS não
retomou o firmware legado; o reset watchdog oficial do esptool retomou o boot,
display/teclado e desktop. O usuário confirmou que desktop e teclado voltaram
a funcionar. O boot legado sincronizou arquivos em `/sd`; a ausência de
escrita aplica-se à referência SD bruta, não a esse boot legado.

Fonte, binários, dumps, comparação e logs de backup/restauração estão em
`maia-rtos-recovery/native-sdmmc-vendor-reference-20261120/`.
A referência contém runtime Arduino/ESP-IDF/FreeRTOS apenas como instrumento
externo de teste; nenhuma dessas dependências foi adicionada ao MaiaRTOS.
Ainda faltam cobertura mais ampla, erros/remoção e integração de leitura e
escrita ao produto. Nenhuma escrita, FatFs ou VFS nativos foi qualificada.

## F2 — Critério de conclusão do SDMMC — 2026-11-21

O objetivo desta etapa é um driver de blocos SDMMC nativo **com leitura e
escrita**, não apenas uma sonda de leitura. As leituras já verificadas são
uma etapa intermediária; não concluem o driver nem antecipam a integração
do FatFs.

Foi implementado um protótipo boot-only de escrita CMD24 para um bloco,
incluindo staging no alias não-cacheável, IDMAC TX, contadores exatos e
confirmação final por CMD13. O teste host cobre payload e falhas; o teste
físico de um bloco também passou no Tab5.

O firmware de referência identificou o cluster FAT32 97 como livre nas duas
FATs espelhadas e forneceu os bytes originais do primeiro setor, LBA 69318
(512 bytes, SHA-256 `32aefed26b737027376dbc961157ae63cfaf6f34577c39d6b8aed51e4700a3c0`).
Antes da escrita, o probe MaiaRTOS leu e comparou esse setor. Em boots separados,
CMD24 gravou o padrão temporário, CMD17 confirmou igualdade exata, CMD24
restaurou o setor original e CMD17 confirmou a restauração. O driver mantém
posse do controlador até reset; por isso, cada operação foi executada por um
boot RAM isolado. O cartão não foi formatado nem montado e nenhuma entrada FAT
foi modificada.

O runner capturou a flash completa duas vezes antes do teste; as capturas
coincidiram com SHA-256 `674ce7e2efe406637f1534388bf4f70136a55c82a82824977a32268916e02e7c`.
Após o teste, gravou e leu novamente a imagem integral; todos os 16777216 bytes
coincidiram com esse backup, e o boot restaurado alcançou MaiaRTOS/`msh>`.
Capturas, imagem de probe, setor original e relatório estão em
`../../../maia-rtos-recovery/native-sdmmc-write-20261121-valid-image/`.

Isso qualifica apenas a operação boot-only de um bloco e o procedimento
controlado de teste/restauração. Ainda faltam uma interface de blocos
reutilizável integrada ao produto, múltiplos setores, erros/timeouts/remoção,
concorrência e validação mais ampla. FatFs e VFS permanecem pendentes; não
formatar automaticamente.

## F2 — Contrato de requisições multi-setor — 2026-11-21

`maia_sd_block_device` agora oferece uma interface síncrona C para leitura e
escrita de um ou mais blocos de 512 bytes, com geometria limitada a `2^32`
blocos, validação do intervalo antes de I/O, status `BUSY`/`NO_MEDIA` e contagem
de blocos completos mesmo quando o transporte retorna erro. Um callback recebe
a requisição inteira e deve serializar seu controlador; a camada converte
sucesso com transferência parcial ou contagem inválida em `MAIA_SD_IO_ERROR`.
Não há alocação implícita.

O mock exercita leitura/escrita de três blocos, último endereço SDHC,
requisições fora de capacidade, contagem zero, timeout com progresso parcial,
ausência de mídia, sucesso incompleto e progresso acima do pedido. `sd_read_test`
e `tab5_sdmmc_boot_test` passaram; `sd_read_test` também passou com ASan/UBSan.
O cabeçalho passou compilação C++98 e o alvo P4 compilou a nova unidade.

Naquele marco, o contrato ainda não tinha consumidor de hardware: nenhum
callback SDMMC de runtime estava conectado, e o probe reconfigurava o SYSTIMER
e usava hooks DMA/cache exclusivos do linker de diagnóstico. O addendum a
seguir registra a implementação posterior do backend no produto; operação
física multi-setor, recuperação e FatFs/VFS continuam pendentes.

## F2 — Backend SDMMC de runtime — 2026-11-21

O `sdmmc_boot.c` agora também fornece o serviço de blocos SDHC/SDXC síncrono:
inicializa e seleciona o cartão antes do scheduler, publica um dispositivo
estável, executa requisições de um ou mais setores em laços CMD17/CMD24 e
propaga progresso parcial. Uma flag protegida por exclusão local de IRQ impede
acesso concorrente ao host/DMA; chamadas sobrepostas retornam `MAIA_SD_BUSY`.
Falhas físicas após iniciar a transferência deixam o driver sticky até reset.

O produto reserva os 640 bytes DMA alinhados a 128 bytes do heap interno já
existente, sem uma nova seção SRAM no linker. O backend usa o alias não-cacheável
para descritor/dados e um wrapper SoC para sincronizar somente a faixa L1D.
Durante a inicialização boot-only, o driver ainda prepara UNIT0 antes do timer
do kernel; após a posse do timer, as esperas usam `mcycle`, sem reconfigurar ou
consultar SYSTIMER UNIT0. A falha de alocação/cartão é registrada e não impede
o boot. Quando disponível, uma tarefa do shell faz uma leitura diagnóstica de
LBA0.

O pinmap da Tab5 mantém `DET=NC`, portanto o serviço não pode distinguir cartão
ausente/removido de outras falhas de comunicação nem emitir `NO_MEDIA` com
confiança. Não houve acesso físico ao cartão nesta etapa; a leitura de runtime
do produto e escrita multi-setor no device continuam pendentes.

Validação: `tab5_sdmmc_boot_test` e `sd_read_test` passaram (2/2); ambos
passaram com ASan/UBSan (2/2); a suíte host completa passou (93/93). O alvo
P4 `maia_m0.elf` e a imagem XIP passaram; a imagem tem 814.096 bytes, cinco
segmentos e checksum/hash válidos. O mapa confirma BSS até `0x4FF1E060`,
stack até `0x4FF22060` e assertions de SRAM aprovados. Nenhuma imagem foi
gravada no device.

## F2 — Qualificação física do serviço SDMMC de runtime — 2026-11-21

O probe `MAIA_NATIVE_PROBE_SDMMC_RUNTIME` inicializou o serviço antes do
scheduler e executou as operações pela API pública em uma tarefa MaiaRTOS,
com SYSTIMER ativo. Leu dois setores contíguos a partir do LBA 69318; depois
escreveu e releu um padrão temporário nos mesmos dois setores. A requisição
reportou dois blocos completos; o backend usa CMD24 por setor e confirma
READY/TRAN antes de contar cada gravação.

O setor inicial era o primeiro setor do cluster FAT32 97, verificado livre nas
duas FATs. Os 1024 bytes originais foram guardados antes da escrita; o primeiro
setor coincidiu com a referência independente já preservada. O runner restaurou
os dois setores e confirmou leitura byte a byte; a restauração passou na
primeira tentativa. Nenhum filesystem foi montado.

O runner capturou a flash completa duas vezes antes do teste e as duas cópias
coincidiram com a imagem de restauração. Após o probe, restaurou e releu os
16 MiB integralmente; hash esperado e obtido:
`674ce7e2efe406637f1534388bf4f70136a55c82a82824977a32268916e02e7c`. A captura
serial do boot restaurado chegou a `msh>`. Relatório, backup dos setores,
padrão e capturas estão em
`maia-rtos-recovery/native-sdmmc-runtime-20261121/`.

O probe foi carregado apenas em SRAM e validado dentro da janela
`0x4FF00000..0x4FF28000`; não foi gravado na flash. O parser do runner e a suíte
host passaram, 94/94. Esta evidência qualifica leitura e escrita multi-setor
do serviço em contexto de tarefa, mas não o boot do produto nem sua leitura
diagnóstica de LBA0. Permanecem pendentes fault injection, estresse/
concorrência, recuperação após perda de energia, FatFs e VFS.

## F2 — Boot do produto e leitura runtime de LBA0 — 2026-11-21

O build atual do produto XIP gerou imagem ESP32-P4 de 814.096 bytes, cinco
segmentos, entry `0x40000020`, checksum/hash válidos; SHA-256
`bf889774e0e2b5a11a12792f3f5c8c70e6de54f84a6ac5d1236d97519ba34659`.
Antes do teste, a leitura de 16 MiB da flash coincidiu com o backup verificado
anteriormente (`674ce7e2efe406637f1534388bf4f70136a55c82a82824977a32268916e02e7c`).

O runner salvou e validou o app de fábrica, gravou temporariamente o produto em
`0x10000` e capturou o boot. O serviço anunciou prontidão; a tarefa do shell
registrou `Runtime SDMMC read LBA0 passed`, executou o self-test e chegou a
`msh>`, sem fatal. A operação no cartão foi somente leitura. O runner restaurou
o app original e confirmou o hash; uma nova captura integral da flash coincidiu
byte a byte com a captura anterior ao teste, com o mesmo SHA-256.

O teste C host agora também provoca uma requisição sobreposta durante uma
transferência mockada: o callback retorna `MAIA_SD_BUSY` sem novo acesso a
MMIO. O teste focado passou com ASan/UBSan e a suíte host completa passou
94/94. O boot após falha de inicialização e fault injection físico continuam
pendentes.

Imagem, capturas, backup do app, logs e relatório estão em
`maia-rtos-recovery/native-product-sdmmc-lba0-20261121/`. A falha de
inicialização do SDMMC não foi induzida; remoção física não é detectável
(`DET=NC`). Permanecem fault injection, concorrência/estresse, recuperação após
perda de energia, FatFs e VFS.

## F2 — Boot não fatal com falha SDMMC simulada — 2026-11-21

O perfil de diagnóstico explícito `MAIA_DIAGNOSTIC_SDMMC_CMD8_TIMEOUT=ON`
injeta um retorno de timeout **em software depois da conclusão real do CMD8**.
O código não envia comandos de dados ou grava setores nesse perfil. O objetivo
é testar a propagação do erro após a aquisição do controlador, e não reproduzir
um timeout elétrico real, um cartão ausente ou todos os estados de falha.
O perfil é desligado por padrão e anuncia a injeção no console.

No host, o teste dedicado confirmou `MAIA_SDMMC_BOOT_TIMEOUT`, estágio CMD0,
ausência de dispositivo publicado e recusa sticky de nova inicialização sem
mais escritas MMIO. Ambos os testes SDMMC passaram (2/2), inclusive com
ASan/UBSan; a suíte host passou 95/95. O build normal, sem diagnóstico, gerou
imagem idêntica àquela validada anteriormente (SHA-256
`bf889774e0e2b5a11a12792f3f5c8c70e6de54f84a6ac5d1236d97519ba34659`).

No Tab5, o produto XIP diagnóstico registrou injeção, `error=-7, stage=6`,
continuou o boot e chegou a `qtest PASS` e `msh>` sem fatal. O runner restaurou
o app original, cujo boot foi capturado; as leituras completas de 16 MiB antes
e depois coincidiram byte a byte, SHA-256
`674ce7e2efe406637f1534388bf4f70136a55c82a82824977a32268916e02e7c`.
Os logs, imagem e backups estão em
`maia-rtos-recovery/native-product-sdmmc-cmd8-fault-20261121/`. Falhas reais
de transporte, estresse e recuperação após perda de energia permanecem abertos.

## F2 — FatFs nativo sobre SDMMC — 2026-11-21

FatFs R0.15 com patch2 foi integrado a partir de
`components/fatfs/src` do ESP-IDF commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`; os avisos/licença de ChaN foram
mantidos. Só o core portátil foi importado. A configuração Maia usa volume
único de setores de 512 bytes, FAT12/16/32, nomes longos UTF-8 e não habilita
exFAT nem `f_mkfs`. Nenhum glue VFS, FreeRTOS ou ESP-IDF foi incluído.

O `diskio` próprio adapta as chamadas síncronas ao dispositivo de blocos Maia.
O produto monta a unidade em contexto de tarefa e continua o boot em caso de
falha; montagem nunca grava ou formata. `FF_FS_REENTRANT=0`: montagem e todas
as operações FatFs devem ser serializadas. Essa serialização passou a ser
fornecida pela API MaiaVFS descrita na seção seguinte; mount/unmount continuam
restritos a contexto quiescente.

No host, o teste usa volume FAT12 sintético em RAM: montagem de mídia inválida
não emitiu escrita; a montagem válida não emitiu escrita; criação, atualização,
leitura/validação e remoção de arquivo passaram. `FA_CREATE_NEW` em nome já
existente retornou `FR_EXIST` sem escrita. `fatfs_test` passou com ASan/UBSan,
e o CTest completo passou 96/96. Os builds P4 normal e diagnóstico geraram
imagens XIP válidas de 816.864 e 831.840 bytes, respectivamente.

No Tab5, o diagnóstico default-OFF montou o cartão e registrou
`MAIA_FATFS_RW_PASS`: criou um nome longo com `FA_CREATE_NEW`, escreveu e
sincronizou 700 bytes, releu/comparou, atualizou um byte, validou novamente e
removeu o arquivo. Portanto, não sobrescreveu um arquivo existente e não
formatou o cartão. A captura contém também a leitura SDMMC LBA0 e a montagem
FatFs; o runner restaurou o app de fábrica e confirmou a escrita pela hash
(`08cee753...`). O boot restaurado voltou a montar `/sd`.
O teste removeu o arquivo temporário, mas atualizou metadados FAT e não fez
restauração byte a byte da mídia completa.

Capturas, imagem de diagnóstico, backup do app e relatório estão em
`maia-rtos-recovery/fatfs-tab5-20261121/`; a imagem de diagnóstico tem SHA-256
`a98fbe45dccca135bc3c4e651d69c6171a7bc5c38eaf2b006ba6952957440d83`.
Não foi feita comparação da flash integral nesta execução. Permanecem pendentes
estresse multi-tarefa, falhas reais de transporte, recuperação após perda de
energia e transferência de arquivos por `maia.py`.

## F2 — API MaiaVFS de arquivos em `/sd` — 2026-11-21

Foi adicionada a biblioteca `maia_vfs`, acima do FatFs Maia, com `open`, `read`,
`write`, `seek`, `sync`, `close` e `unlink`. O mount único é `/sd`; caminhos
fora dele, componentes `.`/`..`, separadores alternativos e `:` são rejeitados.
`CREATE` preserva arquivos existentes, `CREATE_NEW` não sobrescreve,
`TRUNCATE` exige `CREATE`, e `APPEND` reposiciona cada escrita ao fim. O serviço
usa oito handles estáticos com geração, sem heap; `FF_FS_LOCK=8` impede conflitos
de compartilhamento compatíveis com essa capacidade.

Como `FF_FS_REENTRANT=0` e o kernel oferece mutex não bloqueante, cada operação
VFS obtém `maia_mutex_trylock` e retorna `MAIA_VFS_BUSY` na contenção. Não há
polling, yield repetido ou espera em ISR. As chamadas são exclusivas a contexto
de tarefa; mount/unmount exigem quiescência. EOF retorna leitura bem-sucedida de
zero bytes; transferências parciais são reportadas; `close` com falha mantém o
handle válido para nova tentativa. Listagem de diretórios e adaptação ao
shell/Newlib não fazem parte desta unidade.

O teste FAT12 sintético em RAM cobre rota `/sd`, rejeição de caminhos inválidos,
mount ausente, busy/contexto, create-new, preservação de arquivo existente,
leitura, atualização, append, unlink, handles expirados/limite de oito, conflito
de compartilhamento e recuperação de `close/sync` após erro de escrita. CTest
completo passou **96/96**; `fatfs_test` passou com ASan/UBSan; headers públicos
compilaram em C++98. Builds P4 e empacotamento/validação da imagem passaram:
normal, 818.704 bytes, SHA-256
`b4eb03d4e068185888d169d240e0eab1c7b167989aa6317f63d9721ce443a1a3`;
diagnóstico, 838.880 bytes, SHA-256
`d233e1640fd5830158e245ea9307fd1b702e07a4485e740e4e1f3acb154a3e1b`.

No Tab5, o diagnóstico default-OFF chamou a nova API: `CREATE_NEW`, escrita e
sync de 700 bytes, leitura/comparação, atualização de um byte, nova verificação
e remoção; registrou `MAIA_VFS_RW_PASS`, sem marcador fatal. O runner restaurou
o app de fábrica e confirmou a hash
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`; o boot
restaurado voltou a `msh>`. O arquivo temporário foi removido, mas metadados FAT
foram atualizados; não houve restauração byte a byte do cartão nem comparação
integral da flash.

Logs e relatório: `maia-rtos-recovery/maia-vfs-tab5-final-20261121/`.
Concorrência real/estresse, falha física de transporte, recuperação de energia,
diretórios e integração com consumidores permanecem pendentes.

## F2 — SD no shell nativo/LCD, sem desktop — 2026-11-21

O MaiaVFS agora fornece stat, snapshot de posição/tamanho, mkdir/rmdir e
opendir/readdir/closedir, com quatro handles de diretório fixos. O mesmo mutex
try-only serializa arquivos e diretórios. O adaptador
`runtime/newlib_vfs.c` liga os descritores 3–10 e diretórios da Newlib ao VFS;
stdin/stdout/stderr mantêm o console existente. O adaptador usa memória estática
e trava na ordem adaptador → VFS; não há espera ativa para conter acesso ao SD.
O produto XIP inclui essa integração normalmente. A opção default-OFF
`MAIA_DIAGNOSTIC_VFS_SHELL=ON` apenas inicia em `/sd` e mostra instruções;
não inicia desktop, não escreve automaticamente no SD e exclui o diagnóstico
automático `MAIA_DIAGNOSTIC_FATFS_RW`.

Comandos existentes `cd/pwd/ls`, `cat`, `echo >`/`>>`, `mkdir/rm` funcionam sobre
essa camada; `rmdir` foi acrescentado para limpar diretórios vazios. Falhas de
leitura/listagem/redirecionamento e close são reportadas com status não zero.
Não houve mudança na gramática nem nos parsers gerados.

Duas falhas reais foram corrigidas:

- Newlib `__sfp` reutiliza FILE sem zerar o ponteiro de lock liberado por
  `_fclose_r`. A inicialização Maia agora aloca um novo lock dinâmico em vez de
  reutilizar memória liberada. O teste host reproduziu SIGTRAP antes da
  correção e passou depois, com oito ciclos init/acquire/release/close.
- O shell reservava 64 KiB para redirecionamento num heap interno de 32 KiB;
  em OOM imprimia na tela e retornava sucesso sem arquivo. O staging compartilhado
  com pipes começa em 1 KiB, cresce até o limite anterior de 64 KiB e reporta
  OOM/limite sem abrir/truncar o destino. Não é redirecionamento streaming.

Validação final: CTest 96/96; ASan/UBSan nos quatro testes de VFS/adaptador,
locks, heap e shell; headers públicos C++98; build e validação de ambas as
imagens P4 (5 segmentos, entrada `0x40000020`):

- normal: 847.072 bytes, SHA-256
  `dbc358c040b135e32d62b219825a1353ef6d1cef848e59db8f38319f54f0e73c`;
- POC: 847.152 bytes, SHA-256
  `2b8f37660fe0a8fc901ad9e390b335bddde4992aad4bbd673efe6ff5b863f284`.

O teste físico criou um diretório exclusivo, gravou `NATIVE_SD\n`, acrescentou
`APPEND\n`, leu pelo `cat`, listou `poc.txt (17 bytes)` e removeu arquivo e
diretório sem erro. Evidência: `maia-rtos-recovery/vfs-shell-rw-20261121/`.
O runner restaurou/verificou o app anterior pela hash habitual
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Na janela visual de 120 s com a imagem final, o dono respondeu **“Funcionou.”**
Registro: `maia-rtos-recovery/vfs-shell-visual-20261121/`.
O relatório final confirmou `probe_pass=true`, ausência de fatal e
`restore_verified=true`; o boot restaurado chegou a `msh>`. A captura contém
`ls`, `ps`, `pwd` e `ls lib` interativos após o demo automático. `ls ..`/`ls ../`
ainda são rejeitados pelo confinamento de caminhos (sem normalização POSIX).

Limites: oito arquivos e quatro diretórios (também limitados pelos oito share
locks FatFs), caminhos confinados a `/sd`, offsets/metadata Newlib até
`INT32_MAX`; `O_TRUNC` sem `O_CREAT`, criação somente-leitura e flags adicionais
retornam `ENOTSUP`. Não há rename, permissões/timestamps POSIX, POSIX completo
nem qualificação dos comandos de compilador/apps. Um close VFS com erro mantém
o descritor para retry; quem usa `fclose` deve guardar o fd antes do close se
precisar recuperar esse erro. A recuperação automática no shell ainda não
foi qualificada. Sem formatação; metadados FAT mudaram, sem restauração integral
do cartão/flash. O estresse multi-task de SD e a recuperação de falhas de diskio injetadas
foram validados no Tab5 em 21/11; falhas físicas de transporte/remoção e perda
de energia continuam pendentes. O desktop ainda é trabalho separado.

## Estresse multi-task e recovery MaiaVFS no Tab5 — 2026-11-21

Dois workers MaiaRTOS completaram 16 ciclos cada de escrita, sync, leitura,
comparação de 512 bytes e verificação de EOF; contenção `MAIA_VFS_BUSY` tem
retry limitado a 5.000 tentativas, com yield de um tick. O total observado foi
6.173 retries. O teste também injetou falha única de `disk_write` no sync e
confirmou retry bem-sucedido; depois injetou falha de `disk_read`, fechou o
objeto FatFs com erro sticky, reabriu e comparou os dados. Os arquivos e o
diretório exclusivos foram removidos ao final; nenhum formato foi executado.

Validação: CTest 96/96, `fatfs_test` direcionado com ASan/UBSan, build P4
normal e build diagnóstico. O runner de recovery registrou
`MAIA_VFS_STRESS_WORKERS_PASS`, `MAIA_VFS_FAULT_RECOVERY_PASS` e
`MAIA_VFS_STRESS_PASS`, sem fatal; `restore_verified=true` e hash do app de
fábrica `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
O boot restaurado carregou o desktop. Logs:
`maia-rtos-recovery/native-vfs-stress-20261121-final/`.

Limites: falhas são simuladas antes dos callbacks diskio, não representam
falha real do cartão, remoção, perda de energia ou corrupção de mídia. A carga
é concorrência de tarefas em single-core, não SMP. Diretórios de tentativas
diagnósticas anteriores permanecem no cartão para inspeção; não houve formato
nem restauração integral do cartão. Próximo passo no armazenamento: qualificar
falhas de transporte e persistência sem remoção física do cartão; testes
físicos e de perda de energia continuam sem qualificação.

Como preparação não destrutiva, `fatfs_test` sincroniza um arquivo de 512
bytes, desmonta/remonta o FatFs no mesmo disco mock em RAM e compara os dados;
o teste passou normalmente e com ASan/UBSan.

Em seguida, no Tab5, o diagnóstico escreveu, sincronizou e fechou um registro
de 512 bytes, aguardou um reset morno limpo por RTS e, no boot seguinte,
reabriu o arquivo e comparou todos os bytes e EOF. Os markers
`MAIA_VFS_REBOOT_ARMED` e `MAIA_VFS_REBOOT_PERSIST_PASS` passaram, assim como
o estresse e a recuperação de falhas injetadas em ambos os boots. O runner fez
duas leituras idênticas da região de app de 3 MiB, restaurou-a e verificou o
conteúdo byte a byte; o boot restaurado emitiu 25.735 bytes e iniciou o
MaiaOS Desktop (`desktop.mobj`). Evidências:
`maia-rtos-recovery/native-vfs-reboot-clean-20261121/`. O teste não cortou
energia nem removeu o cartão; o arquivo
`/sd/MaiaVFSReboot-20261121/persist.bin` foi mantido no cartão para inspeção.
Cold boot, falhas reais de transporte/remoção e perda de energia continuam
pendentes.

## Falhas SD simuladas sem remover o cartão — 2026-11-21

O proprietário informou que a remoção física não é viável com o teclado
instalado; ela fica fora do plano atual e não é considerada testada. No
pinmap do Tab5, DET é NC: o serviço não distingue remoção de outras falhas de
transporte nem reporta `NO_MEDIA` para ela.

No host, o mock de MMIO agora simula timeout no segundo bloco tanto de leitura
quanto de escrita. Em ambos os casos, um bloco é confirmado e o serviço fica
sticky: as requisições posteriores de leitura e escrita retornam
`MAIA_SD_NOT_READY`, com zero blocos concluídos e sem novos acessos MMIO.
Um teste separado injeta `MAIA_SD_TIMEOUT` e `MAIA_SD_NOT_READY` nos callbacks
mock do dispositivo de blocos e verifica a propagação pelo diskio e MaiaVFS,
inclusive falhas de `sync`/`close`. O mock é restaurado manualmente para
limpeza; isso não demonstra recuperação do serviço real sem reboot. Os três
testes host focados e os dois relevantes com ASan/UBSan passaram.

Esses testes não simulam eletricamente a remoção nem comprovam o estado do
segundo bloco em uma falha física de escrita. Falhas reais de transporte,
cold boot e perda de energia continuam sem teste; a interrupção deliberada de
energia exige autorização específica e um plano de segurança.

## MaiaKV e guard nativo de SPI Flash

Implementado um serviço C de chave/valor com dois snapshots de 8 KiB na área
dedicada `0xC74000..0xC77FFF`. O banco novo só se torna ativo depois de gravar
o payload, o CRC e o marcador de commit; na inicialização, o serviço escolhe
a cópia válida mais recente e não apaga o banco anterior durante a atualização.
O driver Tab5 limita acesso a essa região e executa as chamadas ROM em uma
rotina residente em SRAM, com interrupções locais bloqueadas e caches L1/L2
desabilitados/restaurados. O core secundário permanece estacionado pelo entry
point atual.

Host CTest e ASan/UBSan cobriram round-trip, atualização, exclusão, corrupção e
falhas de operação durante o primeiro commit e atualizações subsequentes. Se
nenhuma cópia tem commit válido, a inicialização retorna `MAIA_KV_RECOVERED_EMPTY`
em vez de ocultar a condição. O ELF P4 linkou e a imagem foi empacotada/validada com cinco
segmentos; `flash_transaction` está em `0x4FF00000`. Na validação inicial,
nenhuma operação de erase/write física havia sido executada; o teste Tab5
posterior está registrado abaixo.
O perfil nativo registra os imports MaiaKV no ELF loader e expõe `maia.kv`
(`get`, `set`, `remove`, `contains` e `freeText`); o teste end-to-end do
compilador/emulador passou junto com os 32 casos anteriores (33/33).
O módulo já está disponível em `/sd/lib/maia/kv.ms`; a leitura integral no
Tab5 (1.259 bytes) conferiu com o fonte local, SHA-256
`2ac4734b6bb1cea998fa5b49f1f742d0dee04ab341f80dba388ad2a35e8e60e0`. Não foi
necessário sobrescrever o arquivo. Os testes físicos do backend e a
compilação/execução on-device do wrapper `maia.kv` foram concluídos.

Em 22/11, além do limite de reserva do vetor de tokens, foram compactados os
eventos/CST do parser, reduzidas as alocações temporárias do codegen e o
assembler passou a reter referências às linhas de origem entre os passes. A
heap interna do produto ficou em 44 KiB. `make -C maiascript test` passou
33/33 casos e os testes de lifetime do AST; `make -C assembler test` passou
21/21 testes de disassembly e 6/6 fontes de montagem.

Com a imagem instrumentada, `maiasc` compilou no Tab5 um fonte mínimo que
importa `maia.kv` para um ELF32 de 2.224 bytes. O runner protegido confirmou
`[SUCCESS] Compiled` e restaurou o aplicativo original com o SHA-256 esperado.
Depois de remover as instrumentações temporárias, a imagem limpa foi
reconstruída e também compilou um fonte equivalente que importa `maia.kv`,
gerando 2.236 bytes ELF32. O segundo probe limpou os arquivos temporários e
restaurou o aplicativo original.

Na tentativa inicial, `maiasc --run` compilou `i32 main(){return 42;}` mas o
loader não observou retorno. O mesmo ocorreu com um ELF C mínimo. Esse bloqueio
foi corrigido pela sincronização de caches descrita abaixo; a evidência antiga
foi mantida em `EVIDENCE.md`.

**Execução ELF nativa destravada.** A causa era coerência de cache: o loader
nativo só executava `fence.i`, enquanto a versão ESP-IDF chama
`esp_cache_msync` (writeback do L1 DCache e invalidação dos L1 ICaches). No P4
isso não é feito por `fence.i`, então a CPU buscava instruções obsoletas.
`maia_esp32p4_cache_sync_code()` agora chama `Cache_WriteBack_Addr(L1D)` e
`Cache_Invalidate_Addr(L1I0|L1I1)` da ROM (endereços do `esp32p4.rom.ld` do
ESP-IDF v5.4) antes de entrar no módulo. Um segundo defeito truncava o `stdout`
dos módulos: `_write` desistia na primeira vez em que o endpoint USB estava
ocupado; agora espera de forma limitada, como o console. No Tab5, `maiasc --run`
imprimiu `MAIA_ENTRY_REACHED` e retornou 42; o firmware original foi restaurado.

### MaiaKV no Tab5 — 2026-11-22

Com a imagem `1caec394e6ee787278e815177ebacb3417572b64b09dbdcf817474ec484da87a`,
o runner USB protegido executou os símbolos nativos `host_kv_*` registrados no
loader. `copilot_probe` foi consultada sem alteração e estava ausente. A chave
temporária `mkvp_221758` foi criada (`set` retornou 0), lida como `"ok"` depois
de uma reinicialização limpa e removida; `contains` e `get` confirmaram
`NOT_FOUND` imediatamente e depois de outro boot. O valor não ficou ativo na
loja após o teste. A operação de set/remove passou pelo erase e commit dos
bancos alternados; não foi feito teste de perda de energia nem de apagamento
seguro do conteúdo antigo do banco inativo.

Cada etapa restaurou o aplicativo original e verificou SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Os probes e módulos temporários no SD foram removidos. Relatórios:
`../../../maia-rtos-recovery/maia-kv-device-host-contains-20261122/`,
`../../../maia-rtos-recovery/maia-kv-device-set-20261122/`,
`../../../maia-rtos-recovery/maia-kv-device-get-20261122/`,
`../../../maia-rtos-recovery/maia-kv-device-remove-20261122/` e
`../../../maia-rtos-recovery/maia-kv-device-check-removed-20261122/`.

O primeiro probe de wrapper que chamava `kv.contains` parou durante o codegen.
No follow-up, a instrumentação mostrou que a saída de assembly já tinha 4.082
bytes e o heap não oferecia um bloco contíguo de 8 KiB para o crescimento do
`stringstream`; a pilha ainda tinha mais de 11 KiB de margem. Redistribuímos a
memória estática para um pool L2 de 49 KiB e uma pilha de shell de 16 KiB.
Com essa configuração, o probe original — `contains`, verificação `isError()`
e retorno `42 + r.value()` — compilou e executou no Tab5, retornando 42.

A verificação foi repetida com imagem sem instrumentação temporária:
`1d1f83ca2ed12e1fb969f1ff6e1a3e99aea8323e4fa2c3605afd4ab8f38b439c`.
`make -C maiascript test` passou 33/33 casos e os testes de lifetime/reparse
do AST; `make -C assembler test` passou 21/21 testes de disassembly e 6/6
fontes de montagem. O runner USB registrou `probe_pass=true`,
`restore_verified=true`, removeu os arquivos de probe do SD e restaurou o app
original com SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Relatório: `../../../maia-rtos-recovery/maia-kv-device-original-clean-heap49-20261122/`.
UQ-018 fica resolvida para esse wrapper; limites de módulos maiores continuam
pendentes.

## RX8130CE nativo — leitura somente-leitura — 2026-11-23

Implementado serviço de leitura do calendário RX8130CE em
`services/rtc_rx8130.c`, usando o I2C gerenciado compartilhado com o touch.
Valida BCD, limites de data/hora, weekday one-hot e VLF; propaga falhas de I2C
e mantém a saída sem alteração quando não há leitura válida. A shell do produto
XIP oferece `rtc`, sem comando de escrita. CTest focado passou (1/1), e
`maia_m0_image` compilou e passou a validação de estrutura da imagem (5
segmentos, 857.088 bytes; SHA-256
`99f38ba380db52d1f70b87f1a4959ca95b2718fce5ce33c0565bf9dbc69a92cc`).

No Tab5, o primeiro limite de captura foi curto para a inicialização completa;
o runner restaurou a app original e verificou o hash. Na repetição com janela
de 30 s, a shell aceitou `rtc` e recebeu os registradores
`47 45 16 0C 28 00 00`, flags `25`. O mês é zero e o weekday tem mais de um
bit, portanto o serviço recusou o calendário; VLF está clear. O teste não
escreveu no RTC, e o runner registrou `probe_pass=true`,
`restore_verified=true`, restaurando a app original com SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Capturas em `../../../maia-rtos-recovery/native-rtc-rx8130-raw-20261123/`.

O driver de leitura está exercitado no hardware, mas o calendário físico ainda
não é válido. Aguarda-se definir uma fonte/valor de horário antes de qualquer
rotina explícita de ajuste; ver UQ-019 e a seção RX8130CE em `EVIDENCE.md`.

## Teclado A164 no produto MaiaRTOS — 2026-11-23

Corrigida a identificação do item F3: a unidade Tab5 conectada usa o teclado
físico M5Stack A164 com coprocessador STM32F030 em `0x6D`; TCA8418 é somente
uma alternativa presente no caminho legado. O produto nativo detectou A164
firmware `0x01`. Na shell MaiaRTOS, comandos digitados no teclado físico
(`ls`, `pwd`) foram executados; `pwd` retornou `/`. O runner USB registrou
`probe_pass=true`, sem fatal, e restaurou a app original com hash verificado.
Setas ainda chegam como texto literal (`[A`, `[B`, `[D`) e não são atalhos de
edição/histórico da linha; isso permanece uma limitação de UX da shell.
Capturas em `../../../maia-rtos-recovery/native-keyboard-a164-live-20261123/`.

## INA226 em leitura somente-leitura — 2026-11-23

Implementados o serviço INA226, a ligação ao I2C compartilhado do produto Tab5
e o comando `power`. A implementação valida os IDs TI (`0x5449`/`0x2260`),
propaga erros I2C, interpreta os registradores assinados e não escreve no
dispositivo. O registrador Mask/Enable (`0x06`) é omitido porque sua leitura
pode limpar flags. Corrente e potência só são calculadas quando o modo é
contínuo; a conversão Tab5 deriva corrente/potência da leitura de tensão do
shunt nominal de `5 mΩ` indicado pela referência M5Stack, sem depender do
registrador CAL. Registradores e efeitos de leitura seguem o
[datasheet INA226](../hardware/datasheets/INA226.pdf).

`ina226_test` passou no host, incluindo identidade incorreta, contenção BUSY,
preservação da saída em falha, cálculo com CAL zero e ausência de escrita.
Imagem P4 e leitura USB passaram; a captura final retornou barramento de
`7987 mV`, shunt bruto `0`, modo contínuo, `CAL=0x0000` e corrente/potência
derivadas iguais a zero. Uma tentativa anterior observou shunt bruto `2`; os
testes foram sem carga conhecida, então a magnitude/direção ainda não está
qualificada. O runner confirmou `probe_pass=true`, `restore_verified=true` e
restaurou a app original com SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Capturas em `../../../maia-rtos-recovery/native-ina226-power-final-20261123/`
e `../../../maia-rtos-recovery/native-ina226-power-derived-20261123/`.

## Bateria Tab5 e contenção do I2C compartilhado — 2026-11-23

O comando `battery` lê a tensão monitorada pelo INA226 e o nível bruto do
sinal P6 do expansor `0x44`; ele não interpreta a polaridade de `CHG_STAT_LED`
nem estima percentual. O primeiro teste USB encontrou contenção do barramento
(`MAIA_I2C_BUSY`); os comandos de leitura I2C `battery`, `power` e `rtc` agora
cedem a tarefa e fazem no máximo três tentativas no total, somente nesse erro
transitório.

O serviço de touch preserva o último snapshot válido e não renova sua idade
quando encontra BUSY; a validade continua limitada pelo timeout existente.
O produto também não registra contenção BUSY como falha/invalidação, inclusive
antes da primeira amostra válida. `touch_input_test`, `tab5_battery_test`,
`pi4ioe5v6408_test`, `ina226_test`, `rtc_rx8130_test` e `i2c_bus_test` passaram.
No teste USB final, `battery` retornou `Monitor bus=7987 mV` e sinal `LOW`,
sem mensagem de falha do touch. O runner confirmou `probe_pass=true`,
`restore_verified=true` e hash original
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Capturas em
`../../../maia-rtos-recovery/native-tab5-battery-touch-busy-final-20261123/`.

## Brilho LEDC do Tab5 — 2026-11-23

No produto MaiaRTOS via USB, `backlight 50` selecionou o sinal LEDC em GPIO22
e a leitura somente-leitura de `LEDC_CH1_DUTY` (`0x500D301C`) retornou
`0x00001FF0`. `backlight 100` retornou o controle a GPIO; leituras mostraram o
bit 22 alto em `GPIO_OUT` (`0x500E0004`) e seletor `0x100` em
`GPIO_FUNC22_OUT_SEL` (`0x500E05B0`). O runner passou sem fatal e restaurou a
aplicação original com hash verificado. Isso confirma a programação do caminho
de saída, mas não substitui a confirmação visual do brilho pelo dono.
Capturas em `../../../maia-rtos-recovery/native-tab5-backlight-20261123/`.

Na varredura visual seguinte (`backlight 100`, `75`, `50`, `25`, `5`, `0`,
`100`, uma linha a cada 5 s via `--input-line-delay`), o dono confirmou:
**“Brilho funcionou.”** O runner restaurou a aplicação original com hash
verificado. Capturas em
`../../../maia-rtos-recovery/native-tab5-backlight-visual-20261123/`.

## Desligamento nativo do Tab5 — 2026-11-23

`boards/m5stack/tab5/power.c` porta `bsp_generate_poweroff_signal()` do BSP
M5Stack: lê o registrador de saída do PI4IOE5V6408 `0x44` e pulsa P4
alto/baixo três vezes (100 ms cada), preservando os demais bits, pelo barramento
System I2C compartilhado (retry curto em BUSY; em falha parcial, P4 volta a
baixo). O atraso é injetado, e `tab5_power_test` cobre sequência, BUSY e falha.
O comando shell `poweroff` apaga o backlight antes dos pulsos.

No Tab5, com o USB conectado, `poweroff` imprimiu `Powering off...`, a porta
USB sumiu e o dono confirmou o aparelho desligado. Após religar, a aplicação
original foi gravada manualmente do backup (hash
`08cee753…abc06` verificado) e o boot capturado mostrou o desktop. Capturas em
`../../../maia-rtos-recovery/native-tab5-poweroff-20261123/`.

## BMI270 nativo — identificação, configuração e leitura — 2026-11-23

O comando `imu id` confirmou CHIP_ID `0x24` no endereço `0x68`. Em seguida,
`imu init` carregou a configuração Bosch e habilitou acelerômetro/giroscópio a
200 Hz (±4 g / ±1000 dps); `imu read` retornou amostras brutas
`accel=(7593,5,-2916)`, `gyro=(3,-1,-3)` com o Tab5 parado. Os testes host
`tab5_bmi270_test`, `i2c_bus_test` e `i2c_bitbang_test` passaram. O build
`maia_m0_image` passou; o build global ainda encontra falhas de link em probes
não relacionados.

O runner terminou com `probe_pass=true` e `restore_verified=true`, restaurando
a aplicação original com SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Capturas em `../../../maia-rtos-recovery/native-tab5-bmi270-id-20261123/` e
`../../../maia-rtos-recovery/native-tab5-bmi270-clean-init-read-20261123/`.

## Desktop nativo: redução de pilha e abertura visual — 2026-11-25

O loader mantinha o escopo de compatibilidade legada mesmo para módulos que
importam apenas a API host v1. O desktop excedia a pilha shell durante uma
leitura SDMMC. O escopo legado passou a ser ativado somente quando a resolução
de imports encontra símbolos legados; o helper fica `noinline` para que o frame
de 848 bytes não seja incorporado ao caminho direto. A tabela de offsets e
endereços do ELF agora reutiliza o mesmo buffer de 256 bytes em fases
sucessivas. Os buffers de 256 bytes (configuração) e 512 bytes (manifest) do
desktop foram movidos da pilha para o BSS do módulo.

Uma primeira execução após a separação do escopo ainda reportou
`MAIA_STACK_FAULT` em `send_command` (`sp=0x4ff0cbd0`); o runner restaurou a
aplicação original e confirmou o hash. Após reduzir mais 256 bytes do frame do
loader, a execução final permaneceu ativa por 60 s sem `MAIA_STACK_FAULT` ou
`MAIA_NATIVE_FATAL`. O usuário confirmou o desktop visível e estável. O runner
removeu o módulo temporário e verificou a restauração da aplicação original
(SHA-256 `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`).

Validação: `legacy_host_compat_test`, `elf_loader_context_test` e
`elf_loader_context_no_tls_test` passaram (3/3); `maia_m0_image` foi reconstruída
sem warnings, com imagem estruturalmente válida de 889.216 bytes e cinco
segmentos; `git diff --check` passou. O módulo `desktop.mobj` foi regenerado e
o SHA-256 no manifesto atualizado para
`c6818372711b42925266ac5d3881e41c9a0a8c79ca7d063a5b2ab6ec1c21b7e4`.
O prólogo do loader no ELF final reserva 1.648 bytes para seu frame local; isso
não mede a margem livre total da pilha nem substitui medição de high-water.

Captura final: `../../../maia-rtos-recovery/native-tab5-desktop-native-visual-probe-20261125-05/`.
Falha anterior preservada em
`../../../maia-rtos-recovery/native-tab5-desktop-native-visual-probe-20261125-04/`.
Em um segundo probe USB, o desktop ficou ativo por 120 s sem fatal e o usuário
confirmou que tocou no Banner, que abriu e retornou ao desktop. O runner removeu
o módulo temporário e restaurou o app original com hash verificado. Isso valida
o fluxo de touch/abertura/retorno do Banner neste caminho de boot; atalhos,
outros aplicativos, boot frio e margem high-water continuam pendentes. Captura:
`../../../maia-rtos-recovery/native-tab5-desktop-interaction-probe-20261125-06/`.

A suíte host ampliada não ficou verde. O build completo não linkou
`product_touch_test` (faltam símbolos `maia_tab5_*_bind`) nem
`tab5_panel_reset_test` (falta `maia_i2c_bus_read8`). Com o build continuando
os demais alvos e o CTest excluindo `product_touch_test`, 107/109 testes
passaram: `tab5_panel_reset_test` não pôde rodar e `f0_flash_runner_test`
falhou com incompatibilidade de assinatura (`capture()` recebeu oito
argumentos, mas aceita no máximo sete). Os três testes focados do loader
passaram.
