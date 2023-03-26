#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "mfp.h"
#include "platform.h"
#include "platform_config.h"
#include "wd1772.h"
#include "ym2149.h"

// .ST format

bool wd_fdc_st_read_sector(wd_fdc_t *fdc, int track, int side, int sector, uint8_t *buffer) {
    int offset = ((((track * fdc->f_sides) + side) * fdc->f_sectors_per_track) + sector) * 512;
    if (fseek(fdc->file, offset, SEEK_SET)) return false;
    return fread(buffer, 512, 1, fdc->file) > 0;
}

static const wd_fdc_format_t fdc_format_st = {
    .read_sector = wd_fdc_st_read_sector
};

static bool wd_fdc_st_open(wd_fdc_t *fdc, FILE *file) {
    fdc->f_starting_track = 0;
    fdc->f_format = &fdc_format_st;

    // calculate file size
    fseek(fdc->file, 0, SEEK_END);
    uint32_t filesize = ftell(fdc->file);
    
    switch (filesize) {
    case 360*1024:
        fdc->f_sectors_per_track = 9;
        fdc->f_sides = 1;
        fdc->f_ending_track = 79;
        break;
    case 370*1024:
        fdc->f_sectors_per_track = 9;
        fdc->f_sides = 1;
        fdc->f_ending_track = 81;
        break;
    case 400*1024:
        fdc->f_sectors_per_track = 10;
        fdc->f_sides = 1;
        fdc->f_ending_track = 79;
        break;
    case 410*1024:
        fdc->f_sectors_per_track = 10;
        fdc->f_sides = 1;
        fdc->f_ending_track = 81;
        break;
    case 720*1024:
        fdc->f_sectors_per_track = 9;
        fdc->f_sides = 2;
        fdc->f_ending_track = 79;
        break;
    case 740*1024:
        fdc->f_sectors_per_track = 9;
        fdc->f_sides = 2;
        fdc->f_ending_track = 81;
        break;
    case 800*1024:
        fdc->f_sectors_per_track = 10;
        fdc->f_sides = 2;
        fdc->f_ending_track = 79;
        break;
    case 810*1024:
        fdc->f_sectors_per_track = 10;
        fdc->f_sides = 2;
        fdc->f_ending_track = 80;
        break;
    case 820*1024:
        fdc->f_sectors_per_track = 10;
        fdc->f_sides = 2;
        fdc->f_ending_track = 81;
        break;
    default:
        debug_printf("wd_fdc_st: unrecognized size %ld\n", filesize);
        return false;
    }
    return true;
}

// .MSA format

static uint16_t fread16be(FILE *file) {
    uint8_t a = fgetc(file);
    uint8_t b = fgetc(file);
    return (a << 8) | b;
}

bool wd_fdc_msa_read_sector(wd_fdc_t *fdc, int track, int side, int sector, uint8_t *buffer) {
    int track_offset = ((track - fdc->f_starting_track) * fdc->f_sides) + side;
    int sector_bytes = 512 * fdc->f_sectors_per_track;
    if (fseek(fdc->file, ((uint32_t*) fdc->data)[track_offset], SEEK_SET)) return false;
    uint16_t track_len = fread16be(fdc->file);
    if (track_len == sector_bytes) {
        // uncompressed track
        if (fseek(fdc->file, 512 * sector, SEEK_CUR)) return false;
        return fread(buffer, 512, 1, fdc->file) > 0;
    } else {
        // compressed track, uncompress first
        // TODO: cache this?
        int decompress_ofs = -(512 * sector);
        while (decompress_ofs < 512) {
            uint8_t bt = fgetc(fdc->file);
            if (bt == 0xE5) {
                bt = fgetc(fdc->file);
                uint16_t count = fread16be(fdc->file);
                if (count > (512 - decompress_ofs)) {
                    count = (512 - decompress_ofs);
                }
                for (int j = 0; j < count; j++) {
                    if ((decompress_ofs++) >= 0) *(buffer++) = bt;
                }
            } else {
                if ((decompress_ofs++) >= 0) *(buffer++) = bt;
            }
        }
        return true;
    }
}

static const wd_fdc_format_t fdc_format_msa = {
    .read_sector = wd_fdc_msa_read_sector
};

