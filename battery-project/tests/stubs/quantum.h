#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define PRODUCT "Bridge75 test"
#define MANUFACTURER "Shortcut"
#define VENDOR_ID 0xfffe
#define PRODUCT_ID 0x003e
uint32_t timer_read32(void);
uint32_t sync_timer_read32(void);
uint32_t sync_timer_elapsed32(uint32_t since);
