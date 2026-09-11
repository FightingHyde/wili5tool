cmake_minimum_required(VERSION 3.20)

set(PICO_BOARD "pico2")
set(PICO_PLATFORM "rp2350-arm-s")

include(pico_sdk_import.cmake)

project(wili5tool C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(wilibsp)
add_subdirectory(app)