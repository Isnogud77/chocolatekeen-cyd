## Hardware
- Board: ESP32-2432S028R, Variante "CYD2USB" (USB-C + Micro-USB)
- MCU: ESP32-WROOM-32, 4 MB Flash, kein PSRAM
- Display: 2.8" 320x240, ST7789, Inversion AUS, BGR-Reihenfolge (getestet), SPI
- Hintergrundbeleuchtung: GPIO 21 (falls dunkel: GPIO 27 prüfen)
- MCP23017 an CN1: SDA = GPIO 22, SCL = GPIO 27, Adresse 0x20
- Controller (MCP23017, active low, Pullups an): GPA0 = Links, GPA1 = Rechts, GPA2 = Hoch, GPA3 = Runter, GPA4 = Taste 1, GPA5 = Taste 2
- Lautsprecher: DAC an GPIO 26 (aktuell keiner angeschlossen)
- SD-Karte: onboard