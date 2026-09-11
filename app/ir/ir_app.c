#include "ir_app.h"
#include "ui.h"
#include "storage.h"
#include "ir/ir_capture.h"
#include "ir/ir_tx.h"
#include "ir/ir_frame.h"
#include "ir/ir_decode.h"
#include "ir/ir_encode.h"
#include "ir/ir_file.h"
#include "ir/ir_types.h"
#include "input/uartkbd.h"
#include <stdio.h>
#include <string.h>

typedef enum { IR_IDLE,IR_SCAN,IR_SAVED,IR_FILES,IR_REPLAY } ir_state_t;

static ir_state_t  state   = IR_IDLE;
static ir_message_t last_msg;
static bool         has_msg = false;
static char  files[16][64];
static int   file_count=0, file_sel=0;
static char  status[80];
static char  decoded[128];
static bool  dirty=true;

void ir_app_init(void) {
    state=IR_IDLE; has_msg=false;
    ir_capture_init();
    snprintf(status,sizeof(status),"OK=scan  B=saved files");
    dirty=true;
}

void ir_app_exit(void) {
    ir_capture_stop();
    ir_tx_stop();
}

void ir_app_handle_key(uint8_t key) {
    switch(key) {
    case FW2_KEY_OK:
        if(state==IR_IDLE) {
            ir_capture_start();
            state=IR_SCAN;
            snprintf(status,sizeof(status),"Scanning...  OK=stop");
        } else if(state==IR_SCAN) {
            ir_capture_stop();
            if(has_msg) {
                char path[128];
                storage_unique_name(STOR_IR,"signal",".ir",path,sizeof(path));
                ir_file_t irf={0};
                irf.entries[0].msg=last_msg;
                snprintf(irf.entries[0].name,sizeof(irf.entries[0].name),
                         "captured");
                irf.entry_count=1;
                ir_file_save(path,&irf);
                snprintf(status,sizeof(status),"Saved: %s",decoded);
                state=IR_SAVED;
            } else {
                snprintf(status,sizeof(status),"Nothing received");
                state=IR_IDLE;
            }
        } else if(state==IR_SAVED) {
            state=IR_IDLE;
            snprintf(status,sizeof(status),"OK=scan  B=files");
        } else if(state==IR_FILES && file_count>0) {
            char path[128];
            snprintf(path,sizeof(path),"%s/%s",STOR_IR,files[file_sel]);
            ir_file_t irf={0};
            if(ir_file_load(path,&irf) && irf.entry_count>0) {
                ir_tx_send_message(&irf.entries[0].msg);
                state=IR_REPLAY;
                snprintf(status,sizeof(status),"Sending %s",files[file_sel]);
            }
        }
        break;
    case FW2_KEY_LEFT:
        if(state==IR_FILES&&file_count>0)
            file_sel=(file_sel+file_count-1)%file_count;
        break;
    case FW2_KEY_RIGHT:
        if(state==IR_FILES&&file_count>0)
            file_sel=(file_sel+1)%file_count;
        break;
    case FW2_KEY_B:
        if(state==IR_IDLE||state==IR_SAVED) {
            storage_list(STOR_IR,files,&file_count,16,".ir");
            file_sel=0; state=IR_FILES;
            snprintf(status,sizeof(status),"L/R=select  OK=replay  B=back");
        } else if(state==IR_FILES) {
            state=IR_IDLE;
            snprintf(status,sizeof(status),"OK=scan  B=files");
        }
        break;
    }
    dirty=true;
}

void ir_app_update(void) {
    if(state==IR_SCAN) {
        ir_frame_t frame;
        if(ir_capture_get_frame(&frame)) {
            ir_message_t msg;
            if(ir_decode(frame.durs,frame.count,&msg)) {
                last_msg=msg; has_msg=true;
                snprintf(decoded,sizeof(decoded),
                         "%s A:%08lX C:%08lX",
                         ir_protocol_name(msg.protocol),
                         (unsigned long)msg.address,
                         (unsigned long)msg.command);
            }
        }
    }
    if(state==IR_REPLAY && !ir_tx_busy()) {
        state=IR_IDLE;
        snprintf(status,sizeof(status),"Sent!  OK=scan  B=files");
        dirty=true;
    }
}

void ir_app_draw(void) {
    if(!dirty) return;
    ui_clear(UI_BG);
    ui_draw_header("Infrared","NEC/SIRC/RC5");

    if(state==IR_FILES) {
        ui_draw_string(10,40,1,"Saved IR signals:",UI_WHITE,UI_BG);
        if(!file_count)
            ui_draw_string(10,60,1,"No .ir files",UI_GRAY,UI_BG);
        else for(int i=0;i<file_count;i++) {
            bool s=(i==file_sel);
            ui_fill_rect(10,58+i*22,460,20,s?UI_SELECTED:UI_BG);
            ui_draw_string(14,62+i*22,1,files[i],
                           s?UI_ACCENT:UI_WHITE,s?UI_SELECTED:UI_BG);
        }
    } else {
        const char *ss="IDLE"; uint16_t sc=UI_GRAY;
        if(state==IR_SCAN)   {ss="SCANNING"; sc=UI_YELLOW;}
        if(state==IR_SAVED)  {ss="SAVED";    sc=UI_GREEN;}
        if(state==IR_REPLAY) {ss="SENDING";  sc=UI_CYAN;}
        ui_draw_string(10,45,2,ss,sc,UI_BG);
        if(has_msg) ui_draw_string(10,85,1,decoded,UI_WHITE,UI_BG);
        else ui_draw_string(10,85,1,"Point IR source at device",UI_GRAY,UI_BG);
    }
    ui_draw_status_bar(status,"BACK=menu");
    dirty=false;
}