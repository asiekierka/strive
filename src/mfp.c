#include <string.h>
#include <stdio.h>
#include "mfp.h"
#include "platform.h"
#include "system.h"

mfp_t atari_mfp;

void mfp_init(void) {
    memset(&atari_mfp, 0, sizeof(mfp_t));

    // emulate color monitor
    atari_mfp.gpio |= MFP_COLOR_MONITOR;
}

NDS_ITCM_CODE
uint8_t mfp_read8(uint8_t addr) {
    switch (addr) {
        case 0x01:
            return atari_mfp.gpio;
        case 0x03:
            return atari_mfp.active_edge;
        case 0x05:
            return atari_mfp.data_direction;
        case 0x07:
            return atari_mfp.intr.enable >> 8;
        case 0x09:
            return atari_mfp.intr.enable;
        case 0x0B:
            return atari_mfp.intr.pending >> 8;
        case 0x0D:
            return atari_mfp.intr.pending;
        case 0x0F:
            return atari_mfp.intr.in_service >> 8;
        case 0x11:
            return atari_mfp.intr.in_service;
        case 0x13:
            return atari_mfp.intr.mask >> 8;
        case 0x15:
            return atari_mfp.intr.mask;
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
            return 0x81;
        case 0x2F:
            return atari_mfp.uart.data;
        default:
            return 0;
    }
}

static uint8_t delay_modes[16] = {
    1, 4, 10, 16, 50, 64, 100, 200,
    1, 4, 10, 16, 50, 64, 100, 200
};

static void mfp_update_timer_a() {
    uint8_t mode = atari_mfp.timer.ctrl_a & 0x0F;
    if (mode > 0) {
        uint32_t mfp_ticks_until = atari_mfp.timer.data_a * delay_modes[mode];
        uint32_t cpu_ticks_until = mfp_ticks_until * 13 / 4;

        atari_mfp.timer.ticks_a = cpu_ticks_until;
        atari_mfp.timer.ticks_reset_a = cpu_ticks_until;
    }
}

static void mfp_update_timer_b() {
    uint8_t mode = atari_mfp.timer.ctrl_b & 0x0F;
    if (mode > 0) {
        uint32_t mfp_ticks_until = atari_mfp.timer.data_b * delay_modes[mode];

        // HACK: this should actually count down every hblank, not just assume
        uint32_t cpu_ticks_until = (mode & 8) ? (mfp_ticks_until * 512) : (mfp_ticks_until * 13 / 4);

        atari_mfp.timer.ticks_b = cpu_ticks_until;
        atari_mfp.timer.ticks_reset_b = cpu_ticks_until;
    }
}

static void mfp_update_timer_c() {
    uint8_t mode = (atari_mfp.timer.ctrl_cd >> 4) & 0x07;
    if (mode > 0) {
        uint32_t mfp_ticks_until = atari_mfp.timer.data_c * delay_modes[mode];
        uint32_t cpu_ticks_until = mfp_ticks_until * 13 / 4;

        // debug_printf("timer c runs for %d (%d) ticks (%d x %d)\n", mfp_ticks_until, cpu_ticks_until, atari_mfp.timer.data_c, mode);

        atari_mfp.timer.ticks_c = cpu_ticks_until;
        atari_mfp.timer.ticks_reset_c = cpu_ticks_until;
    }
}

static void mfp_update_timer_d() {
    uint8_t mode = atari_mfp.timer.ctrl_cd & 0x07;
    if (mode > 0) {
        uint32_t mfp_ticks_until = atari_mfp.timer.data_d * delay_modes[mode];
        uint32_t cpu_ticks_until = mfp_ticks_until * 13 / 4;

        atari_mfp.timer.ticks_d = cpu_ticks_until;
        atari_mfp.timer.ticks_reset_d = cpu_ticks_until;
    }
}

static void mfp_update_interrupts(void) {
    uint32_t pending = atari_mfp.intr.pending & atari_mfp.intr.mask;
    if (pending != 0) {
        int offset = 31 - __builtin_clz(pending);
        system_mfp_interrupt(offset);
        if (atari_mfp.vector_base & MFP_IN_SERVICE_ENABLE) {
            atari_mfp.intr.in_service |= (1 << offset);
            // TODO: clear in_service
        }
    }
}

void mfp_interrupt(uint8_t id) {
    uint16_t mask = 1 << id;
    if (atari_mfp.intr.enable & mask) {
        atari_mfp.intr.pending |= mask;
        mfp_update_interrupts();
    }
}

void mfp_ack_interrupt(uint8_t id) {
    atari_mfp.intr.pending &= ~(1 << id);
}

static const uint8_t mfp_int_id_to_index[8] = {
    0, 0, 0, 0, MFP_INT_B_ACIA, 0, 0, 0
};

void mfp_set_interrupt(uint8_t id) {
    uint8_t mask = (1 << id);
    // debug_printf("mfp: received ext int set %d\n", id);
    if (atari_mfp.gpio & mask) {
        atari_mfp.gpio &= ~mask;
        if (!(atari_mfp.active_edge & mask)) {
            system_mfp_interrupt(mfp_int_id_to_index[id]);
        }
    }
}

