# Entrada serial nativa com dono e fila — 2026-11-11

Estado: **HARDWARE-TESTED, LIMITED** para USB-Serial/JTAG no ESP32-P4 rev v1.3.
Incremento posterior às [filas nativas](native-driver-queues.md). Separa a
recepção USB/UART do teclado/display e oferece um serviço opcional, sem importar
runtime do SDK. O desktop completo continua na imagem FreeRTOS preservada;
este diagnóstico não é um port da shell/desktop nem substitui esse firmware.

## Implementação e contratos

- `soc_serial_rx.c` contém somente leitura não bloqueante do USB-Serial/JTAG,
  seguida de UART0 quando USB está vazio, usando os mesmos registradores do
  console anterior. Há no máximo duas consultas de disponibilidade e uma leitura
  FIFO por chamada; token e acesso ao FIFO ficam sob exclusão local de IRQ.
- Um token estável não nulo reserva a recepção até reset, em inicialização sem
  leitores concorrentes. Token errado retorna -2 **sem acessar MMIO**. NULL é
  o leitor legado somente antes da reserva. A reserva é disciplina de propriedade,
  não isolamento de memória ou mecanismo de segurança. Sem SMP ou prioridade
  justa entre fontes: USB continuamente ativo pode postergar UART.
- `services/serial_input.c` mantém fila estática de **64 bytes** e associa a
  leitura a uma tarefa READY criada antes do início do escalonador, com IRQs
  desabilitadas. Não cria tarefa ou heap implicitamente. O chamador cria a tarefa
  e define sua frequência de serviço usando as APIs nativas.
- Somente essa tarefa pode chamar `poll`, em contexto de thread com IRQs
  habilitadas; cada chamada move até **16 bytes**, sem teclado ou display.
  Fila cheia retorna FULL e não consome outro byte do FIFO. EMPTY/OK/FULL e demais
  erros estão descritos no header. Não há espera bloqueante nem wakeup por IRQ RX.
- Consumidor usa `read`; EMPTY preserva o byte de saída. Leitura/estatísticas
  usam as mesmas seções locais, também permitidas em ISR no mesmo núcleo.
  Publicação na fila e contador received são atômicos entre si. Estatísticas
  são snapshots coerentes; contadores uint32 dão a volta modularmente.
- Inicialização e reserva são permanentes até reset. A tarefa proprietária não
  pode terminar nem ter seu slot reutilizado enquanto o serviço estiver ativo.
  Teardown/reatribuição de dono e geração de handles ficam para outra etapa.
- O console legado passa pelo mesmo driver. Antes da reserva mantém a seleção
  serial/teclado e manutenção de display; depois retorna -1 sem consumir entrada
  ou chamar teclado/display. Quem ativar o serviço deve usar sua API de leitura;
  a shell não é convertida automaticamente. Assinaturas antigas são preservadas.
- TX não é alterado nem tornado concorrente: segue exigindo chamadores
  serializados, inclusive para display. O header deixa de anunciar o console
  como seguro em panic/ISR; fatal traps já usam seu próprio sink limitado.

Direção: aplicação -> serviço -> API de driver/fila nativa -> SoC/arquitetura.
O console de baixo nível não depende do serviço. O build adiciona o driver RX
à lista comum dos alvos nativos; o serviço entra apenas no novo diagnóstico
opt-in `MAIA_NATIVE_PROBE_SERIAL=ON` e nos testes. Nenhum campo do TCB, frame,
ABI de módulo, parser/lexer/EBNF ou fonte de runtime externo foi alterado.

## Testes locais e negativos

macOS arm64, C11, warnings-as-errors. CMake/CTest raiz **15/15 PASS**.
ASan/UBSan serviço serial + fila **2/2 PASS**; novos headers e console passam
C++98 estrito. Fixture usa o código real do serviço, driver e wrapper de console,
com acesso a registradores modelado somente para host. Não executa MMIO no host.

Cobre USB antes de UART, valores 0/255, vazio, leitor legado e teclado antes da
reserva; negação do legado sem MMIO/teclado/display depois dela; inicialização,
repetição e dono inválido; rejeição de poll por outra tarefa/IRQ mascarada;
lotes de 16 bytes; fila cheia sem leitura extra; retomada, ordem, estatísticas,
saída intacta em EMPTY e preservação de máscaras aninhadas. Objetos/estado de
tarefa e registradores modelados não são prova de execução no alvo.

Quatro mutantes isolados são compilados e rejeitados pela fixture: retirar
validação da tarefa proprietária; dobrar o lote; ler FIFO com fila cheia;
permitir token legado após reserva. Nenhum mutante foi enviado ao device.
O runner também testa marcadores incompletos/repetidos e escrita curta no host;
fixture binária não é enviada antes de um marcador completo do diagnóstico.

