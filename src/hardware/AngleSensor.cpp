//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "AngleSensor.h"

#include <cstdio>
#include <cmath>

#include "hardware/i2c.h"

#include "../controller/Storage.h"

#define PINS_I2C_SDA               26
#define PINS_I2C_SCL               27

#define I2C_SPEED                  500000

#define RADIANS_TO_DEGREES         180.0 / M_PI
#define MILLIS_IN_SECOND           1000.0

#define MPU6050                    0x68
#define PWR_MGMT                   0x6B
#define ACCEL_DATA                 0x3B
#define GYRO_DATA                  0x47

#define CALIBRATION_SAMPLES        512
#define GYRO_DIVIDER               131.0
#define GYRO_WEIGHT                0.98
#define ACCEL_FILTER_VALUE         0.1
#define VERTICAL_THRESHOLD         10000

AngleSensor::AngleSensor() {
    i2c_init(i2c1, I2C_SPEED);
    gpio_set_function(PINS_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PINS_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PINS_I2C_SDA);
    gpio_pull_up(PINS_I2C_SCL);

    uint8_t buffer[2] = {PWR_MGMT, 0};
    i2c_write_blocking(i2c1, MPU6050, buffer, 2, false);
    sleep_ms(100);

    calibrateGyro();

    readAccel();
    data.angle = atan2(accelY, -accelX) * RADIANS_TO_DEGREES;
    previousTime = to_ms_since_boot(get_absolute_time());
}

IMUData AngleSensor::readData() {
    readAccel();
    readGyro();

    uint32_t currentTime = to_ms_since_boot(get_absolute_time());
    uint32_t elapsedTime = currentTime - previousTime;
    previousTime = currentTime;

    float accelAngle = atan2(accelY, -accelX) * RADIANS_TO_DEGREES;
    float gyroAngle = data.angle + gyroZ * (elapsedTime / MILLIS_IN_SECOND);

    data.angularVelocity = gyroZ;
    data.angle = gyroAngle * GYRO_WEIGHT + accelAngle * (1.0 - GYRO_WEIGHT);

    return data;
}

bool AngleSensor::isVertical() {
    return abs(accelZ) < VERTICAL_THRESHOLD;
}

void AngleSensor::calibrateGyro() {
    float data = Storage::readAtIndex(0);
    if (!std::isnan(data)) {
        printf("Found gyro value: %f\n", data);
        gyroOffset = data;
        return;
    }

    gyroOffset = 0;
    float sum = 0;
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        readGyro();
        sum += gyroZ;
        sleep_ms(3);
    }
    gyroOffset = sum / CALIBRATION_SAMPLES;

    Storage::writeAtIndex(gyroOffset, 0);
    printf("Written gyro value: %f\n", gyroOffset);
}

void AngleSensor::readAccel() {
    uint8_t buffer[6];
    readFrom(ACCEL_DATA, buffer, 6);

    accelX = (int16_t)((buffer[0] << 8) | buffer[1]);
    accelY = (int16_t)((buffer[2] << 8) | buffer[3]);

    int16_t newAccelZ = (int16_t)(buffer[4] << 8 | buffer[5]);
    accelZ = newAccelZ * ACCEL_FILTER_VALUE + accelZ * (1 - ACCEL_FILTER_VALUE);
}

void AngleSensor::readGyro() {
    uint8_t buffer[2];
    readFrom(GYRO_DATA, buffer, 2);

    gyroZ = (int16_t)(buffer[0] << 8 | buffer[1]);
    gyroZ /= GYRO_DIVIDER;
    gyroZ -= gyroOffset;
}

void AngleSensor::readFrom(uint8_t address, uint8_t *buffer, size_t length) {
    i2c_write_blocking(i2c1, MPU6050, &address, 1, true);
    i2c_read_blocking(i2c1, MPU6050, buffer, length, false);
}
