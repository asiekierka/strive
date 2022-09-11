#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "acia.h"
#include "mfp.h"
#include "platform.h"
#include "system.h"

acia_t atari_acia;

static void acia_ikbd_reset(void) {
    atari_acia.ikbd_in_len = 0;
    atari_acia.ikbd_out_len = 0;
    atari_acia.mouse_rel_xthr = 1;
    atari_acia.mouse_rel_ythr = 1;
}

void acia_init(void) {
    memset(&atari_acia, 0, sizeof(acia_t));
    atari_acia.ikbd_status = 0x02;
    atari_acia.midi_status = 0x02;
    acia_ikbd_reset();
}

static void acia_update_interrupt(bool trig) {
    if (trig && (atari_acia.ikbd_control & 0x03) != 0x03) {
        atari_acia.ikbd_status |= 0x80;
        mfp_set_interrupt(MFP_INT_ID_ACIA);
    } else {
        atari_acia.ikbd_status &= 0x7F;
        mfp_clear_interrupt(MFP_INT_ID_ACIA);
    }
}

uint8_t acia_read8(uint8_t addr) {
    // TODO
    // debug_printf("acia_read8: %02X\n", addr);
    switch (addr) {
    case 0x00:
        return (atari_acia.ikbd_out_len > 0 ? 1 : 0) | atari_acia.ikbd_status;
    case 0x02:
        if (atari_acia.ikbd_out_len > 0) {
            uint8_t res = atari_acia.ikbd_out[0];
            if ((--atari_acia.ikbd_out_len) == 0) {
                debug_printf("acia: emptied buffer\n");
                acia_update_interrupt(false);
            } else {
                memmove(atari_acia.ikbd_out, atari_acia.ikbd_out + 1, atari_acia.ikbd_out_len);
                acia_update_interrupt(atari_acia.ikbd_control & 0x80);
            }
            return res;
        }
        return 0;
    case 0x04:
        return atari_acia.midi_status;
    case 0x06:
        return 0;
    case 0x21:
        // MegaST RTC
        system_bus_error_inner();
        return 0;
    default:
        return 0;
    }
}

void acia_ikbd_send(uint8_t value) {
    if (atari_acia.ikbd_out_len >= ACIA_IKBD_OUTPUT_LEN) {
        atari_acia.ikbd_out_len--;
    }
    atari_acia.ikbd_out[atari_acia.ikbd_out_len++] = value;
}

static inline void acia_ikbd_send_done(void) {
    acia_update_interrupt(atari_acia.ikbd_control & 0x80);
}

static uint8_t acia_number_to_bcd(int v) {
    return (v % 10) | (((v / 10) % 10) << 4);
}

static void acia_daytime_to_bcd(platform_daytime_t *daytime) {
    daytime->year = acia_number_to_bcd(daytime->year);   
    daytime->month = acia_number_to_bcd(daytime->month);
    daytime->day = acia_number_to_bcd(daytime->day);
    daytime->hour = acia_number_to_bcd(daytime->hour);
    daytime->minute = acia_number_to_bcd(daytime->minute);
    daytime->second = acia_number_to_bcd(daytime->second);
}

void acia_advance(int cycles) {
    if (atari_acia.cycles_until_reset > 0) {
        atari_acia.cycles_until_reset -= cycles;
        if (atari_acia.cycles_until_reset <= 0) {
            acia_ikbd_reset();
            acia_ikbd_send(0xF0);
            acia_update_interrupt(atari_acia.ikbd_control & 0x80);
        }
    }
}

