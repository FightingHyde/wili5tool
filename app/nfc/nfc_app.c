#include "nfc_app.h"
#include "st25r3916.h"
#include "nfc_file.h"
#include "ui.h"
#include "storage.h"
#include "input/uartkbd.h"
#include <stdio.h>
#include <string.h>

typedef enum {
    NFC_IDLE, NFC_POLL, NFC_TAG_FOUND,
    NFC_READING, NFC_SAVED, NFC_FILES, NFC_EMULATE
} nfc_state_t;

static nfc_state_t state = NFC_IDLE;
static nfc_tag_t   current_tag;
static bool        chip_ok    = false;
static char        files[16][64];
static int         file_count = 0, file_sel = 0;
static char        status[80];
static char        uid_str[32];
static bool        dirty = true;

static void build_uid_str(const nfc_tag_t *t) {
    uid_str[0] = 0;
    for (int i = 0; i < t->uid_len; i++) {
        char tmp[4];
        snprintf(tmp, sizeof(tmp), i ? " %02X" : "%02X", t->uid[i]);
        strncat(uid_str, tmp, sizeof(uid_str)-strlen(uid_str)-1);
    }
}

void nfc_app_init(void) {
    state = NFC_IDLE;
    chip_ok = st25r3916_init();
    if (!chip_ok)
        snprintf(status, sizeof(status), "ST25R3916 not found!");
    else
        snprintf(status, sizeof(status), "OK=scan  B=saved files");
    dirty = true;
}

void nfc_app_exit(void) {
    st25r3916_emulate_stop();
    st25r3916_set_mode_idle();
}

void nfc_app_handle_key(uint8_t key) {
    switch (key) {
    case FW2_KEY_OK:
        if (!chip_ok) break;
        if (state == NFC_IDLE) {
            memset(&current_tag, 0, sizeof(current_tag));
            state = NFC_POLL;
            snprintf(status, sizeof(status), "Polling... hold card close");
        } else if (state == NFC_TAG_FOUND || state == NFC_READING) {
            /* Save to file */
            char path[128];
            storage_unique_name(STOR_NFC, "tag", ".nfc", path, sizeof(path));
            nfc_file_save(path, &current_tag);
            snprintf(status, sizeof(status), "Saved: %s", uid_str);
            state = NFC_SAVED;
            st25r3916_set_mode_idle();
        } else if (state == NFC_SAVED) {
            state = NFC_IDLE;
            snprintf(status, sizeof(status), "OK=scan  B=files");
        } else if (state == NFC_FILES && file_count > 0) {
            /* Load and emulate */
            char path[128];
            snprintf(path, sizeof(path), "%s/%s", STOR_NFC, files[file_sel]);
            nfc_tag_t loaded;
            if (nfc_file_load(path, &loaded)) {
                current_tag = loaded;
                build_uid_str(&current_tag);
                st25r3916_emulate_uid(current_tag.uid, current_tag.uid_len);
                state = NFC_EMULATE;
                snprintf(status, sizeof(status),
                         "Emulating UID: %s  B=stop", uid_str);
            }
        } else if (state == NFC_EMULATE) {
            st25r3916_emulate_stop();
            state = NFC_IDLE;
            snprintf(status, sizeof(status), "OK=scan  B=files");
        }
        break;

    case FW2_KEY_LEFT:
        if (state == NFC_FILES && file_count > 0)
            file_sel = (file_sel + file_count - 1) % file_count;
        break;
    case FW2_KEY_RIGHT:
        if (state == NFC_FILES && file_count > 0)
            file_sel = (file_sel + 1) % file_count;
        break;

    case FW2_KEY_B:
        if (state == NFC_EMULATE) {
            st25r3916_emulate_stop();
            state = NFC_IDLE;
            snprintf(status, sizeof(status), "OK=scan  B=files");
        } else if (state == NFC_IDLE || state == NFC_SAVED) {
            storage_list(STOR_NFC, files, &file_count, 16, ".nfc");
            file_sel = 0;
            state = NFC_FILES;
            snprintf(status, sizeof(status), "L/R=select  OK=emulate  B=back");
        } else if (state == NFC_FILES) {
            state = NFC_IDLE;
            snprintf(status, sizeof(status), "OK=scan  B=files");
        } else if (state == NFC_POLL) {
            st25r3916_set_mode_idle();
            state = NFC_IDLE;
            snprintf(status, sizeof(status), "Cancelled");
        }
        break;
    }
    dirty = true;
}

void nfc_app_update(void) {
    if (state == NFC_POLL) {
        if (st25r3916_iso14443a_poll(&current_tag)) {
            build_uid_str(&current_tag);
            state = NFC_READING;
            snprintf(status, sizeof(status), "Tag: %s  Reading...",
                     current_tag.type_str);
            dirty = true;
            /* Auto-read Ultralight data */
            if (current_tag.type == NFC_TAG_ULTRALIGHT)
                nfc_mifare_ultralight_read(&current_tag);
            state = NFC_TAG_FOUND;
            snprintf(status, sizeof(status),
                     "Found %s  OK=save  B=files", current_tag.type_str);
        }
    }
}

void nfc_app_draw(void) {
    