#include "rfid_file.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool rfid_file_save(const char *path, const rfid_tag_t *tag) {
    char buf[256];
    int pos = snprintf(buf, sizeof(buf),
        "Filetype: Flipper RFID key\r\n"
        "Version: 1\r\n"
        "Key type: %s\r\n"
        "Data:",
        tag->proto_str);
    for (int i = 0; i < tag->data_len; i++)
        pos += snprintf(buf+pos, 4, " %02X", tag->data[i]);
    pos += snprintf(buf+pos, 4, "\r\n");
    return storage_write(path, (uint8_t*)buf, pos);
}

bool rfid_file_load(const char *path, rfid_tag_t *tag) {
    char buf[256];
    int n = storage_read(path, (uint8_t*)buf, sizeof(buf)-1);
    if (n <= 0) return false;
    buf[n] = 0;
    memset(tag, 0, sizeof(*tag));

    char *line = strtok(buf, "\r\n");
    while (line) {
        if (strncmp(line, "Key type:", 9) == 0) {
            char *t = line + 10;
            while (*t==' ') t++;
            snprintf(tag->proto_str, sizeof(tag->proto_str), "%s", t);
            if (strstr(t,"EM4100"))  tag->proto = RFID_PROTO_EM4100;
            else if (strstr(t,"HID"))tag->proto = RFID_PROTO_HID26;
        } else if (strncmp(line, "Data:", 5) == 0) {
            char *tok = strtok(line+5, " ");
            while (tok && tag->data_len < 8) {
                tag->data[tag->data_len++] = (uint8_t)strtol(tok, NULL, 16);
                tok = strtok(NULL, " ");
            }
        }
        line = strtok(NULL, "\r\n");
    }
    return tag->data_len > 0;
}