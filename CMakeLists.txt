# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(lil_guy C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(lil_guy
    main.c
    display.c
    )

# Add current directory to include path for lwipopts.h
target_include_directories(lil_guy PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )

# pull in common dependencies
target_link_libraries(lil_guy
    pico_stdlib
    hardware_spi
    hardware_i2c
    hardware_gpio
    hardware_pwm
    hardware_adc
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background
)

# enable usb output, disable uart output
pico_enable_stdio_usb(lil_guy 1)
pico_enable_stdio_uart(lil_guy 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(lil_guy)
