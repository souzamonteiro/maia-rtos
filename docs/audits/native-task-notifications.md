# Espera e notificação nativas por tarefa — 2026-11-11

Estado: **HARDWARE-TESTED, LIMITED**, núcleo único ESP32-P4 rev v1.3.
Incremento seguinte aos [workers seriais](native-serial-output.md): acrescenta
bloqueio/despertar por evento, sem laços de delay, FreeRTOS ou runtime externo.
Não migra automaticamente drivers/desktop nem altera a configuração do display.
O firmware funcional completo continua preservado na flash e usa FreeRTOS.

## API e protocolo

- `maia_task_notify(id)` publica um evento em tarefa viva, de boot/thread/ISR,
  preservando a máscara local. Índice inválido/UNUSED/EXITED retorna INVALID.
  Tarefa bloqueada nesta espera passa a READY; uma tarefa dormindo por delay não
  é acordada antecipadamente. Sem espera ativa, retém um bit pendente: eventos
  repetidos coalescem, não formam um contador. Sem payload ou alocação.
- `maia_task_wait_notification(timeout_ticks)` exige thread em RUNNING com IRQs
  habilitadas; outro contexto retorna PERM sem executar ECALL. Retorna OK ao
  consumir um evento, EMPTY se timeout=0 sem pendência, TIMEOUT na expiração
  finita. `MAIA_TASK_WAIT_FOREVER=UINT32_MAX` nunca expira; os demais valores
  positivos são duração finita em ticks. Não há limite máximo de latência medido.
- Consumo/teste/bloqueio são uma transação no trap **após salvar o contexto**,
  identificada por ECALL rotulado, como delay/exit. Notificação entre a validação
  pública e o ECALL fica pendente e é consumida ali: não há janela de evento
  perdido. Deadline relativo é calculado no commit do trap, não antes dele.
- Timeout e post usam a mesma exclusão local; a primeira conclusão prevalece.
  Post que acorda BLOCKED não cria também uma segunda pendência; outro post
  depois do wake fica para a próxima espera. Post após timeout, antes de a tarefa
  voltar a executar, não transforma TIMEOUT em sucesso: fica pendente separado.
- Resultado fica no TCB privado até seleção. Somente **depois de validar guardas,
  alinhamento e frame completo**, o seletor escreve o retorno em a0/slot6 e limpa
  o marcador uma vez. Não altera a0 de interrupções/ecalls sem espera pendente.
  Layout/offsets de frame inteiro/RV32F e ABI de módulos permanecem iguais.
- Expiração usa a comparação modular uint64 já usada para delays; FOREVER tem
  flag explícito, não depende de um deadline numérico especial. Bloqueados não
  são candidatos READY e não acumulam ticks de execução enquanto aguardam.
  Sem tarefas prontas, o idle privado/WFI continua funcionando.
- notify não faz yield implícito: READY será selecionada no próximo ponto de
  escalonamento. Uma thread pode ceder explicitamente; um handler IRQ precisa do
  epílogo de escalonamento do port. O teste usa a IRQ real de timer já suportada;
  isso não implementa roteamento de IRQs de todos os futuros drivers.
- Estado limpo na inicialização/reuso do slot e no exit. IDs continuam índices
  reutilizáveis; chamador deve manter uma identidade viva, sem usar IDs obsoletos.
  Não há handles com geração, cancelamento, espera em vários objetos ou SMP.

A notificação sinaliza um **predicado**, não substitui uma fila de dados. Produtor
publica dados antes de notificar; consumidor drena/reverifica o predicado. Não
usar um evento coalescente como contagem de pacotes. Não há barreira de cache/DMA
nem transferência de propriedade de buffers nesta API. Mutexes, semáforos e suas
esperas continuam exigindo contratos próprios; não são corrigidos por este patch.

## Implementação e referências

Mudanças pequenas no TCB privado/seletor/tick em `kernel/ksched.c`, declarações
públicas/privadas, stub ECALL em `arch/riscv/trap_context.S` e despacho do port em
`soc_timer.c`. Nenhum novo MMIO, pinout, clock, fonte de IRQ ou sequência de boot.
São usados os [contratos de contexto](../architecture/RISCV_CONTEXT.md) e as
referências primárias já registradas em [EVIDENCE](../hardware/EVIDENCE.md),
seções de exclusão local, CLIC/ECALL e timer. Código original do projeto; nenhum
header/implementação FreeRTOS/ESP-IDF/Arduino foi copiado ou ligado.

Publicação/post/commit são O(1); o scan de tick/seletor continua limitado a
16 slots. A atualização do frame não exige copiar o frame inteiro. Isso limita
trabalho algorítmico, não estabelece WCET ou garantia de tempo real.
O fallback de arquitetura no host retorna PERM e não emula suspensão nativa;
fixtures exercitam o modelo privado e retornos imediatos, com essa limitação.

## Testes locais

macOS arm64, C11 com warnings-as-errors. Suite raiz **18/18 PASS**. Fixtures de
notificação inteira/FP32 e atomicidade/idle sob ASan/UBSan **4/4 PASS**. Headers
público/privado passam C++98 estrito. Modelos cobrem:

- posts antecipados/coalescência e injeção na restauração de IRQ entre validação
  pública e entrada no trap, sem perder a notificação;
- bloqueio, post antes do deadline, timeout antes do post, segundo post retido,
  retorno EMPTY e nenhum resultado sobrescrito por uma notificação tardia;
