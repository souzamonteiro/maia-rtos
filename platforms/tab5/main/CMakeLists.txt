file(GLOB_RECURSE APP_LAYER_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/../vendor_app/*.c
    ${CMAKE_CURRENT_LIST_DIR}/../vendor_app/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/../vendor_app/*.cpp
)

set(APP_LAYER_INCS
    ${CMAKE_CURRENT_LIST_DIR}/../vendor_app
)

file(GLOB_RECURSE MY_HAL_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/hal/*.c
    ${CMAKE_CURRENT_LIST_DIR}/hal/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/hal/*.cpp
)

get_filename_component(MAIA_RTOS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

idf_component_register(SRCS "app_main.cpp"
                            "${MAIA_RTOS_ROOT}/components/exec/elf_validator.c"
                            "${MAIA_RTOS_ROOT}/components/exec/elf_loader.c"
                            "${MAIA_RTOS_ROOT}/assembler/src/riscv_isa.cpp"
                            "${MAIA_RTOS_ROOT}/assembler/src/riscv_asm.cpp"
                            "${MAIA_RTOS_ROOT}/assembler/src/riscv_disasm.cpp"
                            "${MAIA_RTOS_ROOT}/maiascript/src/parser.cpp"
                            "${MAIA_RTOS_ROOT}/maiascript/src/codegen_riscv.cpp"
                            "${MAIA_RTOS_ROOT}/lib/maiaregex/maia_regex.c"
                            "${MAIA_RTOS_ROOT}/lib/maiaglob/maia_glob.c"
                            "${MAIA_RTOS_ROOT}/lib/maiaglob/maia_glob_compile.cpp"
                            ${APP_LAYER_SRCS} ${MY_HAL_SRCS}
                    INCLUDE_DIRS "." ${APP_LAYER_INCS}
                                 "${MAIA_RTOS_ROOT}/include"
                                 "${MAIA_RTOS_ROOT}/assembler/include"
                                 "${MAIA_RTOS_ROOT}/maiascript/include"
                                 "${MAIA_RTOS_ROOT}/lib/maiaregex"
                                 "${MAIA_RTOS_ROOT}/lib/maiaglob"
                    EMBED_TXTFILES "../audio/canon_in_d.mp3" "../audio/startup_sfx.mp3" "../audio/shutdown_sfx.mp3")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-stringop-truncation -Wno-unused-variable -Wno-format)
