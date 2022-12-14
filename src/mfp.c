#include <string.h>
#include <stdio.h>
#include "mfp.h"
#include "platform.h"
#include "platform_config.h"
#include "system.h"

NDS_DTCM_BSS
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
        case 0x1F: {
            uint8_t data = atari_mfp.timer.data_a;
            if (data != 0 && (atari_mfp.timer.ctrl_a & 0x0F)) {
                data -= (system_cycles() - atari_mfp.timer.ticks_start_a) / atari_mfp.timer.ticks_step_a;
                if (data < 1) data = 1;
            }
            return data;
        }
        case 0x21: {
            uint8_t data = atari_mfp.timer.data_b;
            if (data != 0 && (atari_mfp.timer.ctrl_b & 0x0F) && !(atari_mfp.timer.ctrl_b & 0x08)) {
                data -= (system_cycles() - atari_mfp.timer.ticks_start_b) / atari_mfp.timer.ticks_step_b;
                if (data < 1) data = 1;
            }
            return data;
        }
        case 0x23: {
            uint8_t data = atari_mfp.timer.data_c;
            if (data != 0 && (atari_mfp.timer.ctrl_cd & 0x70)) {
                data -= (system_cycles() - atari_mfp.timer.ticks_start_c) / atari_mfp.timer.ticks_step_c;
                if (data < 1) data = 1;
            }
            return data;
        }
        case 0x25: {
            uint8_t data = atari_mfp.timer.data_d;
            if (data != 0 && (atari_mfp.timer.ctrl_cd & 0x07)) {
                data -= (system_cycles() - atari_mfp.timer.ticks_start_d) / atari_mfp.timer.ticks_step_d;
                if (data < 1) data = 1;
            }
            return data;
        }
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

        atari_mfp.timer.ticks_step_a = delay_modes[mode] * 13 / 4;
        atari_mfp.timer.ticks_start_a = system_cycles();
        atari_mfp.timer.ticks_end_a = atari_mfp.timer.ticks_start_a + cpu_ticks_until;
    }
}

static void mfp_update_timer_b() {
    uint8_t mode = atari_mfp.timer.ctrl_b & 0x0F;
    if (mode > 0) {
        uint32_t mfp_ticks_until = atari_mfp.timer.data_b * delay_modes[mode];

        // HACK: this should actually count down every hblank, not just assume
        uint32_t cpu_ticks_until = (mode & 8) ? (mfp_ticks_until * 512) : (mfp_ticks_until * 13 / 4);

        atari_mfp.timer.ticks_step_b = cpu_ticks_until / atari_mfp.timer.data_b;
        atari_mfp.timer.ticks_start_b = system_cycles();
        atari_mfp.timer.ticks_end_b = atari_mfp.timer.ticks_start_b + cpu_ticks_until;
    }
}

static void mfp_update_timer_c() {
    uint8_t mode = (atari_mfp.timer.ctrl_cd >> 4) & 0x07;
    if (mode > 0) {
        uint32_t mfp_ticks_until = atari_mfp.timer.data_c * delay_modes[mode];
        uint32_t cpu_ticks_until = mfp_ticks_until * 13 / 4;

        atari_mfp.timer.ticks_step_c = delay_modes[mode] * 13 / 4;
        atari_mfp.timer.ticks_start_c = system_cycles();
        atari_mfp.timer.ticks_end_c = atari_mfp.timer.ticks_start_c + cpu_ticks_until;
    }
}

static void mfp_update_timer_d() {
    uint8_t mode = (atari_mfp.timer.ctrl_cd) & 0x07;
    if (mode > 0) {
        uint32_t mfp_ticks_until = atari_mfp.timer.data_d * delay_modes[mode];
        uint32_t cpu_ticks_until = mfp_ticks_until * 13 / 4;

        atari_mfp.timer.ticks_step_d = delay_modes[mode] * 13 / 4;
        atari_mfp.timer.ticks_start_d = system_cycles();
        atari_mfp.timer.ticks_end_d = atari_mfp.timer.ticks_start_d + cpu_ticks_until;
    }
}

NDS_ITCM_CODE
static void mfp_update_interrupts(void) {
    while (true) {
        uint32_t pending = atari_mfp.intr.pending & atari_mfp.intr.mask;
        if (pending == 0) return;

        int offset = 31 - __builtin_clz(pending);

        uint16_t mask_above_offset = ~((1 << (offset + 1)) - 1);
        if (atari_mfp.intr.in_service & mask_above_offset) {
            return;
        }

        if (system_mfp_interrupt(offset)) {
            if (atari_mfp.vector_base & MFP_IN_SERVICE_ENABLE) {
                atari_mfp.intr.in_service |= (1 << offset);
            }
        }

        return;
    }
}

