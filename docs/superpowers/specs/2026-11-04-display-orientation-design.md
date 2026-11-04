# Display Orientation (Landscape / Portrait) — Design

Date: 2026-11-04
Part 1 of 3 of the full-game work: **1. display layer** → 2. full game flow →
3. saving.
Builds on the level 1 and sound milestones (merged), and on the portrait spike
(branch `spike-portrait`, throwaway, tested on device).

## Goal

The display orientation is chosen at build time: landscape (the current
behavior), or portrait for upright cabinets like Galagino's. The design must
work for the full game later (title, menus, text screens), even though only
level 1 exists now.

**Done when:**

- `cyd` and `host` look exactly as today: 320×200 at y 20 on 320×240.
- `cyd_portrait` and `host_portrait` show gameplay as a 240×200 1:1 window
  following Keen, at y 60 on 240×320.
- In portrait, frames with text or bitmaps are scaled 3:4 to 240×150 at y 85.
- On the device: fps ≈ 24 in both orientations, heap within 1 KB of today
  (49 180 B free).

**Out of scope:** a HUD in the free portrait space, switching orientation at
runtime, and making upstream text screens appear (that is part 2).

## Facts from the spike (on device, 2026-11-04)

- TFT `setRotation(0)` is the right way up in the cabinet; landscape uses
  `setRotation(1)`.
- 240×200 crop following Keen looks good and keeps 24.3 fps. Scaling gameplay
  to 240×150 is too small.
- 3:4 scaling of Keen-font text screens is readable (user chose it over cropping).
- Keen's x in the 320-wide window = `(sprites[0].pos_x >> 8) - (scroll_x >> 8)`.
  Crop offset = clamp(Keen x + 8 − 120, 0, 80); Keen is ~16 px wide.

## Configuration

`platformio.ini` gets the envs `cyd_portrait` (extends `cyd`) and
`host_portrait` (extends `host`), each adding `-DKEEN_PORTRAIT=1`. Without the
flag, the build is landscape.

## Structure

```
port/screen_layout.c/.h   platform-neutral: per-frame layout decision + row writer
port/render.c             reports per frame: text drawn? full redraw? Keen x
port/esp32/, port/host/   set rotation/window size from the layout's screen size,
                          write rows via screen_layout, push at the layout's top
```

`screen_layout.h` (names are fixed here; the plan pins the exact types):

- `ScreenLayout_T`: `mode` (`LAYOUT_LANDSCAPE`, `LAYOUT_CROP`, `LAYOUT_SCALE`),
  `screenW`, `screenH`, `outW`, `outH`, `top`, `cropX`.
- `screen_layout_frame(bool textDrawn, bool fullRedraw, int keenX)` →
  `ScreenLayout_T`: the decision for the frame being presented.
- `screen_layout_row(const ScreenLayout_T *, const uint8_t *src, int stride,
  int y, const uint16_t pal[16], uint16_t *out)`: writes output row `y`
  (`outW` pixels).

## Mode decision (portrait builds)

Per presented frame:

1. Text or a bitmap was drawn since the last present (`drawChar`,
   `drawBitmap`, `showImageFile`) → **SCALE**: 240×150 at y 85, source pixel
   = output × 4/3 in both axes.
2. Otherwise, if a full playfield redraw (`doDrawing`) happened since the last
   present → **CROP**: 240×200 at y 60, x offset from Keen as above.
3. Otherwise (idle refresh) → keep the previous mode, so the screen does not
   flicker between modes.

When the mode changes, the backend clears the screen once (the bars differ in
size). Landscape builds always return 320×200 at y 20, with screen 320×240.

## Testing

Unit tests (host, `pio test -e test`), with `KEEN_PORTRAIT` both on and off:

- landscape layout values;
- portrait game frame → CROP at y 60; text frame → SCALE at y 85; idle →
  previous mode;
- crop clamped to 0 and 80 at level edges;
- scale row mapping: output (x, y) reads source (x·4/3, y·4/3);
- crop row mapping: output x reads source x + cropX.

Host: `host_portrait` with `KEEN_AUTO` and `KEEN_DUMP` → game frames cropped.
The device check (both orientations: picture, fps, heap) happens once the CYD
is connected again.
