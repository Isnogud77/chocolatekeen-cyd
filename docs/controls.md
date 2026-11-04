# Controls and wiring (for cabinet builders)

The game reads one MCP23017 I2C port expander. All inputs are active low
(button to GND) and use the chip's internal pullups, so no external resistors.

## Wiring

| Signal | Where |
|---|---|
| I2C address | 0x20 (A0-A2 to GND) |
| SDA | ESP32 GPIO 22 (CYD connector CN1) |
| SCL | ESP32 GPIO 27 (CYD connector CN1) |
| Power | 3.3 V and GND from the CYD |

## Pin map

| MCP pin | Function | PORT_BTN bit |
|---|---|---|
| GPA0 | Left | 0 |
| GPA1 | Right | 1 |
| GPA2 | Up | 2 |
| GPA3 | Down | 3 |
| GPA4 | Button 1 (jump / confirm) | 4 |
| GPA5 | Button 2 (pogo / cancel) | 5 |
| GPB0 | Start (optional) | 6 |
| GPB1 | Menu (optional) | 7 |

Start and Menu are optional; leave GPB0/GPB1 unconnected (pullup) if you do
not build them. The minimum cabinet is a joystick and two buttons.

## What the controls do

The meaning depends on the context: "play" is a level or the world map;
everything else (menus, text, prompts) is "menu".

| | Play | Menus, text, prompts |
|---|---|---|
| Joystick | arrows | arrows (select, page) |
| Button 1 | Ctrl (jump) | Enter + Y + Space held; sends Enter as a key press (prompts accept it as yes) |
| Button 2 | Alt (pogo) | Esc + N |
| Both buttons | fire (Ctrl+Alt) | none |
| Button 2 held 1 s | Esc pulse (in-game menu) | none |
| Down held 1 s (level only) | status-screen key pulse | none |
| Start (GPB0), Menu (GPB1) | Enter / Esc | Enter / Esc |

A pulse is the key pressed for 100 ms, so the game sees exactly one press.
Button 2 still toggles pogo before the 1 s hold completes.

High-score name entry uses a letter picker: joystick up/down cycles A-Z and
space, right moves to the next letter, button 1 finishes.

## Build for your cabinet

| Env | Orientation |
|---|---|
| `cyd` | landscape |
| `cyd_portrait` | upright cabinet (Galagino style); play view 240x200 follows Keen, text screens scaled 3:4 |

```
pio run -e cyd_portrait -t upload      # or -e cyd
```

Serial monitor is 115200 baud (fps and free heap are logged). Without a
cabinet, `pio run -e host` / `host_portrait` runs the game on a Mac or PC (SDL2).

## Saving

- The game **autosaves** every time Keen is (back) on the world map with
  lives left — after finishing a level and after dying. There is one save
  slot and no save dialog.
- **Continue Game** in the main menu loads it and puts Keen back on the map.
  Without a save, Continue does nothing.
- **Game over** deletes the save, so a lost game cannot be continued.
- **High scores** (names entered with the letter picker) are kept across power
  cycles.
- Storage: the ESP32's internal NVS flash (namespace `keen`), no SD card
  needed. To wipe saves and scores, erase the flash and flash again:
  `pio run -e cyd -t erase` (or `cyd_portrait`), then upload.

## Hot-plug

A hot-plugged controller cable can scribble into the MCP23017 registers
without any I2C error. The firmware therefore rewrites the whole register
setup (`mcp_init_seq` in `port/mcp23017.c`) every second, so a wrongly
configured chip recovers by itself within about a second. If I2C fails, the
game sees no input and logs it once.

## Adding buttons or keys

Upstream reads DOS scancodes, so a new button must end up as a scancode.

1. `port/backend.h`: add a `PORT_BTN_*` bit (the mask is a `uint8_t`; bits 0-7 are taken, so widening the mask is needed for more).
2. `port/mcp23017.c`: map the GPIO bit into the mask in `mcp_buttons_from_gpio` (and extend `test/test_mcp`). Pullups for all pins are already set in `mcp_init_seq`.
3. `port/esp32/esp32_backend.cpp`: `port_read_buttons()` reads GPIOA+GPIOB in one 2-byte read and calls the mapping; change it only if you need other registers.
4. `port/input_map.c`: turn the new bit into a scancode in `input_apply_buttons_at` (context-dependent, with `key_map[...]` for held keys and `key_event(...)` for single presses). Scancodes are listed at the top of the file.
