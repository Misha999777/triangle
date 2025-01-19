//
// Created by Михайло Грошевий on 19/01/2025.
//

#include "Storage.h"

#include <cstdio>
#include <cstring>

#include "pico/stdlib.h"
#include "pico/flash.h"

#include "hardware/flash.h"

#define FLASH_TARGET_OFFSET (1024 * 1024)
const uint8_t* flash_target_contents = (const uint8_t*) (XIP_BASE + FLASH_TARGET_OFFSET);

void Storage::writeAtIndex(float value, int index) {
    float data[FLASH_PAGE_SIZE / 4] = {};
    memcpy(data, flash_target_contents, FLASH_PAGE_SIZE);

    data[index] = value;

    flash_safe_execute(call_flash_range_erase, (void* ) FLASH_TARGET_OFFSET, UINT32_MAX);

    uintptr_t params[] = {FLASH_TARGET_OFFSET, (uintptr_t) data};
    flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
}

float Storage::readAtIndex(int index) {
    float* floatContent = (float* ) flash_target_contents;
    return floatContent[index];
}

void Storage::call_flash_range_erase(void* param) {
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

void Storage::call_flash_range_program(void* param) {
    uint32_t offset = ((uintptr_t *)param)[0];
    uint8_t* data = (uint8_t *) ((uintptr_t*) param)[1];
    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}