static void acia_ikbd_recv(uint8_t value) {
    if (atari_acia.ikbd_in_len >= ACIA_IKBD_INPUT_LEN) {
        atari_acia.ikbd_in_len = 0;
    }
    atari_acia.ikbd_in[atari_acia.ikbd_in_len++] = value;

    switch (atari_acia.ikbd_in[0]) {
    case 0x07: /* Set mouse button action */
        if (atari_acia.ikbd_in_len < 2) return;
        atari_acia.mouse_button_mode = atari_acia.ikbd_in[1] & 0x07;
        break;
    case 0x87: /* Status - mouse button action */
        acia_ikbd_send(atari_acia.mouse_button_mode);
        acia_ikbd_send_done();
        break;
    case 0x08: /* Set relative mouse position reporting */
        debug_printf("acia: set mouse mode to relative\n");
        atari_acia.mouse_mode = ACIA_MOUSE_MODE_RELATIVE;
        atari_acia.mouse_x = 160;
        atari_acia.mouse_y = 100;
        atari_acia.last_mouse_x = atari_acia.mouse_x;
        atari_acia.last_mouse_y = atari_acia.mouse_y;
        break;
    case 0x88: /* Status - mouse mode */
    case 0x89:
    case 0x8A:
        acia_ikbd_send(atari_acia.mouse_mode == ACIA_MOUSE_MODE_DISABLED ? 0x00 : atari_acia.mouse_mode);
        acia_ikbd_send_done();
        break;
    case 0x09: /* Set absolute mouse positioning */
        if (atari_acia.ikbd_in_len < 5) return;
        debug_printf("acia: set mouse mode to absolute\n");
        atari_acia.mouse_mode = ACIA_MOUSE_MODE_ABSOLUTE;
        atari_acia.mouse_abs_xmax = (atari_acia.ikbd_in[1] << 8) | atari_acia.ikbd_in[2];
        atari_acia.mouse_abs_ymax = (atari_acia.ikbd_in[3] << 8) | atari_acia.ikbd_in[4];
        atari_acia.mouse_x = atari_acia.mouse_abs_xmax / 2;
        atari_acia.mouse_y = atari_acia.mouse_abs_ymax / 2;
        atari_acia.last_mouse_x = atari_acia.mouse_x;
        atari_acia.last_mouse_y = atari_acia.mouse_y;
        break;
    case 0x0A: /* Set absolute mouse keycode mode */
        if (atari_acia.ikbd_in_len < 3) return;
        debug_printf("acia: set mouse mode to keycode\n");
        atari_acia.mouse_mode = ACIA_MOUSE_MODE_KEYCODE;
        atari_acia.mouse_key_dx = atari_acia.ikbd_in[1];
        atari_acia.mouse_key_dy = atari_acia.ikbd_in[2];
        atari_acia.mouse_x = 160;
        atari_acia.mouse_y = 100;
        atari_acia.last_mouse_x = atari_acia.mouse_x;
        atari_acia.last_mouse_y = atari_acia.mouse_y;
        break;
    case 0x0B: /* Set mouse threshold */
        if (atari_acia.ikbd_in_len < 3) return;
        atari_acia.mouse_rel_xthr = atari_acia.ikbd_in[1];
        atari_acia.mouse_rel_ythr = atari_acia.ikbd_in[2];
        break;
    case 0x0C: /* Set mouse scale */
        if (atari_acia.ikbd_in_len < 3) return;
        atari_acia.mouse_abs_xscale = atari_acia.ikbd_in[1];
        atari_acia.mouse_abs_yscale = atari_acia.ikbd_in[2];
        break;
    case 0x0D: /* Interrogate mouse position */
        acia_ikbd_send(atari_acia.mouse_button_interrogate);
        atari_acia.mouse_button_interrogate = 0;
        acia_ikbd_send(atari_acia.mouse_x >> 8);
        acia_ikbd_send(atari_acia.mouse_x);
        acia_ikbd_send(atari_acia.mouse_y >> 8);
        acia_ikbd_send(atari_acia.mouse_y);
        acia_ikbd_send_done();
        break;
    case 0x0E: /* Load mouse position */
        if (atari_acia.ikbd_in_len < 6) return;
        atari_acia.mouse_x = (atari_acia.ikbd_in[2] << 8) | atari_acia.ikbd_in[3];
        atari_acia.mouse_y = (atari_acia.ikbd_in[4] << 8) | atari_acia.ikbd_in[5];
        atari_acia.last_mouse_x = atari_acia.mouse_x;
        atari_acia.last_mouse_y = atari_acia.mouse_y;
        break;
    case 0x0F: /* Set Y=0 at bottom */
        atari_acia.mouse_y0_mode = ACIA_MOUSE_Y0_BOTTOM;
        break;
    case 0x10: /* Set Y=0 at top */
        atari_acia.mouse_y0_mode = ACIA_MOUSE_Y0_TOP;
        break;
    case 0x8F: /* Status - Y=0? */
    case 0x90:
        acia_ikbd_send(atari_acia.mouse_y0_mode);
        acia_ikbd_send_done();
        break;
    case 0x11: /* Resume */
        debug_printf("acia: TODO command %02X\n", atari_acia.ikbd_in[0]);
        break;
    case 0x12: /* Disable mouse */
        atari_acia.mouse_mode = ACIA_MOUSE_MODE_DISABLED;
        break;
    case 0x92: /* Status - mouse disabled? */
        acia_ikbd_send(atari_acia.mouse_mode != ACIA_MOUSE_MODE_DISABLED ? 0x00 : atari_acia.mouse_mode);
        acia_ikbd_send_done();
        break;
    case 0x13: /* Pause output */
        debug_printf("acia: TODO command %02X\n", atari_acia.ikbd_in[0]);
        break;
    case 0x14: /* Set joystick event reporting */
        debug_printf("acia: TODO command %02X\n", atari_acia.ikbd_in[0]);
        break;
    case 0x15: /* Set joystick interrogation mode */
        debug_printf("acia: TODO command %02X\n", atari_acia.ikbd_in[0]);
        break;
    case 0x16: /* Joystick interrogate */
        debug_printf("acia: TODO command %02X\n", atari_acia.ikbd_in[0]);
        break;
    case 0x17: /* Set joystick monitoring */
        if (atari_acia.ikbd_in_len < 2) return;
        debug_printf("acia: TODO command %02X\n", atari_acia.ikbd_in[0]);
        break;
    case 0x18: /* Set fire button monitoring */
        if (atari_acia.ikbd_in_len < 2) return;
        debug_printf("acia: TODO command %02X\n", atari_acia.ikbd_in[0]);
        break;
    case 0x19: /* Set joystick keycode mode */
        if (atari_acia.ikbd_in_len < 7) return;
        debug_printf("acia: TODO command %02X\n", atari_acia.ikbd_in[0]);
        break;
    case 0x1A: /* Disable joysticks */
        debug_printf("acia: TODO command %02X\n", atari_acia.ikbd_in[0]);
        break;
    case 0x1B: /* Time-of-day clock set */
        if (atari_acia.ikbd_in_len < 7) return;
        debug_printf("acia: TODO command %02X\n", atari_acia.ikbd_in[0]);
        break;
    case 0x1C: /* Interrogate time-of-day clock */
        {
            platform_daytime_t daytime;
            platform_get_daytime(&daytime, atari_acia.time_offset);
            acia_daytime_to_bcd(&daytime);
            acia_ikbd_send(0xFC);
            acia_ikbd_send(daytime.year);
            acia_ikbd_send(daytime.month);
            acia_ikbd_send(daytime.day);
            acia_ikbd_send(daytime.hour);
            acia_ikbd_send(daytime.minute);
            acia_ikbd_send(daytime.second);
            acia_ikbd_send_done();
        }
        break;
    case 0x80: /* Reset */
        if (atari_acia.ikbd_in_len < 2) return;
        if (atari_acia.ikbd_in[1] == 0x01) {
            atari_acia.cycles_until_reset = 2000000;
        }
        break;
    }
    atari_acia.ikbd_in_len = 0;
}

