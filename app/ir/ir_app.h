#pragma once
#include <stdint.h>

void ir_app_init(void);
void ir_app_update(void);
void ir_app_draw(void);
void ir_app_handle_key(uint8_t key);
void ir_app_exit(void);