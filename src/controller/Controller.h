//
// Created by Михайло Грошевий on 18/01/2025.
//

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "../hardware/AngleSensor.h"

#define STARTING_ANGLE 93.5
#define K1             10.0
#define K2             0.3
#define K3             0.1

class Controller {
public:
  Controller();
  float loop(float shaftVelocity, IMUData data, bool isVertical);
  int handleCommand(const char* command);

private:
  bool isRunning = false;
  bool shouldRun(float error, bool isVertical);
  float controller(float angle, float velocity, float shaftVelocity);
  void adjustTargetAngle(float shaftVelocity);

  float averageVelocity = 0.0;
  int averageVelocityCounter = 0;

  int errorCounter = 0;

  float targetAngle = STARTING_ANGLE;
  float k1 = K1;
  float k2 = K2;
  float k3 = K3;

  int notificationCounter = 0;
};

#endif //CONTROLLER_H
