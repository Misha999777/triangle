//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "Controller.h"

#include <cmath>
#include <cstdio>

#define MAXIMUM_VOLTAGE                5

#define K1                             10.0
#define K2                             0.3
#define K3                             0.1

#define ERROR_MINIMUM_VOLTAGE          10
#define ERROR_MAXIMUM_ANGLE_DIFFERENCE 15
#define ERROR_RELEASE_TIMEOUT          50

#define ANGLE_ADJUSTMENT_TIMEOUT       1000
#define ANGLE_ADJUSTMENT_THRESHOLD     5
#define ANGLE_ADJUSTMENT_STEP          0.05

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

float Controller::loop(float shaftVelocity, IMUData data, bool isVertical, float voltage) {
    auto [angle, angularVelocity] = data;

    float error = angle - targetAngle;
    if (shouldRun(voltage, error, isVertical)) {
        adjustTargetAngle(shaftVelocity);

        return controller(error, angularVelocity, shaftVelocity);
    }

    return 0;
}

bool Controller::shouldRun(float voltage, float error, bool isVertical) {
    if (voltage < ERROR_MINIMUM_VOLTAGE) {
        wasBatteryDrained = true;
    }

    bool isErrorSmallEnough = std::abs(error) < ERROR_MAXIMUM_ANGLE_DIFFERENCE;
    bool isFreeToRun = !wasBatteryDrained && isVertical && isErrorSmallEnough;

    if (isFreeToRun) {
        errorCounter++;
        errorCounter = max(0, min(errorCounter, ERROR_RELEASE_TIMEOUT));
    } else {
        errorCounter = 0;
    }

    return errorCounter == ERROR_RELEASE_TIMEOUT;
}

void Controller::adjustTargetAngle(float shaftVelocity) {
    if (averageVelocityCounter != ANGLE_ADJUSTMENT_TIMEOUT) {
        averageVelocity += shaftVelocity;
        averageVelocityCounter++;
        return;
    }

    averageVelocity /= ANGLE_ADJUSTMENT_TIMEOUT;
    if (averageVelocity > ANGLE_ADJUSTMENT_THRESHOLD) {
        targetAngle -= ANGLE_ADJUSTMENT_STEP;
    } else if (averageVelocity < -ANGLE_ADJUSTMENT_THRESHOLD) {
        targetAngle += ANGLE_ADJUSTMENT_STEP;
    }

    averageVelocity = 0.0;
    averageVelocityCounter = 0;
}

float Controller::controller(float angle, float velocity, float shaftVelocity) {
    float u = K1 * angle + K2 * velocity + K3 * shaftVelocity;
    u = max(-MAXIMUM_VOLTAGE, min(u, MAXIMUM_VOLTAGE));
    return u;
}