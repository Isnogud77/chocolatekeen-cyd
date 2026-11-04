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

// RAM key-value store standing in for NVS / host files (tests only).
#include <string.h>
#define FAKE_STORE_SLOTS 8
static struct { char name[16]; uint8_t data[2048]; int len; bool used; } s_fakeStore[FAKE_STORE_SLOTS];
void fake_store_reset(void) { memset(s_fakeStore, 0, sizeof s_fakeStore); }
static int fakeFind(const char *name) {
    for (int i = 0; i < FAKE_STORE_SLOTS; i++)
        if (s_fakeStore[i].used && !strcmp(s_fakeStore[i].name, name)) return i;
    return -1;
}
WEAK int port_store_load(const char *name, uint8_t *buf, int max) {
    int i = fakeFind(name);
    if (i < 0) return -1;
    int n = s_fakeStore[i].len < max ? s_fakeStore[i].len : max;
    memcpy(buf, s_fakeStore[i].data, n);
    return n;
}
WEAK bool port_store_save(const char *name, const uint8_t *buf, int len) {
    int i = fakeFind(name);
    for (int k = 0; i < 0 && k < FAKE_STORE_SLOTS; k++) if (!s_fakeStore[k].used) i = k;
    if (i < 0 || len > (int)sizeof s_fakeStore[i].data) return false;
    s_fakeStore[i].used = true;
    strncpy(s_fakeStore[i].name, name, sizeof s_fakeStore[i].name - 1);
    memcpy(s_fakeStore[i].data, buf, len);
    s_fakeStore[i].len = len;
    return true;
}
WEAK void port_store_erase(const char *name) {
    int i = fakeFind(name);
    if (i >= 0) s_fakeStore[i].used = false;
}
