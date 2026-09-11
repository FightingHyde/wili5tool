#include "subghz_app.h"
#include "sub_file.h"
#include "ui.h"
#include "storage.h"
#include "radio/cc1101.h"
#include "radio/monitor_engine.h"
#include "radio/capture_store.h"
#include "radio/ook_tx.h"
#include "input/uartkbd.h"
#include <stdio.h>
#include <string.h>

typedef enum {
    SG_SCAN, SG_RECEIVE, SG_SAVED, SG_FILES, SG_REPLAY
} sg_state_t;

static const struct { uint32_t hz; const char *name; } FREQS[] = {
    {315000000,"315.00 MHz"},{433920000,"433.92 MHz"},
    {868350000,"868.35 MHz"},{915000000,"915.00 MHz"},
};
#define FREQ_COUNT 4

static sg_state_t state;
static int        freq_idx = 1;
static int        rssi     = -120;
static bool       capturing = false, replaying = false;
static sub_file_t cap, replay_buf;
static char       files[16][64];
static int        file_count = 0, file_sel = 0;
static char       status[80];
static bool       dirty = true;

void subghz_app_init(void) {
    state = SG_SCAN; capturing = false; replaying = false;
    freq_idx = 1;
    cc1101_init();
    cc1101_set_frequency(FREQS[freq_idx].hz);
    cc1101_monitor_start();
    snprintf(status, sizeof(status), "UP/DN=freq  OK=capture  B=files");
    dirty = true;
}

void subghz_app_exit(void) {
    cc1101_monitor_stop();
    if (capturing)  { capture_store_stop(); capturing = false; }
    if (replaying)  { ook_tx_stop();        replaying = false; }
}

void subghz_app_handle_key(uint8_t key) {
    switch (key) {
    case FW2_KEY_UP:
        if (!capturing) {
            freq_idx = (freq_idx + FREQ_COUNT - 1) % FREQ_COUNT;
            cc1101_set_frequency(FREQS[freq_idx].hz);
        }
        break;
    case FW2_KEY_DOWN:
        if (!capturing) {
            freq_idx = (freq_idx + 1) % FREQ_COUNT;
            cc1101_set_frequency(FREQS[freq_idx].hz);
        }
        break;
    case FW2_KEY_OK:
        if (state == SG_SCAN) {
            memset(&cap, 0, sizeof(cap));
            cap.frequency = FREQS[freq_idx].hz;
            snprintf(cap.preset, sizeof(cap.preset),
                     "FuriHalSubGhzPresetOok650Async");
            capture_store_start();
            capturing = true;
            state = SG_RECEIVE;
            snprintf(status, sizeof(status), "Capturing...  OK=stop");
        } else if (state == SG_RECEIVE) {
            capture_store_stop();
            capturing = false;
            cap.sample_count = capture_store_get(cap.samples, SUB_MAX_SAMPLES);
            if (cap.sample_count > 0) {
                char path[128];
                storage_unique_name(STOR_SUBGHZ,"signal",".sub",path,sizeof(path));
                sub_file_save(path, &cap);
                snprintf(status, sizeof(status),
                         "Saved %d samples", cap.sample_count);
                state = SG_SAVED;
            } else {
                snprintf(status, sizeof(status), "Nothing captured");
                state = SG_SCAN;
                cc1101_monitor_start();
            }
        } else if (state == SG_SAVED) {
            state = SG_SCAN;
            cc1101_monitor_start();
            snprintf(status, sizeof(status), "UP/DN=freq  OK=capture  B=files");
        } else if (state == SG_FILES && file_count > 0) {
            char path[128];
            snprintf(path, sizeof(path), "%s/%s", STOR_SUBGHZ, files[file_sel]);
            if (sub_file_load(path, &replay_buf)) {
                cc1101_set_frequency(replay_buf.frequency);
                ook_tx_start(replay_buf.samples, replay_buf.sample_count);
                replaying = true;
                state = SG_REPLAY;
                snprintf(status, sizeof(status), "Transmitting...");
            }
        }
        break;
    case FW2_KEY_LEFT:
        if (state==SG_FILES && file_count>0)
            file_sel = (file_sel + file_count - 1) % file_count;
        break;
    case FW2_KEY_RIGHT:
        if (state==SG_FILES && file_count>0)
            file_sel = (file_sel + 1) % file_count;
        break;
    case FW2_KEY_B:
        if (state==SG_SCAN || state==SG_SAVED) {
            storage_list(STOR_SUBGHZ, files, &file_count, 16, ".sub");
            file_sel = 0;
            state = SG_FILES;
            snprintf(status,sizeof(status),"L/R=select  OK=replay  B=back");
        } else if (state==SG_FILES) {
            state = SG_SCAN;
            cc1101_monitor_start();
            snprintf(status,sizeof(status),"UP/DN=freq  OK=capture  B=files");
        }
        break;
    }
    dirty = true;
}

void subghz_app_update(void) {
    rssi = cc1101_get_rssi();
    if (replaying && !ook_tx_busy()) {
        replaying = false;
        state = SG_SCAN;
        cc1101_monitor_start();
        snprintf(status,sizeof(status),"Done!  OK=capture  B=files");
        dirty = true;
    }
}

void subghz_app_draw(void) {
    if (!dirty) return;
    ui_clear(UI_BG);
    ui_draw_header("Sub-GHz", "CC1101");

    if (state == SG_FILES) {
        ui_draw_string(10,40,1,"Saved captures:",UI_WHITE,UI_BG);
        if (!file_count) {
            ui_draw_string(10,60,1,"No .sub files",UI_GRAY,UI_BG);
        } else {
            for (int i=0;i<file_count;i++) {
                bool s=(i==file_sel);
                ui_fill_rect(10,58+i*22,460,20,s?UI_SELECTED:UI_BG);
                ui_draw_string(14,62+i*22,1,files[i],
                               s?UI_ACCENT:UI_WHITE,s?UI_SELECTED:UI_BG);
            }
        }
    } else {
        for (int i=0;i<FREQ_COUNT;i++) {
            bool s=(i==freq_idx);
            ui_fill_rect(10,42+i*30,200,26,s?UI_SELECTED:UI_DARK_GRAY);
            ui_draw_rect(10,42+i*30,200,26,s?UI_ACCENT:UI_GRAY);
            ui_draw_string(16,49+i*30,1,FREQS[i].name,
                           s?UI_WHITE:UI_GRAY,s?UI_SELECTED:UI_DARK_GRAY);
        }
        char rbuf[32];
        snprintf(rbuf,sizeof(rbuf),"RSSI: %d dBm",rssi);
        ui_draw_string(230,42,1,rbuf,UI_CYAN,UI_BG);
        int rn = rssi+120; if(rn<0)rn=0; if(rn>120)rn=120;
        ui_draw_progress_bar(230,56,220,18,rn,120,rssi>-70?UI_GREEN:UI_ORANGE);

        const char *ss="IDLE"; uint16_t sc=UI_GRAY;
        if(state==SG_RECEIVE){ss="CAPTURING";sc=UI_RED;}
        if(state==SG_SAVED)  {ss="SAVED";    sc=UI_GREEN;}
        if(state==SG_REPLAY) {ss="REPLAYING";sc=UI_CYAN;}
        ui_draw_string(230,82,2,ss,sc,UI_BG);
    }

    ui_draw_status_bar(status,"BACK=menu");
    dirty = false;
}