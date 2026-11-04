# First target: ESP32-P4 / M5Stack Tab5

**Hardware facts must be verified against primary documents before writing register-level code.** The project assumes a dual-core RISC-V ESP32-P4 without a conventional MMU and a Tab5 with external PSRAM; exact chip revision, clocks, peripheral mappings, PSRAM size, display and keyboard interfaces require confirmation for the actual board.

## Primary references to consult
- Espressif ESP32-P4 Technical Reference Manual: https://www.espressif.com/en/support/documents/technical-documents
- ESP32-P4 datasheet: https://www.espressif.com/en/products/socs/esp32-p4
- Espressif programming guide: https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/
- M5Stack official documentation: https://docs.m5stack.com/

**These are research references, not an authorization to link ESP-IDF runtime components.** Verify exact document links and revisions before implementation.

## Bring-up inventory
Chip revision; board revision; boot strap and UART pins; debug interface; memory map; flash layout; PSRAM interface; clock tree; timer/interrupt/IPI; CPU1 release; display bridge and touch IC; keyboard accessory interface; SD; USB; power rails; reset/watchdog.

See EVIDENCE.md and OPEN_QUESTIONS.md.
