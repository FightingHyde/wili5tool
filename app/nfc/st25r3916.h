#pragma once
#include <stdint.h>
#include <stdbool.h>

#define ST25R3916_I2C_ADDR_50   0x50
#define ST25R3916_I2C_ADDR_51   0x51
#define ST25R3916_CHIP_ID       0x0A
#define ST25R3916B_CHIP_ID      0x0B

#define ST25R3916_REG_IC_IDENTITY    0x3F
#define ST25R3916_REG_OP_CONTROL     0x02
#define ST25R3916_REG_MODE           0x01
#define ST25R3916_REG_BIT_RATE       0x28
#define ST25R3916_REG_TX_DRIVER      0x0A
#define ST25R3916_REG_RX_CONF2       0x21
#define ST25R3916_REG_RX_CONF3       0x22
#define ST25R3916_REG_RX_CONF4       0x23
#define ST25R3916_REG_NUM_TX_BYTES2  0x4A
#define ST25R3916_REG_NUM_TX_BYTES1  0x4B
#define ST25R3916_REG_FIFO_STATUS1   0x43
#define ST25R3916_REG_RESULT1        0x35
#define ST25R3916_REG_FIFO           0x80

#define ST25R3916_CMD_SET_DEFAULT       0xC1
#define ST25R3916_CMD_CLEAR_FIFO        0xC2
#define ST25R3916_CMD_TRANSMIT_WITH_CRC 0xC4
#define ST25R3916_CMD_TRANSMIT_WOUT_CRC 0xC5

#define ST25R3916_OP_EN     0x40
#define ST25R3916_OP_RX_EN  0x08

typedef enum {
    NFC_TAG_NONE=0, NFC_TAG_ULTRALIGHT,
    NFC_TAG_MIFARE_CLASSIC_1K, NFC_TAG_MIFARE_CLASSIC_4K, NFC_TAG_ISO14443A,
} nfc_tag_type_t;

typedef struct {
    nfc_tag_type_t type;
    uint8_t uid[10];
    uint8_t uid_len;
    uint8_t atqa[2];
    uint8_t sak;
    uint8_t data[80];   /* Ultralight: 20 pages x 4 bytes */
    int     pages_read;
    bool    data_valid;
    char    type_str[32];
} nfc_tag_t;

typedef struct {
    bool detected; uint8_t chip_id, revision, i2c_addr; const char *version;
} st25r3916_info_t;

bool    st25r3916_init(void);
bool    st25r3916_get_info(st25r3916_info_t *info);
bool    st25r3916_read_reg(uint8_t reg, uint8_t *val);
bool    st25r3916_write_reg(uint8_t reg, uint8_t val);
bool    st25r3916_write_fifo(const uint8_t *data, uint8_t len);
bool    st25r3916_read_fifo(uint8_t *data, uint8_t len);
bool    st25r3916_send_cmd(uint8_t cmd);
bool    st25r3916_set_mode_idle(void);
bool    st25r3916_set_mode_field_on(void);
bool    st25r3916_iso14443a_poll(nfc_tag_t *tag);
bool    nfc_mifare_ultralight_read(nfc_tag_t *tag);
bool    nfc_mifare_classic_auth(nfc_tag_t *tag, uint8_t block,
                                bool key_a, const uint8_t *key);
bool    nfc_mifare_classic_read_block(nfc_tag_t *tag, uint8_t block,
                                      uint8_t *out16);
bool    st25r3916_emulate_uid(const uint8_t *uid, uint8_t uid_len);
void    st25r3916_emulate_stop(void);