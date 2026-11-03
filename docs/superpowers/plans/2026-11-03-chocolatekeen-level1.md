# Chocolate Keen Level 1 on CYD — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Episode 1 Level 1 of Chocolate Keen runs playable on the ESP32 CYD with the MCP23017 controller, built from upstream game logic plus a new backend.

**Architecture:** Upstream game logic (`game/`, `episodes/episode1*`, `core/`, timing, RLEW/LZ decoders) is vendored nearly unchanged. Upstream `gfx.c` (emulated EGA memory) is replaced by `port/render.c`, which implements the same `CVort_engine_*` draw functions against a 336×224 8-bit playfield buffer and redraws it fully each frame. A thin backend (`port/esp32/` or `port/host/`) presents the 320×200 window, reads buttons, and gives time. Assets are pre-converted on the Mac into `const` arrays in flash.

**Tech Stack:** C99 (upstream + port), C++ only inside `port/esp32/`, PlatformIO (`espressif32` Arduino framework, `native` platform), TFT_eSPI 2.5.43, SDL2 (host only, Homebrew), Unity (`pio test`).

**Spec:** `docs/superpowers/specs/2026-11-03-chocolatekeen-port-design.md`

## Global Constraints

- Board ESP32-WROOM-32, 4 MB flash, no PSRAM. WiFi/BT off.
- Display ST7789 320×240, inversion OFF, BGR (`-DTFT_INVERSION_OFF=1 -DTFT_RGB_ORDER=TFT_BGR`, other TFT flags as in `hwtest/platformio.ini`). Game image at y offset 20.
- MCP23017 @ 0x20, SDA 22, SCL 27, active low, pullups on: GPA0 Left, GPA1 Right, GPA2 Up, GPA3 Down, GPA4 Button 1 (jump), GPA5 Button 2 (pogo). Both buttons = fire.
- Every edit inside `upstream/` carries a `// ESP32-PORT: <reason>` comment.
- `port/render.c` and `port/file_shim.c` contain no platform code; they build identically for `host` and `cyd`.
- Fatal error: red screen + message + same text on serial, then halt.
- Print free heap and largest free block at boot and after level load.
- License GPL-2.0 (copy upstream `gpl-2.0.txt` to repo root as `LICENSE`).

## Deviations from spec (decided while reading upstream)

1. Asset converter is a small **C** tool reusing upstream decoders (`lz.c`, `Cunlzexe.c`, the `decompGraphics` logic), not Python — same output, no re-implemented decompression.
2. Playfield buffer is **336×224 = 75 KB**, not 320×200. The game draws a 21×14 tile grid and scrolls by 0–15 px inside it; the visible 320×200 window is cut out at present time.
3. The whole unpacked `KEEN1.EXE` image goes to flash; upstream reads ~30 tables from it by offset. It is only *written* on big-endian hosts, so read-only is fine.
4. `map_data` shrinks from 64 KB to 40 KB (`0xA000` bytes); the largest Keen 1 level unpacks to 39 456 bytes.
5. Spec testing steps 3 and 4 merge: upstream's `CVort_draw_level` runs scroll, Keen and enemies together.

## Review Focus

1. **Write into flash-resident EXE image** → device crashes with `LoadStoreError`. On host the array is in read-only `__const`, so the same bug segfaults there: Task 5 runs level 1 on host for 60 s as the check.
2. **Sprites partly outside the 336×224 playfield** (Keen at map edges, enemies entering) → must be clipped, never write outside the buffer. Task 4 tests with canary bytes.
3. **Diagonal joystick and both buttons at once** → Left+Up must set both arrow keys; Jump+Pogo must set Ctrl and Alt (upstream turns that into fire). Task 5 unit test.
4. **Controller read fails mid-game** (cable loose) → treat as no buttons, log once, game keeps running. Task 6 manual check.
5. **Slow frames on device** → upstream caps `sprite_sync` at 15 ticks; if present time pushes frames above that, physics drift. Task 6 logs fps; target ≥ 20 fps.

---

### Task 1: Project skeleton and vendored upstream

