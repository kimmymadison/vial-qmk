// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "he_matrix.h"
#include "he_adc.h"

static const pin_t mux_pins[MUX_BITS] = ALL_MUX_PINS;

void init_mux_pins(void) {
    for (int i = 0; i < MUX_BITS; i++) {
        gpio_set_pin_output(mux_pins[i]);
        gpio_write_pin_low(mux_pins[i]);
    }
}

void initialise_hall_sensors(void) {
    wait_ms(100);
    for (uint8_t c = 0; c < MATRIX_COLS; c++) {
        set_mux_pins(c);

        adcConvert(&ADCD1, &adcgrpcfg, init_adc_buf, INIT_ADC_BUFFER_DEPTH);
        uint16_t adc_channel_avg[ADC_NUM_CHANNELS];
        average_adc_buffer(adc_channel_avg, init_adc_buf, INIT_ADC_BUFFER_DEPTH);
        
        for (uint8_t r = 0; r < ROWS_PER_HAND; r++) {
            if (is_keyboard_left()) {
                if ((r == 0 && (c == 0 || c == 7 || c == 12)) ||
                    (r == 1 && (c == 11 || c == 12)) ||
                    (r == 2 && c == 0)) {
                    continue;
                }
            } else if (c == 12 && (r == 0 || r == 1)) {
                continue;
            }

            uint16_t analog_value = adc_channel_avg[r];

            keys[r][c].dynamic_actuation = false;
            keys[r][c].curr_pos = 0;
            keys[r][c].prev_pos = 0;
            
            uint16_t offset = (analog_value + 100) / 200;

            keys[r][c].max_value = analog_value - offset;

            if (analog_value >= 2100) {
                keys[r][c].value_05 = ((uint32_t)analog_value * 98 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 95 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 92 + 50) / 100;
                keys[r][c].value_20 = ((uint32_t)analog_value * 87 + 50) / 100;
                keys[r][c].value_25 = ((uint32_t)analog_value * 81 + 50) / 100;
                keys[r][c].value_30 = ((uint32_t)analog_value * 72 + 50) / 100;
                keys[r][c].min_value = ((uint32_t)analog_value * 72 + 50) / 100 - offset;

            } else if (analog_value >= 2000) {
                keys[r][c].value_05 = ((uint32_t)analog_value * 98 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 95 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 91 + 50) / 100;
                keys[r][c].value_20 = ((uint32_t)analog_value * 86 + 50) / 100;
                keys[r][c].value_25 = ((uint32_t)analog_value * 78 + 50) / 100;
                keys[r][c].value_30 = ((uint32_t)analog_value * 68 + 50) / 100;
                keys[r][c].min_value = ((uint32_t)analog_value * 68 + 50) / 100 - offset;

            } else if (analog_value >= 1900) {
                keys[r][c].value_05 = ((uint32_t)analog_value * 98 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 95 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 90 + 50) / 100;
                keys[r][c].value_20 = ((uint32_t)analog_value * 85 + 50) / 100;
                keys[r][c].value_25 = ((uint32_t)analog_value * 77 + 50) / 100;
                keys[r][c].value_30 = ((uint32_t)analog_value * 66 + 50) / 100;
                keys[r][c].min_value = ((uint32_t)analog_value * 66 + 50) / 100 - offset;
                
            } else if (analog_value >= 1800) {
                keys[r][c].value_05 = ((uint32_t)analog_value * 98 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 94 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 90 + 50) / 100;
                keys[r][c].value_20 = ((uint32_t)analog_value * 84 + 50) / 100;
                keys[r][c].value_25 = ((uint32_t)analog_value * 75 + 50) / 100;
                keys[r][c].value_30 = ((uint32_t)analog_value * 64 + 50) / 100;
                keys[r][c].min_value = ((uint32_t)analog_value * 64 + 50) / 100 - offset;
                
            } else if (analog_value >= 1700) {
                keys[r][c].value_05 = ((uint32_t)analog_value * 98 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 94 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 89 + 50) / 100;
                keys[r][c].value_20 = ((uint32_t)analog_value * 84 + 50) / 100;
                keys[r][c].value_25 = ((uint32_t)analog_value * 75 + 50) / 100;
                keys[r][c].value_30 = ((uint32_t)analog_value * 66 + 50) / 100;
                keys[r][c].min_value = ((uint32_t)analog_value * 66 + 50) / 100 - offset;
                
            } else if (analog_value >= 1650) {
                keys[r][c].value_05 = ((uint32_t)analog_value * 97 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 94 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 89 + 50) / 100;
                keys[r][c].value_20 = ((uint32_t)analog_value * 83 + 50) / 100;
                keys[r][c].value_25 = ((uint32_t)analog_value * 74 + 50) / 100;
                keys[r][c].value_30 = ((uint32_t)analog_value * 68 + 50) / 100;
                keys[r][c].min_value = ((uint32_t)analog_value * 68 + 50) / 100 - offset;
                
            } else if (analog_value >= 1600) {
                keys[r][c].value_05 = ((uint32_t)analog_value * 98 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 94 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 90 + 50) / 100;
                keys[r][c].value_20 = ((uint32_t)analog_value * 83 + 50) / 100;
                keys[r][c].value_25 = ((uint32_t)analog_value * 75 + 50) / 100;
                keys[r][c].value_30 = ((uint32_t)analog_value * 70 + 50) / 100;
                keys[r][c].min_value = ((uint32_t)analog_value * 70 + 50) / 100 - offset;
                
            } else if (analog_value >= 1550) {
                keys[r][c].value_05 = ((uint32_t)analog_value * 98 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 94 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 89 + 50) / 100;
                keys[r][c].value_20 = ((uint32_t)analog_value * 82 + 50) / 100;
                keys[r][c].value_25 = ((uint32_t)analog_value * 75 + 50) / 100;
                keys[r][c].value_30 = ((uint32_t)analog_value * 71 + 50) / 100;
                keys[r][c].min_value = ((uint32_t)analog_value * 71 + 50) / 100 - offset;
                
            } else if (analog_value >= 1500) {
                keys[r][c].value_05 = ((uint32_t)analog_value * 98 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 94 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 89 + 50) / 100;
                keys[r][c].value_20 = ((uint32_t)analog_value * 82 + 50) / 100;
                keys[r][c].value_25 = ((uint32_t)analog_value * 75 + 50) / 100;
                keys[r][c].value_30 = 0;
                keys[r][c].min_value = ((uint32_t)analog_value * 74 + 50) / 100;
                
            } else if (analog_value >= 1400) {
                keys[r][c].value_05 = ((uint32_t)analog_value * 98 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 94 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 89 + 50) / 100;
                keys[r][c].value_20 = ((uint32_t)analog_value * 82 + 50) / 100;
                keys[r][c].value_25 = 0;
                keys[r][c].value_30 = 0;
                keys[r][c].min_value = ((uint32_t)analog_value * 78 + 50) / 100;
                
            } else {
                keys[r][c].value_05 = ((uint32_t)analog_value * 98 + 50) / 100;
                keys[r][c].value_10 = ((uint32_t)analog_value * 94 + 50) / 100;
                keys[r][c].value_15 = ((uint32_t)analog_value * 89 + 50) / 100;
                keys[r][c].value_20 = 0;
                keys[r][c].value_25 = 0;
                keys[r][c].value_30 = 0;
                keys[r][c].min_value = ((uint32_t)analog_value * 85 + 50) / 100;
                
            }
        }
    }
}