// SPDX-License-Identifier: GPL-2.0-or-later
#include QMK_KEYBOARD_H
#include "battery_monitor.h"
#include "wireless.h"

static bool gauge_requested(void) {
    uint8_t layer = get_highest_layer(default_layer_state | layer_state);
    return (layer == 1 || layer == 3) && !rgb_matrix_get_suspend_state() && lpwr_get_state() == LPWR_NORMAL;
}

static void update_gauge_power(void) {
    if (!gauge_requested()) {
        // Cut LED power as well as disabling rendering. Do not save temporary
        // Fn state to EEPROM, or wake the lights for ordinary typing.
        if (rgb_matrix_is_enabled()) rgb_matrix_disable_noeeprom();
        gpio_write_pin_high(LED_POWER_EN_PIN);
        return;
    }

    // The RGB renderer must be active for QMK to call the indicator hook.
    // A black solid-color background leaves only the gauge hook visible.
    if (!rgb_matrix_is_enabled()) rgb_matrix_enable_noeeprom();
    if (rgb_matrix_get_mode() != RGB_MATRIX_SOLID_COLOR) rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
    if (rgb_matrix_get_hue() || rgb_matrix_get_sat() || rgb_matrix_get_val()) rgb_matrix_sethsv_noeeprom(0, 0, 0);
    if (gpio_read_pin(LED_POWER_EN_PIN)) {
        gpio_write_pin_low(LED_POWER_EN_PIN);
        wait_ms(10); // Let the LED power rail stabilize before the next frame.
    }
}

void keyboard_post_init_user(void) {
    update_gauge_power();
}

void housekeeping_task_user(void) {
    update_gauge_power();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // This keymap uses lighting exclusively for the momentary battery gauge.
    // Suppress legacy RGB controls so they cannot enable ambient effects or
    // save temporary gauge settings into EEPROM while Fn is held.
    (void)record;
    return !(IS_UNDERGLOW_KEYCODE(keycode) || IS_RGB_KEYCODE(keycode) || IS_RGB_MATRIX_KEYCODE(keycode));
}

bool rgb_matrix_indicators_user(void) {
    return false; // Suppress the upstream Caps Lock light outside the gauge.
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (gauge_requested()) {
        // Clear this render chunk explicitly: the background effect respects
        // saved LED flags and may otherwise leave excluded LEDs unchanged.
        for (uint8_t index = led_min; index < led_max; index++) {
            rgb_matrix_set_color(index, 0, 0, 0);
        }
        for (uint8_t segment = 0; segment < BRIDGE_BATTERY_GAUGE_KEYS; segment++) {
            uint8_t index = g_led_config.matrix_co[0][segment + 1];
            if (index != NO_LED && index >= led_min && index < led_max) {
                uint8_t rgb[3];
                bridge_battery_gauge(&bridge_battery, timer_read32(), segment, rgb);
                rgb_matrix_set_color(index, rgb[0], rgb[1], rgb[2]);
            }
        }
    }
    return false; // Suppress the upstream colored Fn legend / connection LEDs.
}
