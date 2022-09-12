#include <stdint.h>
#include <stdio.h>
#include "acia.h"
#include "memory.h"
#include "mfp.h"
#include "platform.h"
#include "platform_config.h"
#include "screen.h"
#include "system.h"
#include "wd1772.h"
#include "ym2149.h"

#ifdef USE_VIDEO_MEMORY_DIRTY
NDS_DTCM_BSS
uint8_t video_memory_dirty[448];
#endif
NDS_DTCM_BSS
uint8_t *memory_ram;
NDS_DTCM_BSS
uint8_t *memory_rom;
NDS_DTCM_DATA
uint32_t memory_ram_mask = (1 * 1024 * 1024) - 1;
uint8_t memory_io_bank_cfg = 0b00001010;

NDS_ITCM_CODE
static uint8_t io_read8(uint32_t address) {
    // if (address != 0xFFFA11 && (address < 0xFFFA27 || address >= 0xFFFA30)) debug_printf("I/O read %06lX\n", address);
    
    switch (address & 0xFFFFC0) {
    case 0xFFFA00:
        return mfp_read8(address);
    case 0xFF8200:
    case 0xFF8240:
        return screen_read8(address);
    case 0xFF8600:
        return wd1772_read8(address);
    case 0xFF8800:
        return ym2149_read8(address);
    case 0xFFFC00:
        return acia_read8(address);
    default:
        switch (address) {
        case 0xFF8001:
            return memory_io_bank_cfg;
        default:
            debug_printf("unknown I/O read %06lX\n", address);
            // platform_wait_key();
            system_bus_error_inner();
            return 0;
        }
    }
}

NDS_ITCM_CODE
static inline uint16_t io_read16(uint32_t address) {
    return io_read8(address + 1) | (io_read8(address) << 8);
}

NDS_ITCM_CODE
static void io_write8(uint32_t address, uint8_t value) {
    // if (address != 0xFFFA11 && (address < 0xFFFA27 || address >= 0xFFFA30)) debug_printf("I/O write %06lX = %02X\n", address, ((uint32_t) value) & 0xFF);

    switch (address & 0xFFFFC0) {
    case 0xFFFA00:
        mfp_write8(address, value);
        break;
    case 0xFF8200:
    case 0xFF8240:
        screen_write8(address, value);
        break;
    case 0xFF8600:
        wd1772_write8(address, value);
        break;
    case 0xFF8800:
        ym2149_write8(address, value);
        break;
    case 0xFFFC00:
        acia_write8(address, value);
        break;
    default:
        switch (address) {
        case 0xFF8001:
            memory_io_bank_cfg = value;
            break;
        default:
            debug_printf("unknown I/O write %06lX = %02lX\n", address, ((uint32_t) value) & 0xFF);
            // platform_wait_key();
            system_bus_error_inner();
            break;
        }
    }
}

NDS_ITCM_CODE
static inline void io_write16(uint32_t address, uint16_t value) {
    io_write8(address, value >> 8);
    io_write8(address + 1, value);
}

NDS_ITCM_CODE
uint8_t memory_read8(uint32_t address) {
    if (address < 0x400000) {
        return memory_ram[address ^ 1];
    } else if (address < 0xE00000) {
        system_bus_error_inner();
        return 0;
    } else if (address < 0xF00000) {
        return 0; // TODO: 512k ROMs
    } else if (address < 0xFA0000) {
        return 0; // TODO: io_read8(address);?
    } else if (address < 0xFC0000) {
        return 0; // TODO: 128K cartridge ROM
    } else if (address < 0xFF0000) {
        return memory_rom[(address ^ 1) - 0xFC0000];
    } else {
        return io_read8(address);
    }
}

