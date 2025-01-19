//
// Created by Михайло Грошевий on 18/01/2025.
//

#include "Battery.h"

#include "hardware/adc.h"

#define PIN_BATTERY 28

#define ADC_CHANNEL    2
#define REF_VOLTAGE    3.3
#define RESOLUTION     (1 << 12)
#define DIVIDER_RATION 5

Battery::Battery() {
    adc_init();
    adc_gpio_init(PIN_BATTERY);
}

float Battery::getVoltage() {
    adc_select_input(ADC_CHANNEL);

    float conversion_factor = REF_VOLTAGE / RESOLUTION * DIVIDER_RATION;
    return adc_read() * conversion_factor;
}
