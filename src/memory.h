#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "platform.h"

extern uint8_t *memory_ram;
extern uint8_t *memory_rom;
#ifdef USE_VIDEO_MEMORY_DIRTY
extern uint8_t video_memory_dirty[448];
#endif
extern uint32_t memory_ram_mask;
extern uint8_t memory_io_bank_cfg;

uint8_t memory_read8(uint32_t address);
uint16_t memory_read16(uint32_t address);
uint32_t memory_read32(uint32_t address);
void memory_write8(uint32_t address, uint8_t value);
void memory_write16(uint32_t address, uint16_t value);
void memory_write32(uint32_t address, uint32_t value);

#ifdef STRIVE_68K_CYCLONE
uint32_t strive_cyclone_checkpc(uint32_t pc);
#endif