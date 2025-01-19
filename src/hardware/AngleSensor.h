//
// Created by Михайло Грошевий on 18/01/2025.
//

#ifndef IMU_H
#define IMU_H

#include "pico/stdlib.h"

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

    int16_t accelX = 0;
    int16_t accelY = 0;
    int16_t accelZ = 0;
    int16_t gyroZ = 0;

    long previousTime = 0;
    IMUData data = {};

    void readFrom(uint8_t address, uint8_t *buffer, size_t length);
    void readAccel();
    void readGyro();
    void calibrateGyro();
};

#endif //IMU_H
