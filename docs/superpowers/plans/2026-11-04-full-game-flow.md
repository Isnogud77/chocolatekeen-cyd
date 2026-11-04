# Full Game Flow Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keen 1 plays start to end on the CYD (logo, title, trimmed menu, world map, 16 levels, ending) in landscape and portrait, with arcade controls.

**Architecture:** Upstream `CVort_main_loop` runs unchanged except for small `// ESP32-PORT:` patches (menu trimming, quit paths, play-context markers). A `play_context` flag drives both the input mapping (`input_map.c`) and the portrait rule (`screen_layout.c`). The renderer gains `showImageFile` (decode from flash), image mode, and `scrollText`. Input gains key events (`g_input.key_scane`), long-press pulses, Start/Menu pins and a letter picker.

**Tech Stack:** C99, Unity, PlatformIO (`cyd`, `cyd_portrait`, `host`, `host_portrait`, `test`).

**Spec:** `docs/superpowers/specs/2026-11-04-full-game-flow-design.md`

## Global Constraints

- Scancodes: Up 0x48, Down 0x50, Left 0x4B, Right 0x4D, Ctrl 0x1D, Alt 0x38, Enter 0x1C, Esc 0x01, Space 0x39 (= `CK_ACTION_STATUS`), Y 0x15, N 0x31.
- Key events: upstream reads characters through `g_input.key_scane` (`scancode | 0x80` = new press, translated with `exeFields.trans_map`). A press of a mapped key must set it once, on the press edge.
- Mapping (spec "Input" table). Play context: b1 → Ctrl, b2 → Alt. Menu context: b1 → held Enter+Y+Space, **event Y**; b2 → held Esc+N, **event Esc**. Start → Enter (held + event), Menu → Esc (held + event) in both contexts. Pulses: 100 ms; long press threshold 1000 ms; b2 long press → Esc pulse (play); Down held 1000 ms in a level (not on the map) → Space pulse.
- Button bits: `PORT_BTN_START = 1<<6` (MCP GPB0), `PORT_BTN_MENU = 1<<7` (GPB1).
- Quit: every path that would leave to DOS returns to the title (`g_game.quit_to_title = 1`) or is ignored; `CVort_chg_vid_and_error` with a message → `port_fatal(msg)`; with an empty message (plain quit) → title.
- Menu items: New Game, Continue Game (drawn greyed), Story, Help, High Scores.
- Portrait rule: text drawn → SCALE; not in play → SCALE; in play + full redraw → CROP; otherwise keep the previous mode.
- Flash assets: LEVEL01–16, LEVEL80, LEVEL81, LEVEL90, FINALE, HELPTEXT, STORYTXT, ENDTEXT, PREVIEWS (upstream main loop requires it), SCORES, CTLPANEL, SOUNDS (`.CK1`).
- Every upstream change carries `// ESP32-PORT: <reason>`.
- Device targets: fps ≈ 24 in levels both orientations; heap ≥ 40 KB free at boot and ≥ 30 KB at the main menu (text files are copied to RAM, ~8 KB).

## Review Focus

1. **Character prompts with simultaneous keys** (b1 holds Enter+Y+Space at once) → prompts must get exactly one `Y` event, menus still see Enter held. Task 2 test.
2. **Quit / error paths reaching `exit()`** on device → reboot or hang. Every `CVort_chg_vid_and_error` and DOS-quit call site must be covered. Task 6 grep checklist.
3. **Image mode never cleared** (title → New Game without a level load) → world map drawn without tiles. Task 4 test that `CVort_load_level_data` clears it; Task 7 host check of title → map.
4. **Text boxes erased by the full redraw** while the game waits (status screen, quit prompt, in-level messages) → invisible prompt, game seems frozen. Task 8 host checks each, fixes where needed.
5. **Heap after text files** (~8 KB mallocs at the main loop) → below 30 KB at the menu. Task 9 device check; heap line at menu.

---

### Task 1: All game files in flash

**Files:** Modify `tools/assetdump/assetdump.c` (`rawFiles`), regenerate `assets/keen1_files.c`; Modify `test/test_level_load/test_main.c`, `test/test_assets/test_main.c`.

