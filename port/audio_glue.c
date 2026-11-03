// Upstream sound API (engine_audio.c) on top of the PC speaker player.
// Sound data is read in place from flash; nothing is pre-rendered.
#include "core/globals.h"
#include "keen1_assets.h"
#include "backend.h"
#include "pcspeaker.h"

static bool soundReady, soundPaused;

void CVort_engine_loadSounds(void) {
    const AssetFile_T *f = port_find_asset("SOUNDS.CK1");
    soundPaused = false;
    if (!port_audio_available()) {
        soundReady = false; // nothing would ever tick the player
        port_log("sound: no audio output, playing silently");
        return;
    }
    soundReady = f && pcspeaker_init(f->data, f->size);
    if (!soundReady)
        port_log("sound: SOUNDS.CK1 missing or bad, playing silently");
}

void CVort_engine_setCurSound(uint16_t sound) {
    if (!soundReady || g_game.sound_disabled)
        return;
    pcspeaker_set_muted(!(g_game.want_sound & 0xFF));
    if (sound < 1 || sound > pcspeaker_count()) {
        static bool logged;
        if (!logged) port_log("sound: number %u out of range", sound);
        logged = true;
        return;
    }
    if (pcspeaker_play(sound))
        sound_limiter = pcspeaker_limiter();
}

void CVort_engine_saveCurSound(void) {
    soundPaused = true;
    pcspeaker_set_paused(true);
}

void CVort_engine_restoreCurSound(void) {
    soundPaused = false;
    pcspeaker_set_paused(false);
}

// Waits until the sound ends; returns at once if it can never end.
void CVort_engine_finishCurSound(void) {
    if (g_game.sound_disabled)
        return;
    while (soundReady && !soundPaused && pcspeaker_playing())
        CVort_engine_shortSleep();
}