NDS_ITCM_CODE
uint16_t memory_read16(uint32_t address) {
    if (address < 0x400000) {
        return *((uint16_t*) &memory_ram[address & memory_ram_mask]);
    } else if (address < 0xE00000) {
        system_bus_error_inner();
        return 0;
    } else if (address < 0xF00000) {
        return 0; // TODO: 512k ROMs
    } else if (address < 0xFA0000) {
        return 0; // TODO: io_read16(address);?
    } else if (address < 0xFC0000) {
        return 0; // TODO: 128K cartridge ROM
    } else if (address < 0xFF0000) {
        return *((uint16_t*) &memory_rom[address - 0xFC0000]);
    } else {
        return io_read16(address);
    }
}

NDS_ITCM_CODE
uint32_t memory_read32(uint32_t address) {
    return (memory_read16(address) << 16) | memory_read16(address + 2);
}

NDS_ITCM_CODE
void memory_write8(uint32_t address, uint8_t value) {
    if (address < 0x400000) {
#ifdef USE_VIDEO_MEMORY_DIRTY
        video_memory_dirty[address >> 15] = 1;
#endif
        memory_ram[address ^ 1] = value;
    } else if (address < 0xE00000) {
        system_bus_error_inner();
    } else if (/* (address >= 0xF00000 && address < 0xFA0000) ||  */address >= 0xFF0000) {
        return io_write8(address, value);
    } else {
        // debug_printf("unknown write %06lX = %02X\n", address, ((uint32_t) value) & 0xFF);
    }
}

NDS_ITCM_CODE
void memory_write16(uint32_t address, uint16_t value) {
    if (address < 0x400000) {
#ifdef USE_VIDEO_MEMORY_DIRTY
        video_memory_dirty[address >> 15] = 1;
#endif
        *((uint16_t*) &memory_ram[address]) = value;
    } else if (address < 0xE00000) {
        system_bus_error_inner();
    } else if (/* (address >= 0xF00000 && address < 0xFA0000) ||  */address >= 0xFF0000) {
        return io_write16(address, value);
    } else {
        // debug_printf("unknown write %06lX = %04X\n", address, ((uint32_t) value) & 0xFFFF);
    }
}

NDS_ITCM_CODE
void memory_write32(uint32_t address, uint32_t value) {
    memory_write16(address, value >> 16);
    memory_write16(address + 2, value);
}

#ifdef STRIVE_68K_CYCLONE
#include "Cyclone.h"
extern struct Cyclone cpu_core;

NDS_ITCM_CODE
uint32_t strive_cyclone_checkpc(uint32_t pc) {
    pc -= cpu_core.membase;

    if (pc < 0xE00000) {
        cpu_core.membase = ((uint32_t) memory_ram) - (pc & (~memory_ram_mask));
    } else if (pc >= 0xFC0000 && pc < 0xFF0000) {
        cpu_core.membase = ((uint32_t) memory_rom) - 0xFC0000;
    }

    return pc + cpu_core.membase;
}

NDS_ITCM_CODE
uint32_t strive_cyclone_read8(uint32_t address) {
    return memory_read8(address);
}

NDS_ITCM_CODE
uint32_t strive_cyclone_read16(uint32_t address) {
    return memory_read16(address);
}

NDS_ITCM_CODE
uint32_t strive_cyclone_read32(uint32_t address) {
    return memory_read32(address);
}

NDS_ITCM_CODE
uint32_t strive_cyclone_fetch8(uint32_t address) {
    return memory_read8(address);    
}

NDS_ITCM_CODE
uint32_t strive_cyclone_fetch16(uint32_t address) {
    return memory_read16(address);
}

NDS_ITCM_CODE
uint32_t strive_cyclone_fetch32(uint32_t address) {
    return memory_read32(address);    
}

NDS_ITCM_CODE
void strive_cyclone_write8(uint32_t address, uint8_t value) {
    memory_write8(address, value);
}

NDS_ITCM_CODE
void strive_cyclone_write16(uint32_t address, uint16_t value) {
    memory_write16(address, value);    
}

NDS_ITCM_CODE
void strive_cyclone_write32(uint32_t address, uint32_t value) {
    memory_write32(address, value);
}

#endif
