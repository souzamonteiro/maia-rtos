# Auditoria de Memória e Execução Nativa RISC-V no ESP32-P4 (M5Stack Tab5)

## 1. Visão Geral da Arquitetura de Memória

O microcontrolador **ESP32-P4NRW32** (revisão v1.3) possui uma topologia de memória multicamada com separação explícita entre regiões de dados/instruções internas e externas:

| Região | Endereço Base | Tamanho | Atributos Hardware | Finalidade Primária |
| :--- | :--- | :--- | :--- | :--- |
| **`sram_low` (DIRAM)** | `0x4FF00000` | ~179 KB - 236 KB | **`xrw`** (Exec / Read / Write) | Código IRAM, Vetores CLIC, Pilhas Críticas, **Módulos Executáveis ELF (`.mobj`)** |
| **`sram_high`** | `0x4FF40000` | 256 KB | **`rw`** (Read / Write apenas) | Dados DRAM, buffers de periféricos, pools de comunicação (SEM execução de CPU) |
| **`tcm_idram_seg`** | `0x30100000` | 8 KB | `xr` / `rw` | Tightly-Coupled Memory para rotinas ultrarrápidas de núcleo único |
| **`extern_ram_seg` (PSRAM)** | `0x48000000` | 32 MB Octal @ 200 MHz | **`xrw`** (Cache L1/L2) | Framebuffers de vídeo (ST7121 1280x720), buffers de staging de I/O, Heap estendido |

---

## 2. Diagnóstico de Falhas de Execução e Correções

### 2.1. Conflito entre Heap Allocator e o Barramento de Instruções (`sram_high`)
* **Causa Raiz Identificada:** No arquivo `components/heap/port/esp32p4/memory_layout.c` do ESP-IDF, quando `CONFIG_ESP_SYSTEM_PMP_IDRAM_SPLIT=n`, tanto a região `sram_low` quanto `sram_high` recebem a flag `MALLOC_CAP_EXEC`. O heap allocator genérico podia alocar o buffer de um módulo nativo em endereços acima de `0x4FF40000` (ex: `0x4FF5A580`).
* **Sintoma:** Ao desviar o fluxo de execução para a função `maia_module_main` via `jalr`, o hardware de barramento do núcleo RISC-V abortava a leitura de instruções com exceção `Illegal instruction` (`MCAUSE = 0x00000002`).
* **Solução de Engenharia MaiaRTOS:**
  1. Criação de um slot estático dedicado de execução de 32 KB no segmento `.iram.data` (`0x4FF12B80`), garantindo posicionamento permanente em `sram_low` (`xrw`).
  2. Implementação de alocador de fallback dinâmico que rejeita blocos `>= 0x4FF40000`, preservando a integridade do sistema operacional caso múltiplos módulos sejam carregados.

### 2.2. Coerência de Cache L1 Data Write-Back no ESP32-P4
* **Causa Raiz Identificada:** O ESP32-P4 utiliza arquitetura de cache de dados com política de *Write-Back* (`SOC_CACHE_INTERNAL_MEM_VIA_L1CACHE`). Durante a carga e relocação do ELF:
  - O loader copiava as seções via `memcpy` e limpava o espaço com `memset(0)`.
  - As alterações ficavam sujas (*dirty*) nas linhas de cache de dados L1 e não eram imediatamente propagadas à memória física SRAM.
  - A instrução padrão RISC-V `fence.i` invalida o pipeline e o cache de instruções, mas **não** efetua o *writeback* do cache de dados em barramentos não snooped.
  - O núcleo de instrução realizava o fetch físico e lia zeros (`MTVAL = 0x00000000`), disparando pânico de instrução inválida.
* **Solução de Engenharia MaiaRTOS:**
  - Aplicação de sincronização explícita com alinhamento de 64 bytes via `esp_cache_msync`:
    ```c
    uintptr_t sync_start = (uintptr_t)module_base & ~63;
    size_t sync_size = ((total_alloc + 63) & ~63) + ((uintptr_t)module_base & 63);
    esp_cache_msync((void *)sync_start, sync_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
    esp_cache_msync((void *)sync_start, sync_size, ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_TYPE_INST);
    asm volatile("fence rw, rw" ::: "memory");
    asm volatile("fence.i" ::: "memory");
    ```

### 2.3. Desacoplamento de Redirecionamento I/O via Staging Buffer em PSRAM
* **Problema com Handles FatFS Ativos:** Manter arquivos SDMMC abertos (`fopen(..., "w")`) concorrentemente com operações de relocação e execução em SRAM L2 induzia contenção de DMA e potenciais corrupções de cache line.
* **Arquitetura Adotada (`RedirStaging`):**
  - O shell aloca um buffer de staging de 64 KB na PSRAM externa (`MALLOC_CAP_SPIRAM`).
  - Durante a execução de qualquer comando interno ou módulo ELF nativo (`calc.mobj`, `hello.mobj`), a saída das syscalls (`host_api.write_fd`) é acumulada no buffer.
  - O arquivo de destino na mídia SD (`FATFS`) é aberto e gravado em uma única transação atômica (`fwrite`) após o término do módulo, garantindo zero interferência de barramento.

---

## 3. Matriz de Validação em Hardware Real

Todos os cenários a seguir foram executados e validados no hardware **M5Stack Tab5** conectado via porta `/dev/cu.usbmodem1301`:

```bash
# 1. Empacotamento e inspeção de integridade ELF32 relocável
python3 tools/mnm-pack/mnm_pack.py examples/modules/calc/calc.mobj --entry maia_module_main --name calc --version 1.0.0
# [SUCCESS] Manifest generated: examples/modules/calc/calc.mnm.json

# 2. Execução nativa direta com cálculo de multiplicação e redirecionamento de saída (Sobrescrita >)
calc.mobj 125 * 8 > /sd/res.txt
cat /sd/res.txt
# Saída: Native Calc [RISC-V @ L2MEM]: 125 * 8 = 1000

# 3. Execução nativa direta com cálculo de subtração e concatenação (Append >>)
calc.mobj 999 - 111 >> /sd/res.txt
cat /sd/res.txt
# Saída:
# Native Calc [RISC-V @ L2MEM]: 125 * 8 = 1000
# Native Calc [RISC-V @ L2MEM]: 999 - 111 = 888

# 4. Execução nativa via runelf de módulo de texto para arquivo dedicado
runelf /sd/hello.mobj > /sd/hello_out.txt
cat /sd/hello_out.txt
# Saída: Hello from Native ELF module running on ESP32-P4 RISC-V!
```
