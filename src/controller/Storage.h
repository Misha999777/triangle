//
// Created by Михайло Грошевий on 19/01/2025.
//

#pragma once

#include <cmath>

#include <hardware/flash.h>

class Storage {
public:
    enum class Slot {
        GYRO_OFFSET = 0,
        MOTOR_ZERO_ANGLE = 1,
        MOTOR_DIRECTION = 2,
        K1 = 3,
        K2 = 4,
        K3 = 5,
        TARGET_ANGLE = 6,
    };

    Storage();

    float read(Slot slot, float fallback = NAN);
    void write(Slot slot, float value);
    bool save();

private:
    float values[FLASH_PAGE_SIZE / sizeof(float)] = {};
};
