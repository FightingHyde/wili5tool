#include "st25r3916.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <string.h>

static uint8_t nfc_addr   = 0;
static bool    nfc_inited = false;

bool st25r3916_read_reg(uint8_t reg, uint8_t *val) {
    if (!nfc_addr) return false;
    if (i2c_write_blocking(i2c1, nfc_addr, &reg, 1, true) < 0) return false;
    return i2c_read_blocking(i2c1, nfc_addr, val, 1, false) >= 0;
}

bool st25r3916_write_reg(uint8_t reg, uint8_t val) {
    if (!nfc_addr) return false;
    uint8_t tx[2]={reg,val};
    return i2c_write_blocking(i2c1, nfc_addr, tx, 2, false) >= 0;
}

bool st25r3916_write_fifo(const uint8_t *d, uint8_t len) {
    if (!nfc_addr||!len) return false;
    uint8_t buf[65]; buf[0]=ST25R3916_REG_FIFO;
    memcpy(buf+1,d,len);
    return i2c_write_blocking(i2c1,nfc_addr,buf,len+1,false)>=0;
}

bool st25r3916_read_fifo(uint8_t *d, uint8_t len) {
    if (!nfc_addr||!len) return false;
    uint8_t reg=ST25R3916_REG_FIFO;
    if (i2c_write_blocking(i2c1,nfc_addr,&reg,1,true)<0) return false;
    return i2c_read_blocking(i2c1,nfc_addr,d,len,false)>=0;
}

bool st25r3916_send_cmd(uint8_t cmd) {
    if (!nfc_addr) return false;
    return i2c_write_blocking(i2c1,nfc_addr,&cmd,1,false)>=0;
}

bool st25r3916_init(void) {
    nfc_inited=false; nfc_addr=0;
    uint8_t addrs[]={ST25R3916_I2C_ADDR_50,ST25R3916_I2C_ADDR_51};
    for (int i=0;i<2;i++) {
        uint8_t d;
        if (i2c_read_blocking(i2c1,addrs[i],&d,1,false)>=0) {
            nfc_addr=addrs[i]; break;
        }
    }
    if (!nfc_addr) return false;
    uint8_t id;
    if (!st25r3916_read_reg(ST25R3916_REG_IC_IDENTITY,&id)) return false;
    uint8_t chip=(id>>4)&0x0F;
    if (chip!=ST25R3916_CHIP_ID&&chip!=ST25R3916B_CHIP_ID) return false;
    st25r3916_send_cmd(ST25R3916_CMD_SET_DEFAULT); sleep_ms(10);
    st25r3916_write_reg(ST25R3916_REG_MODE,0x00);
    st25r3916_write_reg(ST25R3916_REG_BIT_RATE,0x00);
    st25r3916_write_reg(ST25R3916_REG_TX_DRIVER,0x23);
    st25r3916_write_reg(ST25R3916_REG_RX_CONF2,0x3D);
    st25r3916_write_reg(ST25R3916_REG_RX_CONF3,0x00);
    st25r3916_write_reg(ST25R3916_REG_RX_CONF4,0x00);
    nfc_inited=true; return true;
}

bool st25r3916_get_info(st25r3916_info_t *info) {
    memset(info,0,sizeof(*info));
    if (!nfc_inited||!nfc_addr) return false;
    uint8_t id;
    if (!st25r3916_read_reg(ST25R3916_REG_IC_IDENTITY,&id)) return false;
    info->detected=true;
    info->chip_id=(id>>4)&0x0F;
    info->revision=id&0x0F;
    info->i2c_addr=nfc_addr;
    info->version=(info->chip_id==ST25R3916B_CHIP_ID)?"ST25R3916B":"ST25R3916";
    return true;
}

bool st25r3916_set_mode_idle(void) {
    return st25r3916_write_reg(ST25R3916_REG_OP_CONTROL,0x00);
}
bool st25r3916_set_mode_field_on(void) {
    return st25r3916_write_reg(ST25R3916_REG_OP_CONTROL,
                               ST25R3916_OP_EN|ST25R3916_OP_RX_EN);
}

/* ── ISO 14443A ──────────────────────────────────────────────────────────── */

static bool send_reqa(uint8_t *atqa) {
    st25r3916_send_cmd(ST25R3916_CMD_CLEAR_FIFO);
    uint8_t r=0x26;
    st25r3916_write_fifo(&r,1);
    st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES2,0x00);
    st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES1,0x07);
    st25r3916_send_cmd(ST25R3916_CMD_TRANSMIT_WOUT_CRC);
    sleep_ms(5);
    uint8_t fs; st25r3916_read_reg(ST25R3916_REG_FIFO_STATUS1,&fs);
    if ((fs&0x1F)<2) return false;
    return st25r3916_read_fifo(atqa,2);
}

