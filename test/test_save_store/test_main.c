// Upstream file API over the key-value store ("file on a blob").
#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "backend.h"
#include "save_store.h"

void fake_store_reset(void);

static uint8_t pattern[2100];

void setUp(void) {
    fake_store_reset();
    for (int i = 0; i < (int)sizeof pattern; i++) pattern[i] = (uint8_t)(i * 7 + 3);
}
void tearDown(void) {}

static int storeLen(const char *name) {
    static uint8_t buf[2048];
    return port_store_load(name, buf, sizeof buf);
}

static void test_write_close_read_roundtrip(void) {
    FILE *w = port_save_fopen("SAVED1.CK1", "wb");
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL(92, fwrite(pattern, 1, 92, w));
    TEST_ASSERT_EQUAL(0, fclose(w));
    FILE *r = port_save_fopen("SAVED1.CK1", "rb");
    TEST_ASSERT_NOT_NULL(r);
    uint8_t got[100] = {0};
    TEST_ASSERT_EQUAL(92, fread(got, 1, sizeof got, r));
    TEST_ASSERT_EQUAL_MEMORY(pattern, got, 92);
    fclose(r);
}

static void test_nothing_stored_before_close(void) {
    FILE *w = port_save_fopen("SAVED1.CK1", "wb");
    fwrite(pattern, 1, 50, w);
    fflush(w);
    TEST_ASSERT_EQUAL(-1, storeLen("SAVED1.CK1"));
    fclose(w);
    TEST_ASSERT_EQUAL(50, storeLen("SAVED1.CK1"));
}

static void test_missing_read_is_null(void) {
    TEST_ASSERT_NULL(port_save_fopen("SAVED1.CK1", "rb"));
}

static void test_unserved_names_null(void) {
    TEST_ASSERT_NULL(port_save_fopen("CTLPANEL.CK1", "rb"));
    TEST_ASSERT_NULL(port_save_fopen("CTLPANEL.CK1", "wb"));
    TEST_ASSERT_NULL(port_save_fopen("DEMO0.CK1", "wb"));
}

static void test_too_large_write_not_committed(void) {
    FILE *w = port_save_fopen("SCORES.CK1", "wb");
    fwrite(pattern, 1, 10, w);
    fclose(w);
    w = port_save_fopen("SCORES.CK1", "wb");
    fwrite(pattern, 1, 2049, w);
    TEST_ASSERT_NOT_EQUAL(0, fclose(w));
    TEST_ASSERT_EQUAL(10, storeLen("SCORES.CK1")); // old good data kept
}

static void test_overwrite_replaces(void) {
    FILE *w = port_save_fopen("SCORES.CK1", "wb");
    fwrite(pattern, 1, 30, w);
    fclose(w);
    w = port_save_fopen("SCORES.CK1", "wb");
    fwrite(pattern + 5, 1, 20, w);
    fclose(w);
    FILE *r = port_save_fopen("SCORES.CK1", "rb");
    uint8_t got[40];
    TEST_ASSERT_EQUAL(20, fread(got, 1, sizeof got, r));
    TEST_ASSERT_EQUAL_MEMORY(pattern + 5, got, 20);
    fclose(r);
}

// Upstream's continue checks CVort_filelength(fp) == 92 (fseek/ftell).
static void test_reader_seek_and_tell(void) {
    FILE *w = port_save_fopen("SAVED1.CK1", "wb");
    fwrite(pattern, 1, 92, w);
    fclose(w);
    FILE *r = port_save_fopen("SAVED1.CK1", "rb");
    TEST_ASSERT_EQUAL(0, fseek(r, 0, SEEK_END));
    TEST_ASSERT_EQUAL(92, ftell(r));
    TEST_ASSERT_EQUAL(0, fseek(r, 0, SEEK_SET));
    uint8_t b;
    TEST_ASSERT_EQUAL(1, fread(&b, 1, 1, r));
    TEST_ASSERT_EQUAL_HEX8(pattern[0], b);
    fclose(r);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_write_close_read_roundtrip);
    RUN_TEST(test_nothing_stored_before_close);
    RUN_TEST(test_missing_read_is_null);
    RUN_TEST(test_unserved_names_null);
    RUN_TEST(test_too_large_write_not_committed);
    RUN_TEST(test_overwrite_replaces);
    RUN_TEST(test_reader_seek_and_tell);
    return UNITY_END();
}
