cmake_minimum_required(VERSION 3.20)

# Point the SDK at the FreeWili2 custom board header BEFORE importing the SDK.
# freewili2.h lives in wilibsp/bsp/boards/ and defines PICO_PSRAM_SIZE_BYTES,
# PICO_PLATFORM (rp2350-arm-s), flash size, and all other board constants.
# Do NOT set PICO_BOARD on the cmake command line — that would override this.
set(PICO_BOARD freewili2 CACHE STRING "Board type")
list(APPEND PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_LIST_DIR}/wilibsp/bsp/boards")

include(pico_sdk_import.cmake)

project(wili5tool C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(wilibsp)
add_subdirectory(app)