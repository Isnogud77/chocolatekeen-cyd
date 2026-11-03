// ESP32 (CYD) backend. Real display/input come in Task 6.
#include <Arduino.h>
#include <stdarg.h>
#include "backend.h"

extern "C" {
void port_init(void) {}
uint32_t port_millis(void) { return millis(); }
void port_sleep_ms(uint32_t ms) { delay(ms); }
uint8_t port_read_buttons(void) { return 0; }
void port_present(const uint8_t *src, int stride, const uint16_t pal565[16]) { (void)src; (void)stride; (void)pal565; }
void port_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}
void port_report_heap(const char *where) {
    port_log("heap @%s: free %u, largest %u", where, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
}
void port_fatal(const char *msg) {
    port_log("FATAL: %s", msg);
    for (;;) delay(1000);
}
}
