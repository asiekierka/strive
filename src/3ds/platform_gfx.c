#include "../memory.h"
#include "platform.h"
#include "screen.h"
#include <stdint.h>
#include <stdio.h>
#include <3ds.h>
#include <citro3d.h>
#include "grapefruit.h"
#include "shader_shbin.h"

static bool palette_changed = false;
static uint32_t palette_mini_lut[16];
static uint32_t palette_mono_lut[4];
static const uint8_t color_map_st[16] = {
    0, 36, 73, 109, 146, 182, 219, 255,
    0, 36, 73, 109, 146, 182, 219, 255
};
static const uint8_t color_map_ste[16] = {
    0, 34, 68, 102, 136, 170, 204, 238,
    17, 51, 85, 119, 153, 187, 221, 255
};

static C3D_RenderTarget *target_top;
static C3D_Mtx proj_top;
static struct ctr_shader_data shader;

static C3D_Tex screen_tex;
static uint32_t *screen_buf;
static bool screen_wide;

void _platform_gfx_init(void) {
    C3D_TexEnv *texEnv;

    uint8_t console_model_id;
    CFGU_GetSystemModel(&console_model_id);
    screen_wide = console_model_id != 3 /* Old 2DS */;

    gfxInitDefault();
    gfxSet3D(false);
    gfxSetWide(screen_wide);
    consoleInit(GFX_BOTTOM, NULL);

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	target_top = C3D_RenderTargetCreate(240, screen_wide ? 800 : 400, GPU_RB_RGB8, GPU_RB_DEPTH16);
	C3D_RenderTargetClear(target_top, C3D_CLEAR_ALL, 0, 0);
	C3D_RenderTargetSetOutput(target_top, GFX_TOP, GFX_LEFT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));

	C3D_TexInitVRAM(&screen_tex, 1024, 256, GPU_RGBA8);
        C3D_TexSetFilter(&screen_tex, GPU_LINEAR, GPU_NEAREST);
	screen_buf = linearAlloc(1024 * 256 * 4);

	ctr_init_shader(&shader, shader_shbin, shader_shbin_size);
	AttrInfo_AddLoader(&(shader.attr), 0, GPU_FLOAT, 3); // v0 = position
	AttrInfo_AddLoader(&(shader.attr), 1, GPU_FLOAT, 2); // v1 = texcoord
	ctr_bind_shader(&shader);

	Mtx_OrthoTilt(&proj_top, 0.0, 400.0, 0.0, 240.0, -1.0, 1.0, true);

	texEnv = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(texEnv, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOpRgb(texEnv, 0, 0, 0);
	C3D_TexEnvOpAlpha(texEnv, 0, 0, 0);
	C3D_TexEnvFunc(texEnv, C3D_Both, GPU_MODULATE);

	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
}

void _platform_gfx_exit(void) {
    linearFree(screen_buf);
    C3D_TexDelete(&screen_tex);

    C3D_RenderTargetDelete(target_top);

    C3D_Fini();
    gfxExit();
}

void platform_gfx_on_palette_update(void) {
    palette_changed = true;
}

static void update_palette(void) {
    if (!palette_changed) return;

    // TODO: STE mode
    const uint8_t *color_map = color_map_st;

    for (int i = 0; i < 16; i++) {
        uint16_t entry = atari_screen.palette[i];
        palette_mini_lut[i] =
            0x000000FF
            | (color_map[entry & 0xF] << 8)
            | (color_map[(entry >> 4) & 0xF] << 16)
            | (color_map[(entry >> 8) & 0xF] << 24);
    }

    if (atari_screen.resolution == 2) {
        uint8_t r0 = palette_mini_lut[0] >> 24;
        uint8_t g0 = palette_mini_lut[0] >> 16;
        uint8_t b0 = palette_mini_lut[0] >> 8;
        uint8_t r1 = palette_mini_lut[1] >> 24;
        uint8_t g1 = palette_mini_lut[1] >> 16;
        uint8_t b1 = palette_mini_lut[1] >> 8;
        for (int i = 0; i < 4; i++) {
            uint16_t r = (r0 * (3 - i)) + (r1 * i);
            uint16_t g = (g0 * (3 - i)) + (g1 * i);
            uint16_t b = (b0 * (3 - i)) + (b1 * i);
            palette_mono_lut[i] = ((r / 3) << 24)
                | ((g / 3) << 16)
                | ((b / 3) << 8) | 0xFF;
        }
    }

    palette_changed = false;
}

static int16_t start_y;

void platform_gfx_frame_start(void) {
    start_y = -20;
}

