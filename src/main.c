#include <stdio.h>
#include "platform.h"
#include "platform_audio.h"
#include "platform_config.h"
#include "system.h"

// #define SPEED_MEASURE

int main(int argc, const char **argv) {
	platform_init();

	printf("STrive emulator\n");

	if (!system_init()) {
		printf("System init failure!\n");
		while(1);
	}

	platform_audio_init();

	double time_behind = 0.0;

	while (platform_check_events()) {
		uint32_t ticks = platform_get_ticks();
		platform_update_input();
		system_frame();

#ifdef SPEED_MEASURE
		ticks = platform_get_ticks() - ticks;
		uint32_t expected_ticks = PLATFORM_TICKS_PER_SECOND / 50;
		printf("framespeed: %d/%d ticks (%d%%)\n",
			ticks, expected_ticks, (expected_ticks * 100 / ticks));
#endif

		platform_wait_vblank();
		/* uint32_t runtime_ticks = platform_get_ticks();
		double time_current = (0.0225 - (((double) ((uint32_t) (runtime_ticks - ticks))) / PLATFORM_TICKS_PER_SECOND));
		if (time_current > 0.0) {
			// printf("sleep %f\n", time_current);
			svcSleepThread(time_current * 1e9);
		} */
	}

	platform_audio_exit();

	platform_exit();
	return 0;
}
