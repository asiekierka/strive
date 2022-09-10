#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "mfp.h"
#include "platform.h"
#include "platform_config.h"
#include "screen.h"
#include "system.h"

// #define TRACE

#ifdef TRACE
#include "Disa.h"
static FILE* trace_file;
#endif

#ifdef STRIVE_68K_CYCLONE
#include "Cyclone.h"
struct Cyclone cpu_core;
static int cpu_cycles_left;

void system_bus_error_inner(void) {
    if (cpu_core.irq != 7) {
        iprintf("bus error (%02X)\n", cpu_core.srh);

        cpu_core.irq = 7;
        cpu_cycles_left += cpu_core.cycles;
        cpu_core.cycles = 0;
    }
}

static int system_cpu_irq_callback(int int_level) {
    cpu_core.irq = 0;
    iprintf("IRQ callback %d (%02X)\n", int_level, cpu_core.srh);

    if (int_level == 7) {
        return 2;
    } else {
        return CYCLONE_INT_ACK_AUTOVECTOR;
    }
}

#ifdef TRACE
static uint16_t system_disa_word(uint32_t a) {
    return memory_read16(a);
}
#endif

static bool system_cpu_init(void) {
    CycloneInit();
    memset(&cpu_core, 0, sizeof(cpu_core));

    // load 192k ROM
    FILE *f = fopen(ROM_FILE, "rb");
    if (f == NULL) return false;
    if (fread(memory_rom, 192 * 1024, 1, f) <= 0) {
        fclose(f);
        return false;
    }
    fclose(f);

    // byteswap 192K ROM
    for (int i = 0; i < 192 * 1024; i += 2) {
        uint8_t t = memory_rom[i];
        memory_rom[i] = memory_rom[i + 1];
        memory_rom[i + 1] = t;
    }

    // init components
    mfp_init();

    // init PC for 192K ROM
    CycloneReset(&cpu_core);
    uint32_t start_addr = 0xFC0000;
    cpu_core.membase = 0;
    cpu_core.pc = strive_cyclone_checkpc(start_addr);
    cpu_core.a[7] = start_addr + 4;
    cpu_core.IrqCallback = system_cpu_irq_callback;

    cpu_cycles_left = 0;

    iprintf("init! membase = %08lX, pc = %06lX\n", cpu_core.membase, (cpu_core.pc - cpu_core.membase) & 0xFFFFFF);

#ifdef TRACE
    trace_file = fopen("/strive_trace.txt", "w");    
    DisaWord = system_disa_word;
    DisaText = malloc(1024);
#endif

    return true;
}

static void system_cpu_run(int cycles) {
    cpu_core.cycles += cpu_cycles_left + cycles;
    cpu_cycles_left = 0;
    CycloneRun(&cpu_core);
}
#endif

bool system_init(void) {
    memory_ram = malloc(4096 * 1024);
    memory_rom = malloc(192 * 1024);

    if (!system_cpu_init()) return false;

    return true;
}

bool system_frame(void) {
#ifndef TRACE
    system_cpu_run(32084988 / 240);
#else
    iprintf("frame start! pc = %06lX, irq = %d\n", (cpu_core.pc - cpu_core.membase) & 0xFFFFFF, cpu_core.irq);

    for (int i = 0; i < 15000; i++) {
        system_cpu_run(1);
        uint32_t pc = (cpu_core.pc - cpu_core.membase) & 0xFFFFFF;
        DisaPc = pc;
        DisaGet();
        fprintf(trace_file, "%06X\t[a0=%08X a1=%08X a2=%08X a3=%08X]\t%s\n", pc,
            cpu_core.a[0], cpu_core.a[1], cpu_core.a[2], cpu_core.a[3],
            DisaText);
    }
    fflush(trace_file);
#endif

    if (cpu_core.irq < 4) {
        iprintf("screen @ %08X, res %d (%02X)\n", atari_screen.base, atari_screen.resolution, atari_mfp.gpio);
        cpu_core.irq = 4; // vblank
    }
    platform_gfx_draw_frame();

    return true;
}
