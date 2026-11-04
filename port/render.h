// Playfield renderer: replaces upstream gfx.c and its emulated EGA memory.
//
// Upstream draws into a 384-byte-wide EGA buffer; the visible tile grid
// (21 x 14 tiles) starts at buffer (32, 32). We keep only that grid as an
// 8-bit playfield (pf) and redraw it fully every frame. The 320x200 window
// inside it moves by the 0-15 px fine scroll.
#ifndef PORT_RENDER_H
#define PORT_RENDER_H

#include <stdbool.h>
#include <stdint.h>

#define PF_W 336
#define PF_H 224
#define PF_ORIGIN 32  // upstream EGA buffer coord of playfield (0,0), both axes

extern uint8_t *pf;   // heap, PF_W * PF_H

bool render_init(void);                    // malloc pf, bind asset tables
uint16_t ega_to_rgb565(uint8_t egaSignal); // 6-bit EGA signal -> RGB565
void render_present(void);                 // visible window -> port_present
void render_mark_full_redraw(void);        // doDrawing redrew the playfield (layout input)

// Image mode: a full-screen image (FINALE etc.) owns the playfield; the per-frame
// tile redraw is skipped until a level is loaded.
bool render_image_mode(void);
void render_image_mode_off(void);
void render_redraw_for_test(void);         // the tile-redraw step of doDrawing (honours image mode)

// Upstream gfx.c internal, kept with its name (also used by tests).
void CVort_private_engine_setEgaMemStartLocAndPanning(uint32_t egaStart, uint16_t panning);

#endif
