# Checklist — MaiaRTOS no Tab5 (ESP32-P4) sem FreeRTOS

Acompanhamento do porte dos drivers e do runtime para o MaiaRTOS nativo.
Cada item só é marcado com evidência (teste no host ou no device) e o commit
correspondente. Detalhes técnicos e evidências ficam em [STATUS.md](STATUS.md).

Legenda: `[x]` concluído · `[ ]` pendente · 🔧 em andamento · 👀 precisa de
conferência do dono (visual, som ou toque)

Escopo: somente o P4. O ESP32-C6 continua com o firmware da Espressif até a F10;
a migração dele é um projeto posterior (ver final).

## Já concluído (antes deste plano)

- [x] Kernel nativo: preempção por timer, FPU, interrupções (CLIC)
- [x] Primitivas: mutex, semáforos, filas, notificações
- [x] Console USB Serial/JTAG nativo
- [x] I²C nativo e expansores PI4IOE
- [x] Alimentação (LDO/PMU) e clocks
- [x] PSRAM a 200 MHz e DMA
- [x] LCD MIPI-DSI nativo com quadros repetidos e imagem visível 👀 (candidato de refresh contínuo estável em repouso; piscada observada a cada tecla)
- [x] Touch ST7121 nativo 👀 (device com firmware 1; GT911 não validado neste caminho)
- [x] 77 testes de host passando (`e018c2c`)

## F0 — Firmware nativo gravado na flash

🔧 **Em andamento:** o build padrão do produto agora usa XIP e o serviço LCD
nativo com framebuffer PSRAM. O dono confirmou visualmente o shell (`msh>`) e
executou comandos na imagem de teste. A tela ciano dos candidatos anteriores
ocorreu no caminho parcial de vídeo, que não usava o serviço LCD e o framebuffer
aprovados. O dono lembrou que as barras também piscavam: a imagem e o toque
foram confirmados. O candidato de refresh contínuo com buffer único ficou
estável parado, mas piscava a cada tecla. No candidato com double buffering,
o dono confirmou ausência de piscada ao teclar. Após corrigir o preenchimento
da margem inferior, confirmou também ausência da faixa suja (19/11).
O desktop completo ainda não
foi integrado. A validação atual usa o estado morno da PSRAM;
boot frio e recuperação segura de falhas continuam pendentes.

- [x] Imagem nativa no formato do app ESP-IDF, inicializada pelo bootloader de 2º estágio (probe, 17/11)
- [x] Kernel/scheduler e duas tarefas executando por XIP, com preempção por timer real (probe, 17/11; imagem de produto pendente)
- [x] Base de linker flash-XIP testada no probe (IROM/DROM + estado em SRAM; PSRAM apenas como janela runtime, 18/11)
- [x] Closure de inicialização PSRAM do probe colocada em segmento SRAM; referências relocáveis a XIP bloqueadas no link e imagem aceita no device (reset morno, 18/11; rotinas PSRAM não foram executadas)
- [x] ELF completo linkado com o linker XIP experimental e imagem validada: IROM `0x40000020`, descriptor DROM `0x400A0020`, 5 segmentos (18/11)
- [x] Boot XIP da imagem experimental do produto, 3 tarefas, timer e shell self-test até `msh>`; backup/restauração verificados (um reset morno, fallback de display em SRAM; PSRAM, desktop nativo e tela sem validação visual, 18/11)
- [x] Regressão LCD + touch nativos com o código atual (`7a9f24f` + alterações locais): quatro cantos, arraste e soltura; 843 relatórios, 0 erros; confirmação visual do dono (RAM, PSRAM morna, 18/11)
- [x] Validar visualmente o caminho de display do produto com framebuffer PSRAM e serviço LCD nativo: sem piscada ao teclar e sem faixa suja inferior, confirmado pelo dono (19/11; reset morno)
  - [x] Preparar imagem só com shell usando o framebuffer PSRAM e serviço LCD nativos (testes de host OK, 18/11)
  - [x] Observar essa imagem no device: cabeçalho e `msh>` em fundo preto, comandos executados (18/11)
  - [x] Tornar esse o caminho de vídeo padrão do produto XIP e excluir o driver legado em bandas SRAM do alvo padrão (build/imagem XIP validada, 18/11)
  - [x] Revalidar visualmente o binário do build padrão após a reorganização: cabeçalho, `msh>` e comandos visíveis (18/11)
  - [x] Implementar rearmamento do DMA na IRQ de conclusão sem desativar DPI/DSI entre quadros; atualizações e parada pausam no limite do quadro e só liberam o framebuffer depois do drain (84/84 testes host)
  - [x] Gerar e validar a imagem XIP candidata (801.200 bytes, 5 segmentos, entry `0x40000020`)
  - [x] Fazer boot eletrônico até `msh>` sem fatal e restaurar o firmware original com hash verificado
  - [x] Observar o candidato de refresh contínuo por 60 s: estável em repouso, mas pisca a cada tecla pressionada
  - [x] Implementar candidato com double buffering: desenho no buffer traseiro e troca no limite do quadro (84/84 testes host; imagem XIP de 801.488 bytes validada)
  - [x] Observar no device o double buffering: dono confirmou ausência de piscada ao teclar; relatou faixa suja de aproximadamente 5 pixels embaixo (19/11; boot e restauração verificados)
  - [x] Corrigir preenchimento incompleto da margem inferior nos dois caminhos de terminal: 22 linhas cobriam 704 dos 720 pixels; teste de raster reproduziu o defeito e passou após a correção (85/85 testes host; nova imagem XIP validada)
  - [x] Confirmar visualmente a eliminação da faixa inferior na nova imagem: dono confirmou “Sem piscada e sem faixa suja” (19/11; boot e restauração verificados)
  - [x] Comparar com o driver original: o painel DPI usa 1 framebuffer de scanout (`num_fbs = 1`), mas o LVGL desenha em 2 buffers de PSRAM (`double_buffer = true`) e copia no flush sem pausar o vídeo/DMA
  - [ ] Medir intervalos de refresh no device; a observação anterior não determina a causa da piscada
  - [x] Integrar o serviço de touch no produto XIP: dono confirmou cantos, arraste, soltura, teclado e ausência de piscada/faixa; boot e restauração verificados (19/11; reset morno, ST7121 fw1)
