#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ACIA_IKBD_INPUT_LEN 16
#define ACIA_IKBD_OUTPUT_LEN 64

#define ACIA_MOUSE_MODE_RELATIVE 0x08
#define ACIA_MOUSE_MODE_ABSOLUTE 0x09
#define ACIA_MOUSE_MODE_KEYCODE  0x0A
#define ACIA_MOUSE_MODE_DISABLED 0x12

#define ACIA_MOUSE_Y0_BOTTOM 0x0F
#define ACIA_MOUSE_Y0_TOP    0x10

#define ACIA_MOUSE_BUTTON_REPORT_PRESS    0x01
#define ACIA_MOUSE_BUTTON_REPORT_RELEASE  0x02
#define ACIA_MOUSE_BUTTON_REPORT_KEYCODES 0x04
#define ACIA_MOUSE_BUTTON_RIGHT_DOWN 0x01
#define ACIA_MOUSE_BUTTON_RIGHT_UP   0x02
#define ACIA_MOUSE_BUTTON_LEFT_DOWN  0x04
#define ACIA_MOUSE_BUTTON_LEFT_UP    0x08

typedef struct acia {
    uint8_t ikbd_control, ikbd_data, ikbd_status;
    uint8_t midi_control, midi_data, midi_status;

    uint8_t ikbd_in[ACIA_IKBD_INPUT_LEN];
    uint8_t ikbd_in_len;
    uint8_t ikbd_out[ACIA_IKBD_OUTPUT_LEN];
    uint8_t ikbd_out_len;

    uint8_t mouse_mode;
    uint8_t mouse_button_mode;
    uint8_t mouse_y0_mode;

    uint8_t mouse_rel_xthr;
    uint8_t mouse_rel_ythr;
    uint16_t mouse_abs_xmax;
    uint16_t mouse_abs_ymax;
    uint8_t mouse_abs_xscale;
    uint8_t mouse_abs_yscale;
    uint8_t mouse_key_dx;
    uint8_t mouse_key_dy;

    int32_t mouse_x, mouse_y;
    int32_t last_mouse_x, last_mouse_y;
    uint8_t mouse_button;
    uint8_t mouse_button_interrogate;

    int cycles_until_reset;
    int64_t time_offset;
} acia_t;

extern acia_t atari_acia;

void acia_init(void);
void acia_advance(int cycles);
uint8_t acia_read8(uint8_t addr);
void acia_write8(uint8_t addr, uint8_t value);

// called by user

void acia_ikbd_set_mouse_pos(int32_t x, int32_t y);
void acia_ikbd_add_mouse_pos(int32_t x, int32_t y);
void acia_ikbd_set_mouse_button(uint8_t v);