#pragma once
#include <stdint.h>
#include <stdbool.h>

#define UI_BLACK      0x0000
#define UI_WHITE      0xFFFF
#define UI_ORANGE     0xFC00
#define UI_GREEN      0x07E0
#define UI_RED        0xF800
#define UI_BLUE       0x001F
#define UI_CYAN       0x07FF
#define UI_YELLOW     0xFFE0
#define UI_GRAY       0x8410
#define UI_DARK_GRAY  0x4208
#define UI_BG         0x0861
#define UI_ACCENT     0xFC00
#define UI_SELECTED   0x2945

#define SCREEN_W  480
#define SCREEN_H  320

void ui_init(void);
void ui_clear(uint16_t color);
void ui_draw_header(const char *title, const char *subtitle);
void ui_draw_status_bar(const char *left, const char *right);
void ui_draw_button(int x, int y, int w, int h, const char *label,
                    bool selected, uint16_t fg, uint16_t bg);
void ui_draw_string(int x, int y, int scale, const char *str,
                    uint16_t fg, uint16_t bg);
void ui_fill_rect(int x, int y, int w, int h, uint16_t color);
void ui_draw_rect(int x, int y, int w, int h, uint16_t color);
void ui_draw_hline(int x, int y, int w, uint16_t color);
void ui_draw_progress_bar(int x, int y, int w, int h,
                          int val, int max, uint16_t color);
void ui_draw_hex_dump(int x, int y, const uint8_t *data,
                      int len, uint16_t fg, uint16_t bg);