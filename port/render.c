// Playfield renderer, replaces upstream gfx.c. See render.h for the idea.
//
// Functions marked "verbatim" are copied unchanged from upstream
// src/render/gfx.c; the rest keep upstream's pixel formulas but write
// into the playfield with clipping instead of into emulated EGA memory.
#include <stdlib.h>
#include <string.h>
#include "core/globals.h"
#include "engine/engine_timing.h"
#include "keen1_assets.h"
#include "backend.h"
#include "render.h"

uint8_t *pf;

static uint8_t *tilePtrs[611], *fontPtrs[256], *bmpPtrs[18], *spritePtrs[119];
static uint16_t pal565[16];
static int fineX, fineY;

// Standard 16-color EGA RGB values (index = IRGB-ish EGA entry, 6 = brown).
const uint32_t engine_egaRGBColorTable[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
};

// verbatim (gfx.c): 200-line mode treats secondary green as intensity.
uint8_t CVort_private_engine_convertEGASignalToEGAEntry(const uint8_t color)
{
	return (color & 7) | ((color & 16) >> 1);
}

static uint16_t rgb888_to_565(uint32_t c) {
    return (uint16_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c & 0xFF) >> 3));
}

uint16_t ega_to_rgb565(uint8_t egaSignal) {
    return rgb888_to_565(engine_egaRGBColorTable[CVort_private_engine_convertEGASignalToEGAEntry(egaSignal)]);
}

static void rebuildPalette(void) {
    for (int i = 0; i < 16; i++)
        pal565[i] = rgb888_to_565(engine_egaRGBColorTable[engine_screen.client.currParsedPalette[i] & 15]);
}

bool render_init(void) {
    if (!pf) pf = (uint8_t *)malloc(PF_W * PF_H);
    if (!pf) return false;
    memset(pf, 0, PF_W * PF_H);

    engine_egaHeadGeneral = keen1_egaHead;
    engine_egaHeadUnmasked = (EGAHeadUnmasked_T *)keen1_bmpHeads;
    engine_maskedSpriteEntry = (MaskedSpriteEntry_T *)keen1_spriteEntries;
    for (int i = 0; i < 611; i++) tilePtrs[i] = (uint8_t *)keen1_tileData + 256 * i;
    for (int i = 0; i < 256; i++) fontPtrs[i] = (uint8_t *)keen1_fontData + 64 * i;
    for (int i = 0; i < 18; i++) bmpPtrs[i] = (uint8_t *)keen1_bmpData + keen1_bmpOffsets[i];
    for (int i = 0; i < 119; i++) spritePtrs[i] = (uint8_t *)keen1_spriteData + keen1_spriteOffsets[i];
    engine_egaTiles = tilePtrs;
    engine_egaFonts = fontPtrs;
    engine_egaBmps = bmpPtrs;
    engine_egaSprites = spritePtrs;

    for (int i = 0; i < 16; i++) engine_screen.client.currParsedPalette[i] = (uint8_t)i;
    rebuildPalette();
    return true;
}

/* Map an upstream EGA byte offset (engine_dstPage + x + y*384, either page)
   to playfield coordinates. */
static void offsetToPf(uint32_t offset, int *x, int *y) {
    uint32_t rel = offset % 0x18000;
    *x = (int)(rel % ENGINE_EGA_GFX_SCANLINE_LEN) - PF_ORIGIN;
    *y = (int)(rel / ENGINE_EGA_GFX_SCANLINE_LEN) - PF_ORIGIN;
}

static inline bool inPf(int x, int y) { return (unsigned)x < PF_W && (unsigned)y < PF_H; }

void CVort_engine_drawChar(uint16_t x, uint16_t y, uint16_t val)
{
	int px, py;
	offsetToPf(engine_dstPage + (x << 3) + y * ENGINE_EGA_GFX_SCANLINE_LEN, &px, &py);
	const uint8_t *src = engine_egaFonts[val & 0xFF];
	for (int cy = 0; cy < 8; cy++)
		for (int cx = 0; cx < 8; cx++, src++)
			if (inPf(px + cx, py + cy)) pf[(py + cy) * PF_W + px + cx] = *src;
	engine_isFrameReadyToDisplay = true;
}

