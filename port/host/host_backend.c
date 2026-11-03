// Host (Mac) backend. Real SDL2 window comes in Task 5.
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "backend.h"

void port_init(void) {}
uint32_t port_millis(void) { return 0; }
void port_sleep_ms(uint32_t ms) { (void)ms; }
uint8_t port_read_buttons(void) { return 0; }
void port_present(const uint8_t *src, int stride, const uint16_t pal565[16]) { (void)src; (void)stride; (void)pal565; }
void port_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}
void port_report_heap(const char *where) { port_log("heap @%s: n/a on host", where); }
_Noreturn void port_fatal(const char *msg) { port_log("FATAL: %s", msg); exit(1); }
