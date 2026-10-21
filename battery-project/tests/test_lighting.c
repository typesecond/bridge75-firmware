// SPDX-License-Identifier: GPL-2.0-or-later
#include "quantum.h"
#include "battery_monitor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void keyboard_post_init_user(void);
void housekeeping_task_user(void);
bool process_record_user(uint16_t keycode, keyrecord_t *record);
bool rgb_matrix_indicators_user(void);
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max);

layer_state_t default_layer_state = 1, layer_state;
led_config_t g_led_config;
static bool enabled = true, powered = true, suspended;
static uint8_t mode = 5, hue = 100, sat = 255, val = 150;
static unsigned lowpower, restarts, stabilizations, drawn;
static bool seen[90];
static uint8_t frame[90][3];

uint8_t get_highest_layer(layer_state_t state) {
    uint8_t result = 0;
    while (state >>= 1) result++;
    return result;
}
bool rgb_matrix_get_suspend_state(void) { return suspended; }
bool rgb_matrix_is_enabled(void) { return enabled; }
void rgb_matrix_disable_noeeprom(void) { enabled = false; restarts++; }
void rgb_matrix_enable_noeeprom(void) { enabled = true; restarts++; }
uint8_t rgb_matrix_get_mode(void) { return mode; }
void rgb_matrix_mode_noeeprom(uint8_t m) { mode = m; restarts++; }
uint8_t rgb_matrix_get_hue(void) { return hue; }
uint8_t rgb_matrix_get_sat(void) { return sat; }
uint8_t rgb_matrix_get_val(void) { return val; }
void rgb_matrix_sethsv_noeeprom(uint8_t h, uint8_t s, uint8_t v) { hue = h; sat = s; val = v; restarts++; }
void gpio_write_pin_high(uint8_t pin) { assert(pin == LED_POWER_EN_PIN); powered = false; }
void gpio_write_pin_low(uint8_t pin) { assert(pin == LED_POWER_EN_PIN); powered = true; }
bool gpio_read_pin(uint8_t pin) { assert(pin == LED_POWER_EN_PIN); return !powered; }
void wait_ms(uint16_t ms) { assert(ms == 10); stabilizations++; }
uint32_t timer_read32(void) { return 100; }
unsigned lpwr_get_state(void) { return lowpower; }
void rgb_matrix_set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    assert(index < 90);
    frame[index][0] = r; frame[index][1] = g; frame[index][2] = b;
    if (r || g || b) assert(index >= 1 && index <= 12);
    if (!seen[index]) { seen[index] = true; drawn++; }
}

int main(void) {
    for (unsigned row = 0; row < 6; row++)
        for (unsigned col = 0; col < 15; col++) g_led_config.matrix_co[row][col] = row * 15 + col;
    bridge_battery_receive(&bridge_battery, 50, 100);
    // A saved enabled rainbow configuration must not light at boot.
    keyboard_post_init_user();
    assert(!enabled && !powered);
    for (int i = 0; i < 100; i++) housekeeping_task_user();
    assert(!enabled && !powered);
    // Ordinary typing does not turn lights on; Fn+Z cannot enable a rainbow.
    keyrecord_t record = {true};
    assert(process_record_user(4, &record));
    assert(!process_record_user(0x7820, &record));
    assert(!process_record_user(0x7842, &record));

    layer_state = 1u << 1;
    housekeeping_task_user();
    assert(enabled && powered && mode == 1 && hue == 0 && sat == 0 && val == 0);
    assert(stabilizations == 1);
    unsigned prior_restarts = restarts;
    for (int i = 0; i < 100; i++) housekeeping_task_user();
    assert(restarts == prior_restarts); // Do not restart rendering every loop.
    assert(!rgb_matrix_indicators_user());
    memset(frame, 255, sizeof(frame)); // Simulate leftover ambient colors.
    assert(!rgb_matrix_indicators_advanced_user(0, 8));
    assert(drawn == 8);
    assert(frame[8][0] == 255); // Do not touch another render chunk.
    assert(!rgb_matrix_indicators_advanced_user(8, 90));
    assert(drawn == 90);
    for (unsigned index = 0; index < 90; index++) {
        bool lit = frame[index][0] || frame[index][1] || frame[index][2];
        assert(lit == (index >= 1 && index <= 6)); // 50% = F1 through F6.
    }
    // Resolve physical LEDs through the matrix, including missing LEDs.
    memset(seen, 0, sizeof(seen)); drawn = 0;
    g_led_config.matrix_co[0][1] = NO_LED;
    g_led_config.matrix_co[0][2] = 12;
    g_led_config.matrix_co[0][12] = 2;
    assert(!rgb_matrix_indicators_advanced_user(0, 90));
    assert(frame[1][0] == 0 && frame[2][0] == 0);
    assert(frame[12][0] == 200 && frame[12][1] == 36);
    for (unsigned col = 1; col <= 12; col++) g_led_config.matrix_co[0][col] = col;

    layer_state = 0; // Releasing Fn cuts LED power in the next housekeeping call.
    housekeeping_task_user();
    assert(!enabled && !powered);
    memset(seen, 0, sizeof(seen)); drawn = 0;
    assert(!rgb_matrix_indicators_advanced_user(0, 90));
    assert(drawn == 0);

    // Both Mac Fn and Windows Fn work, including a temporary USB idle cutoff.
    default_layer_state = 1u << 2;
    layer_state = 1u << 3;
    housekeeping_task_user();
    assert(enabled && powered);
    enabled = false; powered = false;
    housekeeping_task_user();
    assert(enabled && powered);
    suspended = true;
    housekeeping_task_user();
    assert(!enabled && !powered);
    suspended = false;
    housekeeping_task_user();
    assert(enabled && powered);
    lowpower = 1;
    housekeeping_task_user();
    assert(!enabled && !powered);
    lowpower = 0; layer_state = 0;
    housekeeping_task_user();
    assert(!enabled && !powered);
    // Even an external RGB-enable request cannot leave the idle LEDs powered.
    enabled = true; powered = true;
    housekeeping_task_user();
    assert(!enabled && !powered);
    puts("PASS: actual Fn-only lighting hooks: boot, typing, Win/Mac Fn, release, row-only rendering, no render resets, suspend/wake, power cutoff, and RGB shortcuts");
}