void CVort_engine_drawSprite(uint16_t x, uint16_t y, uint16_t num)
{
	if (num >= 4*engine_egaHeadGeneral.spriteNum)
		num = 0;
	x = (x << 3) + ((num & 3) << 1);
	const uint8_t *spritePixelPtr = engine_egaSprites[num/4];
	num -= (num&3);
	int px, py;
	offsetToPf(engine_dstPage + x + y * ENGINE_EGA_GFX_SCANLINE_LEN, &px, &py);
	int w = 8 * engine_maskedSpriteEntry[num].width, h = engine_maskedSpriteEntry[num].height;
	for (int cy = 0; cy < h; cy++)
		for (int cx = 0; cx < w; cx++, spritePixelPtr++)
		{
			if (!inPf(px + cx, py + cy)) continue;
			uint8_t *dst = &pf[(py + cy) * PF_W + px + cx];
			// Upstream formula: mask bit keeps background, else copy pixel.
			*dst = (((((*spritePixelPtr) & 16) >> 4)*15)&(*dst))|((*spritePixelPtr) & 15);
		}
	engine_isFrameReadyToDisplay = true;
}

void CVort_engine_drawTile(uint16_t x, uint16_t y, uint16_t num)
{
	if (((num | 0x8000) ^ 0x8000) >= engine_egaHeadGeneral.tileNum)
		num = num & 0x8000;
	x <<= 3;
	bool isMasked = (num & 0x8000);
	if (isMasked)
		num = (num & 0x7FFF);
	else
		num = anim_plane[anim_plane_i][num];

	const uint8_t *tilePixelPtr = engine_egaTiles[num];
	int px, py;
	offsetToPf(engine_dstPage + x + y * ENGINE_EGA_GFX_SCANLINE_LEN, &px, &py);
	for (int cy = 0; cy < 16; cy++)
		for (int cx = 0; cx < 16; cx++, tilePixelPtr++)
		{
			if (!inPf(px + cx, py + cy)) continue;
			uint8_t *dst = &pf[(py + cy) * PF_W + px + cx];
			if (isMasked) {
				// Upstream: mask comes from the following tile.
				uint8_t tileMask = *(tilePixelPtr+256);
				*dst = (uint8_t)(((*dst)&tileMask)|((*tilePixelPtr)&(tileMask^0xFF)));
			} else
				*dst = *tilePixelPtr;
		}
	engine_isFrameReadyToDisplay = true;
}

void CVort_engine_drawBitmap(uint16_t x, uint16_t y, uint16_t num)
{
	if (num >= engine_egaHeadGeneral.bmpNum)
		num = 0;
	x <<= 3;
	const uint8_t *src = engine_egaBmps[num];
	int px, py;
	offsetToPf(engine_dstPage + x + y * ENGINE_EGA_GFX_SCANLINE_LEN, &px, &py);
	int w = 8 * engine_egaHeadUnmasked[num].h, h = engine_egaHeadUnmasked[num].v;
	for (int cy = 0; cy < h; cy++)
		for (int cx = 0; cx < w; cx++, src++)
			if (inPf(px + cx, py + cy)) pf[(py + cy) * PF_W + px + cx] = *src;
	engine_isFrameReadyToDisplay = true;
}

/* Full redraw of the 21x14 tile grid. Replaces upstream's adaptive tile
   refresh (ATR), which only redrew changed tiles into a page of EGA memory. */
static void redrawAllTiles(uint16_t initTileIndex)
{
	for (uint16_t loopVar = 0; loopVar < 0x126; loopVar++)
	{
		uint16_t mapTile = (uint16_t)map_data_tiles[initTileIndex + loopVar];
		if (mapTile < engine_egaHeadGeneral.tileNum) {
			const uint8_t *src = engine_egaTiles[anim_plane[anim_plane_i][mapTile]];
			uint8_t *dst = pf + (loopVar / 21) * 16 * PF_W + (loopVar % 21) * 16;
			for (int cy = 0; cy < 16; cy++, src += 16, dst += PF_W)
				memcpy(dst, src, 16);
		}
		if (loopVar % 21 == 20)
			initTileIndex += screen_wrap_single;
	}
}

