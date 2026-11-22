cmake_minimum_required(VERSION 3.20)
project(MaiaRTOS VERSION 0.1.0 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(maia_sd_read STATIC drivers/storage/sd_read.c)
target_include_directories(maia_sd_read PUBLIC include)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(maia_sd_read PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(maia_fatfs STATIC
  third_party/fatfs/src/ff.c
  third_party/fatfs/src/ffunicode.c
  drivers/storage/fatfs_diskio.c
  services/fatfs_mount.c
)
target_include_directories(maia_fatfs
  PUBLIC include
  PRIVATE third_party/fatfs/src drivers/storage
)
target_link_libraries(maia_fatfs PUBLIC maia_sd_read)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  set_source_files_properties(
    drivers/storage/fatfs_diskio.c
    services/fatfs_mount.c
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror"
  )
endif()

add_library(maia_vfs STATIC services/vfs.c)
target_include_directories(maia_vfs
    PUBLIC include
    PRIVATE third_party/fatfs/src
)
target_link_libraries(maia_vfs PUBLIC maia_fatfs)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(maia_vfs PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
  set(MAIA_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boards/m5stack/tab5/linker.ld")
  set(MAIA_F0_FLASH_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boards/m5stack/tab5/flash_boot_probe.ld")
  set(MAIA_F0_PRODUCT_LINKER_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/boards/m5stack/tab5/flash_product.ld")
  option(MAIA_USE_FLASH_PRODUCT_LINKER
    "Use the Tab5 flash-XIP product linker and native PSRAM-backed LCD" ON)
  option(MAIA_DIAGNOSTIC_SDMMC_CMD8_TIMEOUT
    "DIAGNOSTIC ONLY: simulate an SDMMC CMD8 timeout during product boot" OFF)
  option(MAIA_DIAGNOSTIC_FATFS_RW
    "DIAGNOSTIC ONLY: exercise MaiaVFS /sd file operations through FatFs" OFF)
  option(MAIA_DIAGNOSTIC_VFS_SHELL
    "POC ONLY: start native LCD shell in /sd without desktop or automatic SD writes" OFF)
  option(MAIA_DIAGNOSTIC_VFS_STRESS
    "DIAGNOSTIC ONLY: run bounded concurrent MaiaVFS stress and one-shot I/O recovery checks" OFF)
  if(MAIA_DIAGNOSTIC_VFS_SHELL AND
     (NOT MAIA_USE_FLASH_PRODUCT_LINKER OR MAIA_DIAGNOSTIC_FATFS_RW))
    message(FATAL_ERROR "VFS shell POC requires flash-XIP and excludes automatic FATFS_RW")
  endif()
  if(MAIA_DIAGNOSTIC_VFS_STRESS AND
     (NOT MAIA_USE_FLASH_PRODUCT_LINKER OR MAIA_DIAGNOSTIC_FATFS_RW OR
      MAIA_DIAGNOSTIC_VFS_SHELL OR MAIA_DIAGNOSTIC_SDMMC_CMD8_TIMEOUT))
    message(FATAL_ERROR "VFS stress requires flash-XIP and excludes other SD diagnostics")
  endif()
  if(MAIA_DIAGNOSTIC_FATFS_RW AND NOT MAIA_USE_FLASH_PRODUCT_LINKER)
    message(FATAL_ERROR "MAIA_DIAGNOSTIC_FATFS_RW requires the flash-XIP product")
  endif()
  if(MAIA_USE_FLASH_PRODUCT_LINKER)
    set(MAIA_LINKER_SCRIPT "${MAIA_F0_PRODUCT_LINKER_SCRIPT}")
  endif()
  add_executable(maia_m0.elf
    arch/riscv/start.S
    arch/riscv/trap_context.S
    soc/espressif/esp32p4/soc_early_init.c
    soc/espressif/esp32p4/soc_console.c
    soc/espressif/esp32p4/soc_serial_rx.c
    soc/espressif/esp32p4/soc_serial_tx.c
    soc/espressif/esp32p4/soc_timer.c
    soc/espressif/esp32p4/soc_cache.c
    arch/riscv/stack.c
    soc/espressif/esp32p4/soc_panic.c
    soc/espressif/esp32p4/soc_board_io.c
    soc/espressif/esp32p4/soc_display_native.c
    soc/espressif/esp32p4/soc_mipi_park.c
    soc/espressif/esp32p4/soc_dsi_access.c
    soc/espressif/esp32p4/soc_boot_i2c.c
    soc/espressif/esp32p4/soc_ldo3_acquire.c
    soc/espressif/esp32p4/soc_ldo3_plan.c
    soc/espressif/esp32p4/soc_ldo3_snapshot.c
    drivers/i2c/bitbang.c
    drivers/gpio/pi4ioe5v6408.c
    drivers/storage/sd_read.c
    boards/m5stack/tab5/sdmmc_boot.c
    boards/m5stack/tab5/panel_reset.c
    boards/m5stack/tab5/touch_identify.c
    boards/m5stack/tab5/st7121.c
    kernel/kmem.c
    kernel/ksync.c
    kernel/ksched.c
    runtime/newlib_locks.c
    runtime/newlib_heap.c
    runtime/newlib_reent.c
    runtime/newlib_syscalls.c
    runtime/gthread.c
    runtime/pthread.c
    runtime/tls.c
    shell/maia_shell_engine.cpp
    components/exec/elf_validator.c
    components/exec/elf_loader.c
    assembler/src/riscv_isa.cpp
    assembler/src/riscv_asm.cpp
    assembler/src/riscv_disasm.cpp
    maiascript/src/parser.cpp
    maiascript/src/codegen_riscv.cpp
    lib/maiaregex/maia_regex.c
    lib/maiaglob/maia_glob.c
    lib/maiaglob/maia_glob_compile.cpp
    boards/m5stack/tab5/app_desc.c
    boards/m5stack/tab5/boot_main.c
  )
  if(MAIA_USE_FLASH_PRODUCT_LINKER)
    target_sources(maia_m0.elf PRIVATE
      runtime/newlib_vfs.c
      soc/espressif/esp32p4/soc_display_product.c
      boards/m5stack/tab5/product_touch.c
      services/lcd_output.c
      services/i2c_bus.c
      services/touch_input.c
      drivers/input/st712x.c
      soc/espressif/esp32p4/soc_dw_dma_irq.c
      soc/espressif/esp32p4/soc_psram_boot.c
      soc/espressif/esp32p4/soc_psram_clock.c
      soc/espressif/esp32p4/soc_psram_supply.c
      soc/espressif/esp32p4/soc_psram_mpll.c
      soc/espressif/esp32p4/soc_psram_array.c
      soc/espressif/esp32p4/soc_psram_mode.c
      soc/espressif/esp32p4/soc_psram_snapshot.c
      soc/espressif/esp32p4/soc_psram_window.c
    )
    target_compile_definitions(maia_m0.elf PRIVATE
      MAIA_DSI_LCD_STREAM=1
      MAIA_DSI_FRAMEBUFFER_PROBE=1
      MAIA_PRODUCT_TOUCH=1
    )
    set_property(SOURCE
      soc/espressif/esp32p4/soc_psram_boot.c
      soc/espressif/esp32p4/soc_psram_clock.c
      soc/espressif/esp32p4/soc_psram_supply.c
      soc/espressif/esp32p4/soc_psram_mpll.c
      soc/espressif/esp32p4/soc_psram_array.c
      soc/espressif/esp32p4/soc_psram_mode.c
      soc/espressif/esp32p4/soc_psram_snapshot.c
      soc/espressif/esp32p4/soc_psram_window.c
      APPEND PROPERTY COMPILE_OPTIONS -Os
    )
  endif()
  if(NOT MAIA_USE_FLASH_PRODUCT_LINKER)
    target_sources(maia_m0.elf PRIVATE soc/espressif/esp32p4/soc_display.c)
  endif()
  target_include_directories(maia_m0.elf PRIVATE
    include
    assembler/include
    maiascript/include
    lib/maiaregex
    lib/maiaglob
  )
  if(MAIA_USE_FLASH_PRODUCT_LINKER)
    target_link_libraries(maia_m0.elf PRIVATE maia_vfs)
    target_compile_definitions(maia_m0.elf PRIVATE MAIA_PRODUCT_FATFS=1)
  endif()
  target_compile_options(maia_m0.elf PRIVATE -Wall -Wextra -O2)
  target_compile_definitions(maia_m0.elf PRIVATE
    MAIA_NATIVE_PSRAM_STARTUP_PENDING=1
    MAIA_ENABLE_CXX_INIT_ARRAY=1
    MAIA_ENABLE_COMPILER_TLS=1
    MAIA_SDMMC_RUNTIME=1
  )
  if(MAIA_DIAGNOSTIC_SDMMC_CMD8_TIMEOUT)
    target_compile_definitions(maia_m0.elf PRIVATE MAIA_SDMMC_DIAGNOSTIC_CMD8_TIMEOUT=1)
  endif()
  if(MAIA_DIAGNOSTIC_FATFS_RW)
    target_compile_definitions(maia_m0.elf PRIVATE MAIA_DIAGNOSTIC_FATFS_RW=1)
  endif()
  if(MAIA_DIAGNOSTIC_VFS_STRESS)
    target_compile_definitions(maia_m0.elf PRIVATE
      MAIA_DIAGNOSTIC_VFS_STRESS=1
      MAIA_FATFS_DIAGNOSTIC_FAULT_INJECTION=1
    )
    target_compile_definitions(maia_fatfs PRIVATE
      MAIA_FATFS_DIAGNOSTIC_FAULT_INJECTION=1
    )
    target_compile_definitions(maia_vfs PRIVATE
      MAIA_DIAGNOSTIC_VFS_STRESS=1
    )
  endif()
  if(MAIA_DIAGNOSTIC_VFS_SHELL)
    target_compile_definitions(maia_m0.elf PRIVATE MAIA_DIAGNOSTIC_VFS_SHELL=1)
  endif()
  target_compile_options(maia_m0.elf PRIVATE
    $<$<COMPILE_LANGUAGE:C,CXX>:-ftls-model=local-exec>
  )
  target_link_options(maia_m0.elf PRIVATE
    "-T${MAIA_LINKER_SCRIPT}"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/maia_m0.map"
  )
  target_link_libraries(maia_m0.elf PRIVATE stdc++ c m gcc)
  set_target_properties(maia_m0.elf PROPERTIES LINK_DEPENDS "${MAIA_LINKER_SCRIPT}")
  message(STATUS "Configured MaiaRTOS bare-metal target (maia_m0.elf) for ESP32-P4 / M5Stack Tab5")

  add_executable(maia_f0_flash_probe.elf
    arch/riscv/start.S
    soc/espressif/esp32p4/soc_early_init.c
    soc/espressif/esp32p4/soc_console.c
    soc/espressif/esp32p4/soc_serial_tx.c
    boards/m5stack/tab5/app_desc.c
    boards/m5stack/tab5/flash_boot_probe_main.c
  )
  target_include_directories(maia_f0_flash_probe.elf PRIVATE include)
  target_compile_definitions(maia_f0_flash_probe.elf PRIVATE MAIA_FLASH_PROBE_RECOVERABLE=1)
  target_compile_options(maia_f0_flash_probe.elf PRIVATE
    -Wall -Wextra -O2
    -ffunction-sections -fdata-sections
  )
  target_link_options(maia_f0_flash_probe.elf PRIVATE
    "-T${MAIA_F0_FLASH_LINKER_SCRIPT}"
    "-Wl,--gc-sections"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/maia_f0_flash_probe.map"
  )
  target_link_libraries(maia_f0_flash_probe.elf PRIVATE gcc)
  set_target_properties(maia_f0_flash_probe.elf PROPERTIES
    LINK_DEPENDS "${MAIA_F0_FLASH_LINKER_SCRIPT}"
  )

  add_executable(maia_f1_runtime_probe.elf
    arch/riscv/start.S
    arch/riscv/trap_context.S
    soc/espressif/esp32p4/soc_early_init.c
    soc/espressif/esp32p4/soc_console.c
    soc/espressif/esp32p4/soc_serial_rx.c
    soc/espressif/esp32p4/soc_serial_tx.c
    soc/espressif/esp32p4/soc_timer.c
    soc/espressif/esp32p4/soc_panic.c
    arch/riscv/stack.c
    kernel/kmem.c
    kernel/ksync.c
    kernel/ksched.c
    runtime/newlib_locks.c
    runtime/newlib_heap.c
    runtime/newlib_reent.c
    runtime/newlib_syscalls.c
    runtime/gthread.c
    runtime/pthread.c
    runtime/tls.c
    boards/m5stack/tab5/app_desc.c
    tests/hardware/native_f1_runtime_boot.c
    tests/hardware/native_f1_runtime_smoke.cpp
  )
  target_include_directories(maia_f1_runtime_probe.elf PRIVATE include)
  target_compile_definitions(maia_f1_runtime_probe.elf PRIVATE
    MAIA_FLASH_PROBE_RECOVERABLE=1
    MAIA_ENABLE_CXX_INIT_ARRAY=1
    MAIA_ENABLE_COMPILER_TLS=1
  )
  target_compile_options(maia_f1_runtime_probe.elf PRIVATE
    -Wall -Wextra -O2
    $<$<COMPILE_LANGUAGE:C,CXX>:-ftls-model=local-exec>
  )
  target_link_options(maia_f1_runtime_probe.elf PRIVATE
    "-T${MAIA_F0_FLASH_LINKER_SCRIPT}"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/maia_f1_runtime_probe.map"
  )
  target_link_libraries(maia_f1_runtime_probe.elf PRIVATE stdc++ c m gcc)
  set_target_properties(maia_f1_runtime_probe.elf PROPERTIES
    LINK_DEPENDS "${MAIA_F0_FLASH_LINKER_SCRIPT}"
  )

  # Independent scheduler diagnostic; does not link the shell/vendor platform.
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(MAIA_ESPTOOL_EXECUTABLE "" CACHE FILEPATH "Standalone esptool.py executable for native image packaging")
  if(MAIA_ESPTOOL_EXECUTABLE)
    if(NOT EXISTS "${MAIA_ESPTOOL_EXECUTABLE}")
      message(FATAL_ERROR "MAIA_ESPTOOL_EXECUTABLE does not exist: ${MAIA_ESPTOOL_EXECUTABLE}")
    endif()
    if(MAIA_USE_FLASH_PRODUCT_LINKER)
      set(MAIA_PRODUCT_FLASH_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/maia_m0.bin")
      add_custom_command(
        OUTPUT "${MAIA_PRODUCT_FLASH_IMAGE}"
        COMMAND "${MAIA_ESPTOOL_EXECUTABLE}" --chip esp32p4 elf2image
          --flash_mode dio
          --flash_freq 80m
          --flash_size 16MB
          --flash-mmu-page-size 64KB
          --output "${MAIA_PRODUCT_FLASH_IMAGE}"
          "$<TARGET_FILE:maia_m0.elf>"
        COMMAND "${MAIA_ESPTOOL_EXECUTABLE}" --chip esp32p4 image_info
          "${MAIA_PRODUCT_FLASH_IMAGE}"
        COMMAND "${Python3_EXECUTABLE}"
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/f0_flash_image_test.py"
          "${MAIA_PRODUCT_FLASH_IMAGE}" --sram-window 0x4FF00000:0x4FF28000
        DEPENDS maia_m0.elf
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/f0_flash_image_test.py"
        VERBATIM
      )
      add_custom_target(maia_m0_image DEPENDS "${MAIA_PRODUCT_FLASH_IMAGE}")
    endif()

    set(MAIA_F0_FLASH_PROBE_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/maia_f0_flash_probe.bin")
    add_custom_command(
      OUTPUT "${MAIA_F0_FLASH_PROBE_IMAGE}"
      COMMAND "${MAIA_ESPTOOL_EXECUTABLE}" --chip esp32p4 elf2image
        --flash_mode dio
        --flash_freq 80m
        --flash_size 16MB
        --flash-mmu-page-size 64KB
        --output "${MAIA_F0_FLASH_PROBE_IMAGE}"
        "$<TARGET_FILE:maia_f0_flash_probe.elf>"
      COMMAND "${MAIA_ESPTOOL_EXECUTABLE}" --chip esp32p4 image_info
        "${MAIA_F0_FLASH_PROBE_IMAGE}"
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/f0_flash_image_test.py"
        "${MAIA_F0_FLASH_PROBE_IMAGE}"
      DEPENDS maia_f0_flash_probe.elf
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/f0_flash_image_test.py"
      VERBATIM
    )
    add_custom_target(maia_f0_flash_probe_image DEPENDS "${MAIA_F0_FLASH_PROBE_IMAGE}")

    set(MAIA_F1_RUNTIME_PROBE_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/maia_f1_runtime_probe.bin")
    add_custom_command(
      OUTPUT "${MAIA_F1_RUNTIME_PROBE_IMAGE}"
      COMMAND "${MAIA_ESPTOOL_EXECUTABLE}" --chip esp32p4 elf2image
        --flash_mode dio
        --flash_freq 80m
        --flash_size 16MB
        --flash-mmu-page-size 64KB
        --output "${MAIA_F1_RUNTIME_PROBE_IMAGE}"
        "$<TARGET_FILE:maia_f1_runtime_probe.elf>"
      COMMAND "${MAIA_ESPTOOL_EXECUTABLE}" --chip esp32p4 image_info
        "${MAIA_F1_RUNTIME_PROBE_IMAGE}"
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/f0_flash_image_test.py"
        "${MAIA_F1_RUNTIME_PROBE_IMAGE}" --sram-window 0x4FF00000:0x4FF28000
      DEPENDS maia_f1_runtime_probe.elf
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/f0_flash_image_test.py"
      VERBATIM
    )
    add_custom_target(maia_f1_runtime_probe_image DEPENDS "${MAIA_F1_RUNTIME_PROBE_IMAGE}")

    add_executable(maia_f0_kernel_psram_probe.elf
      arch/riscv/start.S
      arch/riscv/trap_context.S
      soc/espressif/esp32p4/soc_early_init.c
      soc/espressif/esp32p4/soc_console.c
      soc/espressif/esp32p4/soc_serial_tx.c
      soc/espressif/esp32p4/soc_timer.c
      soc/espressif/esp32p4/soc_panic.c
      soc/espressif/esp32p4/soc_psram_snapshot.c
      soc/espressif/esp32p4/soc_psram_boot.c
      soc/espressif/esp32p4/soc_psram_clock.c
      soc/espressif/esp32p4/soc_psram_supply.c
      soc/espressif/esp32p4/soc_psram_mpll.c
      soc/espressif/esp32p4/soc_psram_array.c
      soc/espressif/esp32p4/soc_psram_mode.c
      soc/espressif/esp32p4/soc_psram_window.c
      arch/riscv/stack.c
      kernel/kmem.c
      kernel/ksched.c
      boards/m5stack/tab5/app_desc.c
      tests/hardware/native_scheduler_probe.c
      tests/hardware/native_register_probe.S
      tests/hardware/native_fp_abi.c
    )
    target_include_directories(maia_f0_kernel_psram_probe.elf PRIVATE
      include
      soc/espressif/esp32p4
    )
    target_compile_definitions(maia_f0_kernel_psram_probe.elf PRIVATE
      MAIA_F0_FLASH_KERNEL_PSRAM_STATE=1
      MAIA_F0_FLASH_HEAP_TEST=1
      MAIA_FLASH_PROBE_RECOVERABLE=1
    )
    target_compile_options(maia_f0_kernel_psram_probe.elf PRIVATE
      -Wall -Wextra -O2
      -ffunction-sections -fdata-sections
    )
    target_link_options(maia_f0_kernel_psram_probe.elf PRIVATE
      "-T${MAIA_F0_PRODUCT_LINKER_SCRIPT}"
      "-Wl,--gc-sections"
      "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/maia_f0_kernel_psram_probe.map"
    )
    target_link_libraries(maia_f0_kernel_psram_probe.elf PRIVATE gcc)
    set_target_properties(maia_f0_kernel_psram_probe.elf PROPERTIES
      LINK_DEPENDS "${MAIA_F0_PRODUCT_LINKER_SCRIPT}"
    )

    set(MAIA_F0_KERNEL_PSRAM_IMAGE "${CMAKE_CURRENT_BINARY_DIR}/maia_f0_kernel_psram_probe.bin")
    add_custom_command(
      OUTPUT "${MAIA_F0_KERNEL_PSRAM_IMAGE}"
      COMMAND "${MAIA_ESPTOOL_EXECUTABLE}" --chip esp32p4 elf2image
        --flash_mode dio
        --flash_freq 80m
        --flash_size 16MB
        --flash-mmu-page-size 64KB
        --output "${MAIA_F0_KERNEL_PSRAM_IMAGE}"
        "$<TARGET_FILE:maia_f0_kernel_psram_probe.elf>"
      COMMAND "${MAIA_ESPTOOL_EXECUTABLE}" --chip esp32p4 image_info
        "${MAIA_F0_KERNEL_PSRAM_IMAGE}"
      COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/f0_flash_image_test.py"
        "${MAIA_F0_KERNEL_PSRAM_IMAGE}" --sram-window 0x4FF00000:0x4FF28000
      DEPENDS maia_f0_kernel_psram_probe.elf
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/f0_flash_image_test.py"
      VERBATIM
    )
    add_custom_target(maia_f0_kernel_psram_probe_image DEPENDS "${MAIA_F0_KERNEL_PSRAM_IMAGE}")
  endif()

  set(MAIA_NATIVE_PROBE_FAULT "" CACHE STRING "RAM fault diagnostic: illegal16, illegal32 or breakpoint16")
  set(MAIA_NATIVE_PROBE_PSRAM_FAST "" CACHE STRING "Qualify native PSRAM at100 or200MHz")
  if(MAIA_NATIVE_PROBE_PSRAM_FAST AND (NOT MAIA_NATIVE_PROBE_PSRAM_DMA OR NOT MAIA_NATIVE_PROBE_PSRAM_FAST MATCHES "^(100|200)$"))
    message(FATAL_ERROR "PSRAM fast requires DMA and rate100 or200")
  endif()
  option(MAIA_NATIVE_PROBE_PSRAM_DMA "Qualify PSRAM source DMA bursts into owned SRAM" OFF)
  if(MAIA_NATIVE_PROBE_PSRAM_DMA AND NOT MAIA_NATIVE_PROBE_PSRAM_WINDOW)
    message(FATAL_ERROR "PSRAM DMA requires window")
  endif()
  option(MAIA_NATIVE_PROBE_PSRAM_WINDOW "Qualify native CPU2MiB PSRAM window and mapping restore" OFF)
  if(MAIA_NATIVE_PROBE_PSRAM_WINDOW AND NOT MAIA_NATIVE_PROBE_PSRAM_ARRAY)
    message(FATAL_ERROR "PSRAM window requires array profile")
  endif()
  option(MAIA_NATIVE_PROBE_PSRAM_ARRAY "Port HEX training array read/write and content restore" OFF)
  if(MAIA_NATIVE_PROBE_PSRAM_ARRAY AND NOT MAIA_NATIVE_PROBE_PSRAM_CLOCK)
    message(FATAL_ERROR "PSRAM array requires clock profile")
  endif()
  option(MAIA_NATIVE_PROBE_PSRAM_CLOCK "Qualify native XTAL20 PSRAM clock and restore" OFF)
  if(MAIA_NATIVE_PROBE_PSRAM_CLOCK AND NOT MAIA_NATIVE_PROBE_PSRAM_ID)
    message(FATAL_ERROR "PSRAM clock requires identity profile")
  endif()
  option(MAIA_NATIVE_PROBE_PSRAM_ID "Bounded inherited HEX mode-register identity and restore" OFF)
  if(MAIA_NATIVE_PROBE_PSRAM_ID AND NOT MAIA_NATIVE_PROBE_PSRAM_STATE)
    message(FATAL_ERROR "PSRAM identity requires state profile")
  endif()
  option(MAIA_NATIVE_PROBE_PSRAM_STATE "Read-only inherited PSRAM state without array access" OFF)
  option(MAIA_NATIVE_PROBE_VDMA_LIST "Qualify finite two-block SRAM descriptor fetch/copy" OFF)
  if(MAIA_NATIVE_PROBE_VDMA_LIST AND (NOT MAIA_NATIVE_PROBE_VDMA_TRANSFER OR MAIA_NATIVE_PROBE_MIPI_PARK OR MAIA_NATIVE_PROBE_VDMA_COUNT))
    message(FATAL_ERROR "VDMA list requires separate SRAM transfer profile without count or MIPI park")
  endif()
  option(MAIA_NATIVE_PROBE_VDMA_COUNT "Qualify raw completion counter using known SRAM copies" OFF)
  if(MAIA_NATIVE_PROBE_VDMA_COUNT AND (NOT MAIA_NATIVE_PROBE_VDMA_TRANSFER OR MAIA_NATIVE_PROBE_MIPI_PARK))
    message(FATAL_ERROR "VDMA count requires separate SRAM transfer profile")
  endif()
  option(MAIA_NATIVE_PROBE_PSRAM_BOOT "Untested native off-supply boot entry before full PSRAM/LCD pipeline" OFF)
  if(MAIA_NATIVE_PROBE_PSRAM_BOOT AND (NOT MAIA_NATIVE_PROBE_PSRAM_PROFILE OR NOT MAIA_NATIVE_PROBE_MPLL_INITIALIZE OR MAIA_NATIVE_PROBE_PSRAM_PARK))
    message(FATAL_ERROR "Native boot requires profile/MPLL pipeline and excludes warm park")
  endif()
  option(MAIA_NATIVE_PROBE_PSRAM_PARK "Native PSRAM/MPLL consumer park/resume before recalibration" OFF)
  if(MAIA_NATIVE_PROBE_PSRAM_PARK AND NOT MAIA_NATIVE_PROBE_PSRAM_SUPPLY)
    message(FATAL_ERROR "PSRAM park requires supply/profile qualification")
  endif()
  option(MAIA_NATIVE_PROBE_PSRAM_SUPPLY "Native calibrated VO2 supply adoption before PSRAM profile" OFF)
  if(MAIA_NATIVE_PROBE_PSRAM_SUPPLY AND NOT MAIA_NATIVE_PROBE_PSRAM_PROFILE)
    message(FATAL_ERROR "PSRAM supply requires PSRAM profile/framebuffer qualification")
  endif()
  option(MAIA_NATIVE_PROBE_PSRAM_PROFILE "Native PSRAM mode/latency initialization and recovery" OFF)
  if(MAIA_NATIVE_PROBE_PSRAM_PROFILE AND NOT MAIA_NATIVE_PROBE_MPLL_INITIALIZE)
    message(FATAL_ERROR "PSRAM profile requires MPLL initialize/framebuffer qualification")
  endif()
  option(MAIA_NATIVE_PROBE_MPLL_INITIALIZE "Native powered MPLL400 programming/calibration before framebuffer" OFF)
  if(MAIA_NATIVE_PROBE_MPLL_INITIALIZE AND NOT MAIA_NATIVE_PROBE_INPUT_LIFECYCLE)
    message(FATAL_ERROR "MPLL initialize requires isolated input lifecycle/framebuffer profile")
  endif()
  option(MAIA_NATIVE_PROBE_INPUT_LIFECYCLE "Native touch stop/restart and bus close/reopen with LCD active" OFF)
  if(MAIA_NATIVE_PROBE_INPUT_LIFECYCLE AND NOT MAIA_NATIVE_PROBE_TOUCH_LCD)
    message(FATAL_ERROR "Input lifecycle requires touch LCD")
  endif()
  option(MAIA_NATIVE_PROBE_TOUCH_LCD "Native scheduled touch contacts and LCD feedback" OFF)
  if(MAIA_NATIVE_PROBE_TOUCH_LCD AND (NOT MAIA_NATIVE_PROBE_LCD_STREAM OR MAIA_NATIVE_PROBE_LCD_LIFECYCLE))
    message(FATAL_ERROR "Touch LCD requires stream and excludes lifecycle harness")
  endif()
  option(MAIA_NATIVE_PROBE_LCD_LIFECYCLE "Native LCD image publication and warm restart cycles" OFF)
  if(MAIA_NATIVE_PROBE_LCD_LIFECYCLE AND NOT MAIA_NATIVE_PROBE_LCD_STREAM)
    message(FATAL_ERROR "LCD lifecycle requires stream profile")
  endif()
  option(MAIA_NATIVE_PROBE_LCD_STREAM "Native LCD task with actual DW-GDMA completion IRQ" OFF)
  if(MAIA_NATIVE_PROBE_LCD_STREAM AND NOT MAIA_NATIVE_PROBE_DPI_FRAMEBUFFER)
    message(FATAL_ERROR "LCD stream requires framebuffer profile")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_FRAMEBUFFER "Native full PSRAM framebuffer to DSI" OFF)
  if(MAIA_NATIVE_PROBE_DPI_FRAMEBUFFER AND (NOT MAIA_NATIVE_PROBE_DPI_BRIDGE OR MAIA_NATIVE_PROBE_DPI_DMA_TILE OR MAIA_NATIVE_PROBE_DPI_DMA_SPATIAL OR MAIA_NATIVE_PROBE_DPI_DMA_CENTER OR MAIA_NATIVE_PROBE_DPI_DMA_STAGE))
    message(FATAL_ERROR "PSRAM framebuffer requires bridge and excludes SRAM diagnostics")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_DMA_CENTER "Central32-row SRAM pixel diagnostic" OFF)
  if(MAIA_NATIVE_PROBE_DPI_DMA_CENTER AND (NOT MAIA_NATIVE_PROBE_DPI_DMA_SPATIAL OR NOT MAIA_NATIVE_PROBE_DPI_BRIDGE OR MAIA_NATIVE_PROBE_DPI_DMA_TILE OR MAIA_NATIVE_PROBE_DPI_DMA_STAGE))
    message(FATAL_ERROR "DPI center requires spatial/bridge without legacy tile/solid stage")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_DMA_SPATIAL "Finite five-block spatial pixel diagnostic" OFF)
  if(MAIA_NATIVE_PROBE_DPI_DMA_SPATIAL AND ((NOT MAIA_NATIVE_PROBE_DPI_DMA_TILE AND NOT MAIA_NATIVE_PROBE_DPI_DMA_CENTER) OR MAIA_NATIVE_PROBE_DPI_DMA_STAGE))
    message(FATAL_ERROR "DPI spatial requires CPU tile/bridge without solid DMA stage/frame")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_DMA_REFRESH "Show a bounded400-frame solid-color sequence" OFF)
  if(MAIA_NATIVE_PROBE_DPI_DMA_REFRESH AND NOT MAIA_NATIVE_PROBE_DPI_DMA_FRAME)
    message(FATAL_ERROR "DPI DMA refresh requires native frame profile")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_DMA_FRAME "Transfer one finite solid-source frame with guarded recovery" OFF)
  if(MAIA_NATIVE_PROBE_DPI_DMA_FRAME AND NOT MAIA_NATIVE_PROBE_DPI_DMA_STAGE)
    message(FATAL_ERROR "DPI DMA frame requires native DMA stage profile")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_DMA_STAGE "Stage inactive solid-source channel0/DSI flow controls" OFF)
  if(MAIA_NATIVE_PROBE_DPI_DMA_STAGE AND NOT MAIA_NATIVE_PROBE_DPI_DMA_TILE)
    message(FATAL_ERROR "DPI DMA stage requires owned SRAM tile profile")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_DMA_TILE "Construct owned SRAM tile and finite DSI DMA descriptors without enabling DMA" OFF)
  if(MAIA_NATIVE_PROBE_DPI_DMA_TILE AND (NOT MAIA_NATIVE_PROBE_DPI_BRIDGE OR MAIA_NATIVE_PROBE_VDMA_TRANSFER))
    message(FATAL_ERROR "DPI DMA tile requires native bridge profile without VDMA transfer")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_BRIDGE "Prepare native bridge/update with pixel output and DMA inactive" OFF)
  if(MAIA_NATIVE_PROBE_DPI_BRIDGE AND (NOT MAIA_NATIVE_PROBE_DPI_CLOCK OR MAIA_NATIVE_PROBE_DPI_PATTERN))
    message(FATAL_ERROR "DPI bridge requires native DPI clock profile without VPG")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_PATTERN "Cycle native host color bars without DMA" OFF)
  if(MAIA_NATIVE_PROBE_DPI_PATTERN AND NOT MAIA_NATIVE_PROBE_DPI_CLOCK)
    message(FATAL_ERROR "DPI pattern requires native DPI clock profile")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_CLOCK "Check panel RGB565 then enable guarded inactive DPI clock" OFF)
  if(MAIA_NATIVE_PROBE_DPI_CLOCK AND NOT MAIA_NATIVE_PROBE_DPI_STAGE)
    message(FATAL_ERROR "DPI clock requires native DPI stage profile")
  endif()
  option(MAIA_NATIVE_PROBE_DPI_STAGE "Stage inactive native RGB565 DPI format/clock/timing" OFF)
  if(MAIA_NATIVE_PROBE_DPI_STAGE AND NOT MAIA_NATIVE_PROBE_PANEL_POWER)
    message(FATAL_ERROR "DPI stage requires native panel power profile")
  endif()
  option(MAIA_NATIVE_PROBE_PANEL_POWER "Read panel power mode before/after native ST7121 program" OFF)
  if(MAIA_NATIVE_PROBE_PANEL_POWER AND NOT MAIA_NATIVE_PROBE_ST7121)
    message(FATAL_ERROR "Panel power requires native ST7121 profile")
  endif()
  option(MAIA_NATIVE_PROBE_ST7121 "Send selected ST7121 command program after native reset/DSI validation" OFF)
  if(MAIA_NATIVE_PROBE_ST7121 AND NOT MAIA_NATIVE_PROBE_DSI_RESET)
    message(FATAL_ERROR "ST7121 program requires native DSI reset profile")
  endif()
  option(MAIA_NATIVE_PROBE_DSI_RESET "Reset identified ST712x under gated MIPI before native DSI reads" OFF)
  if(MAIA_NATIVE_PROBE_DSI_RESET AND NOT MAIA_NATIVE_PROBE_DSI_READ)
    message(FATAL_ERROR "DSI reset requires native DSI read profile")
  endif()
  option(MAIA_NATIVE_PROBE_DSI_READ "Read VC0 RDDID using bounded BTA and restore host controls" OFF)
  if(MAIA_NATIVE_PROBE_DSI_READ AND NOT MAIA_NATIVE_PROBE_DSI_LONG)
    message(FATAL_ERROR "DSI read requires native DSI long profile")
  endif()
  option(MAIA_NATIVE_PROBE_DSI_LONG "Exercise bounded DCS long framing with one-byte NOP content" OFF)
  if(MAIA_NATIVE_PROBE_DSI_LONG AND NOT MAIA_NATIVE_PROBE_DSI_SHORT)
    message(FATAL_ERROR "DSI long requires native DSI short profile")
  endif()
  option(MAIA_NATIVE_PROBE_DSI_SHORT "Send bounded LP DCS NOP short packets" OFF)
  if(MAIA_NATIVE_PROBE_DSI_SHORT AND NOT MAIA_NATIVE_PROBE_DSI_COMMAND)
    message(FATAL_ERROR "DSI short requires native DSI command profile")
  endif()
  option(MAIA_NATIVE_PROBE_DSI_COMMAND "Configure native DSI command host without transmitting" OFF)
  if(MAIA_NATIVE_PROBE_DSI_COMMAND AND NOT MAIA_NATIVE_PROBE_DSI_PHY)
    message(FATAL_ERROR "DSI command requires native DSI PHY profile")
  endif()
  option(MAIA_NATIVE_PROBE_DSI_PHY "Initialize fixed two-lane960Mbps native DSI PHY" OFF)
  if(MAIA_NATIVE_PROBE_DSI_PHY AND NOT MAIA_NATIVE_PROBE_DSI_REFERENCE)
    message(FATAL_ERROR "DSI PHY requires native DSI reference profile")
  endif()
  option(MAIA_NATIVE_PROBE_DSI_REFERENCE "Prepare native nominal20MHz DSI PHY reference" OFF)
  if(MAIA_NATIVE_PROBE_DSI_REFERENCE AND NOT MAIA_NATIVE_PROBE_DSI_ACCESS)
    message(FATAL_ERROR "DSI reference requires native DSI access profile")
  endif()
  option(MAIA_NATIVE_PROBE_DSI_ACCESS "Open reset-state DSI configuration access after VO3 cycle" OFF)
  if(MAIA_NATIVE_PROBE_DSI_ACCESS AND NOT MAIA_NATIVE_PROBE_VO3_CYCLE)
    message(FATAL_ERROR "DSI access requires native VO3 cycle profile")
  endif()
  option(MAIA_NATIVE_PROBE_VO3_CYCLE "Cycle VO3 only after verified native MIPI clock gate" OFF)
  if(MAIA_NATIVE_PROBE_VO3_CYCLE AND NOT MAIA_NATIVE_PROBE_MIPI_GATE)
    message(FATAL_ERROR "VO3 cycle requires native MIPI gate profile")
  endif()
  option(MAIA_NATIVE_PROBE_MIPI_GATE "Gate reset-state MIPI clocks after native parking" OFF)
  if(MAIA_NATIVE_PROBE_MIPI_GATE AND NOT MAIA_NATIVE_PROBE_MIPI_PARK)
    message(FATAL_ERROR "MIPI gate requires native park profile")
  endif()
  option(MAIA_NATIVE_PROBE_MIPI_PARK "Verify reset-state native MIPI parking" OFF)
  if(MAIA_NATIVE_PROBE_MIPI_PARK AND (NOT MAIA_NATIVE_PROBE_MIPI OR MAIA_NATIVE_PROBE_VDMA_STOP))
    message(FATAL_ERROR "MIPI park requires MIPI and excludes VDMA profiles")
  endif()
  option(MAIA_NATIVE_PROBE_VDMA_TRANSFER "Exercise SRAM transfer and active VDMA stop" OFF)
  if(MAIA_NATIVE_PROBE_VDMA_TRANSFER AND (NOT MAIA_NATIVE_PROBE_VDMA_STOP OR MAIA_NATIVE_PROBE_VDMA_ARM_EMPTY))
    message(FATAL_ERROR "VDMA transfer requires stop and excludes empty-arm profile")
  endif()
  option(MAIA_NATIVE_PROBE_VDMA_STOP "Verify boot VDMA whole-controller stop" OFF)
  option(MAIA_NATIVE_PROBE_VDMA_ARM_EMPTY "Arm empty VDMA before stop diagnostic" OFF)
  if(MAIA_NATIVE_PROBE_VDMA_STOP AND NOT MAIA_NATIVE_PROBE_MIPI)
    message(FATAL_ERROR "VDMA stop requires MAIA_NATIVE_PROBE_MIPI")
  endif()
  if(MAIA_NATIVE_PROBE_VDMA_ARM_EMPTY AND NOT MAIA_NATIVE_PROBE_VDMA_STOP)
    message(FATAL_ERROR "Empty VDMA arm requires MAIA_NATIVE_PROBE_VDMA_STOP")
  endif()
  option(MAIA_NATIVE_PROBE_MIPI "Read-only inherited MIPI/DW-GDMA state" OFF)
  option(MAIA_NATIVE_PROBE_LDO3_ACQUIRE "Exercise boot-only VO3 acquisition" OFF)
  option(MAIA_NATIVE_PROBE_LDO3_CYCLE "Controlled VO3 disable/enable diagnostic with DSI gated" OFF)
  if(MAIA_NATIVE_PROBE_LDO3_ACQUIRE AND NOT MAIA_NATIVE_PROBE_LDO3)
    message(FATAL_ERROR "LDO3 acquisition requires MAIA_NATIVE_PROBE_LDO3")
  endif()
  if(MAIA_NATIVE_PROBE_LDO3_CYCLE AND NOT MAIA_NATIVE_PROBE_LDO3_ACQUIRE)
    message(FATAL_ERROR "LDO3 cycle requires MAIA_NATIVE_PROBE_LDO3_ACQUIRE")
  endif()
  option(MAIA_NATIVE_PROBE_LDO3 "Read-only VO3 calibration diagnostic" OFF)
  option(MAIA_NATIVE_PROBE_SDMMC_STATE "Read-only SDMMC/VO4 state diagnostic" OFF)
  option(MAIA_NATIVE_PROBE_SDMMC_CMD8 "Boot-only Tab5 SDMMC CMD0/CMD8 diagnostic" OFF)
  option(MAIA_NATIVE_PROBE_SDMMC_BLOCK0 "Boot-only Tab5 SDHC init and single-block read diagnostic" OFF)
  option(MAIA_NATIVE_PROBE_SDMMC_WRITE "Boot-only Tab5 single-block write/compare diagnostic" OFF)
  option(MAIA_NATIVE_PROBE_SDMMC_RUNTIME "Tab5 SDMMC runtime-service read/write diagnostic" OFF)
  option(MAIA_NATIVE_PROBE_SDMMC_REPEAT "Read two SDHC LBAs consecutively after one initialization" OFF)
  set(MAIA_NATIVE_SDMMC_READ_LBA "0" CACHE STRING "SDHC LBA for the isolated Tab5 read diagnostic")
  set(MAIA_NATIVE_SDMMC_WRITE_LBA "0" CACHE STRING "SDHC LBA for the isolated Tab5 write diagnostic")
  set(MAIA_NATIVE_SDMMC_RUNTIME_LBA "0" CACHE STRING "First SDHC LBA for the runtime-service diagnostic")
  set(MAIA_NATIVE_SDMMC_REPEAT_LBA "0" CACHE STRING "Second SDHC LBA for the consecutive-read diagnostic")
  if(MAIA_NATIVE_PROBE_SDMMC_BLOCK0 AND
     (NOT MAIA_NATIVE_SDMMC_READ_LBA MATCHES "^[0-9]+$" OR
      MAIA_NATIVE_SDMMC_READ_LBA GREATER 4294967295))
    message(FATAL_ERROR "MAIA_NATIVE_SDMMC_READ_LBA must be in [0, 2^32-1]")
  endif()
  if(MAIA_NATIVE_PROBE_SDMMC_WRITE AND
     (NOT MAIA_NATIVE_SDMMC_WRITE_LBA MATCHES "^[0-9]+$" OR
      MAIA_NATIVE_SDMMC_WRITE_LBA GREATER 4294967295))
    message(FATAL_ERROR "MAIA_NATIVE_SDMMC_WRITE_LBA must be in [0, 2^32-1]")
  endif()
  if(MAIA_NATIVE_PROBE_SDMMC_RUNTIME AND
     (NOT MAIA_NATIVE_SDMMC_RUNTIME_LBA MATCHES "^[0-9]+$" OR
      MAIA_NATIVE_SDMMC_RUNTIME_LBA GREATER 4294967294))
    message(FATAL_ERROR "MAIA_NATIVE_SDMMC_RUNTIME_LBA must leave room for two blocks")
  endif()
  if(MAIA_NATIVE_PROBE_SDMMC_REPEAT AND NOT MAIA_NATIVE_PROBE_SDMMC_BLOCK0)
    message(FATAL_ERROR "Consecutive SDMMC reads require MAIA_NATIVE_PROBE_SDMMC_BLOCK0")
  endif()
  if(MAIA_NATIVE_PROBE_SDMMC_REPEAT AND
     (NOT MAIA_NATIVE_SDMMC_REPEAT_LBA MATCHES "^[0-9]+$" OR
      MAIA_NATIVE_SDMMC_REPEAT_LBA GREATER 4294967295))
    message(FATAL_ERROR "MAIA_NATIVE_SDMMC_REPEAT_LBA must be in [0, 2^32-1]")
  endif()
  option(MAIA_NATIVE_PROBE_PANEL_RESET "Exercise checked LCD/touch reset in boot diagnostic" OFF)
  if(MAIA_NATIVE_PROBE_PANEL_RESET AND NOT MAIA_NATIVE_PROBE_EXPANDER)
    message(FATAL_ERROR "Panel reset requires MAIA_NATIVE_PROBE_EXPANDER")
  endif()
  option(MAIA_NATIVE_PROBE_EXPANDER "Extend touch probe with expander same-value writes" OFF)
  if(MAIA_NATIVE_PROBE_EXPANDER AND NOT MAIA_NATIVE_PROBE_TOUCH)
    message(FATAL_ERROR "Expander diagnostic requires MAIA_NATIVE_PROBE_TOUCH")
  endif()
  option(MAIA_NATIVE_PROBE_TOUCH "Read-only boot touch identification (no panel init)" OFF)
  option(MAIA_NATIVE_PROBE_SERIAL_TX_EVENT "Exercise TX worker notifications and quiet waits" OFF)
  option(MAIA_NATIVE_PROBE_NOTIFY "Exercise native notification waits and IRQ wakeups" OFF)
  option(MAIA_NATIVE_PROBE_SERIAL_TX "Exercise queued TX from multiple tasks" OFF)
  option(MAIA_NATIVE_PROBE_SERIAL "Exercise managed serial RX with real host input" OFF)
  option(MAIA_NATIVE_PROBE_QUEUE "Exercise native queue task/IRQ concurrency" OFF)
  set(MAIA_NATIVE_PROBE_STACK "" CACHE STRING "RAM stack fault: underflow, guard, incoming, irqguard, misaligned or zero")
  if(MAIA_NATIVE_PROBE_STACK AND NOT MAIA_NATIVE_PROBE_STACK MATCHES "^(underflow|guard|incoming|irqguard|misaligned|zero)$")
    message(FATAL_ERROR "Unknown stack fault profile")
  endif()
  option(MAIA_NATIVE_PROBE_IDLE "Exercise empty/all-sleeping/all-exited private idle" OFF)
  option(MAIA_NATIVE_PROBE_EMPTY "Start idle diagnostic with no application tasks" OFF)
  if(MAIA_NATIVE_PROBE_EMPTY AND NOT MAIA_NATIVE_PROBE_IDLE)
    message(FATAL_ERROR "Empty probe requires idle profile")
  endif()
  if(MAIA_NATIVE_PROBE_SDMMC_STATE OR MAIA_NATIVE_PROBE_SDMMC_CMD8 OR
     MAIA_NATIVE_PROBE_SDMMC_BLOCK0 OR MAIA_NATIVE_PROBE_SDMMC_WRITE OR
     MAIA_NATIVE_PROBE_SDMMC_RUNTIME)
    if(MAIA_NATIVE_PROBE_SDMMC_STATE)
      set(MAIA_SDMMC_SELECTED_PROBE MAIA_NATIVE_PROBE_SDMMC_STATE)
    elseif(MAIA_NATIVE_PROBE_SDMMC_CMD8)
      set(MAIA_SDMMC_SELECTED_PROBE MAIA_NATIVE_PROBE_SDMMC_CMD8)
    elseif(MAIA_NATIVE_PROBE_SDMMC_WRITE)
      set(MAIA_SDMMC_SELECTED_PROBE MAIA_NATIVE_PROBE_SDMMC_WRITE)
    elseif(MAIA_NATIVE_PROBE_SDMMC_RUNTIME)
      set(MAIA_SDMMC_SELECTED_PROBE MAIA_NATIVE_PROBE_SDMMC_RUNTIME)
    else()
      set(MAIA_SDMMC_SELECTED_PROBE MAIA_NATIVE_PROBE_SDMMC_BLOCK0)
    endif()
    get_cmake_property(MAIA_PROBE_CONFIGURATION_VARIABLES VARIABLES)
    foreach(MAIA_PROBE_VARIABLE IN LISTS MAIA_PROBE_CONFIGURATION_VARIABLES)
      if(MAIA_PROBE_VARIABLE MATCHES "^MAIA_NATIVE_PROBE_" AND
         NOT MAIA_PROBE_VARIABLE STREQUAL MAIA_SDMMC_SELECTED_PROBE AND
         NOT MAIA_PROBE_VARIABLE STREQUAL "MAIA_NATIVE_PROBE_SDMMC_REPEAT")
        if(${MAIA_PROBE_VARIABLE})
          message(FATAL_ERROR "SDMMC requires an isolated diagnostic; conflicts with ${MAIA_PROBE_VARIABLE}")
        endif()
      endif()
    endforeach()
    if(MAIA_NATIVE_PROBE_SDMMC_STATE)
      set(MAIA_PROBE_APP tests/hardware/native_sdmmc_state_probe.c)
    elseif(MAIA_NATIVE_PROBE_SDMMC_CMD8)
      set(MAIA_PROBE_APP tests/hardware/native_sdmmc_cmd8_probe.c boards/m5stack/tab5/sdmmc_boot.c)
    elseif(MAIA_NATIVE_PROBE_SDMMC_WRITE)
      set(MAIA_PROBE_APP tests/hardware/native_sdmmc_write_probe.c boards/m5stack/tab5/sdmmc_boot.c drivers/storage/sd_read.c)
    elseif(MAIA_NATIVE_PROBE_SDMMC_RUNTIME)
      set(MAIA_PROBE_APP tests/hardware/native_sdmmc_runtime_probe.c
        boards/m5stack/tab5/sdmmc_boot.c drivers/storage/sd_read.c
        soc/espressif/esp32p4/soc_cache.c)
    else()
      set(MAIA_PROBE_APP tests/hardware/native_sdmmc_block0_probe.c boards/m5stack/tab5/sdmmc_boot.c drivers/storage/sd_read.c)
    endif()
  elseif(MAIA_NATIVE_PROBE_PSRAM_STATE)
    get_cmake_property(MAIA_PROBE_CONFIGURATION_VARIABLES VARIABLES)
    foreach(MAIA_PROBE_VARIABLE IN LISTS MAIA_PROBE_CONFIGURATION_VARIABLES)
      if(MAIA_PROBE_VARIABLE MATCHES "^MAIA_NATIVE_PROBE_" AND NOT MAIA_PROBE_VARIABLE STREQUAL "MAIA_NATIVE_PROBE_PSRAM_STATE" AND NOT MAIA_PROBE_VARIABLE STREQUAL "MAIA_NATIVE_PROBE_PSRAM_ID" AND NOT MAIA_PROBE_VARIABLE STREQUAL "MAIA_NATIVE_PROBE_PSRAM_CLOCK" AND NOT MAIA_PROBE_VARIABLE STREQUAL "MAIA_NATIVE_PROBE_PSRAM_ARRAY" AND NOT MAIA_PROBE_VARIABLE STREQUAL "MAIA_NATIVE_PROBE_PSRAM_WINDOW" AND NOT MAIA_PROBE_VARIABLE STREQUAL "MAIA_NATIVE_PROBE_PSRAM_DMA" AND NOT MAIA_PROBE_VARIABLE STREQUAL "MAIA_NATIVE_PROBE_PSRAM_FAST")
        if(${MAIA_PROBE_VARIABLE})
          message(FATAL_ERROR "PSRAM state requires isolated read-only profile; conflicts with ${MAIA_PROBE_VARIABLE}")
        endif()
      endif()
    endforeach()
    set(MAIA_PROBE_APP tests/hardware/native_psram_state_probe.c soc/espressif/esp32p4/soc_psram_snapshot.c)
    if(MAIA_NATIVE_PROBE_PSRAM_ID)
      list(APPEND MAIA_PROBE_APP soc/espressif/esp32p4/soc_psram_mode.c)
      if(MAIA_NATIVE_PROBE_PSRAM_CLOCK)
        list(APPEND MAIA_PROBE_APP soc/espressif/esp32p4/soc_psram_clock.c soc/espressif/esp32p4/soc_psram_mpll.c soc/espressif/esp32p4/soc_psram_supply.c)
        if(MAIA_NATIVE_PROBE_PSRAM_ARRAY)
          list(APPEND MAIA_PROBE_APP soc/espressif/esp32p4/soc_psram_array.c)
          if(MAIA_NATIVE_PROBE_PSRAM_WINDOW)
            list(APPEND MAIA_PROBE_APP soc/espressif/esp32p4/soc_psram_window.c)
          endif()
        endif()
      endif()
    endif()
  elseif(MAIA_NATIVE_PROBE_MIPI)
    if(MAIA_NATIVE_PROBE_LDO3 OR MAIA_NATIVE_PROBE_LDO3_ACQUIRE OR MAIA_NATIVE_PROBE_LDO3_CYCLE OR
       MAIA_NATIVE_PROBE_TOUCH OR MAIA_NATIVE_PROBE_EXPANDER OR MAIA_NATIVE_PROBE_PANEL_RESET OR
       MAIA_NATIVE_PROBE_SERIAL_TX_EVENT OR MAIA_NATIVE_PROBE_NOTIFY OR MAIA_NATIVE_PROBE_SERIAL_TX OR
       MAIA_NATIVE_PROBE_SERIAL OR MAIA_NATIVE_PROBE_QUEUE OR MAIA_NATIVE_PROBE_STACK OR
       MAIA_NATIVE_PROBE_IDLE OR MAIA_NATIVE_PROBE_EMPTY OR MAIA_NATIVE_PROBE_FAULT OR MAIA_NATIVE_PROBE_FP32 OR
       MAIA_NATIVE_PROBE_MIXED OR MAIA_NATIVE_PROBE_LIFECYCLE)
      message(FATAL_ERROR "MIPI state requires a separate read-only diagnostic")
    endif()
    if(MAIA_NATIVE_PROBE_MIPI_PARK)
      set(MAIA_PROBE_APP tests/hardware/native_mipi_park_probe.c soc/espressif/esp32p4/soc_mipi_park.c)
    elseif(MAIA_NATIVE_PROBE_VDMA_TRANSFER)
      if(MAIA_NATIVE_PROBE_VDMA_LIST)
        set(MAIA_PROBE_APP tests/hardware/native_vdma_list_probe.c)
      elseif(MAIA_NATIVE_PROBE_VDMA_COUNT)
        set(MAIA_PROBE_APP tests/hardware/native_vdma_count_probe.c)
      else()
        set(MAIA_PROBE_APP tests/hardware/native_vdma_transfer_probe.c)
      endif()
    else()
      set(MAIA_PROBE_APP tests/hardware/native_mipi_probe.c)
    endif()
  elseif(MAIA_NATIVE_PROBE_LDO3)
    if(MAIA_NATIVE_PROBE_TOUCH OR MAIA_NATIVE_PROBE_EXPANDER OR MAIA_NATIVE_PROBE_PANEL_RESET OR
       MAIA_NATIVE_PROBE_SERIAL_TX_EVENT OR MAIA_NATIVE_PROBE_NOTIFY OR MAIA_NATIVE_PROBE_SERIAL_TX OR
       MAIA_NATIVE_PROBE_SERIAL OR MAIA_NATIVE_PROBE_QUEUE OR MAIA_NATIVE_PROBE_STACK OR
       MAIA_NATIVE_PROBE_IDLE OR MAIA_NATIVE_PROBE_FAULT OR MAIA_NATIVE_PROBE_FP32 OR
       MAIA_NATIVE_PROBE_MIXED OR MAIA_NATIVE_PROBE_LIFECYCLE)
      message(FATAL_ERROR "LDO3 probe requires a separate read-only workload")
    endif()
    set(MAIA_PROBE_APP tests/hardware/native_ldo3_probe.c soc/espressif/esp32p4/soc_ldo3_plan.c
      soc/espressif/esp32p4/soc_ldo3_snapshot.c)
  elseif(MAIA_NATIVE_PROBE_TOUCH)
    if(MAIA_NATIVE_PROBE_SERIAL_TX_EVENT OR MAIA_NATIVE_PROBE_NOTIFY OR MAIA_NATIVE_PROBE_SERIAL_TX OR
       MAIA_NATIVE_PROBE_SERIAL OR MAIA_NATIVE_PROBE_QUEUE OR MAIA_NATIVE_PROBE_STACK OR
       MAIA_NATIVE_PROBE_IDLE OR MAIA_NATIVE_PROBE_FAULT OR MAIA_NATIVE_PROBE_FP32 OR
       MAIA_NATIVE_PROBE_MIXED OR MAIA_NATIVE_PROBE_LIFECYCLE)
      message(FATAL_ERROR "Touch probe requires a separate boot-only workload")
    endif()
    set(MAIA_PROBE_APP tests/hardware/native_touch_probe.c drivers/i2c/bitbang.c
      boards/m5stack/tab5/touch_identify.c soc/espressif/esp32p4/soc_boot_i2c.c)
  elseif(MAIA_NATIVE_PROBE_SERIAL_TX_EVENT)
    set(MAIA_PROBE_APP tests/hardware/native_serial_tx_event_probe.c services/serial_output.c kernel/ksync.c)
  elseif(MAIA_NATIVE_PROBE_NOTIFY)
    set(MAIA_PROBE_APP tests/hardware/native_notification_probe.c)
  elseif(MAIA_NATIVE_PROBE_SERIAL_TX)
    set(MAIA_PROBE_APP tests/hardware/native_serial_tx_probe.c services/serial_output.c kernel/ksync.c)
  elseif(MAIA_NATIVE_PROBE_SERIAL)
    set(MAIA_PROBE_APP tests/hardware/native_serial_probe.c services/serial_input.c kernel/ksync.c)
  elseif(MAIA_NATIVE_PROBE_QUEUE)
    set(MAIA_PROBE_APP tests/hardware/native_queue_probe.c kernel/ksync.c)
  elseif(MAIA_NATIVE_PROBE_STACK)
    set(MAIA_PROBE_APP tests/hardware/native_stack_probe.c tests/hardware/native_stack_instructions.S)
  elseif(MAIA_NATIVE_PROBE_IDLE)
    set(MAIA_PROBE_APP tests/hardware/native_idle_probe.c)
  elseif(MAIA_NATIVE_PROBE_FAULT)
    if(NOT MAIA_NATIVE_PROBE_FAULT MATCHES "^(illegal16|illegal32|breakpoint16)$")
      message(FATAL_ERROR "Unknown MAIA_NATIVE_PROBE_FAULT")
    endif()
    set(MAIA_PROBE_APP tests/hardware/native_fault_probe.c tests/hardware/native_fault_instructions.S)
  else()
    set(MAIA_PROBE_APP tests/hardware/native_scheduler_probe.c tests/hardware/native_register_probe.S)
  endif()
  add_executable(maia_sched_probe.elf EXCLUDE_FROM_ALL
    arch/riscv/start.S
    arch/riscv/trap_context.S
    soc/espressif/esp32p4/soc_early_init.c
    soc/espressif/esp32p4/soc_console.c
    soc/espressif/esp32p4/soc_serial_rx.c
    soc/espressif/esp32p4/soc_serial_tx.c
    soc/espressif/esp32p4/soc_timer.c
    arch/riscv/stack.c
    soc/espressif/esp32p4/soc_panic.c
    kernel/ksched.c
    boards/m5stack/tab5/app_desc.c
    ${MAIA_PROBE_APP}
  )
  if(MAIA_NATIVE_PROBE_SDMMC_BLOCK0)
    target_compile_definitions(maia_sched_probe.elf PRIVATE
      MAIA_NATIVE_SDMMC_READ_LBA=${MAIA_NATIVE_SDMMC_READ_LBA})
  endif()
  if(MAIA_NATIVE_PROBE_SDMMC_WRITE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE
      MAIA_NATIVE_SDMMC_WRITE_LBA=${MAIA_NATIVE_SDMMC_WRITE_LBA})
  endif()
  if(MAIA_NATIVE_PROBE_SDMMC_RUNTIME)
    target_compile_definitions(maia_sched_probe.elf PRIVATE
      MAIA_SDMMC_RUNTIME=1
      MAIA_NATIVE_SDMMC_RUNTIME_LBA=${MAIA_NATIVE_SDMMC_RUNTIME_LBA})
  endif()
  if(MAIA_NATIVE_PROBE_SDMMC_REPEAT)
    target_compile_definitions(maia_sched_probe.elf PRIVATE
      MAIA_NATIVE_SDMMC_REPEAT_LBA=${MAIA_NATIVE_SDMMC_REPEAT_LBA}
      MAIA_NATIVE_PROBE_SDMMC_REPEAT=1)
  endif()
  if(MAIA_NATIVE_PROBE_PSRAM_BOOT)
    target_sources(maia_sched_probe.elf PRIVATE
      soc/espressif/esp32p4/soc_psram_boot.c
      tests/hardware/native_memcpy.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PSRAM_BOOT=1)
    set_property(SOURCE tests/hardware/native_scheduler_probe.c tests/hardware/native_mipi_park_probe.c
      APPEND PROPERTY COMPILE_OPTIONS -Os -fno-tree-loop-distribute-patterns)
    set_property(SOURCE tests/hardware/native_memcpy.c APPEND PROPERTY COMPILE_OPTIONS
      -fno-tree-loop-distribute-patterns)
    set_property(SOURCE soc/espressif/esp32p4/soc_psram_boot.c soc/espressif/esp32p4/soc_psram_supply.c
      soc/espressif/esp32p4/soc_psram_mpll.c soc/espressif/esp32p4/soc_psram_array.c
      soc/espressif/esp32p4/soc_psram_clock.c APPEND PROPERTY COMPILE_OPTIONS -Os)
  endif()
  if(MAIA_NATIVE_PROBE_PSRAM_PARK)
    # Fit the boot-only clock code in the unchanged qualified low-SRAM window.
    set_property(SOURCE soc/espressif/esp32p4/soc_psram_clock.c soc/espressif/esp32p4/soc_psram_mpll.c APPEND PROPERTY COMPILE_OPTIONS -Os)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PSRAM_PARK=1)
  endif()
  if(MAIA_NATIVE_PROBE_PSRAM_SUPPLY)
    target_sources(maia_sched_probe.elf PRIVATE soc/espressif/esp32p4/soc_psram_supply.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PSRAM_SUPPLY=1)
  endif()
  if(MAIA_NATIVE_PROBE_PSRAM_PROFILE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PSRAM_PROFILE=1)
  endif()
  if(MAIA_NATIVE_PROBE_MPLL_INITIALIZE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_MPLL_INITIALIZE=1)
  endif()
  if(MAIA_NATIVE_PROBE_INPUT_LIFECYCLE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_INPUT_LIFECYCLE=1)
  endif()
  if(MAIA_NATIVE_PROBE_TOUCH_LCD)
    target_sources(maia_sched_probe.elf PRIVATE services/i2c_bus.c services/touch_input.c drivers/input/st712x.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_TOUCH_LCD=1)
  endif()
  if(MAIA_NATIVE_PROBE_LCD_LIFECYCLE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_LCD_LIFECYCLE=1)
  endif()
  if(MAIA_NATIVE_PROBE_LCD_STREAM)
    target_sources(maia_sched_probe.elf PRIVATE services/lcd_output.c soc/espressif/esp32p4/soc_dw_dma_irq.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_LCD_STREAM=1 MAIA_DSI_LCD_STREAM=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_FRAMEBUFFER)
    target_sources(maia_sched_probe.elf PRIVATE soc/espressif/esp32p4/soc_psram_snapshot.c
      soc/espressif/esp32p4/soc_psram_mode.c soc/espressif/esp32p4/soc_psram_array.c
      soc/espressif/esp32p4/soc_psram_clock.c soc/espressif/esp32p4/soc_psram_mpll.c soc/espressif/esp32p4/soc_psram_supply.c
      soc/espressif/esp32p4/soc_psram_window.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_FRAMEBUFFER=1 MAIA_DSI_FRAMEBUFFER_PROBE=1)
  endif()
  if(MAIA_NATIVE_PROBE_DSI_RESET)
    target_sources(maia_sched_probe.elf PRIVATE drivers/i2c/bitbang.c
      drivers/gpio/pi4ioe5v6408.c boards/m5stack/tab5/touch_identify.c
      boards/m5stack/tab5/panel_reset.c soc/espressif/esp32p4/soc_boot_i2c.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DSI_RESET=1)
  endif()
  if(MAIA_NATIVE_PROBE_ST7121)
    target_sources(maia_sched_probe.elf PRIVATE boards/m5stack/tab5/st7121.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_ST7121=1)
  endif()
  if(MAIA_NATIVE_PROBE_PANEL_POWER)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PANEL_POWER=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_STAGE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_STAGE=1)
  endif()
  if(MAIA_NATIVE_PROBE_PSRAM_FAST)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PSRAM_FAST=${MAIA_NATIVE_PROBE_PSRAM_FAST})
  endif()
  if(MAIA_NATIVE_PROBE_PSRAM_DMA)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PSRAM_DMA=1)
  endif()
  if(MAIA_NATIVE_PROBE_PSRAM_WINDOW)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PSRAM_WINDOW=1)
  endif()
  if(MAIA_NATIVE_PROBE_PSRAM_ARRAY)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PSRAM_ARRAY=1)
  endif()
  if(MAIA_NATIVE_PROBE_PSRAM_CLOCK)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PSRAM_CLOCK=1)
  endif()
  if(MAIA_NATIVE_PROBE_PSRAM_ID)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PSRAM_ID=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_DMA_CENTER)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_DMA_CENTER=1 MAIA_DSI_SPATIAL_CENTER=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_DMA_SPATIAL)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_DMA_SPATIAL=1 MAIA_DSI_SPATIAL_PROBE=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_DMA_REFRESH)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_DMA_REFRESH=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_DMA_FRAME)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_DMA_FRAME=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_DMA_STAGE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_DMA_STAGE=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_DMA_TILE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_DMA_TILE=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_BRIDGE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_BRIDGE=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_PATTERN)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_PATTERN=1)
  endif()
  if(MAIA_NATIVE_PROBE_DPI_CLOCK)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DPI_CLOCK=1)
  endif()
  if(MAIA_NATIVE_PROBE_DSI_READ)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DSI_READ=1)
  endif()
  if(MAIA_NATIVE_PROBE_DSI_LONG)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DSI_LONG=1)
  endif()
  if(MAIA_NATIVE_PROBE_DSI_SHORT)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DSI_SHORT=1)
  endif()
  if(MAIA_NATIVE_PROBE_DSI_COMMAND)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DSI_COMMAND=1)
  endif()
  if(MAIA_NATIVE_PROBE_DSI_PHY)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DSI_PHY=1)
  endif()
  if(MAIA_NATIVE_PROBE_DSI_REFERENCE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DSI_REFERENCE=1)
  endif()
  if(MAIA_NATIVE_PROBE_DSI_ACCESS)
    target_sources(maia_sched_probe.elf PRIVATE soc/espressif/esp32p4/soc_dsi_access.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_DSI_ACCESS=1)
  endif()
  if(MAIA_NATIVE_PROBE_VO3_CYCLE)
    target_sources(maia_sched_probe.elf PRIVATE soc/espressif/esp32p4/soc_ldo3_plan.c
      soc/espressif/esp32p4/soc_ldo3_snapshot.c soc/espressif/esp32p4/soc_ldo3_acquire.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_VO3_CYCLE=1)
  endif()
  if(MAIA_NATIVE_PROBE_MIPI_GATE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_MIPI_GATE=1)
  endif()
  if(MAIA_NATIVE_PROBE_VDMA_STOP)
    target_sources(maia_sched_probe.elf PRIVATE soc/espressif/esp32p4/soc_vdma_stop.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_VDMA_STOP=1)
  endif()
  if(MAIA_NATIVE_PROBE_VDMA_ARM_EMPTY)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_VDMA_ARM_EMPTY=1)
  endif()
  if(MAIA_NATIVE_PROBE_EXPANDER)
    target_sources(maia_sched_probe.elf PRIVATE drivers/gpio/pi4ioe5v6408.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_EXPANDER=1)
  endif()
  if(MAIA_NATIVE_PROBE_LDO3_ACQUIRE)
    target_sources(maia_sched_probe.elf PRIVATE soc/espressif/esp32p4/soc_ldo3_acquire.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_LDO3_ACQUIRE=1)
  endif()
  if(MAIA_NATIVE_PROBE_LDO3_CYCLE)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_LDO3_CYCLE=1)
  endif()
  if(MAIA_NATIVE_PROBE_PANEL_RESET)
    target_sources(maia_sched_probe.elf PRIVATE boards/m5stack/tab5/panel_reset.c)
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_PANEL_RESET=1)
  endif()
  option(MAIA_NATIVE_PROBE_FP32 "Exercise native RV32F/ILP32F context (RAM diagnostic only)" OFF)
  option(MAIA_NATIVE_PROBE_MIXED "Exercise mixed FS states, yield, delay and priorities" OFF)
  option(MAIA_NATIVE_PROBE_LIFECYCLE "Stress atomic create/exit and snapshots in mixed profile" OFF)
  if(MAIA_NATIVE_PROBE_LIFECYCLE)
    if(NOT MAIA_NATIVE_PROBE_MIXED)
      message(FATAL_ERROR "Lifecycle probe requires mixed scheduling profile")
    endif()
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_LIFECYCLE=1)
  endif()
  if(MAIA_NATIVE_PROBE_MIXED)
    if(NOT MAIA_NATIVE_PROBE_FP32)
      message(FATAL_ERROR "MAIA_NATIVE_PROBE_MIXED requires MAIA_NATIVE_PROBE_FP32")
    endif()
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_MIXED=1)
  endif()
  if(MAIA_NATIVE_PROBE_SERIAL_TX_EVENT AND (MAIA_NATIVE_PROBE_NOTIFY OR MAIA_NATIVE_PROBE_SERIAL_TX OR
      MAIA_NATIVE_PROBE_SERIAL OR MAIA_NATIVE_PROBE_QUEUE OR MAIA_NATIVE_PROBE_FAULT OR
      MAIA_NATIVE_PROBE_STACK OR MAIA_NATIVE_PROBE_IDLE OR MAIA_NATIVE_PROBE_MIXED OR
      MAIA_NATIVE_PROBE_LIFECYCLE OR MAIA_NATIVE_PROBE_FP32))
    message(FATAL_ERROR "Event TX probe uses a separate integer workload")
  endif()
  if(MAIA_NATIVE_PROBE_NOTIFY AND (MAIA_NATIVE_PROBE_SERIAL_TX OR MAIA_NATIVE_PROBE_SERIAL OR
      MAIA_NATIVE_PROBE_QUEUE OR MAIA_NATIVE_PROBE_FAULT OR MAIA_NATIVE_PROBE_STACK OR
      MAIA_NATIVE_PROBE_IDLE OR MAIA_NATIVE_PROBE_MIXED OR MAIA_NATIVE_PROBE_LIFECYCLE OR MAIA_NATIVE_PROBE_FP32))
    message(FATAL_ERROR "Notification probe uses a separate integer workload")
  endif()
  if(MAIA_NATIVE_PROBE_SERIAL_TX AND (MAIA_NATIVE_PROBE_SERIAL OR MAIA_NATIVE_PROBE_QUEUE OR
      MAIA_NATIVE_PROBE_FAULT OR MAIA_NATIVE_PROBE_STACK OR MAIA_NATIVE_PROBE_IDLE OR
      MAIA_NATIVE_PROBE_MIXED OR MAIA_NATIVE_PROBE_LIFECYCLE OR MAIA_NATIVE_PROBE_FP32))
    message(FATAL_ERROR "Serial TX probe uses a separate integer workload")
  endif()
  if(MAIA_NATIVE_PROBE_SERIAL AND (MAIA_NATIVE_PROBE_QUEUE OR MAIA_NATIVE_PROBE_FAULT OR
      MAIA_NATIVE_PROBE_STACK OR MAIA_NATIVE_PROBE_IDLE OR MAIA_NATIVE_PROBE_MIXED OR
      MAIA_NATIVE_PROBE_LIFECYCLE OR MAIA_NATIVE_PROBE_FP32))
    message(FATAL_ERROR "Serial probe uses a separate integer workload")
  endif()
  if(MAIA_NATIVE_PROBE_QUEUE AND (MAIA_NATIVE_PROBE_FAULT OR MAIA_NATIVE_PROBE_STACK OR
      MAIA_NATIVE_PROBE_IDLE OR MAIA_NATIVE_PROBE_MIXED OR MAIA_NATIVE_PROBE_LIFECYCLE OR MAIA_NATIVE_PROBE_FP32))
    message(FATAL_ERROR "Queue probe uses a separate integer workload")
  endif()
  if(MAIA_NATIVE_PROBE_FP32)
    target_compile_options(maia_sched_probe.elf PRIVATE -march=rv32imafc_zicsr_zifencei -mabi=ilp32f)
    target_link_options(maia_sched_probe.elf PRIVATE -march=rv32imafc_zicsr_zifencei -mabi=ilp32f)
    target_sources(maia_sched_probe.elf PRIVATE tests/hardware/native_fp_abi.c)
  endif()
  if(MAIA_NATIVE_PROBE_FAULT)
    if(MAIA_NATIVE_PROBE_FP32 OR MAIA_NATIVE_PROBE_MIXED)
      message(FATAL_ERROR "Fault probe uses its own integer-only workload")
    endif()
    if(MAIA_NATIVE_PROBE_FAULT STREQUAL "illegal16")
      set(MAIA_FAULT_KIND 1)
    elseif(MAIA_NATIVE_PROBE_FAULT STREQUAL "illegal32")
      set(MAIA_FAULT_KIND 2)
    else()
      set(MAIA_FAULT_KIND 3)
    endif()
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_PROBE_FAULT_KIND=${MAIA_FAULT_KIND})
  endif()
  if(MAIA_NATIVE_PROBE_IDLE)
    if(MAIA_NATIVE_PROBE_FAULT OR MAIA_NATIVE_PROBE_MIXED OR MAIA_NATIVE_PROBE_LIFECYCLE)
      message(FATAL_ERROR "Idle probe uses a separate workload")
    endif()
    if(MAIA_NATIVE_PROBE_EMPTY)
      target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_EMPTY=1)
    endif()
  endif()
  if(MAIA_NATIVE_PROBE_STACK)
    if(MAIA_NATIVE_PROBE_FAULT OR MAIA_NATIVE_PROBE_IDLE OR MAIA_NATIVE_PROBE_MIXED OR MAIA_NATIVE_PROBE_LIFECYCLE)
      message(FATAL_ERROR "Stack probe uses a separate workload")
    endif()
    set(MAIA_STACK_KINDS underflow guard incoming irqguard misaligned zero)
    list(FIND MAIA_STACK_KINDS ${MAIA_NATIVE_PROBE_STACK} MAIA_STACK_KIND)
    math(EXPR MAIA_STACK_KIND "${MAIA_STACK_KIND} + 1")
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_PROBE_STACK_KIND=${MAIA_STACK_KIND})
  endif()
  target_include_directories(maia_sched_probe.elf PRIVATE include)
  target_compile_options(maia_sched_probe.elf PRIVATE -Wall -Wextra -Wpedantic -Werror -O2)
  # The isolated probe is loaded with esptool load_ram, independent of product XIP.
  set(MAIA_PROBE_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boards/m5stack/tab5/linker.ld")
  if(MAIA_NATIVE_PROBE_SDMMC_BLOCK0 OR MAIA_NATIVE_PROBE_SDMMC_WRITE OR
     MAIA_NATIVE_PROBE_DPI_DMA_CENTER OR MAIA_NATIVE_PROBE_DPI_DMA_TILE OR
     MAIA_NATIVE_PROBE_VDMA_TRANSFER)
    if(MAIA_NATIVE_PROBE_SDMMC_BLOCK0 OR MAIA_NATIVE_PROBE_SDMMC_WRITE)
      set(MAIA_DMA_RESERVATION "tests/hardware/native_sdmmc_dma_memory.ld.in")
    elseif(MAIA_NATIVE_PROBE_DPI_DMA_CENTER)
      set(MAIA_DMA_RESERVATION "tests/hardware/native_dpi_center_memory.ld.in")
    elseif(MAIA_NATIVE_PROBE_DPI_DMA_TILE)
      set(MAIA_DMA_RESERVATION "tests/hardware/native_dpi_dma_memory.ld.in")
    else()
      set(MAIA_DMA_RESERVATION "tests/hardware/native_vdma_memory.ld.in")
    endif()
    file(READ "${MAIA_LINKER_SCRIPT}" MAIA_PROBE_LAYOUT)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${MAIA_DMA_RESERVATION}" MAIA_PROBE_DMA_SECTION)
    string(FIND "${MAIA_PROBE_LAYOUT}" "    .stack (NOLOAD)" MAIA_STACK_ANCHOR)
    if(MAIA_STACK_ANCHOR LESS 0)
      message(FATAL_ERROR "Canonical linker script missing DMA insertion anchor")
    endif()
    string(REPLACE "    .stack (NOLOAD)" "${MAIA_PROBE_DMA_SECTION}\n    .stack (NOLOAD)" MAIA_PROBE_LAYOUT "${MAIA_PROBE_LAYOUT}")
    set(MAIA_PROBE_LINKER_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/native_dma_probe_linker.ld")
    file(WRITE "${MAIA_PROBE_LINKER_SCRIPT}" "${MAIA_PROBE_LAYOUT}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MAIA_LINKER_SCRIPT}"
      "${CMAKE_CURRENT_SOURCE_DIR}/${MAIA_DMA_RESERVATION}")
  endif()
  target_link_options(maia_sched_probe.elf PRIVATE
    "-T${MAIA_PROBE_LINKER_SCRIPT}" "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/maia_sched_probe.map")
  target_link_libraries(maia_sched_probe.elf PRIVATE gcc)
  set_target_properties(maia_sched_probe.elf PROPERTIES LINK_DEPENDS "${MAIA_PROBE_LINKER_SCRIPT}")
  add_custom_command(TARGET maia_sched_probe.elf POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_native_probe.py
      --elf $<TARGET_FILE:maia_sched_probe.elf>
      --map ${CMAKE_CURRENT_BINARY_DIR}/maia_sched_probe.map --nm ${CMAKE_NM}
    VERBATIM)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/maia_sched_probe_ram.elf
    COMMAND ${CMAKE_OBJCOPY} --remove-section=.flash.rodata --remove-section=.flash.text
      $<TARGET_FILE:maia_sched_probe.elf> ${CMAKE_CURRENT_BINARY_DIR}/maia_sched_probe_ram.elf
    DEPENDS maia_sched_probe.elf VERBATIM)
  add_custom_target(maia_sched_probe_ram
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/maia_sched_probe_ram.elf)

  add_executable(echo_test.elf
    arch/riscv/start.S
    soc/espressif/esp32p4/soc_early_init.c
    soc/espressif/esp32p4/soc_console.c
    soc/espressif/esp32p4/soc_serial_rx.c
    soc/espressif/esp32p4/soc_serial_tx.c
    soc/espressif/esp32p4/soc_timer.c
    arch/riscv/stack.c
    soc/espressif/esp32p4/soc_panic.c
    soc/espressif/esp32p4/soc_board_io.c
    soc/espressif/esp32p4/soc_display.c
    boards/m5stack/tab5/app_desc.c
    examples/echo/main.c
    examples/echo/tab5_port.c
    examples/echo/board_impl.c
  )
  target_include_directories(echo_test.elf PRIVATE include)
  target_compile_options(echo_test.elf PRIVATE -Wall -Wextra -Wpedantic -Werror -O2)
  target_link_options(echo_test.elf PRIVATE
    "-T${MAIA_LINKER_SCRIPT}"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/echo_test.map"
  )
  set_target_properties(echo_test.elf PROPERTIES LINK_DEPENDS "${MAIA_LINKER_SCRIPT}")
  message(STATUS "Configured Tab5 Echo Test (echo_test.elf)")

  set(MAIA_EXTERNAL_RGB_TEST_DIR "" CACHE PATH "Optional external Tab5 RGB diagnostic source directory")
  if(MAIA_EXTERNAL_RGB_TEST_DIR)
    add_executable(rgb_test.elf
      arch/riscv/start.S
      soc/espressif/esp32p4/soc_early_init.c
      soc/espressif/esp32p4/soc_console.c
      soc/espressif/esp32p4/soc_serial_rx.c
      soc/espressif/esp32p4/soc_serial_tx.c
      soc/espressif/esp32p4/soc_timer.c
      arch/riscv/stack.c
      soc/espressif/esp32p4/soc_panic.c
      soc/espressif/esp32p4/soc_board_io.c
      soc/espressif/esp32p4/soc_display.c
      boards/m5stack/tab5/app_desc.c
      ${MAIA_EXTERNAL_RGB_TEST_DIR}/maia_rgb_test.c
      ${MAIA_EXTERNAL_RGB_TEST_DIR}/maia_tab5_display_port.c
    )
    target_include_directories(rgb_test.elf PRIVATE include ${MAIA_EXTERNAL_RGB_TEST_DIR})
    target_compile_options(rgb_test.elf PRIVATE -Wall -Wextra -O2)
    target_link_options(rgb_test.elf PRIVATE
      "-T${MAIA_LINKER_SCRIPT}"
      "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/rgb_test.map"
    )
    target_link_libraries(rgb_test.elf PRIVATE gcc)
    set_target_properties(rgb_test.elf PROPERTIES LINK_DEPENDS "${MAIA_LINKER_SCRIPT}")
    message(STATUS "Configured Tab5 RGB Diagnostic Test (rgb_test.elf)")
  endif()

else()
  option(MAIA_BUILD_HOST_TESTS "Build host-side contract tests" ON)
  if(MAIA_BUILD_HOST_TESTS)
    target_compile_definitions(maia_fatfs PRIVATE
      MAIA_FATFS_DIAGNOSTIC_FAULT_INJECTION=1
    )
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME native_probe_gate
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/native_probe_gate_test.py)

    add_executable(maia_contract_test tests/host/contract_test.c)
    target_include_directories(maia_contract_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(maia_contract_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME maia_contract_test COMMAND maia_contract_test)

    add_executable(shell_host_contract tests/host/shell_host_contract_test.c)
    target_include_directories(shell_host_contract PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(shell_host_contract PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME shell_host_contract COMMAND shell_host_contract)

    add_executable(esp32p4_regs_test tests/host/esp32p4_regs_test.c)
    target_include_directories(esp32p4_regs_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(esp32p4_regs_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME esp32p4_regs_test COMMAND esp32p4_regs_test)

    add_executable(sd_read_test tests/host/sd_read_test.c)
    target_link_libraries(sd_read_test PRIVATE maia_sd_read)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(sd_read_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME sd_read_test COMMAND sd_read_test)

    add_executable(fatfs_test tests/host/fatfs_test.c runtime/newlib_vfs.c)
    target_include_directories(fatfs_test PRIVATE third_party/fatfs/src)
    target_compile_definitions(fatfs_test PRIVATE
      MAIA_FATFS_DIAGNOSTIC_FAULT_INJECTION=1
    )
    target_link_libraries(fatfs_test PRIVATE maia_vfs)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(fatfs_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME fatfs_test COMMAND fatfs_test)

    add_executable(kernel_mvp_test
      tests/host/kernel_mvp_test.c
      kernel/kmem.c
      kernel/ksync.c
      kernel/ksched.c
      shell/maia_shell_mvp.c
    )
    target_include_directories(kernel_mvp_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(kernel_mvp_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME kernel_mvp_test COMMAND kernel_mvp_test)
    add_executable(newlib_locks_test
      tests/host/newlib_locks_test.c
      runtime/newlib_locks.c
      kernel/kmem.c
      kernel/ksync.c
    )
    target_include_directories(newlib_locks_test BEFORE PRIVATE
      tests/host/newlib_include
      include
    )
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(newlib_locks_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME newlib_locks_test COMMAND newlib_locks_test)
    add_executable(newlib_heap_test
      tests/host/newlib_heap_test.c
      runtime/newlib_heap.c
      runtime/newlib_locks.c
      kernel/kmem.c
      kernel/ksync.c
    )
    target_include_directories(newlib_heap_test BEFORE PRIVATE
      tests/host/newlib_include
      include
    )
    target_compile_definitions(newlib_heap_test PRIVATE MAIA_NEWLIB_HEAP_NO_CRT_EXPORTS=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(newlib_heap_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME newlib_heap_test COMMAND newlib_heap_test)
    add_executable(newlib_reent_test
      tests/host/newlib_reent_test.c
      runtime/newlib_reent.c
    )
    target_include_directories(newlib_reent_test BEFORE PRIVATE
      tests/host/newlib_include
      include
    )
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(newlib_reent_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME newlib_reent_test COMMAND newlib_reent_test)
    add_executable(newlib_syscalls_test
      tests/host/newlib_syscalls_test.c
      runtime/newlib_syscalls.c
    )
    target_include_directories(newlib_syscalls_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(newlib_syscalls_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME newlib_syscalls_test COMMAND newlib_syscalls_test)
    add_executable(gthread_test
      tests/host/gthread_test.c
      runtime/gthread.c
      kernel/ksync.c
    )
    target_include_directories(gthread_test BEFORE PRIVATE
      tests/host/newlib_include
      include
    )
    target_compile_definitions(gthread_test PRIVATE MAIA_GTHREAD_TEST=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(gthread_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME gthread_test COMMAND gthread_test)
    add_executable(tls_runtime_test
      tests/host/tls_runtime_test.c
      runtime/tls.c
    )
    target_include_directories(tls_runtime_test PRIVATE include)
    target_compile_definitions(tls_runtime_test PRIVATE MAIA_COMPILER_TLS_TEST=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(tls_runtime_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME tls_runtime_test COMMAND tls_runtime_test)
    add_executable(pthread_runtime_test
      tests/host/pthread_runtime_test.c
      runtime/pthread.c
    )
    target_include_directories(pthread_runtime_test BEFORE PRIVATE
      tests/host/newlib_include
      include
    )
    target_compile_definitions(pthread_runtime_test PRIVATE MAIA_PTHREAD_TEST=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(pthread_runtime_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME pthread_runtime_test COMMAND pthread_runtime_test)
    add_executable(kmem_regions_test tests/host/kmem_regions_test.c kernel/kmem.c)
    target_include_directories(kmem_regions_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(kmem_regions_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME kmem_regions_test COMMAND kmem_regions_test)
    # Exercise the larger RV32F frame initialization/bounds on the host too.
    add_executable(kernel_fp_frame_test
      tests/host/kernel_mvp_test.c kernel/kmem.c kernel/ksync.c kernel/ksched.c
      shell/maia_shell_mvp.c)
    target_include_directories(kernel_fp_frame_test PRIVATE include)
    target_compile_definitions(kernel_fp_frame_test PRIVATE __riscv_flen=32)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(kernel_fp_frame_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME kernel_fp_frame_test COMMAND kernel_fp_frame_test)
    add_executable(riscv_trap_policy_test tests/host/riscv_trap_policy_test.c)
    target_include_directories(riscv_trap_policy_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(riscv_trap_policy_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME riscv_trap_policy_test COMMAND riscv_trap_policy_test)
    add_executable(dsi_dpi_test tests/host/dsi_dpi_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_dpi_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_dpi_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1 MAIA_DSI_SPATIAL_PROBE=1)
    target_compile_options(dsi_dpi_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_dpi_test COMMAND dsi_dpi_test)
    add_executable(lcd_output_test tests/host/lcd_output_test.c services/lcd_output.c)
    target_include_directories(lcd_output_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(lcd_output_test PRIVATE MAIA_LCD_OUTPUT_TEST=1)
    target_compile_options(lcd_output_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME lcd_output_test COMMAND lcd_output_test)
    add_executable(product_terminal_test tests/host/product_terminal_test.c)
    target_include_directories(product_terminal_test PRIVATE include soc/espressif/esp32p4)
    target_compile_options(product_terminal_test PRIVATE -Wall -Wextra -Werror -ffunction-sections -fdata-sections)
    if(APPLE)
      target_link_options(product_terminal_test PRIVATE "-Wl,-dead_strip")
    else()
      target_link_options(product_terminal_test PRIVATE "-Wl,--gc-sections")
    endif()
    add_test(NAME product_terminal_test COMMAND product_terminal_test)
    add_executable(product_touch_test tests/host/product_touch_test.c)
    target_include_directories(product_touch_test PRIVATE include)
    target_compile_definitions(product_touch_test PRIVATE MAIA_PRODUCT_TOUCH=1)
    target_compile_options(product_touch_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME product_touch_test COMMAND product_touch_test)
    add_executable(dw_dma_irq_test tests/host/dw_dma_irq_test.c soc/espressif/esp32p4/soc_dw_dma_irq.c)
    target_include_directories(dw_dma_irq_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dw_dma_irq_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(dw_dma_irq_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME dw_dma_irq_test COMMAND dw_dma_irq_test)
    add_executable(dsi_lcd_async_test tests/host/dsi_dpi_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_lcd_async_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_lcd_async_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1 MAIA_DSI_SPATIAL_PROBE=1 MAIA_DSI_FRAMEBUFFER_PROBE=1 MAIA_DSI_LCD_STREAM=1)
    target_compile_options(dsi_lcd_async_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME dsi_lcd_async_test COMMAND dsi_lcd_async_test)
    add_executable(dsi_framebuffer_test tests/host/dsi_dpi_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_framebuffer_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_framebuffer_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1 MAIA_DSI_SPATIAL_PROBE=1 MAIA_DSI_FRAMEBUFFER_PROBE=1)
    target_compile_options(dsi_framebuffer_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_framebuffer_test COMMAND dsi_framebuffer_test)
    add_executable(dsi_center_test tests/host/dsi_dpi_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_center_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_center_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1 MAIA_DSI_SPATIAL_PROBE=1 MAIA_DSI_SPATIAL_CENTER=1)
    target_compile_options(dsi_center_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_center_test COMMAND dsi_center_test)
    add_executable(dsi_read_test tests/host/dsi_read_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_read_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_read_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1)
    target_compile_options(dsi_read_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_read_test COMMAND dsi_read_test)
    add_executable(dsi_long_test tests/host/dsi_long_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_long_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_long_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1)
    target_compile_options(dsi_long_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_long_test COMMAND dsi_long_test)
    add_executable(dsi_short_test tests/host/dsi_short_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_short_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_short_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1)
    target_compile_options(dsi_short_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_short_test COMMAND dsi_short_test)
    add_executable(dsi_command_test tests/host/dsi_command_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_command_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_command_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1)
    target_compile_options(dsi_command_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_command_test COMMAND dsi_command_test)
    add_executable(dsi_phy_test tests/host/dsi_phy_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_phy_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_phy_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1)
    target_compile_options(dsi_phy_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_phy_test COMMAND dsi_phy_test)
    add_executable(dsi_reference_test tests/host/dsi_reference_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_reference_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_reference_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1)
    target_compile_options(dsi_reference_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_reference_test COMMAND dsi_reference_test)
    add_executable(dsi_access_test tests/host/dsi_access_test.c soc/espressif/esp32p4/soc_dsi_access.c
      soc/espressif/esp32p4/soc_ldo3_plan.c soc/espressif/esp32p4/soc_ldo3_snapshot.c)
    target_include_directories(dsi_access_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_access_test PRIVATE MAIA_DSI_ACCESS_TEST=1 MAIA_LDO3_TEST=1)
    target_compile_options(dsi_access_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_access_test COMMAND dsi_access_test)
    add_executable(vo3_csi_prepare_test tests/host/vo3_csi_prepare_test.c)
    target_include_directories(vo3_csi_prepare_test PRIVATE include tests/hardware)
    target_compile_options(vo3_csi_prepare_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME vo3_csi_prepare_test COMMAND vo3_csi_prepare_test)
    add_executable(vo3_cycle_fixture_test tests/host/vo3_cycle_fixture_test.c)
    target_include_directories(vo3_cycle_fixture_test PRIVATE include tests/hardware)
    target_compile_options(vo3_cycle_fixture_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME vo3_cycle_fixture_test COMMAND vo3_cycle_fixture_test)
    add_executable(st7121_program_test tests/host/st7121_program_test.c boards/m5stack/tab5/st7121.c)
    target_include_directories(st7121_program_test PRIVATE include)
    target_compile_definitions(st7121_program_test PRIVATE MAIA_ST7121_TEST=1)
    target_compile_options(st7121_program_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME st7121_program_test COMMAND st7121_program_test)
    add_executable(dsi_reset_integration_test tests/host/dsi_reset_integration_test.c)
    target_include_directories(dsi_reset_integration_test PRIVATE include tests/hardware)
    target_compile_options(dsi_reset_integration_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME dsi_reset_integration_test COMMAND dsi_reset_integration_test)
    add_executable(dsi_spatial_plan_test tests/host/dsi_spatial_plan_test.c)
    target_include_directories(dsi_spatial_plan_test PRIVATE include soc/espressif/esp32p4)
    target_compile_options(dsi_spatial_plan_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME dsi_spatial_plan_test COMMAND dsi_spatial_plan_test)
    add_executable(framebuffer_pattern_test tests/host/framebuffer_pattern_test.c)
    target_include_directories(framebuffer_pattern_test PRIVATE tests/hardware)
    target_compile_options(framebuffer_pattern_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME framebuffer_pattern_test COMMAND framebuffer_pattern_test)
    add_executable(psram_dma_plan_test tests/host/psram_dma_plan_test.c)
    target_include_directories(psram_dma_plan_test PRIVATE tests/hardware)
    target_compile_options(psram_dma_plan_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME psram_dma_plan_test COMMAND psram_dma_plan_test)
    add_executable(psram_window_test tests/host/psram_window_test.c soc/espressif/esp32p4/soc_psram_window.c soc/espressif/esp32p4/soc_psram_snapshot.c soc/espressif/esp32p4/soc_psram_mode.c)
    target_include_directories(psram_window_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(psram_window_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(psram_window_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME psram_window_test COMMAND psram_window_test)
    add_executable(psram_array_test tests/host/psram_array_test.c soc/espressif/esp32p4/soc_psram_snapshot.c soc/espressif/esp32p4/soc_psram_mode.c soc/espressif/esp32p4/soc_psram_array.c)
    target_include_directories(psram_array_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(psram_array_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(psram_array_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME psram_array_test COMMAND psram_array_test)
    add_executable(psram_mpll_init_test tests/host/psram_mpll_init_test.c soc/espressif/esp32p4/soc_psram_mpll.c)
    target_include_directories(psram_mpll_init_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(psram_mpll_init_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(psram_mpll_init_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME psram_mpll_init_test COMMAND psram_mpll_init_test)
    add_executable(psram_mpll_test tests/host/psram_mpll_test.c soc/espressif/esp32p4/soc_psram_mpll.c)
    target_include_directories(psram_mpll_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(psram_mpll_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(psram_mpll_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME psram_mpll_test COMMAND psram_mpll_test)
    add_executable(psram_boot_test tests/host/psram_boot_test.c soc/espressif/esp32p4/soc_psram_boot.c
      soc/espressif/esp32p4/soc_psram_clock.c soc/espressif/esp32p4/soc_psram_supply.c
      soc/espressif/esp32p4/soc_psram_snapshot.c soc/espressif/esp32p4/soc_psram_mpll.c)
    target_include_directories(psram_boot_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(psram_boot_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(psram_boot_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME psram_boot_test COMMAND psram_boot_test)
    add_executable(psram_supply_test tests/host/psram_supply_test.c soc/espressif/esp32p4/soc_psram_supply.c)
    target_include_directories(psram_supply_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(psram_supply_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(psram_supply_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME psram_supply_test COMMAND psram_supply_test)
    add_executable(psram_park_test tests/host/psram_park_test.c soc/espressif/esp32p4/soc_psram_snapshot.c soc/espressif/esp32p4/soc_psram_clock.c soc/espressif/esp32p4/soc_psram_supply.c)
    target_include_directories(psram_park_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(psram_park_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(psram_park_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME psram_park_test COMMAND psram_park_test)
    add_executable(psram_clock_test tests/host/psram_clock_test.c soc/espressif/esp32p4/soc_psram_snapshot.c soc/espressif/esp32p4/soc_psram_clock.c soc/espressif/esp32p4/soc_psram_supply.c)
    target_include_directories(psram_clock_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(psram_clock_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(psram_clock_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME psram_clock_test COMMAND psram_clock_test)
    add_executable(psram_mode_test tests/host/psram_mode_test.c soc/espressif/esp32p4/soc_psram_snapshot.c soc/espressif/esp32p4/soc_psram_mode.c)
    target_include_directories(psram_mode_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(psram_mode_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(psram_mode_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME psram_mode_test COMMAND psram_mode_test)
    add_executable(psram_snapshot_test tests/host/psram_snapshot_test.c soc/espressif/esp32p4/soc_psram_snapshot.c)
    target_include_directories(psram_snapshot_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(psram_snapshot_test PRIVATE MAIA_PSRAM_TEST=1)
    target_compile_options(psram_snapshot_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME psram_snapshot_test COMMAND psram_snapshot_test)
    add_executable(dsi_center_plan_test tests/host/dsi_spatial_plan_test.c)
    target_include_directories(dsi_center_plan_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(dsi_center_plan_test PRIVATE MAIA_DSI_SPATIAL_CENTER=1)
    target_compile_options(dsi_center_plan_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME dsi_center_plan_test COMMAND dsi_center_plan_test)
    add_executable(dsi_dma_descriptor_test tests/host/dsi_dma_descriptor_test.c)
    target_include_directories(dsi_dma_descriptor_test PRIVATE soc/espressif/esp32p4)
    target_compile_options(dsi_dma_descriptor_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME dsi_dma_descriptor_test COMMAND dsi_dma_descriptor_test)
    add_executable(dpi_backlight_fixture_test tests/host/dpi_backlight_fixture_test.c)
    target_include_directories(dpi_backlight_fixture_test PRIVATE include)
    target_compile_options(dpi_backlight_fixture_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME dpi_backlight_fixture_test COMMAND dpi_backlight_fixture_test)
    add_executable(mipi_gate_test tests/host/mipi_gate_test.c soc/espressif/esp32p4/soc_mipi_park.c)
    target_include_directories(mipi_gate_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(mipi_gate_test PRIVATE MAIA_MIPI_PARK_TEST=1)
    target_compile_options(mipi_gate_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME mipi_gate_test COMMAND mipi_gate_test)
    add_executable(mipi_park_test tests/host/mipi_park_test.c soc/espressif/esp32p4/soc_mipi_park.c)
    target_include_directories(mipi_park_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(mipi_park_test PRIVATE MAIA_MIPI_PARK_TEST=1)
    target_compile_options(mipi_park_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME mipi_park_test COMMAND mipi_park_test)
    add_executable(vdma_list_fixture_test tests/host/vdma_list_fixture_test.c)
    target_include_directories(vdma_list_fixture_test PRIVATE include tests/hardware)
    target_compile_options(vdma_list_fixture_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME vdma_list_fixture_test COMMAND vdma_list_fixture_test)
    add_executable(vdma_transfer_fixture_test tests/host/vdma_transfer_fixture_test.c)
    target_include_directories(vdma_transfer_fixture_test PRIVATE include tests/hardware)
    target_compile_options(vdma_transfer_fixture_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME vdma_transfer_fixture_test COMMAND vdma_transfer_fixture_test)
    add_executable(vdma_stop_test tests/host/vdma_stop_test.c soc/espressif/esp32p4/soc_vdma_stop.c)
    target_include_directories(vdma_stop_test PRIVATE include soc/espressif/esp32p4)
    target_compile_definitions(vdma_stop_test PRIVATE MAIA_VDMA_TEST=1)
    target_compile_options(vdma_stop_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME vdma_stop_test COMMAND vdma_stop_test)
    add_executable(mipi_snapshot_test tests/host/mipi_snapshot_test.c)
    target_include_directories(mipi_snapshot_test PRIVATE include tests/hardware)
    target_compile_options(mipi_snapshot_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME mipi_snapshot_test COMMAND mipi_snapshot_test)

    add_executable(sdmmc_snapshot_test tests/host/sdmmc_snapshot_test.c)
    target_include_directories(sdmmc_snapshot_test PRIVATE include tests/hardware)
    target_compile_options(sdmmc_snapshot_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME sdmmc_snapshot_test COMMAND sdmmc_snapshot_test)
    add_executable(tab5_sdmmc_boot_test tests/host/tab5_sdmmc_boot_test.c
      boards/m5stack/tab5/sdmmc_boot.c drivers/storage/sd_read.c)
    target_include_directories(tab5_sdmmc_boot_test PRIVATE include)
    target_compile_definitions(tab5_sdmmc_boot_test PRIVATE MAIA_SDMMC_BOOT_TEST=1)
    target_compile_options(tab5_sdmmc_boot_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME tab5_sdmmc_boot_test COMMAND tab5_sdmmc_boot_test)
    add_executable(tab5_sdmmc_cmd8_fault_test tests/host/tab5_sdmmc_boot_test.c
      boards/m5stack/tab5/sdmmc_boot.c drivers/storage/sd_read.c)
    target_include_directories(tab5_sdmmc_cmd8_fault_test PRIVATE include)
    target_compile_definitions(tab5_sdmmc_cmd8_fault_test PRIVATE
      MAIA_SDMMC_BOOT_TEST=1 MAIA_SDMMC_DIAGNOSTIC_CMD8_TIMEOUT=1)
    target_compile_options(tab5_sdmmc_cmd8_fault_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME tab5_sdmmc_cmd8_fault_test COMMAND tab5_sdmmc_cmd8_fault_test)
    add_executable(ldo3_acquire_test tests/host/ldo3_acquire_test.c
      soc/espressif/esp32p4/soc_ldo3_acquire.c soc/espressif/esp32p4/soc_ldo3_snapshot.c
      soc/espressif/esp32p4/soc_ldo3_plan.c)
    target_include_directories(ldo3_acquire_test PRIVATE include)
    target_compile_definitions(ldo3_acquire_test PRIVATE MAIA_LDO3_TEST=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(ldo3_acquire_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME ldo3_acquire_test COMMAND ldo3_acquire_test)
    add_executable(ldo3_plan_test tests/host/ldo3_plan_test.c soc/espressif/esp32p4/soc_ldo3_plan.c)
    target_include_directories(ldo3_plan_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(ldo3_plan_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME ldo3_plan_test COMMAND ldo3_plan_test)
    add_executable(tab5_panel_reset_test tests/host/tab5_panel_reset_test.c
      boards/m5stack/tab5/panel_reset.c drivers/gpio/pi4ioe5v6408.c)
    target_include_directories(tab5_panel_reset_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(tab5_panel_reset_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME tab5_panel_reset_test COMMAND tab5_panel_reset_test)
    add_executable(pi4ioe5v6408_test tests/host/pi4ioe5v6408_test.c drivers/gpio/pi4ioe5v6408.c)
    target_include_directories(pi4ioe5v6408_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(pi4ioe5v6408_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME pi4ioe5v6408_test COMMAND pi4ioe5v6408_test)
    add_executable(touch_input_test tests/host/touch_input_test.c services/touch_input.c drivers/input/st712x.c)
    target_include_directories(touch_input_test PRIVATE include)
    target_compile_definitions(touch_input_test PRIVATE MAIA_TOUCH_INPUT_TEST=1)
    target_compile_options(touch_input_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME touch_input_test COMMAND touch_input_test)
    add_executable(i2c_bus_test tests/host/i2c_bus_test.c services/i2c_bus.c)
    target_include_directories(i2c_bus_test PRIVATE include)
    target_compile_options(i2c_bus_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME i2c_bus_test COMMAND i2c_bus_test)
    add_executable(i2c_bitbang_test tests/host/i2c_bitbang_test.c drivers/i2c/bitbang.c
      boards/m5stack/tab5/touch_identify.c)
    target_include_directories(i2c_bitbang_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(i2c_bitbang_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME i2c_bitbang_test COMMAND i2c_bitbang_test)
    add_executable(serial_output_test tests/host/serial_output_test.c services/serial_output.c
      soc/espressif/esp32p4/soc_serial_tx.c soc/espressif/esp32p4/soc_serial_rx.c
      soc/espressif/esp32p4/soc_console.c kernel/ksync.c)
    target_include_directories(serial_output_test PRIVATE include)
    target_compile_definitions(serial_output_test PRIVATE MAIA_SERIAL_TX_TEST=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(serial_output_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME serial_output_test COMMAND serial_output_test)
    add_executable(serial_input_test tests/host/serial_input_test.c services/serial_input.c
      soc/espressif/esp32p4/soc_serial_rx.c soc/espressif/esp32p4/soc_serial_tx.c soc/espressif/esp32p4/soc_console.c kernel/ksync.c)
    target_include_directories(serial_input_test PRIVATE include)
    target_compile_definitions(serial_input_test PRIVATE MAIA_SERIAL_RX_TEST=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(serial_input_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME serial_input_test COMMAND serial_input_test)
    add_executable(queue_atomic_test tests/host/queue_atomic_test.c kernel/ksync.c)
    target_include_directories(queue_atomic_test PRIVATE include)
    target_compile_definitions(queue_atomic_test PRIVATE MAIA_QUEUE_TEST=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(queue_atomic_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME queue_atomic_test COMMAND queue_atomic_test)
    foreach(profile integer fp32)
      add_executable(notification_${profile}_test tests/host/notification_test.c kernel/ksched.c)
      target_include_directories(notification_${profile}_test PRIVATE include)
      target_compile_definitions(notification_${profile}_test PRIVATE MAIA_SCHED_TEST=1)
      if(profile STREQUAL "fp32")
        target_compile_definitions(notification_${profile}_test PRIVATE __riscv_flen=32)
      endif()
      if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(notification_${profile}_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
      endif()
      add_test(NAME notification_${profile}_test COMMAND notification_${profile}_test)
    endforeach()
    add_executable(scheduler_atomic_test tests/host/scheduler_atomic_test.c kernel/ksched.c)
    target_include_directories(scheduler_atomic_test PRIVATE include)
    target_compile_definitions(scheduler_atomic_test PRIVATE MAIA_SCHED_TEST=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(scheduler_atomic_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME scheduler_atomic_test COMMAND scheduler_atomic_test)
    add_executable(scheduler_idle_test tests/host/scheduler_idle_test.c kernel/ksched.c)
    target_include_directories(scheduler_idle_test PRIVATE include)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(scheduler_idle_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME scheduler_idle_test COMMAND scheduler_idle_test)
    foreach(profile integer fp32)
      add_executable(scheduler_stack_${profile}_test tests/host/scheduler_stack_test.c kernel/ksched.c)
      target_include_directories(scheduler_stack_${profile}_test PRIVATE include)
      if(profile STREQUAL "fp32")
        target_compile_definitions(scheduler_stack_${profile}_test PRIVATE __riscv_flen=32)
      endif()
      if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(scheduler_stack_${profile}_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
      endif()
      add_test(NAME scheduler_stack_${profile}_test COMMAND scheduler_stack_${profile}_test)
    endforeach()



    add_test(NAME f0_flash_runner_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/f0_flash_runner_test.py)
    add_test(NAME psram_native_boot_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/psram_native_boot_capture_test.py)
    add_test(NAME psram_boot_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/psram_boot_capture_test.py)
    add_test(NAME psram_boot_transport_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/psram_boot_transport_test.py)
    add_test(NAME psram_boot_connection_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/psram_boot_connection_test.py)
    add_test(NAME psram_park_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/psram_park_capture_test.py)
    add_test(NAME psram_supply_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/psram_supply_capture_test.py)
    add_test(NAME psram_profile_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/psram_profile_capture_test.py)
    add_test(NAME mpll_initialize_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/mpll_initialize_capture_test.py)
    add_test(NAME input_lifecycle_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/input_lifecycle_capture_test.py)
    add_test(NAME touch_lcd_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/touch_lcd_capture_test.py)
    add_test(NAME lcd_lifecycle_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/lcd_lifecycle_capture_test.py)
    add_test(NAME lcd_stream_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/lcd_stream_capture_test.py)
    add_test(NAME framebuffer_capture_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/framebuffer_capture_test.py)
    add_test(NAME native_probe_result_test
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/native_probe_result_test.py)
    add_test(NAME sdmmc_cmd8_runner_test
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/sdmmc_cmd8_runner_test.py)
    add_test(NAME sdmmc_block0_runner_test
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/sdmmc_block0_runner_test.py)
    add_test(NAME sdmmc_runtime_probe_test
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/sdmmc_runtime_probe_test.py)

    # The current shell dispatches assembler/compiler/module commands too.
    # Link their real host implementations rather than substituting stubs.
    # This library belongs only to the hosted branch, never the native kernel.
    add_library(maia_shell_host_tools STATIC
      components/exec/elf_validator.c
      components/exec/elf_loader.c
      assembler/src/riscv_isa.cpp
      assembler/src/riscv_asm.cpp
      assembler/src/riscv_disasm.cpp
      maiascript/src/parser.cpp
      maiascript/src/codegen_riscv.cpp
      lib/maiaregex/maia_regex.c
      lib/maiaglob/maia_glob.c
      lib/maiaglob/maia_glob_compile.cpp
    )
    target_include_directories(maia_shell_host_tools PUBLIC
      include assembler/include maiascript/include lib/maiaregex lib/maiaglob)
    set_target_properties(maia_shell_host_tools PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

    add_executable(shell_expr_control_test tests/host/shell_expr_control_test.cpp)
    target_link_libraries(shell_expr_control_test PRIVATE maia_shell_host_tools)
    set_target_properties(shell_expr_control_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
    add_test(NAME shell_expr_control_test COMMAND shell_expr_control_test)
  endif()
  message(STATUS "Host contract tests configured")
endif()
