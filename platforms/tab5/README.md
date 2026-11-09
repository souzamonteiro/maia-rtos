# M5Stack Tab5 Platform Support for MaiaRTOS

Esta pasta contém o suporte ao dispositivo **M5Stack Tab5** (`ESP32-P4NRW32` dual-core RISC-V @ 360 MHz, 32 MB PSRAM @ 200 MHz, ST7121 MIPI-DSI 1280x720, ST7123 Touch, Teclado STM32 I2C 0x6D, Áudio ES8388, Câmera MIPI CSI e VFS SD/USB).

## Atribuição e Licenciamento

- **Código Original do Fabricante**: [M5Stack Tab5 User Demo](https://github.com/m5stack/M5Tab5-UserDemo)
- **Licença de Terceiros**: MIT License da M5Stack Technology Co., Ltd. (consulte [LICENSE-M5STACK.txt](file:///Volumes/External_SSD/Documentos/Projects/maia-rtos/platforms/tab5/LICENSE-M5STACK.txt)).
- **Componentes Importados**:
  - `components/`: Drivers de baixo nível de periféricos Tab5 (MIPI DSI ST7121, touch ST7123, câmera SCCB/IPA/Video, áudio e sensores).
  - `dependencies/`: Frameworks gráficos de suporte (LVGL v8.4.0, mooncake, smooth_ui_toolkit).
  - `vendor_app/`: Aplicações e painéis de referência do fabricante M5Stack (utilizados como demonstradores e base de HAL).
  - `audio/`: Efeitos sonoros padrão embutidos no firmware.
- **Camada MaiaRTOS**:
  - `main/app_main.cpp`: Supervisor de sistema operacional, inicialização de hardware, terminal gráfico 1280x720 em modo paisagem (Rotation 3), motor de execução MaiaScript nativo, subsistema ELF loader e bridge de periféricos.

## Como Compilar

Certifique-se de carregar o ambiente ESP-IDF v5.4+:

```bash
cd platforms/tab5
. $HOME/esp-idf/export.sh
idf.py build
```

## Como Gravar

```bash
idf.py -p /dev/cu.usbmodem1301 flash
```
ou utilizando esptool diretamente:
```bash
python -m esptool --chip esp32p4 -p /dev/cu.usbmodem1301 -b 460800 write_flash 0x10000 build/m5stack_tab5.bin
```
