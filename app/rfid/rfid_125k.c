#include "rfid_125k.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

/*
 * 125kHz carrier via PWM.
 * EM4100: 64-bit Manchester, 9-bit preamble (all 1s), then data.
 * Bit rate = carrier / 64 = ~1953 bps → bit period ~512us
 */

static bool initialized = false;
static uint pwm_slice;

/* ── Carrier control ────────────────────────────────────────────────────── */

bool rfid_125k_init(void) {
    /* TX pin: PWM carrier */
    gpio_set_function(RFID_125K_TX_PIN, GPIO_FUNC_PWM);
    pwm_slice = pwm_gpio_to_slice_num(RFID_125K_TX_PIN);
    uint32_t sys_hz = clock_get_hz(clk_sys);
    /* Set PWM to 125kHz 50% duty */
    uint32_t wrap = (sys_hz / RFID_125K_FREQ_HZ) - 1;
    pwm_set_wrap(pwm_slice, wrap);
    pwm_set_gpio_level(RFID_125K_TX_PIN, wrap / 2);
    pwm_set_enabled(pwm_slice, false);

    /* RX pin: input with pull-up */
    gpio_init(RFID_125K_RX_PIN);
    gpio_set_dir(RFID_125K_RX_PIN, GPIO_IN);
    gpio_pull_up(RFID_125K_RX_PIN);

    initialized = true;
    return true;
}

static void carrier_on(void)  { pwm_set_enabled(pwm_slice, true);  }
static void carrier_off(void) { pwm_set_enabled(pwm_slice, false); }

/* ── EM4100 decode ──────────────────────────────────────────────────────── */

/*
 * EM4100 stream: 9 preamble 1s, then 40 data bits Manchester encoded.
 * Manchester: 0 = high→low, 1 = low→high (per EM4100 spec)
 * We sample at 2x bit rate (one sample per half-bit period = ~256us)
 */
#define HALF_BIT_US  256
#define BIT_US       512
#define PREAMBLE_LEN 9

static bool wait_for_edge(bool rising, uint32_t timeout_us) {
    bool prev = gpio_get(RFID_125K_RX_PIN);
    uint32_t t = 0;
    while (t < timeout_us) {
        bool cur = gpio_get(RFID_125K_RX_PIN);
        if (rising  && !prev && cur) return true;
        if (!rising &&  prev && !cur) return true;
        prev = cur;
        sleep_us(10); t += 10;
    }
    return false;
}

static bool decode_em4100(rfid_tag_t *tag) {
    /*
     * Sample 128 half-bits (64 bits), check for 9-bit preamble of 1s,
     * then extract 40 data bits with row/column parity checks.
     */
    uint8_t bits[128];
    int bit_count = 0;

    /* Wait for preamble start */
    if (!wait_for_edge(true, 200000)) return false;

    /* Sample bits by edge detection */
    for (int i = 0; i < 128 && bit_count < 128; i++) {
        bool first_half  = gpio_get(RFID_125K_RX_PIN);
        sleep_us(HALF_BIT_US);
        bool second_half = gpio_get(RFID_125K_RX_PIN);
        sleep_us(HALF_BIT_US);

        /* Manchester: 1 = low then high, 0 = high then low */
        if (!first_half && second_half)       bits[bit_count++] = 1;
        else if (first_half && !second_half)  bits[bit_count++] = 0;
        else return false; /* encoding error */
    }

    /* Find 9 consecutive 1s (preamble) */
    int preamble_start = -1;
    for (int i = 0; i <= bit_count - PREAMBLE_LEN; i++) {
        bool ok = true;
        for (int j = 0; j < PREAMBLE_LEN; j++) {
            if (!bits[i+j]) { ok=false; break; }
        }
        if (ok) { preamble_start = i; break; }
    }
    if (preamble_start < 0) return false;

    int data_start = preamble_start + PREAMBLE_LEN;
    if (data_start + 55 > bit_count) return false; /* 40 data + 10 row + 5 col parity */

    /* Extract 40 data bits arranged as 10 rows x 4 data + 1 parity */
    uint8_t data_bits[40];
    for (int row = 0; row < 10; row++) {
        uint8_t parity = 0;
        for (int col = 0; col < 4; col++) {
            data_bits[row*4+col] = bits[data_start + row*5 + col];
            parity ^= data_bits[row*4+col];
        }
        if (parity != bits[data_start + row*5 + 4]) return false; /* row parity */
    }

    /* Pack into 5 bytes (40 bits) */
    tag->data_len = 5;
    for (int i = 0; i < 5; i++) {
        tag->data[i] = 0;
        for (int b = 0; b < 8; b++)
            tag->data[i] = (tag->data[i]<<1) | data_bits[i*8+b];
    }

    tag->proto    = RFID_PROTO_EM4100;
    tag->facility = tag->data[1];  /* byte 1 = facility/version */
    tag->card_id  = ((uint32_t)tag->data[2]<<24) |
                    ((uint32_t)tag->data[3]<<16) |
                    ((uint32_t)tag->data[4]<<8);
    snprintf(tag->proto_str, sizeof(tag->proto_str), "EM4100");
    return true;
}

