//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "Controller.h"

#include <algorithm>
#include <cmath>

#define DEFAULT_TARGET_ANGLE           92.5f
#define DEFAULT_K1                     10.0f
#define DEFAULT_K2                     0.3f
#define DEFAULT_K3                     0.1f

#define MAXIMUM_VOLTAGE                5.0f

#define ERROR_MAXIMUM_ANGLE_DIFFERENCE 15
#define ERROR_RELEASE_TIMEOUT          50

#define ANGLE_ADJUSTMENT_TIMEOUT       500
#define ANGLE_ADJUSTMENT_THRESHOLD     5
#define ANGLE_ADJUSTMENT_STEP          0.025f

Controller::Controller(Storage& storage) : storage(storage) {
    targetAngle = storage.read(Storage::Slot::TARGET_ANGLE, DEFAULT_TARGET_ANGLE);
    k1 = storage.read(Storage::Slot::K1, DEFAULT_K1);
    k2 = storage.read(Storage::Slot::K2, DEFAULT_K2);
    k3 = storage.read(Storage::Slot::K3, DEFAULT_K3);
}

float Controller::loop(float shaftVelocity, IMUData data, bool isVertical) {
    auto [angle, angularVelocity] = data;

    float error = angle - targetAngle;
    isRunning = shouldRun(error, isVertical);
    if (isRunning) {
        adjustTargetAngle(shaftVelocity);

        return computeTorque(error, angularVelocity, shaftVelocity);
    }

    return 0;
}

triangle_v1_Parameters Controller::getParameters() {
    return {true, k1, true, k2, true, k3, true, targetAngle};
}

bool Controller::setParameters(const triangle_v1_Parameters& parameters) {
    if ((parameters.has_k1 && !std::isfinite(parameters.k1)) ||
        (parameters.has_k2 && !std::isfinite(parameters.k2)) ||
        (parameters.has_k3 && !std::isfinite(parameters.k3)) ||
        (parameters.has_target_angle && !std::isfinite(parameters.target_angle))) {
        return false;
    }
    if (parameters.has_k1) k1 = parameters.k1;
    if (parameters.has_k2) k2 = parameters.k2;
    if (parameters.has_k3) k3 = parameters.k3;
    if (parameters.has_target_angle) targetAngle = parameters.target_angle;
    return true;
}

bool Controller::saveParameters() {
    // Writing flash pauses both cores, which would stop the motor mid-balance.
    if (isRunning) {
        return false;
    }
    storage.write(Storage::Slot::K1, k1);
    storage.write(Storage::Slot::K2, k2);
    storage.write(Storage::Slot::K3, k3);
    storage.write(Storage::Slot::TARGET_ANGLE, targetAngle);
    return storage.save();
}

bool Controller::isBalancing() {
    return isRunning;
}

float Controller::getTargetAngle() {
    return targetAngle;
}

bool Controller::shouldRun(float error, bool isVertical) {
    bool isErrorSmallEnough = std::abs(error) < ERROR_MAXIMUM_ANGLE_DIFFERENCE;
    bool isFreeToRun = isVertical && isErrorSmallEnough;

    if (isFreeToRun) {
        errorCounter = std::clamp(errorCounter + 1, 0, ERROR_RELEASE_TIMEOUT);
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

    averageVelocity = 0;
    averageVelocityCounter = 0;
}

float Controller::computeTorque(float angle, float velocity, float shaftVelocity) {
    float u = k1 * angle + k2 * velocity + k3 * shaftVelocity;
    return std::clamp(u, -MAXIMUM_VOLTAGE, MAXIMUM_VOLTAGE);
}
