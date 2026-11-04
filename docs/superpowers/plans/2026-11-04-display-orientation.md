# Display Orientation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build-time choice of landscape (today's output) or portrait (240×200 crop following Keen for game frames, 3:4 scaled 240×150 for text frames).

**Architecture:** A platform-neutral `screen_layout` unit decides each frame's layout from three facts `render.c` reports (text drawn, full redraw, Keen x) and writes output rows. Backends size their screen from it and push rows at the layout's `top`. Portrait is a runtime flag of `screen_layout`, set from `KEEN_PORTRAIT` at boot, so tests cover both orientations in one build.

**Tech Stack:** C99, Unity, SDL2 (host), TFT_eSPI DMA (ESP32).

**Spec:** `docs/superpowers/specs/2026-11-04-display-orientation-design.md`

## Global Constraints

- Landscape: screen 320×240, output 320×200 at top 20, `setRotation(1)` — identical to today.
- Portrait: screen 240×320, `setRotation(0)`. CROP: 240×200 at top 60, cropX = clamp(keenX + 8 − 120, 0, 80). SCALE: 240×150 at top 85, source (x·4/3, y·4/3).
- Mode rule (portrait): text/bitmap drawn since last present → SCALE; else full redraw since last present → CROP; else previous mode. Initial mode CROP.
- Keen x in the 320 window = `(g_entities.sprites[0].pos_x >> 8) - (scroll_x >> 8)`.
- Envs `cyd_portrait` / `host_portrait` extend `cyd` / `host` with `-DKEEN_PORTRAIT=1`.
- Device targets: fps ≈ 24 both orientations, heap within 1 KB of 49 180 B free. Device check is deferred until the CYD is connected.
- Spike branch `spike-portrait` is reference only; nothing is merged from it.

## Review Focus

1. **Mode flicker** on screens that redraw every frame and draw text through `draw_func` (both flags set in one frame) → must stay SCALE. Task 1 test `test_text_wins_over_redraw`.
2. **Keen far off-window** (keenX negative or > 320, e.g. world map or before init) → cropX stays in 0..80. Task 1 tests.
3. **Bars left over** from the previous mode after a switch (SCALE bars are 85 px, CROP 60 px) → backend clears once on mode change; review by reading code + host dump after a forced switch.
4. **DMA strip sizing** in portrait SCALE: 150 is not a multiple of 20 → last strip must be 10 lines, no overrun of the 12 800-byte buffers. Review.
5. **Landscape regressions**: output must be byte-identical to today's path. Task 1 test compares landscape row to the source row.

---

### Task 1: screen_layout unit

**Files:**
- Create: `port/screen_layout.h`, `port/screen_layout.c`, `test/test_screen_layout/test_main.c`

**Interfaces:**
- Produces:
  ```c
  typedef enum { LAYOUT_LANDSCAPE, LAYOUT_CROP, LAYOUT_SCALE } LayoutMode_T;
  typedef struct { LayoutMode_T mode; int screenW, screenH, outW, outH, top, cropX; } ScreenLayout_T;
  void screen_layout_set_portrait(bool portrait); // also resets mode to CROP
  bool screen_layout_portrait(void);
  int  screen_layout_screen_w(void);              // 320 or 240
  int  screen_layout_screen_h(void);              // 240 or 320
  ScreenLayout_T screen_layout_frame(bool textDrawn, bool fullRedraw, int keenX);
  void screen_layout_row(const ScreenLayout_T *l, const uint8_t *src, int stride, int y,
                         const uint16_t pal[16], uint16_t *out); // writes l->outW pixels
  ```

- [ ] **Step 1: Failing tests** (each test calls `screen_layout_set_portrait` first):
  - `test_landscape_layout`: portrait off → `{LANDSCAPE, 320, 240, 320, 200, 20, 0}` for any flags.
  - `test_landscape_row_is_identity`: 320-px source row with values 0..15 repeating → out[x] == pal[src[x]].
  - `test_portrait_game_frame_crops`: `frame(false, true, 160)` → `{CROP, 240, 320, 240, 200, 60, 48}`.
  - `test_portrait_text_frame_scales`: `frame(true, false, 160)` → `{SCALE, 240, 320, 240, 150, 85, 0}`.
  - `test_text_wins_over_redraw`: `frame(true, true, 160).mode == LAYOUT_SCALE`.
  - `test_idle_keeps_mode`: SCALE frame, then `frame(false, false, 160)` → SCALE; CROP frame, then idle → CROP.
  - `test_initial_mode_is_crop`: after `set_portrait(true)`, `frame(false, false, 0).mode == LAYOUT_CROP`.
  - `test_crop_clamps`: keenX −50 → cropX 0; keenX 400 → 80; keenX 112 → 0; keenX 200 → 80.
  - `test_crop_row_offset`: CROP cropX 48 → out[x] == pal[src[48 + x]] for all 240.
  - `test_scale_row_mapping`: SCALE, y = 3 → reads source row 4; out[x] == pal[src[4*stride + x*4/3]].
