//
// Created by Михайло Грошевий on 18/01/2025.
//

#pragma once

#include "MotorSensor.h"
#include "Driver.h"
#include "../controller/Storage.h"

class Motor {
public:
    Motor(MotorSensor& motorSensor, Storage& storage);
    void loop();
    void setTorque(float newTorque);

private:
    void calibrate(Storage& storage);
    float electricalAngle();
    void setPhaseVoltage(float voltage, float angle);

    MotorSensor& motorSensor;
    Driver* driver = new Driver();

    float zeroElectricAngle = 0;
    int sensorDirection = 1;
    float torque = 0;
};
