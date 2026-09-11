#pragma once
#include <stdint.h>

void rfid_app_init(void);
void rfid_app_update(void);
void rfid_app_draw(void);
void rfid_app_handle_key(uint8_t key);
void rfid_app_exit(void);