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
    uint64_t previousAngleTime = 0;
    uint64_t velocityPreviousTime = 0;
    float previousAngle = 0;
    float velocityPreviousAngle = 0;
    int fullRotations = 0;
    int velocityFullRotations = 0;
    float velocity = 0;
};
