#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define STOR_BASE    "0:/w5tool"
#define STOR_SUBGHZ  "0:/w5tool/subghz"
#define STOR_IR      "0:/w5tool/ir"
#define STOR_NFC     "0:/w5tool/nfc"
#define STOR_RFID    "0:/w5tool/rfid"
#define STOR_BADUSB  "0:/w5tool/badusb"

bool storage_init(void);
bool storage_mkdir(const char *path);
bool storage_write(const char *path, const uint8_t *data, size_t len);
int  storage_read(const char *path, uint8_t *buf, size_t bufsz);
bool storage_list(const char *dir, char names[][64], int *count,
                  int max, const char *ext);
void storage_unique_name(const char *dir, const char *prefix,
                         const char *ext, char *out, size_t outlen);