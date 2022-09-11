#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "platform_config.h"

/* == General == */

bool platform_init(void);
/**
 * @brief Check platform-specific events.
 * 
 * @return true Continue execution.
 * @return false Cease execution as soon as possible.
 */
bool platform_check_events(void);
bool platform_exit(void);
void platform_wait_key(void);
void platform_wait_vblank(void);
void platform_update_input(void);

/* == Graphics == */

void platform_gfx_on_palette_update(void);
void platform_gfx_draw_frame(void);

/* == Date == */

typedef struct {
    uint32_t year, month, day;
    uint32_t hour, minute, second;
} platform_daytime_t;

void platform_get_daytime(platform_daytime_t *daytime, int64_t offset);