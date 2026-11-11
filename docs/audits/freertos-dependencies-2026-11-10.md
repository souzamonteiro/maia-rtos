# Auditoria de dependências de runtime — 2026-11-10

## Resultado e escopo

**A versão funcional usa FreeRTOS como runtime efetivo. O kernel nativo do
MaiaRTOS não faz parte do ELF instalado anteriormente.** Não se trata apenas
de includes esquecidos. Remover chamadas isoladas não satisfaz R-002 e pode
interromper funcionalidades que hoje dependem de tarefas e drivers do ESP-IDF.

Esta etapa realizou levantamento, comparação e builds isolados. Nenhum código
de execução, configuração, módulo, binário da versão funcional ou conteúdo do
dispositivo foi alterado. Não houve acesso à porta serial, reset ou flash.
Não foi demonstrada a causa das instabilidades anteriores.

Base funcional: commit `bf044d37e43c3b8db2011d6cbf81c9df2d2e1649`
(`Fixed text wrapping in MaiaSCript apps`), árvore inicialmente limpa.
Referência investigada: `../maia-rtos-temp`, com alterações locais preservadas.
O último relato do proprietário confirma a aparência dos dois aplicativos.
Os testes de hardware da etapa anterior estão em
[maiascript-panel-text.md](maiascript-panel-text.md); não foram repetidos aqui.

## Preservação da versão funcional

Foi criada uma cópia independente em:

```text
/Volumes/External_SSD/Documentos/Projects/maia-rtos-recovery/bf044d3-20261110/
```

`manifest.json` registra SHA-256 e tamanho do firmware, ELF, mapa, bootloader,
tabela de partições, parâmetros de flash, configurações e lock de dependências.
É uma cópia dos artefatos locais conhecidos, não uma leitura integral da flash
ou backup dos arquivos do cartão SD. Não execute flash como parte da auditoria.

| Artefato | SHA-256 |
|---|---|
| `m5stack_tab5.bin` | `21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd` |
| `m5stack_tab5.elf` | `f486955925d7fa85539d4f5b8381ea187a04c9d7bb5df37e175a7260f6f1e45c` |
| `m5stack_tab5.map` | `5b768d18c5becfbec61f9d98ad7075fff9cc98d912d90dd4b9f8a4fcb466679a` |

O build existente informa ESP-IDF `v5.4.2-dirty`, checkout em
`/Users/roberto/esp-idf`, HEAD `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`.
As modificações locais do SDK não foram auditadas integralmente nem copiadas.
Portanto, o commit do projeto sozinho não garante reprodução bit a bit.

## Método e limites do inventário

Foram cruzados os arquivos versionados, `compile_commands.json`, dependências
de includes emitidas pelo compilador, `project_description.json`, comando de
link, tabela de símbolos do ELF e seções retidas/descartadas do mapa.
Os arquivos e diffs relevantes da árvore temporária também foram inspecionados.

O [inventário por arquivo](freertos-source-inventory-2026-11-10.tsv) registra
linhas, includes, identificadores e participação no build. Comentários e strings
são excluídos das buscas de identificadores. A presença textual dentro de um
arquivo compilado não prova que aquele ramo passou pelo preprocessador ou link.
O inventário é uma busca por padrões conhecidos, não prova de ausência de código
renomeado, implementação copiada ou dependências escondidas em bibliotecas.

Dados da versão funcional:

- 1.757 entradas de compilação; 863 unidades de tradução dentro do projeto.
- 136 componentes nomeados no grafo do build; 66 fontes no componente `main`.
- 148 arquivos do projeto com referências textuais reconhecidas, incluindo
  headers e código fora do firmware final; 49 são unidades de tradução do build.
- 1.748 arquivos de dependências encontrados para os comandos atuais; 497
  incluem headers do FreeRTOS. Não foram usados depfiles antigos fora da lista
  atual. Nove comandos não têm o depfile correspondente disponível.
- 76 símbolos reconhecidos pelo filtro de tarefas/filas/semáforos/timers/port
  estão definidos no ELF. Incluem funções e dados, e não representam a totalidade
  do kernel FreeRTOS nem apenas APIs públicas.
