// "File on a blob": upstream's save/score code keeps using FILE*, the bytes
// live in the port store (NVS on the ESP32, files on the host).
#if !defined(__APPLE__)
#define _GNU_SOURCE // fopencookie
#endif
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "backend.h"
#include "save_store.h"

static bool served(const char *name) {
    return !strncasecmp(name, "SAVED1.", 7) || !strncasecmp(name, "SCORES.", 7);
}

typedef struct {
    char name[16];
    uint8_t *buf;
    int len;
    bool overflow;
} Writer_T;

static int writerWrite(Writer_T *w, const char *data, int n) {
    if (w->len + n > SAVE_STORE_MAX) {
        w->overflow = true;
        return -1;
    }
    memcpy(w->buf + w->len, data, n);
    w->len += n;
    return n;
}

static int writerClose(Writer_T *w) {
    bool ok = !w->overflow && port_store_save(w->name, w->buf, w->len);
    if (!ok) {
        static bool logged;
        if (!logged) port_log("store: saving %s failed", w->name);
        logged = true;
    }
    free(w->buf);
    free(w);
    return ok ? 0 : -1;
}

#if defined(__APPLE__)
static int cbWrite(void *c, const char *d, int n) { return writerWrite((Writer_T *)c, d, n); }
static int cbClose(void *c) { return writerClose((Writer_T *)c); }
#else
static ssize_t cbWrite(void *c, const char *d, size_t n) { return writerWrite((Writer_T *)c, d, (int)n); }
static int cbClose(void *c) { return writerClose((Writer_T *)c); }
#endif

static FILE *openWriter(const char *name) {
    Writer_T *w = (Writer_T *)calloc(1, sizeof *w);
    if (!w) return NULL;
    w->buf = (uint8_t *)malloc(SAVE_STORE_MAX);
    if (!w->buf) { free(w); return NULL; }
    strncpy(w->name, name, sizeof w->name - 1);
#if defined(__APPLE__)
    FILE *fp = funopen(w, NULL, cbWrite, NULL, cbClose);
#else
    cookie_io_functions_t io = {NULL, cbWrite, NULL, cbClose};
    FILE *fp = fopencookie(w, "wb", io);
#endif
    if (!fp) { free(w->buf); free(w); }
    return fp;
}

// Reader: the blob is copied into a heap buffer owned by the FILE's lifetime.
typedef struct { uint8_t *buf; int len, pos; } Reader_T;

static int readerRead(Reader_T *r, char *out, int n) {
    int left = r->len - r->pos;
    if (n > left) n = left;
    memcpy(out, r->buf + r->pos, n);
    r->pos += n;
    return n;
}
static int readerClose(Reader_T *r) { free(r->buf); free(r); return 0; }

static long readerSeek(Reader_T *r, long off, int whence) {
    long base = whence == SEEK_SET ? 0 : whence == SEEK_CUR ? r->pos : r->len;
    long p = base + off;
    if (p < 0 || p > r->len) return -1;
    r->pos = (int)p;
    return p;
}

#if defined(__APPLE__)
static int cbRead(void *c, char *d, int n) { return readerRead((Reader_T *)c, d, n); }
static int cbRClose(void *c) { return readerClose((Reader_T *)c); }
static fpos_t cbSeek(void *c, fpos_t off, int whence) { return (fpos_t)readerSeek((Reader_T *)c, (long)off, whence); }
#else
static ssize_t cbRead(void *c, char *d, size_t n) { return readerRead((Reader_T *)c, d, (int)n); }
static int cbRClose(void *c) { return readerClose((Reader_T *)c); }
static int cbSeek(void *c, off_t *off, int whence) {
    long p = readerSeek((Reader_T *)c, (long)*off, whence);
    if (p < 0) return -1;
    *off = p;
    return 0;
}
#endif

static FILE *openReader(const char *name) {
    Reader_T *r = (Reader_T *)calloc(1, sizeof *r);
    if (!r) return NULL;
    r->buf = (uint8_t *)malloc(SAVE_STORE_MAX);
    if (!r->buf) { free(r); return NULL; }
    r->len = port_store_load(name, r->buf, SAVE_STORE_MAX);
    if (r->len < 0) { free(r->buf); free(r); return NULL; }
#if defined(__APPLE__)
    FILE *fp = funopen(r, cbRead, NULL, cbSeek, cbRClose);
#else
    cookie_io_functions_t io = {cbRead, NULL, cbSeek, cbRClose};
    FILE *fp = fopencookie(r, "rb", io);
#endif
    if (!fp) { free(r->buf); free(r); }
    return fp;
}

FILE *port_save_fopen(const char *name, const char *mode) {
    if (!name || !mode || !served(name)) return NULL;
    if (mode[0] == 'r') return openReader(name);
    if (mode[0] == 'w') return openWriter(name);
    return NULL;
}
