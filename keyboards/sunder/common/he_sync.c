// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "he_sync.h"
#include "quantum.h"
#include "transactions.h"
#include "split_util.h"

void sync_he_settings(void) {
    for (int row = 0; row < MATRIX_ROWS; row++) {
        for (int col = 0; col < MATRIX_COLS; col++) {
            key_config_t *config = &user_config.key_config[row][col]; 
            uint16_t actuation_point = config->actuation_point;
            uint8_t mode = config->mode;
            uint8_t key_config[] = {
                row,
                col,
                (actuation_point >> 8) & 0xFF,
                actuation_point & 0xFF,
                mode
            };
            transaction_rpc_send(HE_CONFIG_SYNC, sizeof(key_config), key_config);
        }
    }
    uint16_t value_td = user_config.travel_distance;
    uint16_t value_sens = user_config.sensitivity;
    uint8_t user_config_td[] = {
        0,
        (value_td >> 8) & 0xFF,
        value_td & 0xFF
    };
    transaction_rpc_send(HE_CONFIG_SYNC, sizeof(user_config_td), user_config_td);
    uint8_t user_config_sens[] = {
        1,
        (value_sens >> 8) & 0xFF,
        value_sens & 0xFF
    };
    transaction_rpc_send(HE_CONFIG_SYNC, sizeof(user_config_sens), user_config_sens);     
}

void he_sync_slave(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    const uint8_t* data = (const uint8_t*)in_data;

    if (in_buflen == 5) {
        uint8_t row = data[0];
        uint8_t col = data[1];
        key_config_t key = {
            .actuation_point = ((uint16_t)data[2] << 8) | data[3],
            .mode = data[4]
        };
        dynamic_keymap_set_hall_effect_key_config(row, col, &key);
    } else if (in_buflen == 3) {
        uint8_t index = data[0];
        uint16_t value = ((uint16_t)data[1] << 8) | data[2];
        dynamic_keymap_set_hall_effect_user_config(index, &value);
    }
}

void keyboard_post_init_kb(void) {
    transaction_register_rpc(HE_CONFIG_SYNC, he_sync_slave);
    keyboard_post_init_user();
}

void housekeeping_task_kb(void) {
    if (is_keyboard_master()) {
        static uint32_t connect_time = 0;
        static bool sync_connection = false;
        static bool synced = false;

        if (is_transport_connected()) {
            sync_connection = true;
            if (!synced) {
                if (connect_time == 0) {
                    connect_time = timer_read32();
                }
                if (timer_elapsed32(connect_time) > 2000) {
                    sync_he_settings();
                    synced = true;
                }
            }
        } else if (sync_connection) {
            connect_time = 0;
            sync_connection = false;
            synced = false;
        }
    }
}