void acia_write8(uint8_t addr, uint8_t value) {
    // debug_printf("acia_write8: %02X = %02X\n", addr, value);
    switch (addr) {
    case 0x00:
        atari_acia.ikbd_control = value; break;
    case 0x02:
        acia_ikbd_recv(value);
        // TODO
        // acia_update_interrupt((atari_acia.ikbd_control & 0x60) == 0x20);
        break;
    case 0x04:
        atari_acia.midi_control = value; break;
    case 0x06:
        break;
    case 0x21:
        // MegaST RTC
        system_bus_error_inner();
    }
}

static void acia_ikbd_send_mouse_relative(int8_t xd, int8_t yd) {
    // debug_printf("acia: send rel %d %d\n", xd, yd);
    acia_ikbd_send(0xF8 | ((atari_acia.mouse_button & ACIA_MOUSE_BUTTON_LEFT_DOWN) ? 2 : 0) | ((atari_acia.mouse_button & ACIA_MOUSE_BUTTON_RIGHT_DOWN) ? 1 : 0));
    acia_ikbd_send(xd);
    acia_ikbd_send(yd);
}

static void acia_ikbd_update_mouse_relative(bool force) {
    int32_t x_diff = atari_acia.mouse_x - atari_acia.last_mouse_x;
    int32_t y_diff = atari_acia.mouse_y - atari_acia.last_mouse_y;
    if (force || abs(x_diff) >= atari_acia.mouse_rel_xthr || abs(y_diff) >= atari_acia.mouse_rel_ythr) {
        while (x_diff > 127) { acia_ikbd_send_mouse_relative(127, 0); x_diff -= 127; }
        while (x_diff < -128) { acia_ikbd_send_mouse_relative(-128, 0); x_diff += 128; }
        while (y_diff > 127) { acia_ikbd_send_mouse_relative(0, 127); y_diff -= 127; }
        while (y_diff < -128) { acia_ikbd_send_mouse_relative(0, -128); y_diff += 128; }
        acia_ikbd_send_mouse_relative(x_diff, y_diff);
        acia_ikbd_send_done();
        atari_acia.last_mouse_x = atari_acia.mouse_x;
        atari_acia.last_mouse_y = atari_acia.mouse_y;
        atari_acia.mouse_button = 0;
    }
}