// Window into the playfield, from upstream's EGA start address + panning.
void CVort_private_engine_setEgaMemStartLocAndPanning(uint32_t egaStart, uint16_t panning)
{
	int x, y;
	offsetToPf(egaStart, &x, &y);
	x += panning;
	fineX = x < 0 ? 0 : (x > PF_W - 320 ? PF_W - 320 : x);
	fineY = y < 0 ? 0 : (y > PF_H - 200 ? PF_H - 200 : y);
}

void render_present(void) {
    port_present(pf + fineY * PF_W + fineX, PF_W, pal565);
}

void CVort_engine_updateActualDisplay()
{
	if (!engine_isFrameReadyToDisplay)
		return;
	engine_isFrameReadyToDisplay = false;
	render_present();
}

void CVort_engine_setBorderColor(const uint8_t color)
{
	engine_screen.client.currParsedBorderColor = CVort_private_engine_convertEGASignalToEGAEntry(color);
}

void CVort_engine_setPaletteAndBorderColor(const uint8_t *palette) {
	CVort_engine_setBorderColor(palette[16]);
	for (int i = 0; i < 16; i++) {
		engine_screen.client.currParsedPalette[i] =  CVort_private_engine_convertEGASignalToEGAEntry(palette[i]);
	}
	rebuildPalette();
	engine_isFrameReadyToDisplay = true;
}

void CVort_engine_doDrawing()
{
	CVort_engine_egaPageFlip();
	anim_plane_i = (((CVort_private_engine_getTicks() & 65535) >> g_game.anim_speed) & 6) >> 1;
	redrawAllTiles((((map_width_bytes * ((scroll_y & 16777215) >> 12)) & 65535) >> 1) + ((scroll_x & 16777215) >> 12));
	uint32_t origDstPage = engine_dstPage;
	engine_dstPage = engine_currPageStart;
	uint16_t loopVar, drawCounter;
	drawCounter = spritedraws_c;
	if (drawCounter)
		for (loopVar = 0; drawCounter > 0; loopVar++, drawCounter--)
			CVort_engine_drawSprite(spritedraws[loopVar].x_byte, spritedraws[loopVar].y_row, spritedraws[loopVar].sprite_copy);
	drawCounter = tiledraws_c;
	if (drawCounter)
		for (loopVar = 0; drawCounter > 0; loopVar++, drawCounter--)
			CVort_engine_drawTile(tiledraws[loopVar].x_byte, tiledraws[loopVar].y_line, tiledraws[loopVar].tile_id);
	drawCounter = bmpdraws_c;
	if (drawCounter)
		for (loopVar = 0; drawCounter > 0; loopVar++, drawCounter--)
			CVort_engine_drawBitmap(bmpdraws[loopVar].x, bmpdraws[loopVar].y, bmpdraws[loopVar].bmp_loc);
	engine_dstPage = origDstPage;
	if (draw_func)
		(draw_func)();
	CVort_private_engine_setEgaMemStartLocAndPanning(engine_egaStart, pel_panning);
	engine_isFrameReadyToDisplay = true;
	CVort_engine_delay(1);
}

