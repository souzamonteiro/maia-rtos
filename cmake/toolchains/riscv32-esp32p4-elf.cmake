# Standalone RISC-V bare-metal toolchain for ESP32-P4 (No ESP-IDF runtime)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

if(NOT DEFINED MAIA_RISCV_TOOLCHAIN_BIN)
  set(MAIA_RISCV_TOOLCHAIN_BIN "/Users/roberto/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin")
endif()

set(CMAKE_C_COMPILER   "${MAIA_RISCV_TOOLCHAIN_BIN}/riscv32-esp-elf-gcc")
set(CMAKE_CXX_COMPILER "${MAIA_RISCV_TOOLCHAIN_BIN}/riscv32-esp-elf-g++")
set(CMAKE_ASM_COMPILER "${MAIA_RISCV_TOOLCHAIN_BIN}/riscv32-esp-elf-gcc")
set(CMAKE_OBJCOPY      "${MAIA_RISCV_TOOLCHAIN_BIN}/riscv32-esp-elf-objcopy")
set(CMAKE_OBJDUMP      "${MAIA_RISCV_TOOLCHAIN_BIN}/riscv32-esp-elf-objdump")
set(CMAKE_NM           "${MAIA_RISCV_TOOLCHAIN_BIN}/riscv32-esp-elf-nm")
set(CMAKE_READELF      "${MAIA_RISCV_TOOLCHAIN_BIN}/riscv32-esp-elf-readelf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(MAIA_ARCH_FLAGS "-march=rv32imac_zicsr_zifencei -mabi=ilp32 -ffreestanding -fno-builtin -fno-common -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS_INIT "${MAIA_ARCH_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${MAIA_ARCH_FLAGS} -fno-exceptions -fno-rtti")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -Wl,--gc-sections -Wl,--no-relax")

