#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MFP_COLOR_MONITOR 0x80

typedef struct mfp_interrupt {
    uint8_t enable;
    uint8_t pending;
    uint8_t in_service;
    uint8_t mask;
} mfp_interrupt_t;

typedef struct mfp_timer {
    uint8_t ctrl_a, ctrl_b, ctrl_cd;
    uint8_t data_a, data_b, data_c, data_d;
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

    mfp_interrupt_t int_a, int_b;

    uint8_t vector_base;

    mfp_timer_t timer;

    mfp_uart_t uart;
} mfp_t;

extern mfp_t atari_mfp;

void mfp_init(void);
uint8_t mfp_read8(uint8_t addr);
void mfp_write8(uint8_t addr, uint8_t value);