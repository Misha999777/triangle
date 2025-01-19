#include <cmath>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <pico/flash.h>

#include "hardware/MotorSensor.h"
#include "hardware/Motor.h"
#include "hardware/Battery.h"
#include "hardware/AngleSensor.h"

#include "controller/Controller.h"

#define LOOP_TIME 10

Motor* motor;

[[noreturn]] void core1_entry() {
    flash_safe_execute_core_init();

    while (true) {
        if (motor != nullptr) {
            motor->loop();
        }
    }
}

[[noreturn]] int main() {
    stdio_init_all();

    multicore_launch_core1(core1_entry);

    AngleSensor* angleSensor = new AngleSensor();
    Battery* battery = new Battery();
    Controller* controller = new Controller();
    MotorSensor* motorSensor = new MotorSensor();
    motor = new Motor(motorSensor);

    while (true) {
        IMUData data = angleSensor->readData();
        float voltage = battery->getVoltage();
        float shaftVelocity = motorSensor->getVelocity();
        bool isVertical = angleSensor->isVertical();

        float torque = controller->loop(shaftVelocity, data, isVertical, voltage);
        motor->setTorque(torque);

        sleep_ms(LOOP_TIME);
    }
}
