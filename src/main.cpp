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
#include "bluetooth/Telemetry.h"

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

    Communication::Handlers handlers;
    handlers.readParameters = [controller]() {
        Parameters parameters = controller->getParameters();
        return triangle_v1_Parameters{
            true, parameters.k1, true, parameters.k2, true, parameters.k3, true, parameters.targetAngle,
        };
    };
    handlers.writeParameters = [controller](const triangle_v1_Parameters& update) {
        Parameters parameters = controller->getParameters();
        if (update.has_k1) parameters.k1 = update.k1;
        if (update.has_k2) parameters.k2 = update.k2;
        if (update.has_k3) parameters.k3 = update.k3;
        if (update.has_target_angle) parameters.targetAngle = update.target_angle;
        return controller->setParameters(parameters) ? triangle_v1_Error_ERROR_NONE : triangle_v1_Error_ERROR_INVALID_VALUE;
    };
    handlers.executeCommand = [controller](const triangle_v1_Command& command) {
        switch (command.type) {
            case triangle_v1_CommandType_COMMAND_TYPE_SAVE_PARAMETERS:
                return controller->saveParameters() ? triangle_v1_Error_ERROR_NONE : triangle_v1_Error_ERROR_BUSY;
            default:
                return triangle_v1_Error_ERROR_UNKNOWN_COMMAND;
        }
    };
    Communication::init(handlers);

    absolute_time_t nextLoop = get_absolute_time();
    while (true) {
        nextLoop = delayed_by_ms(nextLoop, LOOP_TIME);

        IMUData data = angleSensor->readData();
        float shaftVelocity = motorSensor->getVelocity();
        bool isVertical = angleSensor->isVertical();

        float torque = controller->loop(shaftVelocity, data, isVertical);
        motor->setTorque(torque);

        Telemetry::record(data.angle, data.angularVelocity, shaftVelocity, torque,
                          controller->getTargetAngle(), controller->isBalancing());

        if (absolute_time_diff_us(get_absolute_time(), nextLoop) <= 0) {
            nextLoop = get_absolute_time();
        } else {
            sleep_until(nextLoop);
        }
    }
}
