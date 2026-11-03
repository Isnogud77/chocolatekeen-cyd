# PC Speaker Sound Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keen 1 sound effects as an authentic PC-speaker square wave on the CYD speaker (GPIO 26), and the same sounds on the host build.

**Architecture:** A platform-neutral player (`port/pcspeaker.c`) walks the tone lists of `SOUNDS.CK1` straight from flash, one divisor per 6.87 ms tick, applying upstream's priority rules. `port/audio_glue.c` maps upstream's sound API onto it. The ESP32 backend ticks it from a 145.6 Hz `esp_timer` and drives LEDC PWM; the host backend ticks it from the SDL audio callback.

**Tech Stack:** C99, Arduino-ESP32 core 3.x (`ledcAttach`, `ledcWriteTone`), `esp_timer`, SDL2 audio (host), Unity.

**Spec:** `docs/superpowers/specs/2026-11-04-pc-speaker-sound-design.md`

## Global Constraints

- `SOUNDS.CK1`: magic `SND\0`, count uint16 LE at offset 8 = 60; entry *i* (1-based) at `16*i`: tone-list offset uint16 LE at +0, priority uint8 at +2.
- Tone list: uint16 LE divisors, one per 1/145.6 s (6 868 µs), ended by `0xFFFF`; 0 = silence; frequency = 1 193 182 / divisor.
- Upstream rules: sound numbers are 1-based; start only if nothing plays or priority ≥ `sound_limiter`, then `sound_limiter` = priority; nothing starts while `g_game.sound_disabled`; `want_sound == 0` → silent output, still advancing; `finishCurSound` waits via `CVort_engine_shortSleep` until playback ends.
- The port sets `g_game.want_sound = 1` at boot.
- Speaker pin GPIO 26, 50 % duty square wave.
- `port/pcspeaker.c` and `port/audio_glue.c` contain no platform code.
- Done: four sounds (jump, shot/click, death, exit) audible on device; fps ~24; free heap drop ≤ 1 KB.

## Review Focus

1. **`finishCurSound` hangs** when the player cannot advance (init failed, paused by `saveCurSound`, timer not running): it must return. Task 3 tests init-failed and paused.
2. **Corrupt or truncated sound data** (offset past file end, missing `0xFFFF`) → the player must never read past `len`. Task 2 test with a hand-made truncated buffer.
3. **Extreme divisors** (1 → 1.19 MHz, 0xFFFE → 18 Hz) → LEDC frequency clamped to a range it accepts. Task 2 tests `pcspeaker_divisor_to_hz`.
4. **Race between `play()` on the game task and `tick()` on the timer** → all player entry points take the backend lock; verified by review, not by test.
5. **Sound requested while muted** → still advances and ends, so a later sound is not blocked by priority forever. Task 2 test.

---

### Task 1: SOUNDS.CK1 into flash

**Files:**
- Modify: `tools/assetdump/assetdump.c` (`rawFiles`), regenerate `assets/keen1_files.c`
- Modify: `test/test_assets/test_main.c`

**Interfaces:**
- Produces: `keen1_files` entry `"SOUNDS.CK1"` (8 898 bytes), reachable through `port_fopen_asset`.

- [ ] **Step 1:** Add `test_sounds_present`: entry `"SOUNDS.CK1"` exists, `size == 8898`, first 4 bytes `S N D \0`, uint16 LE at offset 8 `== 60`.
- [ ] **Step 2:** `pio test -e test -f test_assets` → FAIL (`test_sounds_present`).
- [ ] **Step 3:** Add `"SOUNDS.CK1"` to `rawFiles`; `make -C tools/assetdump run`.
- [ ] **Step 4:** `pio test -e test -f test_assets` → PASS.
- [ ] **Step 5:** Commit `feat: SOUNDS.CK1 in flash assets`.

### Task 2: PC speaker player

