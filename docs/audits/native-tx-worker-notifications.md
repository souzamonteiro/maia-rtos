# Worker TX guiado por notificação — 2026-11-11

Estado: **HARDWARE-TESTED, LIMITED**, USB-Serial/JTAG no ESP32-P4 rev v1.3.
Integra a [saída serial nativa](native-serial-output.md) às
[notificações do kernel](native-task-notifications.md). Serviço opcional e
worker de referência no diagnóstico RAM; não ativa uma tarefa oculta nem migra
automaticamente o desktop completo, que continua no firmware FreeRTOS preservado.

## Mudança de produção e contrato

- `maia_serial_output_write` publica registro e estatísticas antes de notificar
  o dono, na mesma seção de exclusão local. FULL/INVALID não notificam. Mantém
  o contrato de aceitação do registro inteiro e a vida útil permanente do dono;
  notify não pode falhar para esse dono vivo. Dono que termina/reutiliza o slot
  viola o contrato, sem promessa de recuperação dessa violação.
- Nova `maia_serial_output_wait()` é exclusiva do dono em thread com IRQs
  habilitadas. Retorna INVALID antes do init e PERM no contexto errado. Se há
  registro na fila **ou bytes ativos pendentes**, retorna OK imediatamente.
  Caso contrário entra na espera FOREVER do kernel, sem polling ou delay interno.
- Escrita entre a checagem de vazio e o ECALL deixa uma notificação persistente;
  escrita após o bloqueio acorda a tarefa. O bit não é apagado pelo serviço.
  Uma notificação antiga/coalescida pode retornar OK sem dados: o dono sempre
  volta a fazer poll e conferir seu predicado, sem interpretar o evento como
  contagem de registros ou confirmação de entrega.
- O dono reserva sua caixa de notificação para TX, ou coordena explicitamente
  outros usos. Init ainda não cria tarefa nem escolhe política de escalonamento.
  `poll` e os layouts de fila/estatísticas permanecem compatíveis.
- **FIFO ocupado não é fila vazia.** `wait()` retorna imediatamente com bytes
  pendentes; usá-la como espera de prontidão do periférico causaria polling.
  Não há IRQ TX ligada a um wakeup neste incremento. O worker de referência usa
  `maia_task_wait_notification(1)` após BUSY: acorda por novo registro ou timeout
  de um tick e tenta novamente, preservando offset. Não dorme para sempre com
  dados pendentes sem uma fonte de despertar.

Padrão do dono (fluxo resumido; erros reais devem ser tratados pelo chamador):

```c
for (;;) {
    maia_status_t result = maia_serial_output_poll();
    if (result == MAIA_ERR_EMPTY) {
        /* OK apenas sinaliza que é hora de conferir a fila outra vez. */
        maia_serial_output_wait();
    } else if (result == MAIA_ERR_BUSY) {
        maia_task_wait_notification(1U); /* Sem IRQ TX, retry finito. */
    } else if (result != MAIA_OK) {
        /* Tratar erro de contexto/serviço; não repetir cegamente. */
        break;
    }
}
```

Não há mudança no kernel/trap, MMIO, configuração de display, ABI de módulos,
parser/lexer/EBNF ou backend funcional ESP-IDF. Código próprio, sem importação
de fonte/header/runtime FreeRTOS. Exclusão local continua sendo single-core,
sem barreira de cache/DMA ou sincronização SMP.

## Testes locais e negativos

Suite raiz **18/18 PASS**, ASan/UBSan TX + notificações inteira/FP **3/3 PASS**,
header público C++98 estrito PASS. A fixture TX mantém seu modelo de registrador
realizando chamadas ao código de driver/fila/serviço; para integração com notify,
modela a API de tarefa e observa que o registro/contadores já estão publicados
quando o sinal chega. Não executa suspensão real no host.

Casos novos: sem sinal em FULL/INVALID; máscara preservada na notificação de ISR;
recusa de wait no contexto errado; trabalho enfileirado e registro ativo sozinho
não entram em espera; sinal antigo seguido de espera sem sinal; injeção de escrita
após checagem de vazio e antes do consumo da notificação, sem perder o evento.
O modelo registra o ponto onde bloquearia; quem prova bloqueio é o alvo nativo.

Cinco mutantes isolados são compilados e rejeitados: retirar sinalização;
sinalizar antes da publicação; ignorar registro ativo; nunca esperar quando
vazio; usar espera periódica de um tick em vez de FOREVER na fila vazia. Nenhum
mutante foi enviado ao device. Decoder rejeita falta dos períodos quietos,
retry finito, IRQ writer ou registro disparador, além das verificações de
integridade/ordem do perfil TX anterior. Perfis incompatíveis são rejeitados
antes de acesso serial/CMake build.