- deadline cruzando UINT64_MAX; FOREVER além de UINT32_MAX e no wrap uint64;
  maior timeout finito não expira antecipadamente;
- delay não interrompido por notify, ambos bloqueados/idle, IDs inválidos,
  exit/reuso sem pendência antiga e máscaras aninhadas;
- a0 de contexto sem retorno pendente preservado, entrega única e recusa de
  escrever o retorno num frame com guarda corrompida.

Cinco mutantes de fonte são compilados isoladamente e rejeitados: perder o bit
pendente; sobrescrever resultado ao postar tarde; tratar FOREVER como finito;
reaplicar retorno em seleções futuras; acordar para SLEEPING em vez de READY.
Nenhum mutante foi enviado ao device. Combinar workload de notificações com
outro perfil é rejeitado pelo CMake.

```sh
cmake -S . -B /tmp/maia-host-contracts-20261110
cmake --build /tmp/maia-host-contracts-20261110 -j4
ctest --test-dir /tmp/maia-host-contracts-20261110 --output-on-failure
cmake -S . -B /tmp/maia-host-contracts-sanitized-20261110 -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build /tmp/maia-host-contracts-sanitized-20261110 --target notification_integer_test notification_fp32_test scheduler_atomic_test scheduler_idle_test -j4
ctest --test-dir /tmp/maia-host-contracts-sanitized-20261110 -R '^(notification_integer_test|notification_fp32_test|scheduler_atomic_test|scheduler_idle_test)$' --output-on-failure
```

## Prova no Tab5 e regressões

GCC esp-14.2.0_20241119, RV32IMAC/ILP32, esptool4.12. Somente RAM, mesma janela
SRAM validada e reset automático em finally. Novo perfil
`MAIA_NATIVE_PROBE_NOTIFY=ON`: duas tarefas, sem busy loop para esperar eventos.
Uma espera posts da IRQ/da outra tarefa, timeout e FOREVER durante 40 rodadas;
a controladora também bloqueia. O stub do callback de display existe somente
no diagnóstico para observar e postar de uma IRQ de timer real; não configura
ou dirige o painel. Nenhum tick/notificação de hardware é sintetizado.

Callback verifica SP/guarda da pilha IRQ, rejeição de wait em contexto IRQ e
`run_ticks` estável enquanto a tarefa está BLOCKED. A controladora publica dois
posts com uma máscara externa para exigir que ambos precedam o retorno do
worker; a API não promete atomicidade entre chamadas separadas. O worker retorna
normalmente ao final, e notify em seu slot EXITED é rejeitado.

Resultado final real em `native-probe-logs/notify-final.log`:

```text
NOTIFY rounds=40 waits=282 timeouts=80 isr_worker=80 isr_controller=41 thread_posts=80 errors=0
WAIT_HW irq=750 ecall=325 blocked_checks=520 idle_ticks=749 irq_perms=750
MAIA_NATIVE_PREEMPT_PASS
```

Os 325 ecalls incluem waits, delay final da controladora e exit normal. Não são
750 traps de wait dentro da IRQ: essas tentativas retornaram PERM sem ECALL.
Idle ticks indicam o contexto selecionado, não uma medição de potência/latência.

Regressão RV32F/mista/lifecycle: 250 IRQs/ticks, 54.012 ecalls, 54.137 switches,
125 creates/returns/nesting checks, 4 slots, zero erros de registrador/IRQ-stack,
MIL=0. Falha ilegal16: causa bruta `0x30000002`, PC exato `0x4ff01402` do ELF,
sem retomada indevida. Gate pós-link passa em todos: objetos próprios + libgcc,
sem símbolos estrangeiros ou não resolvidos. Avisos prévios de LOAD RWX e
segmentos vazios retirados do ELF RAM permanecem.

```sh
cmake -S . -B /tmp/maia-native-notify-20261111 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_NOTIFY=ON
cmake --build /tmp/maia-native-notify-20261111 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-notify-20261111/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-notify-20261111/maia_sched_probe_ram.bin --log /tmp/maia-notify-hardware-final.log
```

Primeiro ensaio também passou; final acrescenta a máscara externa no par de posts
para eliminar uma suposição de timing do teste. Logs/ELFs de ambos preservados.
Uma revisão automática de permissão da repetição expirou antes de executar;
a única nova tentativa foi liberada e concluiu com PASS. Não houve flash/SD.
Após reset, maia.py confirma shell, SciCalc `2+3*4=14`, saída e desktop ativo.
Artefatos/scripts/fontes e SHA256 no diretório irmão
`../maia-rtos-recovery/native-task-notifications-20261111/`.

## Limites e próximo incremento

Sem mutex bloqueante/herança de prioridade, semáforo contador, lista de waiters,
SMP, cancelamento, IRQs genéricas de drivers, novo runtime C++ ou port completo
de shell/desktop. Não há nova garantia de justiça entre prioridades/latência.
Nenhum parser/lexer/EBNF, ABI de módulos ou configuração do display foi alterado.
As dificuldades de display/PSRAM/cache/DMA/reset/painel continuam um risco próprio;
não reutilizar sucesso destes testes como validação dessas sequências.

Próximo incremento pequeno: fazer o worker TX bloquear por notificação quando
sua fila estiver vazia e notificar após publicar um registro; preservar a fila e
validar a janela entre drenar e esperar. Espera por FIFO ocupado/IRQ do periférico
é outro contrato, não substituir por espera infinita sem fonte de wakeup.
