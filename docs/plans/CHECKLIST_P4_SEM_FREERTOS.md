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
- [x] PSRAM/LCD robusto a reset morno: dispositivo já em X8 adotado com perfil verificado; 4 resets seguidos e Files → desktop na mesma sessão passaram (26/11)
- [x] Carregamento de `.mobj` sem gargalo: leitura do arquivo inteiro e liberação do buffer após a relocação; Files 16,8 s → 9 ms e desktop 24,9 s → 12 ms; abertura rápida confirmada pelo dono (26/11)
- [x] Desenho da calculadora MaiaScript com velocidade equivalente à referência (cópia só da região alterada entre framebuffers)
- [x] Imports `host_fs_*` do MaiaScript (Notes) e `strncpy` (Terminal) no loader nativo; Notes, calculadora, Files e Terminal validados no desktop
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
- [x] Heap ampliado de 50 KB para ~257 KB (263.044 B): pool baixo 77 KB + área do bootloader de 2º estágio recuperada (76 KB, `0x4FF28000`) + 104 KB da SRAM alta (`0x4FF4B800`); pilhas da shell (agora 32 KB), LCD, terminal e touch movidas para a SRAM alta. Execução de código na SRAM alta confirmada por sonda no boot (exige writeback do L1D). Dois boots via USB com `mem`/`smp`, desktop 120 s sem fatal, app original restaurado/verificado (26/11). Opção CMake `MAIA_PRODUCT_HIGH_SRAM` (ON).
  - [ ] Pendência pré-existente (também no build sem SRAM alta): abrir o desktop depois do Files na mesma sessão falha com `Native graphics transaction failed`.
- [x] Auditoria de símbolos/runtime no ELF e mapa de link do produto: zero nomes proibidos, zero símbolos indefinidos e zero caminhos de runtimes FreeRTOS/ESP-IDF (20/11; ELF, mapa e relatório preservados em `maia-rtos-recovery/native-product-xip-link-20261120/`)

## F2 — Armazenamento

- [x] Driver SDMMC host nativo para leitura e escrita de blocos; serviço de runtime integrado e leitura/escrita de dois setores verificada no Tab5 (21/11).
  - [x] Observação em RAM de alimentação/pinos/controlador passou em dois boots mornos, sem enviar comandos (20/11).
  - [x] Diagnóstico isolado em RAM configurou o slot 0 e executou `CMD0`/`CMD8`: conclusão de `CMD0` e resposta `CMD8` `0x1aa` em dois boots mornos (20/11). Ainda não lê setores nem está integrado ao produto.
