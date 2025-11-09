#include QMK_KEYBOARD_H

// ---------- Layers (optional) ----------
enum layer_names {
    _BASE = 0,
    _GAME,
};

// ---------- SOCD state ----------
static bool socd_enabled = true;

// Track pressed state for Arrow cluster and WASD
static bool L_pressed = false, R_pressed = false, U_pressed = false, D_pressed = false;
static bool A_pressed = false, Dd_pressed = false, W_pressed = false, S_pressed = false;

// "last pressed" markers for each pair: 1 = left/up (or A/W), 2 = right/down (or D/S)
static uint8_t LR_last = 0;
static uint8_t UD_last = 0;
static uint8_t AD_last = 0;
static uint8_t WS_last = 0;

// Helper: apply Last-Input-Wins logic
static void apply_last_wins_pair(bool *a_down, bool *b_down, uint8_t *last,
                                 uint16_t kc_a, uint16_t kc_b) {
    if (!socd_enabled) {
        if (!*a_down) unregister_code(kc_a);
        if (!*b_down) unregister_code(kc_b);
        return;
    }

    if (*a_down && *b_down) {
        if (*last == 1) {
            register_code(kc_a);
            unregister_code(kc_b);
        } else if (*last == 2) {
            register_code(kc_b);
            unregister_code(kc_a);
        } else {
            unregister_code(kc_a);
            unregister_code(kc_b);
        }
    } else if (*a_down) {
        *last = 1;
        register_code(kc_a);
        unregister_code(kc_b);
    } else if (*b_down) {
        *last = 2;
        register_code(kc_b);
        unregister_code(kc_a);
    } else {
        unregister_code(kc_a);
        unregister_code(kc_b);
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // --- Toggle SOCD when '|' (KC_PIPE) is pressed ---
    if (keycode == KC_PIPE && record->event.pressed) {
        socd_enabled = !socd_enabled;
        return false;
    }

    // --- Intercept opposites we want to clean ---
    switch (keycode) {
        // Arrows
        case KC_LEFT:
            L_pressed = record->event.pressed;
            if (record->event.pressed) LR_last = 1;
            apply_last_wins_pair(&L_pressed, &R_pressed, &LR_last, KC_LEFT, KC_RIGHT);
            return false;

        case KC_RIGHT:
            R_pressed = record->event.pressed;
            if (record->event.pressed) LR_last = 2;
            apply_last_wins_pair(&L_pressed, &R_pressed, &LR_last, KC_LEFT, KC_RIGHT);
            return false;

        case KC_UP:
            U_pressed = record->event.pressed;
            if (record->event.pressed) UD_last = 1;
            apply_last_wins_pair(&U_pressed, &D_pressed, &UD_last, KC_UP, KC_DOWN);
            return false;

        case KC_DOWN:
            D_pressed = record->event.pressed;
            if (record->event.pressed) UD_last = 2;
            apply_last_wins_pair(&U_pressed, &D_pressed, &UD_last, KC_UP, KC_DOWN);
            return false;

        // WASD
        case KC_A:
            A_pressed = record->event.pressed;
            if (record->event.pressed) AD_last = 1;
            apply_last_wins_pair(&A_pressed, &Dd_pressed, &AD_last, KC_A, KC_D);
            return false;

        case KC_D:
            Dd_pressed = record->event.pressed;
            if (record->event.pressed) AD_last = 2;
            apply_last_wins_pair(&A_pressed, &Dd_pressed, &AD_last, KC_A, KC_D);
            return false;

        case KC_W:
            W_pressed = record->event.pressed;
            if (record->event.pressed) WS_last = 1;
            apply_last_wins_pair(&W_pressed, &S_pressed, &WS_last, KC_W, KC_S);
            return false;

        case KC_S:
            S_pressed = record->event.pressed;

