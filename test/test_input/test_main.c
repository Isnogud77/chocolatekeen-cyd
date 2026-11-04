// Controller buttons -> DOS scancodes in the game's key map.
#include <unity.h>
#include <string.h>
#include "core/globals.h"
#include "backend.h"
#include "input_map.h"
#include "play_context.h"
#include "letter_picker.h"
#include "input/actions.h"

static uint8_t g_testButtons;
static uint32_t g_testNow;
uint8_t port_read_buttons(void) { return g_testButtons; }
uint32_t port_millis(void) { return g_testNow; }

void setUp(void) {
    g_testButtons = 0;
    g_testNow = 0;
    memset(g_input.key_map, 0, sizeof g_input.key_map);
    g_input.key_scane = 0;
    input_reset_state();
    play_context_set(true, true);
    input_init_scancodes();
}
void tearDown(void) {}

static int pressedCount(void) {
    int n = 0;
    for (size_t i = 0; i < sizeof g_input.key_map / sizeof g_input.key_map[0]; i++) n += g_input.key_map[i] != 0;
    return n;
}

static void test_picker_overrides_menu_mapping(void) {
    play_context_set(false, false);
    letter_picker_start();
    input_apply_buttons_at(PORT_BTN_UP, 0);
    TEST_ASSERT_EQUAL_HEX16(0x1E | 0x80, g_input.key_scane);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x48]);
    g_input.key_scane = 0;
    input_apply_buttons_at(PORT_BTN_UP | PORT_BTN_JUMP, 10);
    TEST_ASSERT_EQUAL_HEX16(0x1C | 0x80, g_input.key_scane);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x15]);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x1C]);
    letter_picker_stop();
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

static void test_play_b1_is_ctrl(void) {
    input_apply_buttons_at(PORT_BTN_JUMP, 0);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x1D]);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x1C]);
}
static void test_menu_b1_holds_enter_y_space(void) {
    play_context_set(false, false);
    input_apply_buttons_at(PORT_BTN_JUMP, 0);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x1C]);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x15]);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x39]);
}
static void test_menu_b1_event_is_y(void) {
    play_context_set(false, false);
    input_apply_buttons_at(PORT_BTN_JUMP, 0);
    TEST_ASSERT_EQUAL_HEX(0x15 | 0x80, g_input.key_scane);
    g_input.key_scane &= 0x7F;
    input_apply_buttons_at(PORT_BTN_JUMP, 10);
    TEST_ASSERT_EQUAL_HEX(0x15, g_input.key_scane);
}
static void test_menu_b2_event_is_esc(void) {
    play_context_set(false, false);
    input_apply_buttons_at(PORT_BTN_POGO, 0);
    TEST_ASSERT_EQUAL_HEX(0x81, g_input.key_scane);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x01]);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x31]);
}
static void test_long_press_b2_pulses_esc(void) {
    input_apply_buttons_at(PORT_BTN_POGO, 0);
    input_apply_buttons_at(PORT_BTN_POGO, 999);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x01]);
    input_apply_buttons_at(PORT_BTN_POGO, 1000);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x01]);
    input_apply_buttons_at(PORT_BTN_POGO, 1099);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x01]);
    input_apply_buttons_at(PORT_BTN_POGO, 1100);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x01]);
    for (uint32_t t = 1200; t <= 3000; t += 100) {
        input_apply_buttons_at(PORT_BTN_POGO, t);
        TEST_ASSERT_EQUAL(0, g_input.key_map[0x01]);
    }
}
static void test_down_hold_in_level_pulses_space(void) {
    input_apply_buttons_at(PORT_BTN_DOWN, 0);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x39]);
    input_apply_buttons_at(PORT_BTN_DOWN, 1000);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x39]);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x50]);
    input_apply_buttons_at(PORT_BTN_DOWN, 1100);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x39]);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x50]);
}
static void test_down_hold_on_map_no_status(void) {
    play_context_set(true, false);
    input_apply_buttons_at(PORT_BTN_DOWN, 0);
    input_apply_buttons_at(PORT_BTN_DOWN, 1050);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x39]);
}
static void test_start_menu_buttons(void) {
    for (int ctx = 0; ctx < 2; ctx++) {
        play_context_set(ctx, ctx);
        input_reset_state();
        g_input.key_scane = 0;
        input_apply_buttons_at(PORT_BTN_START, 0);
        TEST_ASSERT_EQUAL(1, g_input.key_map[0x1C]);
        TEST_ASSERT_EQUAL_HEX(0x9C, g_input.key_scane);
        input_apply_buttons_at(0, 10);
        g_input.key_scane = 0;
        input_apply_buttons_at(PORT_BTN_MENU, 20);
        TEST_ASSERT_EQUAL(1, g_input.key_map[0x01]);
        TEST_ASSERT_EQUAL_HEX(0x81, g_input.key_scane);
        input_apply_buttons_at(0, 30);
    }
}

static void test_update_drives_action_state(void) {
    play_context_set(false, false);
    g_testButtons = PORT_BTN_POGO;
    CVort_engine_updateInputStatus();
    TEST_ASSERT_TRUE(CK_Action_IsHeld(CK_ACTION_MENU_BACK));
    g_testButtons = 0;
    g_testNow = 10;
    CVort_engine_updateInputStatus();
    TEST_ASSERT_FALSE(CK_Action_IsHeld(CK_ACTION_MENU_BACK));
}
static void test_update_down_hold_status_action(void) {
    g_testButtons = PORT_BTN_DOWN;
    CVort_engine_updateInputStatus();
    TEST_ASSERT_FALSE(CK_Action_IsHeld(CK_ACTION_STATUS));
    g_testNow = 1000;
    CVort_engine_updateInputStatus();
    TEST_ASSERT_TRUE(CK_Action_IsHeld(CK_ACTION_STATUS));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_picker_overrides_menu_mapping);
    RUN_TEST(test_diagonal_left_up);
    RUN_TEST(test_both_buttons);
    RUN_TEST(test_release_clears);
    RUN_TEST(test_keyboard_state_reads_mapping);
    RUN_TEST(test_play_b1_is_ctrl);
    RUN_TEST(test_menu_b1_holds_enter_y_space);
    RUN_TEST(test_menu_b1_event_is_y);
    RUN_TEST(test_menu_b2_event_is_esc);
    RUN_TEST(test_long_press_b2_pulses_esc);
    RUN_TEST(test_down_hold_in_level_pulses_space);
    RUN_TEST(test_down_hold_on_map_no_status);
    RUN_TEST(test_start_menu_buttons);
    RUN_TEST(test_update_drives_action_state);
    RUN_TEST(test_update_down_hold_status_action);
    return UNITY_END();
}
