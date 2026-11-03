// Controller buttons -> DOS scancodes in the game's key map.
#include <unity.h>
#include <string.h>
#include "core/globals.h"
#include "backend.h"
#include "input_map.h"

void setUp(void) {
    memset(g_input.key_map, 0, sizeof g_input.key_map);
    input_init_scancodes();
}
void tearDown(void) {}

static int pressedCount(void) {
    int n = 0;
    for (size_t i = 0; i < sizeof g_input.key_map / sizeof g_input.key_map[0]; i++) n += g_input.key_map[i] != 0;
    return n;
}

static void test_diagonal_left_up(void) {
    input_apply_buttons(PORT_BTN_LEFT | PORT_BTN_UP);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x4B]);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x48]);
    TEST_ASSERT_EQUAL(2, pressedCount());
}

static void test_both_buttons(void) {
    input_apply_buttons(PORT_BTN_JUMP | PORT_BTN_POGO);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x1D]);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x38]);
    TEST_ASSERT_EQUAL(2, pressedCount());
}

static void test_release_clears(void) {
    input_apply_buttons(PORT_BTN_LEFT);
    input_apply_buttons(0);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x4B]);
}

static void test_keyboard_state_reads_mapping(void) {
    input_apply_buttons(PORT_BTN_RIGHT | PORT_BTN_JUMP);
    GameInput_T in = CVort_engine_getKeybCtrlState();
    TEST_ASSERT_EQUAL(2, in.direction); // 2 = right
    TEST_ASSERT_TRUE(in.but1jump);
    TEST_ASSERT_FALSE(in.but2pogo);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_diagonal_left_up);
    RUN_TEST(test_both_buttons);
    RUN_TEST(test_release_clears);
    RUN_TEST(test_keyboard_state_reads_mapping);
    return UNITY_END();
}
