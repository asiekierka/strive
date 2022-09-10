#include "platform.h"
#include <stdint.h>
#include <stdio.h>
#include <3ds.h>

void _platform_gfx_init(void);
void _platform_gfx_exit(void);

bool platform_init(void) {
    romfsInit();

    _platform_gfx_init();

    return true;
}

bool platform_check_events(void) {
    if (!aptMainLoop()) return false;

    hidScanInput();

    return true;
}

bool platform_exit(void) {
    _platform_gfx_exit();

    romfsExit();

    return true;
}

void platform_wait_key(void) {
	while (platform_check_events()) {
		if (hidKeysDown() != 0) break;

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
}

void platform_wait_vblank(void) {
    gspWaitForVBlank();
}