**Files:**
- Create: `port/pcspeaker.h`, `port/pcspeaker.c`, `test/test_pcspeaker/test_main.c`
- Modify: `port/backend.h` (add lock), `test_support/fake_backend.c` (weak no-op lock)

**Interfaces:**
- Produces (`backend.h`): `void port_audio_lock(void); void port_audio_unlock(void);`
- Produces (`pcspeaker.h`):
  ```c
  #define PCSPK_PIT_HZ 1193182u
  #define PCSPK_TICK_US 6868u
  bool     pcspeaker_init(const uint8_t *data, uint32_t len); // false: bad magic/size
  int      pcspeaker_count(void);
  bool     pcspeaker_play(uint16_t sound);   // 1-based; true if started (priority rule)
  uint16_t pcspeaker_tick(void);             // divisor for this step, 0 = silent (takes the lock)
  uint16_t pcspeaker_tick_locked(void);      // same, caller already holds the lock (audio callbacks)
  bool     pcspeaker_playing(void);
  void     pcspeaker_set_muted(bool muted);
  void     pcspeaker_set_paused(bool paused); // paused: tick returns 0, no advance
  uint8_t  pcspeaker_limiter(void);          // current priority limiter
  uint32_t pcspeaker_divisor_to_hz(uint16_t divisor); // 0 -> 0; clamp to [20, 20000]
  ```
  Every function except `divisor_to_hz` runs under `port_audio_lock`. Reading a tone word past `len` ends the sound.

- [ ] **Step 1: Failing tests** `test/test_pcspeaker` (uses `keen1_files` `"SOUNDS.CK1"`):
  - `test_count_60`: init true, `pcspeaker_count() == 60`.
  - `test_bad_magic_rejected`: init on a 32-byte zero buffer → false; `play(1)` → false.
  - `test_plays_tone_list_in_order`: `play(1)` (WLDWALKSND, list at offset 1024) → successive `tick()` equal the file's words from 1024 up to (not incl.) `0xFFFF`; next `tick() == 0`; `playing() == false`.
  - `test_lower_priority_does_not_interrupt`: `play(3)` (prio 255) then `play(1)` (prio 128) → second returns false, `limiter() == 255`.
  - `test_equal_priority_interrupts`: `play(3)` twice → second returns true.
  - `test_muted_advances_silently`: muted, `play(1)` → all ticks return 0 and `playing()` turns false after list length + 1 ticks.
  - `test_paused_holds_position`: `play(1)`, pause, 5 ticks return 0, unpause → next tick is the list's first word.
  - `test_out_of_range_ignored`: `play(0)` and `play(61)` → false.
  - `test_truncated_data_stops`: copy of the file cut to 1030 bytes (entry 1 list starts at 1024, no terminator) → `play(1)`, at most 3 nonzero ticks, then 0 and not playing.
  - `test_divisor_to_hz`: `0 → 0`, `1193 → 1000`, `1 → 20000`, `0xFFFE → 20`.
- [ ] **Step 2:** `pio test -e test -f test_pcspeaker` → FAIL (missing header).
- [ ] **Step 3:** Implement. The player keeps its own limiter (`pcspeaker_limiter()`); copying it into upstream's `sound_limiter` is Task 3's job.
- [ ] **Step 4:** `pio test -e test` → all PASS.
- [ ] **Step 5:** Commit `feat: platform-neutral PC speaker player`.

### Task 3: Upstream sound API

**Files:**
- Create: `port/audio_glue.c`, `test/test_audio_glue/test_main.c`
- Modify: `port/stubs.c` (remove `loadSounds`, `setCurSound`, `finishCurSound`, `saveCurSound`, `restoreCurSound`), `port/game_main.c` (`g_game.want_sound = 1` before `CVort_main()`)

