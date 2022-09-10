#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct acia {
    uint8_t ikbd_control, ikbd_data;
    uint8_t midi_control, midi_data;
} acia_t;

extern acia_t atari_acia;

void acia_init(void);
uint8_t acia_read8(uint8_t addr);
void acia_write8(uint8_t addr, uint8_t value);
