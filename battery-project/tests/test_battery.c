// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the actual UART parser, not a second implementation of it.
#include "battery_monitor.h"
#include "module.h"
#include "smsg.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t now;
static uint8_t rx[64];
static unsigned rx_pos, rx_len, acks;
static smsg_states_t smsg_state;

uint32_t timer_read32(void) { return now; }
uint32_t sync_timer_read32(void) { return now; }
uint32_t sync_timer_elapsed32(uint32_t since) { return now - since; }
void uart_init(uint32_t baud) { assert(baud == 115200); }
bool uart_available(void) { return rx_pos < rx_len; }
uint8_t uart_read(void) { assert(uart_available()); return rx[rx_pos++]; }
void uart_transmit(const uint8_t *data, uint16_t length) {
    assert(length == 3 && data[0] == 0x61 && data[1] == 0x0d && data[2] == 0x0a);
    acks++;
}
void smsg_init(void) { smsg_state = smsg_state_free; }
bool smsg_push(uint8_t *data, uint32_t size) { (void)data; (void)size; return true; }
uint32_t smsg_peek(uint8_t *data) { (void)data; return 0; }
void smsg_pop(void) {}
smsg_states_t smsg_get_state(void) { return smsg_state; }
void smsg_set_state(smsg_states_t state) { smsg_state = state; }
bool smsg_is_busy(void) { return smsg_state == smsg_state_busy; }

static void packet(uint8_t level, bool valid_checksum) {
    rx[0] = MD_REV_CMD_BATVOL;
    rx[1] = level;
    rx[2] = (uint8_t)(rx[0] + level + (valid_checksum ? 0 : 1));
    rx_pos = 0;
    rx_len = 3;
    md_main_task();
    assert(rx_pos == rx_len);
}

