# HAL, BSP and driver framework

## Responsibilities
Architecture port: ISA/traps/context. SoC HAL: clocks, interrupt controller, UART/GPIO/I2C/SPI/DMA/cache. BSP: Tab5-specific pins, power rails, screen/touch, keyboard wiring and board revision. Driver framework: registration, lifecycle, capabilities, asynchronous completion, ISR binding.

## Rules
No driver may rely on FreeRTOS or Arduino. Third-party vendor documentation is acceptable as a reference; do not copy runtime code. All MMIO addresses and pin mappings require primary-source evidence. Document cache maintenance and DMA buffer ownership.

## Bring-up order
UART -> GPIO -> timer -> memory/PSRAM -> I2C/SPI -> SD -> display/touch -> keyboard -> USB/network/audio (optional, later). The actual keyboard interface must be verified for the specific accessory.
