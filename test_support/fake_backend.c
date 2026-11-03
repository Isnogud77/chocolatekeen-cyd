// Weak no-op backend for unit tests (env:test only). A test that needs to
// observe a call defines its own strong version.
#include <stdint.h>
#define WEAK __attribute__((weak))

WEAK void port_present(const uint8_t *src, int stride, const uint16_t pal565[16]) { (void)src; (void)stride; (void)pal565; }
WEAK uint32_t port_millis(void) { return 0; }
WEAK void port_sleep_ms(uint32_t ms) { (void)ms; }
WEAK uint8_t port_read_buttons(void) { return 0; }
WEAK void port_log(const char *fmt, ...) { (void)fmt; }
WEAK void CVort_engine_updateInputStatus(void) {}
WEAK void CK_PlatformSleepMs(unsigned int ms) { (void)ms; }
