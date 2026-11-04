// Playfield renderer: palette, blits, clipping, present window.
#include <unity.h>
#include <string.h>
#include "core/globals.h"
#include "render.h"
#include "screen_layout.h"
#include "play_context.h"

/* --- capture port_present; other fakes come from test_support/ --- */
static const uint8_t *presentSrc;
static int presentStride;
static uint16_t presentPal[16];
static ScreenLayout_T presentLayout;
void port_present(const ScreenLayout_T *layout, const uint8_t *src, int stride, const uint16_t pal565[16]) {
    presentLayout = *layout;
    presentSrc = src;
    presentStride = stride;
    memcpy(presentPal, pal565, sizeof presentPal);
}

/* --- test buffer with guard bytes around the playfield --- */
#define GUARD 64
static uint8_t buf[GUARD + PF_W * PF_H + GUARD];

/* 32x16 test sprite (width 4 bytes): every pixel color 3, no mask. */
static uint8_t spritePix[32 * 16];
static uint8_t *spriteTable[1];
static MaskedSpriteEntry_T spriteEntries[4];

/* tile 0: color 5; tile 1: all-0xFF mask for tile 0 */
static uint8_t tilePix[2 * 256];
static uint8_t *tileTable[2];
static uint16_t animIdentity[2] = {0, 1}; // normally set by level init

static int guardsIntact(void) {
    for (int i = 0; i < GUARD; i++)
        if (buf[i] != 0xAA || buf[GUARD + PF_W * PF_H + i] != 0xAA) return 0;
    return 1;
}

void setUp(void) {
    TEST_ASSERT_TRUE(render_init());
    memset(buf, 0xAA, sizeof buf);
    pf = buf + GUARD;
    memset(pf, 7, PF_W * PF_H);

    memset(spritePix, 3, sizeof spritePix);
    spriteTable[0] = spritePix;
    for (int i = 0; i < 4; i++) { spriteEntries[i].width = 4; spriteEntries[i].height = 16; }
    engine_egaSprites = spriteTable;
    engine_maskedSpriteEntry = spriteEntries;

    memset(tilePix, 5, 256);
    memset(tilePix + 256, 0xFF, 256);
    tileTable[0] = tilePix;
    tileTable[1] = tilePix + 256;
    engine_egaTiles = tileTable;
    for (int i = 0; i < 4; i++) anim_plane[i] = animIdentity;
    anim_plane_i = 0;

    engine_dstPage = 0;
    screen_layout_set_portrait(false);
}
void tearDown(void) {}

static uint8_t at(int x, int y) { return pf[y * PF_W + x]; }

static void test_rgb565_white(void) { TEST_ASSERT_EQUAL_HEX16(0xFFFF, ega_to_rgb565(0x3F)); }
static void test_rgb565_black(void) { TEST_ASSERT_EQUAL_HEX16(0x0000, ega_to_rgb565(0x00)); }
static void test_rgb565_red(void) { TEST_ASSERT_EQUAL_HEX16(0xA800, ega_to_rgb565(0x04)); }
static void test_rgb565_brown(void) { TEST_ASSERT_EQUAL_HEX16(0xAAA0, ega_to_rgb565(0x06)); }
// 200-line mode: secondary green (bit 4) acts as intensity -> light red.
static void test_rgb565_light_red(void) { TEST_ASSERT_EQUAL_HEX16(0xFAAA, ega_to_rgb565(0x14)); }

static void test_tile_at_origin(void) {
    CVort_engine_drawTile(4, 32, 0);
    TEST_ASSERT_EQUAL_UINT8(5, at(0, 0));
    TEST_ASSERT_EQUAL_UINT8(5, at(15, 15));
    TEST_ASSERT_EQUAL_UINT8(7, at(16, 0));
    TEST_ASSERT_EQUAL_UINT8(7, at(0, 16));
}

static void test_masked_tile_keeps_background(void) {
    CVort_engine_drawTile(4, 32, 0x8000 | 0);
    TEST_ASSERT_EQUAL_UINT8(7, at(0, 0));
}

static void test_sprite_clipped_right_bottom(void) {
    // (45<<3) + (1<<1) = 362 -> pf_x 330; y 252 -> pf_y 220
    CVort_engine_drawSprite(45, 252, 1);
    TEST_ASSERT_EQUAL_UINT8(3, at(330, 220));
    TEST_ASSERT_EQUAL_UINT8(3, at(335, 223));
    TEST_ASSERT_EQUAL_UINT8(7, at(0, 221)); // no wrap into next row
    TEST_ASSERT_TRUE(guardsIntact());
}

static void test_sprite_clipped_left_top(void) {
    // x byte 2 -> buffer x 16 -> pf_x -16; y 24 -> pf_y -8
    CVort_engine_drawSprite(2, 24, 0);
    TEST_ASSERT_EQUAL_UINT8(3, at(0, 0));
    TEST_ASSERT_EQUAL_UINT8(3, at(15, 7));
    TEST_ASSERT_EQUAL_UINT8(7, at(16, 0));
    TEST_ASSERT_EQUAL_UINT8(7, at(0, 8));
    TEST_ASSERT_TRUE(guardsIntact());
}

static void test_sprite_mask_keeps_background(void) {
    spritePix[0] = 16; // mask only
    CVort_engine_drawSprite(4, 32, 0);
    TEST_ASSERT_EQUAL_UINT8(7, at(0, 0));
    TEST_ASSERT_EQUAL_UINT8(3, at(1, 0));
}