- [x] Integrar no produto o refresh/framebuffer nativo (double buffering observado sem piscada ao teclar e sem faixa inferior; 19/11)
- [x] Integrar no produto o serviço de touch ST7121 fw1 (validação física limitada, 19/11); PSRAM a frio e handoff cache/MMU continuam pendentes
- [ ] Linker final do produto com seções críticas de inicialização em SRAM e integração PSRAM
- [x] Allocator multi-região — duas regiões SRAM exercitadas no probe XIP (18/11)
- [ ] Inicializar e registrar heap PSRAM após validar boot frio e mapeamento da CPU
- [ ] Inicialização nativa da PSRAM — probe RAM-only recusou o estado após reset normal (`BUSY`, estágio 0, 0 escritas de boot); repetição com a mesma imagem confirmou a recusa, tela preta e retorno ao desktop observado pelo dono, sem chegar ao framebuffer (18/11). Snapshot separado mostrou VO2 habilitado (`0x501151d0 & 0x100`), incompatível com a entrada ROM exigida. Não é teste a frio
  - [x] Instrumentar a recusa com snapshot no instante da chamada e bitmask dos guards; teste host 86/86 e captura de 20/11 identificaram `VO2_ENABLED` + `DIVIDER_NOT_XTAL` (`0x500`). A inicialização e o cold boot seguem pendentes.
- [ ] Inicialização do produto XIP após cache/MMU — cache bootstrap ainda só foi executado no probe integralmente residente em SRAM
- [ ] LCD, touch e console como serviços do kernel (fim das opções de teste encadeadas)
- [x] Gravação com verificação e restauração pelo `maia-rtos-recovery` (`tests/hardware/run_f0_flash_probe.py`)
- [ ] Boot a frio repetido no device (10×) sem falha

## F1 — Biblioteca C/C++ sobre o kernel

