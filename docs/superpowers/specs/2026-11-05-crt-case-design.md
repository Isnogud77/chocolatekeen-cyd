# CRT all-in-one case (design)

A second printable cabinet for the same hardware as `cad/keen-cabinet-landscape.sc`,
styled as a 90s beige all-in-one computer (Mac Classic / C64 spirit): one continuous
shell with a keyboard wedge in front and a CRT face behind it. Still a bartop
(sloped control deck in front of a tilted screen), but reads "PC", not "arcade".
The keyboard is only the control-board housing; its keys are decorative.

## Decisions

| Topic | Decision |
|---|---|
| Monitor + keyboard | one fused body (no cable) |
| Shape | all-in-one: keyboard and monitor share one shell, no swivel foot |
| Orientation | landscape (firmware env `cyd`) |
| Construction | flat panels between two profile sides, glued |
| Joints | 1 mm rabbets (open steps) in the sides; panels butt-glued to each other on design lines (pegs and half-laps dropped, see Joints) |
| Screws | components only (CYD 4x M3, control PCB 4x M2) + 2x M2 on the back hatch |
| Printer | single colour: beige shell, dark detail parts printed separately and glued/pressed in |
| Extras | fake floppy slot, 3 mm power LED, badge recess, vent slots |

## Hardware (unchanged from the landscape cabinet)

- CYD: PCB 86 x 50, M3 hole pattern 78 x 42, pattern centre 2.85 mm off the window
  centre (`cyd_off`, verify side), display module thickness 4 mm (`cyd_so`)
- Control PCB: 50 x 22 mm (gerber in `cad/parts/gerber_cyd-arcade-control-panel_*`),
  5-way switch + two 6x6 tact switches, M2 standoffs at x = +-22.5, y = 8.5 / 6.5
  relative to the board, standoff 5.5 mm so switch tops sit 1.5 mm above the deck surface
- Speaker 35 mm, USB-C panel socket (cutout 11 x 6), slide switch 19.6 x 5.5 flange,
  2x 2.2 mm screw holes 15 mm apart (as in `cad/parts/*.avif`)
- New: 3 mm LED + series resistor between 3V3 and GND from CN1 (always on when powered,
  no firmware change)

## Shape and main parameters

All values are named user parameters in the SolidCraft file. Starting values:

| Name | Value | Meaning |
|---|---|---|
| `W` | 104 | outer width |
| `wall` | 2.4 | panel thickness (deck, bezel, top, back, bottom, front) |
| `side_t` | 3 | side panel thickness (holds the 1 mm rabbets) |
| `H_front` | 16 | keyboard front height |
| `deck_d` | 48 | keyboard depth (horizontal) |
| `deck_ang` | 8 deg | keyboard slope |
| `step_h` | 3 | step from keyboard to monitor face |
| `chin_h` | 14 | face height below the screen bezel (badge, floppy, LED) |
| `face_len` | 90 | face length along its slope (chin + bezel) |
| `face_ang` | 10 deg | face tilt back from vertical |
| `top_d` | 30 | top depth behind the face top (the back leans, so the bottom is ~24 mm deeper) |
| `back_ang` | 12 deg | back panel lean forward (tube taper) |
| `r_top` | 10 | rounding of the top-front edge |
| `win_w`, `win_h`, `win_r`, `bezel` | 60, 47, 4, 2.3 | screen window and 45 deg bevel (as before) |

Expected envelope about 104 W x 115 H x 120 D mm (height 16 + 48 tan 8 + 3 + 90 cos 10 = 114; depth at the floor 48 + 90 sin 10 + 30 + 114 tan 12 = 118).

Layout of the face (bottom to top): chin with badge recess (left), floppy opening
(right) and LED hole next to it; then the screen window centred in a ~12 mm bezel.
The control PCB sits centred near the front edge of the deck; the fake key field
surrounds it (rows behind, partial rows beside).

## Parts

Shell (beige):

