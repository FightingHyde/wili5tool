#include "usb_hid.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include <string.h>
#include <ctype.h>

/* ── TinyUSB HID descriptor ─────────────────────────────────────────────── */

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1))
};

const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer;   (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            const uint8_t *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer;   (void)bufsize;
}

/* ── USB descriptor strings ─────────────────────────────────────────────── */

static const char *string_desc[] = {
    "\x09\x04",          /* 0: Language = English */
    "FightingHyde",      /* 1: Manufacturer */
    "Wili5Tool BadUSB",  /* 2: Product */
    "000001",            /* 3: Serial */
};

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t desc_str[32];
    uint8_t len;
    if (index == 0) {
        memcpy(&desc_str[1], string_desc[0], 2); len=1;
    } else {
        if (index >= sizeof(string_desc)/sizeof(string_desc[0])) return NULL;
        const char *str = string_desc[index];
        len = (uint8_t)strlen(str);
        if (len > 31) len = 31;
        for (int i=0; i<len; i++) desc_str[1+i] = str[i];
    }
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING<<8) | (2*len+2));
    return desc_str;
}

/* ── USB device descriptor ──────────────────────────────────────────────── */

static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x239A,
    .idProduct          = 0x8029,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

const uint8_t *tud_descriptor_device_cb(void) {
    return (const uint8_t*)&desc_device;
}

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(hid_report_descriptor), 0x81, 16, 10),
};

const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

bool usb_hid_init(void) {
    tusb_init();
    return true;
}

void usb_hid_task(void) {
    tud_task();
}

bool usb_hid_ready(void) {
    return tud_hid_ready();
}

void usb_hid_press_key(uint8_t modifier, uint8_t keycode) {
    while (!tud_hid_ready()) tud_task();
    uint8_t keycodes[6] = {keycode,0,0,0,0,0};
    tud_hid_keyboard_report(1, modifier, keycodes);
    sleep_ms(10);
    usb_hid_release();
    sleep_ms(10);
}

void usb_hid_release(void) {
    tud_hid_keyboard_report(1, 0, NULL);
}

/* ASCII → HID keycode table (US layout) */
static const uint8_t ascii_to_hid[][2] = {
    /* keycode, modifier */
    {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},  /* 0x00-0x07 */
    {0,0},{0,0},{HID_KEY_ENTER,0},{0,0},{0,0},{0,0},{0,0},{0,0}, /* 0x08-0x0F */
    {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},  /* 0x10-0x17 */
    {0,0},{0,0},{0,0},{HID_KEY_ESCAPE,0},{0,0},{0,0},{0,0},{0,0}, /* 0x18-0x1F */
    {HID_KEY_SPACE,0},                                /* 0x20 ' ' */
    {HID_KEY_1,KEYBOARD_MODIFIER_LEFTSHIFT},          /* 0x21 '!' */
    {HID_KEY_APOSTROPHE,KEYBOARD_MODIFIER_LEFTSHIFT}, /* 0x22 '"' */
    {HID_KEY_3,KEYBOARD_MODIFIER_LEFTSHIFT},          /* 0x23 '#' */
    {HID_KEY_4,KEYBOARD_MODIFIER_LEFTSHIFT},          /* 0x24 '$' */
    {HID_KEY_5,KEYBOARD_MODIFIER_LEFTSHIFT},          /* 0x25 '%' */
    {HID_KEY_7,KEYBOARD_MODIFIER_LEFTSHIFT},          /* 0x26 '&' */
    {HID_KEY_APOSTROPHE,0},                           /* 0x27 '\'' */
    {HID_KEY_9,KEYBOARD_MODIFIER_LEFTSHIFT},          /* 0x28 '(' */
    {HID_KEY_0,KEYBOARD_MODIFIER_LEFTSHIFT},          /* 0x29 ')' */
    {HID_KEY_8,KEYBOARD_MODIFIER_LEFTSHIFT},          /* 0x2A '*' */
    {HID_KEY_EQUAL,KEYBOARD_MODIFIER_LEFTSHIFT},      /* 0x2B '+' */
    {HID_KEY_COMMA,0},                                /* 0x2C ',' */
    {HID_KEY_MINUS,0},                                /* 0x2D '-' */
    {HID_KEY_PERIOD,0},                               /* 0x2E '.' */
    {HID_KEY_SLASH,0},                                /* 0x2F '/' */
    {HID_KEY_0,0},{HID_KEY_1,0},{HID_KEY_2,0},{HID_KEY_3,0}, /* 0-3 */
    {HID_KEY_4,0},{HID_KEY_5,0},{HID_KEY_6,0},{HID_KEY_7,0}, /* 4-7 */
    {HID_KEY_8,0},{HID_KEY_9,0},                      /* 8-9 */
    {HID_KEY_SEMICOLON,KEYBOARD_MODIFIER_LEFTSHIFT},  /* ':' */
    {HID_KEY_SEMICOLON,0},                            /* ';' */
    {HID_KEY_COMMA,KEYBOARD_MODIFIER_LEFTSHIFT},      /* '<' */
    {HID_KEY_EQUAL,0},                                /* '=' */
    {HID_KEY_PERIOD,KEYBOARD_MODIFIER_LEFTSHIFT},     /* '>' */
    {HID_KEY_SLASH,KEYBOARD_MODIFIER_LEFTSHIFT},      /* '?' */
    {HID_KEY_2,KEYBOARD_MODIFIER_LEFTSHIFT},          /* '@' */
};

