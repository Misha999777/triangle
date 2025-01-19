//
// Created by Михайло Грошевий on 18/01/2025.
//

#ifndef SENSOR_H
#define SENSOR_H

#include "pico/stdlib.h"

class MotorSensor {
public:
    MotorSensor();
    float getAngle();
    float getVelocity();

private:
    float readAngle();

    uint64_t angle_prev_ts = 0;
    uint64_t vel_angle_prev_ts = 0;
    float angle_prev = 0;
    float vel_angle_prev = 0;
    int32_t full_rotations = 0;
    int32_t vel_full_rotations = 0;
};

#endif //SENSOR_H
