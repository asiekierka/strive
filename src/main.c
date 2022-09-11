#include <stdio.h>
#include "platform.h"
#include "system.h"

int main(int argc, const char **argv) {
	platform_init();

	iprintf("STrive emulator\n");

	system_init();

	while (platform_check_events()) {
		platform_update_input();
		system_frame();

		platform_wait_vblank();
	}

	platform_exit();
	return 0;
}
