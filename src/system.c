#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "acia.h"
#include "memory.h"
#include "mfp.h"
#include "platform.h"
#include "platform_config.h"
#include "screen.h"
#include "system.h"
#include "wd1772.h"
#include "ym2149.h"

// #define TRACE

#ifdef TRACE
#include "Disa.h"
static FILE* trace_file;
#endif

static uint32_t system_cycle_count;

#ifdef STRIVE_68K_CYCLONE
#include "Cyclone.h"
struct Cyclone cpu_core;
static int cpu_cycles_running;
static int cpu_cycles_left;
static uint8_t mfp_interrupt_offset;

static inline void system_cpu_stop_inner(void) {
    cpu_cycles_left += cpu_core.cycles;
    cpu_core.cycles = 0;
}

void system_bus_error_inner(void) {
    if (cpu_core.irq != 7) {
        iprintf("bus error (%02X)\n", cpu_core.srh);

        cpu_core.irq = 7;
        system_cpu_stop_inner();
    }
}

void system_mfp_interrupt(uint8_t id) {
    cpu_core.irq = 6;
    mfp_interrupt_offset = 0x40 + id; // TODO: vector base
}

static int system_cpu_irq_callback(int int_level) {
    cpu_core.irq = 0;
    // iprintf("IRQ callback %d (%02X)\n", int_level, cpu_core.srh);

    switch (int_level) {
    case 7:
        // NMI - bus error
        return 2;
    case 6:
        // MFP interrupt
        return mfp_interrupt_offset;
    default:
        // HBLANK, VBLANK interrupt
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
    acia_init();
    wd1772_init();

    // init PC for 192K ROM
    CycloneReset(&cpu_core);
    uint32_t start_addr = 0xFC0000;
    cpu_core.membase = 0;
    cpu_core.pc = strive_cyclone_checkpc(start_addr);
    cpu_core.a[7] = start_addr + 4;
    cpu_core.IrqCallback = system_cpu_irq_callback;

    system_cycle_count = 0;
    cpu_cycles_left = 0;

    iprintf("init! membase = %08lX, pc = %06lX\n", cpu_core.membase, (cpu_core.pc - cpu_core.membase) & 0xFFFFFF);

#ifdef TRACE
    trace_file = fopen("/strive_trace.txt", "w");    
    DisaWord = system_disa_word;
    DisaText = malloc(1024);
#endif

    return true;
}

uint32_t system_cycles(void) {
    return system_cycle_count + cpu_cycles_running - cpu_core.cycles;
}

static void system_cpu_run(int cycles) {
    do {
        cpu_cycles_running = cycles + cpu_cycles_left;
        cpu_core.cycles += cpu_cycles_running;
        cycles = 0;
        cpu_cycles_left = 0;
        CycloneRun(&cpu_core);
        system_cycle_count += cpu_cycles_running - cpu_cycles_left;
    } while (cpu_cycles_left > 0);

    system_cycle_count -= cpu_core.cycles;
    cpu_core.cycles = 0;
}
#endif

bool system_init(void) {
    memory_ram = malloc(4096 * 1024);
    memory_rom = malloc(192 * 1024);

    if (!system_cpu_init()) return false;

    return true;
}

bool system_frame(void) {
    // iprintf("frame start! pc = %06lX, irq = %d\n", (cpu_core.pc - cpu_core.membase) & 0xFFFFFF, cpu_core.irq);

#ifdef TRACE
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
#else
    for (int y = 0; y < 313; y++) {
        if (y < 200 && cpu_core.irq < 2) {
            // Emit HBLANK
            cpu_core.irq = 2; 
        } else if (y == 200 && cpu_core.irq < 4) {
            // Emit VBLANK
            cpu_core.irq = 4;
        }
        system_cpu_run(512);
	mfp_advance(512);
    }    
#endif

    // iprintf("%d cycles\n", system_cycle_count);
    platform_gfx_draw_frame();

    return true;
}
