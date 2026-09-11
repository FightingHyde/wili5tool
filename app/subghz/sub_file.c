#include "sub_file.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool sub_file_save(const char *path, const sub_file_t *sf) {
    char *buf = malloc(SUB_MAX_SAMPLES * 8 + 256);
    if (!buf) return false;
    int pos = snprintf(buf, 256,
        "Filetype: Flipper SubGhz RAW File\r\n"
        "Version: 1\r\n"
        "Frequency: %lu\r\n"
        "Preset: %s\r\n"
        "Protocol: RAW\r\n"
        "RAW_Data:",
        (unsigned long)sf->frequency, sf->preset);
    for (int i = 0; i < sf->sample_count; i++)
        pos += snprintf(buf+pos, 12, " %ld", (long)sf->samples[i]);
    pos += snprintf(buf+pos, 4, "\r\n");
    bool ok = storage_write(path, (uint8_t*)buf, pos);
    free(buf);
    return ok;
}

bool sub_file_load(const char *path, sub_file_t *sf) {
    char *buf = malloc(SUB_MAX_SAMPLES * 8 + 256);
    if (!buf) return false;
    int n = storage_read(path, (uint8_t*)buf, SUB_MAX_SAMPLES*8+255);
    if (n <= 0) { free(buf); return false; }
    buf[n] = 0;
    sf->sample_count = 0;
    sf->frequency    = 433920000;
    snprintf(sf->preset, sizeof(sf->preset), "FuriHalSubGhzPresetOok650Async");
    char *line = strtok(buf, "\r\n");
    while (line) {
        if (strncmp(line, "Frequency:", 10) == 0)
            sf->frequency = (uint32_t)atol(line+11);
        else if (strncmp(line, "Preset:", 7) == 0)
            snprintf(sf->preset, sizeof(sf->preset), "%s", line+8);
        else if (strncmp(line, "RAW_Data:", 9) == 0) {
            char *tok = strtok(line+9, " \t");
            while (tok && sf->sample_count < SUB_MAX_SAMPLES) {
                sf->samples[sf->sample_count++] = (int32_t)atol(tok);
                tok = strtok(NULL, " \t");
            }
        }
        line = strtok(NULL, "\r\n");
    }
    free(buf);
    return sf->sample_count > 0;
}