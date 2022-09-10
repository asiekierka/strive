#pragma once

#include <stdbool.h>
#include <stdint.h>

bool system_init(void);
bool system_frame(void);
void system_bus_error_inner(void);