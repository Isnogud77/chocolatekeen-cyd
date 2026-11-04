#include "letter_picker.h"

enum { SC_BACKSPACE = 0x0E, SC_ENTER = 0x1C, SC_SPACE = 0x39 };
enum { N_SYMS = 27 }; // A..Z, space

// DOS scancodes for A..Z (QWERTY layout).
static const uint8_t k_letterSc[26] = {
    0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
    0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C};

static bool s_active;
static bool s_tentative; // a not-yet-committed letter is on screen
static int s_cur;        // 0..26
static uint8_t s_pending; // letter scancode queued after a Backspace

static uint8_t sym_sc(int i) { return i == 26 ? SC_SPACE : k_letterSc[i]; }

void letter_picker_start(void) {
    s_active = true;
    s_tentative = false;
    s_cur = 0;
    s_pending = 0;
}

void letter_picker_stop(void) { s_active = false; s_pending = 0; }

bool letter_picker_active(void) { return s_active; }

uint8_t letter_picker_step(uint8_t e) {
    if (!s_active) return 0;
    if (s_pending) {
        uint8_t sc = s_pending;
        s_pending = 0;
        return sc;
    }
    if (e & PORT_BTN_JUMP) {
        s_active = false;
        return SC_ENTER;
    }
    if (e & (PORT_BTN_UP | PORT_BTN_DOWN)) {
        bool first = !s_tentative;
        if (!first) s_cur = (s_cur + ((e & PORT_BTN_UP) ? 1 : N_SYMS - 1)) % N_SYMS;
        s_tentative = true;
        if (first) return sym_sc(s_cur);
        s_pending = sym_sc(s_cur);
        return SC_BACKSPACE;
    }
    if (e & PORT_BTN_RIGHT) {
        // The shown letter is already in the buffer; commit it by starting a
        // fresh position at A.
        s_cur = 0;
        s_tentative = true;
        return sym_sc(0);
    }
    return 0;
}
