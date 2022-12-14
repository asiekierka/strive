#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MFP_COLOR_MONITOR 0x80

#define MFP_IN_SERVICE_ENABLE 0x08

#define MFP_INT_ID_ACIA 4
#define MFP_INT_ID_DISK 5

#define MFP_INT_B_CENTRONICS_BUSY 0
#define MFP_INT_B_RS232_DCD       1
#define MFP_INT_B_RS232_CTS       2
#define MFP_INT_B_BLITTER_DONE    3
#define MFP_INT_B_TIMER_D         4
#define MFP_INT_B_TIMER_C         5
#define MFP_INT_B_ACIA            6
#define MFP_INT_B_DISK            7
#define MFP_INT_A_TIMER_B         8
#define MFP_INT_A_RS232_TX_ERROR  9
#define MFP_INT_A_RS232_TX_EMPTY  10
#define MFP_INT_A_RS232_RX_ERROR  11
#define MFP_INT_A_RS232_RX_FULL   12
#define MFP_INT_A_TIMER_A         13
#define MFP_INT_A_RS232_RING      14
#define MFP_INT_A_MONO_DETECT     15

typedef struct mfp_interrupt {
    uint16_t enable;
    uint16_t pending;
    uint16_t in_service;
    uint16_t mask;
} mfp_interrupt_t;

typedef struct mfp_timer {
    uint8_t ctrl_a, ctrl_b, ctrl_cd;
    uint8_t data_a, data_b, data_c, data_d;
    uint8_t data_reset_b;
   
    uint32_t ticks_start_a, ticks_end_a, ticks_step_a;
    uint32_t ticks_start_b, ticks_end_b, ticks_step_b;
    uint32_t ticks_start_c, ticks_end_c, ticks_step_c;
    uint32_t ticks_start_d, ticks_end_d, ticks_step_d;
} mfp_timer_t;

typedef struct mfp_uart {
    uint8_t sync;
    uint8_t ctrl;
    uint8_t rx_status;
    uint8_t tx_status;
    uint8_t data;
} mfp_uart_t;

typedef struct mfp {
    uint8_t gpio;
    uint8_t active_edge;
    uint8_t data_direction;

    mfp_interrupt_t intr;

    uint8_t vector_base;

    mfp_timer_t timer;

    mfp_uart_t uart;
} mfp_t;

extern mfp_t atari_mfp;

void mfp_init(void);
uint8_t mfp_read8(uint8_t addr);
void mfp_write8(uint8_t addr, uint8_t value);
void mfp_interrupt(uint8_t id);
void mfp_ack_interrupt(uint8_t id);
void mfp_set_interrupt(uint8_t id);
void mfp_clear_interrupt(uint8_t id);
void mfp_advance(uint32_t ticks);
void mfp_advance_hblank(void);