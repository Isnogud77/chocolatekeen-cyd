// Autosave slot 1: save on map return, continue, erase on game over.
#include <unity.h>
#include <string.h>
#include "core/globals.h"
#include "backend.h"

void fake_store_reset(void);
bool port_autosave(void);
uint16_t port_continue_autosave(void);
void port_autosave_erase(void);

static int slotLen(void) {
    static uint8_t buf[2048];
    return port_store_load("SAVED1.CK1", buf, sizeof buf);
}

static void setState(void) {
    for (int i = 0; i < 9; i++) keen_gp.stuff[i] = (int16_t)(i + 1);
    for (int i = 0; i < 16; i++) keen_gp.levels[i] = (int16_t)(i & 1);
    keen_gp.lives = 3;
    keen_gp.ammo = 7;
    keen_gp.score = 12345;
    keen_gp.mapX = 0x123456;
    keen_gp.mapY = 0x234567;
    keen_gp.screenX = 0x111111;
    keen_gp.screenY = 0x222222;
    for (int i = 0; i < 8; i++) keen_gp.targets[i] = (int16_t)(8 - i);
}

void setUp(void) {
    fake_store_reset();
    snprintf(game_ext, sizeof(game_ext), "CK1");
    g_game.resuming_saved_game = 0;
}
void tearDown(void) {}

static void test_autosave_roundtrip(void) {
    setState();
    TEST_ASSERT_TRUE(port_autosave());
    TEST_ASSERT_EQUAL(92, slotLen());
    memset(&keen_gp, 0x5A, sizeof keen_gp);
    TEST_ASSERT_EQUAL_UINT16(1, port_continue_autosave());
    TEST_ASSERT_EQUAL(1, g_game.resuming_saved_game);
    TEST_ASSERT_EQUAL_INT16(3, keen_gp.lives);
    TEST_ASSERT_EQUAL_INT16(7, keen_gp.ammo);
    TEST_ASSERT_EQUAL_INT32(12345, keen_gp.score);
    TEST_ASSERT_EQUAL_INT32(0x123456, keen_gp.mapX);
    TEST_ASSERT_EQUAL_INT16(9, keen_gp.stuff[8]);
    TEST_ASSERT_EQUAL_INT16(1, keen_gp.levels[15]);
    TEST_ASSERT_EQUAL_INT16(1, keen_gp.targets[7]);
}

static void test_continue_without_save_is_noop(void) {
    setState();
    TEST_ASSERT_EQUAL_UINT16(0, port_continue_autosave());
    TEST_ASSERT_EQUAL(0, g_game.resuming_saved_game);
    TEST_ASSERT_EQUAL_INT16(3, keen_gp.lives);
}

static void test_corrupt_save_erased(void) {
    const uint8_t junk[10] = {9, 9, 9, 9, 9, 9, 9, 9, 9, 9};
    port_store_save("SAVED1.CK1", junk, sizeof junk);
    setState();
    TEST_ASSERT_EQUAL_UINT16(0, port_continue_autosave());
    TEST_ASSERT_EQUAL(-1, slotLen());
    TEST_ASSERT_EQUAL_INT16(3, keen_gp.lives); // state untouched
}

static void test_game_over_erases(void) {
    setState();
    port_autosave();
    port_autosave_erase();
    TEST_ASSERT_EQUAL(-1, slotLen());
}

// High scores go through upstream's own writer/reader and the store.
static void test_scores_survive_reboot(void) {
    CVort_load_high_scores_table(); // first boot: upstream writes its defaults
    high_scores_table.scores[0] = 54321;
    strcpy(high_scores_table.names[0], "KEENY");
    CVort_save_high_scores_table();
    memset(&high_scores_table, 0, sizeof high_scores_table); // "reboot": RAM state gone
    CVort_load_high_scores_table();
    TEST_ASSERT_EQUAL_INT32(54321, high_scores_table.scores[0]);
    TEST_ASSERT_EQUAL_STRING("KEENY", high_scores_table.names[0]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_autosave_roundtrip);
    RUN_TEST(test_continue_without_save_is_noop);
    RUN_TEST(test_corrupt_save_erased);
    RUN_TEST(test_game_over_erases);
    RUN_TEST(test_scores_survive_reboot);
    return UNITY_END();
}
