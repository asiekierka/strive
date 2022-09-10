#include <string.h>
#include "acia.h"

acia_t atari_acia;

void acia_init(void) {
    memset(&atari_acia, 0, sizeof(acia_t));
}

uint8_t acia_read8(uint8_t addr) {
    // TODO
    iprintf("todo: acia_read8 %02X\n", addr);
    switch (addr) {
    case 0x00:
        return atari_acia.ikbd_control;
    case 0x02:
        return 0;
    case 0x04:
        return atari_acia.midi_control;
    case 0x06:
        return 0;
    }
}

void acia_write8(uint8_t addr, uint8_t value) {
    // TODO
    iprintf("todo: acia_write8 %02X = %02X\n", addr, value);
    switch (addr) {
    case 0x00:
        atari_acia.ikbd_control = value; break;
    case 0x02:
        break;
    case 0x04:
        atari_acia.midi_control = value; break;
    case 0x06:
        break;
    }
}
