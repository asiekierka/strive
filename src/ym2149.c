#include <string.h>
#include "ym2149.h"
#include "platform_config.h"

#if defined(STRIVE_AY38910_FLUBBA)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
#include "AY38910.h"

AY38910 ym2149;
static uint8_t ym_port_a;

void ym2149_init(void) {
    memset(&ym2149, 0, sizeof(AY38910));
    ay38910Reset(&ym2149);
}

uint8_t ym2149_get_port_a(void) {
    return ym_port_a;
}

uint8_t ym2149_read8(uint8_t addr) {
    switch (addr) {
    case 0x00:
        if (ym2149.ayRegIndex == 0x0E) {
            return ym_port_a;
        }
        return ay38910DataR(&ym2149);
    default:
        return 0;
    }
}

void ym2149_write8(uint8_t addr, uint8_t value) {
    switch (addr) {
    case 0x00:
        ay38910IndexW(value, &ym2149);
        break;
    case 0x02:
        if (ym2149.ayRegIndex == 0x0E) {
            ym_port_a = value;
        }
        ay38910DataW(value, &ym2149);
        break;
    }
}

#elif defined(STRIVE_AY38910_DUMMY)

void ym2149_init(void) {
    // TODO
}

uint8_t ym2149_get_port_a(void) {
    return 0; // TODO
}

uint8_t ym2149_read8(uint8_t addr) {
    // TODO
    switch (addr) {
    default:
        return 0;
    }
}

void ym2149_write8(uint8_t addr, uint8_t value) {
    // TODO
}

#endif