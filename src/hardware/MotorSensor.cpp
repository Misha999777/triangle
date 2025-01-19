//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "MotorSensor.h"

#include <cmath>

#include "hardware/spi.h"

#define PIN_CS    5
#define PIN_MISO  4
#define PIN_MOSI  3
#define PIN_SCK   2

#define SPI_PORT  spi0
#define SPI_SPEED 1000000

#define ANGLE_REGISTER     0x3FFF
#define SENSOR_RESOLUTION  14
#define SENSOR_MAX_VALUE   16383.0f

#define _2PI 6.28318530718f

MotorSensor::MotorSensor() {
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
    uint8_t tx_buf[2];
    uint8_t rx_buf[2];

    tx_buf[0] = ANGLE_REGISTER >> 8 & 0xFF;
    tx_buf[1] = ANGLE_REGISTER & 0xFF;

    gpio_put(PIN_CS, false);
    spi_write_blocking(SPI_PORT, tx_buf, 2);
    gpio_put(PIN_CS, true);

    sleep_us(1);

    gpio_put(PIN_CS, false);
    spi_read_blocking(SPI_PORT, 0, rx_buf, 2);
    gpio_put(PIN_CS, true);

    uint16_t data = rx_buf[0] << 8 | rx_buf[1];
    uint16_t data_mask = 0xFFFF >> 16 - SENSOR_RESOLUTION;

    uint16_t masked = data & data_mask;
    return (float) masked / SENSOR_MAX_VALUE * _2PI;
}

float MotorSensor::getAngle() {
    angle_prev_ts = time_us_64();
    float angle = readAngle();
    float d_angle = angle - angle_prev;
    if(abs(d_angle) > 0.8f * _2PI) {
        full_rotations += d_angle > 0 ? -1 : 1;
    }
    angle_prev = angle;

    return angle;
}

float MotorSensor::getVelocity() {
    float Ts = (angle_prev_ts - vel_angle_prev_ts) * 1e-6f;

    float velocity = ( (full_rotations - vel_full_rotations) * _2PI + (angle_prev - vel_angle_prev) ) / Ts;

    vel_angle_prev = angle_prev;
    vel_angle_prev_ts = angle_prev_ts;
    vel_full_rotations = full_rotations;

    return velocity;
}
