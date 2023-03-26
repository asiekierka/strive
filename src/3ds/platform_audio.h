#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PLATFORM_AUDIO_CHANNEL_YM2149 = 0, /* signed 16-bit, 31300 Hz */
    PLATFORM_AUDIO_CHANNEL_COUNT
} platform_audio_channel_t;

void platform_audio_push(platform_audio_channel_t channel, uint8_t *data, int len);

void platform_audio_init(void);
void platform_audio_exit(void);