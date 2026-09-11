set(APP_SOURCES
    main.c
    ui.c
    storage.c
    subghz/subghz_app.c
    subghz/sub_file.c
    ir/ir_app.c
    nfc/st25r3916.c
    nfc/nfc_app.c
    nfc/nfc_file.c
    rfid/rfid_125k.c
    rfid/rfid_app.c
    rfid/rfid_file.c
    badusb/usb_hid.c
    badusb/ducky_script.c
    badusb/badusb_app.c
)

add_executable(wili5tool.elf ${APP_SOURCES})

target_include_directories(wili5tool.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${wilibsp_SOURCE_DIR}/bsp
    ${wilibsp_SOURCE_DIR}/bsp/platform
    ${wilibsp_SOURCE_DIR}/bsp/display
    ${wilibsp_SOURCE_DIR}/bsp/input
    ${wilibsp_SOURCE_DIR}/bsp/keyboard
    ${wilibsp_SOURCE_DIR}/bsp/radio
    ${wilibsp_SOURCE_DIR}/bsp/ir
    ${wilibsp_SOURCE_DIR}/bsp/usbhost
    ${wilibsp_SOURCE_DIR}/bsp/third_party/fatfs
    nfc
    rfid
    badusb
    subghz
    ir
)

target_link_libraries(wili5tool.elf PRIVATE
    freewili2_bsp
    pico_stdlib
    hardware_gpio
    hardware_i2c
    hardware_spi
    hardware_uart
    hardware_pio
    hardware_pwm
    hardware_dma
    hardware_timer
    tinyusb_device
    tinyusb_board
)

fw2_display_app(wili5tool.elf
    POWER_ZONES DISPLAY
    VERSION 100
    DESCRIPTION "Wili5Tool Flipper-class multi-tool"
)

pico_add_extra_outputs(wili5tool.elf)