- [x] Mutex do kernel protege aquisição/liberação contra preempção local (host tests + compilação RV32, 18/11)
- [x] Backend retargetable de locks da Newlib com mutex do kernel (host test, compilação RV32 e contenção `std::mutex` no probe; link do produto pendente)
- [x] Entradas Newlib `malloc/free/calloc/realloc` direcionadas ao heap Maia (host tests, compilação RV32 e `calloc/realloc/free` no probe; link do produto pendente)
- [x] Smoke de runtime Newlib/gthread no Tab5: quatro boots por reset morno passaram; runner restaurou o app original e verificou o hash (sem display/PSRAM)
- [x] Syscalls/heap/tempo no device: libc `write`, `read` sem dados (`EAGAIN`), `clock_gettime`, `nanosleep`, `calloc/realloc/free`; leitura positiva não testada no device
- [x] `reent` por tarefa: `errno` isolado e ponteiros `__getreent()` distintos em dois workers; reciclagem/reinicialização de slot ainda pendente
- [x] Gthread no device: `std::mutex` com contenção, `pthread_key_t` isolado e destrutores ao retornar das tarefas; `std::thread` não testado
- [ ] Construtores estáticos e guardas de inicialização C++ no device
- [ ] TLS de compilador (`thread_local`/ELF TLS); as chaves pthread testadas não significam TLS ELF
- [ ] Link do produto com o runtime F1 atual e aceitação XIP completa; tentativa exploratória parou no guard SRAM de `maia_esp32p4_psram_cache_adopt`
- [ ] Auditoria final de símbolos FreeRTOS no ELF do produto (o scan sem FreeRTOS foi apenas no probe F1)

## F2 — Armazenamento

- [ ] Driver SDMMC host nativo
- [ ] Protocolo de cartão SD (inicialização, leitura, escrita)
- [ ] FatFs integrado
- [ ] VFS do Maia com `/sd`
- [ ] Configurações persistentes na flash
- [ ] Teste de estresse de leitura/escrita
- [ ] Transferência de arquivos pelo `maia.py`

## F3 — Periféricos da placa

- [ ] Teclado TCA8418 (teclas no console)
- [ ] RTC RX8130 (data e hora)
- [ ] INA226 (consumo)
- [ ] Bateria IP2326 (carga e nível)
- [ ] Brilho da tela (LEDC) 👀
- [ ] Desligamento
- [ ] Sensor de movimento BMI270

## F4 — Sistema utilizável sem FreeRTOS

- [ ] Shell completa sobre o kernel nativo
- [ ] Jobs e processos da shell
- [ ] Carregador `.mobj` (memória executável + sincronização de cache)
- [ ] Compilador MaiaScript no device
- [ ] MNM (serviços nativos) sobre o kernel
- [ ] Desktop 👀
- [ ] Apps: SciCalc (`2+3*4=14`), Alarm, Notes, Files, Term, Editor, Clock, Sysinfo, Banner 👀
- [ ] Suíte de testes do `maia.py` passando

## F9 — Segundo núcleo (SMP)

- [ ] Partida do core 1
- [ ] Interrupções entre processadores (IPI)
- [ ] Filas de execução por núcleo
- [ ] Spinlocks e revisão das seções críticas dos drivers
- [ ] Teste de estresse concorrente com drivers ativos

## F5 — Áudio

- [ ] I2S com DMA
- [ ] Codecs ES8388 e ES7210
- [ ] Amplificador NS4150B
- [ ] Beep e som de boot 👀
- [ ] Reprodução e gravação de WAV 👀
- [ ] MP3 (libhelix) 👀

## F6 — Pendrive USB

- [ ] TinyUSB host com OSAL do MaiaRTOS
- [ ] Classe MSC
- [ ] Montar, ler, escrever e desmontar pendrive

## F7 — Wi-Fi (C6 com firmware atual da Espressif)

- [ ] SDIO host nativo
- [ ] `esp_hosted` (lado host) sobre o kernel
- [ ] lwIP com `sys_arch` do Maia
- [ ] Busca de redes, conexão e status
- [ ] DHCP e DNS
- [ ] SNTP
- [ ] Servidor HTTP

## F8 — Câmera e vídeo

- [ ] SCCB e sensor
- [ ] MIPI CSI e ISP
- [ ] PPA
- [ ] JPEG por hardware
- [ ] Captura e visualização 👀
- [ ] Vídeo 👀

## F10 — Entrega final do P4

- [ ] Auditoria do ELF: nenhum código FreeRTOS ou runtime ESP-IDF
- [ ] Testes longos de estabilidade
- [ ] Documentação atualizada
- [ ] Build ESP-IDF aposentado
- [ ] Equivalência funcional com o firmware antigo confirmada 👀

## Projeto posterior — ESP32-C6

- [ ] Kernel portado para o C6 (RV32IMAC, sem FPU)
- [ ] Compilador MaiaScript com alvo sem FPU, acesso a registradores e interrupções
- [ ] SDIO slave nativo
- [ ] `esp_hosted` (lado slave) — parte em MaiaScript
- [ ] Tabela de adaptação do Wi-Fi (`wifi_osi_funcs_t`) em C
- [ ] Atualização por OTA mantida, com rollback para o firmware da Espressif
- [ ] Wi-Fi completo com o P4 sem alterações
