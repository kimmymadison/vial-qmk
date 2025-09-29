// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

#ifdef SPLIT_KEYBOARD
#    define ROWS_PER_HAND (MATRIX_ROWS / 2)
#else
#    define ROWS_PER_HAND (MATRIX_ROWS)
#endif

#define ADC_NUM_CHANNELS ROWS_PER_HAND
#define ADC_BUFFER_DEPTH 1
#define INIT_ADC_BUFFER_DEPTH 16

extern adcsample_t adc_buf[ADC_NUM_CHANNELS * ADC_BUFFER_DEPTH];
extern adcsample_t init_adc_buf[ADC_NUM_CHANNELS * INIT_ADC_BUFFER_DEPTH];

void average_adc_buffer(uint16_t *output, const adcsample_t *buffer, uint8_t buffer_depth);
void average_adc_buffer_nx4(uint16_t *output, const adcsample_t *buffer);

extern const ADCConversionGroup adcgrpcfg;

void adc_dma_init(void);

void set_mux_pins(uint8_t col_index);