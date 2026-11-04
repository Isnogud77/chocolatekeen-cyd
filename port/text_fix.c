// Text files: upstream expects CR LF line ends; some shipped files (STORYTXT,
// ENDTEXT) use a bare LF, which the viewer would draw as a glyph.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "text_fix.h"

// Returns a malloc'd copy of buf[0..len) with every LF not preceded by CR
// turned into CR LF (*outLen = new length). NULL if out of memory. Frees buf.
uint8_t *port_text_crlf(uint8_t *buf, uint32_t len, uint32_t *outLen) {
    uint32_t extra = 0;
    for (uint32_t i = 0; i < len; i++)
        if (buf[i] == 0x0A && (i == 0 || buf[i - 1] != 0x0D)) extra++;
    uint8_t *out = (uint8_t *)malloc(len + extra);
    if (!out) return NULL;
    uint32_t o = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (buf[i] == 0x0A && (i == 0 || buf[i - 1] != 0x0D)) out[o++] = 0x0D;
        out[o++] = buf[i];
    }
    free(buf);
    *outLen = o;
    return out;
}
