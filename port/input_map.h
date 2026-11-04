// Controller buttons -> DOS scancodes, the way the game expects keys.
#ifndef PORT_INPUT_MAP_H
#define PORT_INPUT_MAP_H

#include <stdint.h>

void input_init_scancodes(void);       // arrows + Ctrl (jump) + Alt (pogo)
void input_apply_buttons(uint8_t mask); // = input_apply_buttons_at(mask, 0)
// PORT_BTN_* -> g_input.key_map (held keys) and g_input.key_scane (press events).
void input_apply_buttons_at(uint8_t mask, uint32_t nowMs);
void input_reset_state(void);           // forget edges and pulses

#endif
