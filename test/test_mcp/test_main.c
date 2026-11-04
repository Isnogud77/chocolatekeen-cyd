// MCP23017 setup sequence: every register the input path depends on.
#include <unity.h>
#include "mcp23017.h"
#include "backend.h"

void setUp(void) {}
void tearDown(void) {}

static int indexOf(uint8_t reg) {
    for (int i = 0; i < mcp_init_count; i++)
        if (mcp_init_seq[i].reg == reg) return i;
    return -1;
}

static void assertWrite(uint8_t reg, uint8_t val) {
    int i = indexOf(reg);
    TEST_ASSERT_TRUE_MESSAGE(i >= 0, "register missing from init sequence");
    TEST_ASSERT_EQUAL_HEX8(val, mcp_init_seq[i].val);
}

// Garbage seen on the device after a hot-plug: IPOLA = 0x7A, IOCON = 0x7A.
static void test_polarity_reset(void) { assertWrite(0x02, 0x00); assertWrite(0x03, 0x00); }
static void test_iocon_reset(void) { assertWrite(0x0A, 0x00); }
static void test_inputs_with_pullups(void) {
    assertWrite(0x00, 0xFF); assertWrite(0x01, 0xFF);
    assertWrite(0x0C, 0xFF); assertWrite(0x0D, 0xFF);
}
static void test_interrupts_off(void) { assertWrite(0x04, 0x00); }

// If IOCON.BANK got set, 0x05 is IOCON; writing 0 there first brings
// the chip back to BANK=0 addressing (in BANK=0, 0x05 is GPINTENB: 0 is fine).
static void test_bank_reset_first(void) {
    TEST_ASSERT_EQUAL_HEX8(0x05, mcp_init_seq[0].reg);
    TEST_ASSERT_EQUAL_HEX8(0x00, mcp_init_seq[0].val);
}

static void test_reinit_due_every_second(void) {
    TEST_ASSERT_FALSE(mcp_reinit_due(1000, 1999));
    TEST_ASSERT_TRUE(mcp_reinit_due(1000, 2000));
    TEST_ASSERT_TRUE(mcp_reinit_due(0xFFFFFF00u, 0x00000300u)); // millis wrap
}

// Buttons are active low; GPA0..5 = game buttons, GPB0 = Start, GPB1 = Menu.
static void test_buttons_none(void) { TEST_ASSERT_EQUAL_HEX8(0, mcp_buttons_from_gpio(0xFF, 0xFF)); }
static void test_buttons_gpa(void) {
    TEST_ASSERT_EQUAL_HEX8(PORT_BTN_LEFT, mcp_buttons_from_gpio(0xFE, 0xFF));
    TEST_ASSERT_EQUAL_HEX8(PORT_BTN_POGO, mcp_buttons_from_gpio(0xDF, 0xFF));
    TEST_ASSERT_EQUAL_HEX8(PORT_BTN_JUMP | PORT_BTN_POGO, mcp_buttons_from_gpio(0xCF, 0xFF));
}
static void test_buttons_gpb(void) {
    TEST_ASSERT_EQUAL_HEX8(PORT_BTN_START, mcp_buttons_from_gpio(0xFF, 0xFE));
    TEST_ASSERT_EQUAL_HEX8(PORT_BTN_MENU, mcp_buttons_from_gpio(0xFF, 0xFD));
    TEST_ASSERT_EQUAL_HEX8(PORT_BTN_START | PORT_BTN_MENU, mcp_buttons_from_gpio(0xFF, 0xFC));
}
static void test_buttons_ignore_unused_pins(void) {
    TEST_ASSERT_EQUAL_HEX8(0, mcp_buttons_from_gpio(0x3F, 0x03)); // GPA6/7 and GPB2..7 low: not buttons
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_polarity_reset);
    RUN_TEST(test_iocon_reset);
    RUN_TEST(test_inputs_with_pullups);
    RUN_TEST(test_interrupts_off);
    RUN_TEST(test_bank_reset_first);
    RUN_TEST(test_reinit_due_every_second);
    RUN_TEST(test_buttons_none);
    RUN_TEST(test_buttons_gpa);
    RUN_TEST(test_buttons_gpb);
    RUN_TEST(test_buttons_ignore_unused_pins);
    return UNITY_END();
}
