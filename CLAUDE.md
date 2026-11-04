## Hardware
- Board: ESP32-2432S028R, "CYD2USB" variant (USB-C + Micro-USB)
- MCU: ESP32-WROOM-32, 4 MB flash, no PSRAM
- Display: 2.8" 320x240, ST7789, inversion OFF, BGR order (tested), SPI
- Backlight: GPIO 21 (if the screen stays dark: check GPIO 27)
- MCP23017 on CN1: SDA = GPIO 22, SCL = GPIO 27, address 0x20
- Controller (MCP23017, active low, pullups on): GPA0 = Left, GPA1 = Right, GPA2 = Up, GPA3 = Down, GPA4 = Button 1, GPA5 = Button 2, optional GPB0 = Start, GPB1 = Menu
- A controller hot-plug can write garbage into the MCP23017 registers (seen: IPOLA = IOCON = 0x7A) without any I2C error, so the full register setup is re-applied every 1 s (port/mcp23017.c)
- Speaker: DAC on GPIO 26, connected. The amplifier is very loud; the faint background "flicker" noise comes from the board (also with plain PWM), not from the code
- SD card: onboard (not used yet)

## Chocolate Keen port
- Specs in docs/superpowers/specs/, plans in docs/superpowers/plans/, open follow-ups in docs/superpowers/followups-full-game-flow.md
- Vendored upstream subset in upstream/ (local changes marked `// ESP32-PORT:`), own code in port/, generated assets in assets/
- Regenerate assets: `make -C tools/assetdump run`
- Tests (Mac): `pio test -e test`
- Mac build ("fake CYD", SDL2): `pio run -e host && .pio/build/host/program` (arrows, Z = button 1, X = button 2, Esc = quit)
  - Debug aids: `KEEN_AUTO` (scripted buttons, see port/host/host_backend.c), `KEEN_DUMP=frame%02d.ppm` (save frames), `KEEN_TEXT_PROBE=1` (forces text frames → scaled portrait layout)
- Portrait (Galagino-style upright cabinet): envs `host_portrait` / `cyd_portrait` (`-DKEEN_PORTRAIT=1`); gameplay is a 240x200 window following Keen, text frames are scaled 3:4
- Device: `pio run -e cyd -t upload` (portrait: `-e cyd_portrait`), serial 115200. The port name depends on the USB socket (seen: /dev/cu.usbserial-120, -2140): pass `--upload-port <port>` then
- Controls, pin map, saving and how to add buttons: docs/controls.md
- Saves/scores: port/save_store.c over port_store_* (ESP32 NVS namespace `keen`; host `~/.keen-esp32/`). Autosave slot `SAVED1.CK1`

## Measurements (device, 2026-11-04)
- fps ≈ 24 in levels, both orientations
- Free heap: 48 212 B after boot, 36 508 B at the main menu, 39 076 B in levels (flat across many level loads)
- Static RAM 106 104 B, flash 1 002 442 B of 1 310 720 B
- Sound: DAC square wave 0..16 (`kSpeakerAmplitude` in port/esp32/esp32_backend.cpp), tone timer capped at 5 kHz
