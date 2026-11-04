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
static int s_pollCount = -1; // >= 0: scripted, b1 from the 5th poll on; fail instead of hanging
uint8_t port_read_buttons(void) {
    if (s_pollCount >= 0) {
        if (++s_pollCount > 5000) TEST_FAIL_MESSAGE("translate_key(0) never returned");
        return s_pollCount >= 5 ? PORT_BTN_JUMP : 0;
    }
    return g_testButtons;
}
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

static void test_picker_waits_for_consume(void) {
    play_context_set(false, false);
    letter_picker_start();
    input_apply_buttons_at(PORT_BTN_UP, 0);
    g_input.key_scane = 0; // upstream consumed the first letter
    input_apply_buttons_at(PORT_BTN_UP | PORT_BTN_LEFT, 5); // wait: UP still held, no new edge
    input_apply_buttons_at(0, 6);
    TEST_ASSERT_EQUAL_HEX16(0, g_input.key_scane);
    input_apply_buttons_at(PORT_BTN_UP, 7);
    TEST_ASSERT_EQUAL_HEX16(0x0E | 0x80, g_input.key_scane);
    for (int i = 0; i < 10; i++) input_apply_buttons_at(0, 8 + i); // not consumed
    TEST_ASSERT_EQUAL_HEX16(0x0E | 0x80, g_input.key_scane);
    g_input.key_scane = 0;
    input_apply_buttons_at(0, 30);
    TEST_ASSERT_EQUAL_HEX16(0x30 | 0x80, g_input.key_scane);
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
static void test_menu_b1_event_is_enter(void) {
    play_context_set(false, false);
    input_apply_buttons_at(PORT_BTN_JUMP, 0);
    TEST_ASSERT_EQUAL_HEX(0x1C | 0x80, g_input.key_scane);
    g_input.key_scane &= 0x7F;
    input_apply_buttons_at(PORT_BTN_JUMP, 10);
    TEST_ASSERT_EQUAL_HEX(0x1C, g_input.key_scane);
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

// A character prompt in play (quit, sound...) answers with menu keys: b1 = Enter, b2 = Esc.
static void test_char_wait_in_play_b1_event_is_enter(void) {
    play_context_set(true, true);
    CVort_test_setWaitingForCharInput(true);
    input_apply_buttons_at(PORT_BTN_JUMP, 0);
    TEST_ASSERT_EQUAL_HEX(0x1C | 0x80, g_input.key_scane);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x1D]);
    input_apply_buttons_at(0, 10);
    g_input.key_scane = 0;
    input_apply_buttons_at(PORT_BTN_POGO, 20);
    TEST_ASSERT_EQUAL_HEX(0x81, g_input.key_scane);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x38]);
    CVort_test_setWaitingForCharInput(false);
}
static void test_not_char_waiting_in_play_maps_ctrl_alt(void) {
    play_context_set(true, true);
    CVort_test_setWaitingForCharInput(false);
    input_apply_buttons_at(PORT_BTN_JUMP | PORT_BTN_POGO, 0);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x1D]);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x38]);
    TEST_ASSERT_EQUAL_HEX(0, g_input.key_scane);
}

// The status screen waits in CVort_translate_key(0): in play that wait must answer to button 1 (Enter).
static void test_key_wait_in_play_b1_event_is_enter(void) {
    play_context_set(true, true);
    CVort_test_setKeyWaitActive(true);
    input_apply_buttons_at(PORT_BTN_JUMP, 0);
    TEST_ASSERT_EQUAL_HEX(0x1C | 0x80, g_input.key_scane);
    TEST_ASSERT_EQUAL(0, g_input.key_map[0x1D]);
    CVort_test_setKeyWaitActive(false);
}
static void test_key_wait_inactive_b1_is_ctrl(void) {
    play_context_set(true, true);
    CVort_test_setKeyWaitActive(false);
    input_apply_buttons_at(PORT_BTN_JUMP, 0);
    TEST_ASSERT_EQUAL(1, g_input.key_map[0x1D]);
    TEST_ASSERT_EQUAL_HEX(0, g_input.key_scane);
}
static uint8_t s_transMap[128];
static void test_real_translate_key_zero_returns_on_b1(void) {
    play_context_set(true, true);
    memset(s_transMap, 0, sizeof s_transMap);
    s_transMap[0x1C] = 0x0D;
    exeFields.trans_map = s_transMap;
    g_input.key_scane = 0;
    s_pollCount = 0;
    uint16_t r = CVort_translate_key(0);
    s_pollCount = -1;
    TEST_ASSERT_EQUAL_HEX(0x0D, r & 0xFF);
    TEST_ASSERT_FALSE(CVort_test_keyWaitActive());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_key_wait_in_play_b1_event_is_enter);
    RUN_TEST(test_key_wait_inactive_b1_is_ctrl);
    RUN_TEST(test_real_translate_key_zero_returns_on_b1);
    RUN_TEST(test_char_wait_in_play_b1_event_is_enter);
    RUN_TEST(test_not_char_waiting_in_play_maps_ctrl_alt);
    RUN_TEST(test_picker_overrides_menu_mapping);
    RUN_TEST(test_picker_waits_for_consume);
    RUN_TEST(test_diagonal_left_up);
    RUN_TEST(test_both_buttons);
    RUN_TEST(test_release_clears);
    RUN_TEST(test_keyboard_state_reads_mapping);
    RUN_TEST(test_play_b1_is_ctrl);
    RUN_TEST(test_menu_b1_holds_enter_y_space);
    RUN_TEST(test_menu_b1_event_is_enter);
    RUN_TEST(test_menu_b2_event_is_esc);
    RUN_TEST(test_long_press_b2_pulses_esc);
    RUN_TEST(test_down_hold_in_level_pulses_space);
    RUN_TEST(test_down_hold_on_map_no_status);
    RUN_TEST(test_start_menu_buttons);
    RUN_TEST(test_update_drives_action_state);
    RUN_TEST(test_update_down_hold_status_action);
    return UNITY_END();
}