static void test_present_window_max_fine(void) {
    scroll_x = scroll_y = 0x0F00;
    CVort_engine_egaPageFlip();
    CVort_private_engine_setEgaMemStartLocAndPanning(engine_egaStart, pel_panning);
    render_present();
    TEST_ASSERT_EQUAL_PTR(pf + 15 * PF_W + 15, presentSrc);
    TEST_ASSERT_EQUAL_INT(PF_W, presentStride);
}

static void test_palette_applies(void) {
    uint8_t palette[17] = {0x3F, 1, 2, 3, 4, 5, 6, 7, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0};
    CVort_engine_setPaletteAndBorderColor(palette);
    render_present();
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, presentPal[0]);
    TEST_ASSERT_EQUAL_HEX16(0xA800, presentPal[4]);
}

static void test_present_after_char_scales(void) {
    screen_layout_set_portrait(true);
    render_present(); // consume anything earlier tests left
    CVort_engine_drawChar(0, 0, 'A');
    render_present();
    TEST_ASSERT_EQUAL_INT(LAYOUT_SCALE, presentLayout.mode);
}

static void test_present_after_redraw_crops(void) {
    screen_layout_set_portrait(true);
    render_present();
    play_context_set(true, true);
    render_mark_full_redraw();
    render_present();
    play_context_set(false, false);
    TEST_ASSERT_EQUAL_INT(LAYOUT_CROP, presentLayout.mode);
}

static void test_present_redraw_outside_play_scales(void) {
    screen_layout_set_portrait(true);
    play_context_set(false, false);
    render_mark_full_redraw();
    render_present();
    TEST_ASSERT_EQUAL_INT(LAYOUT_SCALE, presentLayout.mode);
}

static void test_flags_cleared_after_present(void) {
    screen_layout_set_portrait(true);
    render_present();
    CVort_engine_drawChar(0, 0, 'A');
    render_present();
    render_present(); // idle: nothing drawn -> keeps SCALE
    TEST_ASSERT_EQUAL_INT(LAYOUT_SCALE, presentLayout.mode);
    play_context_set(true, true);
    render_mark_full_redraw(); // text flag must not be sticky
    render_present();
    play_context_set(false, false);
    TEST_ASSERT_EQUAL_INT(LAYOUT_CROP, presentLayout.mode);
}

/* --- tile cache: outside play only changed tiles are redrawn (upstream ATR) --- */
static void setupMap(int16_t tile) {
    map_data_tiles = map_data + 16;
    for (int i = 0; i < 0x126; i++) map_data_tiles[i] = tile;
    screen_wrap_single = 0;
    map_width_bytes = 42;
    scroll_x = scroll_y = 0;
    render_image_mode_off();
    CVort_engine_clearOverlay();
}

static void test_overlay_stays_outside_play(void) {
    play_context_set(false, false);
    setupMap(0);
    render_redraw_for_test();
    TEST_ASSERT_EQUAL_UINT8(5, at(0, 0));
    pf[0] = 9; // text drawn once by a draw_func
    render_redraw_for_test();
    TEST_ASSERT_EQUAL_UINT8(9, at(0, 0));
}

static void test_changed_tile_redrawn_outside_play(void) {
    play_context_set(false, false);
    setupMap(0);
    render_redraw_for_test();
    pf[0] = 9;
    map_data_tiles[0] = 1;
    render_redraw_for_test();
    TEST_ASSERT_EQUAL_UINT8(0xFF, at(0, 0));
}

static void test_clear_overlay_erases_text(void) {
    play_context_set(false, false);
    setupMap(0);
    render_redraw_for_test();
    pf[0] = 9;
    CVort_engine_clearOverlay();
    render_redraw_for_test();
    TEST_ASSERT_EQUAL_UINT8(5, at(0, 0));
}

static void test_scroll_redraws_all(void) {
    play_context_set(false, false);
    setupMap(0);
    render_redraw_for_test();
    pf[0] = 9;
    scroll_x = 0x1000; // one tile right
    render_redraw_for_test();
    TEST_ASSERT_EQUAL_UINT8(5, at(0, 0));
}

static void test_in_play_redraws_every_frame(void) {
    setupMap(0);
    play_context_set(true, true);
    render_redraw_for_test();
    pf[0] = 9;
    render_redraw_for_test();
    play_context_set(false, false);
    TEST_ASSERT_EQUAL_UINT8(5, at(0, 0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rgb565_white);
    RUN_TEST(test_rgb565_black);
    RUN_TEST(test_rgb565_red);
    RUN_TEST(test_rgb565_brown);
    RUN_TEST(test_rgb565_light_red);
    RUN_TEST(test_tile_at_origin);
    RUN_TEST(test_masked_tile_keeps_background);
    RUN_TEST(test_sprite_clipped_right_bottom);
    RUN_TEST(test_sprite_clipped_left_top);
    RUN_TEST(test_sprite_mask_keeps_background);
    RUN_TEST(test_present_window_max_fine);
    RUN_TEST(test_palette_applies);
    RUN_TEST(test_present_after_char_scales);
    RUN_TEST(test_present_after_redraw_crops);
    RUN_TEST(test_present_redraw_outside_play_scales);
    RUN_TEST(test_flags_cleared_after_present);
    RUN_TEST(test_overlay_stays_outside_play);
    RUN_TEST(test_changed_tile_redrawn_outside_play);
    RUN_TEST(test_clear_overlay_erases_text);
    RUN_TEST(test_scroll_redraws_all);
    RUN_TEST(test_in_play_redraws_every_frame);
    return UNITY_END();
}
