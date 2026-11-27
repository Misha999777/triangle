//
// Created by Михайло Грошевий on 18/01/2025.
//

#pragma once

#include "../hardware/AngleSensor.h"
#include "../bluetooth/Communication.h"
#include "Storage.h"

class Controller : public Communication::Delegate {
public:
    Controller(Storage& storage);
    float loop(float shaftVelocity, IMUData data, bool isVertical);

    triangle_v1_Parameters getParameters() override;
    bool setParameters(const triangle_v1_Parameters& parameters) override;
    bool saveParameters() override;

    bool isBalancing();
    float getTargetAngle();

private:
    bool shouldRun(float error, bool isVertical);
    float computeTorque(float angle, float velocity, float shaftVelocity);
    void adjustTargetAngle(float shaftVelocity);

    Storage& storage;

    bool isRunning = false;

    float averageVelocity = 0;
    int averageVelocityCounter = 0;

    int errorCounter = 0;

    float targetAngle = 0;
    float k1 = 0;
    float k2 = 0;
    float k3 = 0;
};