- [ ] **Step 2:** `pio test -e test -f test_screen_layout` → FAIL (missing header).
- [ ] **Step 3:** Implement.
- [ ] **Step 4:** `pio test -e test` → all PASS.
- [ ] **Step 5:** Commit `feat: screen_layout for landscape/portrait`.

### Task 2: render.c reports frame facts; new present signature

**Files:**
- Modify: `port/backend.h` (`port_present`), `port/render.c`, `port/render.h`, `port/game_main.c`, `test/test_render/test_main.c`, `test_support/fake_backend.c`, `port/host/host_backend.c` and `port/esp32/esp32_backend.cpp` (signature only — still landscape drawing, `l->top`/`l->outW` used)

**Interfaces:**
- Consumes: Task 1.
- Produces: `void port_present(const ScreenLayout_T *layout, const uint8_t *src, int stride, const uint16_t pal565[16]);` — `src` is the 320×200 game window as today. `render.c`: `drawChar`, `drawBitmap`, `showImageFile` set `textDrawn`; `doDrawing` sets `fullRedraw`; `render_present` computes keenX, calls `screen_layout_frame`, passes the layout, clears both flags. `port_game_run` calls `screen_layout_set_portrait(KEEN_PORTRAIT_ON)` (macro from `-DKEEN_PORTRAIT`, default 0) before `port_init()`.

- [ ] **Step 1: Failing tests** in `test/test_render` (capture the layout in the fake `port_present`; set portrait on in these tests, off in the existing ones):
  - `test_present_after_char_scales`: `drawChar(0, 0, 'A')`, `render_present()` → captured mode SCALE.
  - `test_present_after_redraw_crops`: set `fullRedraw` via a minimal `doDrawing`-free path — call the new `render_mark_full_redraw()` (test hook used by `doDrawing`), `render_present()` → CROP.
  - `test_flags_cleared_after_present`: after a SCALE present, a second `render_present()` with nothing drawn keeps SCALE (idle) — proves flags were consumed, not sticky-true (a following `render_mark_full_redraw()` + present → CROP).
  - Existing `test_present_window_max_fine` keeps passing with the new signature.
- [ ] **Step 2:** `pio test -e test -f test_render` → FAIL.
- [ ] **Step 3:** Implement (`render_mark_full_redraw` is called by `doDrawing`; declare in `render.h`). Update both backends to the new signature, still writing landscape at `layout->top`.
- [ ] **Step 4:** `pio test -e test` → all PASS; `pio run -e host` and `pio run -e cyd` → SUCCESS.
- [ ] **Step 5:** Commit `feat: render reports frame facts to screen_layout`.

### Task 3: Host portrait

**Files:**
- Modify: `port/host/host_backend.c`, `platformio.ini` (add `[env:host_portrait]`)

**Interfaces:**
- Consumes: `screen_layout_screen_w/h`, `screen_layout_row`.
- Produces: window `3 × screen_w` by `3 × screen_h` (portrait: 720×960 is too tall for many screens — use 2× in portrait: 480×640), texture and `screen[]` sized from `screen_layout_screen_w/h` (allocate 320×320 max), `KEEN_DUMP` PPM header from the same size; on mode change clear `screen[]`.

- [ ] **Step 1:** Implement; `pio run -e host` and `pio run -e host_portrait` → SUCCESS; `pio test -e test` → PASS.
- [ ] **Step 2:** `KEEN_AUTO=RJ KEEN_DUMP=... .pio/build/host_portrait/program` 10 s → dumped frame is 240×320 with the game window at rows 60..259 and Keen visible. Same with `host` → 320×240, picture at rows 20..219 (unchanged).
- [ ] **Step 3:** Forced text frame check: run `host_portrait` with `KEEN_TEXT_PROBE=1` (host-only debug aid: draws `"PORTRAIT"` with `CVort_engine_drawChar` once at boot after the first level frame) → one dump shows the SCALE layout (picture rows 85..234, bars black). Remove nothing; keep the aid documented in `CLAUDE.md` next to `KEEN_AUTO`.
- [ ] **Step 4:** Commit `feat: host portrait build`.

### Task 4: ESP32 portrait

**Files:**
- Modify: `port/esp32/esp32_backend.cpp`, `platformio.ini` (add `[env:cyd_portrait]`), `CLAUDE.md` (envs, debug aid)

**Interfaces:**
- Consumes: `screen_layout_*`.
- Produces: `port_init` sets `setRotation(screen_layout_portrait() ? 0 : 1)`; `port_present` pushes `outH` lines in strips of up to 20 lines (last strip may be shorter), each row `outW` wide via `screen_layout_row`; on mode change `dmaWait()` then `fillScreen(TFT_BLACK)`.

- [ ] **Step 1:** Implement; `pio run -e cyd` and `pio run -e cyd_portrait` → SUCCESS; `pio test -e test` → PASS.
- [ ] **Step 2:** Commit `feat: ESP32 portrait build`.
- [ ] **Step 3 (when the CYD is connected):** flash `cyd_portrait`, then `cyd`: picture right way up, crop follows Keen, fps ≈ 24 on serial, heap @boot ≥ 48 180 B. Record results in `CLAUDE.md`.
