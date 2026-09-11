#pragma once
#include <stdint.h>
#include <stdbool.h>

/* TinyUSB HID keyboard wrapper */

bool usb_hid_init(void);
void usb_hid_task(void);
bool usb_hid_ready(void);
void usb_hid_press_key(uint8_t modifier, uint8_t keycode);
void usb_hid_release(void);
void usb_hid_type_string(const char *str);
void usb_hid_type_key_name(const char *name); /* e.g. "ENTER", "GUI" */
void usb_hid_delay_ms(uint32_t ms);