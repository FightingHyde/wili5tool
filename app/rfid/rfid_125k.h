#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
 * 125kHz RFID driver using RP2350 hardware PWM (carrier) + GPIO input (read)
 *
 * VERIFY THESE PINS from https://docs.freewili.com/hardware/pinmap/
 * before flashing — these are best-guess placeholders.
 */
#define RFID_125K_TX_PIN   10   /* PWM carrier output → coil driver */
#define RFID_125K_RX_PIN   11   /* Demodulated input from comparator */
#define RFID_125K_FREQ_HZ  125000

typedef enum {
    RFID_PROTO_NONE = 0,
    RFID_PROTO_EM4100,
    RFID_PROTO_HID26,
    RFID_PROTO_INDALA,
} rfid_proto_t;

typedef struct {
    rfid_proto_t proto;
    uint8_t  data[8];
    int      data_len;
    uint32_t facility;   /* HID: facility code */
    uint32_t card_id;    /* HID/EM: card number */
    char     proto_str[16];
} rfid_tag_t;

bool rfid_125k_init(void);
bool rfid_125k_read(rfid_tag_t *tag, uint32_t timeout_ms);
bool rfid_125k_emulate(const rfid_tag_t *tag);
void rfid_125k_stop(void);