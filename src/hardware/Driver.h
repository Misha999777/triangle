//
// Created by Михайло Грошевий on 18/01/2025.
//

#pragma once

typedef struct RP2350DriverParams {
    int slice[3];
    int chan[3];
    int wrap[3];
} RP2350DriverParams;

class Driver {
public:
    Driver();
    void writeDutyCycle(float dc_a, float dc_b, float dc_c);

private:
    void setupPWM(int pin, int index);
    void writeDutyCycle(float val, int slice, int chan, int wrap);

    RP2350DriverParams* params;
};