void mfp_clear_interrupt(uint8_t id) {
    uint8_t mask = (1 << id);
    // debug_printf("mfp: received ext int clr %d\n", id);
    if (!(atari_mfp.gpio & mask)) {
        atari_mfp.gpio |= mask;
        if (atari_mfp.active_edge & mask) {
            system_mfp_interrupt(mfp_int_id_to_index[id]);
        }
    }
}

NDS_ITCM_CODE
void mfp_advance(uint32_t ticks) {
    if ((atari_mfp.timer.ctrl_a & 0x0F) != 0 && atari_mfp.timer.data_a != 0) {
        atari_mfp.timer.ticks_a -= ticks;
        if (atari_mfp.timer.ticks_a <= 0) {
            atari_mfp.timer.ticks_a += atari_mfp.timer.ticks_reset_a;
            mfp_interrupt(MFP_INT_A_TIMER_A);
        }
    }

    if ((atari_mfp.timer.ctrl_b & 0x0F) != 0 && atari_mfp.timer.data_b != 0) {
        atari_mfp.timer.ticks_b -= ticks;
        if (atari_mfp.timer.ticks_b <= 0) {
            atari_mfp.timer.ticks_b += atari_mfp.timer.ticks_reset_b;
            mfp_interrupt(MFP_INT_A_TIMER_B);
        }
    }

    if ((atari_mfp.timer.ctrl_cd & 0x07) != 0 && atari_mfp.timer.data_c != 0) {
        atari_mfp.timer.ticks_c -= ticks;
        if (atari_mfp.timer.ticks_c <= 0) {
            atari_mfp.timer.ticks_c += atari_mfp.timer.ticks_reset_c;
            mfp_interrupt(MFP_INT_B_TIMER_C);
        }
    }

    if ((atari_mfp.timer.ctrl_cd & 0x70) != 0 && atari_mfp.timer.data_d != 0) {
        atari_mfp.timer.ticks_d -= ticks;
        if (atari_mfp.timer.ticks_d <= 0) {
            atari_mfp.timer.ticks_d += atari_mfp.timer.ticks_reset_d;
            mfp_interrupt(MFP_INT_B_TIMER_D);
        }
    }
}

NDS_ITCM_CODE
void mfp_write8(uint8_t addr, uint8_t value) {
    switch (addr) {
        case 0x03:
            atari_mfp.active_edge = value; break;
        case 0x05:
            atari_mfp.data_direction = value; break;
        case 0x07:
            atari_mfp.intr.enable = (atari_mfp.intr.enable & 0xFF) | (value << 8); break;
        case 0x09:
            atari_mfp.intr.enable = (atari_mfp.intr.enable & 0xFF00) | value; break;
        case 0x0B:
            atari_mfp.intr.pending = (atari_mfp.intr.pending & 0xFF) | (value << 8); mfp_update_interrupts(); break;
        case 0x0D:
            atari_mfp.intr.pending = (atari_mfp.intr.pending & 0xFF00) | value; mfp_update_interrupts(); break;
        case 0x0F:
            if (!(atari_mfp.vector_base & MFP_IN_SERVICE_ENABLE)) break;
            atari_mfp.intr.in_service &= (atari_mfp.intr.in_service & 0xFF) | (value << 8); mfp_update_interrupts(); break;
        case 0x11:
            if (!(atari_mfp.vector_base & MFP_IN_SERVICE_ENABLE)) break;
            atari_mfp.intr.in_service &= (atari_mfp.intr.in_service & 0xFF00) | value; mfp_update_interrupts(); break;
        case 0x13:
            atari_mfp.intr.mask = (atari_mfp.intr.mask & 0xFF) | (value << 8); mfp_update_interrupts(); break;
        case 0x15:
            atari_mfp.intr.mask = (atari_mfp.intr.mask & 0xFF00) | value; mfp_update_interrupts(); break;
        case 0x17:
            atari_mfp.vector_base = value;
            if (!(atari_mfp.vector_base & MFP_IN_SERVICE_ENABLE)) {
                atari_mfp.intr.in_service = 0;
            }
            break;
        case 0x19:
            atari_mfp.timer.ctrl_a = value & 0x0F; mfp_update_timer_a(); break;
        case 0x1B:
            atari_mfp.timer.ctrl_b = value & 0x0F; mfp_update_timer_b(); break;
        case 0x1D:
            atari_mfp.timer.ctrl_cd = value & 0x77; mfp_update_timer_c(); mfp_update_timer_d(); break;
        case 0x1F:
            atari_mfp.timer.data_a = value; mfp_update_timer_a(); break;
        case 0x21:
            atari_mfp.timer.data_b = value; mfp_update_timer_b(); break;
        case 0x23:
            atari_mfp.timer.data_c = value; mfp_update_timer_c(); break;
        case 0x25:
            atari_mfp.timer.data_d = value; mfp_update_timer_d(); break;
        case 0x27:
            atari_mfp.uart.sync = value; break;
        case 0x29:
            atari_mfp.uart.ctrl = value; break;
        case 0x2B:
            atari_mfp.uart.rx_status = value; break;
        case 0x2D:
            atari_mfp.uart.tx_status = value; break;
        case 0x2F:
            debug_printf("%c", value); break;
    }
}