cmake_minimum_required(VERSION 3.20)
project(MaiaRTOS VERSION 0.1.0 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
  set(MAIA_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boards/m5stack/tab5/linker.ld")
  add_executable(maia_m0.elf
    arch/riscv/start.S
    arch/riscv/trap_context.S
    soc/espressif/esp32p4/soc_early_init.c
    soc/espressif/esp32p4/soc_console.c
    soc/espressif/esp32p4/soc_timer.c
    soc/espressif/esp32p4/soc_board_io.c
    soc/espressif/esp32p4/soc_display.c
    kernel/kmem.c
    kernel/ksync.c
    kernel/ksched.c
    shell/maia_shell_engine.cpp
    boards/m5stack/tab5/app_desc.c
    boards/m5stack/tab5/boot_main.c
  )
  target_include_directories(maia_m0.elf PRIVATE include)
  target_compile_options(maia_m0.elf PRIVATE -Wall -Wextra -O2)
  target_link_options(maia_m0.elf PRIVATE
    "-T${MAIA_LINKER_SCRIPT}"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/maia_m0.map"
  )
  target_link_libraries(maia_m0.elf PRIVATE gcc)
  set_target_properties(maia_m0.elf PROPERTIES LINK_DEPENDS "${MAIA_LINKER_SCRIPT}")
  message(STATUS "Configured MaiaRTOS bare-metal target (maia_m0.elf) for ESP32-P4 / M5Stack Tab5")

  add_executable(echo_test.elf
    arch/riscv/start.S
    soc/espressif/esp32p4/soc_early_init.c
    soc/espressif/esp32p4/soc_console.c
    soc/espressif/esp32p4/soc_timer.c
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
else()
  option(MAIA_BUILD_HOST_TESTS "Build host-side contract tests" ON)
  if(MAIA_BUILD_HOST_TESTS)
    enable_testing()

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

    add_executable(shell_expr_control_test tests/host/shell_expr_control_test.cpp)
    target_include_directories(shell_expr_control_test PRIVATE include)
    set_target_properties(shell_expr_control_test PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
    add_test(NAME shell_expr_control_test COMMAND shell_expr_control_test)
  endif()
  message(STATUS "Host contract tests configured")
endif()