**Files:**
- Create: `platformio.ini` (repo root), `LICENSE`, `upstream/` (copied tree), `data/KEEN1/` (copied game data), `test/test_sanity/test_main.c`
- Modify: `.gitignore` (add `assets/*.ppm`)

**Interfaces:**
- Produces `platformio.ini` with `[platformio] src_dir = .` and three envs:
  - `cyd`: esp32dev, arduino, `build_src_filter = -<*> +<upstream/> +<port/> +<assets/> -<port/host/>`.
  - `host`: `platform = native`, SDL2 via `!sdl2-config --cflags --libs`, `build_src_filter = -<*> +<upstream/> +<port/> +<assets/> -<port/esp32/>`.
  - `test`: `platform = native`, `test_build_src = yes`, `build_src_filter = -<*>` at first; **each later task adds exactly the sources its tests need** (Task 2: `+<assets/>`; Task 3: `+<port/file_shim.c>`; Task 4: `+<port/render.c> +<upstream/render/memdata.c>`; Task 5: `+<port/input_map.c>`). Tests supply their own fake `port_*` functions. Never add `port/host/` or `port/game_main.c` (they own `main`/the game loop).
  - All three add `-Iupstream -Iupstream/core -Iport -Iport/sdl_shim -Iassets` and disable episodes 2 and 3 (use the real macro names from `upstream/chocolate-keen_config.h`).
  - All `pio test` commands below use `-e test`, not `-e host`.

- [ ] **Step 1:** `brew install sdl2`; verify `sdl2-config --version` prints 2.x.
- [ ] **Step 2:** Copy from upstream commit (record its hash in `upstream/UPSTREAM.md`): `src/core`, `src/game`, `src/episodes/episode1*`, `src/episodes/episode_macros.h`, `src/engine/engine_timing.*`, `src/engine/engine_io.*`, `src/engine/engine_loader.c`, `src/render/memdata.c`, `src/render/gfx.h`, `src/decompression`, `src/third_party/cgenius`, `src/input/actions.*`, `src/chocolate-keen_config.h`. Do **not** copy `render/gfx.c`, `input/input.c`, `ui/`, `platform/`, `render/opengl.h`. Copy `data/GAMEDATA/KEEN1` → `data/KEEN1`.
- [ ] **Step 3:** Write `test/test_sanity/test_main.c` with Unity `test_true` asserting `1 == 1`; run `pio test -e test -f test_sanity` → PASS. (`test` env has no sources yet, so only Unity is built.)
- [ ] **Step 4:** Commit `chore: vendor chocolatekeen subset and add pio envs`.

### Task 2: Asset dumper

**Files:**
- Create: `tools/assetdump/assetdump.c`, `tools/assetdump/Makefile`, generated `assets/keen1_assets.h`, `assets/keen1_gfx.c`, `assets/keen1_exe.c`, `assets/keen1_files.c`, `test/test_assets/test_main.c`

**Interfaces:**
- Consumes: upstream `third_party/cgenius/fileio/lz.c`, `compression/Cunlzexe.c`, logic of upstream `gfx.c:47-334` (`CVort_engine_decompGraphics`) and `engine_loader.c:172-220` (`load_exe_image`).
- Produces (`assets/keen1_assets.h`):
  ```c
  extern const EGAHeadGeneralSection_T keen1_egaHead;      // from render/gfx.h
  extern const EGAHeadUnmasked_T     keen1_bmpHeads[];      // bmpNum
  extern const MaskedSpriteEntry_T   keen1_spriteEntries[]; // 4*spriteNum
  extern const uint8_t keen1_tileData[];    // tileNum*256, 1 byte = EGA index 0..15
  extern const uint8_t keen1_fontData[];    // fontNum*64
  extern const uint8_t keen1_bmpData[];     // concatenated, offsets in keen1_bmpOffsets
  extern const uint32_t keen1_bmpOffsets[];
  extern const uint8_t keen1_spriteData[];  // bit4 = mask, bits0-3 = color (upstream format)
  extern const uint32_t keen1_spriteOffsets[];
  extern const uint8_t keen1_exeImage[];  extern const uint32_t keen1_exeImageSize;
  typedef struct { const char *name; const uint8_t *data; uint32_t size; } AssetFile_T;
  extern const AssetFile_T keen1_files[]; extern const int keen1_filesCount; // LEVEL01.CK1
  ```
  Pixel layout must be byte-identical to what upstream `gfx.c` puts in `engine_egaTiles`/`engine_egaSprites`/`engine_egaBmps`/`engine_egaFonts`, so upstream blit code works unchanged.
