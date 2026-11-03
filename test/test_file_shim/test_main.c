// fopen shim: game data files served from flash arrays.
#include <unity.h>
#include <stdio.h>

FILE *port_fopen_asset(const char *name);

void setUp(void) {}
void tearDown(void) {}

static void test_open_level01(void) {
    FILE *fp = port_fopen_asset("LEVEL01.CK1");
    TEST_ASSERT_NOT_NULL(fp);
    fseek(fp, 0, SEEK_END);
    TEST_ASSERT_EQUAL_INT32(1368, ftell(fp));
    fclose(fp);
}

static void test_open_lowercase(void) {
    FILE *fp = port_fopen_asset("level01.ck1");
    TEST_ASSERT_NOT_NULL(fp);
    fclose(fp);
}

static void test_missing(void) {
    TEST_ASSERT_NULL(port_fopen_asset("LEVEL99.CK1"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_open_level01);
    RUN_TEST(test_open_lowercase);
    RUN_TEST(test_missing);
    return UNITY_END();
}
