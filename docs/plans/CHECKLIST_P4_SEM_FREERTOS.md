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
- [x] Linker XIP do produto com closure crítica da PSRAM em SRAM; endereço do `cache_adopt` verificado e imagem aceita (20/11; boot morno, não qualifica cold boot)
- [x] Allocator multi-região — duas regiões SRAM exercitadas no probe XIP (18/11)
- [ ] Inicializar e registrar heap PSRAM após validar boot frio e mapeamento da CPU
- [x] Inicialização nativa da PSRAM no produto XIP após reset morno: cache-adopt, clock/modo e window-sync retornaram 0; serviço LCD anunciou framebuffer PSRAM pronto (20/11). Cold boot e integridade de dados ainda pendentes.
  - [x] Probe RAM-only separado continua recusando aquele snapshot (`VO2_ENABLED` + `DIVIDER_NOT_XTAL`, `0x500`); guard preservado. Não substitui o resultado do caminho canônico do produto nem é teste a frio.
- [x] Boot do produto XIP após cache/MMU: serviços LCD e touch iniciados, self-test do shell (`qtest`) passou e chegou a `msh>`; um reset morno, backup/restauração verificados (20/11).
- [x] LCD, touch e console iniciaram como serviços do produto (20/11); observação visual e interação touch foram confirmadas em execuções posteriores.
- [x] Observação visual do produto XIP por 60 s: dono confirmou tela correta e estável, sem ciano, piscada ou artefatos (reset morno, 20/11)
- [x] Interação touch no produto XIP atual: dono confirmou cruz seguindo o dedo e sumindo ao soltar (reset morno, 20/11)
- [x] Gravação com verificação e restauração pelo `maia-rtos-recovery` (`tests/hardware/run_f0_flash_probe.py`)
- [ ] Boot a frio repetido no device (10×) sem falha

## F1 — Biblioteca C/C++ sobre o kernel

- [x] Mutex do kernel protege aquisição/liberação contra preempção local (host tests + compilação RV32, 18/11)
- [x] Backend retargetable de locks da Newlib com mutex do kernel (host test, compilação RV32, contenção `std::mutex` no probe e link do produto XIP, 20/11)
- [x] Entradas Newlib `malloc/free/calloc/realloc` direcionadas ao heap Maia (host tests, compilação RV32, chamadas no probe e link do produto XIP, 20/11)
- [x] Smoke de runtime Newlib/gthread no Tab5: quatro boots por reset morno passaram; runner restaurou o app original e verificou o hash (sem display/PSRAM)
- [x] Syscalls/heap/tempo no device: `write`, `read` vazio (`EAGAIN`) e positivo (4 bytes `Maia` por USB Serial/JTAG, 4 boots), `clock_gettime`, `nanosleep`, `calloc/realloc/free` (20/11)
- [x] `reent` por tarefa: `errno` isolado e ponteiros `__getreent()` distintos; slot/ID reciclado com `errno` reinicializado e chave pthread limpa (4 boots mornos, 20/11)
- [x] Gthread no device: `std::mutex` com contenção, `pthread_key_t` isolado e destrutores ao retornar das tarefas
- [x] Construtores estáticos e guardas C++ no device: probe com construtor global e guarda local concorrente (4 boots mornos); produto XIP chegou a `msh>` (1 boot morno, restauração verificada, 20/11)
- [x] TLS estático local-exec por tarefa (`thread_local` ELF): isolamento em workers, inicialização no boot, inicialização zero, alinhamento de 64 bytes e reuso de slot; 4 boots mornos no Tab5 (20/11). TLS dinâmico e inicializadores/destrutores não triviais continuam pendentes.
- [x] Subconjunto `std::thread`: criação, `get_id`/`this_thread::get_id`, `join` e `detach`; stack liberada após estado `EXITED` e troca para a stack de IRQ; host com reuso de ID e 16 ciclos sequenciais (8 `join`, 8 `detach`) por boot, dois boots mornos no Tab5 (20/11)
- [x] Link e empacotamento do produto XIP com Newlib/gthread atuais (ELF e imagem passaram em 20/11; boot do produto registrado abaixo)
- [x] Boot morno do produto XIP no device: shell `msh>`, `qtest`, serviços LCD/touch; tela estável e interação touch confirmadas visualmente (20/11; cold boot pendente)
  - [x] Repetir boot eletrônico do produto XIP com o runtime TLS integrado: `msh>`, `qtest`, serviços LCD/touch; restauração verificada (20/11; 1 boot morno, sem nova observação visual)
- [x] Auditoria de símbolos/runtime no ELF e mapa de link do produto: zero nomes proibidos, zero símbolos indefinidos e zero caminhos de runtimes FreeRTOS/ESP-IDF (20/11; ELF, mapa e relatório preservados em `maia-rtos-recovery/native-product-xip-link-20261120/`)

## F2 — Armazenamento

- [ ] Driver SDMMC host nativo
  - [x] Observação em RAM de alimentação/pinos/controlador passou em dois boots mornos, sem enviar comandos (20/11).
  - [x] Diagnóstico isolado em RAM configurou o slot 0 e executou `CMD0`/`CMD8`: conclusão de `CMD0` e resposta `CMD8` `0x1aa` em dois boots mornos (20/11). Ainda não lê setores nem está integrado ao produto.
- [ ] Protocolo de cartão SD (inicialização, leitura, escrita)
  - 🔧 Base de leitura SDHC/SDXC: geometria CSD v2 e verificação de endereço para `CMD17` testadas no host; falta completar inicialização, transporte de dados e validação da leitura no Tab5 (20/11).
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

- [ ] Auditoria ampla do ELF/link completo: nenhum código FreeRTOS ou runtime ESP-IDF (a varredura de símbolos e caminhos de arquivos passou; revisão ampla restante)
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
