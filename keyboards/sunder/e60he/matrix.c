#include "analog.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "matrix.h"
#include "quantum.h"
#include "hall_effect.h"
#include "hardware/gpio.h"

#ifdef SPLIT_KEYBOARD
#    include "split_common/split_util.h"
#    include "split_common/transactions.h"

#    define ROWS_PER_HAND (MATRIX_ROWS / 2)
uint8_t thisHand, thatHand;
#else
#    define ROWS_PER_HAND (MATRIX_ROWS)
#endif

static const pin_t row_pins[ROWS_PER_HAND] = MATRIX_ROW_PINS;
static const pin_t mux_pins[MUX_BITS] = ALL_MUX_PINS;

matrix_row_t matrix[MATRIX_ROWS];
analog_key_t keys[ROWS_PER_HAND][MATRIX_COLS];

uint16_t *lut;

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

#define ADC_NUM_CHANNELS 3
#define ADC_BUFFER_DEPTH 1
#define INIT_ADC_BUFFER_DEPTH 32

static adcsample_t adc_buf[ADC_NUM_CHANNELS * ADC_BUFFER_DEPTH];
static adcsample_t init_adc_buf[ADC_NUM_CHANNELS * INIT_ADC_BUFFER_DEPTH];

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

volatile uint32_t adc_cb_counter = 0;

volatile bool adc_conversion_done = false;

static void adc_end_callback(ADCDriver *adcp) {
    (void)adcp;
    adc_conversion_done = true;
}

static void adc_error_callback(ADCDriver *adcp, adcerror_t err) {
    (void)adcp;
    (void)err;
    uprintf("ADC error!\n");
}

static const ADCConversionGroup adcgrpcfg = {
    .circular     = false,
    .num_channels = ADC_NUM_CHANNELS,
    .end_cb       = adc_end_callback,
    .error_cb     = adc_error_callback,
    .channel_mask = RP_ADC_CH0 | RP_ADC_CH1 | RP_ADC_CH2,
};

void adc_dma_init(void) {
    for (uint8_t r = 0; r < ROWS_PER_HAND; r++) {
        palSetLineMode(row_pins[r], PAL_MODE_INPUT_ANALOG);
    }
    adcStop(&ADCD1);
    adcStart(&ADCD1, &adc_cfg);
    adcConvert(&ADCD1, &adcgrpcfg, adc_buf, ADC_BUFFER_DEPTH);
}

void init_mux_pins(void) {
    for (int i = 0; i < MUX_BITS; i++) {
        gpio_set_pin_output(mux_pins[i]);
        gpio_write_pin_low(mux_pins[i]);
    }
}

