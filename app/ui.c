#include "ui.h"
#include "display/st7796.h"
#include <stdio.h>
#include <string.h>

void ui_init(void)        { st7796_init(); ui_clear(UI_BG); }
void ui_clear(uint16_t c) { st7796_fill(c); }

void ui_fill_rect(int x, int y, int w, int h, uint16_t c) {
    st7796_fill_rect(x, y, w, h, c);
}
void ui_draw_hline(int x, int y, int w, uint16_t c) {
    st7796_fill_rect(x, y, w, 1, c);
}
void ui_draw_rect(int x, int y, int w, int h, uint16_t c) {
    st7796_fill_rect(x, y, w, 1, c);
    st7796_fill_rect(x, y+h-1, w, 1, c);
    st7796_fill_rect(x, y, 1, h, c);
    st7796_fill_rect(x+w-1, y, 1, h, c);
}
void ui_draw_string(int x, int y, int sc, const char *s,
                    uint16_t fg, uint16_t bg) {
    st7796_draw_string(x, y, sc, s, fg, bg);
}

void ui_draw_header(const char *title, const char *sub) {
    ui_fill_rect(0, 0, SCREEN_W, 32, UI_ACCENT);
    ui_draw_string(8, 8, 2, title, UI_BLACK, UI_ACCENT);
    if (sub && sub[0])
        ui_draw_string(SCREEN_W - (int)(strlen(sub)*6+8), 10, 1,
                       sub, UI_BLACK, UI_ACCENT);
    ui_draw_hline(0, 32, SCREEN_W, UI_WHITE);
}

void ui_draw_status_bar(const char *left, const char *right) {
    int y = SCREEN_H - 20;
    ui_fill_rect(0, y, SCREEN_W, 20, UI_DARK_GRAY);
    ui_draw_hline(0, y, SCREEN_W, UI_GRAY);
    if (left)  ui_draw_string(4, y+5, 1, left,  UI_WHITE, UI_DARK_GRAY);
    if (right) ui_draw_string(SCREEN_W-(int)(strlen(right)*6+6),
                               y+5, 1, right, UI_GRAY, UI_DARK_GRAY);
}

void ui_draw_button(int x, int y, int w, int h, const char *label,
                    bool sel, uint16_t fg, uint16_t bg) {
    uint16_t fill = sel ? UI_SELECTED : bg;
    uint16_t bord = sel ? UI_ACCENT   : UI_GRAY;
    ui_fill_rect(x, y, w, h, fill);
    ui_draw_rect(x, y, w, h, bord);
    int tx = x + (w - (int)(strlen(label)*6)) / 2;
    int ty = y + (h - 7) / 2;
    ui_draw_string(tx, ty, 1, label, fg, fill);
}

void ui_draw_progress_bar(int x, int y, int w, int h,
                          int val, int max, uint16_t color) {
    ui_fill_rect(x, y, w, h, UI_DARK_GRAY);
    ui_draw_rect(x, y, w, h, UI_GRAY);
    if (max > 0 && val > 0) {
        int fw = (val*(w-2))/max;
        if (fw > 0) ui_fill_rect(x+1, y+1, fw, h-2, color);
    }
}

void ui_draw_hex_dump(int x, int y, const uint8_t *data,
                      int len, uint16_t fg, uint16_t bg) {
    char line[48];
    int row = 0;
    for (int i = 0; i < len; i += 8) {
        int p = snprintf(line, sizeof(line), "%02X: ", i);
        for (int j = 0; j < 8 && (i+j) < len; j++)
            p += snprintf(line+p, sizeof(line)-p, "%02X ", data[i+j]);
        ui_draw_string(x, y + row*9, 1, line, fg, bg);
        row++;
    }
}