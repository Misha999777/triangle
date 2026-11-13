//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "pico/multicore.h"
#include <pico/flash.h>
#include <pico/stdio.h>
#include <pico/time.h>

#include "hardware/MotorSensor.h"
#include "hardware/Motor.h"
#include "hardware/AngleSensor.h"

#include "controller/Controller.h"
#include "bluetooth/Communication.h"

#define LOOP_TIME 10

Motor* volatile motor = nullptr;

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
    Controller* controller = new Controller();
    MotorSensor* motorSensor = new MotorSensor();
    motor = new Motor(motorSensor);

    Communication::init();
    Communication::setCallback([controller](const char* command) {
        return controller->handleCommand(command);
    });

    absolute_time_t nextLoop = get_absolute_time();
    while (true) {
        nextLoop = delayed_by_ms(nextLoop, LOOP_TIME);

        IMUData data = angleSensor->readData();
        float shaftVelocity = motorSensor->getVelocity();
        bool isVertical = angleSensor->isVertical();

        float torque = controller->loop(shaftVelocity, data, isVertical);
        motor->setTorque(torque);

        if (absolute_time_diff_us(get_absolute_time(), nextLoop) <= 0) {
            nextLoop = get_absolute_time();
        } else {
            sleep_until(nextLoop);
        }
    }
}
