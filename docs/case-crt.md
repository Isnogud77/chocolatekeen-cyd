# CRT all-in-one case

A 90s beige all-in-one computer (CRT plus keyboard) for the same hardware as
the Galagino cabinet: CYD, control board, speaker, USB-C socket and slide
switch. The keyboard holds the control board. Its keys are decorative; only
the joystick and the two square keys are real.

![front](../cad/print-crt/front.png) ![iso](../cad/print-crt/iso.png)

- Design file: [cad/keen-crt-landscape.sc](../cad/keen-crt-landscape.sc) (SolidCraft, every size is a named parameter)
- Print files: [cad/print-crt/](../cad/print-crt/)
- Firmware: landscape, `pio run -e cyd -t upload`
- Size: 104 × 117 × 114 mm (W × D × H)

## Parts to print

PLA, 0.2 mm layers, no supports.

| File | Colour | Print orientation |
|---|---|---|
| SideLeft, SideRight | beige | outside face down (rabbets face up) |
| Bottom | beige | outside face down |
| Front | beige | inside face down |
| Deck | beige | outside (top) face down |
| Bezel | beige | front face down |
| Top | beige | outside face down; the front rounding gives a short overhang on the first few layers |
| Back | beige | outside face down |
| KeyField | dark | plate down |
| KeyCap1, KeyCap2 | dark | flange down |
| JoyCap | dark | top down |
| FloppyInsert | dark | back face down |
| Badge | dark | plate down, text up |

## Hardware

| Part | Qty |
|---|---|
| M3 self-tapping screw, 6 mm (CYD to Bezel) | 4 |
| M2 self-tapping screw, 6 mm (control board to Deck) | 4 |
| M2 self-tapping screw, 8 mm (back hatch) | 2 |
| M2 self-tapping screw, 4 mm (slide switch to Back, from inside) | 2 |
| 3 mm LED + 220 Ω resistor, between 3.3 V and GND on CN1 (lights whenever the unit is on, no firmware change) | 1 |
| Rubber foot, 10 mm diameter, max 1 mm recess | 4 |
| 35 mm speaker, USB-C panel socket, slide switch | as in the Galagino build |
| Glue for PLA (CA or plastic cement) | |

## Assembly

1. **Keyboard.** Put KeyCap1 and KeyCap2 into the square holes of the Deck from below. The flange stays under the deck.
   Screw the control board to the four standoffs under the Deck (M2), pressing the switch shafts into the caps.
   Press the JoyCap onto the 5-way switch and glue the KeyField into its pocket.
   The control board cannot be removed after the next step, so solder a 4-pin plug
   (3.3 V, GND, SDA, SCL) on its cable first.
2. **Shell.** Glue SideLeft, Bottom, Front, Deck, Bezel and Top together, then SideRight.
   Every panel sits in a 1 mm step (rabbet) along the inside edge of each side,
   which squares the case. Tape it while the glue sets. Leave the Back off.
3. **Front details.** Press the FloppyInsert and the Badge into the chin. Push the LED into its hole from behind and glue it.
4. **CYD.** Screw the CYD to the four standoffs behind the screen window (M3) through the back opening.
   The hole pattern sits 2.85 mm right of the window centre; if the display is off-centre the
   other way on your board, flip `cyd_off` in the design and re-export the Bezel.
5. **Back hatch.** Glue the speaker into the ring, fit the USB-C socket and screw the slide switch on.
   Wire everything up.
6. **Close.** Slide the hatch's bottom tongue forward under the lip on the Bottom and lay the hatch into
   its rabbets. Screw it to the two tabs under the Top (M2). To open it, undo the two screws and pull the hatch
   straight back.

Controls and pin map: [controls.md](controls.md).
