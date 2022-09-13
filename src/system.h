#pragma once

#include <stdbool.h>
#include <stdint.h>

bool system_init(void);
bool system_frame(void);
bool system_mfp_interrupt(uint8_t id);
void system_mfp_interrupt_inner(uint8_t id);
void system_bus_error_inner(void);
uint32_t system_cycles(void);
int16_t system_line_y(void);