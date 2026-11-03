// Upstream sound API on top of the PC speaker player.
#include <unity.h>
#include "core/globals.h"
#include "pcspeaker.h"

extern bool fake_audio_available;

void setUp(void) {
    fake_audio_available = true;
    g_game.sound_disabled = 0;
    g_game.want_sound = 1;
    sound_limiter = 0;
    CVort_engine_loadSounds();
}
void tearDown(void) {}

static void test_set_sound_starts_player(void) {
    CVort_engine_setCurSound(1);
    TEST_ASSERT_TRUE(pcspeaker_playing());
    TEST_ASSERT_EQUAL_UINT8(128, sound_limiter);
}

static void test_sound_disabled_blocks(void) {
    g_game.sound_disabled = 1;
    CVort_engine_setCurSound(1);
    TEST_ASSERT_FALSE(pcspeaker_playing());
}

static void test_want_sound_off_mutes(void) {
    g_game.want_sound = 0;
    CVort_engine_setCurSound(1);
    TEST_ASSERT_TRUE(pcspeaker_playing());
    TEST_ASSERT_EQUAL_HEX16(0, pcspeaker_tick());
}

// Nothing ticks the player in this test, so a wrong loop never returns.
static void test_finish_returns_when_paused(void) {
    CVort_engine_setCurSound(1);
    CVort_engine_saveCurSound();
    CVort_engine_finishCurSound();
    TEST_ASSERT_TRUE(pcspeaker_playing());
}

static void test_finish_returns_when_not_initialized(void) {
    static const uint8_t zeros[32];
    pcspeaker_init(zeros, sizeof zeros);
    CVort_engine_finishCurSound();
    TEST_ASSERT_FALSE(pcspeaker_playing());
}

// No tick source (no audio device / timer failed): nothing would ever end a
// sound, so finishCurSound must not wait. Hangs if wrong.
static void test_finish_returns_without_tick_source(void) {
    fake_audio_available = false;
    CVort_engine_loadSounds();
    CVort_engine_setCurSound(1);
    CVort_engine_finishCurSound();
    TEST_ASSERT_FALSE(pcspeaker_playing());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_set_sound_starts_player);
    RUN_TEST(test_sound_disabled_blocks);
    RUN_TEST(test_want_sound_off_mutes);
    RUN_TEST(test_finish_returns_when_paused);
    RUN_TEST(test_finish_returns_when_not_initialized);
    RUN_TEST(test_finish_returns_without_tick_source);
    return UNITY_END();
}
