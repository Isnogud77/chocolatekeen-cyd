// Host (Mac) backend: a 320x240 "fake CYD" window, scaled 3x.
// Arrows = joystick, Z = jump, X = pogo, Esc = quit.
#include <SDL2/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "backend.h"
#include "pcspeaker.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static uint16_t screen[320 * 240];
static SDL_AudioDeviceID audioDev; // 0 until audio is opened

// SDL audio thread: advance the PC speaker every 6868 us of samples and
// write a square wave at its frequency. SDL holds the device lock here.
static void audioCallback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    static uint32_t usInTick = PCSPK_TICK_US; // tick on the first sample
    static uint32_t hz;
    static uint64_t phase; // in units of 1/(2*44100) cycles
    int16_t *out = (int16_t *)stream;
    for (int i = 0; i < len / 2; i++) {
        usInTick += 1000000u / 44100u;
        if (usInTick >= PCSPK_TICK_US) {
            usInTick -= PCSPK_TICK_US;
            hz = pcspeaker_divisor_to_hz(pcspeaker_tick_locked());
        }
        if (!hz) { out[i] = 0; continue; }
        phase += 2 * (uint64_t)hz;
        out[i] = ((phase / 44100) & 1) ? -3000 : 3000;
    }
}

static void openAudio(void) {
    SDL_AudioSpec want = {0}, got;
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = audioCallback;
    audioDev = SDL_OpenAudioDevice(NULL, 0, &want, &got, 0);
    if (!audioDev) { port_log("audio: %s (continuing silently)", SDL_GetError()); return; }
    SDL_PauseAudioDevice(audioDev, 0);
}

void port_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) port_fatal(SDL_GetError());
    openAudio();
    window = SDL_CreateWindow("keen-esp32 (host)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, 320, 240);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 320, 240);
    if (!window || !renderer || !texture) port_fatal(SDL_GetError());
}

void port_audio_lock(void) { if (audioDev) SDL_LockAudioDevice(audioDev); }
void port_audio_unlock(void) { if (audioDev) SDL_UnlockAudioDevice(audioDev); }

uint32_t port_millis(void) { return SDL_GetTicks(); }
void port_sleep_ms(uint32_t ms) { SDL_Delay(ms); }

static void pumpEvents(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e))
        if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
            exit(0);
}

// KEEN_AUTO (debug aid): "R" holds right, "J" jumps every 1.5 s, "P" pogo toggles every 4 s.
static uint8_t autoButtons(void) {
    const char *a = getenv("KEEN_AUTO");
    if (!a) return 0;
    uint32_t t = SDL_GetTicks();
    uint8_t b = 0;
    if (strchr(a, 'R')) b |= PORT_BTN_RIGHT;
    if (strchr(a, 'J') && t % 1500 < 300) b |= PORT_BTN_JUMP;
    if (strchr(a, 'P') && t % 4000 < 100) b |= PORT_BTN_POGO;
    return b;
}

uint8_t port_read_buttons(void) {
    pumpEvents();
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    return autoButtons() | (k[SDL_SCANCODE_LEFT] ? PORT_BTN_LEFT : 0) | (k[SDL_SCANCODE_RIGHT] ? PORT_BTN_RIGHT : 0) |
           (k[SDL_SCANCODE_UP] ? PORT_BTN_UP : 0) | (k[SDL_SCANCODE_DOWN] ? PORT_BTN_DOWN : 0) |
           (k[SDL_SCANCODE_Z] ? PORT_BTN_JUMP : 0) | (k[SDL_SCANCODE_X] ? PORT_BTN_POGO : 0);
}

// KEEN_DUMP=<path>: write the latest frame as PPM every 30 presents (debug aid).
// A "%d" in the path is replaced by the dump number.
static void dumpFrame(void) {
    static int count;
    const char *path = getenv("KEEN_DUMP");
    if (!path || ++count % 30) return;
    char name[512];
    snprintf(name, sizeof name, path, count / 30);
    FILE *f = fopen(name, "wb");
    if (!f) return;
    fprintf(f, "P6\n320 240\n255\n");
    for (int i = 0; i < 320 * 240; i++) {
        uint16_t c = screen[i];
        uint8_t rgb[3] = {(uint8_t)((c >> 11) << 3), (uint8_t)(((c >> 5) & 63) << 2), (uint8_t)((c & 31) << 3)};
        fwrite(rgb, 3, 1, f);
    }
    fclose(f);
}

void port_present(const uint8_t *src, int stride, const uint16_t pal565[16]) {
    for (int y = 0; y < 200; y++)
        for (int x = 0; x < 320; x++)
            screen[(y + 20) * 320 + x] = pal565[src[y * stride + x] & 15];
    dumpFrame();
    SDL_UpdateTexture(texture, NULL, screen, 320 * 2);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    pumpEvents();
}

void port_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

void port_report_heap(const char *where) { port_log("heap @%s: n/a on host", where); }

_Noreturn void port_fatal(const char *msg) {
    port_log("FATAL: %s", msg);
    exit(1);
}
