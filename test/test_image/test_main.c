// Image screens (FINALE), image mode and text scrolling.
#include <unity.h>
#include <string.h>
#include "core/globals.h"
#include "keen1_assets.h"
#include "render.h"

/* Time advances on every query so the frame-sync delays in showImageFile finish. */
uint32_t port_millis(void) { static uint32_t t; return t += 20; }

void setUp(void) {
    // Timing as port_game_run sets it; 0 makes CVort_engine_delay divide by
    // zero (traps on x86 Linux, silently 0 on Apple Silicon).
    engine_refreshTimeInMicroSec = ((int64_t)1000000) * ENGINE_EGAVGA_REFRESHRATE_SCALE_FACTOR / ENGINE_EGA_REFRESHRATE_SCALED;
    snprintf(game_ext, sizeof(game_ext), "CK1");
    TEST_ASSERT_TRUE(render_init());
    render_image_mode_off();
}
void tearDown(void) {}

/* Independent decode: 4-byte LE length, then runs (>=0x80: n-0x7F literals,
   else next byte repeated n+3 times); 4 planes of 0x2000 bytes. */
static uint8_t planes[0x8000];
static void decodeOracle(const AssetFile_T *f) {
    const uint8_t *p = f->data + 4;
    uint32_t out = 0;
    while (out < sizeof planes) {
        uint8_t c = *p++;
        if (c >= 0x80) {
            for (int n = c - 0x7F; n-- && out < sizeof planes;) planes[out++] = *p++;
        } else {
            uint8_t v = *p++;
            for (int n = c + 3; n-- && out < sizeof planes;) planes[out++] = v;
        }
    }
}
static uint8_t oraclePixel(int x, int y) {
    int idx = y * 320 + x, sh = 7 - (idx % 8), v = 0;
    for (int pl = 0; pl < 4; pl++) v |= ((planes[pl * 0x2000 + idx / 8] >> sh) & 1) << pl;
    return (uint8_t)v;
}
#define PX(x, y) pf[(y) * PF_W + (x)]

static void test_finale_pixels_match_oracle(void) {
    const AssetFile_T *f = port_find_asset("FINALE.CK1");
    TEST_ASSERT_NOT_NULL(f);
    decodeOracle(f);
    CVort_engine_showImageFile("FINALE.CK1");
    int nonzero = 0;
    for (int i = 0; i < 50; i++) {
        int idx = (i * 1277 + 31) % 64000, x = idx % 320, y = idx / 320;
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(oraclePixel(x, y), PX(x, y), "pixel");
        nonzero += oraclePixel(x, y) != 0;
    }
    TEST_ASSERT_GREATER_THAN(0, nonzero); // the sample is not all black
    TEST_ASSERT_EQUAL(0, scroll_x);
    TEST_ASSERT_EQUAL(0, scroll_y);
    TEST_ASSERT_TRUE(render_image_mode());
}

static void test_image_mode_skips_tiles(void) {
    CVort_engine_showImageFile("FINALE.CK1");
    static uint8_t snap[PF_W * PF_H];
    memcpy(snap, pf, sizeof snap);
    render_redraw_for_test();
    TEST_ASSERT_EQUAL_MEMORY(snap, pf, sizeof snap);
    // a sprite draw still lands
    CVort_engine_drawSprite(24, 32 + 60, 1);
    TEST_ASSERT_TRUE(memcmp(snap, pf, sizeof snap) != 0);
}

static void test_level_load_clears_image_mode(void) {
    CVort_engine_showImageFile("FINALE.CK1");
    TEST_ASSERT_TRUE(render_image_mode());
    CVort_load_level_data(1);
    TEST_ASSERT_FALSE(render_image_mode());
}

static void fillRows(void) {
    for (int r = 0; r < PF_H; r++) memset(pf + r * PF_W, r, PF_W);
}
static void test_scroll_text_up_down(void) {
    engine_dstPage = 0x3000; // pf row 0
    fillRows();
    CVort_engine_scrollText(2, 5, 0);
    for (int r = 16; r < 40; r++) TEST_ASSERT_EQUAL_UINT8(r + 8, PX(10, r));
    TEST_ASSERT_EQUAL_UINT8(15, PX(10, 15));
    TEST_ASSERT_EQUAL_UINT8(40, PX(10, 40));
    TEST_ASSERT_EQUAL_UINT8(47, PX(PF_W - 1, 39));
    fillRows();
    CVort_engine_scrollText(2, 5, 1);
    for (int r = 24; r < 48; r++) TEST_ASSERT_EQUAL_UINT8(r - 8, PX(10, r));
    TEST_ASSERT_EQUAL_UINT8(23, PX(10, 23));
    TEST_ASSERT_EQUAL_UINT8(48, PX(10, 48));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_finale_pixels_match_oracle);
    RUN_TEST(test_image_mode_skips_tiles);
    RUN_TEST(test_level_load_clears_image_mode);
    RUN_TEST(test_scroll_text_up_down);
    return UNITY_END();
}
