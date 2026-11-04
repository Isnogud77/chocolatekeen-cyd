// Arcade-style letter picker for the high score name: Up/Down cycle the
// current letter, Right commits it and moves on, b1 finishes.
#ifndef PORT_LETTER_PICKER_H
#define PORT_LETTER_PICKER_H

#include <stdbool.h>
#include <stdint.h>

#include "backend.h"

void letter_picker_start(void);
void letter_picker_stop(void);
bool letter_picker_active(void);
// pressedEdges: PORT_BTN_* bits that went down this frame. Returns the
// scancode to emit as a key event now (0 = none). Events go through a FIFO and
// are released one at a time, only when slotFree (upstream consumed the last).
uint8_t letter_picker_step(uint8_t pressedEdges, bool slotFree);

#endif
