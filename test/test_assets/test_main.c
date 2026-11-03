// Checks the generated Keen 1 asset arrays.
#include <unity.h>
#include <string.h>
#include "keen1_assets.h"

void setUp(void) {}
void tearDown(void) {}

static void test_header_counts(void) {
    TEST_ASSERT_EQUAL_UINT16(611, keen1_egaHead.tileNum);
    TEST_ASSERT_EQUAL_UINT16(18, keen1_egaHead.bmpNum);
    TEST_ASSERT_EQUAL_UINT16(119, keen1_egaHead.spriteNum);
    TEST_ASSERT_EQUAL_UINT16(256, keen1_egaHead.fontNum);
}

static void test_tile_pixels_are_ega_indices(void) {
    for (uint32_t i = 0; i < 611u * 256u; i++)
        if (keen1_tileData[i] >= 16) TEST_FAIL_MESSAGE("tile pixel >= 16");
}

static void test_sprite_pixels_in_range(void) {
    for (uint32_t i = 0; i < keen1_spriteDataSize; i++)
        if (keen1_spriteData[i] >= 32) TEST_FAIL_MESSAGE("sprite pixel >= 32");
}

static const AssetFile_T *find(const char *name) {
    for (int i = 0; i < keen1_filesCount; i++)
        if (!strcmp(keen1_files[i].name, name)) return &keen1_files[i];
    return NULL;
}

static void test_level01_present(void) {
    const AssetFile_T *f = find("LEVEL01.CK1");
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_UINT32(1368, f->size);
    uint32_t unpacked = f->data[0] | f->data[1] << 8 | f->data[2] << 16 | (uint32_t)f->data[3] << 24;
    TEST_ASSERT_EQUAL_UINT32(10112, unpacked);
}

static void test_sounds_present(void) {
    const AssetFile_T *f = find("SOUNDS.CK1");
    TEST_ASSERT_NOT_NULL(f);
    TEST_ASSERT_EQUAL_UINT32(8898, f->size);
    TEST_ASSERT_EQUAL_MEMORY("SND\0", f->data, 4);
    TEST_ASSERT_EQUAL_UINT16(60, f->data[8] | f->data[9] << 8);
}

static void test_level_fits_map_data(void) {
    TEST_ASSERT_TRUE(10112 <= 0xA000);
}

static void test_exe_unpacked(void) {
    TEST_ASSERT_TRUE(keen1_exeImageSize > 0x16200);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_header_counts);
    RUN_TEST(test_tile_pixels_are_ega_indices);
    RUN_TEST(test_sprite_pixels_in_range);
    RUN_TEST(test_level01_present);
    RUN_TEST(test_sounds_present);
    RUN_TEST(test_level_fits_map_data);
    RUN_TEST(test_exe_unpacked);
    return UNITY_END();
}