static bool wd_fdc_msa_open(wd_fdc_t *fdc, FILE *file) {
    fseek(fdc->file, 0, SEEK_SET);

    // check ID marker
    if (fread16be(file) != 0x0E0F) return false;

    // read data
    fdc->f_sectors_per_track = fread16be(file);
    fdc->f_sides = fread16be(file) + 1;
    fdc->f_starting_track = fread16be(file);
    fdc->f_ending_track = fread16be(file);
    if (fdc->f_starting_track >= fdc->f_ending_track || (fdc->f_sides < 1 || fdc->f_sides > 2)) return false;

    // generate offsets
    int track_offset_count = (fdc->f_ending_track - fdc->f_starting_track + 1) * fdc->f_sides;
    uint32_t *track_offsets = malloc(sizeof(uint32_t) * track_offset_count);
    for (int i = 0; i < track_offset_count; i++) {
        track_offsets[i] = ftell(file);
        uint16_t track_len = fread16be(file);
        if (fseek(file, track_len, SEEK_CUR)) {
            free(track_offsets);
            return false;
        }
    }
    debug_printf("wd_fdc_msa: opened with %d tracks (%ds, %ds, %d-%d)\n", track_offset_count, fdc->f_sectors_per_track, fdc->f_sides, fdc->f_starting_track, fdc->f_ending_track);

    fdc->f_format = &fdc_format_msa;
    fdc->data = track_offsets;

    return true;
}

// FDC/WD1772 code

static void wd_fdc_close(wd_fdc_t *fdc) {
    if (fdc->file != NULL) {
        fclose(fdc->file);
        fdc->file = NULL;
    }

    if (fdc->data != NULL) {
        free(fdc->data);
        fdc->data = NULL;
    }
}

#define HINT_EXTENSION_IS(hf, ext) (strcasecmp((hf) + strlen(hf) - strlen(ext), (ext)) == 0)

void wd_fdc_open(wd_fdc_t *fdc, FILE *file, const char *hint_filename) {
    wd_fdc_close(fdc);

    fdc->file = file;
    fdc->f_head = 0;

    if (file != NULL) {
#ifdef STRIVE_SETVBUF_SIZE
        setvbuf(file, NULL, _IOFBF, STRIVE_SETVBUF_SIZE);
#endif
        if (HINT_EXTENSION_IS(hint_filename, ".msa") && wd_fdc_msa_open(fdc, file)) {
            return;
        } else if (HINT_EXTENSION_IS(hint_filename, ".st") && wd_fdc_st_open(fdc, file)) {
            return;
        } else {
            // hint extension check failed - autodetection attempt

            if (!wd_fdc_st_open(fdc, file)) {
                if (!wd_fdc_msa_open(fdc, file)) {
                    wd_fdc_close(fdc);
                    return;
                }
            }
        }
    }
}

wd1772_t atari_wd1772;

static void wd1772_dma_reset(void) {
    atari_wd1772.sector_count = 0;
    atari_wd1772.dma_status = 1;
}