static bool anticoll_select(uint8_t cas, uint8_t *uid, uint8_t *ulen,
                             uint8_t *sak) {
    uint8_t cmd[2]={cas,0x20};
    st25r3916_send_cmd(ST25R3916_CMD_CLEAR_FIFO);
    st25r3916_write_fifo(cmd,2);
    st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES2,0x00);
    st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES1,0x10);
    st25r3916_send_cmd(ST25R3916_CMD_TRANSMIT_WOUT_CRC);
    sleep_ms(5);
    uint8_t fs; st25r3916_read_reg(ST25R3916_REG_FIFO_STATUS1,&fs);
    if ((fs&0x1F)<5) return false;
    uint8_t ct[5]; if (!st25r3916_read_fifo(ct,5)) return false;
    uint8_t sel[7]={cas,0x70}; memcpy(sel+2,ct,5);
    st25r3916_send_cmd(ST25R3916_CMD_CLEAR_FIFO);
    st25r3916_write_fifo(sel,7);
    st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES2,0x00);
    st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES1,0x38);
    st25r3916_send_cmd(ST25R3916_CMD_TRANSMIT_WITH_CRC);
    sleep_ms(5);
    st25r3916_read_reg(ST25R3916_REG_FIFO_STATUS1,&fs);
    if ((fs&0x1F)<1) return false;
    if (!st25r3916_read_fifo(sak,1)) return false;
    if (ct[0]==0x88) { memcpy(uid+*ulen,ct+1,3); *ulen+=3; }
    else             { memcpy(uid+*ulen,ct,4);   *ulen+=4; }
    return true;
}

bool st25r3916_iso14443a_poll(nfc_tag_t *tag) {
    if (!nfc_inited) return false;
    memset(tag,0,sizeof(*tag));
    st25r3916_set_mode_field_on(); sleep_ms(5);
    if (!send_reqa(tag->atqa)) return false;
    uint8_t cas[]={0x93,0x95,0x97}, sak=0;
    for (int l=0;l<3;l++) {
        uint8_t prev=tag->uid_len;
        if (!anticoll_select(cas[l],tag->uid,&tag->uid_len,&sak)) return false;
        tag->sak=sak;
        if (tag->uid_len==prev||!(sak&0x04)) break;
    }
    if (!tag->uid_len) return false;
    if      ((sak&0x60)==0x20){ tag->type=NFC_TAG_ISO14443A;
                                 snprintf(tag->type_str,32,"ISO14443-4A"); }
    else if (sak==0x00)       { tag->type=NFC_TAG_ULTRALIGHT;
                                 snprintf(tag->type_str,32,"MIFARE Ultralight"); }
    else if (sak==0x08)       { tag->type=NFC_TAG_MIFARE_CLASSIC_1K;
                                 snprintf(tag->type_str,32,"MIFARE Classic 1K"); }
    else if (sak==0x18)       { tag->type=NFC_TAG_MIFARE_CLASSIC_4K;
                                 snprintf(tag->type_str,32,"MIFARE Classic 4K"); }
    else { tag->type=NFC_TAG_ISO14443A;
           snprintf(tag->type_str,32,"ISO14443A SAK:%02X",sak); }
    return true;
}

bool nfc_mifare_ultralight_read(nfc_tag_t *tag) {
    tag->pages_read=0;
    for (uint8_t pg=0;pg<20;pg++) {
        uint8_t cmd[2]={0x30,pg};
        st25r3916_send_cmd(ST25R3916_CMD_CLEAR_FIFO);
        st25r3916_write_fifo(cmd,2);
        st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES2,0x00);
        st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES1,0x10);
        st25r3916_send_cmd(ST25R3916_CMD_TRANSMIT_WITH_CRC);
        sleep_ms(5);
        uint8_t fs; st25r3916_read_reg(ST25R3916_REG_FIFO_STATUS1,&fs);
        if ((fs&0x1F)<4) break;
        if (!st25r3916_read_fifo(tag->data+pg*4,4)) break;
        tag->pages_read++;
    }
    tag->data_valid=(tag->pages_read>0);
    return tag->data_valid;
}

bool nfc_mifare_classic_auth(nfc_tag_t *tag, uint8_t block,
                              bool key_a, const uint8_t *key) {
    uint8_t cmd[12];
    cmd[0]=key_a?0x60:0x61; cmd[1]=block;
    memcpy(cmd+2,key,6); memcpy(cmd+8,tag->uid,4);
    st25r3916_send_cmd(ST25R3916_CMD_CLEAR_FIFO);
    st25r3916_write_fifo(cmd,12);
    st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES2,0x00);
    st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES1,0x60);
    st25r3916_send_cmd(ST25R3916_CMD_TRANSMIT_WITH_CRC);
    sleep_ms(10);
    uint8_t res; st25r3916_read_reg(ST25R3916_REG_RESULT1,&res);
    return !(res&0x01);
}

bool nfc_mifare_classic_read_block(nfc_tag_t *tag, uint8_t block,
                                   uint8_t *out16) {
    (void)tag;
    uint8_t cmd[2]={0x30,block};
    st25r3916_send_cmd(ST25R3916_CMD_CLEAR_FIFO);
    st25r3916_write_fifo(cmd,2);
    st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES2,0x00);
    st25r3916_write_reg(ST25R3916_REG_NUM_TX_BYTES1,0x10);
    st25r3916_send_cmd(ST25R3916_CMD_TRANSMIT_WITH_CRC);
    sleep_ms(5);
    uint8_t fs; st25r3916_read_reg(ST25R3916_REG_FIFO_STATUS1,&fs);
    if ((fs&0x1F)<16) return false;
    return st25r3916_read_fifo(out16,16);
}

static bool emulating=false;
bool st25r3916_emulate_uid(const uint8_t *uid, uint8_t uid_len) {
    if (!nfc_inited) return false;
    st25r3916_write_reg(ST25R3916_REG_MODE,0x80);
    st25r3916_write_reg(ST25R3916_REG_OP_CONTROL,0x41);
    st25r3916_send_cmd(ST25R3916_CMD_CLEAR_FIFO);
    st25r3916_write_fifo(uid,uid_len);
    emulating=true; return true;
}
void st25r3916_emulate_stop(void) {
    if(emulating){st25r3916_set_mode_idle();emulating=false;}
}