//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "Driver.h"

#include <hardware/clocks.h>
#include <hardware/pwm.h>

#define PINS_PWM_1    10
#define PINS_PWM_2    11
#define PINS_PWM_3    12

#define PWM_FREQUENCY 24000
#define PWM_DIVIDER   16

Driver::Driver() {
    params = new RP2040DriverParams();

    setupPWM(PINS_PWM_1, 0);
    setupPWM(PINS_PWM_2, 1);
    setupPWM(PINS_PWM_3, 2);

    for (uint i = 0; i < NUM_PWM_SLICES; i++) {
        pwm_set_enabled(i, false);
        pwm_set_counter(i, 0);
    }

    pwm_set_mask_enabled(0xFF);
}

void Driver::writeDutyCycle(float dc_a, float dc_b, float dc_c) {
    writeDutyCycle(dc_a, params->slice[0], params->chan[0], params->wrap[0] + 1);
    writeDutyCycle(dc_b, params->slice[1], params->chan[1], params->wrap[1] + 1);
    writeDutyCycle(dc_c, params->slice[2], params->chan[2], params->wrap[2] + 1);
}

void Driver::setupPWM(uint pin, uint8_t index) {
    uint32_t div = PWM_DIVIDER;
    uint32_t sysclock_hz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000;
    uint32_t wrap = sysclock_hz * 8 / div / PWM_FREQUENCY - 1;

    gpio_set_function(pin, GPIO_FUNC_PWM);

    uint slice = pwm_gpio_to_slice_num(pin);
    uint chan = pwm_gpio_to_channel(pin);

    pwm_set_clkdiv_int_frac(slice, div>>4, div&0xF);
    pwm_set_phase_correct(slice, true);
    pwm_set_wrap(slice, wrap);
    pwm_set_chan_level(slice, chan, 0);

    params->wrap[index] = wrap;
    params->slice[index] = slice;
    params->chan[index] = chan;
}

void Driver::writeDutyCycle(float val, uint slice, uint chan, uint wrap) {
    pwm_set_chan_level(slice, chan, wrap * val);
}
