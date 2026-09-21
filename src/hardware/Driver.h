//
// Created by Михайло Грошевий on 18/01/2025.
//

#ifndef PWM_H
#define PWM_H

#include "pico/stdlib.h"

typedef struct RP2350DriverParams {
    uint slice[3];
    uint chan[3];
    uint wrap[3];
} RP2350DriverParams;

class Driver {
public:
    Driver();
    void writeDutyCycle(float dc_a, float dc_b, float dc_c);

private:
    void setupPWM(uint pin, uint8_t index);
    void writeDutyCycle(float val, uint slice, uint chan, uint wrap);

    RP2350DriverParams* params;
};

#endif //PWM_H
