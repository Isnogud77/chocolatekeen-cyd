// Controller buttons -> DOS scancodes, the way the game expects keys.
#ifndef PORT_INPUT_MAP_H
#define PORT_INPUT_MAP_H

#include <stdint.h>

void input_init_scancodes(void);       // arrows + Ctrl (jump) + Alt (pogo)
void input_apply_buttons(uint8_t mask); // PORT_BTN_* -> g_input.key_map

#endif
