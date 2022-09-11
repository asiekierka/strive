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
    REG_BG2PA = 320;
    REG_BG2PB = 0;
    REG_BG2PC = 0;
    REG_BG2PD = 267;
    REG_BG2X = 0;
    REG_BG2Y = 0;
    REG_BG2HOFS = 0;
    REG_BG2VOFS = 0;
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

#define COMBINE_4PX_ST_LOW() \
    ((bp0 >> 15) | ((bp1 >> 14) & 0x02) \
        | ((bp2 >> 13) & 0x04) | ((bp3 >> 12) & 0x08) \
        | ((bp0 >> 6) & 0x100) | ((bp1 >> 5) & 0x200) \
        | ((bp2 >> 4) & 0x400) | ((bp3 >> 3) & 0x800) \
        | ((bp0 << 3) & 0x10000) | ((bp1 << 4) & 0x20000) \
        | ((bp2 << 5) & 0x40000) | ((bp3 << 6) & 0x80000) \
        | ((bp0 << 12) & 0x1000000) | ((bp1 << 13) & 0x2000000) \
        | ((bp2 << 14) & 0x4000000) | ((bp3 << 15) & 0x8000000))

NDS_ITCM_CODE
__attribute__((optimize("-O3")))
void platform_gfx_draw_frame(void) {
    update_palette();

    uint8_t *src = memory_ram + (atari_screen.base & memory_ram_mask);

    if (atari_screen.resolution == 1) {
        // TODO
    } else {
        uint32_t *dst = ((uint32_t*) BG_GFX);
        for (int y = 0; y < 200; y++) {
            for (int x = 0; x < 320; x += 16, src += 8, dst += 4) {
                uint16_t bp0 = src[0] | (src[1] << 8);
                uint16_t bp1 = src[2] | (src[3] << 8);
                uint16_t bp2 = src[4] | (src[5] << 8);
                uint16_t bp3 = src[6] | (src[7] << 8);
                dst[0] = COMBINE_4PX_ST_LOW();
                bp0 <<= 4; bp1 <<= 4; bp2 <<= 4; bp3 <<= 4;
                dst[1] = COMBINE_4PX_ST_LOW();
                bp0 <<= 4; bp1 <<= 4; bp2 <<= 4; bp3 <<= 4;
                dst[2] = COMBINE_4PX_ST_LOW();
                bp0 <<= 4; bp1 <<= 4; bp2 <<= 4; bp3 <<= 4;
                dst[3] = COMBINE_4PX_ST_LOW();
                bp0 <<= 4; bp1 <<= 4; bp2 <<= 4; bp3 <<= 4;
            }
            dst += (512 - 320) >> 2;
        }
    }
}