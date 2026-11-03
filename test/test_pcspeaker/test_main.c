// PC speaker player over the real SOUNDS.CK1.
#include <unity.h>
#include <string.h>
#include "keen1_assets.h"
#include "pcspeaker.h"

static const AssetFile_T *snd;

static uint16_t word(uint32_t off) { return snd->data[off] | snd->data[off + 1] << 8; }

void setUp(void) {
    for (int i = 0; i < keen1_filesCount; i++)
        if (!strcmp(keen1_files[i].name, "SOUNDS.CK1")) snd = &keen1_files[i];
    TEST_ASSERT_NOT_NULL(snd);
    TEST_ASSERT_TRUE(pcspeaker_init(snd->data, snd->size));
}
void tearDown(void) {}

static void test_count_60(void) { TEST_ASSERT_EQUAL_INT(60, pcspeaker_count()); }

static void test_bad_magic_rejected(void) {
    static const uint8_t zeros[32];
    TEST_ASSERT_FALSE(pcspeaker_init(zeros, sizeof zeros));
    TEST_ASSERT_FALSE(pcspeaker_play(1));
}

static void test_plays_tone_list_in_order(void) {
    TEST_ASSERT_TRUE(pcspeaker_play(1));
    uint32_t off = word(16); // 1024
    TEST_ASSERT_EQUAL_UINT32(1024, off);
    for (; word(off) != 0xFFFF; off += 2)
        TEST_ASSERT_EQUAL_HEX16(word(off), pcspeaker_tick());
    TEST_ASSERT_EQUAL_HEX16(0, pcspeaker_tick());
    TEST_ASSERT_FALSE(pcspeaker_playing());
}

static void test_lower_priority_does_not_interrupt(void) {
    TEST_ASSERT_TRUE(pcspeaker_play(3)); // prio 255
    TEST_ASSERT_FALSE(pcspeaker_play(1)); // prio 128
    TEST_ASSERT_EQUAL_UINT8(255, pcspeaker_limiter());
}

static void test_equal_priority_interrupts(void) {
    TEST_ASSERT_TRUE(pcspeaker_play(3));
    TEST_ASSERT_TRUE(pcspeaker_play(3));
}

static void test_muted_advances_silently(void) {
    pcspeaker_set_muted(true);
    TEST_ASSERT_TRUE(pcspeaker_play(1));
    int n = 0;
    for (uint32_t off = word(16); word(off) != 0xFFFF; off += 2) n++;
    for (int i = 0; i <= n; i++) TEST_ASSERT_EQUAL_HEX16(0, pcspeaker_tick());
    TEST_ASSERT_FALSE(pcspeaker_playing());
    pcspeaker_set_muted(false);
}

static void test_paused_holds_position(void) {
    TEST_ASSERT_TRUE(pcspeaker_play(1));
    pcspeaker_set_paused(true);
    for (int i = 0; i < 5; i++) TEST_ASSERT_EQUAL_HEX16(0, pcspeaker_tick());
    pcspeaker_set_paused(false);
    TEST_ASSERT_EQUAL_HEX16(word(1024), pcspeaker_tick());
}

static void test_out_of_range_ignored(void) {
    TEST_ASSERT_FALSE(pcspeaker_play(0));
    TEST_ASSERT_FALSE(pcspeaker_play(61));
}

static void test_truncated_data_stops(void) {
    static uint8_t cut[1030];
    memcpy(cut, snd->data, sizeof cut); // entry 1's list starts at 1024: 3 words, no terminator
    TEST_ASSERT_TRUE(pcspeaker_init(cut, sizeof cut));
    TEST_ASSERT_TRUE(pcspeaker_play(1));
    for (int i = 0; i < 3; i++) pcspeaker_tick();
    TEST_ASSERT_EQUAL_HEX16(0, pcspeaker_tick());
    TEST_ASSERT_FALSE(pcspeaker_playing());
}

static void test_divisor_to_hz(void) {
    TEST_ASSERT_EQUAL_UINT32(0, pcspeaker_divisor_to_hz(0));
    TEST_ASSERT_EQUAL_UINT32(1000, pcspeaker_divisor_to_hz(1193));
    TEST_ASSERT_EQUAL_UINT32(20000, pcspeaker_divisor_to_hz(1));
    TEST_ASSERT_EQUAL_UINT32(20, pcspeaker_divisor_to_hz(0xFFFE));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_count_60);
    RUN_TEST(test_bad_magic_rejected);
    RUN_TEST(test_plays_tone_list_in_order);
    RUN_TEST(test_lower_priority_does_not_interrupt);
    RUN_TEST(test_equal_priority_interrupts);
    RUN_TEST(test_muted_advances_silently);
    RUN_TEST(test_paused_holds_position);
    RUN_TEST(test_out_of_range_ignored);
    RUN_TEST(test_truncated_data_stops);
    RUN_TEST(test_divisor_to_hz);
    return UNITY_END();
}
