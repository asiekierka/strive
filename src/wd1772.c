#include <string.h>
#include "wd1772.h"

wd1772_t atari_wd1772;

void wd1772_init(void) {
    memset(&atari_wd1772, 0, sizeof(wd1772_t));
}

uint8_t wd1772_read8(uint8_t addr) {
    // TODO
    // debug_printf("todo: wd1772_read8 %02X\n", addr);
    switch (addr) {
    case 0x04:
        return 0xFF;
    case 0x05:
        return 0x00;
    case 0x06:
        return atari_wd1772.dma_status >> 8;
    case 0x07:
        return atari_wd1772.dma_status;
    case 0x09:
        return atari_wd1772.dta >> 16;
    case 0x0B:
        return atari_wd1772.dta >> 8;
    case 0x0D:
        return atari_wd1772.dta;
    }
}

void wd1772_write8(uint8_t addr, uint8_t value) {
    // TODO
    // debug_printf("todo: wd1772_write8 %02X = %02X\n", addr, value);
    switch (addr) {
    case 0x05:
        if (atari_wd1772.dma_mode & 0x10) {
            atari_wd1772.sector_count = value;
        } else {
            // TODO
            atari_wd1772.dma_status |= 0x01;
        }
        break;
    case 0x06:
        atari_wd1772.dma_mode = (atari_wd1772.dma_mode & 0x00FF) | (value << 8);
        break;
    case 0x07:
        atari_wd1772.dma_mode = (atari_wd1772.dma_mode & 0xFF00) | value;
        break;
    case 0x09:
        atari_wd1772.dta = (atari_wd1772.dta & 0x00FFFF) | ((value & 0x3F) << 16);
        break;
    case 0x0B:
        atari_wd1772.dta = (atari_wd1772.dta & 0xFF00FF) | (value << 8);
        break;
    case 0x0D:
        atari_wd1772.dta = (atari_wd1772.dta & 0xFFFF00) | (value & 0xFE);
        break;
    }
}
