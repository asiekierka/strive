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
static const uint8_t color_map_st[8] = {
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

    for (int i = 0; i < 16; i++) {
        // TODO: STE mode
        uint16_t entry = atari_screen.palette[i];
        BG_PALETTE[i] = 0x8000 
            | (color_map_st[entry & 0x7] << 10)
            | (color_map_st[(entry >> 4) & 0x7] << 5)
            | (color_map_st[(entry >> 8) & 0x7]);
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

NDS_ITCM_CODE
__attribute__((optimize("-O3")))
void platform_gfx_draw_frame(void) {
    uint32_t ticks = _TIMER_TICKS(0);

    update_palette();

    uint8_t *src = memory_ram + (atari_screen.base & memory_ram_mask);

    if (atari_screen.resolution == 1) {
        // TODO
    } else {
        uint32_t *dst = ((uint32_t*) BG_GFX);
        src -= 168;
        for (int y = 0; y < 200; y++) {
            src += 320;
            #pragma GCC unroll 20
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
    }

    ticks = _TIMER_TICKS(0) - ticks;
    // iprintf("%d ticks\n", ticks);
}