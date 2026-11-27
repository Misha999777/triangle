//
// Created by Михайло Грошевий on 18/01/2025.
//

#pragma once

class Driver {
public:
    Driver();
    void writeDutyCycle(float dutyCycleA, float dutyCycleB, float dutyCycleC);

private:
    struct Phase {
        int slice;
        int channel;
        int wrap;
    };

    Phase setupPWM(int pin);
    void writeDutyCycle(const Phase& phase, float dutyCycle);

    Phase phases[3] = {};
};
