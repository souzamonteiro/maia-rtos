# Filas nativas para a migração dos drivers — 2026-11-11

Estado: **HARDWARE-TESTED, LIMITED**. Este incremento corrige a fila nativa para
concorrência local entre tarefas e IRQ. É um pré-requisito da comunicação dos
workers dos drivers; não substitui os drivers ESP-IDF nem torna o desktop atual
independente do FreeRTOS. O firmware funcional continua preservado na flash.

## Levantamento e escolha do incremento

Referências de contexto: [auditoria do runtime](freertos-dependencies-2026-11-10.md),
[proteção de pilhas](native-stack-protection.md) e
[atomicidade do escalonador](native-scheduler-atomicity.md).

| Fronteira examinada | Situação e decisão |
|---|---|
| Tarefas, preempção, idle e pilhas | Fundações nativas já têm testes específicos no alvo; não alterar o frame/contexto neste incremento |
| `kernel/ksync.c`, filas | Copiava payload e alterava head/tail/count sem exclusão. Corrigir antes de compartilhar eventos de drivers |
| `../maia-rtos-temp/kernel/ksync.c` | Mesmo núcleo de fila sem exclusão; recv_timeout usa repetição de delay e não é uma espera atômica. Não transplantar como solução |
| Mutexes e semáforos nativos | Continuam fora desta correção; não anunciar suas operações como seguras por causa das filas |
| Console nativo | USB/UART/teclado e display estão acoplados; contador de saída USB e consumidores não têm contrato de múltiplos donos. Precisa de fronteira/serviço próprio |
| Timer/display | Callback de manutenção DMA é chamado pelo trap e também por polling do console. Propriedade e execução dessa manutenção requerem auditoria separada antes de integrar display |
| Firmware completo | Continua com as dependências diretas/indiretas registradas na auditoria; nenhuma remoção nesse ELF é reivindicada aqui |

Leitura de referência, sem cópia/importação: o `queue.c` disponibilizado pelo
proprietário em `/Users/roberto/esp-idf/FreeRTOS/FreeRTOS/Source/queue.c` protege
a operação de cópia/contagem nos caminhos de tarefa e ISR. A correção Maia usa
sua própria fila e a primitiva local de IRQ já validada; não implementa os mesmos
serviços de bloqueio, listas de espera ou herança de prioridade do FreeRTOS.

## Contrato e mudanças

- `maia_queue_send`, `recv` e `count` preservam o estado anterior de MIE;
  checagem de capacidade, cópia completa e atualização dos índices/contagem
  pertencem à mesma seção crítica. Chamada aninhada com IRQ já desabilitada não
  reabilita interrupções. Isso serializa o acesso apenas no mesmo núcleo.
- Sem espera, heap, bloqueio ou transferência de propriedade do payload.
  A inicialização/reinicialização exige objeto sem usuários concorrentes.
  Objeto e armazenamento devem continuar válidos; acessar campos/armazenamento
  diretamente durante uso concorrente viola o contrato. Buffers de entrada/saída
  não podem sobrepor o objeto ou seu armazenamento e devem ser estáveis na chamada.
- Novo limite explícito: **256 bytes por item**, para limitar a quantidade copiada
  com IRQs desabilitadas. Use descritores com vida útil definida para dados maiores.
  Este limite não é uma medição de latência máxima. A API e o layout da struct
  são preservados; quem antes inicializava itens maiores receberá INVALID.
- `init` também rejeita multiplicação capacidade*tamanho e fim de endereço com
  overflow; falha deixa o objeto intacto. FULL/EMPTY não mudam fila ou payload.
  Todos os consumidores encontrados fora de dependências da plataforma usam
  itens de 4 bytes (`boot_main`, shell MVP/completa e fixture do kernel).
- Não há proteção contra outro núcleo, DMA ou código que corrompa os campos.
  Não adicionar estas filas a um caminho SMP sem sincronização entre núcleos.

Nenhum endereço MMIO novo. A base de exclusão é a CSR MIE/CSRRC[I]/CSRRS[I]
já documentada em [EVIDENCE](../hardware/EVIDENCE.md), seção de exclusão local do
escalonador. Não há nova afirmação sobre protocolos de periféricos.

## Verificação local

macOS arm64, C11 com warnings-as-errors. Build/CTest raiz: **14/14 PASS**.
ASan/UBSan: fila e fixtures integer/FP do kernel, **3/3 PASS**. Header público
passa C++98 estrito. A fixture cobre:

- 1.000 ciclos FIFO/wrap com capacidade 3 e itens de 256 bytes;
- cópia inteira com IRQ desabilitada, snapshot protegido e máscaras aninhadas;
- produtor de IRQ modelado somente na restauração da máscara;
- FULL/EMPTY, saída intacta, guardas externos ao armazenamento e argumentos nulos;
- limite de item, multiplicação/fim de endereço com overflow e init sem mutação
  em erro. Ponteiro artificial de overflow nunca é dereferenciado.

