//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "Motor.h"

#include <cstdio>
#include <cmath>

#include <hardware/gpio.h>
#include <pico/time.h>

#define PINS_EN                 13

#define POLE_PAIRS              11
#define SUPPLY_VOLTAGE          12.0f

#define CALIBRATION_VOLTAGE     3.0f
#define CALIBRATION_STEPS       500
#define CALIBRATION_STEP_MS     2
#define CALIBRATION_SETTLE_MS   1000

#define TWO_PI                  6.28318530718f
#define THREE_HALVES_PI         4.71238898038f
#define SQRT3_HALF              0.86602540378f

Motor::Motor(MotorSensor& motorSensor, Storage& storage) : motorSensor(motorSensor) {
    gpio_init(PINS_EN);
    gpio_set_dir(PINS_EN, GPIO_OUT);
    gpio_put(PINS_EN, false);

    calibrate(storage);
}

void Motor::loop() {
    setPhaseVoltage(torque, electricalAngle());
}

void Motor::setTorque(float newTorque) {
    torque = newTorque;
}

void Motor::calibrate(Storage& storage) {
    gpio_put(PINS_EN, true);

    float angleOffset = storage.read(Storage::Slot::MOTOR_ZERO_ANGLE);
    float direction = storage.read(Storage::Slot::MOTOR_DIRECTION);
    if (!std::isnan(angleOffset) && !std::isnan(direction)) {
        printf("Found offset value: %f\n", angleOffset);
        printf("Found direction value: %f\n", direction);

        zeroElectricAngle = angleOffset;
        sensorDirection = direction < 0 ? -1 : 1;
        return;
    }

    float midAngle = motorSensor.getAngle();
    for (int i = CALIBRATION_STEPS; i >= 0; i--) {
        float angle = THREE_HALVES_PI + TWO_PI * i / CALIBRATION_STEPS;
        setPhaseVoltage(CALIBRATION_VOLTAGE, angle);
        sleep_ms(CALIBRATION_STEP_MS);
    }
    float endAngle = motorSensor.getAngle();
    if (midAngle < endAngle) {
        sensorDirection = -1;
    }

    setPhaseVoltage(CALIBRATION_VOLTAGE, THREE_HALVES_PI);
    sleep_ms(CALIBRATION_SETTLE_MS);
    zeroElectricAngle = electricalAngle();

    driver->writeDutyCycle(0, 0, 0);

    storage.write(Storage::Slot::MOTOR_ZERO_ANGLE, zeroElectricAngle);
    storage.write(Storage::Slot::MOTOR_DIRECTION, sensorDirection);
    storage.save();
    printf("Written offset value: %f\n", zeroElectricAngle);
    printf("Written direction value: %d\n", sensorDirection);
}

float Motor::electricalAngle() {
    float angle = motorSensor.getAngle();
    float electricalAngle = sensorDirection * POLE_PAIRS * angle - zeroElectricAngle;
    float a = std::fmod(electricalAngle, TWO_PI);
    return a >= 0 ? a : a + TWO_PI;
}

void Motor::setPhaseVoltage(float voltage, float angle) {
    float sa = std::sin(angle);
    float ca = std::cos(angle);

    // Inverse Park transform
    float alpha = - sa * voltage;
    float beta = ca * voltage;

    // Inverse Clarke transform
    float a = alpha;
    float b = -0.5f * alpha + SQRT3_HALF * beta;
    float c = -0.5f * alpha - SQRT3_HALF * beta;

    float center = SUPPLY_VOLTAGE / 2;
    a += center;
    b += center;
    c += center;

    driver->writeDutyCycle(a / SUPPLY_VOLTAGE, b / SUPPLY_VOLTAGE, c / SUPPLY_VOLTAGE);
}
