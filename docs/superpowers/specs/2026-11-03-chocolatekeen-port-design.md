# Chocolate Keen → ESP32 CYD Port — Design

Date: 2026-11-03
Upstream: https://github.com/jamesfmackenzie/chocolatekeen (C + SDL2, GPL-2.0)

## Goal

A learning project: port Chocolate Keen to the ESP32-2432S028R ("CYD2USB") with the
MCP23017 controller board. The path matters as much as the result: each step should be
small, visible, and explain *why* it is needed.

**Version 1 is done when** Episode 1, Level 1 loads from flash, scrolls, and is playable
on the device: Keen moves, jumps, pogos and fires; enemies, death and level exit work.

**Out of scope for v1:** menus, world map, sound, save games, Episodes 2–3, status screen.

## Constraints

- ESP32-WROOM-32, 4 MB flash, **no PSRAM**, ~300 KB usable heap.
- Display ST7789 320×240 over SPI, inversion OFF, BGR order (see `CLAUDE.md`).
- Controller via MCP23017 @ 0x20 (SDA 22, SCL 27), active low:
  GPA0 Left, GPA1 Right, GPA2 Up, GPA3 Down, GPA4 Button 1, GPA5 Button 2.
- Upstream as-is does not fit: emulated EGA memory alone is 393 KB
  (`ENGINE_EGA_MEMORY_IN_BITS 0x60000`, 1 byte/pixel), and `gfx.c` decompresses tiles,
  sprites and bitmaps to 8 bpp heap buffers.
- Hardware is "CYD first": the design must allow switching to a PSRAM board (e.g.
  ESP32-S3) by replacing only the backend.

## Chosen approach

Keep upstream game logic; replace the rendering/platform layer at the existing seam.

The game code (`game/`, `episodes/`) talks to the engine through a small API:
`CVort_engine_drawTile`, `drawTileAt`, `drawSpriteAt`, `drawSprite`, `drawBitmap`,
`drawChar`, `drawScreen`, `syncDrawing`, `clearOverlay`, `delay`, `setCurSound`,
`updateInputStatus`, `setPaletteAndBorderColor`, and file helpers. We implement that API
against a plain 320×200 framebuffer instead of emulated EGA memory. `gfx.c`, SDL, the
launcher GUI and the input mapper are dropped.

Two backends share all drawing code:

- `esp32` — the device.
- `host` — macOS SDL2 window ("fake CYD") built from the same sources, for fast
  debugging and memory measurement.

Rejected alternatives:
- *Keep `gfx.c`, shrink EGA memory to 4 bpp:* still 100–200 KB plus deep SDL removal in a
  3.6k-line file; likely does not fit, hard to debug.

## Structure

```
keen-esp32/
├── platformio.ini        # envs: cyd (ESP32), host (native, SDL2)
├── upstream/             # Chocolate Keen game logic, copied in, minimal patches
├── port/
│   ├── backend.h         # the seam (engine API the game calls)
│   ├── backend_common.c  # framebuffer, tile/sprite/bitmap/char blits, clipping
│   ├── esp32/            # ST7789 SPI DMA, MCP23017 input, esp_timer ticks
│   └── host/             # SDL2 window + keyboard
├── assets/               # generated const C arrays (do not edit by hand)
├── tools/
│   └── convert_assets.py # KEEN1 data → assets/*.c (+ PNG previews)
└── hwtest/               # existing hardware test firmware
```

Rules:
- Every change inside `upstream/` is marked `// ESP32-PORT: <reason>`.
- `backend_common.c` contains no platform code; it builds identically for both envs.

## Assets

`tools/convert_assets.py` runs on the Mac and reads `GAMEDATA/KEEN1`:

- `EGAHEAD`/`EGALATCH`/`EGASPRIT`: decompress (LZ) and de-plane to **8 bpp, one byte per
  EGA color index**, emitted as `const uint8_t` arrays → flash, 0 bytes RAM. Sprites keep
  their mask as a separate plane/array. Expected total < 1 MB.
- `LEVEL01.CK1`: emitted raw; decompressed at runtime by the upstream RLEW code.
- `KEEN1.EXE`: unpacked (LZEXE, upstream `Cunlzexe` logic or Python port) and only the
  tables the game reads are emitted (palettes, trans map, random table, physics/points
  tables — the offsets used in `episodes/episode1_engine.c`).
- Writes PNG previews of tile and sprite sheets for visual checking.

A tiny read-only file shim (`fopen`/`fread`/`fseek` over flash arrays) lets upstream file
code load levels unchanged.

## Runtime and data flow

Startup: init display, MCP23017, tick timer → bind asset tables → load Level 1.

Per frame:
1. Game logic draws into the **framebuffer** (RAM, 320×200 × 8 bit = 64 KB). Each frame
   redraws the visible tiles and sprites, so no large scroll buffer is needed.
2. `syncDrawing()` pushes the frame in 20-line strips: palette lookup 8-bit → RGB565
   into one of two 12.8 KB DMA buffers while the other is being sent (ping-pong).
3. Output to ST7789 at y offset 20 (centered, 20 px bars top and bottom).

Expected throughput: ~26 ms per full frame at 40 MHz SPI (~35 fps ceiling), above the
game's own update cadence.

Timing: `esp_timer` provides the same tick rate upstream expects from the DOS PIT, so
`delay`/tick-based physics keep their meaning.

Input: one 2-byte MCP23017 read per frame (~0.3 ms) → game key state:

| Controller | Game (original key) |
|---|---|
| Joystick | Arrows |
| Button 1 | Jump (Ctrl) |
| Button 2 | Pogo (Alt) |
| Both buttons | Fire (Ctrl+Alt) |

The game runs in one FreeRTOS task with a large stack; WiFi/BT stay off.

## RAM budget (target)

| Item | KB |
|---|---|
| Framebuffer | 64 |
| DMA strips (2×) | 26 |
| Level map (decompressed) | ≤ 32 |
| Game state, objects, EXE tables | ~20 |
| Stack, FreeRTOS, misc | ~30 |
| **Total** | **~170 of ~300** |

If measurement shows this does not hold, the first fallback is a smaller framebuffer
(4 bpp, 32 KB); the second is the PSRAM board via a new backend.

## Error handling

- Fatal errors (missing asset, level decompress failure, allocation failure): red screen
  with a short message, same text on serial, halt.
- I2C read failure: treat as no input, log once, keep running.
- Print free heap and largest free block at boot and after level load.

## Testing

Each step is verified on host first, then on the device:

| Step | Host check | Device check |
|---|---|---|
| 1. Asset converter | PNG tile/sprite sheets look right | — |
| 2. Backend draws one tile + sprite | SDL window | same image on CYD |
| 3. Level 1 loads and scrolls (no Keen) | map correct | scroll speed OK, fps on serial |
| 4. Keen moves, jumps, pogos, fires | keyboard | joystick + buttons |
| 5. Enemies, death, exit | play through Level 1 | play through, heap stable |

Host unit tests (`pio test -e host`): palette → RGB565, blit clipping at screen edges,
flash file shim. Reuse fitting tests from upstream `tests/`.

## Licensing

Chocolate Keen is GPL-2.0; this port stays GPL-2.0. Episode 1 is shareware and its data
files ship with upstream.
