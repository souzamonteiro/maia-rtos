# Saída serial nativa com registros e dono — 2026-11-11

Estado: **HARDWARE-TESTED, LIMITED (USB-Serial/JTAG)**. Continuação da
[entrada serial](native-serial-input.md), sobre as [filas nativas](native-driver-queues.md).
O serviço é opcional e foi testado em RAM; não substitui o firmware completo
FreeRTOS preservado na flash nem migra automaticamente shell/desktop/aplicativos.

## Contratos e implementação

- `soc_serial_tx.c` separa USB/UART do display. `try_put` consulta o periférico
  uma vez e aceita um byte ou retorna ocupado; não espera nem faz heap. Token,
  contador de pacote e acesso MMIO são protegidos por exclusão local de IRQ.
  UART usa o limiar conservador de 120 já presente no console anterior; USB
  confirma escrita com WR_DONE, em watermark de software de 32 bytes ou flush.
  O watermark não é uma afirmação de capacidade máxima do hardware.
- NULL identifica o escritor legado até a reserva de boot por token estável.
  A reserva descarrega o pacote USB parcial anterior e dura até reset. Token
  errado não acessa MMIO. O token exige dono lógico único; não é uma fronteira
  de segurança/memória e não sincroniza núcleos distintos.
- `serial_output` vincula uma tarefa READY antes do escalonador, IRQs off, a um
  destino USB **ou** UART0. Nenhuma tarefa ou alocação é criada implicitamente.
  Reserva o TX inteiro: o destino não selecionado também não recebe escritor
  legado concorrente. RX tem sua reserva independente.
- Vários produtores usam `write`: registros de **1 a 64 bytes** são copiados
  inteiros, em ordem de enqueue, ou rejeitados com FULL/INVALID. Fila estática
  tem **8 registros**, mais um possível registro ativo do dono. Fonte deve
  permanecer estável até retorno; não pode sobrepor estado/armazenamento privado.
  Retorno OK permite reutilizar a fonte. Publicação e contadores são atômicos.
- `write` e snapshots são permitidos em thread/ISR no mesmo núcleo. `poll` é
  exclusivo do dono, em thread com IRQs habilitadas; rejeita outro contexto.
  Faz até **16 tentativas** de byte por chamada. Registra progresso/offset sem
  retirar outro registro enquanto o atual não terminar. BUSY preserva os bytes
  restantes; flush ocorre no fim de registro e de poll, sem converter CR/LF.
- Contadores de bytes escritos/registros completos representam **aceitação no
  FIFO**, não recebimento/ACK do host. Estatísticas locais preservam máscaras;
  uint32 faz wrap modular. Não há garantia de latência/throughput/justiça.
- Dono deve permanecer vivo e seu slot não pode ser reutilizado até reset.
  Teardown, timeout, descarte deliberado e wakeup de produtores não existem.
  Periférico desconectado pode manter registro pendente e fila cheia sem prazo.
- Console antigo usa o driver, mantendo CRLF e espelhamento de display antes
  da reserva. Depois, suas escritas/flush não acessam TX nem display. Chamadores
  que ativarem o serviço devem usar a API com retorno, não funções void legadas.
  O serviço não depende de display nem chama suas rotinas.
- Fatal trap continua com sink emergencial independente. Pode interromper o
  fluxo normal em falha fatal; não o enfileirar no serviço nem assumir integridade
  da fila/pilha normal para imprimir diagnóstico de corrupção.

Estrutura/assinaturas da fila, TCB, contexto de CPU, ABI de módulos e parsers não
mudam. CMake inclui o driver nos alvos nativos e o serviço apenas no diagnóstico
opt-in `MAIA_NATIVE_PROBE_SERIAL_TX=ON`/testes. Nenhum SDK runtime é importado.
O preenchimento explícito e limitado do registro evita dependência implícita de
libc gerada por inicialização agregada; ELF só admite projeto + libgcc.

## Testes locais e negativos

macOS arm64, C11, warnings-as-errors: suite raiz **16/16 PASS**.
ASan/UBSan saída/entrada/fila **3/3 PASS**. Novos headers e console compilam como
C++98 estrito. Fixture usa driver, serviço, console e fila reais, substituindo
somente MMIO/contexto de tarefa no host; não executa MMIO físico.

Cobre watermark/flush, UART ocupado/livre e destino inválido; CRLF/espelhamento
legado; reserva com pacote parcial; negação de escritor legado; init/contextos
inválidos; limites de tamanho; FULL sem aceitação parcial; fonte reutilizada
após enqueue; orçamento de 16 bytes; BUSY no primeiro byte e após progresso;
retomada exata; FIFO, snapshot e máscaras aninhadas incluindo enqueue de ISR.
UART físico/serviço UART completo não foram revalidados no alvo.

Cinco mutantes isolados são compilados e rejeitados: retirar verificação do dono;
dobrar orçamento; avançar offset em BUSY; retirar flush final; permitir console
legado após reserva. Nunca enviados ao device. O decoder de captura rejeita
registros ausentes, duplicados, trocados, corrompidos, CRLF indevido, falta de
FULL/BUSY/IRQs ou vazamento do escritor legado. Combinações inválidas de modos
são rejeitadas antes do acesso serial e no CMake.

