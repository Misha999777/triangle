//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "Motor.h"

#include <cstdio>
#include <cmath>

#include "../controller/Storage.h"

#define PINS_EN  13

#define _2PI     6.28318530718f
#define _3PI_2   4.71238898038f
#define _SQRT3_2 0.86602540378f

Motor::Motor(MotorSensor* sensor) {
    motorSensor = sensor;

    gpio_init(PINS_EN);
    gpio_set_dir(PINS_EN, GPIO_OUT);
    gpio_put(PINS_EN, false);

    driver = new Driver();

    calibrate();
}

void Motor::loop() {
    float electric_angle = electricalAngle();
    setPhaseVoltage(torque, electric_angle);
}

void Motor::setTorque(float newTorque) {
    torque = newTorque;
}

void Motor::calibrate() {
    gpio_put(PINS_EN, true);

    float angleOffset = Storage::readAtIndex(1);
    float direction = Storage::readAtIndex(2);
    if (!std::isnan(angleOffset) && !std::isnan(direction)) {
        printf("Found offset value: %f\n", angleOffset);
        printf("Found direction value: %f\n", direction);

        zero_electric_angle = angleOffset;
        sensor_direction = direction < 0 ? -1 : 1;
        return;
    }

    float mid_angle = motorSensor->getAngle();
    for (int i = 500; i >= 0; i--) {
        float angle = _3PI_2 + _2PI * i / 500.0f ;
        setPhaseVoltage(3, angle);
        sleep_ms(2);
    }
    float end_angle = motorSensor->getAngle();
    if (mid_angle < end_angle) {
        sensor_direction = -1;
    }

    setPhaseVoltage(3, _3PI_2);
    sleep_ms(1000);
    zero_electric_angle = electricalAngle();

    driver->writeDutyCycle(0, 0, 0);

    Storage::writeAtIndex(zero_electric_angle, 1);
    Storage::writeAtIndex(sensor_direction, 2);
    printf("Written offset value: %f\n", zero_electric_angle);
    printf("Written direction value: %d\n", sensor_direction);
}

float Motor::electricalAngle() {
    float angle = motorSensor->getAngle();
    float electricalAngle = sensor_direction * 11 * angle - zero_electric_angle;
    float a = fmod(electricalAngle, _2PI);
    return a >= 0 ? a : a + _2PI;
}

void Motor::setPhaseVoltage(float Uq, float angle_el) {
    float sa = sin(angle_el);
    float ca = cos(angle_el);

    // Inverse Park transform
    float alpha = - sa * Uq;
    float beta = ca * Uq;

    // Inverse Clarke transform
    float a = alpha;
    float b = -0.5f * alpha + _SQRT3_2 * beta;
    float c = -0.5f * alpha - _SQRT3_2 * beta;

    float center = 6;
    a += center;
    b += center;
    c += center;

    driver->writeDutyCycle(a / 12, b / 12, c / 12);
}
