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
NDS_DTCM_BSS
struct Cyclone cpu_core;
static int cpu_cycles_overrun;
static int cpu_cycles_left;
static uint8_t mfp_interrupt_offset;

static inline void system_cpu_stop_inner(void) {
    cpu_cycles_left += cpu_core.cycles;
    cpu_core.cycles = 0;
}

void system_bus_error_inner(void) {
    if (cpu_core.irq < 7) {
        iprintf("bus error (%02X)\n", cpu_core.srh);

        cpu_core.irq = 7;
        system_cpu_stop_inner();
    }
}

void system_mfp_interrupt(uint8_t id) {
    if (cpu_core.irq < 6) {
        cpu_core.irq = 6;
        mfp_interrupt_offset = id;
    }   
}

void system_mfp_interrupt_inner(uint8_t id) {
    system_mfp_interrupt(id);
    system_cpu_stop_inner();
}

static int system_cpu_irq_callback(int int_level) {
    cpu_core.irq = 0;
#ifdef TRACE
    if (int_level == 6)
        fprintf(trace_file, "[!] IRQ callback %d (vb%02X, p%d)\n", int_level, atari_mfp.vector_base, mfp_interrupt_offset);
    else
        fprintf(trace_file, "[!] IRQ callback %d\n", int_level);
#endif

    switch (int_level) {
    case 7:
        // NMI - bus error
        return 2;
    case 6:
        // MFP interrupt
        mfp_ack_interrupt(mfp_interrupt_offset);
        // TODO: vector base
        return (atari_mfp.vector_base & 0xF0) | mfp_interrupt_offset;
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
    screen_init();
    acia_init();
    wd1772_init();
    ym2149_init();

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
    return system_cycle_count;
}

NDS_ITCM_CODE
static void system_cpu_run(int cycles) {
    while (cycles > 0) {
        cpu_core.cycles += cycles;
        cpu_cycles_left = 0;
        uint32_t cycles_start = cpu_core.cycles;
        CycloneRun(&cpu_core);
        uint32_t cycles_performed = (cycles_start - cpu_core.cycles) - cpu_cycles_left;
        system_cycle_count += cycles_performed;
        cycles -= cycles_performed;
    }
}
#endif

bool system_init(void) {
    memory_ram = malloc(memory_ram_mask + 1);
    memory_rom = malloc(192 * 1024);

    if (!system_cpu_init()) return false;

    return true;
}

NDS_ITCM_CODE
bool system_frame(void) {
    // iprintf("frame start! pc = %06lX, irq = %d\n", (cpu_core.pc - cpu_core.membase) & 0xFFFFFF, cpu_core.irq)

    platform_gfx_draw_frame();

    acia_advance(512 * 313);
    for (int y = 0; y < 313; y++) {
        if (y < 200 && cpu_core.irq < 2) {
            // Emit HBLANK
            cpu_core.irq = 2; 
        } else if (y == 200 && cpu_core.irq < 4) {
            // Emit VBLANK
            cpu_core.irq = 4;
        }
        int cycles = system_cycle_count;
        int cycles_expected = 512 - cpu_cycles_overrun;
#ifdef TRACE
        while ((system_cycle_count - cycles) < cycles_expected) {
            system_cpu_run(1);
            uint32_t pc = (cpu_core.pc - cpu_core.membase) & 0xFFFFFF;
            DisaPc = pc;
            DisaGet();
            fprintf(trace_file, "%06X\t[a0=%08X a1=%08X a4=%08X a7=%08X i%d]\t%s\n", pc,
                cpu_core.a[0], cpu_core.a[1], cpu_core.a[4], cpu_core.a[7], cpu_core.irq,
                DisaText);
        }
#else
        system_cpu_run(cycles_expected);
#endif
        cycles = system_cycle_count - cycles;
        cpu_cycles_overrun = cycles - cycles_expected;
        mfp_advance(cycles);
    }

#ifdef TRACE
    fflush(trace_file);
#endif

    // iprintf("%d cycles\n", system_cycle_count);

    return true;
}
