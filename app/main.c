/**
 * Wili5Tool — Flipper-class multi-tool for FREE-WILi 2
 * Modules: Sub-GHz | IR | NFC | RFID 125kHz | BadUSB
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

/* BSP */
#include "board.h"
#include "display/st7796.h"
#include "input/uartkbd.h"
#include "input/ft6336.h"
#include "platform/ioexp.h"

/* App */
#include "ui.h"
#include "storage.h"
#include "subghz/subghz_app.h"
#include "ir/ir_app.h"
#include "nfc/nfc_app.h"
#include "rfid/rfid_app.h"
#include "badusb/badusb_app.h"

/* ── App modes ──────────────────────────────────────────────────────────── */

typedef enum {
    MODE_MENU = 0,
    MODE_SUBGHZ,
    MODE_IR,
    MODE_NFC,
    MODE_RFID,
    MODE_BADUSB,
} app_mode_t;

static app_mode_t current_mode  = MODE_MENU;
static int        menu_sel      = 0;
static bool       menu_dirty    = true;

#define MENU_COUNT  5

static const struct {
    const char *label;
    const char *desc;
    uint16_t    color;
} MENU_ITEMS[MENU_COUNT] = {
    { "Sub-GHz",   "CC1101  315/433/868/915 MHz",  UI_ORANGE },
    { "Infrared",  "NEC / SIRC / RC5 / Raw",        UI_YELLOW },
    { "NFC",       "ISO14443A  MIFARE  ST25R3916B", UI_CYAN   },
    { "RFID 125k", "EM4100 / HID26  125 kHz",       UI_GREEN  },
    { "BadUSB",    "DuckyScript HID keyboard",       UI_RED    },
};

/* ── Exit current module, return to menu ───────────────────────────────── */

static void exit_current_module(void) {
    switch (current_mode) {
    case MODE_SUBGHZ: subghz_app_exit(); break;
    case MODE_IR:     ir_app_exit();     break;
    case MODE_NFC:    nfc_app_exit();    break;
    case MODE_RFID:   rfid_app_exit();   break;
    case MODE_BADUSB: badusb_app_exit(); break;
    default: break;
    }
    current_mode = MODE_MENU;
    menu_dirty   = true;
}

/* ── Enter selected module ──────────────────────────────────────────────── */

static void enter_module(int idx) {
    switch (idx) {
    case 0: current_mode=MODE_SUBGHZ; subghz_app_init(); break;
    case 1: current_mode=MODE_IR;     ir_app_init();     break;
    case 2: current_mode=MODE_NFC;    nfc_app_init();    break;
    case 3: current_mode=MODE_RFID;   rfid_app_init();   break;
    case 4: current_mode=MODE_BADUSB; badusb_app_init(); break;
    }
}

/* ── Menu draw ──────────────────────────────────────────────────────────── */

static void draw_menu(void) {
    if (!menu_dirty) return;
    ui_clear(UI_BG);

    /* Title */
    ui_fill_rect(0, 0, SCREEN_W, 36, UI_ACCENT);
    ui_draw_string(12, 4,  2, "Wili5Tool", UI_BLACK, UI_ACCENT);
    ui_draw_string(12, 20, 1, "FREE-WILi 2 Multi-Tool", UI_BLACK, UI_ACCENT);
    ui_draw_hline(0, 36, SCREEN_W, UI_WHITE);

    /* Menu items */
    for (int i = 0; i < MENU_COUNT; i++) {
        bool sel = (i == menu_sel);
        int  y   = 44 + i * 48;

        ui_fill_rect(8, y, SCREEN_W-16, 44, sel ? UI_SELECTED : UI_DARK_GRAY);
        ui_draw_rect(8, y, SCREEN_W-16, 44, sel ? MENU_ITEMS[i].color : UI_GRAY);

        /* Colored left bar */
        ui_fill_rect(8, y, 6, 44, MENU_ITEMS[i].color);

        /* Label + description */
        ui_draw_string(22, y+6,  2, MENU_ITEMS[i].label,
                       sel ? MENU_ITEMS[i].color : UI_WHITE,
                       sel ? UI_SELECTED : UI_DARK_GRAY);
        ui_draw_string(22, y+26, 1, MENU_ITEMS[i].desc,
                       sel ? UI_WHITE : UI_GRAY,
                       sel ? UI_SELECTED : UI_DARK_GRAY);
    }

    ui_draw_status_bar("UP/DN=select  OK=enter", "v1.0");
    menu_dirty = false;
}

