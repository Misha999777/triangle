//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "Controller.h"

#include <cmath>
#include <cstdio>

#include "../bluetooth/Communication.h"
#include "Storage.h"

#define MAXIMUM_VOLTAGE                5

#define ERROR_MINIMUM_VOLTAGE          10
#define ERROR_MAXIMUM_ANGLE_DIFFERENCE 15
#define ERROR_RELEASE_TIMEOUT          50

#define ANGLE_ADJUSTMENT_TIMEOUT       1000
#define ANGLE_ADJUSTMENT_THRESHOLD     5
#define ANGLE_ADJUSTMENT_STEP          0.05

#define NOTIFICATION_TIMEOUT           50

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

Controller::Controller() {
    float k1_flash = Storage::readAtIndex(3);
    if (!std::isnan(k1_flash)) k1 = k1_flash;

    float k2_flash = Storage::readAtIndex(4);
    if (!std::isnan(k2_flash)) k2 = k2_flash;

    float k3_flash = Storage::readAtIndex(5);
    if (!std::isnan(k3_flash)) k3 = k3_flash;

    float targetAngle_flash = Storage::readAtIndex(6);
    if (!std::isnan(targetAngle_flash)) targetAngle = targetAngle_flash;
}

float Controller::loop(float shaftVelocity, IMUData data, bool isVertical) {
    auto [angle, angularVelocity] = data;

    if (notificationCounter++ >= NOTIFICATION_TIMEOUT) {
        char buffer[21];
        snprintf(buffer, sizeof(buffer), "T:%.2f C:%.2f", targetAngle, angle);
        Communication::sendNotification(buffer);
        notificationCounter = 0;
    }

    float error = angle - targetAngle;
    isRunning = shouldRun(error, isVertical);
    if (isRunning) {
        adjustTargetAngle(shaftVelocity);

        return controller(error, angularVelocity, shaftVelocity);
    }

    return 0;
}

int Controller::handleCommand(const char* command) {
    if (strcmp(command, "save") == 0) {
        if (isRunning) {
            return 0x80;
        }
        Storage::writeAtIndex(k1, 3);
        Storage::writeAtIndex(k2, 4);
        Storage::writeAtIndex(k3, 5);
        Storage::writeAtIndex(targetAngle, 6);
        return 0;
    }

    char name[16];
    float value;
    if (sscanf(command, "%15[^=]=%f", name, &value) == 2) {
        if (strcmp(name, "k1") == 0) k1 = value;
        else if (strcmp(name, "k2") == 0) k2 = value;
        else if (strcmp(name, "k3") == 0) k3 = value;
        else if (strcmp(name, "angle") == 0) targetAngle = value;
        else return 0x80;
        return 0;
    }
    
    return 0x80;
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