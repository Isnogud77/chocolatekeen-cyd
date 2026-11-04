# Full Game Flow — Design

Date: 2026-11-04
Part 2 of 3 of the full-game work: 1. display layer (done) → **2. full game
flow** → 3. saving.
Builds on: level 1, sound and display orientation milestones (all merged).

## Goal

Keen 1 plays start to end in the cabinet, in landscape and in portrait:
Apogee logo, title, menu, world map, all 16 levels, the ending. The front end
is trimmed for an arcade cabinet.

**Done when:**

- Power-on → logo → title → menu. The menu has New Game, Continue (greyed out),
  Story, Help, High Scores.
- New Game → world map → any level → back to the map; dying and finishing work.
  The ending (FINALE image + end text) returns to the title.
- Story and Help open in the text viewer and page with the joystick.
- In a level: holding down for 1 s shows the status screen; holding button 2
  for 1 s opens the in-game menu; "quit" returns to the title.
- High score name entry works with the letter picker (scores kept in RAM).
- Portrait: menus, title, text screens and the ending are scaled 3:4; levels
  and the world map use the 240×200 window following Keen.
- On the device in both orientations: fps ≈ 24 in levels, heap after boot
  ≥ 40 KB free, no reset during a session of several levels.
- `docs/controls.md` exists for other cabinet builders.

**Out of scope:** saving games, writing high scores and settings to storage
(part 3); demos; DOS text mode output (boot messages and the ANSI end screen
go to serial only); Ordering Info, Previews, About ID, Quit to DOS.

## Approach

Run upstream's front end (`CVort_main_loop`: logo, title, menus, world map,
levels, ending) with small `// ESP32-PORT:` patches, instead of writing an own
front end. The level 1 shortcut (`port_level_loop`) is removed.

## Front end trimming (patches in `upstream/game/menus.c`)

- `CVort_draw_menu` shows only: New Game, Continue Game (greyed out until part
  3), Story, Help, High Scores. The selection logic skips hidden entries.
  "Help" opens upstream's help text viewer (`CVort_do_help`).
- Every path that would quit to DOS returns to the title (`quit_to_title`)
  instead.
- DOS text mode (`setVideoMode(TEXT)`, `copyToTxtMemory`, `puts`) shows
  nothing on the display; `puts` keeps logging to serial.

## Data in flash

Added to `keen1_files`: `LEVEL01`–`LEVEL16`, `LEVEL80`, `LEVEL81`, `LEVEL90`,
`FINALE`, `HELPTEXT`, `STORYTXT`, `ENDTEXT`, `SCORES`, `CTLPANEL` (`.CK1`).
About 90 KB more; flash stays under 1.1 MB of 1.3 MB. `PREVIEW*`, `ORDER.FRM`
and `VENDOR.DOC` are not added.

Text files are read in place from flash where upstream would `malloc` a copy;
if upstream modifies a text buffer while processing it, it is copied into RAM
once instead (the plan decides per file after reading the code).

## Input

The controller still produces DOS scancodes; the mapping depends on the
**play context** (a level or the world map loop is running):

| | Play (level/map) | Menus, text, prompts |
|---|---|---|
| Joystick | arrows | arrows (select, page) |
| Button 1 | Ctrl (jump) | Enter + Y + Space |
| Button 2 | Alt (pogo) | Esc + N |
| Both buttons | fire (Ctrl+Alt) | — |
| Button 2 held 1 s | Esc pulse (in-game menu) | — |
| Down held 1 s (level only) | status-screen key pulse | — |
| Start (GPB0), Menu (GPB1) | Enter / Esc | Enter / Esc |

- A pulse is the key pressed for 100 ms, so the game sees exactly one press.
- Pressing button 2 still toggles pogo before the 1 s hold completes;
  accepted, since Esc pauses the game.
- The status-screen key is whatever upstream binds to `CK_ACTION_STATUS`
  (the plan reads it from `actions.c`).
- Start and Menu buttons are optional hardware on free MCP23017 pins;
  `PORT_BTN_START` and `PORT_BTN_MENU` are read and mapped from day one.

**Letter picker** for the high score name: while upstream waits for typed
characters, joystick up/down cycles A–Z and space, right moves to the next
letter, button 1 finishes. It emits the scancodes upstream expects.

## Renderer additions

- **`showImageFile`:** decompress the image straight from its flash asset into
  `map_data` (upstream's RLE format, 4 planes × 0x2000 bytes), then de-plane it
  into the playfield. Upstream instead copies the packed file to
  `map_data + 0x4000`, which would overflow the port's 40 KB `map_data` for
  FINALE (8 565 bytes).
- **Image mode:** set by `showImageFile`. While set, the per-frame full redraw
  skips map tiles but still draws sprites, tile draws, bitmaps and
  `draw_func`. Cleared when `CVort_load_level_data` loads a level or the map.
- **`scrollText`:** moves pixel rows in the playfield like upstream moves them
  in EGA memory (text viewer).
- **Text boxes drawn once** (status screen, in-game menu, messages) must stay
  visible while the game waits. Where a full redraw would erase one, the plan
  fixes that case (for example by drawing the box again after the redraw).
- **Portrait rule (refines part 1):** CROP only while the play context is set
  and no text was drawn; otherwise SCALE. The play context comes from
  `// ESP32-PORT:` markers at the start and end of the level and world map
  loops, and is the same flag the input mapping uses.

## Memory

Levels and the map decompress into the existing 40 KB `map_data` (the
largest, LEVEL13, needs 39 456 B). Target: heap after boot ≥ 40 KB free.

## Testing

Unit tests (`pio test -e test`):

- Input context: play button 1 → Ctrl; menu button 1 → Enter + Y + Space;
  button 2 held 1 s → exactly one Esc pulse; down held 1 s in a level → one
  status pulse; Start/Menu → Enter/Esc.
- Letter picker: up/down cycle with wrap, right advances, button 1 ends, emitted
  scancodes.
- `showImageFile`: FINALE in the playfield matches an independent decoder at
  sample pixels.
- Image mode: a full redraw keeps the image and still draws sprites.
- `scrollText`: rows move by one text line up and down.
- Portrait rule: play + no text → CROP; anything else → SCALE.
- All 16 levels, LEVEL80, LEVEL81 and LEVEL90 load correctly (oracle decode,
  as for level 14).

Host: `KEEN_AUTO` gains a menu script; frame dumps of logo, title, menu,
world map and a level are checked by eye.

Device (user): the whole flow in both orientations, status screen, in-game
menu, death, a finished level, fps and heap on serial.

## Documentation

`docs/controls.md`, written for other cabinet builders: pin map including
Start/Menu, the context table, long presses, the letter picker, the
orientation build envs, and how to add more buttons or keys
(`port/input_map.c`, `port/mcp23017.c`).
