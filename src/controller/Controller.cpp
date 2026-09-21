//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "Controller.h"

#include <cmath>

#include "../bluetooth/Communication.h"

#define MAXIMUM_VOLTAGE                5

#define ERROR_MINIMUM_VOLTAGE          10
#define ERROR_MAXIMUM_ANGLE_DIFFERENCE 15
#define ERROR_RELEASE_TIMEOUT          50

#define ANGLE_ADJUSTMENT_TIMEOUT       1000
#define ANGLE_ADJUSTMENT_THRESHOLD     5
#define ANGLE_ADJUSTMENT_STEP          0.05

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

float Controller::loop(float shaftVelocity, IMUData data, bool isVertical) {
    auto [angle, angularVelocity] = data;

    currentState = "Target angle: " + std::to_string(targetAngle) +
        ", Current angle: " + std::to_string(angle);
    Communication::setCurrentValue(currentState);

    float error = angle - targetAngle;
    if (shouldRun(error, isVertical)) {
        adjustTargetAngle(shaftVelocity);

        return controller(error, angularVelocity, shaftVelocity);
    }

    return 0;
}

void Controller::setParameter(int index, float value) {
    switch (index) {
        case 1:
            k1 = value;
            break;
        case 2:
            k2 = value;
            break;
        case 3:
            k3 = value;
            break;
        case 4:
            targetAngle = value;
            break;
    }
}

bool Controller::shouldRun(float error, bool isVertical) {
    bool isErrorSmallEnough = std::abs(error) < ERROR_MAXIMUM_ANGLE_DIFFERENCE;
    bool isFreeToRun = isVertical && isErrorSmallEnough;

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
    float u = k1 * angle + k2 * velocity + k3 * shaftVelocity;
    u = max(-MAXIMUM_VOLTAGE, min(u, MAXIMUM_VOLTAGE));
    return u;
}