- [x] Contrato C de dispositivo de blocos para leitura/escrita multi-setor, limites, progresso parcial e propagação de erros; CTest, ASan/UBSan, C++98 e compilação P4 passaram (21/11).
- [x] Backend SDMMC síncrono de runtime integrado ao boot do produto: DMA alinhado reservado no heap interno, serialização single-core e cache por região; serviço e leitura LBA0 validados no boot completo do produto, além da leitura/escrita física em tarefa MaiaRTOS (21/11).
- [x] Protocolo de cartão SD (inicialização, leitura, escrita)
  - [x] Base SDHC/SDXC: geometria CSD v2 e limites de endereço testados no host; inicialização, leitura IDMAC e escritas CMD24 de bloco único e do serviço de runtime qualificadas no Tab5. Produto XIP chegou a `msh>` após leitura LBA0 pelo serviço (21/11).
  - [x] Diagnóstico isolado em RAM identificou o SDHC (ACMD41, CID/RCA/CSD, seleção e estado pronto); a resposta de `CMD17` e `DATA_OVER` foram observados (20/11).
  - [x] Investigação PIO caracterizou a falha sem aceitar dados: RX_WMARK=0 tornou RXDR observável; acknowledgement W1C, espera por `DATA_OVER` e `CARDTHRCTL=0` não resolveram os 128 acessos zero/FIFO_COUNT=128; disassembly confirmou `lw` em `0x50083200` (20/11). Ver evidências de DTO/CARDTHRCTL.
  - [x] Sonda IDMAC somente de leitura passou em LBA 0 (duas vezes), LBA 1 e início da partição em LBA 32768 (duas vezes); os bytes de LBA 32768 coincidiram exatamente nas repetições e o BPB FAT32 concordou com a partição do MBR. Cache L1D foi sincronizado, ownership retornou e o runner validou capacidade e conclusão (20/11).
  - [x] O setor de boot FAT32 de backup em LBA 32774 coincidiu byte a byte com o setor principal em LBA 32768; a sonda passou a exigir 512 bytes nos dois contadores do host antes de publicar o bloco. Testes host rejeitaram 0, 511 e 513 bytes em cada contador, e a regressão física de LBA 0 passou (20/11).
  - [x] Duas leituras CMD17 consecutivas em um boot para LBA 32768 e 32774; cada transferência passou os contadores TCBCNT/TBBCNT e os buffers coincidiram byte a byte. A comparação é entre setores do mesmo cartão, não com leitor independente (20/11).
  - [x] Referência de software independente no próprio Tab5: firmware auxiliar Arduino/ESP-IDF leu os LBAs 0, 1, 32768 e 32774 sem montar filesystem; os 2048 bytes coincidiram com as capturas nativas. Flash integral restaurada/verificada; desktop e teclado confirmados pelo usuário. Mesmo controlador/cartão, sem teste com outro leitor físico (21/11).
  - [x] Validar no produto Tab5 a inicialização do serviço e leitura LBA0 pela tarefa do shell; boot chegou a `msh>` sem fatal (21/11).
  - [x] Validar o serviço de runtime em tarefa MaiaRTOS: leitura de dois setores contíguos em LBA 69318 e escrita/leitura de padrão nos mesmos dois setores; inicialização, sincronização DMA/cache e conclusão passaram (21/11).
  - [x] Qualificar fisicamente a escrita CMD24 de um bloco: protótipo boot-only passou em LBA 69318, num cluster FAT32 livre; comparação do original, gravação e releitura do padrão, restauração e releitura byte a byte passaram. Flash integral restaurada/verificada (21/11).
  - [x] Validar fisicamente escrita multi-setor pelo serviço de runtime, preservando e restaurando os dois setores originais; ambos os blocos foram confirmados prontos e a releitura coincidiu (21/11). O backend emite CMD24 por setor; CMD25 não foi implementado.
  - [x] Cobrir no host inicialização sticky, timeout simulado no segundo bloco de leitura e de escrita (1/2 concluído) e requisição sobreposta. Após timeout, leituras e escritas retornam `MAIA_SD_NOT_READY` sem novo acesso a MMIO; requisição sobreposta retorna `MAIA_SD_BUSY`. Testes focados e ASan/UBSan passaram (21/11).
  - [x] Cobrir no host a propagação de `MAIA_SD_TIMEOUT` e `MAIA_SD_NOT_READY` dos callbacks mock pelo diskio e MaiaVFS, incluindo erros em `sync`/`close`; o mock é restabelecido manualmente apenas para a limpeza do teste, não para provar recuperação do serviço real (21/11).
  - [x] Testar o boot do produto após falha de inicialização SDMMC: timeout **simulado em software** após CMD8 fez o serviço ficar indisponível, mas o boot chegou a `msh>` e `qtest PASS`; flash restaurada e comparada integralmente (21/11).
  - [x] Estresse concorrente no Tab5: dois workers MaiaRTOS fizeram 16 ciclos cada de escrita/sync/leitura/comparação de registros de 512 bytes, com contenção `BUSY` limitada e yield; arquivos removidos ao final (21/11).
  - [x] Recuperação de erros simulados no diskio: falha única de escrita durante sync seguida de retry e falha de leitura seguida de close/reopen; host e Tab5 passaram. A injeção ocorre antes do callback físico e não simula remoção ou perda de energia (21/11).
  - [x] Host: arquivo sincronizado, fechado, volume desmontado/remontado e conteúdo relido byte a byte; usa disco mock em RAM e não demonstra reboot ou perda real de energia (21/11).
  - [x] Tab5: registro de 512 bytes sincronizado/fechado, reset morno limpo por RTS e conteúdo/EOF verificados no boot seguinte; região de app restaurada e comparada byte a byte. Não testa cold boot, falha física nem perda de energia (21/11).
  - [ ] Falhas reais de transporte no Tab5: não testadas; as simulações host e de diskio não substituem o teste físico.
  - [ ] Remoção física do cartão: não testada e fora do plano com o teclado instalado, pois o slot fica de difícil acesso. DET é NC: o driver não distingue remoção de outras falhas de transporte nem sinaliza `NO_MEDIA`.
  - [ ] Perda de energia durante operações SD: não testada; qualquer interrupção deliberada exige autorização específica e plano de segurança para a mídia. O reset morno limpo já validado não cobre esse caso.
  - [x] Validar leitura e escrita no cartão de teste em um bloco: LBA 69318 preservado antes do teste; padrão escrito e comparado; setor original restaurado e comparado novamente. O backup dos bytes foi obtido pela referência de software no Tab5 (21/11).
  - [x] Leitura e escrita do serviço integradas e verificadas no Tab5 sem dependências Arduino/ESP-IDF/FreeRTOS; estresse concorrente de software e recuperação de falhas injetadas também passaram (21/11).