void wd1772_init(void) {
    memset(&atari_wd1772, 0, sizeof(wd1772_t));
    wd1772_dma_reset();
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

static uint8_t wd1772_current_side() {
    uint8_t port_a = ym2149_get_port_a();
    return (port_a & 0x01) ^ 0x01;
}

static uint8_t fdc_read(wd_fdc_t *fdc, uint8_t addr) {
    if (fdc == NULL) return 0;
    // debug_printf("fdc read %d\n", addr);
    switch (addr) {
    case 0x00: { /* Status */
        uint8_t status = atari_wd1772.fdc_status;
        atari_wd1772.fdc_status &= ~FDC_STATUS_BUSY;
        // debug_printf("fdc status is %02X\n", status);
        return status;
    }
    case 0x02: /* Track */
        return atari_wd1772.fdc_track_idx;
    case 0x04: /* Sector */
        return atari_wd1772.fdc_sector_idx;
    case 0x06: /* Data */
        return atari_wd1772.fdc_data;
    default:
        return 0;
    }
}

static void fdc_move_head(wd_fdc_t *fdc, int offset) {
    atari_wd1772.fdc_dir = offset == 0 ? 0 : (offset < 0 ? -1 : 1);
    fdc->f_head += offset;

    uint8_t span = fdc->f_ending_track + 1 - fdc->f_starting_track;
    while (fdc->f_head < fdc->f_starting_track) fdc->f_head += span;
    while (fdc->f_head > fdc->f_ending_track) fdc->f_head -= span;
}

static void fdc_write(wd_fdc_t *fdc, uint8_t addr, uint8_t value) {
    if (fdc == NULL) return;
    // debug_printf("fdc write %d = %02X\n", addr, value);

    switch (addr) {
    case 0x00: /* Control */
        // debug_printf("fdc command %02X\n", value);
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
                // debug_printf("fdc: restore\n");
                atari_wd1772.fdc_status &= ~0x0E;
                atari_wd1772.fdc_status |= FDC_STATUS_BUSY;
                if (fdc->file == NULL || fdc->f_starting_track > 0) {
                    atari_wd1772.fdc_status |= FDC_STATUS_SEEK_ERROR;
                }
                fdc->f_head = 0;
                atari_wd1772.fdc_track_idx = 0;
                mfp_set_interrupt(MFP_INT_ID_DISK);
                break;
            case 0x10: /* Seek */
                // debug_printf("fdc: seek from %d to %d\n", atari_wd1772.fdc_track_idx, atari_wd1772.fdc_data);
                atari_wd1772.fdc_status &= ~0x0E;
                atari_wd1772.fdc_status |= FDC_STATUS_BUSY;
                if (fdc->file == NULL) {
                    atari_wd1772.fdc_status |= FDC_STATUS_SEEK_ERROR;
                } else {
                    fdc_move_head(fdc, atari_wd1772.fdc_data - atari_wd1772.fdc_track_idx);
                    atari_wd1772.fdc_track_idx = atari_wd1772.fdc_data;
                }
                mfp_set_interrupt(MFP_INT_ID_DISK);
                break;
            case 0x20: /* Step */
            case 0x30:
                atari_wd1772.fdc_status &= ~0x0E;
                atari_wd1772.fdc_status |= FDC_STATUS_BUSY;
                if (fdc->file == NULL) {
                    atari_wd1772.fdc_status |= FDC_STATUS_SEEK_ERROR;
                } else {
                    fdc_move_head(fdc, atari_wd1772.fdc_dir);
                    if (value & 0x10) atari_wd1772.fdc_track_idx += atari_wd1772.fdc_dir;
                }
                mfp_set_interrupt(MFP_INT_ID_DISK);
                break;
            case 0x40: /* Step-in */
            case 0x50:
                atari_wd1772.fdc_status &= ~0x0E;
                atari_wd1772.fdc_status |= FDC_STATUS_BUSY;
                if (fdc->file == NULL) {
                    atari_wd1772.fdc_status |= FDC_STATUS_SEEK_ERROR;
                } else {
                    fdc_move_head(fdc, 1);
                    if (value & 0x10) atari_wd1772.fdc_track_idx++;
                }
                mfp_set_interrupt(MFP_INT_ID_DISK);
                break;
            case 0x60: /* Step-out */
            case 0x70:
                atari_wd1772.fdc_status &= ~0x0E;
                atari_wd1772.fdc_status |= FDC_STATUS_BUSY;
                if (fdc->file == NULL) {
                    atari_wd1772.fdc_status |= FDC_STATUS_SEEK_ERROR;
                } else {
                    fdc_move_head(fdc, -1);
                    if (value & 0x10) atari_wd1772.fdc_track_idx--;
                }
                mfp_set_interrupt(MFP_INT_ID_DISK);
                break;
            case 0x80: /* Read Sector */
            case 0x90:
                // debug_printf("fdc: read sector\n");
                atari_wd1772.fdc_status &= ~0x7F;
                atari_wd1772.fdc_status |= FDC_STATUS_BUSY;
                if (fdc->file == NULL
                    || atari_wd1772.fdc_sector_idx > fdc->f_sectors_per_track
                ) {
                    atari_wd1772.fdc_status |= FDC_STATUS_RECORD_NOT_FOUND;
                } else {
                    int last_sector = (value & 0x10) ? fdc->f_sectors_per_track : atari_wd1772.fdc_sector_idx;
                    while (atari_wd1772.fdc_sector_idx <= last_sector) {
                        if (atari_wd1772.sector_count == 0) break;

                        uint8_t *buffer = memory_ram + atari_wd1772.dta;
                        if (!fdc->f_format->read_sector(
                            fdc, atari_wd1772.fdc_track_idx, wd1772_current_side(), atari_wd1772.fdc_sector_idx - 1, buffer
                        )) {
                            atari_wd1772.fdc_status |= FDC_STATUS_RECORD_NOT_FOUND;
                            break;
                        }

                        // byteswap loaded data
                        for (int i = 0; i < 512; i += 2) {
                            uint8_t t = buffer[i];
                            buffer[i] = buffer[i + 1];
                            buffer[i + 1] = t;
                        }

                        atari_wd1772.fdc_sector_idx++;
                        atari_wd1772.dta += 512;

                        atari_wd1772.sector_count--;
                        atari_wd1772.dma_status |= 0x01;
                    }
                    mfp_set_interrupt(MFP_INT_ID_DISK);
                    // debug_printf("sector read ok %06X %04X %02X\n", atari_wd1772.dta, atari_wd1772.dma_status, atari_wd1772.fdc_status);
                }
                break;
            case 0xA0: /* Write Sector */
            case 0xB0:
                atari_wd1772.fdc_status &= ~0x7F;
                atari_wd1772.fdc_status |= 0x10;
                debug_printf("todo: fdc write sector\n");
                break;
            case 0xC0: /* Read Address */
                atari_wd1772.fdc_status &= ~0x7F;
                atari_wd1772.fdc_status |= 0x10;
                debug_printf("todo: fdc read address\n");
                break;
            case 0xE0: /* Read Track */
                atari_wd1772.fdc_status &= ~0x7F;
                atari_wd1772.fdc_status |= 0x10;
                debug_printf("todo: fdc read track\n");
                break;
            case 0xF0: /* Write Track */
                atari_wd1772.fdc_status &= ~0x7F;
                atari_wd1772.fdc_status |= 0x10;
                debug_printf("todo: fdc write track\n");
                break;
            }
            atari_wd1772.dma_status &= ~0x02;
            if (atari_wd1772.sector_count > 0) {
                atari_wd1772.dma_status |= 0x02;
            }
            if (value < 0x80) {
                atari_wd1772.fdc_status &= ~FDC_STATUS_TRACK_0;
                if (atari_wd1772.fdc_track_idx == 0) {
                    atari_wd1772.fdc_status |= FDC_STATUS_TRACK_0;
                }
            }
        }
        break;
    case 0x02: /* Track */
        atari_wd1772.fdc_track_idx = value;
        break;
    case 0x04: /* Sector */
        atari_wd1772.fdc_sector_idx = value;
        break;
    case 0x06: /* Data */
        atari_wd1772.fdc_data = value;
        break;
    }
}

