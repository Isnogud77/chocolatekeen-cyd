// The seam between game code and the board (ESP32) or the Mac (host).
#ifndef PORT_BACKEND_H
#define PORT_BACKEND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Button bits use the same order as the MCP23017 GPA pins.
enum {
    PORT_BTN_LEFT = 1 << 0,
    PORT_BTN_RIGHT = 1 << 1,
    PORT_BTN_UP = 1 << 2,
    PORT_BTN_DOWN = 1 << 3,
    PORT_BTN_JUMP = 1 << 4,
    PORT_BTN_POGO = 1 << 5,
};

void port_init(void);                 // display, input, timer
uint32_t port_millis(void);
void port_sleep_ms(uint32_t ms);
uint8_t port_read_buttons(void);      // PORT_BTN_* mask, 1 = pressed
// Show a 320x200 window of 8-bit EGA indices; stride in bytes per row.
void port_present(const uint8_t *src, int stride, const uint16_t pal565[16]);
void port_log(const char *fmt, ...);
void port_report_heap(const char *where);
_Noreturn void port_fatal(const char *msg);
// Guards state shared with the audio timer/callback (pcspeaker.c).
void port_audio_lock(void);
void port_audio_unlock(void);

#ifdef __cplusplus
}
#endif
#endif
