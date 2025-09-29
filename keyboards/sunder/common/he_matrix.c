// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "he_matrix.h"
#include "he_adc.h"
#include "lut.h"

static const pin_t mux_pins[MUX_BITS] = ALL_MUX_PINS;

uint16_t *lut;

void init_mux_pins(void) {
    for (int i = 0; i < MUX_BITS; i++) {
        gpio_set_pin_output(mux_pins[i]);
        gpio_write_pin_low(mux_pins[i]);
    }
}

void initialise_hall_sensors(void) {
    wait_ms(100);
    uint8_t offset_multiplier;

    if (user_config.travel_distance == 320) {
        lut = lut_320;
        offset_multiplier = 12;
    } else if (user_config.travel_distance == 380) {
        lut = lut_380;
        offset_multiplier = 15;
    } else if (user_config.travel_distance == 390) {
        lut = lut_390;
        offset_multiplier = 18;
    } else {
        lut = lut_350;
        offset_multiplier = 15;
    }

    for (uint8_t c = 0; c < MATRIX_COLS; c++) {
        set_mux_pins(c);

        adcConvert(&ADCD1, &adcgrpcfg, init_adc_buf, INIT_ADC_BUFFER_DEPTH);
        uint16_t adc_channel_avg[ADC_NUM_CHANNELS];
        average_adc_buffer(adc_channel_avg, init_adc_buf, INIT_ADC_BUFFER_DEPTH);
        
        for (uint8_t r = 0; r < ROWS_PER_HAND; r++) {

            uint16_t analog_value = adc_channel_avg[r];

            if (analog_value < 100) {
                continue;
            }
            
            keys[r][c].dynamic_actuation = false;
            keys[r][c].curr_pos = 0;
            keys[r][c].prev_pos = 0;
            
            uint16_t offset = (analog_value + 50) / 100 * offset_multiplier; 

            keys[r][c].max_value = analog_value + offset;
            keys[r][c].min_value = analog_value + 1;
        }
    }
}