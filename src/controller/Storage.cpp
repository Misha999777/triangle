//
// Created by Михайло Грошевий on 19/01/2025.
//

#include "Storage.h"

#include <cstring>

#include <pico/flash.h>

#define FLASH_TARGET_OFFSET (1024 * 1024)

namespace {
    void eraseAndProgram(void* data) {
        flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
        flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*) data, FLASH_PAGE_SIZE);
    }
}

Storage::Storage() {
    memcpy(values, (const void*) (XIP_BASE + FLASH_TARGET_OFFSET), sizeof(values));
}

float Storage::read(Slot slot, float fallback) {
    float value = values[(int) slot];
    return std::isnan(value) ? fallback : value;
}

void Storage::write(Slot slot, float value) {
    values[(int) slot] = value;
}

bool Storage::save() {
    return flash_safe_execute(eraseAndProgram, values, UINT32_MAX) == PICO_OK;
}