- [ ] **Step 1: Failing tests:** `test_all_levels_load`: for n in 1..16, 80, 81, 90 → `checkLevel(n, "LEVELnn.CK1")` (oracle as for 14). `test_text_files_present`: FINALE, HELPTEXT, STORYTXT, ENDTEXT, PREVIEWS, SCORES, CTLPANEL found via `port_find_asset`, sizes equal `data/KEEN1` file sizes (8565, 1859, 3411, 902, 2112, 204, 44).
- [ ] **Step 2:** `pio test -e test -f test_level_load` → FAIL (LEVEL02 missing).
- [ ] **Step 3:** Add the files; `make -C tools/assetdump run`.
- [ ] **Step 4:** `pio test -e test` → PASS; `pio run -e cyd` → flash < 1 150 000 B.
- [ ] **Step 5:** Commit `feat: all Keen 1 game files in flash`.

### Task 2: Play context, key events, pulses, Start/Menu

**Files:** Create `port/play_context.h/.c`; Modify `port/input_map.c/.h`, `port/backend.h` (button bits), `test/test_input/test_main.c`.

**Interfaces:**
- Produces: `void play_context_set(bool inPlay, bool inLevel); bool play_context_in_play(void); bool play_context_in_level(void);`
- Produces: `void input_apply_buttons_at(uint8_t mask, uint32_t nowMs);` (pure; `CVort_engine_updateInputStatus` calls it with `port_read_buttons(), port_millis()`). Keeps `input_apply_buttons(mask)` as `input_apply_buttons_at(mask, 0)` for old tests.

- [ ] **Step 1: Failing tests** (each resets key_map, key_scane and the input state via a new `input_reset_state()`):
  - `test_play_b1_is_ctrl`: play on, b1 → key_map[0x1D] = 1, [0x1C] = 0.
  - `test_menu_b1_holds_enter_y_space`: play off, b1 → [0x1C], [0x15], [0x39] = 1.
  - `test_menu_b1_event_is_y`: play off, press b1 → `key_scane == (0x15 | 0x80)`; still held next call → key_scane unchanged (no second event after upstream clears it to 0x15).
  - `test_menu_b2_event_is_esc`: → `key_scane == 0x81`, key_map[0x01] and [0x31] = 1.
  - `test_long_press_b2_pulses_esc`: play on, b2 held from t=0; at t=999 key_map[0x01] = 0; at t=1000..1099 = 1; at t=1100 = 0; holding to t=3000 gives no second pulse.
  - `test_down_hold_in_level_pulses_space`: play+level on, Down held 1000 ms → Space pulse (100 ms), Down still held as 0x50.
  - `test_down_hold_on_map_no_status`: play on, level off → no Space.
  - `test_start_menu_buttons`: Start → Enter held + event 0x9C; Menu → Esc held + event 0x81, in both contexts.
  - Existing tests keep passing (they run with play on: `setUp` sets `play_context_set(true, true)`).
- [ ] **Step 2:** `pio test -e test -f test_input` → FAIL.
- [ ] **Step 3:** Implement; edge detection per button; pulses as timed state.
- [ ] **Step 4:** `pio test -e test` → PASS.
- [ ] **Step 5:** Commit `feat: context input mapping with key events and long presses`.

### Task 3: Letter picker for the high score name

**Files:** Create `port/letter_picker.h/.c`, `test/test_letter_picker/test_main.c`; Modify `port/input_map.c` (active picker overrides menu mapping), `upstream/game/worldmap.c` (start/stop picker around the name entry loop, ESP32-PORT).

**Interfaces:**
- Produces: `void letter_picker_start(void); void letter_picker_stop(void); bool letter_picker_active(void); uint8_t letter_picker_step(uint8_t pressedEdges);` — returns a scancode to emit as event (0 = none): Up/Down cycle the current letter A..Z, space (wrap) and emit Backspace (0x0E) + the letter's scancode so upstream's echo shows it; Right emits the current letter again as "commit, next" (cursor moves); b1 emits Enter (0x1C) and stops.

