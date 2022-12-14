#include <stdio.h>
#include "platform.h"
#include "platform_config.h"
#include "system.h"

// #define SPEED_MEASURE

int main(int argc, const char **argv) {
	platform_init();

	iprintf("STrive emulator\n");

	system_init();

	while (platform_check_events()) {
		uint32_t ticks = platform_get_ticks();
		platform_update_input();
		system_frame();

#ifdef SPEED_MEASURE
		ticks = platform_get_ticks() - ticks;
		uint32_t expected_ticks = PLATFORM_TICKS_PER_SECOND / 50;
		iprintf("framespeed: %d/%d ticks (%d%%)\n",
			ticks, expected_ticks, (expected_ticks * 100 / ticks));
#endif

		platform_wait_vblank();
	}

	platform_exit();
	return 0;
}