**Interfaces:**
- Consumes: Task 2 player, `port_fopen_asset`-backed `CVort_engine_cross_ro_data_fopen`.
- Produces upstream signatures: `void CVort_engine_loadSounds(void)` (init player from `"SOUNDS.CK1"` data pointer found via `keen1_files`, log one line on failure), `void CVort_engine_setCurSound(uint16_t sound)` (skip if `g_game.sound_disabled`; `pcspeaker_set_muted(!(g_game.want_sound & 0xFF))`; on start set `sound_limiter = pcspeaker_limiter()`), `void CVort_engine_finishCurSound(void)` (loop `CVort_engine_shortSleep()` while playing **and not paused and initialized**), `saveCurSound` → pause, `restoreCurSound` → unpause.

- [ ] **Step 1: Failing tests** `test/test_audio_glue`:
  - `test_set_sound_starts_player`: `loadSounds()`, `want_sound = 1`, `setCurSound(1)` → `pcspeaker_playing()`, `sound_limiter == 128`.
  - `test_sound_disabled_blocks`: `sound_disabled = 1`, `setCurSound(1)` → not playing.
  - `test_want_sound_off_mutes`: `want_sound = 0`, `setCurSound(1)` → playing, `tick() == 0`.
  - `test_finish_returns_when_paused`: playing, `saveCurSound()`, `finishCurSound()` returns (test would hang otherwise; Unity run must finish).
  - `test_finish_returns_when_not_initialized`: player init with bad buffer, `finishCurSound()` returns.
- [ ] **Step 2:** `pio test -e test -f test_audio_glue` → FAIL (STUB versions do nothing).
- [ ] **Step 3:** Implement; remove the five stubs; set `want_sound` in `port_game_run`.
- [ ] **Step 4:** `pio test -e test` → all PASS; `pio run -e host` and `pio run -e cyd` → SUCCESS.
- [ ] **Step 5:** Commit `feat: upstream sound API on PC speaker player`.

### Task 4: Host audio

**Files:**
- Modify: `port/host/host_backend.c`

**Interfaces:**
- Consumes: `pcspeaker_tick`, `pcspeaker_divisor_to_hz`.
- Produces: `port_audio_lock/unlock` = `SDL_LockAudioDevice/SDL_UnlockAudioDevice`; SDL audio device 44 100 Hz mono S16, callback: every 44100 × 6868 / 1e6 ≈ 303 samples call `pcspeaker_tick()`, write a ±3000 square wave at `divisor_to_hz` (0 → silence). The callback uses `pcspeaker_tick_locked` (SDL already holds the device lock inside the callback).

- [ ] **Step 1:** Implement.
- [ ] **Step 2:** `pio test -e test` → all PASS.
- [ ] **Step 3:** `KEEN_AUTO=RJ .pio/build/host/program` 20 s → no crash, no `STUB` lines for sound functions in stderr. User listens on the Mac: jump sounds audible.
- [ ] **Step 4:** Commit `feat: host audio via SDL`.

### Task 5: ESP32 audio

**Files:**
- Modify: `port/esp32/esp32_backend.cpp`

**Interfaces:**
- Consumes: `pcspeaker_tick_locked`, `pcspeaker_divisor_to_hz`.
- Produces: `port_audio_lock/unlock` = `portENTER_CRITICAL/portEXIT_CRITICAL` on a `portMUX_TYPE`; in `port_init`: `ledcAttach(26, 1000, 8)`, `ledcWriteTone(26, 0)`, periodic `esp_timer` with `PCSPK_TICK_US`. Timer callback: lock → `tick_locked` → unlock, then `ledcWriteTone(26, hz)` only when `hz` changed (no LEDC call inside the critical section).

- [ ] **Step 1:** Implement; `pio run -e cyd` → SUCCESS.
- [ ] **Step 2:** Flash; read serial 30 s → fps line ≈ 24, heap @boot free ≥ 48 600 (≤ 1 KB below 49 644).
- [ ] **Step 3:** User checks on device: jump, shot (or empty click), death, exit sounds.
- [ ] **Step 4:** Update `CLAUDE.md` measured values (sound line, heap); commit `feat: PC speaker sound on device`.
