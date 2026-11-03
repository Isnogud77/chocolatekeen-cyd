// PC speaker player. Rules follow upstream engine_audio.c: a sound starts if
// nothing plays or its priority >= the limiter; the limiter is then set to it.
#include <string.h>
#include "backend.h"
#include "pcspeaker.h"

static const uint8_t *sndData;
static uint32_t sndLen;
static int sndCount;
static int current = -1;  // 1-based sound number, -1 = none
static uint32_t pos;      // byte offset of the next tone word
static uint8_t limiter;
static bool muted, paused;

static uint16_t rd16(uint32_t off) { return (uint16_t)(sndData[off] | sndData[off + 1] << 8); }

bool pcspeaker_init(const uint8_t *data, uint32_t len) {
    port_audio_lock();
    sndData = NULL;
    sndLen = 0;
    sndCount = 0;
    current = -1;
    limiter = 0;
    muted = paused = false;
    bool ok = data && len >= 16 && !memcmp(data, "SND\0", 4);
    if (ok) {
        int count = data[8] | data[9] << 8;
        ok = len >= 16u * (uint32_t)(count + 1);
        if (ok) {
            sndData = data;
            sndLen = len;
            sndCount = count;
        }
    }
    port_audio_unlock();
    return ok;
}

int pcspeaker_count(void) { return sndCount; }

bool pcspeaker_play(uint16_t sound) {
    bool started = false;
    port_audio_lock();
    if (sndData && sound >= 1 && sound <= sndCount) {
        uint8_t prio = sndData[16u * sound + 2];
        if (current < 0 || prio >= limiter) {
            current = sound;
            pos = rd16(16u * sound);
            limiter = prio;
            started = true;
        }
    }
    port_audio_unlock();
    return started;
}

uint16_t pcspeaker_tick_locked(void) {
    if (current < 0 || paused)
        return 0;
    if (pos + 1 >= sndLen || rd16(pos) == 0xFFFF) { // end, or data cut short
        current = -1;
        return 0;
    }
    uint16_t divisor = rd16(pos);
    pos += 2;
    return muted ? 0 : divisor;
}

uint16_t pcspeaker_tick(void) {
    port_audio_lock();
    uint16_t d = pcspeaker_tick_locked();
    port_audio_unlock();
    return d;
}

bool pcspeaker_playing(void) {
    port_audio_lock();
    bool p = current >= 0;
    port_audio_unlock();
    return p;
}

void pcspeaker_set_muted(bool m) {
    port_audio_lock();
    muted = m;
    port_audio_unlock();
}

void pcspeaker_set_paused(bool p) {
    port_audio_lock();
    paused = p;
    port_audio_unlock();
}

uint8_t pcspeaker_limiter(void) { return limiter; }

uint32_t pcspeaker_divisor_to_hz(uint16_t divisor) {
    if (!divisor)
        return 0;
    uint32_t hz = PCSPK_PIT_HZ / divisor;
    return hz < 20 ? 20 : (hz > 20000 ? 20000 : hz);
}
