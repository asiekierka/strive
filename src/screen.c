#include "screen.h"
#include <string.h>

screen_t atari_screen;

void screen_init(void) {
    memset(&atari_screen, 0, sizeof(screen_t));
}

uint8_t screen_read8(uint8_t addr) {
    if ((addr & 0xE0) == 0x40) {
        addr -= 0x40;
        if (addr & 0x01) {
            return atari_screen.palette[addr >> 1];
        } else {
            return atari_screen.palette[addr >> 1] >> 8;
        }
    }

    switch (addr) {
        case 0x01:
            return atari_screen.base >> 16;
        case 0x03:
            return atari_screen.base >> 8;
        case 0x05:
            return atari_screen.counter >> 16;
        case 0x07:
            return atari_screen.counter >> 8;
        case 0x09:
            return atari_screen.counter;
        case 0x0A:
            return atari_screen.sync_mode;
        case 0x60:
            return atari_screen.resolution; 
        default:
            return 0;
    }
}

void screen_write8(uint8_t addr, uint8_t value) {
    if ((addr & 0xE0) == 0x40) {
        addr -= 0x40;
        uint16_t pal = atari_screen.palette[addr >> 1];
        if (addr & 0x01) {
            atari_screen.palette[addr >> 1] = (pal & 0xFF00) | value;
        } else {
            atari_screen.palette[addr >> 1] = (pal & 0x00FF) | (value << 8);
        }
        platform_gfx_on_palette_update();
        return;
    }

    switch (addr) {
        case 0x01:
            atari_screen.base = (atari_screen.base & 0x00FFFF) | (value << 16); break;
        case 0x03:
            atari_screen.base = (atari_screen.base & 0xFF00FF) | (value << 8); break;
        case 0x05:
            atari_screen.counter = (atari_screen.counter & 0x00FFFF) | (value << 16); break;
        case 0x07:
            atari_screen.counter = (atari_screen.counter & 0xFF00FF) | (value << 8); break;
        case 0x09:
            atari_screen.counter = (atari_screen.counter & 0xFFFF00) | value; break;
        case 0x0A:
            atari_screen.sync_mode = value; break;
        case 0x60:
            atari_screen.resolution = value; break;
    }
}