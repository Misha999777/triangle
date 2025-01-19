//
// Created by Михайло Грошевий on 18/01/2025.
//

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "../hardware/AngleSensor.h"

#define STARTING_ANGLE 90.8

class Controller {
public:
  float loop(float shaftVelocity, IMUData data, bool isVertical, float voltage);

private:
  bool shouldRun(float voltage, float error, bool isVertical);
  float controller(float angle, float velocity, float shaftVelocity);
  void adjustTargetAngle(float shaftVelocity);

  float targetAngle = STARTING_ANGLE;
  float averageVelocity = 0.0;
  int averageVelocityCounter = 0;

  bool wasBatteryDrained = false;
  int errorCounter = 0;
};

#endif //CONTROLLER_H
