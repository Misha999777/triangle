//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "MotorSensor.h"

#include <cmath>

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/time.h>

#define PIN_CS    5
#define PIN_MISO  4
#define PIN_MOSI  3
#define PIN_SCK   2

#define SPI_PORT  spi0
#define SPI_SPEED 1000000

#define ANGLE_REGISTER     0x3FFF
#define SENSOR_RESOLUTION  14
#define SENSOR_MAX_VALUE   16383.0f

#define TWO_PI 6.28318530718f

MotorSensor::MotorSensor() {
    critical_section_init(&lock);

    spi_init(SPI_PORT, SPI_SPEED);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, true);
}

float MotorSensor::readAngle() {
    uint8_t txBuffer[2];
    uint8_t rxBuffer[2];

    txBuffer[0] = ANGLE_REGISTER >> 8 & 0xFF;
    txBuffer[1] = ANGLE_REGISTER & 0xFF;

    gpio_put(PIN_CS, false);
    spi_write_blocking(SPI_PORT, txBuffer, 2);
    gpio_put(PIN_CS, true);

    sleep_us(1);

    gpio_put(PIN_CS, false);
    spi_read_blocking(SPI_PORT, 0, rxBuffer, 2);
    gpio_put(PIN_CS, true);

    int data = rxBuffer[0] << 8 | rxBuffer[1];
    int dataMask = 0xFFFF >> (16 - SENSOR_RESOLUTION);

    int masked = data & dataMask;
    return (float) masked / SENSOR_MAX_VALUE * TWO_PI;
}

float MotorSensor::getAngle() {
    uint64_t ts = time_us_64();
    float angle = readAngle();

    critical_section_enter_blocking(&lock);
    float angleDelta = angle - previousAngle;
    if (std::abs(angleDelta) > 0.8f * TWO_PI) {
        fullRotations += angleDelta > 0 ? -1 : 1;
    }
    previousAngle = angle;
    previousAngleTime = ts;
    critical_section_exit(&lock);

    return angle;
}

float MotorSensor::getVelocity() {
    critical_section_enter_blocking(&lock);
    uint64_t ts = previousAngleTime;
    float angle = previousAngle;
    int rotations = fullRotations;
    critical_section_exit(&lock);

    if (ts == velocityPreviousTime) {
        return velocity;
    }

    float elapsedSeconds = (ts - velocityPreviousTime) * 1e-6f;
    velocity = ((rotations - velocityFullRotations) * TWO_PI + (angle - velocityPreviousAngle)) / elapsedSeconds;

    velocityPreviousAngle = angle;
    velocityPreviousTime = ts;
    velocityFullRotations = rotations;

    return velocity;
}
