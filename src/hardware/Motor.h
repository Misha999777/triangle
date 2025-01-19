//
// Created by Михайло Грошевий on 18/01/2025.
//

#ifndef MOTOR_H
#define MOTOR_H

#include "MotorSensor.h"
#include "Driver.h"

class Motor {
public:
    Motor(MotorSensor* sensor);
    void loop();
    void setTorque(float newTorque);

private:
    void calibrate();
    float electricalAngle();
    void setPhaseVoltage(float Uq, float angle_el);

    Driver* driver = nullptr;
    MotorSensor* motorSensor = nullptr;

    float zero_electric_angle = 0;
    int sensor_direction = 1;
    float torque = 0;
};

#endif //MOTOR_H
