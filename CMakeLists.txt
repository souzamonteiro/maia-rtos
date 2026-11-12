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
    soc/espressif/esp32p4/soc_serial_rx.c
    soc/espressif/esp32p4/soc_serial_tx.c
    soc/espressif/esp32p4/soc_timer.c
    arch/riscv/stack.c
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
  option(MAIA_NATIVE_PROBE_LDO3_ACQUIRE "Exercise boot-only VO3 acquisition" OFF)
  option(MAIA_NATIVE_PROBE_LDO3_CYCLE "Controlled VO3 disable/enable diagnostic with DSI gated" OFF)
  if(MAIA_NATIVE_PROBE_LDO3_ACQUIRE AND NOT MAIA_NATIVE_PROBE_LDO3)
    message(FATAL_ERROR "LDO3 acquisition requires MAIA_NATIVE_PROBE_LDO3")
  endif()
  if(MAIA_NATIVE_PROBE_LDO3_CYCLE AND NOT MAIA_NATIVE_PROBE_LDO3_ACQUIRE)
    message(FATAL_ERROR "LDO3 cycle requires MAIA_NATIVE_PROBE_LDO3_ACQUIRE")
  endif()
  option(MAIA_NATIVE_PROBE_LDO3 "Read-only VO3 calibration diagnostic" OFF)
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
  if(MAIA_NATIVE_PROBE_LDO3)
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
