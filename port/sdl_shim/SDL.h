// Fake SDL.h: just enough types and macros for the vendored upstream code.
// No real SDL is used by upstream sources in this port; the host backend
// includes the real SDL2 by its own path (<SDL2/SDL.h>).
#ifndef PORT_SDL_SHIM_H
#define PORT_SDL_SHIM_H

#include <stdint.h>
#include <stddef.h>
// Real SDL.h (SDL_stdinc.h) pulls these in; upstream relies on it.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef uint8_t Uint8;   typedef int8_t Sint8;
typedef uint16_t Uint16; typedef int16_t Sint16;
typedef uint32_t Uint32; typedef int32_t Sint32;
typedef uint64_t Uint64; typedef int64_t Sint64;

#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321
#define SDL_BYTEORDER SDL_LIL_ENDIAN
#define SDL_VERSION_ATLEAST(x, y, z) 1

static inline Uint16 SDL_Swap16(Uint16 x) { return (Uint16)((x << 8) | (x >> 8)); }
static inline Uint32 SDL_Swap32(Uint32 x) {
    return (x << 24) | ((x << 8) & 0x00FF0000u) | ((x >> 8) & 0x0000FF00u) | (x >> 24);
}
#define SDL_SwapLE16(x) (x)
#define SDL_SwapLE32(x) (x)

// Opaque host types named in core/types.h; only pointers are ever declared.
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Overlay SDL_Overlay;
typedef struct SDL_VideoInfo SDL_VideoInfo;
typedef void *SDL_GLContext;
typedef struct { int dummy; } SDL_RendererInfo;
typedef struct { int x, y, w, h; } SDL_Rect;
typedef struct SDL_Joystick SDL_Joystick;
#define SDL_NUM_SCANCODES 512

// Time comes from the port backend.
uint32_t port_millis(void);
void port_sleep_ms(uint32_t ms);
#define SDL_GetTicks port_millis
#define SDL_Delay port_sleep_ms

#endif
