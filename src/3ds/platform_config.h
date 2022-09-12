#pragma once

#define STRIVE_68K_CYCLONE
#define STRIVE_AY38910_FLUBBA
#define ROM_FILE "romfs:/etos192uk.img"

#define DEBUG

// #define TRACE
// #define TRACE_CPU

#if defined(TRACE)
#include <stdio.h>

extern FILE* trace_file;
#define debug_printf(...) fprintf(trace_file, __VA_ARGS__)
#elif defined(DEBUG)
#define debug_printf(...) iprintf(__VA_ARGS__)
#else
#define debug_printf(...)
#endif

#define PLATFORM_TICKS_PER_SECOND 134055928
#define NDS_ITCM_CODE
#define NDS_DTCM_DATA
#define NDS_DTCM_BSS
