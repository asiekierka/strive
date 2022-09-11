#include "../memory.h"
#include "nds/arm9/background.h"
#include "platform.h"
#include "platform_config.h"
#include "screen.h"
#include <stdint.h>
#include <stdio.h>
#include <nds.h>

static bool palette_changed = false;
NDS_DTCM_DATA
static const uint8_t color_map_st[16] = {
    0, 4, 9, 13, 18, 22, 27, 31,
    0, 4, 9, 13, 18, 22, 27, 31
};
NDS_DTCM_DATA
static const uint8_t color_map_ste[16] = {
    0, 4, 8, 12, 17, 21, 25, 29,
    2, 6, 10, 14, 19, 23, 27, 31
};

void _platform_gfx_init(void) {
    consoleDemoInit();

    videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE);
    vramSetBankE(VRAM_E_MAIN_BG);

    REG_BG2CNT = BG_BMP8_512x256 | BG_BMP_BASE(0);
    REG_BG2PA = -320;
    REG_BG2PB = 0;
    REG_BG2PC = 0;
    REG_BG2PD = 267;
    REG_BG2X = 320 << 8;
    REG_BG2Y = 0;
}

void _platform_gfx_exit(void) {

}

void platform_gfx_on_palette_update(void) {
    palette_changed = true;
}

NDS_ITCM_CODE
static void update_palette(void) {
    if (!palette_changed) return;

    // TODO: STE mode
    const uint8_t *color_map = color_map_st;

    if (atari_screen.resolution == 2) {
        uint16_t entry1 = atari_screen.palette[0];
        uint16_t entry2 = atari_screen.palette[1];

        uint8_t r1 = color_map[entry1 & 0xF];
        uint8_t g1 = color_map[entry1 & 0xF];
        uint8_t b1 = color_map[entry1 & 0xF];
        uint8_t r2 = color_map[entry2 & 0xF];
        uint8_t g2 = color_map[entry2 & 0xF];
        uint8_t b2 = color_map[entry2 & 0xF];

        for (int i = 0; i < 16; i++) {
            int set_bits = (i & 1) + (i & 2) + (i & 4) + (i & 8);
            uint8_t r = (r1 * (4 - set_bits)) + (r2 * set_bits);
            uint8_t g = (g1 * (4 - set_bits)) + (g2 * set_bits);
            uint8_t b = (b1 * (4 - set_bits)) + (b2 * set_bits);

            BG_PALETTE[i] = 0x8000 
                | ((r >> 2) << 10)
                | ((g >> 2) << 5)
                | ((b >> 2));
        }
    } else if (atari_screen.resolution == 1) {
        for (int i = 0; i < 16; i++) {
            uint16_t entry1 = atari_screen.palette[i >> 2];
            uint16_t entry2 = atari_screen.palette[i & 3];

            uint8_t r1 = color_map[entry1 & 0xF];
            uint8_t g1 = color_map[entry1 & 0xF];
            uint8_t b1 = color_map[entry1 & 0xF];
            uint8_t r2 = color_map[entry2 & 0xF];
            uint8_t g2 = color_map[entry2 & 0xF];
            uint8_t b2 = color_map[entry2 & 0xF];

            BG_PALETTE[i] = 0x8000 
                | (((r1 + r2) >> 1) << 10)
                | (((g1 + g2) >> 1) << 5)
                | (((b1 + b2) >> 1));
        }
    } else {
        for (int i = 0; i < 16; i++) {
            uint16_t entry = atari_screen.palette[i];
            BG_PALETTE[i] = 0x8000 
                | (color_map[entry & 0xF] << 10)
                | (color_map[(entry >> 4) & 0xF] << 5)
                | (color_map[(entry >> 8) & 0xF]);
        }
    }
    palette_changed = false;
}

/* #define COMBINE_4PX_ST_LOW() \
    ((bp & 0x8040201) | ((bp >> 7) & 0x02) \
        | ((bp >> 14) & 0x04) | ((bp >> 21) & 0x08) \
        | ((bp << 7) & 0x100) \
        | ((bp >> 7) & 0x400) | ((bp >> 14) & 0x800) \
        | ((bp << 14) & 0x10000) | ((bp << 7) & 0x20000) \
        | ((bp >> 7) & 0x80000) \
        | ((bp << 21) & 0x1000000) | ((bp << 14) & 0x2000000) \
        | ((bp << 7) & 0x4000000)) */
#define COMBINE_4PX_ST_LOW() ( \
        (bp & 0x8040201) \
        | ((bp >> 7) & 0x80402) \
        | ((bp >> 14) & 0x804) \
        | ((bp >> 21) & 0x8) \
        | ((bp << 7) & 0x4020100) \
        | ((bp << 14) & 0x2010000) \
        | ((bp << 21) & 0x1000000) \
    )
#define COMBINE_4PX_ST_HIGH() ( \
        (bp & 0x03) | ((bp >> 14) & 0x0C) \
        | ((bp << 6) & 0x300) | ((bp >> 8) & 0xC00) \
        | ((bp << 12) & 0x30000) | ((bp >> 2) & 0xC0000) \
        | ((bp << 18) & 0x3000000) | ((bp << 4) & 0xC000000) \
    )

NDS_ITCM_CODE
__attribute__((optimize("-O3")))
void platform_gfx_draw_frame(void) {
    uint32_t ticks = _TIMER_TICKS(0);

    update_palette();

    if (!video_memory_dirty[atari_screen.base >> 15] && !video_memory_dirty[(atari_screen.base >> 15) + 1])
        return;
    video_memory_dirty[atari_screen.base >> 15] = 0;
    video_memory_dirty[(atari_screen.base >> 15) + 1] = 0;

    uint8_t *src = memory_ram + (atari_screen.base & memory_ram_mask);
    uint32_t *dst = ((uint32_t*) BG_GFX);

    if (atari_screen.resolution <= 1) {
        // ST-LOW, ST-MED
        src -= 168;
        for (int y = 0; y < 200; y++) {
            src += 320;
            for (int x = 0; x < 320; x += 16, src -= 8, dst += 4) {
                uint32_t bp;
                bp = src[0] | (src[2] << 8) | (src[4] << 16) | (src[6]) << 24;
                dst[0] = COMBINE_4PX_ST_LOW();
                bp >>= 4;
                dst[1] = COMBINE_4PX_ST_LOW();
                bp = src[1] | (src[3] << 8) | (src[5] << 16) | (src[7]) << 24;
                dst[2] = COMBINE_4PX_ST_LOW();
                bp >>= 4;
                dst[3] = COMBINE_4PX_ST_LOW();
            }
            dst += (512 - 320) >> 2;
        }
    } else {
        // ST-HIGH
        src -= 162;
        for (int y = 0; y < 200; y++) {
            src += 240;
            for (int x = 0; x < 640; x += 16, src -= 2, dst += 2) {
                uint32_t bp;
                bp = src[0] | (src[1] << 8) | (src[80] << 16) | (src[81]) << 24;
                dst[0] = COMBINE_4PX_ST_HIGH();
                bp >>= 8;
                dst[1] = COMBINE_4PX_ST_HIGH();
            }
            dst += (512 - 320) >> 2;
        }
    }

    ticks = _TIMER_TICKS(0) - ticks;
     iprintf("%d ticks\n", ticks);
}