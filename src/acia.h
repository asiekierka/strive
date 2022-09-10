#pragma once

#include <stdbool.h>
#include <stdint.h>

uint8_t acia_read8(uint8_t addr);
void acia_write8(uint8_t addr, uint8_t value);