- [ ] **Step 1:** Read the name entry code in `upstream/game/worldmap.c` (around `ranknames`, line ~319–460) and note how it reads characters (expected: `CVort_read_char_with_echo` / `translate_key`). If it differs, adapt the event sequence and record a Ruling.
- [ ] **Step 2: Failing tests:** `test_up_cycles_letters` (A→B→…→Z→space→A), `test_down_wraps_backwards`, `test_right_commits_and_starts_at_a`, `test_b1_emits_enter_and_stops`, `test_scancodes_for_letters` (A = 0x1E, Z = 0x2C, space = 0x39).
- [ ] **Step 3:** `pio test -e test -f test_letter_picker` → FAIL; implement; → PASS.
- [ ] **Step 4:** Wire into `input_map` (picker active → only picker events) and the ESP32-PORT start/stop markers in `worldmap.c`; `pio test -e test` → PASS.
- [ ] **Step 5:** Commit `feat: arcade letter picker for high score names`.

### Task 4: showImageFile, image mode, scrollText

**Files:** Modify `port/render.c`, `port/render.h`, `upstream/game/gameplay.c` (`CVort_load_level_data` calls `render_image_mode_off()`, ESP32-PORT); Create `test/test_image/test_main.c`; Modify `test/test_render/test_main.c`.

**Interfaces:**
- Produces: `void CVort_engine_showImageFile(const char *filename)` (upstream signature): find asset; decompress with upstream's `do_image_file_decomp` from the flash data into `map_data` (planes 4 × 0x2000); de-plane 320×200 into the playfield at the screen origin (same mapping as `drawBitmap` at engine x 4, y 0 relative to `engine_dstPage`); set `scroll_x = scroll_y = 0`; image mode on; textDrawn. `void render_image_mode_off(void); bool render_image_mode(void);`. `void CVort_engine_scrollText(int16_t top, int16_t bottom, int16_t dir)`: same row ranges as upstream gfx.c:3372, applied to the playfield.

- [ ] **Step 1: Failing tests:**
  - `test_finale_pixels_match_oracle`: `showImageFile("FINALE.CK1")` → 50 sample pixels of the visible 320×200 window equal an independent decode in the test (RLE: upstream format, then 4 planes of 0x2000 bytes, bit 7 first).
  - `test_image_mode_skips_tiles`: after showImageFile, `CVort_engine_doDrawing()`-equivalent full redraw (call `render_redraw_for_test()` exposed in render.h) leaves sample pixels unchanged; a sprite draw still lands.
  - `test_level_load_clears_image_mode`: showImageFile, then `CVort_load_level_data(1)` → `render_image_mode() == false`.
  - `test_scroll_text_up_down`: fill rows with their row index, `scrollText(top, bottom, 0)` → row r holds old r+8 within the range; dir 1 the reverse.
- [ ] **Step 2:** run → FAIL; **Step 3:** implement; **Step 4:** `pio test -e test` → PASS.
- [ ] **Step 5:** Commit `feat: image screens, image mode and text scrolling`.

### Task 5: Play context markers and portrait rule

**Files:** Modify `upstream/game/gameplay.c` (`CVort_draw_level`: `play_context_set(true, true)` on entry, `(false, false)` on every return), `upstream/game/worldmap.c` (`CVort_draw_worldmap` loop: `(true, false)` while on the map, cleared when entering a level/leaving), `port/screen_layout.c/.h` (`screen_layout_frame(bool textDrawn, bool fullRedraw, bool inPlay, int keenX)`), `port/render.c` (passes `play_context_in_play()`), tests.

Rule (portrait): textDrawn → SCALE; else !inPlay → SCALE; else fullRedraw → CROP; else keep the previous mode (an open status box with idle presents must not flip back to CROP).

- [ ] **Step 1: Failing tests** in `test_screen_layout`: `test_play_redraw_crops`, `test_play_with_text_scales`, `test_no_play_scales_even_on_redraw`, `test_play_idle_keeps_scale_after_text` (text frame, then idle frames in play → SCALE until the next full redraw) — existing tests get `inPlay = true`; update `test_render` (set play context in its portrait tests).
- [ ] **Step 2:** FAIL; **Step 3:** implement; **Step 4:** `pio test -e test` → PASS.
- [ ] **Step 5:** Commit `feat: play context drives portrait layout`.