```sh
cmake -S . -B /tmp/maia-host-contracts-20261110
cmake --build /tmp/maia-host-contracts-20261110 -j4
ctest --test-dir /tmp/maia-host-contracts-20261110 --output-on-failure
cmake -S . -B /tmp/maia-host-contracts-sanitized-20261110 -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build /tmp/maia-host-contracts-sanitized-20261110 --target serial_output_test notification_integer_test notification_fp32_test -j4
ctest --test-dir /tmp/maia-host-contracts-sanitized-20261110 -R '^(serial_output_test|notification_integer_test|notification_fp32_test)$' --output-on-failure
```

## Evidência no Tab5

GCC esp-14.2.0_20241119, RV32IMAC/ILP32, esptool4.12, RAM apenas. Novo perfil
`MAIA_NATIVE_PROBE_SERIAL_TX_EVENT=ON`, com duas tarefas produtoras e dono de TX.
Antes do primeiro envio, todas bloqueiam; durante 20 observações de IRQ de timer,
owner fica BLOCKED com `polls` e `run_ticks` inalterados. Timer apenas acorda os
produtores; quem acorda o dono é a publicação pelo serviço.

As duas tarefas publicam os mesmos 80 registros de 48 bytes do ensaio anterior;
FULL e BUSY continuam exercitados e o host verifica todos os bytes/ordem por
fonte. Ao terminar, ambos os produtores retornam/EXITED e o dono bloqueia de novo
na fila vazia. Outras 21 observações confirmam `polls` e `run_ticks` estáveis.
O callback então publica **um registro real via write em contexto IRQ**, sem
notificar o dono diretamente; o dono acorda e transmite `TX_EVENT_TRIGGER`.
O callback só existe no diagnóstico, não substitui IRQ de prontidão do USB.

Captura real em `native-probe-logs/tx-event-final.log`:

```text
TX_EVENT_TRIGGER
TX_SUM records=80 bytes=3840 errors=0
TX_HW irq=302 full=344 busy=313
TX_EVENT head=20 tail=21 idle=301
TX_WAIT empty=2 retry=313 timeout=240 irq_write=1
MAIA_NATIVE_PREEMPT_PASS
```

Sumário de payload exclui o disparador de 17 bytes e os cinco registros finais
de relatório, todos publicados pelo serviço. Antes do relatório, contadores
nativos verificam 81 registros aceitos/completos e 3857 bytes. Os 240 timeouts
são retries de FIFO ocupado; a fila vazia usa espera indefinida. Não há tick
sintético nem polling do dono nos períodos quietos. A janela exata antes do
ECALL é exercitada no modelo local; o alvo verifica o despertar depois de
bloquear e a integração real serviço/kernel, sem alegar reprodução exaustiva de
interleavings. Idle ticks não são medição de potência ou percentual de CPU livre.

Regressão do perfil TX anterior, sem usar wait(): PASS, 80 registros/3840 bytes,
260 IRQs, FULL=344, BUSY=239, errors=0. O novo sinal não interrompe o delay antigo;
a pendência pode coalescer sem mudar o protocolo de transmissão. Gates pós-link
passam nos dois perfis: projeto + libgcc, sem símbolos estrangeiros/não resolvidos.
Avisos antigos de LOAD RWX e segmentos vazios removidos no ELF RAM permanecem.

```sh
cmake -S . -B /tmp/maia-native-tx-event-20261111 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_SERIAL_TX_EVENT=ON
cmake --build /tmp/maia-native-tx-event-20261111 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-tx-event-20261111/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-tx-event-20261111/maia_sched_probe_ram.bin --log /tmp/maia-tx-event-hardware.log --serial-output-event
```

Runner confina imagem à janela SRAM existente e reseta em finally. Flash/SD
não são escritos. Após o último reset, maia.py confirma shell, SciCalc
`2+3*4=14`, saída do módulo e desktop ativo. Logs junto ao relatório; ELFs/bins/
mapas/cache/fontes/scripts e SHA256 no diretório irmão
`../maia-rtos-recovery/native-tx-worker-notifications-20261111/`.
Referências primárias já registradas em [EVIDENCE](../hardware/EVIDENCE.md),
seções de timer/trap/notificação e TX; nenhum fato novo de registrador/pinout.

## Limites e próximo passo

Sem IRQ de prontidão TX, espera de produtores por espaço, teardown/rebind,
SMP, tratamento de reconexão física, UART físico revalidado ou ACK de entrega
no host. Buffers e dono permanecem estáticos até reset; erro de vida útil não
é recuperado. API de polling continua disponível para chamadores existentes.

Arquivos: serviço/header TX, CMake, fixture local, novo diagnóstico RAM, runner,
checks de captura e evidência/status/handoff. Nenhuma configuração do painel
foi alterada. Próximo trabalho: auditar e preservar a sequência de inicialização
do display funcional, comparando fontes/artefatos e dependências, antes de portar
seu driver. PSRAM/cache/DMA, reset/alimentação e variantes de painel continuam
riscos próprios; sucesso no USB não valida o display. POSIX permanece DESIGN.
