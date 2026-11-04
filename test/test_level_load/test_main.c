// Level loading into the (port-shrunk) map_data buffer.
#include <unity.h>
#include <string.h>
#include "core/globals.h"
#include "keen1_assets.h"

void setUp(void) { snprintf(game_ext, sizeof(game_ext), "CK1"); }
void tearDown(void) {}

static const AssetFile_T *find(const char *name) {
    for (int i = 0; i < keen1_filesCount; i++)
        if (!strcmp(keen1_files[i].name, name)) return &keen1_files[i];
    return NULL;
}

// Independent RLEW decoder (flag 0xFEFE) as the oracle: words after the
// 4-byte size header, i.e. what map_data must hold after loading.
static uint16_t expected[0xA000 / 2];
static uint32_t decodeOracle(const AssetFile_T *f) {
    const uint8_t *p = f->data;
    uint32_t size = p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
    uint32_t in = 4, out = 0;
    while (out < size / 2 && in + 1 < f->size) {
        uint16_t w = p[in] | p[in + 1] << 8;
        in += 2;
        if (w == 0xFEFE) {
            uint16_t n = p[in] | p[in + 1] << 8, v = p[in + 2] | p[in + 3] << 8;
            in += 4;
            while (n-- && out < size / 2) expected[out++] = v;
        } else
            expected[out++] = w;
    }
    return out;
}

static void checkLevel(int num, const char *file) {
    const AssetFile_T *f = find(file);
    TEST_ASSERT_NOT_NULL(f);
    uint32_t words = decodeOracle(f);
    // Upstream CRLE_expandSwapped decodes the 2nd (zero) header word as data, so it stops one word
    // short; the final word (bottom-right sprite-plane cell, unused by the game) is never produced.
    if (words) words--;
    memset(map_data, 0x55, sizeof(map_data));
    CVort_load_level_data(num);
    for (uint32_t i = 0; i < words; i++)
        if ((uint16_t)map_data[i] != expected[i]) {
            char msg[64];
            snprintf(msg, sizeof msg, "word %u: got 0x%04X want 0x%04X", i, (uint16_t)map_data[i], expected[i]);
            TEST_FAIL_MESSAGE(msg);
        }
}

static void test_level01_loads(void) { checkLevel(1, "LEVEL01.CK1"); }
// 25632 bytes unpacked: past the first 20478 bytes the old memmove moved.
static void test_level14_loads(void) { checkLevel(14, "LEVEL14.CK1"); }

static void test_all_levels_load(void) {
    static const int nums[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 80, 81, 90};
    for (unsigned i = 0; i < sizeof nums / sizeof nums[0]; i++) {
        char name[16];
        snprintf(name, sizeof name, "LEVEL%02d.CK1", nums[i]);
        checkLevel(nums[i], name);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_level01_loads);
    RUN_TEST(test_level14_loads);
    RUN_TEST(test_all_levels_load);
    return UNITY_END();
}