- Also writes `assets/tiles.ppm` (16 tiles per row) and `assets/sprites.ppm` with the default EGA palette for eyeballing.

- [ ] **Step 1: Write failing test** `test/test_assets/test_main.c` (host, links only `assets/*.c`):
  - `test_header_counts`: `keen1_egaHead.tileNum == 611`, `.bmpNum == 18`, `.spriteNum == 119`, `.fontNum == 256`.
  - `test_tile_pixels_are_ega_indices`: every byte of `keen1_tileData[0 .. 611*256)` is `< 16`.
  - `test_sprite_pixels_in_range`: every byte of `keen1_spriteData` up to the last offset+size is `< 32`.
  - `test_level01_present`: `keen1_files` has `"LEVEL01.CK1"` with `size == 1368` and its first 4 bytes LE == `10112`.
  - `test_level_fits_map_data`: `10112 <= 0xA000`.
  - `test_exe_unpacked`: `keen1_exeImageSize > 0x16200` (highest offset upstream reads is `0x1615E`+).
- [ ] **Step 2:** Run `pio test -e test -f test_assets` → FAIL (missing symbols).
- [ ] **Step 3:** Implement `assetdump.c` (`int main(int argc, char **argv)`: argv[1] = data dir, argv[2] = output dir). Reuse upstream decode code by `#include`-ing / linking `lz.c` and `Cunlzexe.c`; port the `decompGraphics` body with globals made local. Emit arrays as hex with 16 bytes per line. `make -C tools/assetdump && ./tools/assetdump/assetdump data/KEEN1 assets`.
- [ ] **Step 4:** Run `pio test -e test -f test_assets` → PASS. Open `assets/tiles.ppm` (`open -a Preview`) and check it looks like Keen 1 tiles (Mars ground, blocks). Ask the user to confirm the picture.
- [ ] **Step 5:** Commit generated `.c/.h` (they are build inputs) and the tool: `feat: asset dumper and generated Keen 1 assets`.

### Task 3: SDL shim, backend seam, file shim — everything links

**Files:**
- Create: `port/sdl_shim/SDL.h`, `port/sdl_shim/SDL_opengl.h`, `port/backend.h`, `port/file_shim.c`, `port/stubs.c`, `port/render.c` (empty functions for now), `port/host/host_backend.c` (stub bodies), `port/esp32/esp32_backend.cpp` (stub bodies), `test/test_file_shim/test_main.c`
- Modify: `upstream/render/memdata.c:216` and `upstream/core/globals.h:570` (`map_data[0xA000 / 2]`, ESP32-PORT), `upstream/engine/engine_io.c` (route `CVort_engine_cross_ro_data_fopen` to `port_fopen_asset`, ESP32-PORT)

**Interfaces:**
- Produces `port/backend.h`:
  ```c
  enum { PORT_BTN_LEFT=1<<0, PORT_BTN_RIGHT=1<<1, PORT_BTN_UP=1<<2,
         PORT_BTN_DOWN=1<<3, PORT_BTN_JUMP=1<<4, PORT_BTN_POGO=1<<5 }; // = MCP GPA bit order
  void     port_init(void);                 // display, input, timer
  uint32_t port_millis(void);
  void     port_sleep_ms(uint32_t ms);
  uint8_t  port_read_buttons(void);         // PORT_BTN_* mask, 1 = pressed
  void     port_present(const uint8_t *src, int stride, const uint16_t pal565[16]); // 320x200 window
  void     port_log(const char *fmt, ...);
  void     port_report_heap(const char *where);
  _Noreturn void port_fatal(const char *msg);
  ```
  `port/file_shim.c`: `FILE *port_fopen_asset(const char *name)` — case-insensitive lookup in `keen1_files`, returns `fmemopen((void*)data, size, "rb")`, or `NULL`.
