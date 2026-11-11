# Projeto futuro: camada POSIX mínima para o MaiaRTOS

Estado: **DESIGN — não implementado**. Registro da intenção do proprietário em
2026-11-11. Este documento planeja uma etapa posterior à migração funcional sem
FreeRTOS; não inicia sua implementação nem altera a próxima etapa dos drivers.

## Objetivo e modelo

Oferecer um subconjunto POSIX útil para portar bibliotecas e aplicações C/C++,
com **mínimas alterações no sistema que estiver pronto quando esta etapa começar**.
A API nativa MaiaRTOS continua sendo a base e permanece utilizável diretamente.

O modelo inicial é um único processo implícito, com múltiplas threads que
compartilham memória. Isso preserva a natureza multitarefa do MaiaRTOS: cada
thread tem execução, pilha, contexto e prioridade próprios. Não exige introduzir
processos isolados, memória virtual ou um segundo escalonador.

PSE51 (Minimal Realtime System Profile), associado ao histórico IEEE 1003.13,
é uma referência de escopo, não uma declaração de conformidade. O subconjunto
inicial será identificado por uma matriz de interfaces e limitações. Certificação
ou conformidade integral com qualquer perfil exige projeto e validação separados.
POSIX completo continua fora do escopo da arquitetura v0.1.

## Regra de preservação e dependências

```text
Aplicações que optarem por POSIX
             |
Camada POSIX opcional e biblioteca de adaptação
             |
APIs públicas nativas de serviços e kernel MaiaRTOS
             |
Arquitetura / SoC / BSP
```

- Preferir adaptação na borda: kernel e drivers não incluem cabeçalhos POSIX,
  não chamam a camada de adaptação e não passam a depender dela.
- Manter APIs/ABI nativas, comportamento dos aplicativos, formato dos módulos e
  contratos de pilha/contexto existentes. Mudança inevitável terá justificativa,
  escopo, teste de regressão e commit próprio antes de ser usada pelo adaptador.
- Não acessar TCBs, filas privadas ou detalhes do trap diretamente do adaptador.
  Capacidade ausente deve virar uma pequena primitiva nativa reutilizável.
- Não substituir mecanismos funcionais para imitar internamente um Unix.
  Também não simular bloqueio com laços de polling ou yields repetidos.
- Não introduzir FreeRTOS, Arduino, ESP-IDF runtime ou implementações copiadas
  desses projetos. A camada POSIX não é uma compatibilidade com FreeRTOS.
- Ser opcional no build, inicialmente desativada. Seus cabeçalhos e símbolos
  não devem sobrepor os da libc nem contaminar builds nativos por padrão.
  Diretórios/nomes definitivos serão definidos após a auditoria futura.

## Pré-condições e auditoria na época da implementação

Antes de escrever o adaptador, registrar o commit funcional de referência,
artefatos de recuperação e regressões do sistema completo. Confirmar ausência
de runtime FreeRTOS no firmware equivalente, e não apenas em um diagnóstico.
SMP não é requisito para iniciar POSIX; se já existir, o adaptador deverá usar
os contratos de sincronização nativos então validados.

Inventariar novamente APIs públicas, libc/libstdc++, linker e serviços realmente
implementados. Para cada interface candidata, registrar: semântica exigida,
primitiva nativa disponível, lacuna, custo de RAM/tempo, contexto permitido,
limites, teste e alteração mínima proposta. Priorizar um consumidor real.

O checkpoint atual é apenas referência, não um desenho obrigatório para o futuro:
`include/maia/kernel.h` expõe tarefas com entrada `void (*)(void *)`, identificação,
delay e primitivas de tentativa de sincronização; não oferece publicamente todo
o ciclo POSIX de retorno/join/detach, espera atômica ou TLS. Preservar `tp` no
contexto não constitui, sozinho, um runtime TLS. Declarações em cabeçalhos também
não provam que cada serviço tenha sido validado no hardware.

## Escopo candidato, a confirmar pela matriz de lacunas

| Grupo | Interfaces candidatas | Condição para oferecer suporte |
|---|---|---|
| Tempo | `clock_gettime`, `clock_getres`, `nanosleep` | Conversões seguras, resolução declarada e bloqueio por prazo; monotônico primeiro. Relógio civil somente com serviço correspondente |
| Threads | `pthread_create`, `pthread_self`, `pthread_equal`, `pthread_exit`, `pthread_join`, `pthread_detach` e atributos necessários | Adaptar retorno `void *`, propriedade de pilha, término, recuperação de recursos e identidade estável durante a vida do objeto |
| Sincronização | Mutexes, semáforos e variáveis de condição | Esperas nativas atômicas, despertar correto, timeout e ciclo de vida definidos |
| Estado por thread | `errno`, `pthread_key_create`, `pthread_getspecific`, `pthread_setspecific` e funções relacionadas necessárias | Integração com a libc e limpeza no término; estado independente para tarefas participantes |
| Arquivos/dispositivos | `open`, `read`, `write`, `close`, `lseek`, `fstat` | Adaptar VFS/dispositivos existentes, descritores, acesso concorrente e operações parciais; não criar outro filesystem |

A tabela é um conjunto de candidatos, não uma promessa de entregar tudo na
primeira versão. Cada grupo só é anunciado depois de seus contratos e testes.
Um consumidor pode exigir funções auxiliares ou semânticas além da lista.
Não prometer compatibilidade binária com Linux; o objetivo inicial é portar
fontes recompilados com headers, libc e ABI definidos para o MaiaRTOS.

