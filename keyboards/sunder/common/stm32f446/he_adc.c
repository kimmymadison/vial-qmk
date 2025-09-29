// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "he_adc.h"

static const pin_t row_pins[ROWS_PER_HAND] = MATRIX_ROW_PINS;

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

void average_adc_buffer_nx4(uint16_t *output, const adcsample_t *buffer) {
    for (uint8_t ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
        uint16_t v0 = buffer[0 * ADC_NUM_CHANNELS + ch];
        uint16_t v1 = buffer[1 * ADC_NUM_CHANNELS + ch];
        uint16_t v2 = buffer[2 * ADC_NUM_CHANNELS + ch];
        uint16_t v3 = buffer[3 * ADC_NUM_CHANNELS + ch];

        uint16_t sum = v0 + v1 + v2 + v3;

        uint16_t minv = umin(umin(v0, v1), umin(v2, v3));
        uint16_t maxv = umax(umax(v0, v1), umax(v2, v3));

        output[ch] = (sum - minv - maxv + 1) >> 1;
    }
}

const ADCConversionGroup adcgrpcfg = {
    FALSE,
    ADC_NUM_CHANNELS,
    NULL,
    NULL,
    0,
    ADC_CR2_SWSTART,
    0,
    ADC_SMPR2_SMP_AN4(ADC_SAMPLE_15) |
    ADC_SMPR2_SMP_AN5(ADC_SAMPLE_15) |
    ADC_SMPR2_SMP_AN6(ADC_SAMPLE_15) |
    ADC_SMPR2_SMP_AN7(ADC_SAMPLE_15),
    0,
    0,
    ADC_SQR1_NUM_CH(ADC_NUM_CHANNELS),
    0,
    ADC_SQR3_SQ1_N(ADC_CHANNEL_IN4) |
    ADC_SQR3_SQ2_N(ADC_CHANNEL_IN5) |
    ADC_SQR3_SQ3_N(ADC_CHANNEL_IN6) |
    ADC_SQR3_SQ4_N(ADC_CHANNEL_IN7)
};

void adc_dma_init(void) {
    for (uint8_t r = 0; r < ROWS_PER_HAND; r++) {
        palSetLineMode(row_pins[r], PAL_MODE_INPUT_ANALOG);
    }

    adcStart(&ADCD1, NULL);

    // Enable Option 2 (mask flash noise during ADC sampling)
    SYSCFG->PMC |= SYSCFG_PMC_ADC1DC2;  // For ADC1

    adcConvert(&ADCD1, &adcgrpcfg, adc_buf, ADC_BUFFER_DEPTH);
}

void set_mux_pins(uint8_t col_index) {
    GPIOB->BSRR.W = ((col_index & 0x7) << 5) | ((0b111 << 5) << 16);
    wait_us(1);
}