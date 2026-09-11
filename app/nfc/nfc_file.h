#pragma once
#include "st25r3916.h"
#include <stdbool.h>

bool nfc_file_save(const char *path, const nfc_tag_t *tag);
bool nfc_file_load(const char *path, nfc_tag_t *tag);