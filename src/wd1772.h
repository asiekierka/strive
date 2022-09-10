#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct wd1772 {
    uint16_t dma_status;
    uint16_t dma_mode;
    uint32_t dta;
    uint8_t sector_count;
} wd1772_t;

extern wd1772_t atari_wd1772;

void wd1772_init(void);
uint8_t wd1772_read8(uint8_t addr);
void wd1772_write8(uint8_t addr, uint8_t value);
