// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

#ifdef SPLIT_KEYBOARD
#    define ROWS_PER_HAND (MATRIX_ROWS / 2)
#else
#    define ROWS_PER_HAND (MATRIX_ROWS)
#endif

void init_mux_pins(void);

void initialise_hall_sensors(void);