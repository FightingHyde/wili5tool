#include "nfc_file.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool nfc_file_save(const char *path, const nfc_tag_t *tag) {
    char *buf = malloc(4096);
    if (!buf) return false;
    int pos = 0;

    pos += snprintf(buf+pos, 256,
        "Filetype: Flipper NFC device\r\n"
        "Version: 4\r\n"
        "Device type: %s\r\n",
        tag->type_str);

    pos += snprintf(buf+pos, 64, "UID:");
    for (int i = 0; i < tag->uid_len; i++)
        pos += snprintf(buf+pos, 4, " %02X", tag->uid[i]);
    pos += snprintf(buf+pos, 4, "\r\n");

    pos += snprintf(buf+pos, 32, "ATQA: %02X %02X\r\n",
                    tag->atqa[0], tag->atqa[1]);
    pos += snprintf(buf+pos, 16, "SAK: %02X\r\n", tag->sak);

    if (tag->type == NFC_TAG_ULTRALIGHT && tag->data_valid) {
        pos += snprintf(buf+pos, 32, "Pages total: %d\r\n", tag->pages_read);
        pos += snprintf(buf+pos, 32, "Pages read: %d\r\n",  tag->pages_read);
        for (int p = 0; p < tag->pages_read; p++) {
            pos += snprintf(buf+pos, 32, "Page %d: %02X %02X %02X %02X\r\n",
                p,
                tag->data[p*4+0], tag->data[p*4+1],
                tag->data[p*4+2], tag->data[p*4+3]);
        }
    }

    bool ok = storage_write(path, (uint8_t*)buf, pos);
    free(buf);
    return ok;
}

bool nfc_file_load(const char *path, nfc_tag_t *tag) {
    char *buf = malloc(4096);
    if (!buf) return false;
    int n = storage_read(path, (uint8_t*)buf, 4095);
    if (n <= 0) { free(buf); return false; }
    buf[n] = 0;
    memset(tag, 0, sizeof(*tag));

    char *line = strtok(buf, "\r\n");
    while (line) {
        if (strncmp(line, "UID:", 4) == 0) {
            char *tok = strtok(line+4, " ");
            while (tok && tag->uid_len < 10) {
                tag->uid[tag->uid_len++] = (uint8_t)strtol(tok, NULL, 16);
                tok = strtok(NULL, " ");
            }
        } else if (strncmp(line, "ATQA:", 5) == 0) {
            sscanf(line+5, " %hhx %hhx", &tag->atqa[0], &tag->atqa[1]);
        } else if (strncmp(line, "SAK:", 4) == 0) {
            sscanf(line+4, " %hhx", &tag->sak);
        } else if (strncmp(line, "Device type:", 12) == 0) {
            snprintf(tag->type_str, sizeof(tag->type_str), "%s", line+13);
            if (strstr(line, "Ultralight"))