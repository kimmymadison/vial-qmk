// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "matrix.h"
#include "quantum.h"

#include "he_adc.h"
#include "he_debug.h"
#include "he_keys.h"
#include "he_matrix.h"

#ifdef SPLIT_KEYBOARD
#    include "split_common/split_util.h"
#    include "split_common/transactions.h"

#    define ROWS_PER_HAND (MATRIX_ROWS / 2)
uint8_t thisHand, thatHand;
#else
#    define ROWS_PER_HAND (MATRIX_ROWS)
#endif

matrix_row_t matrix[MATRIX_ROWS];
analog_key_t keys[ROWS_PER_HAND][MATRIX_COLS];

#if defined(DEBUG_MATRIX_SCAN_RATE)
static uint32_t matrix_timer = 0;
#endif

#ifdef SPLIT_KEYBOARD
bool matrix_post_scan(void) {
    bool changed = false;
    if (is_keyboard_master()) {
        static bool  last_connected              = false;
        matrix_row_t slave_matrix[ROWS_PER_HAND] = {0};
        if (transport_master_if_connected(matrix + thisHand, slave_matrix)) {
            changed = memcmp(matrix + thatHand, slave_matrix, sizeof(slave_matrix)) != 0;

            last_connected = true;
        } else if (last_connected) {
            // reset other half when disconnected
            memset(slave_matrix, 0, sizeof(slave_matrix));
            changed = true;

            last_connected = false;
        }

        if (changed) memcpy(matrix + thatHand, slave_matrix, sizeof(slave_matrix));

        matrix_scan_kb();
    } else {
        transport_slave(matrix + thatHand, matrix + thisHand);

        matrix_slave_scan_kb();
    }

    return changed;
}

__attribute__((weak)) void matrix_slave_scan_kb(void) {
    matrix_slave_scan_user();
}
__attribute__((weak)) void matrix_slave_scan_user(void) {}
#endif

__attribute__((weak)) void matrix_init_kb(void) { matrix_init_user(); }

__attribute__((weak)) void matrix_scan_kb(void) { matrix_scan_user(); }

__attribute__((weak)) void matrix_init_user(void) {}

__attribute__((weak)) void matrix_scan_user(void) {}

matrix_row_t matrix_get_row(uint8_t row) {
    return matrix[row];
}

inline bool matrix_is_on(uint8_t row, uint8_t col) {
    return (matrix[row] & ((matrix_row_t)1 << col));
}

void matrix_print(void) {
    //
}

void bootmagic_scan(void) {
    uint8_t row = BOOTMAGIC_ROW;
    uint8_t col = BOOTMAGIC_COLUMN;

#if defined(SPLIT_KEYBOARD) && defined(BOOTMAGIC_ROW_RIGHT) && defined(BOOTMAGIC_COLUMN_RIGHT)
    if (!is_keyboard_left()) {
        row = BOOTMAGIC_ROW_RIGHT - thisHand;
        col = BOOTMAGIC_COLUMN_RIGHT;
    }
#endif
    set_mux_pins(col);
    adcConvert(&ADCD1, &adcgrpcfg, adc_buf, ADC_BUFFER_DEPTH);

    uint16_t analog_value = adc_buf[row];

    if (analog_value > 2450) {
        // Jump to bootloader.
        bootloader_jump();
    }
}

void matrix_init(void) {

#ifdef SPLIT_KEYBOARD
    thisHand = isLeftHand ? 0 : (ROWS_PER_HAND);
    thatHand = ROWS_PER_HAND - thisHand;
#endif

    memset(matrix, 0, sizeof(matrix));
    adc_dma_init();
    init_mux_pins();
    initialise_hall_sensors();

    // This *must* be called for correct keyboard behavior
    matrix_init_kb();
}