- A tabela de referências cruzadas cita 47 arquivos `.a` consumidores desses
  símbolos. Somente 41 também têm membros com seções não vazias retidas no mapa;
  mesmo num membro retido, uma função específica pode ter sido descartada.
- Nenhuma fonte de `kernel/`, `arch/` ou `boards/` do projeto entra nesse build.

O coletor e o JSON completo foram preservados em `audit/` dentro da cópia de
recuperação, junto do `nm`, comando de link, descrições do build e logs.
Reprodução, sem compilar ou acessar o dispositivo:

```sh
python3 ../maia-rtos-recovery/bf044d3-20261110/audit/collect.py \
  --root . --out /tmp/maia-freertos-inventory-20261110.json \
  --nm /Users/roberto/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin/riscv32-esp-elf-nm
```

## Achados prioritários

**P0 — Inicialização e escalonamento pertencem ao FreeRTOS.**
`platforms/tab5/CMakeLists.txt:3` inclui `project.cmake` do ESP-IDF. O grafo
compila 15 fontes do componente FreeRTOS, das quais 11 membros de arquivo têm
seções retidas no mapa. No SDK local, `components/freertos/app_startup.c`
cria `main_task`, inicia `vTaskStartScheduler` e chama `app_main` nessa tarefa.
O ELF contém `esp_startup_start_app`, `vTaskStartScheduler`, `vTaskDelay`,
`xTaskCreatePinnedToCore`, `pxPortInitialiseStack`, `xPortStartScheduler` e
`vPortYield`, além das implementações de filas e bloqueios.

**P0 — O kernel nativo é outro produto de build, ainda sem equivalência.**
O CMake raiz seleciona o kernel próprio somente no alvo `Generic`.
`maia_sched_init`, `maia_task_create` e `_maia_trap_entry` não estão no ELF
funcional. `CONFIG_FREERTOS_NUMBER_OF_CORES=2`, o `main` configurado no core 0
e o teclado criado no core 1 explicam o uso atual dos núcleos por esse runtime.
Não há evidência nesta imagem de dois escalonadores, MaiaRTOS e FreeRTOS,
concorrendo simultaneamente. Não se deve converter essa hipótese em diagnóstico.

**P1 — Há dependências indiretas essenciais.**
Mesmo substituindo cada chamada visível na aplicação, drivers, libc/C++, rede,
eventos e armazenamento continuam requisitando serviços do runtime. Os símbolos
`esp_timer_get_time`, `esp_ipc_call_blocking`, `esp_crosscore_int_send_yield`,
`heap_caps_malloc`, `esp_psram_init`, `esp_cache_msync`, `pthread_mutex_lock`,
`_lock_acquire` e `_lock_acquire_recursive` permanecem no ELF.

**P1 — A cobertura de regressão geral da árvore atual está quebrada.**
Um build host novo falha nos contratos da shell: testes usam `MSH_O_READ`,
`MSH_O_WRITE`, `MSH_ERR_AGAIN` e outros nomes ausentes do header atual, que expõe
`MSH_O_RDONLY`, `MSH_O_WRONLY` etc. O build nativo raiz chega à shell C++ e falha
em `riscv_asm.hpp` não encontrado. São falhas reproduzidas antes de qualquer
alteração de runtime nesta etapa. Corrigir só o include não certificará o restante
da integração freestanding. Não alterar a ABI vigente para fazer testes antigos
passarem sem reconciliar o contrato e os consumidores.

**P2 — Documentação histórica contradiz os artefatos atuais.**
README, STATUS e ADR-010 descreviam estados diferentes. O ADR aceita um backend
do fabricante, mas não pode substituir a exigência R-002 de independência.
Os relatos da árvore temporária descrevem um firmware diferente. O seu texto
sobre a imagem flash com falha não descreve o dispositivo funcional atual.

## Mapa por subsistema

As linhas abaixo identificam código e mecanismos observados; não constituem
uma revalidação física de cada periférico.

