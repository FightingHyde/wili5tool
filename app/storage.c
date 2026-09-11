#include "storage.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

static FATFS fs;
static bool  mounted = false;

bool storage_init(void) {
    if (mounted) return true;
    if (f_mount(&fs, "0:", 1) != FR_OK) return false;
    mounted = true;
    storage_mkdir(STOR_BASE);
    storage_mkdir(STOR_SUBGHZ);
    storage_mkdir(STOR_IR);
    storage_mkdir(STOR_NFC);
    storage_mkdir(STOR_RFID);
    storage_mkdir(STOR_BADUSB);
    return true;
}

bool storage_mkdir(const char *p) {
    FRESULT r = f_mkdir(p);
    return r == FR_OK || r == FR_EXIST;
}

bool storage_write(const char *path, const uint8_t *data, size_t len) {
    FIL f; UINT bw;
    if (f_open(&f, path, FA_WRITE|FA_CREATE_ALWAYS) != FR_OK) return false;
    FRESULT r = f_write(&f, data, len, &bw);
    f_close(&f);
    return r == FR_OK && bw == len;
}

int storage_read(const char *path, uint8_t *buf, size_t bufsz) {
    FIL f; UINT br;
    if (f_open(&f, path, FA_READ) != FR_OK) return -1;
    FRESULT r = f_read(&f, buf, bufsz, &br);
    f_close(&f);
    return r == FR_OK ? (int)br : -1;
}

bool storage_list(const char *dir, char names[][64], int *count,
                  int max, const char *ext) {
    DIR d; FILINFO fi;
    *count = 0;
    if (f_opendir(&d, dir) != FR_OK) return false;
    while (*count < max && f_readdir(&d, &fi) == FR_OK && fi.fname[0]) {
        if (fi.fattrib & AM_DIR) continue;
        if (ext) {
            char *dot = strrchr(fi.fname, '.');
            if (!dot || strcasecmp(dot, ext) != 0) continue;
        }
        strncpy(names[*count], fi.fname, 63);
        names[*count][63] = 0;
        (*count)++;
    }
    f_closedir(&d);
    return true;
}

void storage_unique_name(const char *dir, const char *prefix,
                         const char *ext, char *out, size_t outlen) {
    for (int i = 1; i < 9999; i++) {
        snprintf(out, outlen, "%s/%s_%04d%s", dir, prefix, i, ext);
        FIL f;
        if (f_open(&f, out, FA_READ) != FR_OK) return;
        f_close(&f);
    }
}