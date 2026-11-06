# Auditoria de Fronteira do MaiaShell (Shell Boundary Audit)

## 1. Contexto e Objetivo
Esta auditoria analisa a fronteira entre o núcleo portátil do **MaiaShell**, o sistema operacional **MaiaRTOS** e os módulos nativos **MNM-1**, atendendo ao *Milestone N1* e às instruções de engenharia estabelecidas em `MaiaRTOS_Native_Execution_Architecture.md`.

---

## 2. Inventário de Estados Globais e Reentrância

| Componente | Estado Global / Estático | Localização no Código | Risco de Reentrância | Estratégia de Mitigação |
| :--- | :--- | :--- | :--- | :--- |
| **I/O Redirection** | `s_stage` (`RedirStaging`) | `maia_shell.hpp` | Alto (sessão única) | Migrar para ponteiro em contexto de sessão de shell (`msh_context_t`) |
| **Console Callback** | `s_con` (`OutputFn`) | `maia_shell.hpp` | Médio | Injetado via `msh_host_v1` por instância de terminal |
| **Redirection Callback** | `s_cb` (`OutputFn`) | `maia_shell.hpp` | Médio | Confinado à chamada de execução ativa |
| **Pipeline Staging** | `s_pipe_in` (`PipeStaging`) | `maia_shell.hpp` | Alto (sessão única) | Buffer efêmero alocado em PSRAM liberado pós-pipeline |
| **Native Exec Pool** | `s_module_exec_pool[32KB]` | `elf_loader.c` | Alto (módulo único) | Semaforizado com `s_module_pool_in_use`; fallback dinâmico para DIRAM |
| **Variáveis de Shell** | `vars[MSH_MAX_VARS]` | `maia_shell.hpp` | Médio | Pertence ao contexto do processo/sessão do shell |
| **Diretório CWD** | `cwd[MSH_PATH_LEN]` | `maia_shell.hpp` | Médio | Gerenciado por sessão ou delegado ao `msh_host_v1.cwd_get/set` |

---

## 3. Mapeamento de Pontos de Contato com I/O

O MaiaShell atualmente interage com o ambiente por meio das seguintes chamadas diretas de I/O e Sistema de Arquivos:

1. **Console / Terminal:**
   - Callback de saída `output_cb` (`void (*)(const char *)`), desacoplado e roteado para o console serial e para o framebuffer ST7121 (1280x720) do Tab5.
   - Entrada de teclado: polling via barramento I2C Ext.Port1 (`0x6D`) no harness Tab5, traduzido para bytes alimentados em `shell.feed_char(c)`.

2. **Sistema de Arquivos e Armazenamento:**
   - `fopen`, `fclose`, `fread`, `fwrite`, `fseek`, `ftell` (usados em `cat`, `write`, `touch`, `cmd_runelf` e no flush de redirecionamento `>`).
   - `opendir`, `readdir`, `closedir`, `stat`, `mkdir`, `unlink` (usados em `ls`, `cd`, `rm`, `mkdir`).
   - Todos operam sobre o ponto de montagem VFS `/sd` suportado pelo driver de 4 bits SDMMC do ESP32-P4.

3. **Carregamento e Relocação Nativa:**
   - `mnm_load_and_run`: função pura C sem dependência de SO, aloca memória executável em `sram_low`, aplica relocação ELF32, invoca a barreira de sincronização de cache `esp_cache_msync` / `fence.i`, e despacha a execução para `maia_module_main(&ctx)`.

---

## 4. Camada de Abstração: `MaiaShellHost v1` (`host_v1.h`)

Para que o mesmo parser de shell e o mesmo avaliador semântico possam executar identicamente no **MaiaRTOS nativo**, no **MaiaOS (WASM)** e em simulações de desenvolvimento, a interface [`include/maia_shell/host_v1.h`](file:///Volumes/External_SSD/Documentos/Projects/maia-rtos/include/maia_shell/host_v1.h) formaliza o despacho:

```c
typedef struct msh_host_v1 {
    uint32_t struct_size;
    uint32_t version;
    void *user;
    int32_t (*read)(void *user, msh_fd_t fd, void *dst, uint32_t n);
    int32_t (*write)(void *user, msh_fd_t fd, const void *src, uint32_t n);
    int32_t (*open)(void *user, const char *path, uint32_t flags, msh_fd_t *out);
    int32_t (*close)(void *user, msh_fd_t fd);
    int32_t (*pipe_create)(void *user, msh_fd_t *read_end, msh_fd_t *write_end);
    int32_t (*fd_dup)(void *user, msh_fd_t src, msh_fd_t *dst);
    int32_t (*spawn)(void *user, const char *path, const char *const *argv,
                     msh_fd_t in, msh_fd_t out, msh_fd_t err, msh_job_t *job);
    int32_t (*wait)(void *user, msh_job_t job, int32_t *status);
    int32_t (*cwd_get)(void *user, char *dst, uint32_t cap);
    int32_t (*cwd_set)(void *user, const char *path);
    int32_t (*glob)(void *user, const char *pattern, void *sink);
} msh_host_v1;
```

---

## 5. Fatos Observados vs. Suposições Não Verificadas

- **Fato Observado:** O interpretador de comandos em `maia_shell.hpp` opera de maneira totalmente autônoma e freestanding, sem qualquer dependência de `Arduino.h` ou tarefas FreeRTOS.
- **Fato Observado:** A execução nativa de módulos `.mobj` (ELF32 ET_REL) não requer subsistema POSIX completo nem emulação de Linux; ela interage exclusivamente com a tabela de despacho [`maia_host_api_v1`](file:///Volumes/External_SSD/Documentos/Projects/maia-rtos/include/maia/exec/elf_loader.h).
- **Fato Observado:** O hardware ESP32-P4 exige sincronização explícita de linhas sujas de cache de dados L1 antes que o cache de instruções possa executar o código relocável.
- **Suposição a Validar:** A concorrência multi-core simultânea de múltiplos módulos nativos exigirá sincronização via IPI (Inter-Processor Interrupt) e isolamento dos registradores de PMP por core.
