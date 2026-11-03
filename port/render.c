// Playfield renderer, replaces upstream gfx.c. Filled in Task 4.
#include "core/globals.h"

void CVort_engine_clearOverlay() {}
void CVort_engine_copyToTxtMemory(uint8_t *buffer) { (void)buffer; }
void CVort_engine_decompGraphics() {}
void CVort_engine_drawBitmap(uint16_t x, uint16_t y, uint16_t num) { (void)x; (void)y; (void)num; }
void CVort_engine_drawChar(uint16_t x, uint16_t y, uint16_t val) { (void)x; (void)y; (void)val; }
void CVort_engine_drawScreen() {}
void CVort_engine_drawSprite(uint16_t x, uint16_t y, uint16_t num) { (void)x; (void)y; (void)num; }
uint16_t CVort_engine_drawSpriteAt(int32_t pos_x, int32_t pos_y, uint16_t frame) { (void)pos_x; (void)pos_y; (void)frame; return 0; }
void CVort_engine_drawTile(uint16_t x, uint16_t y, uint16_t num) { (void)x; (void)y; (void)num; }
uint16_t CVort_engine_drawTileAt(int32_t pos_x, int32_t pos_y, uint16_t tilenum) { (void)pos_x; (void)pos_y; (void)tilenum; return 0; }
void CVort_engine_gotoXY(uint8_t x, uint8_t y) { (void)x; (void)y; }
void CVort_engine_scrollText(int16_t top_line_offs, int16_t bot_line_offs, int16_t direction) { (void)top_line_offs; (void)bot_line_offs; (void)direction; }
void CVort_engine_setBorderColor(uint8_t color) { (void)color; }
void CVort_engine_setPaletteAndBorderColor(const uint8_t *palette) { (void)palette; }
bool CVort_engine_setVideoMode(int16_t vidMode) { (void)vidMode; return true; }
void CVort_engine_showImageFile(const char *filename) { (void)filename; }
void CVort_engine_syncDrawing() {}
void CVort_engine_updateActualDisplay() {}
