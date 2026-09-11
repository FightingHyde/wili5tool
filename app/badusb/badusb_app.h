#pragma once
#include <stdint.h>

void badusb_app_init(void);
void badusb_app_update(void);
void badusb_app_draw(void);
void badusb_app_handle_key(uint8_t key);
void badusb_app_exit(void);