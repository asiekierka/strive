#pragma once

#define STRIVE_68K_CYCLONE
#define STRIVE_AY38910_FLUBBA
#define ROM_FILE "romfs:/etos192uk.img"

#define debug_printf(...) iprintf(__VA_ARGS__)

#define PLATFORM_TICKS_PER_SECOND 134055928
#define NDS_ITCM_CODE
#define NDS_DTCM_DATA
#define NDS_DTCM_BSS