void set_mux_pins_batch(uint8_t col_index, const pin_t mux_pins[4]) {
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

void bootmagic_scan(void) {
    uint8_t row = BOOTMAGIC_ROW;
    uint8_t col = BOOTMAGIC_COLUMN;

#if defined(SPLIT_KEYBOARD) && defined(BOOTMAGIC_ROW_RIGHT) && defined(BOOTMAGIC_COLUMN_RIGHT)
    if (!is_keyboard_left()) {
        row = BOOTMAGIC_ROW_RIGHT - thisHand;
        col = BOOTMAGIC_COLUMN_RIGHT;
    }
#endif
    set_mux_pins_batch(col, mux_pins);
    adcConvert(&ADCD1, &adcgrpcfg, adc_buf, ADC_BUFFER_DEPTH);

    uint16_t adc_channel_avg[ADC_NUM_CHANNELS];
    average_adc_buffer(adc_channel_avg, adc_buf, ADC_BUFFER_DEPTH);

    uint16_t analog_value = adc_channel_avg[row];

    if (analog_value > 2450) {
        // Jump to bootloader.
        bootloader_jump();
    }
}

void initialise_hall_sensors(void) {
    uint8_t offset_multiplier;

    if (user_config.travel_distance == 320) {
        lut = lut_320;
        offset_multiplier = 12;
    } else {
        lut = lut_350;
        offset_multiplier = 15;
    }

    for (uint8_t c = 0; c < MATRIX_COLS; c++) {
        set_mux_pins_batch(c, mux_pins);

        adcConvert(&ADCD1, &adcgrpcfg, init_adc_buf, INIT_ADC_BUFFER_DEPTH);
        uint16_t adc_channel_avg[ADC_NUM_CHANNELS];
        average_adc_buffer(adc_channel_avg, init_adc_buf, INIT_ADC_BUFFER_DEPTH);
        
        for (uint8_t r = 0; r < ROWS_PER_HAND; r++) {
            uint16_t analog_value = adc_channel_avg[r];
            
            keys[r][c].dynamic_actuation = false;
            keys[r][c].curr_pos = 0;
            keys[r][c].prev_pos = 0;
            
            uint16_t offset = (analog_value + 50) / 100 * offset_multiplier; 

            keys[r][c].max_value = analog_value + offset;
            keys[r][c].min_value = analog_value + 1;
        }
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
        set_mux_pins_batch(col_index, mux_pins);

        adcConvert(&ADCD1, &adcgrpcfg, adc_buf, ADC_BUFFER_DEPTH);
        // uint16_t adc_channel_avg[ADC_NUM_CHANNELS];
        // average_adc_buffer(adc_channel_avg, adc_buf, ADC_BUFFER_DEPTH);
        
        for (uint8_t row_index = 0; row_index < ROWS_PER_HAND; row_index++) {
            analog_key_t *key = &keys[row_index][col_index];
            uint16_t analog_value = adc_buf[row_index];
            // uint16_t analog_value = adc_channel_avg[row_index];

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

        uprintf("(%u, %u, %u) ", keys[0][8].curr_pos, keys[0][8].min_value, keys[0][8].test_value);
        uprintf("(%u, %u, %u) ", keys[0][7].curr_pos, keys[0][7].min_value, keys[0][7].test_value);
        uprintf("(%u, %u, %u) ", keys[0][6].curr_pos, keys[0][6].min_value, keys[0][6].test_value);
        uprintf("(%u, %u, %u) ", keys[0][5].curr_pos, keys[0][5].min_value, keys[0][5].test_value);
        uprintf("(%u, %u, %u) ", keys[0][4].curr_pos, keys[0][4].min_value, keys[0][4].test_value);
        uprintf("(%u, %u, %u)\n", keys[0][3].curr_pos, keys[0][3].min_value, keys[0][3].test_value);

        uprintf("(%u, %u, %u) ", keys[0][9].curr_pos, keys[0][9].min_value, keys[0][9].test_value);
        uprintf("(%u, %u, %u) ", keys[0][0].curr_pos, keys[0][0].min_value, keys[0][0].test_value);
        uprintf("(%u, %u, %u) ", keys[0][1].curr_pos, keys[0][1].min_value, keys[0][1].test_value);
        uprintf("(%u, %u, %u) ", keys[0][2].curr_pos, keys[0][2].min_value, keys[0][2].test_value);
        uprintf("(%u, %u, %u) ", keys[1][4].curr_pos, keys[1][4].min_value, keys[1][4].test_value);
        uprintf("(%u, %u, %u)\n", keys[1][2].curr_pos, keys[1][2].min_value, keys[1][2].test_value);

        uprintf("(%u, %u, %u) ", keys[1][8].curr_pos, keys[1][8].min_value, keys[1][8].test_value);
        uprintf("(%u, %u, %u) ", keys[1][7].curr_pos, keys[1][7].min_value, keys[1][7].test_value);
        uprintf("(%u, %u, %u) ", keys[1][6].curr_pos, keys[1][6].min_value, keys[1][6].test_value);
        uprintf("(%u, %u, %u) ", keys[1][5].curr_pos, keys[1][5].min_value, keys[1][5].test_value);
        uprintf("(%u, %u, %u) ", keys[1][3].curr_pos, keys[1][3].min_value, keys[1][3].test_value);
        uprintf("(%u, %u, %u)\n", keys[1][1].curr_pos, keys[1][1].min_value, keys[1][1].test_value);

        uprintf("(%u, %u, %u) ", keys[1][9].curr_pos, keys[1][9].min_value, keys[1][9].test_value);
        uprintf("(%u, %u, %u) ", keys[1][0].curr_pos, keys[1][0].min_value, keys[1][0].test_value);
        uprintf("(%u, %u, %u) ", keys[2][7].curr_pos, keys[2][7].min_value, keys[2][7].test_value);
        uprintf("(%u, %u, %u) ", keys[2][6].curr_pos, keys[2][6].min_value, keys[2][6].test_value);
        uprintf("(%u, %u, %u) ", keys[2][5].curr_pos, keys[2][5].min_value, keys[2][5].test_value);
        uprintf("(%u, %u, %u)\n", keys[2][4].curr_pos, keys[2][4].min_value, keys[2][4].test_value);

        uprintf("(%u, %u, %u) ", keys[2][8].curr_pos, keys[2][8].min_value, keys[2][8].test_value);
        uprintf("(%u, %u, %u) ", keys[2][9].curr_pos, keys[2][9].min_value, keys[2][9].test_value);
        uprintf("(%u, %u, %u) ", keys[2][0].curr_pos, keys[2][0].min_value, keys[2][0].test_value);
        uprintf("(%u, %u, %u) ", keys[2][1].curr_pos, keys[2][1].min_value, keys[2][1].test_value);
        uprintf("(%u, %u, %u) ", keys[2][2].curr_pos, keys[2][2].min_value, keys[2][2].test_value);
        uprintf("(%u, %u, %u)\n\n", keys[2][3].curr_pos, keys[2][3].min_value, keys[2][3].test_value);

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