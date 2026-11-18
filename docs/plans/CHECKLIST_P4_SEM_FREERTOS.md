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
- [x] LCD MIPI-DSI nativo com refresh contínuo 👀
- [x] Touch GT911 nativo 👀
- [x] 77 testes de host passando (`e018c2c`)

## F0 — Firmware nativo gravado na flash

🔧 **Em andamento:** em 17/11 dois probes sem FreeRTOS foram iniciados pelo
bootloader do aparelho a partir da flash. O segundo executou kernel, duas tarefas
e timer com XIP; observou PSRAM sem escrita e foi seguido pela restauração do
firmware original e retorno do desktop. Falta a imagem de produto. O target
completo `maia_m0.elf` continua bloqueado: `-ffreestanding` impede headers
hosted do libstdc++; um ensaio temporário `-fhosted` expôs conflitos de includes
e wrappers `malloc/free` no shell. Resolver a integração de runtime na F1.

- [x] Imagem nativa no formato do app ESP-IDF, inicializada pelo bootloader de 2º estágio (probe, 17/11)
- [x] Kernel/scheduler e duas tarefas executando por XIP, com preempção por timer real (probe, 17/11; imagem de produto pendente)
- [x] Base de linker flash-XIP testada no probe (IROM/DROM + estado em SRAM; PSRAM apenas como janela runtime, 18/11)
- [x] Closure de inicialização PSRAM do probe colocada em segmento SRAM; referências relocáveis a XIP bloqueadas no link e imagem aceita no device (reset morno, 18/11; rotinas PSRAM não foram executadas)
- [ ] Linker final do produto com seções críticas de inicialização em SRAM e integração PSRAM
- [x] Allocator multi-região — duas regiões SRAM exercitadas no probe XIP (18/11)
- [ ] Inicializar e registrar heap PSRAM após validar boot frio e mapeamento da CPU
- [ ] Inicialização de PSRAM a frio em todo boot — snapshot de 54 leituras de controlador passou, mas não houve inicialização nem acesso à memória PSRAM
- [ ] LCD, touch e console como serviços do kernel (fim das opções de teste encadeadas)
- [x] Gravação com verificação e restauração pelo `maia-rtos-recovery` (`tests/hardware/run_f0_flash_probe.py`)
- [ ] Boot a frio repetido no device (10×) sem falha

## F1 — Biblioteca C/C++ sobre o kernel

- [x] Mutex do kernel protege aquisição/liberação contra preempção local (host tests + compilação RV32, 18/11)
- [ ] Bloqueios da newlib (locks) com mutex do kernel
- [ ] `malloc`/`free` sobre os heaps do kernel
- [ ] Chamadas de sistema (`_write`, `_read`, `_sbrk`, tempo)
- [ ] Contexto `reent` por tarefa
- [ ] Construtores globais e guardas de inicialização C++
- [ ] `std::mutex` / gthread nativo
- [ ] Testes C/C++ no device sem símbolos FreeRTOS

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
