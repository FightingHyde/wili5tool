#include "badusb_app.h"
#include "usb_hid.h"
#include "ducky_script.h"
#include "ui.h"
#include "storage.h"
#include "input/uartkbd.h"
#include <stdio.h>
#include <string.h>

typedef enum {
    BU_IDLE, BU_FILES, BU_READY, BU_RUNNING, BU_DONE, BU_ERROR
} bu_state_t;

static bu_state_t state      = BU_IDLE;
static char       files[16][64];
static int        file_count = 0, file_sel = 0;
static char       status[80];
static bool       usb_inited = false;
static bool       dirty      = true;

void badusb_app_init(void) {
    state = BU_IDLE;
    if (!usb_inited) {
        usb_hid_init();
        usb_inited = true;
    }
    storage_list(STOR_BADUSB, files, &file_count, 16, ".txt");
    file_sel = 0;

    if (file_count == 0) {
        snprintf(status, sizeof(status),
                 "No scripts in w5tool/badusb/  Add .txt files");
    } else {
        state = BU_FILES;
        snprintf(status, sizeof(status),
                 "L/R=select  OK=run  BACK=menu");
    }
    dirty = true;
}

void badusb_app_exit(void) {
    /* Nothing to clean — USB HID stays active until reboot */
}

void badusb_app_handle_key(uint8_t key) {
    switch (key) {
    case FW2_KEY_LEFT:
        if (state==BU_FILES && file_count>0)
            file_sel = (file_sel + file_count - 1) % file_count;
        break;
    case FW2_KEY_RIGHT:
        if (state==BU_FILES && file_count>0)
            file_sel = (file_sel + 1) % file_count;
        break;
    case FW2_KEY_OK:
        if (state==BU_FILES && file_count>0) {
            state = BU_READY;
            snprintf(status, sizeof(status),
                     "Plug USB cable in, then OK to fire");
        } else if (state==BU_READY) {
            state = BU_RUNNING;
            snprintf(status, sizeof(status), "Running...");
            dirty = true;
            /* Draw first so user sees RUNNING */
            badusb_app_draw();

            char path[128];
            snprintf(path, sizeof(path), "%s/%s",
                     STOR_BADUSB, files[file_sel]);
            bool ok = ducky_run_file(path);
            state = ok ? BU_DONE : BU_ERROR;
            snprintf(status, sizeof(status),
                     ok ? "Done!  L/R=other  OK=run again"
                        : "Error: USB not ready or file bad");
        } else if (state==BU_DONE || state==BU_ERROR) {
            state = BU_FILES;
            snprintf(status, sizeof(status), "L/R=select  OK=run");
        }
        break;
    case FW2_KEY_B:
        if (state==BU_READY) {
            state = BU_FILES;
            snprintf(status, sizeof(status), "L/R=select  OK=run");
        }
        break;
    }
    dirty = true;
}

void badusb_app_update(void) {
    if (usb_inited) usb_hid_task();
}

void badusb_app_draw(void) {
    if (!dirty) return;
    ui_clear(UI_BG);
    ui_draw_header("BadUSB", "DuckyScript");

    if (file_count == 0) {
        ui_draw_string(10, 50, 1,
            "Upload .txt DuckyScript files to:", UI_WHITE, UI_BG);
        ui_draw_string(10, 70, 1,
            "SD:/w5tool/badusb/", UI_CYAN, UI_BG);
        ui_draw_string(10, 90, 1,
            "Then re-enter this menu.", UI_GRAY, UI_BG);
    } else {
        /* File list */
        ui_draw_string(10, 38, 1, "Select script:", UI_GRAY, UI_BG);
        for (int i = 0; i < file_count && i < 8; i++) {
            bool s = (i == file_sel);
            ui_fill_rect(10, 52+i*22, 460, 20, s ? UI_SELECTED : UI_BG);
            ui_draw_string(14, 56+i*22, 1, files[i],
                           s ? UI_ACCENT : UI_WHITE,
                           s ? UI_SELECTED : UI_BG);
        }

        /* State indicator */
        const char *ss=""; uint16_t sc=UI_GRAY;
        if (state==BU_READY)  { ss="READY — plug USB then OK"; sc=UI_YELLOW; }
        if (state==BU_RUNNING){ ss="RUNNING";                  sc=UI_RED;    }
        if (state==BU_DONE)   { ss="DONE";                     sc=UI_GREEN;  }
        if (state==BU_ERROR)  { ss="ERROR";                    sc=UI_RED;    }
        if (ss[0]) ui_draw_string(10, 240, 2, ss, sc, UI_BG);
    }

    ui_draw_status_bar(status, "BACK=menu");
    dirty = false;
}