uint8_t wd1772_read8(uint8_t addr) {
    // TODO
    // if (addr >= 0x06 || (atari_wd1772.dma_mode & WD1772_ACCESS_SECTOR_COUNT)) debug_printf("wd1772: read %02X %04X\n", addr, atari_wd1772.dma_status);
    switch (addr) {
    case 0x04:
        return 0xFF;
    case 0x05: {
        if (atari_wd1772.dma_mode & WD1772_ACCESS_SECTOR_COUNT) {
            return 0xFF;
        } else {
            if (atari_wd1772.dma_mode & WD1772_USE_HDC) {
                // TODO: HDC not implemented
                return 0xFF;
            } else {
                return fdc_read(wd1772_current_fdc(), atari_wd1772.dma_mode & 0x06);
            }
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
    // if (addr >= 0x06 || (atari_wd1772.dma_mode & WD1772_ACCESS_SECTOR_COUNT)) debug_printf("wd1772: write %02X = %02X\n", addr, value);
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
    case 0x06: {
        uint16_t old_dma_mode = atari_wd1772.dma_mode;
        atari_wd1772.dma_mode = (atari_wd1772.dma_mode & 0x00FF) | (value << 8);
        if ((old_dma_mode ^ atari_wd1772.dma_mode) & 0x100) {
            wd1772_dma_reset();
        }
    } break;
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
