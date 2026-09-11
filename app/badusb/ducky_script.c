#include "ducky_script.h"
#include "usb_hid.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool ducky_run_line(const char *line) {
    if (!line || line[0] == '#' || line[0] == 0) return true;

    /* REM — comment */
    if (strncmp(line, "REM", 3) == 0) return true;

    /* DELAY <ms> */
    if (strncmp(line, "DELAY ", 6) == 0) {
        usb_hid_delay_ms((uint32_t)atoi(line + 6));
        return true;
    }

    /* STRING <text> */
    if (strncmp(line, "STRING ", 7) == 0) {
        usb_hid_type_string(line + 7);
        return true;
    }

    /* Single-word keys */
    const char *singles[] = {
        "ENTER","SPACE","TAB","BACKSPACE","DELETE","ESCAPE",
        "UP","DOWN","LEFT","RIGHT",
        "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
        NULL
    };
    for (int i = 0; singles[i]; i++) {
        if (strcmp(line, singles[i]) == 0) {
            usb_hid_type_key_name(line);
            return true;
        }
    }

    /* Modifier combos */
    uint8_t     mod  = 0;
    const char *rest = NULL;

    if      (strncmp(line,"CTRL-ALT ",9)==0)
        { mod=KEYBOARD_MODIFIER_LEFTCTRL|KEYBOARD_MODIFIER_LEFTALT; rest=line+9; }
    else if (strncmp(line,"CTRL-SHIFT ",11)==0)
        { mod=KEYBOARD_MODIFIER_LEFTCTRL|KEYBOARD_MODIFIER_LEFTSHIFT; rest=line+11; }
    else if (strncmp(line,"ALT-SHIFT ",10)==0)
        { mod=KEYBOARD_MODIFIER_LEFTALT|KEYBOARD_MODIFIER_LEFTSHIFT; rest=line+10; }
    else if (strncmp(line,"CTRL ",5)==0)
        { mod=KEYBOARD_MODIFIER_LEFTCTRL;  rest=line+5; }
    else if (strncmp(line,"ALT ",4)==0)
        { mod=KEYBOARD_MODIFIER_LEFTALT;   rest=line+4; }
    else if (strncmp(line,"GUI ",4)==0)
        { mod=KEYBOARD_MODIFIER_LEFTGUI;   rest=line+4; }
    else if (strncmp(line,"SHIFT ",6)==0)
        { mod=KEYBOARD_MODIFIER_LEFTSHIFT; rest=line+6; }
    else if (strncmp(line,"WINDOWS ",8)==0)
        { mod=KEYBOARD_MODIFIER_LEFTGUI;   rest=line+8; }

    if (mod && rest) {
        /* rest is a single key name or character */
        uint8_t keycode = 0;
        if      (strcmp(rest,"ENTER")==0)  keycode=HID_KEY_ENTER;
        else if (strcmp(rest,"TAB")==0)    keycode=HID_KEY_TAB;
        else if (strcmp(rest,"SPACE")==0)  keycode=HID_KEY_SPACE;
        else if (strcmp(rest,"F1")==0)     keycode=HID_KEY_F1;
        else if (strcmp(rest,"F2")==0)     keycode=HID_KEY_F2;
        else if (strcmp(rest,"F3")==0)     keycode=HID_KEY_F3;
        else if (strcmp(rest,"F4")==0)     keycode=HID_KEY_F4;
        else if (strcmp(rest,"F10")==0)    keycode=HID_KEY_F10;
        else if (strcmp(rest,"F11")==0)    keycode=HID_KEY_F11;
        else if (strcmp(rest,"F12")==0)    keycode=HID_KEY_F12;
        else if (strcmp(rest,"DELETE")==0) keycode=HID_KEY_DELETE;
        else if (strcmp(rest,"ESCAPE")==0) keycode=HID_KEY_ESCAPE;
        else if (strlen(rest)==1) {
            char c = rest[0];
            if (c>='a'&&c<='z') keycode = HID_KEY_A+(c-'a');
            else if (c>='A'&&c<='Z') keycode = HID_KEY_A+(c-'A');
        }
        if (keycode) usb_hid_press_key(mod, keycode);
        return true;
    }

    /* Unrecognised — ignore */
    return true;
}

bool ducky_run_file(const char *path) {
    char *buf = malloc(16384);
    if (!buf) return false;
    int n = storage_read(path, (uint8_t*)buf, 16383);
    if (n <= 0) { free(buf); return false; }
    buf[n] = 0;

    /* Wait for USB host to enumerate */
    for (int i = 0; i < 50 && !usb_hid_ready(); i++) {
        usb_hid_task();
        usb_hid_delay_ms(100);
    }
    if (!usb_hid_ready()) { free(buf); return false; }

    char *line = strtok(buf, "\r\n");
    while (line) {
        /* Strip trailing whitespace */
        int len = (int)strlen(line);
        while (len>0 && (line[len-1]==' '||line[len-1]=='\t')) {
            line[--len]=0;
        }
        ducky_run_line(line);
        usb_hid_task();
        line = strtok(NULL, "\r\n");
    }

    free(buf);
    return true;
}