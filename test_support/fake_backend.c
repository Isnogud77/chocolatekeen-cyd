// Weak no-op backend for unit tests (env:test only). A test that needs to
// observe a call defines its own strong version.
#include <stdbool.h>
#include <stdint.h>
#include "backend.h"
#define WEAK __attribute__((weak))

WEAK void port_present(const ScreenLayout_T *layout, const uint8_t *src, int stride, const uint16_t pal565[16]) { (void)layout; (void)src; (void)stride; (void)pal565; }
WEAK uint32_t port_millis(void) { return 0; }
WEAK void port_sleep_ms(uint32_t ms) { (void)ms; }
WEAK uint8_t port_read_buttons(void) { return 0; }
WEAK void port_log(const char *fmt, ...) { (void)fmt; }
WEAK void CVort_engine_updateInputStatus(void) {}
WEAK void CK_PlatformSleepMs(unsigned int ms) { (void)ms; }
WEAK void port_init(void) {}
WEAK void port_report_heap(const char *where) { (void)where; }
WEAK void port_fatal(const char *msg) { (void)msg; for (;;) {} }
WEAK void port_audio_lock(void) {}
WEAK void port_audio_unlock(void) {}
// Tests flip this to simulate a backend whose audio tick source failed.
bool fake_audio_available = true;
WEAK bool port_audio_available(void) { return fake_audio_available; }
