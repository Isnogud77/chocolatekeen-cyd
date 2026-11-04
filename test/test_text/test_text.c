// Text files (STORYTXT, HELPTEXT, ENDTEXT) after upstream's loader: upstream
// expects CR LF line ends. A stray LF is drawn as a glyph (a blue ball).
#include <unity.h>
#include <stdlib.h>
#include <string.h>
#include "core/globals.h"

void CVort_load_and_process_text_file(const char *filename, uint8_t **pBuffer);

void setUp(void) {}
void tearDown(void) {}

static void checkNoLineFeeds(const char *name) {
    uint8_t *buf = NULL;
    CVort_load_and_process_text_file(name, &buf);
    TEST_ASSERT_NOT_NULL(buf);
    size_t n = 0, lf = 0, cr = 0;
    for (; buf[n] != 0x1A; n++) {
        lf += buf[n] == 0x0A && (n == 0 || buf[n - 1] != 0x0D);
        cr += buf[n] == 0x0D;
    }
    TEST_ASSERT_GREATER_THAN(100, n);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, lf, "LF without CR left in text (drawn as a glyph)");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, cr, "no line ends");
    free(buf);
}

static void test_story(void) { checkNoLineFeeds("STORYTXT.CK1"); }
static void test_end(void) { checkNoLineFeeds("ENDTEXT.CK1"); }
static void test_help(void) { checkNoLineFeeds("HELPTEXT.CK1"); }

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_story);
    RUN_TEST(test_end);
    RUN_TEST(test_help);
    return UNITY_END();
}
