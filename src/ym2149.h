#pragma once

#include <stdbool.h>
#include <stdint.h>

#define YM2149_A_FDC_SIDE  0x01
#define YM2149_A_FDC0      0x02
#define YM2149_A_FDC1      0x04
#define YM2149_A_RS232_RTS 0x08
#define YM2149_A_RS232_DTR 0x10

void ym2149_init(void);
uint8_t ym2149_get_port_a(void);
uint8_t ym2149_read8(uint8_t addr);
void ym2149_write8(uint8_t addr, uint8_t value);