#pragma once
#include <stdint.h>
#include <stdbool.h>

#define SUB_MAX_SAMPLES 2048

typedef struct {
    uint32_t frequency;
    char     preset[32];
    int32_t  samples[SUB_MAX_SAMPLES];
    int      sample_count;
} sub_file_t;

bool sub_file_save(const char *path, const sub_file_t *f);
bool sub_file_load(const char *path, sub_file_t *f);