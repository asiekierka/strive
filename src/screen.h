#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct screen {
    uint32_t base;
    uint32_t counter;
    uint16_t palette[16];
    uint8_t sync_mode;
    uint8_t resolution;
} screen_t;

extern screen_t atari_screen;

void screen_init(void);
uint8_t screen_read8(uint8_t addr);
void screen_write8(uint8_t addr, uint8_t value);