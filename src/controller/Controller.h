//
// Created by Михайло Грошевий on 18/01/2025.
//

#pragma once

#include "../hardware/AngleSensor.h"

#define STARTING_ANGLE 92.5f
#define K1             10.0f
#define K2             0.3f
#define K3             0.1f

struct Parameters {
  float k1;
  float k2;
  float k3;
  float targetAngle;
};

class Controller {
public:
  Controller();
  float loop(float shaftVelocity, IMUData data, bool isVertical);

  Parameters getParameters() const;
  bool setParameters(const Parameters& parameters);
  bool saveParameters();

  bool isBalancing() const;
  float getTargetAngle() const;

private:
  bool isRunning = false;
  bool shouldRun(float error, bool isVertical);
  float controller(float angle, float velocity, float shaftVelocity);
  void adjustTargetAngle(float shaftVelocity);

  float averageVelocity = 0;
  int averageVelocityCounter = 0;

  int errorCounter = 0;

  float targetAngle = STARTING_ANGLE;
  float k1 = K1;
  float k2 = K2;
  float k3 = K3;
};
