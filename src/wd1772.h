#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define WD1772_USE_HDC             0x0008
#define WD1772_ACCESS_SECTOR_COUNT 0x0010
#define WD1772_DMA_WRITE           0x0100

#define FDC_STATUS_BUSY             0x01
#define FDC_STATUS_DRQ              0x02
#define FDC_STATUS_INDEX_PULSE      0x02
#define FDC_STATUS_TRACK_0          0x04
#define FDC_STATUS_CRC_ERROR        0x08
#define FDC_STATUS_SEEK_ERROR       0x10
#define FDC_STATUS_RECORD_NOT_FOUND 0x10
#define FDC_STATUS_SPIN_UP          0x20
#define FDC_STATUS_RECORD_DELETED   0x20
#define FDC_STATUS_WRITE_PROTECT    0x40
#define FDC_STATUS_MOTOR_ON         0x80

typedef struct wd_fdc {
    FILE *file;

    uint8_t control, track_idx, sector_idx, status;
} wd_fdc_t;

typedef struct wd1772 {
    uint16_t dma_status;
    uint16_t dma_mode;
    uint32_t dta;
    uint8_t sector_count;
    wd_fdc_t fdc_a, fdc_b;
} wd1772_t;

extern wd1772_t atari_wd1772;

void wd1772_init(void);
uint8_t wd1772_read8(uint8_t addr);
void wd1772_write8(uint8_t addr, uint8_t value);
