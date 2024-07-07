#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include "platform.h"
#include "platform_config.h"

typedef struct {
    ndspWaveBuf hw_buffer[2];
    uint8_t *buffer;
    uint8_t *buffer_local;
    uint32_t buffer_pos;
    uint32_t buffer_size;
    uint8_t sample_size;
    bool hw_buffer_idx;
    bool started;
} ctr_audio_channel_t;

static ctr_audio_channel_t channels[PLATFORM_AUDIO_CHANNEL_COUNT];

void platform_audio_push(platform_audio_channel_t idx, uint8_t *data, int len) {
    ctr_audio_channel_t *ch = &channels[idx];

    if (len > (ch->buffer_size - ch->buffer_pos)) len = (ch->buffer_size - ch->buffer_pos);
    if (len > 0) {
        memcpy(ch->buffer_local + (ch->buffer_pos * ch->sample_size), data, len * ch->sample_size);
        ch->buffer_pos += len;
    }
}

static void platform_audio_callback(void* dud) {
    for (int idx = 0; idx < PLATFORM_AUDIO_CHANNEL_COUNT; idx++) {
        ctr_audio_channel_t *ch = &channels[idx];
        if (ch->hw_buffer[ch->hw_buffer_idx].status == NDSP_WBUF_DONE) {
            // copy data from buffer
            // TODO: memmove -> ring buffer

            int len = ch->buffer_size / 2;
            int len_copy = len;
            if (ch->buffer_pos < (ch->buffer_size * 3 / 5)) {
                len_copy = 0;
            }
            debug_printf("%d %d %d\n", len_copy, len, ch->buffer_pos);
            uint8_t *src = ch->buffer_local;
            uint8_t *dst = (uint8_t*) ch->hw_buffer[ch->hw_buffer_idx].data_vaddr;
            memcpy(dst, src, len_copy * ch->sample_size);
            dst += (len_copy * ch->sample_size);
            if (len > len_copy) {
                memset(dst, ch->sample_size == 2 ? 0 : 0x80, (len - len_copy) * ch->sample_size);
            }

            if (ch->buffer_pos > len_copy) {
                if (len_copy > 0) {
                    memmove(
                        ch->buffer_local,
                        ch->buffer_local + (len_copy * ch->sample_size),
                        (ch->buffer_pos - len_copy) * ch->sample_size
                    );
                }
                ch->buffer_pos -= len_copy;
            } else {
                ch->buffer_pos = 0;
            }

            DSP_FlushDataCache(channels[idx].buffer, ch->buffer_size * ch->sample_size);

            ndspChnWaveBufAdd(idx, &(ch->hw_buffer[ch->hw_buffer_idx]));
            ch->hw_buffer_idx = !ch->hw_buffer_idx;

            debug_printf("-> %d %d %d\n", len_copy, len, ch->buffer_pos);
        }
    }
}

static void audio_channel_init(platform_audio_channel_t idx, float rate, int achannels, int encoding, uint32_t buffer_size) {
	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = mix[1] = 1.0f;

    channels[idx].sample_size = achannels * (encoding == NDSP_ENCODING_PCM16 ? 2 : 1);
    channels[idx].buffer_size = buffer_size * 2;

    ndspChnReset(idx);
    ndspChnSetInterp(idx, NDSP_INTERP_LINEAR);
    ndspChnSetRate(idx, rate);
    ndspChnSetFormat(idx, NDSP_CHANNELS(achannels) * NDSP_ENCODING(encoding));
    ndspChnSetMix(idx, mix);

    uint32_t buffer_size_bytes = channels[idx].sample_size * buffer_size;
    channels[idx].buffer_local = malloc(buffer_size_bytes * 2);
    channels[idx].buffer = linearAlloc(buffer_size_bytes * 2);
    memset(channels[idx].buffer, 0, buffer_size_bytes * 2);
    
    memset(channels[idx].hw_buffer, 0, sizeof(ndspWaveBuf) * 2);
    channels[idx].hw_buffer[0].data_vaddr = channels[idx].buffer;
    channels[idx].hw_buffer[0].nsamples = buffer_size;
    channels[idx].hw_buffer[1].data_vaddr = channels[idx].buffer + buffer_size_bytes;
    channels[idx].hw_buffer[1].nsamples = buffer_size;

    channels[idx].buffer_pos = 0;
    channels[idx].hw_buffer_idx = false;
    channels[idx].started = false;
    
    DSP_FlushDataCache(channels[idx].buffer, buffer_size_bytes);

    ndspChnWaveBufAdd(idx, &(channels[idx].hw_buffer[0]));
    ndspChnWaveBufAdd(idx, &(channels[idx].hw_buffer[1]));
}

static void audio_channel_exit(platform_audio_channel_t idx) {
    ndspChnWaveBufClear(idx);

    linearFree(channels[idx].buffer);
    free(channels[idx].buffer_local);
    channels[idx].buffer = NULL;
}

void platform_audio_init(void) {
    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    audio_channel_init(
        PLATFORM_AUDIO_CHANNEL_YM2149,
        31300.0f,
        1, NDSP_ENCODING_PCM16,
        2048
    );

    ndspSetMasterVol(1.0f);
	ndspSetCallback(platform_audio_callback, NULL);
}

void platform_audio_exit(void) {
    audio_channel_exit(PLATFORM_AUDIO_CHANNEL_YM2149);

	ndspExit();
}