| Área | Evidência principal | Trabalho necessário para independência |
|---|---|---|
| Boot, tarefas e núcleos | CMake Tab5; SDK `freertos/app_startup.c`; ELF e `sdkconfig` | Startup, contextos, timer, IRQ, escalonamento e IPI próprios, com testes independentes |
| Teclado, serial e execução de comandos | `main/app_main.cpp:1014`, `:3886`, `:3906` | Portar I2C/console e definir publicação/consumo dos eventos entre tarefas |
| Shell e jobs em background | `include/maia_shell/maia_shell.hpp:1635` | Preservar semântica de jobs, conclusão, cancelamento e propriedade dos buffers |
| Compilação MaiaScript no device | Mesmo header, `:2838` | Preservar a stack grande do worker, sinalização de conclusão e falhas de criação |
| ABI de módulos e tempo | `components/exec/elf_loader.c:76`; `app_main.cpp:3242` | Implementar sleep/relógio nativos, mantendo exports, convenções de chamada e erros |
| Memória executável, heap e cache | `elf_loader.c:208`; `heap_caps_*`, `esp_cache_msync` | Manter execução aninhada Desktop → App, alinhamento, permissões/capacidades e coerência |
| USB e armazenamento removível | `app_main.cpp:1212`, `:3132`; BSP `m5stack_tab5.c:1772` | Substituir filas, workers, callbacks, timeouts e o backend USB/VFS |
| SD/FAT e arquivos | `bsp_sdcard_init`, `esp_vfs_fat_sdmmc_mount`; mapa de FATFS, SDMMC e VFS | Drivers e sincronização próprios; preservar dados e comandos de transferência da CLI |
| Áudio e microfone | `app_main.cpp:1349`; codec `esp_codec_dev_os.c`; I2S no mapa | Portar bloqueios, espera de DMA/I/O, buffers e serviços de áudio |
| Wi-Fi e transporte | `esp_hosted/host/port/src/os_wrapper.c`; `sdio_wrapper.c`; lwIP | Portar tarefas, filas, mutexes, timers, eventos e SDIO; auditar firmware do coprocessador separadamente |
| HTTP, câmera e vídeo | Mutexes da aplicação; `esp_video.c`, `esp_video_isp_device.c`, `esp_video_isp_pipeline.c` | Proteger sessão/stream, cancelamento, DMA e callbacks de ISR; preservar contratos HTTP |
| Display e touch | `bsp_display_new_with_handles_to_st7123`, LCD MIPI DSI e ST7123 no ELF | Portar PSRAM/cache/DMA/I2C e sequências verificadas, preservando o framebuffer e a UI |
| libc, C++ e logs | `newlib/locks.c`, `log_lock.c`, `cxx_guards.cpp`, pthread no mapa | Dar suporte nativo a locks/TLS/reentrância usados pelas bibliotecas ou substituir os backends |

O ring de teclado (`app_main.cpp:622`) usa índices comuns, e a passagem de
comandos (`:116`, `:576`, `:3908`) usa flag `volatile` e buffer compartilhado.
Não foi encontrada sincronização explícita nessas estruturas. É um ponto de
revisão de concorrência antes de qualquer migração; esta inspeção não prova que
tenha causado a instabilidade relatada.

## Dependências compiladas, mas fora da imagem final

O LVGL local é **9.2.2**, embora o README da plataforma cite 8.4.0.
`CONFIG_LV_USE_OS=0`; o corpo de `src/osal/lv_freertos.c` é condicional.
Seu objeto não define símbolos. Não há símbolos `lv_*` ou `lvgl_port_*` no ELF,
nem seções retidas de `liblvgl.a` ou `liblvgl_port_lib.a` no mapa.

Os fontes de `vendor_app/` e `main/hal/` entram por glob no componente principal,
mas os únicos membros retidos de `libmain.a` são a aplicação, loader/validator,
assembler/disassembler, compilador e regex/glob: 11 membros. Os painéis do demo,
HAL duplicado e seu `TaskController_t` não têm seções retidas nesse ELF.
O player MP3 `chmorgan__esp-audio-player` também é compilado, mas sua biblioteca
não tem membros retidos. Isso não elimina a dependência ativa do áudio via codec.

Esses itens são candidatos a uma limpeza futura do conjunto de compilação,
verificada por comparação de símbolos/seções e testes do desktop. Não devem
ser apresentados como causas ativas apenas porque uma busca textual os encontra.
Uma futura aplicação pode reativá-los, portanto não se pode certificar o checkout
inteiro como independente só examinando esta imagem.

## Bibliotecas pré-compiladas e bootloader

