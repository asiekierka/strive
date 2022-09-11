#pragma once

#include <stdbool.h>
#include <stdint.h>

void ym2149_init(void);
uint8_t ym2149_read8(uint8_t addr);
void ym2149_write8(uint8_t addr, uint8_t value);