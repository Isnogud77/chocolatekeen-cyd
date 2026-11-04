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
static uint16_t screen[320 * 320]; // big enough for either orientation
static int scrW, scrH;               // from screen_layout at port_init
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
    scrW = screen_layout_screen_w();
    scrH = screen_layout_screen_h();
    int zoom = scrH > scrW ? 2 : 3; // portrait at 3x would not fit most displays
    window = SDL_CreateWindow("keen-esp32 (host)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, zoom * scrW, zoom * scrH, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, scrW, scrH);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, scrW, scrH);
    if (!window || !renderer || !texture) port_fatal(SDL_GetError());
}

bool port_audio_available(void) { return audioDev != 0; }
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
// "M" (menu script): nothing for 3 s, button 1 for 300 ms every 2 s until 13 s
// (logo, title, New Game, ...), then the R/J autoplay; until 25 s down is
// also tapped (150 ms every 1.5 s), which walks Keen from his map start to
// level 1 (down-right of it).
// "S"/"H"/"C" (front-end scripts): one button 1 press at 3 s skips the logo and
// opens the menu; Down is tapped 2/3/4 times (from 5 s, every 0.8 s) to reach
// Story/Help/High Scores; button 1 selects (8 s); Story and Help are paged down
// three times (10, 11.5, 13 s); button 2 (15 s) goes back.
static uint8_t frontEndScript(char which, uint32_t t) {
    int downs = which == 'S' ? 2 : which == 'H' ? 3 : 4;
    if (t >= 3000 && t < 3300) return PORT_BTN_JUMP;
    for (int i = 0; i < downs; i++)
        if (t >= 5000 + 800u * i && t < 5150 + 800u * i) return PORT_BTN_DOWN;
    if (t >= 8000 && t < 8300) return PORT_BTN_JUMP;
    if (which != 'C')
        for (int i = 0; i < 3; i++)
            if (t >= 10000 + 1500u * i && t < 10150 + 1500u * i) return PORT_BTN_DOWN;
    if (t >= 15000 && t < 15300) return PORT_BTN_POGO;
    return 0;
}

static uint32_t keenAutoAt(void) {
    const char *at = getenv("KEEN_AUTO_AT");
    return at ? (uint32_t)atoi(at) : 30000;
}

static uint8_t autoButtons(void) {
    const char *a = getenv("KEEN_AUTO");
    if (!a) return 0;
    uint32_t t = SDL_GetTicks();
    uint8_t b = 0;
    if (strchr(a, 'S')) return frontEndScript('S', t);
    if (strchr(a, 'H')) return frontEndScript('H', t);
    if (strchr(a, 'C')) return frontEndScript('C', t);
    if (strchr(a, 'M')) {
        if (t < 3000) return 0;
        // With D/X Keen stands still 1.5 s before the hold (and nothing else is pressed after it).
        if ((strchr(a, 'D') || strchr(a, 'X')) && t >= keenAutoAt() - 1500) {
            if (t < keenAutoAt() || t >= keenAutoAt() + 1400) return 0;
            return strchr(a, 'D') ? PORT_BTN_DOWN : PORT_BTN_POGO;
        }
        if (t < 13000) return (t - 3000) % 2000 < 300 ? PORT_BTN_JUMP : 0;
        b |= PORT_BTN_RIGHT;
        if (t < 25000 && t % 1500 >= 700 && t % 1500 < 850) b |= PORT_BTN_DOWN;
        if (t % 1500 < 300) b |= PORT_BTN_JUMP;
    }
    // "D"/"X" (text box checks): hold Down / button 2 for 1.4 s from KEEN_AUTO_AT ms
    // (a long press opens the status screen / the in-game menu).
    uint32_t t0 = keenAutoAt();
    if (t >= t0 && t < t0 + 1400) {
        if (strchr(a, 'D')) b |= PORT_BTN_DOWN;
        if (strchr(a, 'X')) b |= PORT_BTN_POGO;
    }
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

// KEEN_DUMP=<path>: write the latest frame as PPM every 30 presents (debug aid;
// KEEN_DUMP_EVERY=<n> changes the 30). A "%d" in the path is replaced by the dump number.
static void dumpFrame(void) {
    static int count;
    const char *path = getenv("KEEN_DUMP");
    const char *everyEnv = getenv("KEEN_DUMP_EVERY");
    int every = everyEnv && atoi(everyEnv) > 0 ? atoi(everyEnv) : 30;
    if (!path || ++count % every) return;
    char name[512];
    snprintf(name, sizeof name, path, count / every);
    FILE *f = fopen(name, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", scrW, scrH);
    for (int i = 0; i < scrW * scrH; i++) {
        uint16_t c = screen[i];
        uint8_t rgb[3] = {(uint8_t)((c >> 11) << 3), (uint8_t)(((c >> 5) & 63) << 2), (uint8_t)((c & 31) << 3)};
        fwrite(rgb, 3, 1, f);
    }
    fclose(f);
    if (getenv("KEEN_DUMP_LOG")) fprintf(stderr, "dump %d at %u ms\n", count / every, (unsigned)SDL_GetTicks());
}

// KEEN_TEXT_PROBE (debug aid): from the 60th frame on, draw "PORTRAIT" with the
// game font every frame, so the text (SCALE) layout can be checked.
void CVort_engine_drawChar(uint16_t x, uint16_t y, uint16_t val);
static void textProbe(void) {
    static int frames;
    if (!getenv("KEEN_TEXT_PROBE") || ++frames < 60) return;
    const char *s = "PORTRAIT";
    for (int i = 0; s[i]; i++) CVort_engine_drawChar((uint16_t)(5 + i), 40, (uint16_t)s[i]);
}

void port_present(const ScreenLayout_T *layout, const uint8_t *src, int stride, const uint16_t pal565[16]) {
    static int lastMode = -1;
    if ((int)layout->mode != lastMode) { // bars differ per mode: clear once
        memset(screen, 0, sizeof screen);
        lastMode = layout->mode;
    }
    for (int y = 0; y < layout->outH; y++)
        screen_layout_row(layout, src, stride, y, pal565, screen + (layout->top + y) * scrW);
    textProbe();
    dumpFrame();
    SDL_UpdateTexture(texture, NULL, screen, scrW * 2);
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