- `SDL.h` shim: `Uint8..Uint64/Sint*` typedefs, `SDL_BYTEORDER SDL_LIL_ENDIAN`, `SDL_VERSION_ATLEAST(a,b,c) 1`, opaque `typedef struct SDL_Window SDL_Window;` etc. for every SDL type named in `core/types.h`/`globals.h`, `SDL_Rect`, `#define SDL_GetTicks port_millis`, `#define SDL_Delay port_sleep_ms`, `SDL_Swap16/SwapLE16/SwapLE32` as identity/byte swap. Add more only as the compiler demands.
- `port/stubs.c`: one function per remaining undefined symbol, body = `port_log("STUB %s", __func__)` once (static flag) and a neutral return (0/false). Keep the list alphabetical.

- [ ] **Step 1: Failing test** `test/test_file_shim`: `test_open_level01` (non-NULL, `fseek` end → `ftell == 1368`), `test_open_lowercase` (`"level01.ck1"` works), `test_missing` (`"LEVEL99.CK1"` → NULL).
- [ ] **Step 2:** `pio test -e test -f test_file_shim` → FAIL.
- [ ] **Step 3:** Implement `port_fopen_asset`; test → PASS.
- [ ] **Step 4:** Build loop: `pio run -e host`; for each compile error adjust the shim, for each undefined symbol add a stub or (if it is a `CVort_engine_draw*`/`syncDrawing`/`drawScreen`/`doDrawing`/palette function) an empty body in `port/render.c`. Repeat until it links. Then `pio run -e cyd` until it links.
- [ ] **Step 5:** `pio run -e cyd -t size` → record DRAM `.bss`+`.data` in commit message; must be < 160 KB (framebuffer is heap-allocated later, not static).
- [ ] **Step 6:** Commit `feat: upstream links on host and cyd via SDL shim and stubs`.

### Task 4: Playfield renderer

**Files:**
- Modify: `port/render.c`
- Create: `port/render.h`, `test/test_render/test_main.c`

**Interfaces:**
- Consumes: asset arrays (Task 2), `port_present` (Task 3), upstream globals `scroll_x`, `scroll_y`, `ATR`, `spritedraws`, `tiledraws`, `bmpdraws`, `anim_plane`, `map_data_tiles`, `draw_func`.
- Produces `port/render.h`:
  ```c
  #define PF_W 336
  #define PF_H 224
  #define PF_ORIGIN 32          // upstream EGA buffer coord of playfield (0,0), both axes
  extern uint8_t *pf;           // heap, PF_W*PF_H
  bool     render_init(void);   // malloc pf, bind engine_ega* pointer tables to asset arrays
  uint16_t ega_to_rgb565(uint8_t egaSignal);  // 6-bit EGA signal -> RGB565
  void     render_present(void);              // window at (fine_x, fine_y) -> port_present
  ```
  plus upstream's own signatures from `gfx.c`: `CVort_engine_drawTile/drawSprite/drawBitmap/drawChar(uint16_t x, uint16_t y, uint16_t num)`, `drawSpriteAt`, `drawTileAt`, `clearOverlay`, `syncDrawing`, `drawScreen`, `doDrawing`, `setPaletteAndBorderColor(const uint8_t*)`, `setBorderColor`, `updateActualDisplay`.

