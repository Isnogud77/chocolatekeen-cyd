#include "letter_picker.h"

enum { SC_BACKSPACE = 0x0E, SC_ENTER = 0x1C, SC_SPACE = 0x39 };
enum { N_SYMS = 27 }; // A..Z, space

// DOS scancodes for A..Z (QWERTY layout).
static const uint8_t k_letterSc[26] = {
    0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
    0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C};

enum { QLEN = 8 };

static bool s_active;
static bool s_tentative; // a not-yet-committed letter is on screen
static bool s_finishing; // Enter queued; ignore further edges
static int s_cur;        // 0..26
static uint8_t s_q[QLEN];
static int s_qn;

static uint8_t sym_sc(int i) { return i == 26 ? SC_SPACE : k_letterSc[i]; }

static void push(uint8_t sc) {
    if (s_qn < QLEN) s_q[s_qn++] = sc;
}

void letter_picker_start(void) {
    s_active = true;
    s_tentative = false;
    s_finishing = false;
    s_cur = 0;
    s_qn = 0;
}

void letter_picker_stop(void) { s_active = false; s_finishing = false; s_qn = 0; }

bool letter_picker_active(void) { return s_active; }

uint8_t letter_picker_step(uint8_t e, bool slotFree) {
    if (!s_active) return 0;
    if (!s_finishing) {
        if (e & PORT_BTN_JUMP) {
            // Enter must never be dropped: overwrite the newest entry if full.
            if (s_qn == QLEN) s_qn--;
            push(SC_ENTER);
            s_finishing = true;
        } else if (e & (PORT_BTN_UP | PORT_BTN_DOWN)) {
            bool first = !s_tentative;
            if (!first) s_cur = (s_cur + ((e & PORT_BTN_UP) ? 1 : N_SYMS - 1)) % N_SYMS;
            s_tentative = true;
            if (!first) push(SC_BACKSPACE);
            push(sym_sc(s_cur));
        } else if (e & PORT_BTN_RIGHT) {
            // The shown letter is already in the buffer; commit it by starting
            // a fresh position at A.
            s_cur = 0;
            s_tentative = true;
            push(sym_sc(0));
        }
    }
    if (!slotFree || s_qn == 0) return 0;
    uint8_t sc = s_q[0];
    for (int i = 1; i < s_qn; i++) s_q[i - 1] = s_q[i];
    s_qn--;
    if (sc == SC_ENTER) s_active = false;
    return sc;
}
