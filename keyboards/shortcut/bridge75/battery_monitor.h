// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BRIDGE_BATTERY_STALE_MS 15000u
#define BRIDGE_BATTERY_UNKNOWN_PERCENT 255u
#define BRIDGE_BATTERY_GAUGE_KEYS 12u

typedef enum { BRIDGE_BATTERY_UNKNOWN, BRIDGE_BATTERY_FRESH, BRIDGE_BATTERY_STALE } bridge_battery_state_t;
typedef enum { BRIDGE_BATTERY_NO_BAND, BRIDGE_BATTERY_LOW, BRIDGE_BATTERY_YELLOW, BRIDGE_BATTERY_BLUE, BRIDGE_BATTERY_GREEN } bridge_battery_band_t;

typedef struct {
    uint32_t received_at;
    uint32_t accepted;
    uint32_t rejected;
    uint8_t percent;
    bridge_battery_band_t band;
    bool has_sample;
    bool invalidated;
} bridge_battery_t;

extern bridge_battery_t bridge_battery;

void bridge_battery_reset(bridge_battery_t *battery);
void bridge_battery_invalidate(bridge_battery_t *battery);
bool bridge_battery_receive(bridge_battery_t *battery, uint8_t percent, uint32_t now);
bridge_battery_state_t bridge_battery_state(const bridge_battery_t *battery, uint32_t now);
uint32_t bridge_battery_age(const bridge_battery_t *battery, uint32_t now);
// RGB for one F1-F12 segment. The final segment is proportionally illuminated.
void bridge_battery_gauge(const bridge_battery_t *battery, uint32_t now, uint8_t segment, uint8_t rgb[3]);
// Read-only diagnostic packet. Returns false for unrelated/short requests.
bool bridge_battery_packet(const bridge_battery_t *battery, uint32_t now, uint8_t power_flags, uint8_t device, uint8_t *data, uint8_t length);