- [x] FatFs R0.15 integrado ao SDMMC nativo: montagem sem formatar; create/update/read/remove de arquivo testados no host e no Tab5 (21/11)
- [x] API MaiaVFS de arquivos em `/sd`: open/read/write/seek/sync/close/unlink, caminhos confinados e handles fixos; host, sanitizers, build P4 e diagnóstico no Tab5 passaram (21/11)
- [x] Listagem de diretórios e integração do VFS com shell/Newlib: `cd/pwd/ls`, `cat`, `echo >`/`>>`, `mkdir/rmdir/rm`; lifecycle de arquivo de 17 bytes verificado no Tab5 e POC shell/LCD sem desktop aprovada pelo dono (21/11). Estresse de dois workers, recuperação de falhas injetadas e persistência após reset morno passaram; falhas físicas e perda de energia permanecem pendentes.
- [ ] Configurações persistentes na flash
  - [x] Serviço C MaiaKV com snapshots alternados de 8 KiB, CRC e marcador de commit; falhas injetadas no host preservam a última cópia válida e a recuperação de estado vazio/corrompido é reportada explicitamente. CTest, ASan/UBSan e imagem P4 passaram; nenhuma gravação foi feita no Tab5.
  - [x] Expor `maia.kv` no perfil nativo Tab5, registrar os imports MaiaKV no ELF loader e testar `get/set/remove/contains` pelo compilador/emulador MaiaScript (33/33).
  - [x] Reduzir a reserva excessiva do vetor de tokens; testes host 33/33 e AST lifetime passaram, e um script mínimo compilou no Tab5 para ELF32 de 416 bytes com imagem limpa (22/11).
  - [x] Reduzir o uso temporário de memória no parser/CST, codegen e assembler; o fonte mínimo que importa `maia.kv` compilou no Tab5 para ELF32 de 2.224 bytes, e os testes do assembler passaram (22/11).
  - [x] Executar o módulo compilado no Tab5: corrigida a coerência de cache do loader (writeback L1D + invalidação L1I pela ROM, como `esp_cache_msync` no IDF) e a truncagem do `stdout`; `maiasc --run` imprimiu `MAIA_ENTRY_REACHED` e retornou 42.
  - [x] Qualificar o serviço MaiaKV e os imports nativos de flash no Tab5: chave temporária gravada (set=0), lida após reinicialização ("ok"), removida e confirmada ausente no boot seguinte; `copilot_probe` estava ausente. Runner USB restaurou o aplicativo original em cada etapa (22/11). Sem teste de perda de energia.
  - [x] Executar os wrappers de `import maia.kv` no Tab5: o probe original com `kv.contains`, `isError()` e `42 + r.value()` compilou e retornou 42 em imagem limpa. A investigação identificou o crescimento contíguo do `stringstream`; pool L2 ajustado para 49 KiB e pilha do shell para 16 KiB. Runner USB restaurou o app original e removeu os arquivos temporários (22/11). A cópia de `kv.ms` no SD foi conferida integralmente; sem gravação duplicada.
