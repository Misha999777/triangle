//
// Created by Михайло Грошевий on 18/01/2025.
//

#include <pico/flash.h>
#include <pico/multicore.h>
#include <pico/stdio.h>
#include <pico/time.h>

#include "hardware/MotorSensor.h"
#include "hardware/Motor.h"
#include "hardware/AngleSensor.h"

#include "controller/Controller.h"
#include "controller/Storage.h"
#include "bluetooth/Communication.h"
#include "bluetooth/Telemetry.h"

#define LOOP_TIME 10

Motor* volatile motor = nullptr;

void core1Entry() {
    flash_safe_execute_core_init();

    while (true) {
        if (motor != nullptr) {
            motor->loop();
        }
    }
}

int main() {
    stdio_init_all();

    multicore_launch_core1(core1Entry);

    Storage* storage = new Storage();
    AngleSensor* angleSensor = new AngleSensor(*storage);
    Controller* controller = new Controller(*storage);
    MotorSensor* motorSensor = new MotorSensor();
    motor = new Motor(*motorSensor, *storage);

    Telemetry* telemetry = new Telemetry();
    Communication* communication = new Communication(*controller, *telemetry);

    absolute_time_t nextLoop = get_absolute_time();
    while (true) {
        nextLoop = delayed_by_ms(nextLoop, LOOP_TIME);

        IMUData data = angleSensor->readData();
        float shaftVelocity = motorSensor->getVelocity();
        bool isVertical = angleSensor->isVertical();

        float torque = controller->loop(shaftVelocity, data, isVertical);
        motor->setTorque(torque);

        telemetry->record(data.angle, data.angularVelocity, shaftVelocity, torque,
                          controller->getTargetAngle(), controller->isBalancing());

        if (absolute_time_diff_us(get_absolute_time(), nextLoop) <= 0) {
            nextLoop = get_absolute_time();
        } else {
            sleep_until(nextLoop);
        }
    }
}
