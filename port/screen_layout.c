// Per-frame screen layout. See screen_layout.h and the orientation spec.
#include "screen_layout.h"

static bool portrait;
static LayoutMode_T lastMode = LAYOUT_CROP;

void screen_layout_set_portrait(bool p) {
    portrait = p;
    lastMode = LAYOUT_CROP;
}

bool screen_layout_portrait(void) { return portrait; }
int screen_layout_screen_w(void) { return portrait ? 240 : 320; }
int screen_layout_screen_h(void) { return portrait ? 320 : 240; }

ScreenLayout_T screen_layout_frame(bool textDrawn, bool fullRedraw, bool inPlay, int keenX) {
    if (!portrait)
        return (ScreenLayout_T){LAYOUT_LANDSCAPE, 320, 240, 320, 200, 20, 0};
    if (textDrawn || !inPlay)
        lastMode = LAYOUT_SCALE;
    else if (fullRedraw)
        lastMode = LAYOUT_CROP;
    if (lastMode == LAYOUT_SCALE)
        return (ScreenLayout_T){LAYOUT_SCALE, 240, 320, 240, 150, 85, 0};
    int cropX = keenX + 8 - 120; // Keen is ~16 px wide: keep him centered
    cropX = cropX < 0 ? 0 : (cropX > 80 ? 80 : cropX);
    return (ScreenLayout_T){LAYOUT_CROP, 240, 320, 240, 200, 60, cropX};
}

void screen_layout_row(const ScreenLayout_T *l, const uint8_t *src, int stride, int y,
                       const uint16_t pal[16], uint16_t *out) {
    if (l->mode == LAYOUT_SCALE) {
        const uint8_t *row = src + (y * 4 / 3) * stride;
        for (int x = 0; x < l->outW; x++) out[x] = pal[row[x * 4 / 3] & 15];
        return;
    }
    const uint8_t *row = src + y * stride + l->cropX;
    for (int x = 0; x < l->outW; x++) out[x] = pal[row[x] & 15];
}
