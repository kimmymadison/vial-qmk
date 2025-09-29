// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "he_keys.h"
#include "quantum.h"
#include "transactions.h"
#include "split_util.h"

user_config_t user_config = {
    .travel_distance = TRAVEL_DISTANCE,
    .sensitivity = SENSITIVITY,
    .key_config = { [0 ... MATRIX_ROWS - 1][0 ... MATRIX_COLS - 1] = {ACTUATION_POINT, RAPID_TRIGGER_MODE} }
};