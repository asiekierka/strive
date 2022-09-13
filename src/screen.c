#include <string.h>
#include "platform.h"
#include "screen.h"
#include "system.h"

screen_t atari_screen;

void screen_init(void) {
    memset(&atari_screen, 0, sizeof(screen_t));
}

static uint32_t screen_current_counter(void) {
    uint16_t current_y = system_line_y();
    uint32_t current_counter = atari_screen.counter;
    if (current_y < 200) current_counter += 160 * current_y;
    return current_counter;
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
            return screen_current_counter() >> 16;
        case 0x07:
            return screen_current_counter() >> 8;
        case 0x09:
            return screen_current_counter();
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
        platform_gfx_frame_draw_to(system_line_y());
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
            platform_gfx_frame_draw_to(system_line_y());
            atari_screen.base = (atari_screen.base & 0x00FFFF) | (value << 16); break;
        case 0x03:
            platform_gfx_frame_draw_to(system_line_y());
            atari_screen.base = (atari_screen.base & 0xFF00FF) | (value << 8); break;
        /* case 0x05:
            atari_screen.counter = (atari_screen.counter & 0x00FFFF) | (value << 16); break;
        case 0x07:
            atari_screen.counter = (atari_screen.counter & 0xFF00FF) | (value << 8); break;
        case 0x09:
            atari_screen.counter = (atari_screen.counter & 0xFFFF00) | value; break; */
        case 0x0A:
            atari_screen.sync_mode = value; break;
        case 0x60:
            atari_screen.resolution = value;
            platform_gfx_on_palette_update();
            break;
    }
}