O comando de link inclui `libesp_ipa.a`, `libtinyh264.a` e `libopenh264.a`.
Inspeção de imports dos arquivos completos, incluindo membros possivelmente
descartados, encontrou:

- IPA: `esp_timer_get_time` e serviços de log ESP-IDF, além de alocação.
- TinyH264: `heap_caps_*`, log ESP-IDF e alocação.
- OpenH264: `pthread_create/join`, mutexes, condições e alocação.

Isso exige revisão de proveniência e configuração antes de certificação. Não
significa que todos esses imports estejam ativos no firmware atual; por exemplo,
`pthread_create` não está definido no ELF final, mas `pthread_mutex_lock` está.
Ausência de nomes FreeRTOS não certifica a independência de um arquivo binário.

Não foram encontrados símbolos definidos `xTask*`, `vTask*`, `xQueue*`, `vQueue*`
ou `xSemaphore*` no ELF do bootloader, pelo filtro aplicado. O bootloader existente
continua sendo um artefato do build ESP-IDF; este resultado limitado não é uma
auditoria de proveniência nem aprovação do contrato completo de boot independente.
Não foi alterado. O firmware remoto usado por ESP-Hosted não foi lido nem testado.

## O que aproveitar de maia-rtos-temp

| Material | Decisão desta auditoria |
|---|---|
| `docs/AGENT_HANDOFF.md`, auditorias e logs | Aproveitar evidências e limites; pertencem ao console experimental, não à imagem atual |
| `scripts/check_runtime_contract.py` e testes negativos | Boa base de guardas; ampliar escopo para fontes realmente compiladas, arquivos `.a`, imagem e proveniência |
| Console nativo e ferramenta ROM/RAM | Úteis num alvo experimental isolado; não substituem shell, desktop, VFS nem execução de módulos |
| CMake que desabilita a plataforma Tab5 | Não transplantar: impediria reconstruir a versão funcional antes de existir substituta equivalente |
| Trocas de chamadas em `platforms/tab5/main/app_main.cpp` | Não transplantar: essa aplicação foi excluída do build temporário; as trocas não têm validação integrada |
| `maia_sched_poll` e tasks periódicas | Revisar isoladamente: callbacks executados no chamador não equivalem a tarefas preemptivas com contexto próprio |
| Mutexes, filas e waits adicionados | Não usar como substitutos SMP: acesso sem proteção e timeouts contados em iterações não demonstram equivalência |
| Patches de ESP-Hosted/SDIO | Tratar como mudanças separadas de retry/slots; não removem o runtime e precisam de regressão SD + rede |

O build temporário nativo usa RV32IMAC/ILP32, enquanto o firmware funcional usa
RV32IMAFC/ILP32F. O trap nativo inspecionado salva registradores inteiros, sem
salvar/restaurar registradores FP ou FCSR. Antes de migrar a calculadora e o
compilador, é necessário fechar ABI e preservação do contexto FP, sem tomar a
execução de um console inteiro como evidência de suporte a essas aplicações.

Os logs temporários registram `timer_irq=0`, display desabilitado após stall
e core secundário estacionado. Nesta etapa só foram repetidos testes do host
e builds: as limitações físicas continuam evidência histórica não revalidada.

## Verificações executadas nesta etapa

Todos os builds abaixo usam diretórios novos em `/tmp`, não os builds da versão
funcional. Ambiente: macOS arm64; compiladores locais; GCC RISC-V 14.2.0.

| Verificação | Resultado real |
|---|---|
| Coletor de fontes, manifesto, ELF e mapa | Concluído; inventário e JSON preservados |
| `python3 tests/host/test_maiascript_text.py` na versão atual | PASS: limites dos painéis, origens das linhas, branco/vazio/null |
| `./bin/test_compiler` em `maiascript/` atual | PASS: 32/32 |
| CMake host atual, build completo | FAIL: contrato dos testes da shell incompatível; CTest completo não executado |
| Alvo `maia_m0.elf` atual em build independente | FAIL: `riscv_asm.hpp` não encontrado |
| CMake/CTest host temporário | PASS: 8/8, incluindo casos negativos do verificador |
| Build temporário `maia_ram_elf`, display OFF | PASS; warnings RWX e segmentos vazios do objcopy permanecem |
| Verificador temporário contra fontes atuais selecionadas | FAIL esperado: 19 diagnósticos, alguns heurísticos, não 19 dependências distintas |
| Verificador temporário contra o ELF funcional atual | FAIL esperado; confirma incompatibilidade com o contrato de independência |
| Verificação de hashes e alterações do firmware funcional | Sem alteração |
| Hardware/serial nesta etapa | NOT RUN; dispositivo preservado |

