# Saving — Design

Date: 2026-11-05
Part 3 of 3 of the full-game work: 1. display layer (done) → 2. full game flow
(done) → **3. saving**.

## Goal

Arcade-style persistence on the cabinet: the game autosaves, "Continue Game"
resumes, and high scores survive power loss. No SD card needed.

**Done when:**

- Returning to the world map (after finishing a level or dying with lives
  left) autosaves to one slot without any dialog.
- "Continue Game" in the menu loads that slot and puts Keen on the map with
  the same lives, ammo, items and finished levels; with no save it does
  nothing (as today).
- Game over deletes the autosave.
- High scores (with names from the letter picker) survive a power cycle.
- Works on the device after pulling the power, and on the host after a
  restart.
- `docs/controls.md` has a short "Saving" section.

**Out of scope:** manual save slots, the save dialog (F5), control settings
(`CTLPANEL`), demos, SD card storage.

## Storage

ESP32 NVS (namespace `keen`), wear-levelled and atomic per entry; no
partition change. Host: files in `~/.keen-esp32/`. Data is small (a save game
is a few hundred bytes; the score table well under 1 KB).

## Approach

Keep upstream's serialization; replace only where bytes go. Upstream reads and
writes saves and scores through `CVort_engine_cross_rw_misc_fopen(name, mode)`
(`upstream/engine/engine_io.c`, today it returns NULL). The port implements it
as a "file on a blob":

- `"rb"`: load the blob into a buffer, return `fmemopen` over it (NULL if the
  blob does not exist).
- `"wb"`: return a write-only FILE whose close callback commits the bytes to
  the store (`fopencookie` on the ESP32/newlib, `funopen` on macOS). Nothing
  reaches the store before `fclose`.

Rejected: an own save struct written straight to NVS (duplicates upstream's
save format and can miss fields).

## Structure

```
port/save_store.c/.h   platform-neutral: rw_misc_fopen over the backend
backend.h              + port_store_load / port_store_save / port_store_erase
port/esp32/            NVS implementation (Preferences or nvs_* API)
port/host/             files in ~/.keen-esp32/
test_support/          + RAM fake store for unit tests
```

Backend interface:

- `int port_store_load(const char *name, uint8_t *buf, int max)` → bytes read,
  or −1 if missing.
- `bool port_store_save(const char *name, const uint8_t *buf, int len)`
- `void port_store_erase(const char *name)`

Names are the upstream file names (e.g. `SAVEGAM1.CK1`, `SCORES.CK1`); NVS keys
are limited to 15 characters, which these fit.

## Upstream patches (`// ESP32-PORT: <reason>`)

- **Autosave:** when the world map loop is re-entered after a level returns
  with lives left, call upstream's save-game write for slot 1 directly (no
  dialog, no prompt).
- **Continue:** "Continue Game" loads slot 1 directly instead of asking for a
  slot; with no save it stays a no-op.
- **Game over:** erase slot 1.
- `CTLPANEL` writes and demo record/playback stay off (still NULL).

The plan reads the upstream save/load/continue code and pins the exact call
sites.

## Errors

- Write fails (store full or broken): log once, the game continues; only that
  save is lost.
- Load finds a corrupt or truncated save (upstream's read reports failure):
  treat as no save, erase the entry, show the menu normally.
- Power loss while saving: the store keeps either the old or the new entry.

## Memory

The write buffer (at most 2 KB) exists only while a file is open for writing
and is freed on close. Target unchanged: heap ≥ 30 KB at the menu.

## Testing

Unit tests (`pio test -e test`, RAM fake store):

- write → close → read returns the same bytes;
- nothing is stored before close;
- reading a missing name → NULL;
- corrupt save → no save, entry erased;
- Continue without a save → no-op;
- game over → autosave erased;
- scores survive a "reboot" (store kept, game state reset).

Host: finish a level, quit, restart, Continue → back on the map with the same
state.

Device (user): finish a level, pull the power, Continue → same state; a high
score survives a reboot.