- [x] Teste de estresse multi-task de leitura/escrita no Tab5: 32 registros de 512 bytes verificados e removidos; recovery runner restaurou o firmware (21/11)
- [ ] Transferência de arquivos pelo `maia.py`
  - [x] Cliente host valida caminhos, confirma cada bloco por leitura de volta, não repete append após timeout e cobre arquivos vazios/erros com serial simulado (9 testes, incluindo abertura com DTR/RTS baixos).
    - [x] Validar `/sd/lib/maia/kv.ms` no Tab5 pelo cliente de transferência do `maia.py`: o arquivo já existia; pull completo de 1.259 bytes conferiu com o fonte local (SHA-256 `2ac4734b6bb1cea998fa5b49f1f742d0dee04ab341f80dba388ad2a35e8e60e0`). Não foi sobrescrito; app original restaurado pelo runner USB (22/11).

## F3 — Periféricos da placa

- [x] Teclado físico M5Stack A164 (STM32F030, 0x6D): entrada nativa na shell verificada com `ls` e `pwd` digitados no teclado (23/11); TCA8418 permanece como fallback sem validação física
- [ ] RTC RX8130 (data e hora)
  - [x] Implementar leitura somente-leitura pelo I2C compartilhado; testes host validam BCD, calendário, weekday, VLF e erros
  - [x] Exercitar `rtc` no Tab5 por USB sem escrita; dados físicos inválidos detectados (`month=00`, `weekday=0x0C`), app original restaurada e hash verificado (23/11)
  - [ ] Definir fonte/valor de data e hora e autorizar inicialização do RTC; não ajustar automaticamente
- [ ] INA226 (consumo)
  - [x] Serviço INA226 somente-leitura no I2C compartilhado; valida IDs TI, configuração, calibração e medições, sem escrever registradores nem ler Mask/Enable
  - [x] Teste host cobre ordem/decodificação, identidade, erros BUSY, ausência de escrita e cálculo pela tensão do shunt Tab5 de 5 mΩ sem depender de CAL
  - [x] Teste USB: modo contínuo e leitura de barramento/shunt confirmados; CAL veio zero, então corrente/potência são derivadas do shunt sem alterar o INA226; app original restaurada e hash verificado (23/11)
  - [ ] Repetir sob carga conhecida para qualificar a magnitude/direção de corrente e potência (adiado: dono sem carga de referência no momento)
- [ ] Bateria IP2326 (carga e nível)
  - [x] Comando `battery` lê a tensão pelo INA226 e informa o nível bruto do sinal P6 do expansor, sem afirmar estado de carga nem estimar percentual
  - [x] Teste USB no Tab5: `7987 mV`, sinal `LOW`; contenção BUSY foi tratada com retry limitado, touch não invalidou entrada e o app original foi restaurado/verificado (23/11)
  - [ ] Confirmar polaridade/semântica do sinal de carga e definir método de estado de carga antes de exibir percentual
- [x] Brilho da tela (LEDC)
  - [x] Teste USB `backlight 50`/`100`: LEDC duty `0x1FF0` e GPIO22 reassociado ao LEDC/GPIO; app original restaurado/verificado (23/11)
  - [x] Varredura visual `100→75→50→25→5→0→100` confirmada pelo dono ("Brilho funcionou."); app original restaurado/verificado (23/11)
- [x] Desligamento
  - [x] `maia_tab5_power_off` (porta nativa de `bsp_generate_poweroff_signal`): 3 pulsos de 100 ms em P4 do expansor 0x44, preservando os demais bits, retry em BUSY e P4 sempre devolvido a baixo em falha parcial; teste host `tab5_power_test`
  - [x] Comando shell `poweroff` apaga o backlight antes dos pulsos e informa se o aparelho continuar ligado
  - [x] Teste USB no Tab5: desligou com o USB conectado (confirmado pelo dono); após religar, app original restaurada com hash verificado e desktop de volta (23/11)
- [x] Sensor de movimento BMI270
  - [x] Identificação somente-leitura no endereço `0x68`: `imu id` leu CHIP_ID `0x24`; app original restaurada e verificada (23/11)
  - [x] `imu init` carregou a configuração Bosch e habilitou accel/gyro em 200 Hz; `imu read` retornou amostras brutas no Tab5; app original restaurada e verificada (23/11)

## F4 — Sistema utilizável sem FreeRTOS

