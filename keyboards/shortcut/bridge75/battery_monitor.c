// SPDX-License-Identifier: GPL-2.0-or-later
#include "battery_monitor.h"
#include <string.h>

bridge_battery_t bridge_battery = {.percent = BRIDGE_BATTERY_UNKNOWN_PERCENT};

void bridge_battery_reset(bridge_battery_t *battery) {
    memset(battery, 0, sizeof(*battery));
    battery->percent = BRIDGE_BATTERY_UNKNOWN_PERCENT;
}

void bridge_battery_invalidate(bridge_battery_t *battery) {
    battery->invalidated = true;
}

uint32_t bridge_battery_age(const bridge_battery_t *battery, uint32_t now) {
    return battery->has_sample ? now - battery->received_at : UINT32_MAX;
}

bridge_battery_state_t bridge_battery_state(const bridge_battery_t *battery, uint32_t now) {
    if (!battery->has_sample) return BRIDGE_BATTERY_UNKNOWN;
    if (battery->invalidated || bridge_battery_age(battery, now) >= BRIDGE_BATTERY_STALE_MS) return BRIDGE_BATTERY_STALE;
    return BRIDGE_BATTERY_FRESH;
}

static bridge_battery_band_t band_for(uint8_t percent) {
    if (percent <= 10) return BRIDGE_BATTERY_LOW;
    if (percent <= 50) return BRIDGE_BATTERY_YELLOW;
    if (percent <= 90) return BRIDGE_BATTERY_BLUE;
    return BRIDGE_BATTERY_GREEN;
}

bool bridge_battery_receive(bridge_battery_t *battery, uint8_t percent, uint32_t now) {
    if (percent > 100) {
        battery->rejected++;
        return false;
    }
    bridge_battery_band_t band = band_for(percent);
    // Downgrades are immediate. Require three points above a boundary before
    // upgrading a fresh indicator, so 10/11, 50/51, 90/91 noise cannot flicker.
    // The raw percentage is never smoothed, clamped, or made monotonic.
    if (bridge_battery_state(battery, now) == BRIDGE_BATTERY_FRESH && band > battery->band) {
        static const uint8_t upgrade_at[] = {0, 13, 53, 93};
        if (percent < upgrade_at[band - 1]) band = band - 1;
    }
    battery->band = band;
    battery->percent = percent;
    battery->received_at = now;
    battery->has_sample = true;
    battery->invalidated = false;
    battery->accepted++;
    return true;
}

void bridge_battery_gauge(const bridge_battery_t *battery, uint32_t now, uint8_t segment, uint8_t rgb[3]) {
    memset(rgb, 0, 3);
    if (segment >= BRIDGE_BATTERY_GAUGE_KEYS) return;
    if (bridge_battery_state(battery, now) != BRIDGE_BATTERY_FRESH) {
        rgb[0] = 8;
        rgb[2] = 12; // Dim purple means unavailable, never a full battery.
        return;
    }
    if (battery->percent == 0) {
        if (segment == 0 && (now / 500) % 2 == 0) rgb[0] = 160;
        return;
    }
    int16_t fill = battery->percent * BRIDGE_BATTERY_GAUGE_KEYS - segment * 100;
    if (fill <= 0) return;
    if (fill > 100) fill = 100;
    // Fixed red -> orange -> yellow -> green scale, using integer math only.
    uint16_t position = segment * 400 / (BRIDGE_BATTERY_GAUGE_KEYS - 1);
    rgb[0] = (position <= 200 ? 200 : 400 - position) * fill / 100;
    rgb[1] = (position <= 200 ? position : 200) * fill / 100;
}

static void put_u32(uint8_t *data, uint32_t value) {
    for (uint8_t i = 0; i < 4; i++) data[i] = value >> (8 * i);
}

bool bridge_battery_packet(const bridge_battery_t *battery, uint32_t now, uint8_t power_flags, uint8_t device, uint8_t *data, uint8_t length) {
    static const uint8_t signature[] = {0x42, 'B', 'R', '7', '5'};
    if (length != 32 || memcmp(data, signature, sizeof(signature)) != 0) return false;
    memset(data + 5, 0, length - 5);
    data[5] = 1; // Protocol version
    data[6] = bridge_battery_state(battery, now);
    data[7] = battery->percent;
    data[8] = battery->band;
    data[9] = power_flags; // Bit 0: USB cable present; bit 1: charging pin active
    data[10] = device;
    put_u32(data + 11, bridge_battery_age(battery, now));
    put_u32(data + 15, battery->accepted);
    put_u32(data + 19, battery->rejected);
    return true;
}
