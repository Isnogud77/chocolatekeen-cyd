#include <unity.h>
#include "letter_picker.h"

void setUp(void) { letter_picker_start(); }
void tearDown(void) { letter_picker_stop(); }

// Press, then flush the queued second event; returns the letter scancode.
static uint8_t cycle(uint8_t btn) {
    uint8_t first = letter_picker_step(btn, true);
    if (first != 0x0E) return first;
    return letter_picker_step(0, true);
}

static void test_scancodes_for_letters(void) {
    TEST_ASSERT_EQUAL_HEX8(0x1E, letter_picker_step(PORT_BTN_UP, true)); // A
    uint8_t sc = 0;
    for (int i = 0; i < 25; i++) sc = cycle(PORT_BTN_UP);
    TEST_ASSERT_EQUAL_HEX8(0x2C, sc); // Z
    TEST_ASSERT_EQUAL_HEX8(0x39, cycle(PORT_BTN_UP)); // space
}

static void test_up_cycles_letters(void) {
    TEST_ASSERT_EQUAL_HEX8(0x1E, letter_picker_step(PORT_BTN_UP, true)); // A
    TEST_ASSERT_EQUAL_HEX8(0x0E, letter_picker_step(PORT_BTN_UP, true));
    TEST_ASSERT_EQUAL_HEX8(0x30, letter_picker_step(0, true)); // B
    for (int i = 0; i < 24; i++) cycle(PORT_BTN_UP);     // ... Z
    TEST_ASSERT_EQUAL_HEX8(0x39, cycle(PORT_BTN_UP));    // space
    TEST_ASSERT_EQUAL_HEX8(0x1E, cycle(PORT_BTN_UP));    // wraps to A
}

static void test_down_wraps_backwards(void) {
    letter_picker_step(PORT_BTN_UP, true); // A
    TEST_ASSERT_EQUAL_HEX8(0x39, cycle(PORT_BTN_DOWN)); // space
    TEST_ASSERT_EQUAL_HEX8(0x2C, cycle(PORT_BTN_DOWN)); // Z
}

static void test_right_commits_and_starts_at_a(void) {
    letter_picker_step(PORT_BTN_UP, true);
    cycle(PORT_BTN_UP); // B shown
    TEST_ASSERT_EQUAL_HEX8(0x1E, letter_picker_step(PORT_BTN_RIGHT, true)); // new position, A
    TEST_ASSERT_EQUAL_HEX8(0x0E, letter_picker_step(PORT_BTN_UP, true));    // replaces only the new A
    TEST_ASSERT_EQUAL_HEX8(0x30, letter_picker_step(0, true));
}

static void test_b1_emits_enter_and_stops(void) {
    TEST_ASSERT_TRUE(letter_picker_active());
    TEST_ASSERT_EQUAL_HEX8(0x1C, letter_picker_step(PORT_BTN_JUMP, true));
    TEST_ASSERT_FALSE(letter_picker_active());
    TEST_ASSERT_EQUAL_HEX8(0, letter_picker_step(PORT_BTN_UP, true));
}

static void test_queued_edges_not_dropped(void) {
    letter_picker_step(PORT_BTN_UP, true); // A shown
    TEST_ASSERT_EQUAL_HEX8(0x0E, letter_picker_step(PORT_BTN_UP, true));
    TEST_ASSERT_EQUAL_HEX8(0, letter_picker_step(PORT_BTN_JUMP, false)); // slot busy
    TEST_ASSERT_EQUAL_HEX8(0x30, letter_picker_step(0, true));
    TEST_ASSERT_EQUAL_HEX8(0x1C, letter_picker_step(0, true));
    TEST_ASSERT_FALSE(letter_picker_active());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scancodes_for_letters);
    RUN_TEST(test_up_cycles_letters);
    RUN_TEST(test_down_wraps_backwards);
    RUN_TEST(test_right_commits_and_starts_at_a);
    RUN_TEST(test_queued_edges_not_dropped);
    RUN_TEST(test_b1_emits_enter_and_stops);
    return UNITY_END();
}