## Contratos que não podem ser reduzidos a renomear funções

- Erros: documentar o retorno específico de cada função e o uso de `errno`;
  funções pthread normalmente retornam o número do erro diretamente. Não usar
  uma única conversão genérica dos códigos nativos para todas as famílias.
- Threads: não tratar índice reutilizável de tarefa como identidade eterna.
  Controlar término versus join/detach, resultado, vida da pilha e reuso de slots,
  inclusive convivência entre threads POSIX e tarefas nativas.
- Esperas: resolver atomicamente sinalização versus bloqueio/timeout. Para
  condições, liberar o mutex e entrar em espera sem perder notificações;
  readquirir o mutex ao retornar e admitir reavaliação do predicado pelo chamador.
- Prioridades: mapear faixas e políticas reais. Herança/proteção de prioridade
  só será anunciada quando implementada e validada; não aceitar atributos sem
  cumprir sua semântica. Não inferir `SCHED_FIFO`/`SCHED_RR` do nome do escalonador.
- Tempo: definir relógio de cada espera, arredondamento, limites e overflow;
  relógio monotônico não deve ser apresentado como hora civil.
- Recursos: definir limites configuráveis e falha por esgotamento. Alocação
  deve ocorrer em contexto permitido, com propriedade e liberação explícitas;
  nenhuma alocação implícita em ISR ou promessa de latência sem medição.
- Features ausentes: não retornar sucesso fictício nem anunciar macros de
  conformidade indevidas. Documentar ausência e usar erros permitidos pela
  especificação da interface quando ela estiver exposta. ISR não é thread POSIX;
  permissões ISR das APIs nativas não são transferidas automaticamente.
- Libc/C++: auditar headers, `errno`, TLS, alocador e suporte a threads antes de
  ligar bibliotecas. Impedir ligação acidental com pthread do host ou shims do
  firmware legado. Uma API pthread parcial não garante suporte a `std::thread`.

## Entrega incremental e aceitação

1. **Inventário e contrato:** escolher versão normativa POSIX, primeiro consumidor
   e matriz de suporte. Estimar mudanças; se houver refatoração ampla, reduzir
   o subconjunto ou adiar a interface, em vez de reescrever a base funcional.
2. **Primeiro grupo pequeno:** build opcional isolado e uma família de APIs útil.
   Criar primitivas nativas faltantes em incrementos separados e reutilizáveis.
3. **Expandir por necessidade:** acrescentar grupos da matriz somente após testar
   os anteriores. Não modificar shell/MaiaScript para obrigar uso de POSIX.
4. **Validar cada incremento:** contratos de retorno/erro, esgotamento, atributos
   não suportados, prazos, contenção, término/reuso e ausência de vazamentos.
   Usar host/sanitizadores para algoritmos e hardware para concorrência real.
5. **Regredir o sistema completo:** POSIX desativado e ativado; tarefas nativas
   convivendo com threads POSIX; desktop, shell, MaiaScript/apps e drivers.
   Conferir mapa/símbolos/dependências de link e custo de memória. SMP será
   testado se suportado, sem usar exclusão local de IRQ como trava entre núcleos.
6. **Publicar um ponto de retorno:** logs, configuração, limites, documentação,
   commit descritivo e push após cada unidade verificada, mantendo recuperação.

Aceitação da primeira entrega: programa C recompilado usando somente o
subconjunto declarado executa no alvo; casos negativos falham corretamente;
regressões nativas passam; build desativado não exige a camada; imagem final
continua sem runtimes proibidos. Não basta compilar wrappers ou passar mocks.

Fora da primeira etapa: `fork`, `exec`, processos isolados, memória virtual,
sinais complexos, cancelamento assíncrono, sockets/rede e conformidade integral.
Qualquer ampliação precisa de escopo e testes próprios.

## Referências e situação deste registro

- [Arquitetura nativa e não objetivos](../architecture/OVERVIEW.md).
- [Requisitos, especialmente R-002](../requirements/REQUIREMENTS.md).
- [Contexto/pilhas atualmente validados](../audits/native-stack-protection.md).
- [Zephyr: perfis POSIX e situação histórica do IEEE 1003.13](https://docs.zephyrproject.org/latest/services/portability/posix/aep/index.html).
  Referência de perfil, não fonte de código nem definição do nosso subconjunto.
- [The Open Group: POSIX.1-2024, pthread_create](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_create.html)
  e [convenções gerais de interfaces](https://pubs.opengroup.org/onlinepubs/9799919799/functions/V2_chap02.html).
  Revalidar a edição escolhida e cada contrato na etapa de inventário; o acesso
  automatizado a estas duas páginas retornou HTTP 403 nesta consulta.

Este incremento adiciona apenas planejamento e referências no roadmap/status/
handoff. Implementação POSIX e testes de execução: **NOT RUN — não existem nesta
etapa**. Sem alteração de firmware, parser, ABI, configuração ou acesso ao device.
Validação deste registro: links locais e consistência do diff. Risco principal
futuro: lacunas semânticas exigirem mais primitivas que o previsto; o inventário
prévio e os incrementos separados limitam esse impacto. Próxima tarefa imediata
continua sendo a auditoria/integração incremental dos drivers nativos.