// verbatim (gfx.c)
uint16_t CVort_engine_drawSpriteAt(int32_t pos_x, int32_t pos_y, uint16_t frame)
{
	int16_t tileX1, tileX2, tileY1, tileY2;
	int16_t sdraw_xbyte, sdraw_yrow;
	uint16_t sdraw_scopy;
	uint16_t tileID;
	int16_t tileType;

	sdraw_xbyte = pos_x / 0x100 - ((scroll_x / 0x100)&0xFFF0);
	sdraw_yrow  = pos_y / 0x100 - ((scroll_y / 0x100)&0xFFF0);

	if ((sdraw_xbyte < -0x20) || (sdraw_yrow < -0x20) || (sdraw_xbyte > 0x150) || (sdraw_yrow > 0xC7))
		return 0;
	// getMSE
	sdraw_scopy = (sdraw_xbyte&7)/2+(frame<<2);
	g_entities.temp_MSE = engine_maskedSpriteEntry[sdraw_scopy];

	sdraw_xbyte = (sdraw_xbyte+0x20)/8-4;
	tileX1 = sdraw_xbyte/2;
	if (tileX1 > ENGINE_SPRITE_CLIP_MAX_X)
		return 0;
	if (tileX1 < 0)
		tileX1 = 0;
	tileX2 = (sdraw_xbyte+g_entities.temp_MSE.width-1)/2;
	if (tileX2 < 0)
		return 0;
	if (tileX2 > ENGINE_SPRITE_CLIP_MAX_X)
		tileX2 = ENGINE_SPRITE_CLIP_MAX_X;
	tileY1 = sdraw_yrow/0x10;
	if (tileY1 > ENGINE_SPRITE_CLIP_MAX_Y1) // This is not a typo. 0xE, not 0xD.
		return 0;
	if (tileY1 < 0)
		tileY1 = 0;
	tileY2 = (sdraw_yrow+g_entities.temp_MSE.height-1)/0x10;
	if (tileY2 < 0)
		return 0;
	if (tileY2 > ENGINE_SPRITE_CLIP_MAX_Y2) // And yet, here we have 0xD as a bound.
		tileY2 = ENGINE_SPRITE_CLIP_MAX_Y2;

	sdraw_xbyte += 4;
	sdraw_yrow += 0x20;

	for (int16_t currY = tileY1, currX; currY <= tileY2; currY++)
		for (currX = tileX1; currX <= tileX2; currX++)
		{
			tileID = map_data_tiles[(((currY+scroll_y_tile)*map_width_tile)&65535)+(currX+scroll_x_tile)];
			tileType = TILEINFO_Type[tileID];
			if (tileType >= 0)
			{
				// For ATR: Update this tile
				screentiles[screentiles_i] = currY*ENGINE_SCREEN_TILE_ROW_WIDTH+currX;
				screentiles_i++;
			}
			else
			{
				tiledraws[tiledraws_i].x_byte = (currX<<1)+4;
				tiledraws[tiledraws_i].y_line = (currY<<4)+0x20;
				tiledraws[tiledraws_i].tile_id = tileID;
				// Do masked
				if (tileType == -2)
				{
					tiledraws[tiledraws_i].tile_id |= 0x8000;
					// For ATR: Update this tile
					screentiles[screentiles_i] = currY*ENGINE_SCREEN_TILE_ROW_WIDTH+currX;
					screentiles_i++;
				}
				tiledraws_c++;
				tiledraws_i++;
			}
		}
	spritedraws[spritedraws_i].x_byte = sdraw_xbyte;
	spritedraws[spritedraws_i].y_row = sdraw_yrow;
	spritedraws[spritedraws_i].sprite_copy = sdraw_scopy;
	spritedraws_c++;
	spritedraws_i++;
	return 1;
}

// verbatim (gfx.c)
uint16_t CVort_engine_drawTileAt(int32_t pos_x, int32_t pos_y, uint16_t tilenum)
{
	int16_t tileX1, tileX2, tileY1, tileY2;
	int16_t sdraw_xbyte, sdraw_yrow;
	sdraw_xbyte = pos_x / 0x100 - ((scroll_x / 0x100)&0xFFF0);
	sdraw_yrow = pos_y / 0x100 - ((scroll_y / 0x100)&0xFFF0);
	if ((sdraw_xbyte < -0x20) || (sdraw_yrow < -0x20))
		return 0;

	sdraw_xbyte = (sdraw_xbyte+0x20)/8-4;
	tileX1 = sdraw_xbyte/2;
	if (tileX1 > ENGINE_SPRITE_CLIP_MAX_X)
		return 0;
	if (tileX1 < 0)
		tileX1 = 0;
	tileX2 = (sdraw_xbyte+g_entities.temp_MSE.width-1)/2;
	if (tileX2 < 0)
		return 0;
	if (tileX2 > ENGINE_SPRITE_CLIP_MAX_X2)
		tileX2 = ENGINE_SPRITE_CLIP_MAX_X2;
	tileY1 = sdraw_yrow/0x10;
	if (tileY1 > ENGINE_SPRITE_CLIP_MAX_Y2) // 0xD is a common bound here...
		return 0;
	if (tileY1 < 0)
		tileY1 = 0;
	tileY2 = (sdraw_yrow+g_entities.temp_MSE.height-1)/0x10;
	if (tileY2 < 0)
		return 0;
	if (tileY2 > ENGINE_SPRITE_CLIP_MAX_Y2) // ...and here.
		tileY2 = ENGINE_SPRITE_CLIP_MAX_Y2;

	sdraw_xbyte += 4;
	sdraw_yrow += 0x20;

	for (int16_t currY = tileY1, currX; currY <= tileY2; currY++)
		for (currX = tileX1; currX <= tileX2; currX++)
		{
			screentiles[screentiles_i] = currY*ENGINE_SCREEN_TILE_ROW_WIDTH+currX;
			screentiles_i++;
		}
	tiledraws[tiledraws_i].x_byte = sdraw_xbyte;
	tiledraws[tiledraws_i].y_line = sdraw_yrow;
	tiledraws[tiledraws_i].tile_id = tilenum;
	tiledraws_c++;
	tiledraws_i++;
	return 1;
}