static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int main(void) {
    md_init();
    assert(*md_getp_bat() == 255); // Startup must not report the old default 100%.
    assert(bridge_battery_state(&bridge_battery, now) == BRIDGE_BATTERY_UNKNOWN);
    assert(bridge_battery_age(&bridge_battery, now) == UINT32_MAX);

    now = 100;
    packet(71, false);
    assert(acks == 0 && !bridge_battery.has_sample);
    packet(255, true);
    assert(bridge_battery.rejected == 1 && !bridge_battery.has_sample);
    assert(*md_getp_bat() == 255);
    packet(71, true);
    assert(bridge_battery.percent == 71 && *md_getp_bat() == 71);
    assert(bridge_battery.band == BRIDGE_BATTERY_BLUE);
    assert(bridge_battery_state(&bridge_battery, now + 14999) == BRIDGE_BATTERY_FRESH);
    assert(bridge_battery_state(&bridge_battery, now + 15000) == BRIDGE_BATTERY_STALE);

    now = 5000;
    packet(71, true); // An unchanged value is still a real new packet.
    assert(bridge_battery.received_at == now);
    now = 6000;
    packet(101, true); // Invalid values must not extend freshness or alter cache.
    assert(bridge_battery.received_at == 5000 && *md_getp_bat() == 71);
    bridge_battery_invalidate(&bridge_battery);
    assert(bridge_battery_state(&bridge_battery, now) == BRIDGE_BATTERY_STALE);
    packet(71, true);
    assert(bridge_battery_state(&bridge_battery, now) == BRIDGE_BATTERY_FRESH);

    // All three indicator boundaries: down immediately, up after hysteresis.
    const uint8_t thresholds[] = {10, 50, 90};
    for (unsigned i = 0; i < 3; i++) {
        packet(thresholds[i], true);
        assert(bridge_battery.band == (bridge_battery_band_t)(i + 1));
        for (int repeat = 0; repeat < 20; repeat++) {
            packet(thresholds[i] + 1, true);
            assert(bridge_battery.percent == thresholds[i] + 1);
            assert(bridge_battery.band == (bridge_battery_band_t)(i + 1));
            packet(thresholds[i], true);
        }
        packet(thresholds[i] + 3, true);
        assert(bridge_battery.band == (bridge_battery_band_t)(i + 2));
    }
    packet(0, true);
    assert(bridge_battery.percent == 0 && bridge_battery.band == BRIDGE_BATTERY_LOW);
    assert(bridge_battery_state(&bridge_battery, now) == BRIDGE_BATTERY_FRESH);

    // Timestamp zero and 32-bit timer rollover are both legitimate.
    bridge_battery_reset(&bridge_battery);
    now = UINT32_MAX - 9;
    packet(50, true);
    now = 10;
    assert(bridge_battery_age(&bridge_battery, now) == 20);
    assert(bridge_battery_state(&bridge_battery, now) == BRIDGE_BATTERY_FRESH);
    now = 0;
    packet(90, true);
    assert(bridge_battery_state(&bridge_battery, 0) == BRIDGE_BATTERY_FRESH);

    uint8_t data[32] = {0x42, 'B', 'R', '7', '5'};
    assert(!bridge_battery_packet(&bridge_battery, 25, 3, 6, data, 31));
    assert(bridge_battery_packet(&bridge_battery, 25, 3, 6, data, 32));
    assert(data[5] == 1 && data[6] == BRIDGE_BATTERY_FRESH && data[7] == 90);
    assert(data[9] == 3 && data[10] == 6 && u32(data + 11) == 25);
    assert(u32(data + 15) == 2 && u32(data + 19) == 0);
    data[0] = 1; // Normal VIA commands must pass through untouched.
    assert(!bridge_battery_packet(&bridge_battery, 25, 0, 0, data, 32));

    // Fragmented UART packets establish freshness only after checksum arrives.
    bridge_battery_reset(&bridge_battery);
    rx[0] = MD_REV_CMD_BATVOL; rx[1] = 42; rx[2] = MD_REV_CMD_BATVOL + 42;
    rx_pos = 0; rx_len = 1; md_main_task();
    assert(!bridge_battery.has_sample);
    rx_len = 2; md_main_task();
    assert(!bridge_battery.has_sample);
    rx_len = 3; md_main_task();
    assert(bridge_battery.percent == 42);

    // Check the exact rendering function linked into the firmware for all 101
    // percentages. At 50%, six full segments; at 100%, all twelve.
    uint8_t rgb[3];
    for (uint8_t percent = 1; percent <= 100; percent++) {
        packet(percent, true);
        unsigned lit = 0;
        for (uint8_t segment = 0; segment < 12; segment++) {
            bridge_battery_gauge(&bridge_battery, now, segment, rgb);
            if (rgb[0] || rgb[1] || rgb[2]) lit++;
            assert(rgb[2] == 0);
            if (segment * 100 >= percent * 12) assert(rgb[0] == 0 && rgb[1] == 0);
        }
        assert(lit == (unsigned)(percent * 12 + 99) / 100);
    }
    bridge_battery_gauge(&bridge_battery, now, 11, rgb);
    assert(rgb[0] == 0 && rgb[1] == 200); // Full green F12.
    packet(50, true);
    bridge_battery_gauge(&bridge_battery, now, 5, rgb);
    assert(rgb[0] == 200 && rgb[1] == 181);
    bridge_battery_gauge(&bridge_battery, now, 6, rgb);
    assert(rgb[0] == 0 && rgb[1] == 0);
    packet(51, true);
    bridge_battery_gauge(&bridge_battery, now, 6, rgb);
    assert(rgb[0] > 0 && rgb[0] < 30 && rgb[1] == 24); // 12% of F7.
    packet(0, true);
    bridge_battery_gauge(&bridge_battery, 0, 0, rgb);
    assert(rgb[0] == 160 && rgb[1] == 0);
    bridge_battery_gauge(&bridge_battery, 500, 0, rgb);
    assert(rgb[0] == 0);
    bridge_battery_invalidate(&bridge_battery);
    bridge_battery_gauge(&bridge_battery, now, 6, rgb);
    assert(rgb[0] == 8 && rgb[1] == 0 && rgb[2] == 12);
    bridge_battery_gauge(&bridge_battery, now, 12, rgb);
    assert(rgb[0] == 0 && rgb[1] == 0 && rgb[2] == 0);

    puts("PASS: UART parsing, checksum/range validation, startup, freshness, invalidation, zero, hysteresis, rollover, fragmentation, telemetry, and all 101 gauge levels");
}
