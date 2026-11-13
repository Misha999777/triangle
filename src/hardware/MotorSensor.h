//
// Created by Михайло Грошевий on 18/01/2025.
//

#pragma once

#include <pico/types.h>
#include <pico/critical_section.h>

class MotorSensor {
public:
    MotorSensor();
    float getAngle();
    float getVelocity();

private:
    float readAngle();

    critical_section_t lock;
    uint64_t angle_prev_ts = 0;
    uint64_t vel_angle_prev_ts = 0;
    float angle_prev = 0;
    float vel_angle_prev = 0;
    int full_rotations = 0;
    int vel_full_rotations = 0;
    float velocity = 0;
};
