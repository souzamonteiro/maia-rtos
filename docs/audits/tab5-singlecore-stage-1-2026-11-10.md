# Primeira etapa experimental: retirar o SMP do runtime legado

Data: 2026-11-10. **Experimento parcial; não aprovado como firmware padrão.**
Complementa o [levantamento inicial](freertos-dependencies-2026-11-10.md).

## Comparação solicitada e histórico

Foram comparados os fontes/configurações/documentos de
`/Users/roberto/M5Tab5-UserDemo` e `/Users/roberto/M5Tab5-UserDemo-test`, excluindo
Git, builds e managed components. Entre 2.029 arquivos comuns examinados,
somente três diferem: `platforms/tab5/main/app_main.cpp`, o CMake desse componente
e `idf_component.yml`. Há seis headers adicionais de fonte/módulos embarcados.
O JSON completo foi preservado nos artefatos desta etapa.

Os dois demos usam ESP-IDF e FreeRTOS. O demo original executa `app::Update`
numa tarefa com `vTaskDelay`. A variante `-test` substitui o supervisor por
shell/desktop/loader/compilador MaiaRTOS e aponta seu CMake para os fontes deste
checkout. BSP e HAL comparados permanecem iguais entre os dois demos. Portanto,
essa variante não é uma versão independente pronta para restaurar.

O proprietário identificou `1775c0d` como a importação da pasta da plataforma.
O diff confirma essa importação. Há chamadas FreeRTOS condicionais no header da
shell desde `0c83028` (2026-11-06). Isso demonstra presença no código, não qual
imagem foi instalada ou se aquele ramo era executado. O alvo raiz anterior a
`1775c0d` continua sendo o alvo nativo separado. Ainda não foi identificado um
artefato completo equivalente ao desktop atual que possa ser certificado como
última versão funcional sem FreeRTOS. Não foi feito checkout/reset da árvore.

O download em `/Users/roberto/esp-idf/FreeRTOS` declara **V11.3.3** em `task.h`.
O kernel efetivamente usado pelo SDK local declara **V10.5.1**, com alterações
do ESP-IDF. Não são intercambiáveis simplesmente trocando diretórios. Nenhum
fonte dos demos, SDK ou kernel FreeRTOS foi copiado para o kernel MaiaRTOS.

## Por que não apagar o escalonador de uma vez

O startup atual cria a tarefa principal e inicia o escalonador; USB, eventos,
rede, locks da libc/C++ e outros serviços bloqueiam e acordam através dele.
Deixar essas dependências como estão e retirar somente o escalonador impediria
a execução do sistema. O kernel nativo precisa assumir esses contratos em etapas.

Nesta primeira implementação foi criado um perfil **opt-in** que desativa o
SMP estrangeiro, mantendo provisoriamente tarefas/filas/locks do runtime legado.
Ele não entrega independência R-002, não introduz o kernel nativo no desktop e
não deixa o core secundário disponível para iniciar outro kernel em paralelo.
O SDK local inclusive rejeita combinações AMP inconsistentes em
`components/freertos/app_startup.c`. Não foram criados shims FreeRTOS no kernel.

