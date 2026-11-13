//
// Created by Михайло Грошевий on 18/01/2025.
//

#pragma once

#include <pico/types.h>

struct IMUData {
    float angle;
    float angularVelocity;
};

class AngleSensor {
public:
    AngleSensor();
    IMUData readData();
    bool isVertical();

private:
    float gyroOffset = 0;

    int accelX = 0;
    int accelY = 0;
    int accelZ = 0;
    float gyroZ = 0;

    int previousTime = 0;
    IMUData data = {};

    void readFrom(int address, uint8_t *buffer, int length);
    void readAccel();
    void readGyro();
    void calibrateGyro();
};