// verbatim (gfx.c)
void CVort_engine_clearOverlay()
{	// Luckily this is endianness independent!!
	memset(ATR[0], 0xFF, 2*0x126);
	memset(ATR[1], 0xFF, 2*0x126);
}

// verbatim (gfx.c)
void CVort_engine_syncDrawing() {
	tiledraws_c = bmpdraws_c = spritedraws_c = 0; // Counts = 0

	spritedraws_i = 0;
	bmpdraws_i = 0;
	tiledraws_i = 0;
	screentiles_i = 0;

        // Instead of a busy loop or even a similar loop that ends with a
        // potentially too long sleep, we use an AMORTIZED delay function.
        CVort_engine_delayInGameTicks(ticks_sync, 6);
        // Should be non-negative, in fact ~6. For demos this is FORCED to 6.
	if (engine_arguments.extras.vorticonsDemoModeToggle) {
		g_game.sprite_sync = 6;
		ticks_sync += 6;
		CVort_private_engine_setTicks(ticks_sync);
	} else {
	g_game.sprite_sync = (CVort_private_engine_getTicks()&0xFFFF)-(ticks_sync&0xFFFF);
		if (g_game.sprite_sync > 15)
			g_game.sprite_sync = 15;
		ticks_sync = CVort_private_engine_getTicks();
	}
}

// verbatim (gfx.c)
void CVort_engine_drawScreen()
{
	CVort_engine_doDrawing();
	uint8_t refreshPage = engine_currPage^1;
	// For ATR:  Update these tiles
	for (int tileCounter = 0; tileCounter < screentiles_i; tileCounter++)
		ATR[refreshPage][screentiles[tileCounter]] = 0xFFFF;
}

// verbatim (gfx.c)
void CVort_engine_egaPageFlip()
{
	// Hope it is all right... note that we measure by pixel==BYTE,
	// not BIT (within a single EGA plane).
	engine_currPage ^= 1;
	engine_currPageStart = 0x18000*engine_currPage;

	engine_egaStart = 0x3020+(0x180*(unsigned)((scroll_y >> 8) & 0xF)) + (((scroll_x & 0x800) >> 11) << 3) + engine_currPageStart;
	engine_dstPage = (engine_egaStart >> 7) << 7;

	pel_panning = (scroll_x >> 8) & 7;
}

/* Not needed for the level-1 milestone (text mode, image files). */
void CVort_engine_copyToTxtMemory(uint8_t *buffer) { (void)buffer; }
void CVort_engine_decompGraphics() {} // assets are pre-converted in flash
void CVort_engine_gotoXY(uint8_t x, uint8_t y) { (void)x; (void)y; }
void CVort_engine_scrollText(int16_t top_line_offs, int16_t bot_line_offs, int16_t direction) { (void)top_line_offs; (void)bot_line_offs; (void)direction; }
bool CVort_engine_setVideoMode(int16_t vidMode) { (void)vidMode; return true; }
void CVort_engine_showImageFile(const char *filename) { (void)filename; port_log("showImageFile(%s) not supported", filename); }
