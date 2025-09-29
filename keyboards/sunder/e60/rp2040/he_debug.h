// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

extern const uint8_t left_print_index[][3];
extern const size_t left_print_index_len;

extern const uint8_t right_print_index[][3];
extern const size_t right_print_index_len;

void print_key_matrix(const uint8_t keys_arr[][3], size_t len);