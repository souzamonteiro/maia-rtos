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
    soc/espressif/esp32p4/soc_panic.c
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

  # Independent scheduler diagnostic; does not link the shell/vendor platform.
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(MAIA_NATIVE_PROBE_FAULT "" CACHE STRING "RAM fault diagnostic: illegal16, illegal32 or breakpoint16")
  if(MAIA_NATIVE_PROBE_FAULT)
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
    soc/espressif/esp32p4/soc_timer.c
    soc/espressif/esp32p4/soc_panic.c
    kernel/ksched.c
    boards/m5stack/tab5/app_desc.c
    ${MAIA_PROBE_APP}
  )
  option(MAIA_NATIVE_PROBE_FP32 "Exercise native RV32F/ILP32F context (RAM diagnostic only)" OFF)
  option(MAIA_NATIVE_PROBE_MIXED "Exercise mixed FS states, yield, delay and priorities" OFF)
  if(MAIA_NATIVE_PROBE_MIXED)
    if(NOT MAIA_NATIVE_PROBE_FP32)
      message(FATAL_ERROR "MAIA_NATIVE_PROBE_MIXED requires MAIA_NATIVE_PROBE_FP32")
    endif()
    target_compile_definitions(maia_sched_probe.elf PRIVATE MAIA_NATIVE_PROBE_MIXED=1)
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
  target_include_directories(maia_sched_probe.elf PRIVATE include)
  target_compile_options(maia_sched_probe.elf PRIVATE -Wall -Wextra -Wpedantic -Werror -O2)
  target_link_options(maia_sched_probe.elf PRIVATE
    "-T${MAIA_LINKER_SCRIPT}" "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/maia_sched_probe.map")
  target_link_libraries(maia_sched_probe.elf PRIVATE gcc)
  set_target_properties(maia_sched_probe.elf PROPERTIES LINK_DEPENDS "${MAIA_LINKER_SCRIPT}")
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
    soc/espressif/esp32p4/soc_timer.c
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
      soc/espressif/esp32p4/soc_timer.c
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
    add_test(NAME native_probe_result_test
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/native_probe_result_test.py)

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
