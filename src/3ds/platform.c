#include "platform.h"
#include "3ds/os.h"
#include "acia.h"
#include "platform_config.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>

void _platform_gfx_init(void);
void _platform_gfx_exit(void);

bool platform_init(void) {
    cfguInit();
    romfsInit();

    _platform_gfx_init();

    return true;
}

bool platform_check_events(void) {
    if (!aptMainLoop()) return false;

    hidScanInput();

    int exitMask = (KEY_L | KEY_R | KEY_START | KEY_SELECT);
    if ((hidKeysHeld() & exitMask) == exitMask) return false;

    return true;
}

bool platform_exit(void) {
    _platform_gfx_exit();

    romfsExit();
    cfguExit();

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

uint32_t platform_get_ticks(void) {
    return svcGetSystemTick();
}

static touchPosition starting_touch_position, last_touch;
static int ticks_since_touch = 0;
static bool touch_button_down = false;
static bool touch_button_up_next = false;
#define TOUCH_CLICK_TICKS 10
#define TOUCH_CLICK_THRESHOLD 4

void platform_update_input(void) {
    if (touch_button_up_next) {
        touch_button_up_next = false;
    }

    if (hidKeysDown() & KEY_TOUCH) {
        ticks_since_touch = 0;
        touch_button_down = false;
        hidTouchRead(&starting_touch_position);
    }

    if (hidKeysHeld() & KEY_TOUCH) {
        ticks_since_touch++;
        hidTouchRead(&last_touch);
        acia_ikbd_set_mouse_pos(last_touch.px, last_touch.py - 20);

        if (ticks_since_touch == TOUCH_CLICK_TICKS &&
            abs(last_touch.px - starting_touch_position.px) <= TOUCH_CLICK_THRESHOLD &&
            abs(last_touch.py - starting_touch_position.py) <= TOUCH_CLICK_THRESHOLD) {
            acia_ikbd_set_mouse_button(ACIA_MOUSE_BUTTON_LEFT_DOWN);
            touch_button_down = true;
        }
    }

    if (hidKeysUp() & KEY_TOUCH) {
        if (ticks_since_touch > 0 && ticks_since_touch < TOUCH_CLICK_TICKS &&
            abs(last_touch.px - starting_touch_position.px) <= TOUCH_CLICK_THRESHOLD &&
            abs(last_touch.py - starting_touch_position.py) <= TOUCH_CLICK_THRESHOLD) {
            debug_printf("click\n");
            acia_ikbd_set_mouse_button(ACIA_MOUSE_BUTTON_LEFT_DOWN);
            acia_ikbd_set_mouse_button(ACIA_MOUSE_BUTTON_LEFT_UP);
        } else if (touch_button_down) {
            acia_ikbd_set_mouse_button(ACIA_MOUSE_BUTTON_LEFT_UP);
        }
    }
}
