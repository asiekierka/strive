#include "platform.h"
#include "acia.h"
#include "platform_config.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <nds.h>
#include <fat.h>
#include <filesystem.h>

void _platform_gfx_init(void);
void _platform_gfx_exit(void);

bool platform_init(void) {
    _platform_gfx_init();

    fatInitDefault();
    nitroFSInit("nitro:/");

	// Timers 0-1 - profiling timers
	TIMER0_DATA = 0;
	TIMER1_DATA = 0;
	TIMER0_CR = TIMER_ENABLE | TIMER_DIV_1;
	TIMER1_CR = TIMER_ENABLE | TIMER_CASCADE;

    return true;
}

bool platform_check_events(void) {
    scanKeys();

    int exitMask = (KEY_L | KEY_R | KEY_START | KEY_SELECT);
    if ((keysHeld() & exitMask) == exitMask) return false;

    return true;
}

bool platform_exit(void) {
    _platform_gfx_exit();

    return true;
}

void platform_wait_key(void) {
	while (platform_check_events()) {
		if (keysDown() != 0) break;

        swiWaitForVBlank();
	}
}

void platform_wait_vblank(void) {
    swiWaitForVBlank();
}

void platform_update_input(void) {

}

uint32_t platform_get_ticks(void) {
    return _TIMER_TICKS(0);
}