__attribute__((optimize("-O3")))
void platform_gfx_frame_draw_to(int16_t end_y) {
    if (start_y >= end_y) return;
    if (end_y <= -20) return;
    
    update_palette();

    int32_t end_y_border = (end_y > 220) ? 220 : end_y;
    for (int y = start_y; y < end_y_border; y++) {
        screen_buf[768 + (1024 * (y + 20))] =  palette_mini_lut[0];     
    }

    if (end_y <= 0) return;
    if (end_y > 200) end_y = 200;

    uint8_t *src = memory_ram + (atari_screen.base & memory_ram_mask) + (start_y * 160);

    if (atari_screen.resolution == 2) {
        for (int y = start_y; y < end_y; y++) {
            uint32_t *dst = screen_buf + (1024 * y);
            for (int x = 0; x < 640; x += 16, src += 2) {
                uint16_t bp0 = src[0] | (src[1] << 8);
                uint16_t bp1 = src[80] | (src[81] << 8);
                for (int i = 0; i < 16; i++, bp0 <<= 1, bp1 <<= 1) {
                    uint8_t color = (bp0 >> 15) | ((bp1 >> 14) & 0x02);
                    *(dst++) = palette_mono_lut[color];
                }
            }
            src += 80;
        }
    } else if (atari_screen.resolution == 1) {
        for (int y = start_y; y < end_y; y++) {
            uint32_t *dst = screen_buf + (1024 * y);
            for (int x = 0; x < 640; x += 16, src += 4) {
                uint16_t bp0 = src[0] | (src[1] << 8);
                uint16_t bp1 = src[2] | (src[3] << 8);
                for (int i = 0; i < 16; i++, bp0 <<= 1, bp1 <<= 1) {
                    uint8_t color = (bp0 >> 15) | ((bp1 >> 14) & 0x02);
                    *(dst++) = palette_mini_lut[color];
                }
            }
        }
    } else {
        for (int y = start_y; y < end_y; y++) {
            uint32_t *dst = screen_buf + (1024 * y);
            for (int x = 0; x < 320; x += 16, src += 8) {
                uint16_t bp0 = src[0] | (src[1] << 8);
                uint16_t bp1 = src[2] | (src[3] << 8);
                uint16_t bp2 = src[4] | (src[5] << 8);
                uint16_t bp3 = src[6] | (src[7] << 8);
                for (int i = 0; i < 16; i++, bp0 <<= 1, bp1 <<= 1, bp2 <<= 1, bp3 <<= 1) {
                    uint8_t color = (bp0 >> 15) | ((bp1 >> 14) & 0x02)
                        | ((bp2 >> 13) & 0x04) | ((bp3 >> 12) & 0x08);
                    *(dst++) = palette_mini_lut[color];
                }
            }
        }
    }
    
    start_y = end_y;
}

void platform_gfx_frame_finish(void) {
	float xmin, ymin, xmax, ymax, txmin, tymin, txmax, tymax;
    float txmin_border, tymin_border, txmax_border, tymax_border;
    uint16_t scr_width, scr_height;

    if (atari_screen.resolution >= 1) {
        scr_width = 640;
        scr_height = 200;
    } else {
        scr_width = 320;
        scr_height = 200;
    }

	GSPGPU_FlushDataCache(screen_buf, 1024 * 256 * 4);
	C3D_SyncDisplayTransfer(screen_buf, GX_BUFFER_DIM(1024, 256), screen_tex.data,
        GX_BUFFER_DIM(screen_tex.width, screen_tex.height),
		(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
		GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
	);

    C3D_FrameBegin(0);
    // C3D_RenderTargetClear(target_top, C3D_CLEAR_ALL, palette_mini_lut[0] >> 8, 0);

	xmin = (400 - 320) / 2.0f;
	ymin = (240 - 200) / 2.0f;
	xmax = xmin + 320;
	ymax = ymin + 200;
	txmax = ((float) scr_width / screen_tex.width);
	txmin = 0.0f;
    tymin = ((float) scr_height / screen_tex.height);
	tymax = 0.0f;
    txmax_border = 769.0f / screen_tex.width;
    txmin_border = 768.0f / screen_tex.width;
    tymin_border = 240.0f / screen_tex.height;
    tymax_border = 0.0f;

	C3D_FrameDrawOn(target_top);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shader.proj_loc, &proj_top);

	C3D_TexBind(0, &screen_tex);
	C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
    // draw border
		C3D_ImmSendAttrib(0, 0, 1.0f, 0.0f);
		C3D_ImmSendAttrib(txmin_border, tymin_border, 0.0f, 0.0f);

		C3D_ImmSendAttrib(target_top->frameBuf.height, 0, 1.0f, 0.0f);
		C3D_ImmSendAttrib(txmax_border, tymin_border, 0.0f, 0.0f);

		C3D_ImmSendAttrib(0, 240, 1.0f, 0.0f);
		C3D_ImmSendAttrib(txmin_border, tymax_border, 0.0f, 0.0f);

		C3D_ImmSendAttrib(target_top->frameBuf.height, 240, 1.0f, 0.0f);
		C3D_ImmSendAttrib(txmax_border, tymax_border, 0.0f, 0.0f);

    // draw screen
		C3D_ImmSendAttrib(xmin, ymin, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmin, tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmax, ymin, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmax, tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmin, ymax, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmin, tymax, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmax, ymax, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmax, tymax, 0.0f, 0.0f);
	C3D_ImmDrawEnd();

	C3D_FrameEnd(0);
}