void usb_hid_type_string(const char *str) {
    while (*str) {
        char c = *str++;
        uint8_t keycode = 0, modifier = 0;

        if (c >= 'a' && c <= 'z') {
            keycode  = HID_KEY_A + (c-'a');
        } else if (c >= 'A' && c <= 'Z') {
            keycode  = HID_KEY_A + (c-'A');
            modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        } else if ((uint8_t)c < sizeof(ascii_to_hid)/sizeof(ascii_to_hid[0])) {
            keycode  = ascii_to_hid[(uint8_t)c][0];
            modifier = ascii_to_hid[(uint8_t)c][1];
        }

        if (keycode) usb_hid_press_key(modifier, keycode);
        sleep_ms(5);
    }
}

void usb_hid_type_key_name(const char *name) {
    if      (!strcmp(name,"ENTER"))     usb_hid_press_key(0,HID_KEY_ENTER);
    else if (!strcmp(name,"SPACE"))     usb_hid_press_key(0,HID_KEY_SPACE);
    else if (!strcmp(name,"TAB"))       usb_hid_press_key(0,HID_KEY_TAB);
    else if (!strcmp(name,"BACKSPACE")) usb_hid_press_key(0,HID_KEY_BACKSPACE);
    else if (!strcmp(name,"DELETE"))    usb_hid_press_key(0,HID_KEY_DELETE);
    else if (!strcmp(name,"ESCAPE"))    usb_hid_press_key(0,HID_KEY_ESCAPE);
    else if (!strcmp(name,"UP"))        usb_hid_press_key(0,HID_KEY_ARROW_UP);
    else if (!strcmp(name,"DOWN"))      usb_hid_press_key(0,HID_KEY_ARROW_DOWN);
    else if (!strcmp(name,"LEFT"))      usb_hid_press_key(0,HID_KEY_ARROW_LEFT);
    else if (!strcmp(name,"RIGHT"))     usb_hid_press_key(0,HID_KEY_ARROW_RIGHT);
    else if (!strcmp(name,"GUI")
          || !strcmp(name,"WINDOWS"))   usb_hid_press_key(KEYBOARD_MODIFIER_LEFTGUI,0);
    else if (!strcmp(name,"CTRL"))      usb_hid_press_key(KEYBOARD_MODIFIER_LEFTCTRL,0);
    else if (!strcmp(name,"ALT"))       usb_hid_press_key(KEYBOARD_MODIFIER_LEFTALT,0);
    else if (!strcmp(name,"SHIFT"))     usb_hid_press_key(KEYBOARD_MODIFIER_LEFTSHIFT,0);
    else if (!strcmp(name,"F1"))        usb_hid_press_key(0,HID_KEY_F1);
    else if (!strcmp(name,"F2"))        usb_hid_press_key(0,HID_KEY_F2);
    else if (!strcmp(name,"F3"))        usb_hid_press_key(0,HID_KEY_F3);
    else if (!strcmp(name,"F4"))        usb_hid_press_key(0,HID_KEY_F4);
    else if (!strcmp(name,"F5"))        usb_hid_press_key(0,HID_KEY_F5);
    else if (!strcmp(name,"F10"))       usb_hid_press_key(0,HID_KEY_F10);
    else if (!strcmp(name,"F11"))       usb_hid_press_key(0,HID_KEY_F11);
    else if (!strcmp(name,"F12"))       usb_hid_press_key(0,HID_KEY_F12);
}

void usb_hid_delay_ms(uint32_t ms) { sleep_ms(ms); }