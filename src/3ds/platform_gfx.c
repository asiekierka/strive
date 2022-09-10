#include "memory.h"
#include "platform.h"
#include "screen.h"
#include <stdint.h>
#include <stdio.h>
#include <3ds.h>
#include <citro3d.h>
#include "grapefruit.h"
#include "shader_shbin.h"

typedef struct {
    uint32_t first, second, third, fourth;
} palette_lut_entry_t;

static bool palette_changed = false;
static palette_lut_entry_t palette_lut_2bpp[256];
static palette_lut_entry_t palette_lut_4bpp[256];
static uint32_t palette_mini_lut[16];
static uint8_t color_map_st[8] = {
    0, 36, 73, 109, 146, 182, 219, 255
};
static uint8_t color_map_ste[16] = {
    0, 34, 68, 102, 136, 170, 204, 238,
    17, 51, 85, 119, 153, 187, 221, 255
};

static C3D_RenderTarget *target_top;
static C3D_Mtx proj_top;
static struct ctr_shader_data shader;

static C3D_Tex screen_tex;
static uint32_t *screen_buf;

void _platform_gfx_init(void) {
    C3D_TexEnv *texEnv;

    gfxInitDefault();
    gfxSet3D(false);
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    consoleInit(GFX_BOTTOM, NULL);

	target_top = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, GPU_RB_DEPTH16);
	C3D_RenderTargetClear(target_top, C3D_CLEAR_ALL, 0, 0);
	C3D_RenderTargetSetOutput(target_top, GFX_TOP, GFX_LEFT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));

	C3D_TexInitVRAM(&screen_tex, 1024, 512, GPU_RGBA8);
	screen_buf = linearAlloc(1024 * 512 * 4);

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
    gfxExit();
}

void platform_gfx_on_palette_update(void) {
    palette_changed = true;
}

static void update_palette(void) {
    if (!palette_changed) return;

    for (int i = 0; i < 16; i++) {
        // TODO: STE mode
        uint16_t entry = atari_screen.palette[i];
        palette_mini_lut[i] =
            0x000000FF
            | (color_map_st[entry & 0x7] << 8)
            | (color_map_st[(entry >> 4) & 0x7] << 16)
            | (color_map_st[(entry >> 8) & 0x7] << 24);
    }
    for (int i = 0; i < 256; i++) {
        palette_lut_2bpp[i] = (palette_lut_entry_t) {
            palette_mini_lut[i >> 6],
            palette_mini_lut[(i >> 4) & 0x3],
            palette_mini_lut[(i >> 2) & 0x3],
            palette_mini_lut[i & 0x3]
        };
        palette_lut_4bpp[i] = (palette_lut_entry_t) {
            palette_mini_lut[i >> 4],
            palette_mini_lut[i >> 4],
            palette_mini_lut[i & 0xF],
            palette_mini_lut[i & 0xF]
        };
    }

    palette_changed = false;
}

void platform_gfx_draw_frame(void) {
	float xmin, ymin, xmax, ymax, txmin, tymin, txmax, tymax;

    update_palette();

    uint8_t *src = memory_ram + (atari_screen.base & memory_ram_mask);

    // TODO: Optimize, implement other drawing modes than 0
    for (int y = 0; y < 200; y++) {
        uint32_t *dst = screen_buf + (1024 * y);
        for (int x = 0; x < 320; x += 16, src += 8) {
            uint16_t bp0 = src[1] | (src[0] << 8);
            uint16_t bp1 = src[3] | (src[2] << 8);
            uint16_t bp2 = src[5] | (src[4] << 8);
            uint16_t bp3 = src[7] | (src[6] << 8);
            for (int i = 0; i < 16; i++, bp0 <<= 1, bp1 <<= 1, bp2 <<= 1, bp3 <<= 1) {
                uint8_t color = (bp0 >> 15) | ((bp1 >> 14) & 0x02)
                    | ((bp2 >> 13) & 0x04) | ((bp3 >> 12) & 0x08);
                *(dst++) = palette_mini_lut[color];
            }
         }
    }

	GSPGPU_FlushDataCache(screen_buf, 1024 * 512 * 4);
	C3D_SyncDisplayTransfer(screen_buf, GX_BUFFER_DIM(1024, 512), screen_tex.data,
        GX_BUFFER_DIM(screen_tex.width, screen_tex.height),
		(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
		GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
	);

    C3D_FrameBegin(0);

	xmin = (400 - 320) / 2.0f;
	ymin = (240 - 200) / 2.0f;
	xmax = xmin + 320;
	ymax = ymin + 200;
	txmax = ((float) 320 / screen_tex.width);
	txmin = 0.0f;
	tymin = ((float) 200 / screen_tex.height);
	tymax = 0.0f;

	C3D_FrameDrawOn(target_top);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shader.proj_loc, &proj_top);

	C3D_TexBind(0, &screen_tex);
	C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
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