Mutantes locais, nunca enviados ao device, são rejeitados: retirar a máscara
local, retirar só a proteção do envio, retirar o limite de item ou as checagens
de overflow. Estes testes de algoritmo não simulam execução simultânea do host
nem substituem a prova de preempção no alvo.

Comandos (a partir da raiz):

```sh
cmake -S . -B /tmp/maia-host-contracts-20261110
cmake --build /tmp/maia-host-contracts-20261110 -j4
ctest --test-dir /tmp/maia-host-contracts-20261110 --output-on-failure
cmake -S . -B /tmp/maia-host-contracts-sanitized-20261110 -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build /tmp/maia-host-contracts-sanitized-20261110 --target queue_atomic_test kernel_mvp_test kernel_fp_frame_test -j4
ctest --test-dir /tmp/maia-host-contracts-sanitized-20261110 -R '^(queue_atomic_test|kernel_mvp_test|kernel_fp_frame_test)$' --output-on-failure
c++ -std=c++98 -Wall -Wextra -Wpedantic -Werror -I include -x c++ -fsyntax-only include/maia/kernel.h
```

## Evidência no Tab5 e recuperação

ESP32-P4 rev v1.3, GCC esp-14.2.0_20241119, RV32IMAC/ILP32, esptool 4.12.
Perfil opt-in `MAIA_NATIVE_PROBE_QUEUE=ON`, sem misturar outros workloads. Usa
uma fila de 7 slots e pacotes de 256 bytes: fonte, sequência e 62 palavras de
padrão verificadas. Dois produtores em tarefas fazem rajadas de 3 tentativas e
dormem por 3/4 ticks; um produtor em IRQ usa o timer real. Consumidor drena e
verifica ordem por fonte, integridade e contagens antes de encerrar.

A substituição do callback de display existe **apenas no diagnóstico** para
exercitar o contexto ISR real, conferir SP dentro da pilha IRQ e testar MIE
preservado. Não usa o driver de display nem afirma que ele foi migrado. Nenhum
tick sintético é injetado; o serviço de produção ainda precisará de integração.

Resultado final, captura real em `native-probe-logs/queue-final.log`:

```text
QUEUE irq=253 switches=276 errors=0 done=2
SOURCE id=0 sent=250 received=250 full=0
SOURCE id=1 sent=249 received=249 full=0
SOURCE id=2 sent=189 received=189 full=0
MAIA_NATIVE_PREEMPT_PASS
```

O total é 688 pacotes. O perfil final não provocou FULL no hardware; esgotamento
é coberto pela fixture local. A primeira carga, com produtores continuamente
prontos, terminou sem erro de integridade, mas falhou no piso arbitrário de 20
envios por fonte (84/499/7 aceitos). Isso não era uma garantia de justiça da fila.
A carga foi ajustada para rajadas periódicas; esse FAIL é preservado em
`native-probe-logs/queue-initial-workload.log`, sem apresentá-lo como PASS ou
como reprodução de corrupção. O ELF daquela primeira carga não foi preservado;
os artefatos de recuperação correspondem à carga final.

```sh
cmake -S . -B /tmp/maia-native-queue-20261111 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_QUEUE=ON
cmake --build /tmp/maia-native-queue-20261111 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-queue-20261111/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-queue-20261111/maia_sched_probe_ram.bin --log /tmp/maia-queue-hardware-final.log
```

O gate pós-link passa: objetos do projeto + libgcc, sem símbolos estrangeiros
ou não resolvidos. Permanecem avisos conhecidos de LOAD RWX e segmentos vazios
retirados no ELF RAM. O runner confina a imagem à janela SRAM já validada e
reseta em finally; nenhum comando escreve flash ou SD.

Após o reset, a interface `maia.py` confirmou shell responsivo, SciCalc
`2+3*4=14`, saída do módulo e desktop ativo. Captura:
`native-probe-logs/queue-desktop-restored.log`. ELF/bin/map/cache e fontes do
incremento, logs, comandos e SHA256 ficam preservados no diretório irmão
`../maia-rtos-recovery/native-driver-queues-20261111/`.

## Limites e próxima tarefa

Não implementados: esperas bloqueantes/timeout atômico, wakeup/reschedule por
fila, sincronização SMP, herança de prioridade e port dos drivers completos.
Mutexes/semafóros existentes ainda exigem revisão própria; os waits da árvore
temporária não são uma solução validada. Não há latência máxima medida nem
alegação de conformidade POSIX. Nenhum parser/EBNF, ABI de módulos, fonte do
SDK, firmware funcional ou configuração da plataforma Tab5 foi alterado.

Arquivos desta unidade: `kernel/ksync.c`, contrato em `include/maia/kernel.h`,
CMake, fixture local, diagnóstico RAM e registros de evidência/status/handoff.
Próxima tarefa pequena: definir a fronteira e o dono do I/O serial nativo,
separando o acesso USB/UART de teclado/display, e validar a passagem de entrada
entre serviço e tarefa usando a fila agora protegida. Display/PSRAM/DMA,
armazenamento e demais drivers permanecem em incrementos separados.