- [ ] Shell completa sobre o kernel nativo
- [ ] Jobs e processos da shell
- [ ] Carregador `.mobj` (memória executável + sincronização de cache)
  - [x] `runelf <arquivo.mobj>` lê ELF por offsets sem buffer integral; testes host com ASan/UBSan (3/3) e probe USB abriram SciCalc de 57.256 bytes, alocaram 36.096 bytes e chegaram a `maia_module_main` (20 s sem marcador fatal; firmware original restaurado/verificado, 24/11). Isso valida o caminho de leitura/dispatch, não a interface.
  - [x] Adaptador nativo para `draw_*`, `host_bind_*`, `host_app_*` e `powf`; 6 testes host focados com ASan/UBSan e build P4 validados. O produto agora usa RV32IMAFC/ILP32F, compatível com os módulos MaiaScript single-float, e preserva o último estado de toque quando ainda não há amostra válida.
  - [x] Confirmar SciCalc nativa no Tab5: título/controles visíveis, toque e teclado produziram `2+3*4 = 14`, saída normal código 0 após encerramento pelo dono; firmware original restaurado/verificado (24/11). O runner encerrou antes dos 60 s solicitados por causa dessa saída normal.
- [ ] Compilador MaiaScript no device
  - [x] Suíte host compilador → assembler → emulador RV32: 33/33, incluindo lógica real da SciCalc (`2+3*4=14`, potência e divisão por zero); interface gráfica simulada, sem validação visual nativa (23/11)
- [ ] MNM (serviços nativos) sobre o kernel
  - [x] Ponte `sleep_ms` usa a espera do scheduler nativo e propaga falhas; regressões host e módulo MaiaScript no Tab5 (300 ms solicitados, 309 ms medidos; app original restaurada/verificada, 23/11)
  - [x] Preservar o host dos wrappers MNM em chamadas aninhadas e falhas de validação; testes host com/sem TLS e smoke no Tab5, com app original restaurada/verificada (23/11). O carregador ainda exige serialização.
  - [x] `write_fd` da shell retorna a contagem correta ou erro observável; testes host e módulo no Tab5 confirmaram stdout, stderr e redirecionamento, com app original restaurada/verificada (23/11). A saída atual é textual, sem NUL embutido, e o callback não confirma transmissão física.
  - [x] Impedir anúncio de framebuffer fixo não qualificado: gráficos MNM nativos são informados como indisponíveis e `fill_rect` retorna erro; teste host e smoke USB passaram (23/11).
  - 🔧 Integrar os callbacks gráficos ao serviço LCD sem violar a propriedade dos framebuffers
    - [x] Lease incremental, preservação do quadro e apresentação serializados pelo owner LCD; exclusividade aninhada e limpeza ao retorno de módulos (testes host focados).
    - [x] Build do firmware nativo P4 concluído.
      - [x] Criar e compilar fixture MNM que desenha quadro colorido e publica uma atualização parcial; runner USB usa gravação/restauração verificadas.
      - [x] Executar duas vezes o smoke gráfico no Tab5 via USB: dois lotes publicados, módulo retornou 0, shell viva, diretório temporário limpo e firmware original restaurado/verificado; na repetição, cada quadro ficou 8 s (24/11).
      - [x] Confirmar visualmente o quadro colorido e sua preservação após a atualização parcial; confirmação do usuário na repetição com quadros de 8 s (24/11).
      - [x] Confirmar SciCalc nativa com controles, toque, teclado e saída normal (24/11).
      - [x] Confirmar desktop nativo visualmente por 60 s, com elementos visíveis e estáveis, sem fatal; confirmação do usuário e restauração verificada (25/11).