```sh
cmake -S . -B /tmp/maia-host-contracts-20261110
cmake --build /tmp/maia-host-contracts-20261110 -j4
ctest --test-dir /tmp/maia-host-contracts-20261110 --output-on-failure
cmake -S . -B /tmp/maia-host-contracts-sanitized-20261110 -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build /tmp/maia-host-contracts-sanitized-20261110 --target serial_output_test serial_input_test queue_atomic_test -j4
ctest --test-dir /tmp/maia-host-contracts-sanitized-20261110 -R '^(serial_output_test|serial_input_test|queue_atomic_test)$' --output-on-failure
```

## Captura e regressão no Tab5

ESP32-P4 rev v1.3, GCC esp-14.2.0_20241119, RV32IMAC/ILP32, esptool 4.12.
Dois produtores enviam 40 registros de 48 bytes cada, identificados por fonte,
sequência e padrão. FULL é exercitado atrasando o dono 20 ticks; produtores
repetem a tentativa somente após delay nativo. Dono faz duas chamadas de poll
por iteração e dorme um tick. A segunda chamada permite observar BUSY do USB
no hardware; cada chamada de produção continua limitada a 16 tentativas.

Todos os 80 registros são comparados no host, com ordem por fonte e conteúdo
exato, sem aceitar mistura de bytes. Não há garantia de alternância/justiça
entre produtores: nesta carga os registros de A precederam os de B. Os próprios
sumário e PASS são publicados pelo serviço, após drenar os 80 registros;
o sumário abaixo exclui os três registros finais de relatório.

Captura final real, `native-probe-logs/tx-final.log`:

```text
TX_SUM records=80 bytes=3840 errors=0
TX_HW irq=260 full=344 busy=239
MAIA_NATIVE_PREEMPT_PASS
```

Primeiro ensaio também passou 80 registros/3.840 bytes e 260 IRQs, mas ainda não
exigia medir BUSY real. O ensaio final acrescenta essa verificação; não se trata
de uma falha de integridade anterior. Ambos os logs/ELFs são preservados.

Regressão de RX, com serviço TX desativado e console legado no novo driver:
256 valores de byte corretos, FULL=1, **305 IRQs**, errors=0 e heartbeat=21675174.
Gate pós-link passa nos dois perfis: sem símbolos estrangeiros/não resolvidos,
só objetos próprios e libgcc. Avisos existentes de LOAD RWX/segmentos vazios
removidos do ELF RAM permanecem. Nenhum tick/IRQ é sintetizado.

```sh
cmake -S . -B /tmp/maia-native-tx-20261111 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_SERIAL_TX=ON
cmake --build /tmp/maia-native-tx-20261111 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-tx-20261111/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-tx-20261111/maia_sched_probe_ram.bin --log /tmp/maia-tx-hardware-final.log --serial-output
```

Runner valida a janela SRAM antes de abrir a porta e reseta em finally; não
escreve flash/SD. Após o último reset, a interface maia.py confirmou shell,
SciCalc `2+3*4=14`, saída do módulo e desktop ativo. Logs neste diretório de
capturas; artefatos exatos, fontes/scripts e SHA256 no diretório irmão
`../maia-rtos-recovery/native-serial-output-20261111/`.

## Evidência, limites e próxima tarefa

[Referências primárias dos registradores](../hardware/EVIDENCE.md): EP1/EP1_CONF,
WR_DONE, IN_EP_DATA_FREE e UART STATUS/TXFIFO_CNT já existentes. Apenas leitura
de documentação/headers do SDK, sem includes/código de runtime externo.

Teste físico cobre USB com host conectado. Desconexão/reconexão física e UART
externo são **NOT RUN**; simulação local de ocupação não os certifica. Ensaiar
reconexão exige manter o dispositivo alimentado, enfileirar até FULL sem host,
retomar a leitura do mesmo firmware e conferir registro/offset e contagens.
Para UART, primeiro validar pinagem/fiação do BSP e adaptar a captura ao destino,
sem presumir que o endereço do controlador define os pinos físicos.

Sem SMP, espera bloqueante, wakeup por IRQ, cancelamento, espelhamento gerenciado
em múltiplos destinos, integração de VFS/shell/desktop ou conformidade POSIX.
O legado mantém tentativas limitadas, mas pode descartar byte ao esgotar o limite;
removida a estimativa não medida de ~5ms do comentário. Não reutilizar sua API
void como garantia de entrega na migração.

Arquivos da unidade: driver/header TX, serviço/header de saída, adaptação do
console, CMake, fixtures/runner e evidência/status/handoff. Próxima tarefa pequena:
esperas/despertares atômicos nativos para workers, com auditoria das primitivas
de sincronização e teste de notificação versus timeout, antes de migrar novos
drivers que precisem bloquear. Não transplantar laços de delay da árvore temporária.
