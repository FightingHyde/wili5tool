#pragma once
#include <stdint.h>

void subghz_app_init(void);
void subghz_app_update(void);
void subghz_app_draw(void);
void subghz_app_handle_key(uint8_t key);
void subghz_app_exit(void);