Comandos usados a partir de cada checkout, com `current` ou `temp` no caminho
de saída correspondente:

```sh
cmake -S . -B /tmp/maia-freertos-current-host-20261110 -DMAIA_BUILD_HOST_TESTS=ON
cmake --build /tmp/maia-freertos-current-host-20261110 -j4
# O build atual falhou; CTest só foi executado no checkout temporário.
ctest --test-dir /tmp/maia-freertos-temp-host-20261110 --output-on-failure
cmake -S . -B /tmp/maia-freertos-current-native-20261110 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake
cmake --build /tmp/maia-freertos-current-native-20261110 --target maia_m0.elf -j4
# Em maia-rtos-temp:
cmake -S . -B /tmp/maia-freertos-temp-native-20261110 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake \
  -DMAIA_TAB5_ENABLE_DISPLAY=OFF
cmake --build /tmp/maia-freertos-temp-native-20261110 --target maia_ram_elf -j4
```

Logs `maia-freertos-*.log` estão em `audit/` na cópia de recuperação. Os comandos
do verificador usam o script da árvore temporária, explicitamente com `--root`
ou `--elf` da árvore que se deseja inspecionar. Seu escopo original exclui a
plataforma: passar nesse teste não significa que um desktop completo foi portado.

## Sequência de implementação recomendada

1. **Restabelecer verificações sem mudar o runtime.** Reconciliar testes com a ABI
   vigente da shell, preservar parser/semântica e acrescentar relatório de
   dependências por alvo. Testes de kernel independente devem rejeitar FreeRTOS;
   a imagem funcional deve ser marcada explicitamente como dependente. Não
   transformar a conformidade em simples exclusão de diretórios da busca.
2. **Separar o código portátil das ligações de plataforma.** Extrair uma fronteira
   mínima para tempo, alocação, I/O e execução de tarefas, preservando exports e
   comportamento. Não criar um shim FreeRTOS no kernel. Manter o build funcional
   reproduzível e o build nativo como alvo distinto durante a migração.
3. **Validar fundações nativas isoladamente.** Timer/IRQ com tarefa que não cede
   voluntariamente, contextos incluindo FP, memória executável, waits e IPC.
   Definir propriedade, concorrência, deadlines e erros antes de portar serviços.
   Não tratar o poll cooperativo temporário como preempção comprovada.
4. **Restaurar a experiência completa no alvo nativo.** Resolver PSRAM/cache/DMA,
   display, teclado/touch, armazenamento, shell integral e loader; validar o mesmo
   desktop, aplicativos e interface `maia.py`, sem trocar a UI por um console.
   Portar áudio/USB/câmera/rede com testes próprios e nenhuma importação do runtime.
5. **Validar SMP e a imagem final.** Após a correção single-core, verificar startup
   secundário, IPI, atomics, ordenação e estresse concorrente com drivers ativos.
   Auditar bootloader, bibliotecas e firmware remoto conforme o escopo acordado.
   Promover o firmware somente com equivalência funcional e evidências no Tab5.

Cada etapa deve ser pequena e ter critério próprio de aceitação. Para a imagem
substituta, a lista mínima inclui boot repetido, CLI, SD/transferência, desktop,
toque/teclado, compilação e módulos aninhados, `2+3*4=14`, funções FP, timer/alarm,
áudio, USB, câmera/rede, falhas de recursos e operação concorrente. Uma etapa
parcial não pode ser apresentada como substituta completa.

**Próxima alteração pequena:** recuperar os testes do contrato atual da shell e
definir a verificação por alvo, sem alterar a ABI pública nem o firmware ativo.
As remoções do runtime ainda não foram implementadas. Esta auditoria não afirma
garantias de tempo real, preempção nativa, SMP próprio ou independência concluída.