/* ── Menu key handler ───────────────────────────────────────────────────── */

static void handle_menu_key(uint8_t key) {
    switch (key) {
    case FW2_KEY_UP:
        menu_sel = (menu_sel + MENU_COUNT - 1) % MENU_COUNT;
        menu_dirty = true;
        break;
    case FW2_KEY_DOWN:
        menu_sel = (menu_sel + 1) % MENU_COUNT;
        menu_dirty = true;
        break;
    case FW2_KEY_OK:
    case FW2_KEY_RIGHT:
        enter_module(menu_sel);
        break;
    }
}

/* ── Hardware init ──────────────────────────────────────────────────────── */

static void hw_init(void) {
    stdio_init_all();
    board_init();

    /* I2C1 for NFC (SDA=26, SCL=27) and sensors */
    i2c_init(i2c1, 400000);
    gpio_set_function(26, GPIO_FUNC_I2C);
    gpio_set_function(27, GPIO_FUNC_I2C);
    gpio_pull_up(26);
    gpio_pull_up(27);

    /* IO expander (needed for IR power gate, USB power, etc.) */
    ioexp_init();

    /* Enable IR power rail */
    ioexp_ir_pwr(true);

    /* Display */
    ui_init();

    /* Touch */
    ft6336_init();

    /* 14-button UART keyboard (UART1 GPIO38/39 @ 62500 8N1) */
    uartkbd_init();

    /* Storage (SD via OneWili / FatFs) */
    storage_init();
}

/* ── Main ───────────────────────────────────────────────────────────────── */

int main(void) {
    hw_init();

    /* Splash */
    ui_clear(UI_BG);
    ui_fill_rect(0, 0, SCREEN_W, 36, UI_ACCENT);
    ui_draw_string(12, 8, 2, "Wili5Tool", UI_BLACK, UI_ACCENT);
    ui_draw_string(60, 120, 3, "Wili5Tool", UI_ORANGE, UI_BG);
    ui_draw_string(80, 165, 1, "Flipper-class tool for FREE-WILi 2", UI_WHITE, UI_BG);
    ui_draw_string(170, 190, 1, "Loading...", UI_GRAY, UI_BG);
    sleep_ms(1800);

    while (true) {
        /* Poll buttons */
        uartkbd_poll();
        uint8_t key = uartkbd_get_key();

        /* BACK always returns to menu from any module */
        if (key == FW2_KEY_BACK && current_mode != MODE_MENU) {
            exit_current_module();
            key = 0;
        }

        switch (current_mode) {
        case MODE_MENU:
            if (key) handle_menu_key(key);
            draw_menu();
            break;

        case MODE_SUBGHZ:
            if (key) subghz_app_handle_key(key);
            subghz_app_update();
            subghz_app_draw();
            break;

        case MODE_IR:
            if (key) ir_app_handle_key(key);
            ir_app_update();
            ir_app_draw();
            break;

        case MODE_NFC:
            if (key) nfc_app_handle_key(key);
            nfc_app_update();
            nfc_app_draw();
            break;

        case MODE_RFID:
            if (key) rfid_app_handle_key(key);
            rfid_app_update();
            rfid_app_draw();
            break;

        case MODE_BADUSB:
            if (key) badusb_app_handle_key(key);
            badusb_app_update();
            badusb_app_draw();
            break;
        }

        sleep_ms(16); /* ~60 fps */
    }
}