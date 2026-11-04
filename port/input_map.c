// Controller buttons -> DOS scancodes. Jump + Pogo together = Ctrl + Alt,
// which upstream already turns into "fire".
#include "core/globals.h"
#include "backend.h"
#include "input_map.h"
#include "play_context.h"

enum { SC_UP = 0x48, SC_RIGHT = 0x4D, SC_DOWN = 0x50, SC_LEFT = 0x4B, SC_CTRL = 0x1D, SC_ALT = 0x38,
       SC_ENTER = 0x1C, SC_ESC = 0x01, SC_SPACE = 0x39, SC_Y = 0x15, SC_N = 0x31 };

enum { LONG_PRESS_MS = 1000, PULSE_MS = 100 };

// Per-button state: edge detection and the long-press pulse window per hold.
static uint8_t s_prev;
static uint32_t s_pressAt[8];

void input_init_scancodes(void) {
    // Order: up, up-right, right, down-right, down, down-left, left, up-left.
    static const uint16_t dirs[8] = {0x48, 0x49, 0x4D, 0x51, 0x50, 0x4F, 0x4B, 0x47};
    for (int i = 0; i < 8; i++) g_input.sc_dir[i] = dirs[i];
    g_input.sc_but1 = SC_CTRL;
    g_input.sc_but2 = SC_ALT;
}

void input_reset_state(void) {
    s_prev = 0;
    for (int i = 0; i < 8; i++) { s_pressAt[i] = 0; }
}

// True while the single pulse of a long hold of button bit b is active.
static bool pulse_active(int b, uint32_t nowMs) {
    uint32_t held = nowMs - s_pressAt[b];
    if (held < LONG_PRESS_MS) return false;
    return held < LONG_PRESS_MS + PULSE_MS;
}

static void key_event(uint16_t sc) { g_input.key_scane = sc | 0x80; }

void input_apply_buttons_at(uint8_t mask, uint32_t nowMs) {
    uint8_t pressed = mask & ~s_prev;
    for (int b = 0; b < 8; b++) {
        if (pressed & (1 << b)) { s_pressAt[b] = nowMs; }
    }
    bool play = play_context_in_play();
    bool level = play_context_in_level();
    bool b1 = (mask & PORT_BTN_JUMP) != 0, b2 = (mask & PORT_BTN_POGO) != 0;
    bool start = (mask & PORT_BTN_START) != 0, menu = (mask & PORT_BTN_MENU) != 0;

    g_input.key_map[SC_LEFT] = (mask & PORT_BTN_LEFT) != 0;
    g_input.key_map[SC_RIGHT] = (mask & PORT_BTN_RIGHT) != 0;
    g_input.key_map[SC_UP] = (mask & PORT_BTN_UP) != 0;
    g_input.key_map[SC_DOWN] = (mask & PORT_BTN_DOWN) != 0;
    g_input.key_map[SC_CTRL] = play && b1;
    g_input.key_map[SC_ALT] = play && b2;
    g_input.key_map[SC_Y] = !play && b1;
    g_input.key_map[SC_N] = !play && b2;
    g_input.key_map[SC_ENTER] = start || (!play && b1);
    g_input.key_map[SC_ESC] = menu || (!play && b2);
    g_input.key_map[SC_SPACE] = !play && b1;

    if (play && b2 && pulse_active(5, nowMs)) g_input.key_map[SC_ESC] = 1;
    if (level && (mask & PORT_BTN_DOWN) && pulse_active(3, nowMs)) g_input.key_map[SC_SPACE] = 1;

    // Press events: one per press edge, never while the key stays held.
    if (pressed & PORT_BTN_JUMP) { if (!play) key_event(SC_Y); }
    if (pressed & PORT_BTN_POGO) { if (!play) key_event(SC_ESC); }
    if (pressed & PORT_BTN_START) key_event(SC_ENTER);
    if (pressed & PORT_BTN_MENU) key_event(SC_ESC);
    s_prev = mask;
}

void input_apply_buttons(uint8_t mask) { input_apply_buttons_at(mask, 0); }

void CVort_engine_updateInputStatus() { input_apply_buttons_at(port_read_buttons(), port_millis()); }
