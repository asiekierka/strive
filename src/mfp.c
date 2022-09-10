#include <string.h>
#include "mfp.h"

mfp_t atari_mfp;

void mfp_init(void) {
    memset(&atari_mfp, 0, sizeof(mfp_t));

    // emulate color monitor
    atari_mfp.gpio |= MFP_COLOR_MONITOR;
}

uint8_t mfp_read8(uint8_t addr) {
    switch (addr) {
        case 0x01:
            return atari_mfp.gpio;
        case 0x03:
            return atari_mfp.active_edge;
        case 0x05:
            return atari_mfp.data_direction;
        case 0x07:
            return atari_mfp.int_a.enable;
        case 0x09:
            return atari_mfp.int_b.enable;
        case 0x0B:
            return atari_mfp.int_a.pending;
        case 0x0D:
            return atari_mfp.int_b.pending;
        case 0x0F:
            return atari_mfp.int_a.in_service;
        case 0x11:
            return atari_mfp.int_b.in_service;
        case 0x13:
            return atari_mfp.int_a.mask;
        case 0x15:
            return atari_mfp.int_b.mask;
        case 0x17:
            return atari_mfp.vector_base;
        case 0x19:
            return atari_mfp.timer.ctrl_a;
        case 0x1B:
            return atari_mfp.timer.ctrl_b;
        case 0x1D:
            return atari_mfp.timer.ctrl_cd;
        case 0x1F:
            return atari_mfp.timer.data_a;
        case 0x21:
            return atari_mfp.timer.data_b;
        case 0x23:
            return atari_mfp.timer.data_c;
        case 0x25:
            return atari_mfp.timer.data_d;
        case 0x27:
            return atari_mfp.uart.sync;
        case 0x29:
            return atari_mfp.uart.ctrl;
        case 0x2B:
            return atari_mfp.uart.rx_status;
        case 0x2D:
            return atari_mfp.uart.tx_status;
        case 0x2F:
            return atari_mfp.uart.data;
        default:
            return 0;
    }
}

void mfp_write8(uint8_t addr, uint8_t value) {
    switch (addr) {
        case 0x01:
            atari_mfp.gpio = (atari_mfp.gpio & (~0x20)) | (value & 0x20); break;
        case 0x03:
            atari_mfp.active_edge = value; break;
        case 0x05:
            atari_mfp.data_direction = value; break;
        case 0x07:
            atari_mfp.int_a.enable = value; break;
        case 0x09:
            atari_mfp.int_b.enable = value; break;
        case 0x0B:
            atari_mfp.int_a.pending = value; break;
        case 0x0D:
            atari_mfp.int_b.pending = value; break;
        case 0x0F:
            atari_mfp.int_a.in_service = value; break;
        case 0x11:
            atari_mfp.int_b.in_service = value; break;
        case 0x13:
            atari_mfp.int_a.mask = value; break;
        case 0x15:
            atari_mfp.int_b.mask = value; break;
        case 0x17:
            atari_mfp.vector_base = value; break;
        case 0x19:
            atari_mfp.timer.ctrl_a = value; break;
        case 0x1B:
            atari_mfp.timer.ctrl_b = value; break;
        case 0x1D:
            atari_mfp.timer.ctrl_cd = value; break;
        case 0x1F:
            atari_mfp.timer.data_a = value; break;
        case 0x21:
            atari_mfp.timer.data_b = value; break;
        case 0x23:
            atari_mfp.timer.data_c = value; break;
        case 0x25:
            atari_mfp.timer.data_d = value; break;
        case 0x27:
            atari_mfp.uart.sync = value; break;
        case 0x29:
            atari_mfp.uart.ctrl = value; break;
        case 0x2B:
            atari_mfp.uart.rx_status = value; break;
        case 0x2D:
            atari_mfp.uart.tx_status = value; break;
        case 0x2F:
            atari_mfp.uart.data = value; break;
    }
}