### Task 6: Front end trimming and quit paths

**Files:** Modify `upstream/game/menus.c`, `upstream/game/gameplay.c`, `upstream/game/ui.c` as needed, `port/game_main.c` (remove `port_level_loop`; `CVort_main` → `CVort_main_loop` again), `port/stubs.c` (`CVort_engine_handleQuit` → title), `port/host/host_backend.c` (`KEEN_AUTO` letters: `M` = menu script).

- [ ] **Step 1:** Patch `CVort_draw_menu` (5 items; Continue drawn with the greyed string style upstream uses for disabled text, or plain if none exists — Ruling) and the selection code in `CVort_do_intro_and_menu` so the cursor rows map to: 0 New Game, 1 Continue (no-op until part 3), 2 Story, 3 Help (`CVort_do_help`), 4 High Scores.
- [ ] **Step 2:** Quit checklist — `grep -n "chg_vid_and_error\|exit(" upstream/game upstream/episodes` and patch each: quit prompts → title or ignore; error messages → `port_fatal`. Record the list in the ledger.
- [ ] **Step 3:** `KEEN_AUTO=M` script: wait 3 s, press b1 (New Game) every 2 s for 10 s, then `R`+`J` autoplay. Host run 40 s with `KEEN_DUMP`: frames show logo/title, menu with 5 items, world map, a level. Check by eye.
- [ ] **Step 4:** `pio test -e test` → PASS; all four envs build.
- [ ] **Step 5:** Commit `feat: arcade front end on upstream main loop`.

### Task 7: Host flow checks (title → map, story, help, scores)

- [ ] **Step 1:** Extend `KEEN_AUTO` with `S` (open Story: down×2, b1, page down ×3, b2) and `H` (Help), `C` (High Scores). Dump frames for each in `host` and `host_portrait`.
- [ ] **Step 2:** Check by eye: text viewer scrolls, portrait scales all of them, world map after New Game has tiles (image mode cleared).
- [ ] **Step 3:** Fix what fails (systematic-debugging), each with a test where the cause is in port code. Commit `fix: ...` per fix.

### Task 8: Text boxes over the game

- [ ] **Step 1:** Host scripted checks in a level: status screen (Down 1 s), in-game menu (b2 1 s) incl. the quit prompt, and the level-entry message box. Dump frames while each box waits.
- [ ] **Step 2:** For each box that disappears: find which redraw erases it, fix in `render.c` (e.g. skip the tile redraw while upstream is blocked in `read_char_with_echo` / `wait_for_key`, signalled by `g_isWaitingForCharInput`), add a test reproducing it. Commit `fix: keep text boxes visible while waiting`.

### Task 9: ESP32 Start/Menu pins, device check, docs

**Files:** Modify `port/esp32/esp32_backend.cpp` (read GPIOA+GPIOB in one 2-byte read; GPB0 → bit 6, GPB1 → bit 7), `port/mcp23017.c` (GPPUB already 0xFF — verify in test), `port_game_run` heap line at the main menu (`port_report_heap("menu")` via ESP32-PORT marker in `CVort_do_intro_and_menu` first draw); Create `docs/controls.md`; Modify `CLAUDE.md`.

- [ ] **Step 1:** Implement; `pio run` all envs → SUCCESS; `pio test -e test` → PASS.
- [ ] **Step 2: Device (user):** `cyd_portrait` and `cyd`: full flow, status screen, in-game menu, death, finished level, Story; serial: fps ≈ 24 in levels, heap @boot ≥ 40 KB, @menu ≥ 30 KB.
- [ ] **Step 3:** Write `docs/controls.md` for other cabinet builders: pin map incl. GPB0/GPB1, context table, long presses, letter picker, orientation envs, how to add buttons (`port/backend.h` bits, `port/input_map.c`, `port/esp32/esp32_backend.cpp` read).
- [ ] **Step 4:** Commit `feat: Start/Menu pins, controls docs`.
