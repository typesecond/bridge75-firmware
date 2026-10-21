#pragma once
#include <stdbool.h>
#include <stdint.h>
#define LED_POWER_EN_PIN 7
#define RGB_MATRIX_SOLID_COLOR 1
#define NO_LED 255
#define IS_UNDERGLOW_KEYCODE(c) ((c) >= 0x7820 && (c) <= 0x782a)
#define IS_RGB_KEYCODE(c) ((c) >= 0x782b && (c) <= 0x783f)
#define IS_RGB_MATRIX_KEYCODE(c) ((c) >= 0x7840 && (c) <= 0x784c)
typedef uint32_t layer_state_t;
typedef struct { bool pressed; } keyrecord_t;
typedef struct { uint8_t matrix_co[6][15]; } led_config_t;
extern layer_state_t default_layer_state, layer_state;
extern led_config_t g_led_config;
uint8_t get_highest_layer(layer_state_t state);
bool rgb_matrix_get_suspend_state(void);
bool rgb_matrix_is_enabled(void);
void rgb_matrix_disable_noeeprom(void);
void rgb_matrix_enable_noeeprom(void);
uint8_t rgb_matrix_get_mode(void);
void rgb_matrix_mode_noeeprom(uint8_t mode);
uint8_t rgb_matrix_get_hue(void);
uint8_t rgb_matrix_get_sat(void);
uint8_t rgb_matrix_get_val(void);
void rgb_matrix_sethsv_noeeprom(uint8_t h, uint8_t s, uint8_t v);
void rgb_matrix_set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void gpio_write_pin_high(uint8_t pin);
void gpio_write_pin_low(uint8_t pin);
bool gpio_read_pin(uint8_t pin);
void wait_ms(uint16_t ms);
uint32_t timer_read32(void);
