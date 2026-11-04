// Per-frame layout decision for landscape / portrait.
#include <unity.h>
#include "screen_layout.h"

static uint16_t pal[16];
static uint8_t src[200 * 320];
static uint16_t out[320];

void setUp(void) {
    for (int i = 0; i < 16; i++) pal[i] = (uint16_t)(0x1000 + i);
    for (int i = 0; i < 200 * 320; i++) src[i] = (uint8_t)((i * 7 + i / 320) & 15);
}
void tearDown(void) {}

static void assertLayout(ScreenLayout_T l, LayoutMode_T mode, int sw, int sh, int ow, int oh, int top, int cropX) {
    TEST_ASSERT_EQUAL_INT(mode, l.mode);
    TEST_ASSERT_EQUAL_INT(sw, l.screenW);
    TEST_ASSERT_EQUAL_INT(sh, l.screenH);
    TEST_ASSERT_EQUAL_INT(ow, l.outW);
    TEST_ASSERT_EQUAL_INT(oh, l.outH);
    TEST_ASSERT_EQUAL_INT(top, l.top);
    TEST_ASSERT_EQUAL_INT(cropX, l.cropX);
}

static void test_landscape_layout(void) {
    screen_layout_set_portrait(false);
    assertLayout(screen_layout_frame(true, true, true, 160), LAYOUT_LANDSCAPE, 320, 240, 320, 200, 20, 0);
    assertLayout(screen_layout_frame(false, false, true, 0), LAYOUT_LANDSCAPE, 320, 240, 320, 200, 20, 0);
    TEST_ASSERT_EQUAL_INT(320, screen_layout_screen_w());
    TEST_ASSERT_EQUAL_INT(240, screen_layout_screen_h());
}

static void test_landscape_row_is_identity(void) {
    screen_layout_set_portrait(false);
    ScreenLayout_T l = screen_layout_frame(false, true, true, 0);
    screen_layout_row(&l, src, 320, 5, pal, out);
    for (int x = 0; x < 320; x++) TEST_ASSERT_EQUAL_HEX16(pal[src[5 * 320 + x]], out[x]);
}

static void test_portrait_game_frame_crops(void) {
    screen_layout_set_portrait(true);
    assertLayout(screen_layout_frame(false, true, true, 160), LAYOUT_CROP, 240, 320, 240, 200, 60, 48);
    TEST_ASSERT_EQUAL_INT(240, screen_layout_screen_w());
    TEST_ASSERT_EQUAL_INT(320, screen_layout_screen_h());
}

static void test_portrait_text_frame_scales(void) {
    screen_layout_set_portrait(true);
    assertLayout(screen_layout_frame(true, false, true, 160), LAYOUT_SCALE, 240, 320, 240, 150, 85, 0);
}

static void test_text_wins_over_redraw(void) {
    screen_layout_set_portrait(true);
    TEST_ASSERT_EQUAL_INT(LAYOUT_SCALE, screen_layout_frame(true, true, true, 160).mode);
}

static void test_idle_keeps_mode(void) {
    screen_layout_set_portrait(true);
    screen_layout_frame(true, false, true, 160);
    TEST_ASSERT_EQUAL_INT(LAYOUT_SCALE, screen_layout_frame(false, false, true, 160).mode);
    screen_layout_frame(false, true, true, 160);
    TEST_ASSERT_EQUAL_INT(LAYOUT_CROP, screen_layout_frame(false, false, true, 160).mode);
}

static void test_play_redraw_crops(void) {
    screen_layout_set_portrait(true);
    TEST_ASSERT_EQUAL_INT(LAYOUT_CROP, screen_layout_frame(false, true, true, 160).mode);
}

static void test_play_with_text_scales(void) {
    screen_layout_set_portrait(true);
    TEST_ASSERT_EQUAL_INT(LAYOUT_SCALE, screen_layout_frame(true, false, true, 160).mode);
}

static void test_no_play_scales_even_on_redraw(void) {
    screen_layout_set_portrait(true);
    TEST_ASSERT_EQUAL_INT(LAYOUT_SCALE, screen_layout_frame(false, true, false, 160).mode);
}

static void test_play_idle_keeps_scale_after_text(void) {
    screen_layout_set_portrait(true);
    screen_layout_frame(true, false, true, 160);
    TEST_ASSERT_EQUAL_INT(LAYOUT_SCALE, screen_layout_frame(false, false, true, 160).mode);
    TEST_ASSERT_EQUAL_INT(LAYOUT_SCALE, screen_layout_frame(false, false, true, 160).mode);
    TEST_ASSERT_EQUAL_INT(LAYOUT_CROP, screen_layout_frame(false, true, true, 160).mode);
}

static void test_initial_mode_is_crop(void) {
    screen_layout_set_portrait(true);
    TEST_ASSERT_EQUAL_INT(LAYOUT_CROP, screen_layout_frame(false, false, true, 0).mode);
}

static void test_crop_clamps(void) {
    screen_layout_set_portrait(true);
    TEST_ASSERT_EQUAL_INT(0, screen_layout_frame(false, true, true, -50).cropX);
    TEST_ASSERT_EQUAL_INT(80, screen_layout_frame(false, true, true, 400).cropX);
    TEST_ASSERT_EQUAL_INT(0, screen_layout_frame(false, true, true, 112).cropX);
    TEST_ASSERT_EQUAL_INT(80, screen_layout_frame(false, true, true, 200).cropX);
}

static void test_crop_row_offset(void) {
    screen_layout_set_portrait(true);
    ScreenLayout_T l = screen_layout_frame(false, true, true, 160); // cropX 48
    screen_layout_row(&l, src, 320, 7, pal, out);
    for (int x = 0; x < 240; x++) TEST_ASSERT_EQUAL_HEX16(pal[src[7 * 320 + 48 + x]], out[x]);
}

static void test_scale_row_mapping(void) {
    screen_layout_set_portrait(true);
    ScreenLayout_T l = screen_layout_frame(true, false, true, 160);
    screen_layout_row(&l, src, 320, 3, pal, out);
    for (int x = 0; x < 240; x++) TEST_ASSERT_EQUAL_HEX16(pal[src[4 * 320 + x * 4 / 3]], out[x]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_landscape_layout);
    RUN_TEST(test_landscape_row_is_identity);
    RUN_TEST(test_portrait_game_frame_crops);
    RUN_TEST(test_portrait_text_frame_scales);
    RUN_TEST(test_text_wins_over_redraw);
    RUN_TEST(test_idle_keeps_mode);
    RUN_TEST(test_play_redraw_crops);
    RUN_TEST(test_play_with_text_scales);
    RUN_TEST(test_no_play_scales_even_on_redraw);
    RUN_TEST(test_play_idle_keeps_scale_after_text);
    RUN_TEST(test_initial_mode_is_crop);
    RUN_TEST(test_crop_clamps);
    RUN_TEST(test_crop_row_offset);
    RUN_TEST(test_scale_row_mapping);
    return UNITY_END();
}
