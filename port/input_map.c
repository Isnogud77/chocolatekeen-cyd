// Controller buttons -> DOS scancodes. Jump + Pogo together = Ctrl + Alt,
// which upstream already turns into "fire".
#include "core/globals.h"
#include "backend.h"
#include "input_map.h"

enum { SC_UP = 0x48, SC_RIGHT = 0x4D, SC_DOWN = 0x50, SC_LEFT = 0x4B, SC_CTRL = 0x1D, SC_ALT = 0x38 };

void input_init_scancodes(void) {
    // Order: up, up-right, right, down-right, down, down-left, left, up-left.
    static const uint16_t dirs[8] = {0x48, 0x49, 0x4D, 0x51, 0x50, 0x4F, 0x4B, 0x47};
    for (int i = 0; i < 8; i++) g_input.sc_dir[i] = dirs[i];
    g_input.sc_but1 = SC_CTRL;
    g_input.sc_but2 = SC_ALT;
}

void input_apply_buttons(uint8_t mask) {
    g_input.key_map[SC_LEFT] = (mask & PORT_BTN_LEFT) != 0;
    g_input.key_map[SC_RIGHT] = (mask & PORT_BTN_RIGHT) != 0;
    g_input.key_map[SC_UP] = (mask & PORT_BTN_UP) != 0;
    g_input.key_map[SC_DOWN] = (mask & PORT_BTN_DOWN) != 0;
    g_input.key_map[SC_CTRL] = (mask & PORT_BTN_JUMP) != 0;
    g_input.key_map[SC_ALT] = (mask & PORT_BTN_POGO) != 0;
}

void CVort_engine_updateInputStatus() { input_apply_buttons(port_read_buttons()); }
