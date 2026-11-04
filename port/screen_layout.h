// Per-frame screen layout: landscape (320x200 picture on 320x240), or portrait
// (240x320) where game frames show a 240x200 window following Keen and frames
// with text/bitmaps are scaled 3:4 to 240x150. No platform code.
#ifndef PORT_SCREEN_LAYOUT_H
#define PORT_SCREEN_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { LAYOUT_LANDSCAPE, LAYOUT_CROP, LAYOUT_SCALE } LayoutMode_T;

typedef struct {
    LayoutMode_T mode;
    int screenW, screenH; // physical screen
    int outW, outH, top;  // picture size and its first screen row
    int cropX;            // CROP only: first source column
} ScreenLayout_T;

void screen_layout_set_portrait(bool portrait); // also resets the mode to CROP
bool screen_layout_portrait(void);
int screen_layout_screen_w(void);
int screen_layout_screen_h(void);

// Decision for the frame being presented (see spec "Mode decision").
ScreenLayout_T screen_layout_frame(bool textDrawn, bool fullRedraw, bool inPlay, int keenX);

// Writes output row y (l->outW pixels) from the 320x200 game window src.
void screen_layout_row(const ScreenLayout_T *l, const uint8_t *src, int stride, int y,
                       const uint16_t pal[16], uint16_t *out);

#ifdef __cplusplus
}
#endif
#endif