Rules:
- Upstream x/y arguments are EGA buffer coords (`x<<3` bytes, scanline 384). Map to playfield: `pf_x = bufX - PF_ORIGIN`, `pf_y = bufY - PF_ORIGIN`. Clip every blit to `[0,PF_W)×[0,PF_H)`.
- Copy `drawSpriteAt`, `drawTileAt`, `clearOverlay`, `syncDrawing` from upstream `gfx.c:3068-3227` verbatim. Keep the blend formulas of `drawSprite`/`drawTile` (masked) verbatim.
- `doDrawing`: compute `anim_plane_i` as upstream, then **draw all 21×14 visible map tiles** (replaces `adaptiveTileRefresh`/ATR/page flip), then sprites, then tile draws, then bitmaps, then `draw_func`, then `CVort_engine_delay(1)`.
- Fine scroll: `fine_x = (scroll_x >> 8) & 15`, `fine_y = (scroll_y >> 8) & 15`; visible window = `pf + fine_y*PF_W + fine_x`.
- Palette: `setPaletteAndBorderColor` stores 16 entries (`convertEGASignalToEGAEntry` logic from `gfx.c:3418`), rebuilds `pal565[16]`.

- [ ] **Step 1: Failing tests** `test/test_render` (host, real `render.c`, fake `port_present` that captures its args):
  - `test_rgb565_white`: `ega_to_rgb565(0x3F) == 0xFFFF`; `test_rgb565_black`: `(0x00) == 0x0000`; `test_rgb565_red`: `(0x04) == 0xA800`; `test_rgb565_brown`: `(0x14) == 0xAAA0` (EGA brown #AA5500).
  - `test_tile_at_origin`: set `anim_plane[anim_plane_i][n] = n`, call `CVort_engine_drawTile(4, 32, n)` (unmasked path; high bit clear) → `pf[0..15][0..15]` equals tile `n` pixels.
  - `test_masked_tile_keeps_background`: `CVort_engine_drawTile(4, 32, 0x8000|m)` with a tile whose mask byte is `0xFF` → background unchanged at that pixel.
  - `test_sprite_clipped_right_bottom`: draw a 4-byte-wide sprite with its left at `pf_x = 330`, top at `pf_y = 220`; canary 64 bytes after `pf` unchanged; column 335 written, nothing wraps to column 0 of next row.
  - `test_sprite_clipped_left_top`: sprite at buffer `x_byte = 0, y = 0` (pf −32,−32) writes only rows/cols ≥ 0.
  - `test_sprite_mask_keeps_background`: pixel value `16` over background `7` stays `7`.
  - `test_present_window_max_fine`: `scroll_x = scroll_y = 0x0F00` → `port_present` gets `src == pf + 15*PF_W + 15`, `stride == PF_W`.
- [ ] **Step 2:** `pio test -e test -f test_render` → FAIL.
- [ ] **Step 3:** Implement; tests → PASS.
- [ ] **Step 4:** Commit `feat: playfield renderer replacing EGA memory emulation`.

### Task 5: Host backend, input mapping, level 1 runs on Mac

**Files:**
- Modify: `port/host/host_backend.c`
- Create: `port/game_main.c`, `port/input_map.c`, `port/input_map.h`, `port/host/main.c`, `test/test_input/test_main.c`

**Interfaces:**
- Consumes: everything above; upstream `CVort1_engine_processEXE()`, `CVort_engine_prepareEpisodeSpecificFuncPointers()`, `CVort_draw_level(uint16_t)`, globals `exeImage`, `game_ext`, `engine_gameVersion`, `g_input`.
- Produces:
  - `void input_apply_buttons(uint8_t mask)` (`input_map.c`): writes `g_input.key_map[...]` for scancodes Up `0x48`, Right `0x4D`, Down `0x50`, Left `0x4B`, Ctrl `0x1D` (jump), Alt `0x38` (pogo). `void input_init_scancodes(void)` sets `g_input.sc_dir[8] = {0x48,0x49,0x4D,0x51,0x50,0x4F,0x4B,0x47}`, `sc_but1 = 0x1D`, `sc_but2 = 0x38`. `CVort_engine_updateInputStatus()` (moved out of stubs) = `input_apply_buttons(port_read_buttons())`.
  - `void port_game_run(void)` (`game_main.c`): `render_init()` (fatal on false) → set `exeImage = (uint8_t*)keen1_exeImage`, `engine_gameVersion = GAMEVER_KEEN1`, `game_ext = "CK1"` → `CVort_engine_prepareEpisodeSpecificFuncPointers()` → `CVort1_engine_processEXE()` → `input_init_scancodes()` → `port_report_heap("boot")` → loop forever: `CVort_draw_level(1)`, `port_report_heap("level end")`. Report heap once after level load via a hook in `level_init_screen` (ESP32-PORT) or right after first `drawScreen`.
  - Host backend: SDL2 window 320×240 ×3, game image at y 20, arrow keys + `Z` jump + `X` pogo → `port_read_buttons`; `port_millis` = `SDL_GetTicks`; Esc quits.

- [ ] **Step 1: Failing test** `test/test_input`: `test_diagonal_left_up` (`LEFT|UP` → key_map[0x4B] and [0x48] = 1, others 0), `test_both_buttons` (`JUMP|POGO` → [0x1D] and [0x38] = 1), `test_release_clears` (apply `LEFT`, then `0` → [0x4B] == 0).
- [ ] **Step 2:** `pio test -e test -f test_input` → FAIL; implement; → PASS.
- [ ] **Step 3:** `pio run -e host && .pio/build/host/program`. Expected: Mars level 1 visible, Keen moves/jumps/pogos/fires, scroll smooth. Play 60 s (Review Focus 1: no segfault). Look at the `STUB` log lines: any stub called every frame that matters for gameplay gets a real body now.
- [ ] **Step 4:** Commit `feat: level 1 playable on host`.

### Task 6: ESP32 backend

**Files:**
- Modify: `port/esp32/esp32_backend.cpp`
- Create: `port/esp32/main.cpp`

**Interfaces:**
- Consumes: `backend.h` contract, `port_game_run()`.
- Produces: real `cyd` implementations:
  - Display via TFT_eSPI (flags from Global Constraints), `setRotation(1)`, SPI 40 MHz. `port_present`: for each of 10 strips of 20 lines convert indices → RGB565 (byte-swapped for SPI) into one of two 12 800-byte buffers from `heap_caps_malloc(..., MALLOC_CAP_DMA)`, `pushImageDMA(0, 20 + strip*20, 320, 20, buf)`, alternate buffers, `dmaWait()` before reuse.
  - Input: Wire 100 kHz→400 kHz on SDA 22/SCL 27; init IODIR/GPPU as in `hwtest/src/main.cpp`; `port_read_buttons` = `~GPIOA & 0x3F`; on I2C error return 0 and `port_log` once.
  - `port_millis` = `esp_timer_get_time()/1000`; `port_sleep_ms` = `vTaskDelay`/`delay`.
  - `port_report_heap`: `heap_caps_get_free_size(MALLOC_CAP_8BIT)` and `heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)` to serial.
  - `port_fatal`: `fillScreen(TFT_RED)`, white text, serial, `while(1) delay(1000)`.
  - `main.cpp`: `setup()` starts `port_game_run` in a task with 16 KB stack on core 1; `loop()` empty. Log fps (presents per 5 s) from `port_present`.
- [ ] **Step 1:** `pio run -e cyd -t upload`, then read serial 30 s (pyserial script as used for `hwtest`). Expected: heap lines "boot" and after level load, free heap > 40 KB, fps line ≥ 20.
- [ ] **Step 2:** User checks the screen: level 1 visible with correct colors; joystick and buttons control Keen; both buttons fire.
- [ ] **Step 3:** Review Focus 4: user unplugs the controller for a few seconds mid-game → game keeps running, one I2C log line, controls work again after replug.
- [ ] **Step 4:** Commit `feat: ESP32 backend, level 1 on device`.

### Task 7: Play-through and notes

**Files:**
- Modify: `CLAUDE.md` (add build/run commands and measured numbers), spec (mark done items)

- [ ] **Step 1:** User plays Level 1 on device to the exit and also dies once. Expected: level restarts each time, no reset/crash; serial heap value after each restart is the same ±1 KB (no leak).
- [ ] **Step 2:** Leave it running 10 minutes; serial shows no reset (`rst:` line) and stable heap.
- [ ] **Step 3:** Write measured fps, free heap, flash use (`pio run -e cyd -t size`) into `CLAUDE.md`; commit `docs: level 1 milestone results`.
