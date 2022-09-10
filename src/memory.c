#include <stdint.h>
#include <stdio.h>
#include "acia.h"
#include "memory.h"
#include "mfp.h"
#include "platform.h"
#include "screen.h"
#include "system.h"
#include "ym2149.h"

uint8_t *memory_ram;
uint8_t *memory_rom;
uint32_t memory_ram_mask = (4 * 1024 * 1024) - 1;
uint8_t memory_io_bank_cfg = 0b00001010;

static uint8_t io_read8(uint32_t address) {
    switch (address & 0xFFFFC0) {
    case 0xFFFA00:
        return mfp_read8(address);
    case 0xFF8200:
    case 0xFF8240:
        return screen_read8(address);
    case 0xFF8800:
        return ym2149_read8(address);
    case 0xFFFC00:
        return acia_read8(address);
    default:
        switch (address) {
        case 0xFF8001:
            return memory_io_bank_cfg;
        default:
            iprintf("unknown I/O read %06lX\n", address);
            platform_wait_key();
            system_bus_error_inner();
            return 0;
        }
    }
}

static uint16_t io_read16(uint32_t address) {
    return io_read8(address) | (io_read8(address + 1) << 8);
}

static uint32_t io_read32(uint32_t address) {
    return io_read8(address + 2) | (io_read8(address + 3) << 8) | (io_read8(address) << 16) | (io_read8(address + 1) << 24);
}

static void io_write8(uint32_t address, uint8_t value) {
    switch (address & 0xFFFFC0) {
    case 0xFFFA00:
        mfp_write8(address, value);
        break;
    case 0xFF8200:
    case 0xFF8240:
        screen_write8(address, value);
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
            iprintf("unknown I/O write %06lX = %02X\n", address, ((uint32_t) value) & 0xFF);
            platform_wait_key();
            system_bus_error_inner();
            break;
        }
    }
}

static void io_write16(uint32_t address, uint16_t value) {
    io_write8(address, value);
    io_write8(address + 1, value >> 8);
}

static void io_write32(uint32_t address, uint32_t value) {
    io_write8(address + 2, value);
    io_write8(address + 3, value >> 8);
    io_write8(address, value >> 16);
    io_write8(address + 1, value >> 24);
}

uint8_t memory_read8(uint32_t address) {
    if (address < 0xE00000) {
        return memory_ram[address & memory_ram_mask];
    } else if (address < 0xF00000) {
        return 0; // TODO: 512k ROMs
    } else if (address < 0xFA0000) {
        return 0; // TODO: io_read8(address);?
    } else if (address < 0xFC0000) {
        return 0; // TODO: 128K cartridge ROM
    } else if (address < 0xFF0000) {
        return memory_rom[address - 0xFC0000];
    } else {
        return io_read8(address);
    }
}

uint16_t memory_read16(uint32_t address) {
    if (address < 0xE00000) {
        return *((uint16_t*) &memory_ram[address & memory_ram_mask]);
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

uint32_t memory_read32(uint32_t address) {
    return (memory_read16(address) << 16) | memory_read16(address + 2);
}

void memory_write8(uint32_t address, uint8_t value) {
    if (address < 0xE00000) {
        memory_ram[address & memory_ram_mask] = value;
    } else if (/* (address >= 0xF00000 && address < 0xFA0000) ||  */address >= 0xFF0000) {
        return io_write8(address, value);
    } else {
        // iprintf("unknown write %06lX = %02X\n", address, ((uint32_t) value) & 0xFF);
    }
}

void memory_write16(uint32_t address, uint16_t value) {
    if (address < 0xE00000) {
        *((uint16_t*) &memory_ram[address & memory_ram_mask]) = value;
    } else if (/* (address >= 0xF00000 && address < 0xFA0000) ||  */address >= 0xFF0000) {
        return io_write16(address, value);
    } else {
        // iprintf("unknown write %06lX = %04X\n", address, ((uint32_t) value) & 0xFFFF);
    }
}

void memory_write32(uint32_t address, uint32_t value) {
    memory_write16(address, value >> 16);
    memory_write16(address + 2, value);
}

#ifdef STRIVE_68K_CYCLONE
#include "Cyclone.h"
extern struct Cyclone cpu_core;

uint32_t strive_cyclone_checkpc(uint32_t pc) {
    pc -= cpu_core.membase;

    if (pc < 0xE00000) {
        cpu_core.membase = ((uint32_t) memory_ram) - (pc & (~memory_ram_mask));
    } else if (pc >= 0xFC0000 && pc < 0xFF0000) {
        cpu_core.membase = ((uint32_t) memory_rom) - 0xFC0000;
    }

    return pc + cpu_core.membase;
}

uint32_t strive_cyclone_read8(uint32_t address) {
    return memory_read8(address);
}

uint32_t strive_cyclone_read16(uint32_t address) {
    return memory_read16(address);
}

uint32_t strive_cyclone_read32(uint32_t address) {
    return memory_read32(address);
}

uint32_t strive_cyclone_fetch8(uint32_t address) {
    return memory_read8(address);    
}

uint32_t strive_cyclone_fetch16(uint32_t address) {
    return memory_read16(address);
}

uint32_t strive_cyclone_fetch32(uint32_t address) {
    return memory_read32(address);    
}

void strive_cyclone_write8(uint32_t address, uint8_t value) {
    memory_write8(address, value);
}

void strive_cyclone_write16(uint32_t address, uint16_t value) {
    memory_write16(address, value);    
}

void strive_cyclone_write32(uint32_t address, uint32_t value) {
    memory_write32(address, value);
}

#endif
