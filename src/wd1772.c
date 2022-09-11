#include <string.h>
#include "mfp.h"
#include "platform.h"
#include "platform_config.h"
#include "wd1772.h"
#include "ym2149.h"

wd1772_t atari_wd1772;

void wd1772_init(void) {
    memset(&atari_wd1772, 0, sizeof(wd1772_t));
}

static wd_fdc_t *wd1772_current_fdc() {
    uint8_t port_a = ym2149_get_port_a();
    switch (port_a & 0x06) {
    case 0x02:
        return &atari_wd1772.fdc_b;
    case 0x04:
        return &atari_wd1772.fdc_a;
    default:
        return NULL;
    }
}

static uint8_t fdc_read(wd_fdc_t *fdc, uint8_t addr) {
    if (fdc == NULL) return 0;
    // debug_printf("fdc read %d\n", addr);
    switch (addr) {
    case 0x00: /* Status */
        return fdc->status;
    case 0x02: /* Track */
        return fdc->track_idx;
    case 0x04: /* Sector */
        return fdc->sector_idx;
    case 0x06: /* Data */
        return 0;
    default:
        return 0;
    }
}

static void fdc_write(wd_fdc_t *fdc, uint8_t addr, uint8_t value) {
    if (fdc == NULL) return;
    // debug_printf("fdc write %d = %02X\n", addr, value);
    switch (addr) {
    case 0x00: /* Control */
        mfp_clear_interrupt(MFP_INT_ID_DISK);
        if ((value & 0xF0) == 0xD0) {
            /* Force Interrupt */
            if (value == 0xD8) {
                mfp_set_interrupt(MFP_INT_ID_DISK);
            } else if (value != 0xD0) {
                debug_printf("todo: fdc force interrupt %02X", value);
            }
        } else {
            switch (value & 0xF0) {
            case 0x00: /* Restore */
                fdc->status &= ~0x0E;
                if (fdc->file == NULL) {
                    fdc->status |= FDC_STATUS_SEEK_ERROR;
                } else {
                    fdc->track_idx = 0;
                    mfp_set_interrupt(MFP_INT_ID_DISK);
                }
                break;
            case 0x10: /* Seek */
                fdc->status &= ~0x0E;
                if (fdc->file == NULL) {
                    fdc->status |= FDC_STATUS_SEEK_ERROR;
                } else {
                    fdc->track_idx = value;
                    mfp_set_interrupt(MFP_INT_ID_DISK);
                }
                break;
            case 0x20: /* Step */
            case 0x30:
                fdc->status &= ~0x0E;
                debug_printf("todo: fdc step\n");
                break;
            case 0x40: /* Step-in */
            case 0x50:
                fdc->status &= ~0x0E;
                debug_printf("todo: fdc step-in\n");
                break;
            case 0x60: /* Step-out */
            case 0x70:
                fdc->status &= ~0x0E;
                debug_printf("todo: fdc step-out\n");
                break;
            case 0x80: /* Read Sector */
            case 0x90:
                debug_printf("todo: fdc read sector\n");
                break;
            case 0xA0: /* Write Sector */
            case 0xB0:
                debug_printf("todo: fdc write sector\n");
                break;
            case 0xC0: /* Read Address */
                debug_printf("todo: fdc read address\n");
                break;
            case 0xE0: /* Read Track */
                debug_printf("todo: fdc read track\n");
                break;
            case 0xF0: /* Write Track */
                debug_printf("todo: fdc write track\n");
                break;
            }
            if (value <= 0x80) {
                fdc->status &= ~FDC_STATUS_TRACK_0;
                if (fdc->track_idx == 0) {
                    fdc->status |= FDC_STATUS_TRACK_0;
                }
            }
        }
        break;
    case 0x02: /* Track */
        fdc->track_idx = value;
        break;
    case 0x04: /* Sector */
        fdc->sector_idx = value;
        break;
    case 0x06: /* Data */
        // TODO
        break;
    }
}

uint8_t wd1772_read8(uint8_t addr) {
    // TODO
    // debug_printf("wd1772: read %02X\n", addr);
    switch (addr) {
    case 0x04:
        return 0xFF;
    case 0x05: {
        if (atari_wd1772.dma_mode & WD1772_ACCESS_SECTOR_COUNT) {
            return 0xFF;
        } else {
            return fdc_read(wd1772_current_fdc(), atari_wd1772.dma_mode & 0x06);
        }
    }
    case 0x06:
        return atari_wd1772.dma_status >> 8;
    case 0x07:
        return atari_wd1772.dma_status;
    case 0x09:
        return atari_wd1772.dta >> 16;
    case 0x0B:
        return atari_wd1772.dta >> 8;
    case 0x0D:
        return atari_wd1772.dta;
    default:
        return 0;
    }
}

void wd1772_write8(uint8_t addr, uint8_t value) {
    // TODO
    // debug_printf("wd1772: write %02X = %02X\n", addr, value);
    switch (addr) {
    case 0x05:
        if (atari_wd1772.dma_mode & WD1772_ACCESS_SECTOR_COUNT) {
            atari_wd1772.sector_count = value;
        } else {
            atari_wd1772.dma_status &= ~0x01;
            if (atari_wd1772.dma_mode & WD1772_USE_HDC) {
                // TODO: HDC not implemented
            } else {
                fdc_write(wd1772_current_fdc(), atari_wd1772.dma_mode & 0x06, value);
            }
        }
        break;
    case 0x06:
        atari_wd1772.dma_mode = (atari_wd1772.dma_mode & 0x00FF) | (value << 8);
        break;
    case 0x07:
        atari_wd1772.dma_mode = (atari_wd1772.dma_mode & 0xFF00) | value;
        break;
    case 0x09:
        atari_wd1772.dta = (atari_wd1772.dta & 0x00FFFF) | ((value & 0x3F) << 16);
        break;
    case 0x0B:
        atari_wd1772.dta = (atari_wd1772.dta & 0xFF00FF) | (value << 8);
        break;
    case 0x0D:
        atari_wd1772.dta = (atari_wd1772.dta & 0xFFFF00) | (value & 0xFE);
        break;
    }
}
