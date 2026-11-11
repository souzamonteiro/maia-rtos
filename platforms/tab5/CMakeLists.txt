cmake_minimum_required(VERSION 3.5)

option(MAIA_TAB5_SINGLECORE_DRIVERS
    "Experimental migration profile: keep legacy driver services on one core" OFF)

if(MAIA_TAB5_SINGLECORE_DRIVERS)
    message(STATUS "Experimental legacy driver profile: one-core FreeRTOS; native scheduler not selected")
    # Never turn the known working build/configuration into the experiment.
    if(CMAKE_BINARY_DIR STREQUAL "${CMAKE_CURRENT_LIST_DIR}/build")
        message(FATAL_ERROR "Use a separate build directory for MAIA_TAB5_SINGLECORE_DRIVERS")
    endif()
    set(_maia_profile_config "${CMAKE_BINARY_DIR}/sdkconfig.singlecore")
    if(NOT EXISTS "${_maia_profile_config}" AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")
        # Preserve the tested board, memory and peripheral settings. Only the
        # core policy is changed; Kconfig derives dependent settings afterwards.
        file(READ "${CMAKE_CURRENT_LIST_DIR}/sdkconfig" _maia_config_seed)
        string(REGEX REPLACE "(^|\n)(# )?CONFIG_FREERTOS_UNICORE[^\n]*" ""
            _maia_config_seed "${_maia_config_seed}")
        file(WRITE "${_maia_profile_config}" "${_maia_config_seed}\nCONFIG_FREERTOS_UNICORE=y\n")
    endif()
    set(SDKCONFIG "${_maia_profile_config}" CACHE FILEPATH "Isolated single-core configuration" FORCE)
    set(SDKCONFIG_DEFAULTS
        "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults;${CMAKE_CURRENT_LIST_DIR}/sdkconfig.singlecore.defaults"
        CACHE STRING "Single-core migration defaults" FORCE)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_CURRENT_LIST_DIR}/dependencies"
)

project(m5stack_tab5)
