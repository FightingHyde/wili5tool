cmake_minimum_required(VERSION 3.20)

# ── Board config — MUST come before pico_sdk_import.cmake ──────────────────
# The freewili2 board header lives inside wilibsp and defines PICO_PSRAM_SIZE_BYTES,
# PICO_PLATFORM (rp2350-arm-s), and all other board constants.
# Do NOT pass -DPICO_BOARD on the cmake command line; it would override this.
set(PICO_BOARD freewili2 CACHE STRING "Board type")
list(APPEND PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_LIST_DIR}/wilibsp/bsp/boards")

# ── PIO-USB path — MUST come before pico_sdk_init() ────────────────────────
# The Pico SDK creates the tinyusb_pico_pio_usb target (with pio_usb.h include
# paths) only at init time. wilibsp/bsp links against that target, so the path
# must be known before pico_sdk_init() runs — not inside add_subdirectory(wilibsp).
set(PICO_PIO_USB_PATH "${CMAKE_CURRENT_LIST_DIR}/wilibsp/bsp/third_party/Pico-PIO-USB")

include(pico_sdk_import.cmake)

project(wili5tool C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(wilibsp)
add_subdirectory(app)