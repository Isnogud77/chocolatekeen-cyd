// Small helpers copied verbatim from upstream src/engine/engine.c, which
// as a whole is SDL-bound and not vendored.
#include <stdarg.h>
#include "core/globals.h"
#include "backend.h"

void CVort_engine_cross_logMessage(CVort_Log_Message_Class_T msgClass, const char *format, ...) {
    va_list args;
    va_start(args, format);
    switch (msgClass) {
        case CVORT_LOG_MSG_NORMAL: fprintf(stderr, "Log: "); break;
        case CVORT_LOG_MSG_WARNING: fprintf(stderr, "Warning: "); break;
        case CVORT_LOG_MSG_ERROR: fprintf(stderr, "Error: "); break;
    }
    vfprintf(stderr, format, args);
    va_end(args);
}

int16_t CVort_engine_signExtend8To16(int8_t inputVal) {
    int16_t result = ((int16_t)inputVal) & 0xFF;
    if (inputVal & 0x80)
        result += 0xFF00;
    return result;
}

int16_t CVort_engine_toupper(int16_t c) {
    if (c == -1)
        return -1;
    if (exeFields.uppercase_table[c & 0xff] & 8)
        return (c & 0xff) - 0x20;
    return (c & 0xff);
}

// Upstream writes to emulated text mode; we only log.
void CVort_engine_puts(const char *str) { port_log("%s", str); }

GameInput_T CVort_engine_getKeybCtrlState() {
    GameInput_T result;
    int16_t horiz = 0, vert = 0;
    if (g_input.key_map[g_input.sc_dir[0]])
        vert = -1;
    if (g_input.key_map[g_input.sc_dir[2]])
        horiz = 1;
    if (g_input.key_map[g_input.sc_dir[4]])
        vert = 1;
    if (g_input.key_map[g_input.sc_dir[6]])
        horiz = -1;
    if (g_input.key_map[g_input.sc_dir[1]]) {
        vert = -1;
        horiz = 1;
    }
    if (g_input.key_map[g_input.sc_dir[7]])
        vert = horiz = -1;
    if (g_input.key_map[g_input.sc_dir[3]])
        vert = horiz = 1;
    if (g_input.key_map[g_input.sc_dir[5]]) {
        vert = 1;
        horiz = -1;
    }
    switch (vert * 3 + horiz + 4) {
        case 0: result.direction = 7;
            break;
        case 1: result.direction = 0;
            break;
        case 2: result.direction = 1;
            break;
        case 3: result.direction = 6;
            break;
        case 4: result.direction = 8;
            break;
        case 5: result.direction = 2;
            break;
        case 6: result.direction = 5;
            break;
        case 7: result.direction = 4;
            break;
        case 8: result.direction = 3;
            break;
        default: break;
    }
    result.but1jump = g_input.key_map[g_input.sc_but1];
    result.but2pogo = g_input.key_map[g_input.sc_but2];
    return result;
}
