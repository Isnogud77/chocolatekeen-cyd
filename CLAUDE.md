## Hardware
- Board: ESP32-2432S028R, Variante "CYD2USB" (USB-C + Micro-USB)
- MCU: ESP32-WROOM-32, 4 MB Flash, kein PSRAM
- Display: 2.8" 320x240, ST7789, Inversion AUS, BGR-Reihenfolge (getestet), SPI
- Hintergrundbeleuchtung: GPIO 21 (falls dunkel: GPIO 27 prüfen)
- MCP23017 an CN1: SDA = GPIO 22, SCL = GPIO 27, Adresse 0x20
- Controller (MCP23017, active low, Pullups an): GPA0 = Links, GPA1 = Rechts, GPA2 = Hoch, GPA3 = Runter, GPA4 = Taste 1, GPA5 = Taste 2
- Lautsprecher: DAC an GPIO 26, angeschlossen. Verstärker sehr laut; leises "Flimmern" kommt von der Platine (auch mit reinem PWM), nicht aus dem Code
- SD-Karte: onboard- MCP23017 bekommt bei Hot-Plug Müll in die Register (gesehen: IPOLA = IOCON = 0x7A), ohne I2C-Fehler. Deshalb volle Register-Init alle 1 s (port/mcp23017.c).

## Chocolate-Keen-Port
- Spec: docs/superpowers/specs/2026-11-03-chocolatekeen-port-design.md, Plan: docs/superpowers/plans/2026-11-03-chocolatekeen-level1.md
- Upstream-Teilkopie in upstream/ (Änderungen mit `// ESP32-PORT:` markiert), eigener Code in port/, generierte Assets in assets/
- Assets neu erzeugen: `make -C tools/assetdump run`
- Tests (Mac): `pio test -e test`
- Mac-Version ("fake CYD", SDL2): `pio run -e host && .pio/build/host/program` (Pfeile, Z = Springen, X = Pogo, Esc = Ende)
  - Debug-Hilfen: `KEEN_AUTO=RJ` (Knöpfe per Skript), `KEEN_DUMP=frame%02d.ppm` (Bilder speichern), `KEEN_TEXT_PROBE=1` (erzwingt Text-Frames → skaliertes Hochformat)
- Hochformat (Galagino-Gehäuse): Envs `host_portrait` / `cyd_portrait` (`-DKEEN_PORTRAIT=1`); Spielbild 240x200 folgt Keen, Text-Frames 3:4 skaliert
- Gerät: `pio run -e cyd -t upload` (Hochformat: `-e cyd_portrait`), Serial 115200. Port wechselt je nach USB-Buchse (gesehen: /dev/cu.usbserial-120, -2140): dann `--upload-port <port>` angeben

## Messwerte Level 1 (2026-11-03, Sound 2026-11-04)
- fps 24.3 stabil, 10 min ohne Reset
- Heap nach Boot: 49 180 B frei (mit Sound), größter Block 29 684 B
- Statischer RAM 105 136 B, Flash 907 814 B von 1 310 720 B
- Hochformat geprüft 2026-11-04: aufrecht (Rotation 0), Fenster folgt Keen, 24.3 fps, Heap 49 156 B frei (Querformat gleich)
- Sound: DAC-Rechteck 0..16 (`kSpeakerAmplitude` in port/esp32/esp32_backend.cpp), Ton-Timer max. 5 kHz
