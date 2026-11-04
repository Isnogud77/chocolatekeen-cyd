// Quit paths on a cabinet: no DOS, a plain quit lands in the title, an error halts.
#include <unity.h>
#include <setjmp.h>
#include <string.h>
#include "core/globals.h"

static jmp_buf fatalJmp;
static const char *fatalMsg;
void port_fatal(const char *msg) {
    fatalMsg = msg;
    longjmp(fatalJmp, 1);
}

void setUp(void) {
    g_game.quit_to_title = 0;
    fatalMsg = NULL;
}
void tearDown(void) {}

static void test_plain_quit_returns_to_title(void) {
    if (setjmp(fatalJmp)) TEST_FAIL_MESSAGE("plain quit must not halt");
    CVort_chg_vid_and_error("");
    TEST_ASSERT_EQUAL_INT(1, g_game.quit_to_title);
    TEST_ASSERT_NULL(fatalMsg);
}

static void test_error_message_is_fatal(void) {
    if (!setjmp(fatalJmp)) {
        CVort_chg_vid_and_error("Bad think pointer!");
        TEST_FAIL_MESSAGE("error must halt");
    }
    TEST_ASSERT_EQUAL_STRING("Bad think pointer!", fatalMsg);
    TEST_ASSERT_EQUAL_INT(0, g_game.quit_to_title);
}

static void test_engine_handle_quit_goes_to_title(void) {
    CVort_engine_handleQuit();
    TEST_ASSERT_EQUAL_INT(1, g_game.quit_to_title);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_plain_quit_returns_to_title);
    RUN_TEST(test_error_message_is_fatal);
    RUN_TEST(test_engine_handle_quit_goes_to_title);
    return UNITY_END();
}