- [x] Desktop nativo — abertura visual confirmada 👀
- [x] Desktop nativo — toque, abertura e retorno do app Banner confirmados pelo usuário (probe USB de 120 s, 25/11)
- [x] Files — execução direta, navegação, setas ↑/↓ sem fechar o app e fechamento no X confirmados no Tab5; o usuário confirmou tempo de resposta normal e setas funcionando. A API MNM normaliza as sequências ANSI do teclado; testes host passaram. O caminho de listagem VFS direta mediu `/sd` (74 entradas) em 130–137 ms e `/sd/DCIM` (2 entradas) em 59 ms, contra ~4,35 s na medição anterior de `/sd`. O loader registrou 16,75 s para carregar `files.mobj`; `/sd/apps` não foi cronometrado novamente.
- [x] Files — abertura pelo ícone do desktop nativo revalidada via USB com a imagem P4 sem diagnóstico (26/11). O usuário confirmou entrada quase instantânea em `/sd/apps`, navegação com ↑/↓ e retorno imediato ao desktop pelo X. A abertura levou ~15 s; firmware original restaurado e verificado.
- [ ] Investigar e reduzir a demora de ~15 s entre o toque no ícone e a abertura do Files (separada da navegação após a abertura)
- [ ] Validar abertura/retorno dos demais aplicativos a partir do desktop
- [ ] Validar funcionalidade completa dos apps: Alarm, Notes, Term, Editor, Clock, Sysinfo e Banner (Banner teve apenas abertura/retorno confirmados); operações de arquivos do Files também permanecem sem validação 👀
  - [x] Files: navegação por pastas, setas ↑/↓ e fechamento no X confirmados; ainda falta validar as operações de arquivos.
  - [x] SciCalc (`2+3*4=14`): desenho, toque, teclado e saída confirmados no firmware nativo (24/11)
- [x] Suíte host de transferências do `maia.py` passando (14/14, 24/11)
- [x] Restabelecer a suíte host após o primeiro desktop nativo: build limpo no Mac e 110/110 testes CTest aprovados, sem exclusões (25/11).

## F9 — Segundo núcleo (SMP)

- [x] Integrar a partida do core 1 ao produto
  - [x] Partida isolada via USB no Tab5: worker e pilha em SRAM interna, `hart=1`, 1.024 trocas atômicas verificadas e estacionamento confirmado em duas partidas; app original restaurado/verificado (26/11). Testes host positivos/negativos e validação da imagem passaram.
  - [x] Produto `maia_m0` liga o core 1 no boot (`[F9] MAIA_CORE1_ONLINE hart=1`), que fica online como worker em SRAM com heartbeat; comando `smp` e `cpuinfo` na shell. Duas partidas via USB: 3/3 pings OK por partida, heartbeat crescendo; desktop por 120 s sem fatal; app original restaurado/verificado (26/11).
  - [x] Executar tarefas do kernel no core 1: o scheduler roda nos dois núcleos; `maia_shell`, desktop e apps ficam no core 1 (afinidade 2, opção `MAIA_PRODUCT_SHELL_AFFINITY`); LCD, touch, telemetria e idle ficam no core 0. `ps` mostra `lcd` (CPU 0) e `maia_shell` (CPU 1) em RUN ao mesmo tempo (26/11).
- [x] Interrupções entre processadores (IPI): `FROM_CPU` 0/1 na linha CLIC 17 dos dois núcleos; tick de 1 kHz no core 0 dispara IPI de time-slice no core 1; acordar uma tarefa avisa o núcleo dela (26/11).
- [x] Filas de execução por núcleo: tarefa atual, idle e afinidade por núcleo no `ksched`; teste host `scheduler_smp_test` (26/11).
- [x] Spinlocks e revisão das seções críticas: `maia_arch_irq_save` agora também pega um lock global recursivo do kernel; trap mantém o lock entre commit e seleção (migração segura); não há seções só-locais nos drivers (26/11).
  - [x] Flash/KV a partir do core 1: a operação migra para o core 0 e o core 1 espera em SRAM durante o suspend do L2. Módulo KV no core 1: 234 operações, 234 freezes e 234 migrações, sem erro (26/11).
- [x] Desktop no core 1 com o dono no aparelho: Files, Notes e calculadora funcionando (26/11).
- [x] Desempenho dos apps MaiaScript igual à referência: apps desenham direto no framebuffer exibido, e o `display_sync` só faz writeback da área alterada, como na referência. Dono: "Está excelente! Parece mais rápido para carregar os apps" (26/11)
- [x] Estresse concorrente de 10 min no Tab5: 3.703 iterações no core 1, com leituras KV/SD e desenho direto via LCD/DMA; serviço touch ST7121 pronto no boot. 19 checkpoints, `MAIA_SMP_STRESS_PASS`, exit 0, sem fatal; módulo temporário removido e firmware de referência restaurado/verificado (26/11).

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
