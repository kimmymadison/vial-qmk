// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "he_adc.h"
#include "hardware/gpio.h"

static const pin_t row_pins[ROWS_PER_HAND] = MATRIX_ROW_PINS;
static const pin_t mux_pins[MUX_BITS] = ALL_MUX_PINS;

adcsample_t adc_buf[ADC_NUM_CHANNELS * ADC_BUFFER_DEPTH];
adcsample_t init_adc_buf[ADC_NUM_CHANNELS * INIT_ADC_BUFFER_DEPTH];

static inline uint16_t umin(uint16_t a, uint16_t b) { return a < b ? a : b; }
static inline uint16_t umax(uint16_t a, uint16_t b) { return a > b ? a : b; }

void average_adc_buffer(uint16_t *output, const adcsample_t *buffer, uint8_t buffer_depth) {
    for (uint8_t ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < buffer_depth; i++) {
            sum += buffer[i * ADC_NUM_CHANNELS + ch];
        }
        output[ch] = (sum + (buffer_depth >> 1)) >> __builtin_ctz(buffer_depth);
    }
}

const ADCConfig adc_cfg = {
    .div_int  = 0,
    .div_frac = 0,
    .shift    = false,
};

const ADCConversionGroup adcgrpcfg = {
    .circular     = false,
    .num_channels = ADC_NUM_CHANNELS,
    .end_cb       = NULL,
    .error_cb     = NULL,
    .channel_mask = RP_ADC_CH0 | RP_ADC_CH1 | RP_ADC_CH2,
};

void adc_dma_init(void) {
    for (uint8_t r = 0; r < ROWS_PER_HAND; r++) {
        palSetLineMode(row_pins[r], PAL_MODE_INPUT_ANALOG);
    }
    adcStart(&ADCD1, &adc_cfg);
    adcConvert(&ADCD1, &adcgrpcfg, adc_buf, ADC_BUFFER_DEPTH);
}

void set_mux_pins(uint8_t col_index) {
    uint32_t set_mask = 0;
    uint32_t clr_mask = 0;

    for (int i = 0; i < 4; i++) {
        if (col_index & (1 << i)) {
            set_mask |= (1u << mux_pins[i]);
        } else {
            clr_mask |= (1u << mux_pins[i]);
        }
    }
    // Clear LOWs first, then set HIGHs
    sio_hw->gpio_clr = clr_mask;
    sio_hw->gpio_set = set_mask;
}