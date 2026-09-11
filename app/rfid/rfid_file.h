#pragma once
#include "rfid_125k.h"
#include <stdbool.h>

bool rfid_file_save(const char *path, const rfid_tag_t *tag);
bool rfid_file_load(const char *path, rfid_tag_t *tag);