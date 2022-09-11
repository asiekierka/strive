#pragma once

#define STRIVE_68K_CYCLONE
#define STRIVE_AY38910_FLUBBA
#define ROM_FILE "/etos192uk.img"

#define NDS_ITCM_CODE __attribute__((section(".itcm"), long_call))
#define NDS_DTCM_DATA __attribute__((section(".dtcm")))
#define NDS_DTCM_BSS __attribute__((section(".sbss")))

#define _TIMER_TICKS(tid) (TIMER_DATA((tid)) | (TIMER_DATA((tid)+1) << 16))