void acia_ikbd_set_mouse_pos(int32_t x, int32_t y) {
    if (atari_acia.mouse_mode == ACIA_MOUSE_MODE_ABSOLUTE) {
        if (x < 0) x = 0;
        else if (x > atari_acia.mouse_abs_xmax) x = atari_acia.mouse_abs_xmax;
        if (y < 0) y = 0;
        else if (y > atari_acia.mouse_abs_ymax) y = atari_acia.mouse_abs_ymax;
    }

    atari_acia.mouse_x = x;
    atari_acia.mouse_y = y;

    if (atari_acia.mouse_mode == ACIA_MOUSE_MODE_RELATIVE) {
        acia_ikbd_update_mouse_relative(false);
    }
}

void acia_ikbd_add_mouse_pos(int32_t x, int32_t y) {
    acia_ikbd_set_mouse_pos(x + atari_acia.mouse_x, y + atari_acia.mouse_y);
}

void acia_ikbd_set_mouse_button(uint8_t v) {
    uint8_t last_mouse_button = atari_acia.mouse_button;
        
    atari_acia.mouse_button_interrogate |= v;
    atari_acia.mouse_button |= (v & 0x05);

    if (last_mouse_button != atari_acia.mouse_button) {
        if (atari_acia.mouse_mode == ACIA_MOUSE_MODE_RELATIVE) {
            acia_ikbd_update_mouse_relative(true);
        }
    }

    last_mouse_button = atari_acia.mouse_button;

    if (v & ACIA_MOUSE_BUTTON_LEFT_UP) {
        atari_acia.mouse_button &= ~ACIA_MOUSE_BUTTON_LEFT_DOWN;
    }
    if (v & ACIA_MOUSE_BUTTON_RIGHT_UP) {
        atari_acia.mouse_button &= ~ACIA_MOUSE_BUTTON_RIGHT_DOWN;
    }

    if (last_mouse_button != atari_acia.mouse_button) {
        if (atari_acia.mouse_mode == ACIA_MOUSE_MODE_RELATIVE) {
            acia_ikbd_update_mouse_relative(true);
        }
    }
}