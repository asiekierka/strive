#pragma once

#define STRIVE_68K_CYCLONE
#define STRIVE_AY38910_FLUBBA
#define ROM_FILE "/etos192uk.img"

#define debug_printf(...)

#define USE_VIDEO_MEMORY_DIRTY

#define PLATFORM_TICKS_PER_SECOND 33513982
#define NDS_ITCM_CODE __attribute__((section(".itcm"), long_call))
#define NDS_DTCM_DATA __attribute__((section(".dtcm")))
#define NDS_DTCM_BSS __attribute__((section(".sbss")))

#define _TIMER_TICKS(tid) (TIMER_DATA((tid)) | (TIMER_DATA((tid)+1) << 16))