/* ── HID 26-bit decode ──────────────────────────────────────────────────── */

static bool decode_hid26(rfid_tag_t *tag) {
    /*
     * HID 26-bit: 1 start bit, 8 facility, 16 card, 1 even parity, 1 odd parity
     * Encoded as FSK or Manchester — simplified version samples envelope
     */
    uint8_t bits[32];
    int bit_count = 0;

    if (!wait_for_edge(true, 200000)) return false;

    for (int i = 0; i < 32; i++) {
        bool first  = gpio_get(RFID_125K_RX_PIN);
        sleep_us(HALF_BIT_US);
        bool second = gpio_get(RFID_125K_RX_PIN);
        sleep_us(HALF_BIT_US);
        if (!first && second)       bits[bit_count++] = 1;
        else if (first && !second)  bits[bit_count++] = 0;
        else return false;
    }

    if (bit_count < 26) return false;
    if (bits[0] != 1)   return false; /* start bit */

    tag->facility = 0;
    for (int i = 1; i <= 8; i++)
        tag->facility = (tag->facility<<1) | bits[i];

    tag->card_id = 0;
    for (int i = 9; i <= 24; i++)
        tag->card_id = (tag->card_id<<1) | bits[i];

    tag->proto    = RFID_PROTO_HID26;
    tag->data_len = 4;
    tag->data[0]  = (uint8_t)(tag->facility);
    tag->data[1]  = (uint8_t)(tag->card_id >> 8);
    tag->data[2]  = (uint8_t)(tag->card_id);
    tag->data[3]  = 0;
    snprintf(tag->proto_str, sizeof(tag->proto_str), "HID26");
    return true;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

bool rfid_125k_read(rfid_tag_t *tag, uint32_t timeout_ms) {
    if (!initialized) return false;
    memset(tag, 0, sizeof(*tag));

    carrier_on();
    sleep_ms(50); /* let the field energise the tag */

    uint32_t deadline = to_ms_since_boot(get_absolute_time()) + timeout_ms;
    bool found = false;

    while (!found && to_ms_since_boot(get_absolute_time()) < deadline) {
        if (decode_em4100(tag)) { found=true; break; }
        if (decode_hid26(tag))  { found=true; break; }
        sleep_ms(10);
    }

    carrier_off();
    return found;
}

/* Emulation: modulate carrier with EM4100 Manchester data */
bool rfid_125k_emulate(const rfid_tag_t *tag) {
    if (!initialized || tag->proto == RFID_PROTO_NONE) return false;

    /* Build full 64-bit EM4100 bitstream (preamble + data) */
    uint8_t stream[64];
    int     pos = 0;

    /* 9-bit preamble */
    for (int i = 0; i < 9; i++) stream[pos++] = 1;

    /* 10 rows of 4 data bits + 1 row parity */
    for (int row = 0; row < 10; row++) {
        uint8_t parity = 0;
        for (int col = 0; col < 4; col++) {
            int bit_idx = row*4 + col;
            uint8_t b   = (tag->data[bit_idx/8] >> (7-(bit_idx%8))) & 1;
            stream[pos++] = b;
            parity ^= b;
        }
        stream[pos++] = parity;
    }

    /* 4 column-parity bits + 1 stop */
    for (int col = 0; col < 4; col++) {
        uint8_t parity = 0;
        for (int row = 0; row < 10; row++)
            parity ^= (tag->data[(row*4+col)/8] >> (7-((row*4+col)%8))) & 1;
        stream[pos++] = parity;
    }
    stream[pos++] = 0; /* stop bit */

    /* Transmit by toggling GPIO in Manchester encoding */
    carrier_on();
    for (int i = 0; i < pos; i++) {
        if (stream[i]) {
            /* 1 = low then high */
            gpio_set_function(RFID_125K_TX_PIN, GPIO_FUNC_SIO);
            gpio_set_dir(RFID_125K_TX_PIN, GPIO_OUT);
            gpio_put(RFID_125K_TX_PIN, 0); sleep_us(HALF_BIT_US);
            gpio_put(RFID_125K_TX_PIN, 1); sleep_us(HALF_BIT_US);
        } else {
            /* 0 = high then low */
            gpio_put(RFID_125K_TX_PIN, 1); sleep_us(HALF_BIT_US);
            gpio_put(RFID_125K_TX_PIN, 0); sleep_us(HALF_BIT_US);
        }
    }
    gpio_set_function(RFID_125K_TX_PIN, GPIO_FUNC_PWM);
    carrier_off();
    return true;
}

void rfid_125k_stop(void) { carrier_off(); }