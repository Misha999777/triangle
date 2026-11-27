//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "Driver.h"

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

#define PINS_PWM_1    10
#define PINS_PWM_2    11
#define PINS_PWM_3    12

#define PWM_FREQUENCY 24000
#define PWM_DIVIDER   16

Driver::Driver() {
    phases[0] = setupPWM(PINS_PWM_1);
    phases[1] = setupPWM(PINS_PWM_2);
    phases[2] = setupPWM(PINS_PWM_3);

    for (int i = 0; i < NUM_PWM_SLICES; i++) {
        pwm_set_enabled(i, false);
        pwm_set_counter(i, 0);
    }

    pwm_set_mask_enabled(0xFF);
}

void Driver::writeDutyCycle(float dutyCycleA, float dutyCycleB, float dutyCycleC) {
    writeDutyCycle(phases[0], dutyCycleA);
    writeDutyCycle(phases[1], dutyCycleB);
    writeDutyCycle(phases[2], dutyCycleC);
}

Driver::Phase Driver::setupPWM(int pin) {
    int divider = PWM_DIVIDER;
    int systemClockHz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000;
    int wrap = systemClockHz / divider * 8 / PWM_FREQUENCY - 1;

    gpio_set_function(pin, GPIO_FUNC_PWM);

    int slice = pwm_gpio_to_slice_num(pin);
    int channel = pwm_gpio_to_channel(pin);

    pwm_set_clkdiv_int_frac(slice, divider >> 4, divider & 0xF);
    pwm_set_phase_correct(slice, true);
    pwm_set_wrap(slice, wrap);
    pwm_set_chan_level(slice, channel, 0);

    return {slice, channel, wrap};
}

void Driver::writeDutyCycle(const Phase& phase, float dutyCycle) {
    pwm_set_chan_level(phase.slice, phase.channel, (phase.wrap + 1) * dutyCycle);
}
