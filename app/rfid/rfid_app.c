#include "rfid_app.h"
#include "rfid_125k.h"
#include "rfid_file.h"
#include "ui.h"
#include "storage.h"
#include "input/uartkbd.h"
#include <stdio.h>
#include <string.h>

typedef enum { RFID_IDLE,RFID_SCAN,RFID_FOUND,RFID_SAVED,
               RFID_FILES,RFID_EMULATE } rfid_state_t;

static rfid_state_t state      = RFID_IDLE;
static rfid_tag_t   current_tag;
static bool         reading    = false;
static char         files[16][64];
static int          file_count = 0, file_sel = 0;
static char         status[80];
static bool         dirty      = true;

void rfid_app_init(void) {
    state = RFID_IDLE;
    rfid_125k_init();
    snprintf(status, sizeof(status), "OK=scan  B=saved files");
    dirty = true;
}

void rfid_app_exit(void) {
    rfid_125k_stop();
}

void rfid_app_handle_key(uint8_t key) {
    switch (key) {
    case FW2_KEY_OK:
        if (state == RFID_IDLE) {
            memset(&current_tag, 0, sizeof(current_tag));
            state = RFID_SCAN;
            reading = true;
            snprintf(status, sizeof(status), "Scanning 125kHz...  B=cancel");
        } else if (state == RFID_FOUND) {
            char path[128];
            storage_unique_name(STOR_RFID, "tag", ".rfid", path, sizeof(path));
            rfid_file_save(path, &current_tag);
            snprintf(status, sizeof(status), "Saved %s", current_tag.proto_str);
            state = RFID_SAVED;
        } else if (state == RFID_SAVED) {
            state = RFID_IDLE;
            snprintf(status, sizeof(status), "OK=scan  B=files");
        } else if (state == RFID_FILES && file_count > 0) {
            char path[128];
            snprintf(path, sizeof(path), "%s/%s", STOR_RFID, files[file_sel]);
            rfid_tag_t loaded;
            if (rfid_file_load(path, &loaded)) {
                current_tag = loaded;
                state = RFID_EMULATE;
                snprintf(status, sizeof(status),
                         "Emulating %s  OK=stop", loaded.proto_str);
            }
        } else if (state == RFID_EMULATE) {
            rfid_125k_stop();
            state = RFID_IDLE;
            snprintf(status, sizeof(status), "OK=scan  B=files");
        }
        break;

    case FW2_KEY_LEFT:
        if (state==RFID_FILES&&file_count>0)
            file_sel=(file_sel+file_count-1)%file_count;
        break;
    case FW2_KEY_RIGHT:
        if (state==RFID_FILES&&file_count>0)
            file_sel=(file_sel+1)%file_count;
        break;

    case FW2_KEY_B:
        rfid_125k_stop(); reading=false;
        if (state==RFID_IDLE||state==RFID_SAVED) {
            storage_list(STOR_RFID,files,&file_count,16,".rfid");
            file_sel=0; state=RFID_FILES;
            snprintf(status,sizeof(status),"L/R=select  OK=emulate  B=back");
        } else if (state==RFID_FILES) {
            state=RFID_IDLE;
            snprintf(status,sizeof(status),"OK=scan  B=files");
        } else {
            state=RFID_IDLE;
            snprintf(status,sizeof(status),"Cancelled");
        }
        break;
    }
    dirty=true;
}

void rfid_app_update(void) {
    if (state == RFID_SCAN && reading) {
        /* Non-blocking: try a quick 500ms read per update cycle */
        if (rfid_125k_read(&current_tag, 500)) {
            reading = false;
            state = RFID_FOUND;
            snprintf(status, sizeof(status),
                     "Found %s  FC:%lu  ID:%lu  OK=save",
                     current_tag.proto_str,
                     (unsigned long)current_tag.facility,
                     (unsigned long)current_tag.card_id);
            dirty = true;
        }
    }
    if (state == RFID_EMULATE) {
        /* Continuously replay the tag (blocking ~64 bit periods = ~33ms) */
        rfid_125k_emulate(&current_tag);
    }
}

void rfid_app_draw(void) {
    if (!dirty) return;
    ui_clear(UI_BG);
    ui_draw_header("RFID 125kHz", "EM4100/HID");

    if (state == RFID_FILES) {
        ui_draw_string(10,40,1,"Saved RFID tags:",UI_WHITE,UI_BG);
        if (!file_count)
            ui_draw_string(10,60,1,"No .rfid files",UI_GRAY,UI_BG);
        else for (int i=0;i<file_count;i++) {
            bool s=(i==file_sel);
            ui_fill_rect(10,58+i*22,460,20,s?UI_SELECTED:UI_BG);
            ui_draw_string(14,62+i*22,1,files[i],
                           s?UI_ACCENT:UI_WHITE,s?UI_SELECTED:UI_BG);
        }
    } else {
        const char *ss="IDLE"; uint16_t sc=UI_GRAY;
        if(state==RFID_SCAN)   {ss="SCANNING";  sc=UI_YELLOW;}
        if(state==RFID_FOUND)  {ss="TAG FOUND"; sc=UI_GREEN;}
        if(state==RFID_SAVED)  {ss="SAVED";     sc=UI_GREEN;}
        if(state==RFID_EMULATE){ss="EMULATING"; sc=UI_ORANGE;}
        ui_draw_string(10,45,2,ss,sc,UI_BG);

        if(state==RFID_FOUND||state==RFID_SAVED||state==RFID_EMULATE) {
            char tmp[64];
            snprintf(tmp,sizeof(tmp),"Protocol: %s",current_tag.proto_str);
            ui_draw_string(10,85,1,tmp,UI_WHITE,UI_BG);
            snprintf(tmp,sizeof(tmp),"Facility: %lu",
                     (unsigned long)current_tag.facility);
            ui_draw_string(10,100,1,tmp,UI_CYAN,UI_BG);
            snprintf(tmp,sizeof(tmp),"Card ID:  %lu",
                     (unsigned long)current_tag.card_id);
            ui_draw_string(10,115,1,tmp,UI_CYAN,UI_BG);
            ui_draw_hex_dump(10,135,current_tag.data,
                             current_tag.data_len,UI_WHITE,UI_BG);
        } else if(state==RFID_IDLE) {
            ui_draw_string(10,80,1,
                "Hold RFID card close to rear of device",UI_GRAY,UI_BG);
        }
    }
    ui_draw_status_bar(status,"BACK=menu");
    dirty=false;
}