```sh
cmake -S . -B /tmp/maia-host-contracts-20261110
cmake --build /tmp/maia-host-contracts-20261110 -j4
ctest --test-dir /tmp/maia-host-contracts-20261110 --output-on-failure
cmake -S . -B /tmp/maia-host-contracts-sanitized-20261110 -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build /tmp/maia-host-contracts-sanitized-20261110 --target serial_input_test queue_atomic_test -j4
ctest --test-dir /tmp/maia-host-contracts-sanitized-20261110 -R '^(serial_input_test|queue_atomic_test)$' --output-on-failure
```

## Teste interativo no Tab5

GCC esp-14.2.0_20241119, RV32IMAC/ILP32, esptool 4.12. Somente RAM; janela SRAM
validada antes de abrir a porta e reset em finally. O runner opt-in `--serial-input`
envia bytes 0..63 em blocos de 16, espaçados 5ms, após `READY fill`. A tarefa dona
faz poll e dorme 1 tick; o consumidor inicialmente espera a fila chegar a 64 e
o dono observar FULL. Após `READY drain`, o host envia bytes 64..255 com a mesma
cadência. O consumidor confere cada valor, sem conversão de CR/LF/NUL, e dorme
quando vazio; uma terceira tarefa ocupada mede progresso concorrente.

Resultado final real, `native-probe-logs/serial-final.log`:

```text
SERIAL received=256 published=256 full=1 polls=317 irq=317 heartbeat=22532623 errors=0
MAIA_NATIVE_PREEMPT_PASS
```

O consumidor também verifica rejeição de poll por tarefa errada e leitura pelo
console antigo. Stubs apenas no diagnóstico detectam chamadas indevidas a
teclado/display pelo caminho gerenciado; o stub de display do timer continua
restrito ao contexto IRQ. Nenhum tick é sintetizado. O contador heartbeat indica
progresso desta carga, não throughput mínimo ou garantia de justiça/latência.

Primeira carga: mesmos 256 bytes corretos, FULL=1 e errors=0, mas heartbeat=0;
resultado **FAIL** pelo critério de progresso auxiliar. Consumidor sempre pronto
mais o produtor de alta prioridade despertando a cada tick não garantiam tempo
à outra tarefa nesta ordem de seleção. O consumidor foi ajustado para dormir
quando vazio/aguardando preenchimento. Isso altera só o teste, não acrescenta
uma promessa de round-robin justo ao escalonador. Log e ELF dessa primeira carga
são preservados; não apresentá-la como PASS ou como falha de integridade serial.

Regressão com serviço desativado: perfil anterior de filas passa **253 IRQs,
688 pacotes íntegros, errors=0**. Gate de ambos os ELFs: objetos do projeto +
libgcc, sem símbolos estrangeiros/não resolvidos. Avisos já existentes de LOAD
RWX e segmentos vazios removidos do ELF RAM permanecem.

```sh
cmake -S . -B /tmp/maia-native-serial-20261111 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_SERIAL=ON
cmake --build /tmp/maia-native-serial-20261111 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-serial-20261111/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-serial-20261111/maia_sched_probe_ram.bin --log /tmp/maia-serial-hardware-final.log --serial-input
```

Somente o diagnóstico serial usa `--serial-input`; não enviar a sequência à
shell/desktop. Outros perfis não são combináveis com este workload. A interface
real `maia.py` confirmou shell, SciCalc `2+3*4=14`, saída do módulo e desktop após
o último reset. Não houve gravação de flash/SD. Logs no diretório acima; ELF,
binário, mapa, configuração, fontes/scripts e SHA256 no diretório irmão
`../maia-rtos-recovery/native-serial-input-20261111/`.

## Evidência e limitações

Referências primárias dos registradores reaproveitados:
[EVIDENCE, entrada serial](../hardware/EVIDENCE.md). Headers de referência do SDK
foram apenas lidos; não são includes/dependências do build nativo. Teste físico
cobre USB-Serial/JTAG. UART físico não foi revalidado; fallback/prioridade são
cobertos pelo modelo local. Validar a fiação UART do BSP antes de um ensaio
externo; não inferir pinagem de macros de endereço do controlador.

Fila cheia impede novas leituras pelo serviço, mas não impede overflow dos
buffers do hardware/host se a origem exceder a capacidade de serviço. Ensaio
usa tráfego cadenciado, não certifica recepção sem perdas em taxa arbitrária.
Sem IRQ RX/deferred wakeup, SMP, cancelamento, hot teardown, TX concorrente,
tratamento de erro UART, merge de teclado/touch, VFS ou port da shell completa.

Próxima tarefa pequena: separar e definir a propriedade da saída serial, hoje
acoplada a UART/USB/display, antes de integrar múltiplos workers/apps. Esperas
bloqueantes nativas e drivers de teclado/display/armazenamento continuam como
incrementos próprios. POSIX permanece apenas um projeto futuro.