NDS_ITCM_CODE
void mfp_interrupt(uint8_t id) {
    uint16_t mask = 1 << id;
    if (atari_mfp.intr.enable & mask) {
        atari_mfp.intr.pending |= mask;
        mfp_update_interrupts();
    }
}

NDS_ITCM_CODE
void mfp_ack_interrupt(uint8_t id) {
    atari_mfp.intr.pending &= ~(1 << id);
}

static const uint8_t mfp_int_id_to_index[8] = {
    0, 0, 0, 0, MFP_INT_B_ACIA, MFP_INT_B_DISK, 0, 0
};

void mfp_set_interrupt(uint8_t id) {
    uint8_t mask = (1 << id);
    if (atari_mfp.gpio & mask) {
        atari_mfp.gpio &= ~mask;
        if (!(atari_mfp.active_edge & mask)) {
            mfp_interrupt(mfp_int_id_to_index[id]);
            // debug_printf("mfp: queued ^ interrupt %d\n", mfp_int_id_to_index[id]);
        }
    }
}

void mfp_clear_interrupt(uint8_t id) {
    uint8_t mask = (1 << id);
    if (!(atari_mfp.gpio & mask)) {
        atari_mfp.gpio |= mask;
        if (atari_mfp.active_edge & mask) {
            mfp_interrupt(mfp_int_id_to_index[id]);
            // debug_printf("mfp: queued v interrupt %d\n", mfp_int_id_to_index[id]);
        }
    }
}

NDS_ITCM_CODE
void mfp_advance_hblank(void) {
    if (atari_mfp.timer.data_b && (atari_mfp.timer.ctrl_b & 0x08)) {
        if ((--atari_mfp.timer.data_b) == 0) {
            atari_mfp.timer.data_b = atari_mfp.timer.data_reset_b;
            mfp_interrupt(MFP_INT_A_TIMER_B);
        }
    }
}

NDS_ITCM_CODE
void mfp_advance(uint32_t ticks) {
    if ((atari_mfp.timer.ctrl_a & 0x0F) && atari_mfp.timer.data_a) {
        if (((int) (ticks - atari_mfp.timer.ticks_end_a)) >= 0) {
            uint32_t ticks_full_step = atari_mfp.timer.ticks_end_a - atari_mfp.timer.ticks_start_a;
            atari_mfp.timer.ticks_start_a += ticks_full_step;
            atari_mfp.timer.ticks_end_a += ticks_full_step;
            mfp_interrupt(MFP_INT_A_TIMER_A);
        }
    }

    if ((atari_mfp.timer.ctrl_b & 0x0F) && !(atari_mfp.timer.ctrl_b & 0x08) && atari_mfp.timer.data_b) {
        if (((int) (ticks - atari_mfp.timer.ticks_end_b)) >= 0) {
            uint32_t ticks_full_step = atari_mfp.timer.ticks_end_b - atari_mfp.timer.ticks_start_b;
            atari_mfp.timer.ticks_start_b += ticks_full_step;
            atari_mfp.timer.ticks_end_b += ticks_full_step;
            mfp_interrupt(MFP_INT_A_TIMER_B);
        }
    }

    if ((atari_mfp.timer.ctrl_cd & 0x70) && atari_mfp.timer.data_c) {
        if (((int) (ticks - atari_mfp.timer.ticks_end_c)) >= 0) {
            uint32_t ticks_full_step = atari_mfp.timer.ticks_end_c - atari_mfp.timer.ticks_start_c;
            atari_mfp.timer.ticks_start_c += ticks_full_step;
            atari_mfp.timer.ticks_end_c += ticks_full_step;
            mfp_interrupt(MFP_INT_B_TIMER_C);
        }
    }

    if ((atari_mfp.timer.ctrl_cd & 0x07) && atari_mfp.timer.data_d) {
        if (((int) (ticks - atari_mfp.timer.ticks_end_d)) >= 0) {
            uint32_t ticks_full_step = atari_mfp.timer.ticks_end_d - atari_mfp.timer.ticks_start_b;
            atari_mfp.timer.ticks_start_d += ticks_full_step;
            atari_mfp.timer.ticks_end_d += ticks_full_step;
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
            atari_mfp.intr.enable = (atari_mfp.intr.enable & 0xFF) | (value << 8); atari_mfp.intr.pending &= ((value << 8) | 0xFF); mfp_update_interrupts(); break;
        case 0x09:
            atari_mfp.intr.enable = (atari_mfp.intr.enable & 0xFF00) | value; atari_mfp.intr.pending &= (value | 0xFF00); mfp_update_interrupts(); break;
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
            atari_mfp.timer.data_b = atari_mfp.timer.data_reset_b = value; mfp_update_timer_b(); break;
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