| Part | Features | Fixing |
|---|---|---|
| SideLeft / SideRight | full profile, 1 mm deep rabbets (`wall + 0.3` wide, open to the outside) for every panel, vent slots top rear | glued |
| Bottom | rubber-foot recesses, lip for the hatch hook | glued |
| Front | keyboard lip | glued |
| Deck | flat top (KeyField glued on top), joystick + 2 square cap holes, 4x M2 standoffs below | glued (with PCB already screwed on) |
| Bezel | chin + screen face, window + bevel, badge recess, floppy opening, LED hole 3.1, 4x M3 CYD standoffs; lower edge forms the step | glued |
| Top | vent slots, rounded front edge, 2 screw tabs for the hatch | glued |
| Back | speaker grille + speaker ring, USB-C cutout, slide switch slot + 2 pilot holes, hook on the lower edge | 2x M2 into the Top tabs |

Detail parts (dark):

| Part | Features |
|---|---|
| KeyField | plate with raised fake keys (1.5 mm proud, 0.4 mm gaps), cutouts for the real controls; glued on top of the Deck, 4 mm in from its edges |
| KeyCap x2 | square 9 mm cap, 3.1 mm press-fit socket, retaining flange under the deck |
| JoyCap | low square cap on the 5-way stem |
| FloppyInsert | slot face with eject bump, 0.2 mm press fit into the Bezel |
| Badge | plate with raised "KEEN", 0.2 mm press fit into the recess |

## Joints

- Every panel end sits 0.9 mm deep in a 1 mm rabbet along the inside edge of each side.
  The rabbet is open to the outside, so the panel's outer face is flush with the side's
  outline; the rabbets square the case. (Closed grooves would need the sides to stand
  proud of the panels.)
- No pegs: a 2 mm peg on a 2.4 mm panel edge leaves under 0.4 mm of wall on the outside.
  The panels form a closed loop between two rabbets and locate each other; tape them while
  the glue sets.
- Panel-to-panel edges are plain butt joints on design lines (floor, keyboard front,
  step, top rounding tangent, back plane); half-laps were dropped.
- The Back hatch slides straight forward: a tongue on its lower edge goes under an
  L-shaped lip on the Bottom (0.5 mm clearance above, 0.1 mm below) and is held by 2x M2 self-tapping screws
  (1.8 mm pilots) into tabs on the Top. Speaker, USB-C socket and switch are mounted
  on the hatch.

## Assembly

1. Put the KeyCaps into the Deck from below, screw the control PCB to the Deck (4x M2),
   glue the KeyField on, then press the JoyCap on through the KeyField.
2. Glue the shell: sides, Bottom, Front, Deck, Bezel, Top. Back stays off.
3. Press in FloppyInsert, Badge and the LED.
4. Screw the CYD to the Bezel standoffs (4x M3) through the back opening.
5. Wire: control PCB to CN1 through a 4-pin plug (3V3, GND, SDA, SCL); LED + resistor
   on 3V3/GND; speaker, USB-C and switch on the hatch.
6. Hook the hatch in and fix it with 2x M2.

The control PCB is not removable without breaking the glue; the plug lets the deck
be disconnected. This is accepted.

## Fits

| Fit | Value |
|---|---|
| Groove width | panel thickness + 0.3 |
| Pegs / holes | 2.0 / 2.4 |
| Cap hole | cap + 0.6 |
| Fake key gap | 0.4 all round |
| Press fits (floppy, badge) | 0.2 |
| LED hole | 3.1 |
| Other cutouts | as in the landscape cabinet |

## Printing

All shell panels print flat (outside or inside face down), no supports. Vent slots run
along the print direction. Sides print with the rabbets facing up.

## Verification

- Volume and bounding box of each body after each stage
- Interference checks: CYD envelope vs Bezel/Top, control PCB + switches vs Deck/Bottom,
  speaker vs Back, hatch hook vs Bottom lip
- Switch tops 1.5 mm above the KeyField surface
- Screenshots front, side, iso

## Files

- `cad/keen-crt-landscape.sc` (new design; the landscape cabinet file is not touched)
- `cad/print-crt/*.stl`, one STL per part

## Out of scope

- Functional SD slot behind the floppy opening
- Portrait variant
- Light pipe for the onboard CYD RGB LED