Referência primária: ESP-IDF **v5.4.2**, seção
[Single-Core Mode](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-reference/system/freertos_idf.html#single-core-mode).
Ela descreve `CONFIG_FREERTOS_UNICORE` e a retirada do comportamento SMP nesse
modo. Esta opção continua sendo FreeRTOS, não um substituto do RTOS.

## Implementação

- `MAIA_TAB5_SINGLECORE_DRIVERS=OFF` mantém o perfil padrão. O experimento exige
  outro diretório de build: o CMake rejeita usar `platforms/tab5/build` com a opção.
- A configuração experimental reside no diretório de build, em
  `sdkconfig.singlecore`. Na primeira configuração, recebe uma cópia das opções
  conhecidas da placa, alterando a política de núcleo. O `sdkconfig` original
  não é regravado. Para atualizar essa base futuramente, use um build novo.
- Uma segunda guarda rejeita configurar o perfil sem `CONFIG_FREERTOS_UNICORE`.
- Teclado e jobs em background escolhem core 0 quando há um núcleo; mantêm
  core 1 no perfil padrão de dois núcleos. USB já usava core 0.
- O primeiro teste pesado expôs starvation do idle pelo compilador de prioridade
  5. No perfil de um núcleo, seu worker passou à prioridade idle, preservando a
  prioridade anterior no perfil padrão. O watchdog permanece habilitado.
  O teste seguinte encontrou outra falha, descrita abaixo; não há certificação
  de compilação pesada com essa mudança de prioridade.
- Adicionados testes das guardas de configuração e um diagnóstico real de
  jobs/transferência/compilação/execução de módulo pela interface `maia.py`.

Arquivos de implementação: `platforms/tab5/CMakeLists.txt`,
`platforms/tab5/main/CMakeLists.txt`, `sdkconfig.singlecore.defaults`,
`platforms/tab5/main/app_main.cpp` e `include/maia_shell/maia_shell.hpp`.
O kernel, parsers, bibliotecas MaiaScript, módulos e os diretórios de referência
do proprietário não foram alterados.

## Evidências dos builds

Build experimental:

```sh
cd platforms/tab5
source /Users/roberto/esp-idf/export.sh
idf.py -B /tmp/maia-tab5-singlecore-20261110 \
  -D MAIA_TAB5_SINGLECORE_DRIVERS=ON build
```

Resultado: build PASS. Configuração gerada:

```text
CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE=y
CONFIG_FREERTOS_UNICORE=y
CONFIG_FREERTOS_NUMBER_OF_CORES=1
```

Foram identificadas 20 diferenças de configuração, incluindo valores derivados,
watchdogs/afinidades do core removido e aliases legados. As configurações de
placa, memória e periféricos foram preservadas; o diff está nos artefatos.
O ELF não define `esp_startup_start_app_other_cores`,
`xTaskIncrementTickOtherCores` nem `esp_ipc_call_blocking`. A variável
`port_xSchedulerRunning` tem quatro bytes em vez de oito. **O símbolo
`vTaskStartScheduler` continua definido**, como esperado para esta etapa parcial.

O perfil padrão também foi recompilado com sucesso:

```sh
idf.py -D MAIA_TAB5_SINGLECORE_DRIVERS=OFF build
```

Seu `sdkconfig` e `sdkconfig.defaults` conservaram os hashes iniciais. O novo
binário padrão foi gerado, mas não foi usado como substituto da cópia preservada
na restauração. Warnings existentes do SDK/configuração permanecem.

| Artefato da etapa | SHA-256 |
|---|---|
| Experimento inicial, antes do ajuste de prioridade | `00faa0eb3fcaf3ac9ab8b8789780115a737ed64fe1215e7dc63990a61da94b13` |
| Experimento após ajuste de prioridade | `5e98bfad5d3adb3eb7ae2ed83e7582aee17ea57ed766349c1d9d9e41f3cad264` |
| Perfil padrão recompilado | `138949eda7f5e97796e4d82ef1605b39ccd2cacca3504ffcffc901ee823fcb5d` |
| Imagem funcional preservada, usada para restaurar o aparelho | `21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd` |

## Testes reais e falhas preservadas

| Verificação | Resultado |
|---|---|
| `python3 tests/host/test_tab5_singlecore_profile.py` | PASS: perfil padrão intacto, configuração isolada, preservação das opções e rejeição de configurações perigosas |
| `python3 tests/host/test_maiascript_text.py` | PASS |
| `maiascript/bin/test_compiler` no host | PASS: 32/32, também após esta etapa |
| Build experimental e build padrão | PASS |
| `test_maia_apps.py` no experimento inicial | PASS: atalhos, cálculos, erro de domínio, timer de 10 s, cancelamento, retorno ao desktop |
| Job `echo WORKER_BACKGROUND_OK &`, seguido de `jobs` | PASS: job termina com exit 0 no perfil de um núcleo |
| Compilar `/sd/alarm.ms` no perfil inicial | FAIL: watchdog acusa idle CPU0 sem executar enquanto `maiasc_psram` ocupa a CPU |
| Compilar programa sem includes, com 32 somas, após ajuste de prioridade | FAIL: load access fault em `xQueueSemaphoreTake`, vindo de destruição de mutex pthread/CST |
| Compilar o mesmo fonte na imagem original preservada | FAIL: `assert failed: xQueueSemaphoreTake queue.c:1709 (( pxQueue ))` |

O fault experimental foi simbolizado com seu ELF: `pthread_mutex_lock_internal`,
`pthread_mutex_destroy` e liberação de `std::shared_ptr<CSTNode>` aparecem no
caminho inspecionado. A comparação com a imagem original mostra uma falha
preexistente nessa área, mas não prova causa raiz idêntica para os dois dumps.
Não se deve atribuí-la automaticamente ao FreeRTOS, ao SMP ou à mudança de
prioridade. Os 32 testes host do compilador da auditoria anterior não exercitavam
esse backend C++/pthread no dispositivo.

O coletor inicial da comparação classificou a execução como `INCONCLUSIVE`
porque só reconhecia as mensagens Guru/watchdog. A transcrição efetivamente
contém a asserção acima e a entrada no GDB stub. O diagnóstico permanente agora
reconhece também `assert failed:`. Não foram editados os logs recebidos.

Os testes de transporte precisaram respeitar dois limites existentes: 63
caracteres pendentes no ring de teclado e **62 caracteres úteis** na linha do
terminal do desktop. Tentativas de comandos maiores truncaram a entrada; esses
resultados não foram tratados como falha do compilador. O script agora limita
comandos, envia blocos cadenciados e verifica os bytes de cada transferência.

## Restauração, limitações e próximo passo

Somente a aplicação em `0x10000` foi gravada nos testes, por `idf.py app-flash`.
Bootloader e tabela de partições não foram substituídos. Após as falhas,
esptool restaurou a imagem funcional preservada e verificou os dados gravados.
Os arquivos temporários identificados nos testes foram removidos ao final.
A verificação de restauração está em `maia-singlecore-restored-verification.log`.
Ela passou: eco da shell, limpeza dos arquivos identificados, `2+3*4=14` e
retorno ao desktop. A imagem experimental não ficou instalada.

Logs, comparação dos demos, configurações, ELF/mapa/binário experimental e diff
da implementação foram preservados em:

```text
../maia-rtos-recovery/bf044d3-20261110/experiments/singlecore-20261110/
```

Não foram certificados câmera, rede, USB de armazenamento, microfone, toque
físico, qualidade de áudio ou latências no perfil experimental. A compilação no
dispositivo falha e impede sua promoção. O escalonador completo do FreeRTOS e
suas dependências indiretas ainda não foram removidos. Esta etapa não implementa
SMP nativo, timer/preempção nativos ou port completo dos serviços dos drivers.

Próximo passo pequeno: reproduzir o problema de mutex/ownership do compilador
num diagnóstico mínimo, separando biblioteca C++, parser e allocator. Preservar
ABI/gramática e testar falhas de recursos. Depois validar os contratos nativos
de tarefas/waits/IRQ e portar os serviços dos drivers individualmente, antes
de retirar o último escalonador estrangeiro do desktop completo.
