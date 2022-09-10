#include <stdio.h>
#include <3ds.h>
#include "3ds/services/hid.h"
#include "Cyclone.h"
#include "platform.h"
#include "system.h"

int main(int argc, const char **argv) {
	platform_init();

	iprintf("STrive emulator\n");

	system_init();

	while (platform_check_events()) {
		// TODO
		if (hidKeysHeld() == 0) system_frame();

		gspWaitForVBlank();
	}

	platform_exit();
	return 0;
}