uint8_t matrix_scan(void) {

    matrix_row_t curr_matrix[ROWS_PER_HAND] = {0};

    #ifdef SPLIT_KEYBOARD
    memcpy(curr_matrix, matrix + thisHand, sizeof(matrix_row_t) * ROWS_PER_HAND);
    #else
    memcpy(curr_matrix, matrix, sizeof(matrix));
    #endif
    
    for (uint8_t col_index = 0; col_index < MATRIX_COLS; col_index++) {
        set_mux_pins(col_index);
        adcConvert(&ADCD1, &adcgrpcfg, adc_buf, ADC_BUFFER_DEPTH);
        
        for (uint8_t row_index = 0; row_index < ROWS_PER_HAND; row_index++) {
            analog_key_t *key = &keys[row_index][col_index];

            if (key->max_value == 0) {
                continue;
            }

            uint16_t analog_value = adc_buf[row_index];

            key_config_t *config = &user_config.key_config[row_index + thisHand][col_index]; 

            #if defined(DEBUG_MATRIX_SCAN_RATE)
            key->test_value = analog_value;
            #endif

            if (analog_value > key->max_value + MIN_MAX_BUFFER)
                key->max_value = analog_value;
            else
                analog_value = MIN(MAX(key->min_value, analog_value), key->max_value);

            // Calculate current position
            uint32_t curr_value = analog_value - key->min_value;
            uint16_t range = key->max_value - key->min_value;
            uint16_t norm_value = ((curr_value << 10) + (range >> 1)) / range;

            key->curr_pos = ((uint32_t)lut[norm_value] * user_config.travel_distance + 512) >> 10;

            // Update key states
            if (config->mode != 0) {
                // Rapid Trigger mode enabled
                if (key->dynamic_actuation) {
                    if (curr_matrix[row_index] & (1 << col_index)) {
                        // Key is 'pressed'
                        if (key->curr_pos > key->prev_pos) {
                            key->prev_pos = key->curr_pos;
                        } else if (key->curr_pos < key->prev_pos - user_config.sensitivity) {
                            curr_matrix[row_index] &= ~(1 << col_index);
                            key->prev_pos = key->curr_pos;
                        }
                    } else {
                        // Key is 'not pressed'
                        if (key->curr_pos < key->prev_pos) {
                            key->prev_pos = key->curr_pos;
                        } else if (key->curr_pos > key->prev_pos + user_config.sensitivity) {
                            curr_matrix[row_index] |= (1 << col_index);
                            key->prev_pos = key->curr_pos;
                        }
                    }
                    if ((config->mode == 1 && key->curr_pos <= config->actuation_point - 10) || (config->mode == 2 && key->curr_pos == 0)) {
                        // Key is above reset point
                        curr_matrix[row_index] &= ~(1 << col_index);
                        key->prev_pos = key->curr_pos; //
                        key->dynamic_actuation = false;
                    }
                } else if (key->curr_pos > config->actuation_point) {
                    curr_matrix[row_index] |= (1 << col_index);
                    key->prev_pos = key->curr_pos; //
                    key->dynamic_actuation = true;
                }
            } else {
                // Rapid Trigger mode disabled
                if (curr_matrix[row_index] & (1 << col_index)) {
                    // Key is 'pressed'
                    if (key->curr_pos <= config->actuation_point - 10) {
                        curr_matrix[row_index] &= ~(1 << col_index);
                    }
                } else {
                    // Key is 'not pressed'
                    if (key->curr_pos > config->actuation_point) {
                        curr_matrix[row_index] |= (1 << col_index);
                    }
                }
            }
        }
    }
    
    #if defined(DEBUG_MATRIX_SCAN_RATE)
    uint32_t timer_now = timer_read32();
    
    if (TIMER_DIFF_32(timer_now, matrix_timer) >= 500) {
        uprintf("matrix scan rate: %lu\n", get_matrix_scan_rate());

        if (is_keyboard_left()) {
            print_key_matrix(left_print_index, left_print_index_len);
        } else {
            print_key_matrix(right_print_index, right_print_index_len);
        }
        
        matrix_timer = timer_now;
    }
    #endif

    bool changed;

#ifdef SPLIT_KEYBOARD
    changed = memcmp(matrix + thisHand, curr_matrix, sizeof(curr_matrix)) != 0;
    if (changed) memcpy(matrix + thisHand, curr_matrix, sizeof(curr_matrix));
    matrix_post_scan();
#else
    changed = memcmp(matrix, curr_matrix, sizeof(curr_matrix)) != 0;
    if (changed) memcpy(matrix, curr_matrix, sizeof(curr_matrix));
    // This *must* be called for correct keyboard behavior
    matrix_scan_kb();
#endif

    return changed;
}