#include "u7port.h"
#include "arena.h"
#include "init.h"
#include "lowlevel.h"
#include "view.h"

/* Screen offset of each row of tiles, eight lines apart. */
const uint16_t TileRowOffsets[] = {
	0, 2560, 5120, 7680, 10240, 12800, 15360, 17920, 20480, 23040,
	25600, 28160, 30720, 33280, 35840, 38400, 40960, 43520, 46080, 48640,
	51200, 53760, 56320, 58880, 61440
};
int32_t ViewportFirstRow = 0;	/* linear address of the viewport's first row */
int16_t FlatModeFlags = 0;

/* Copy an 8x8 tile to the viewport at tile column x and tile row y. */
void DrawTile(int32_t tile, int16_t x, int16_t y)
{
	/* Pointers taken once: through LINEAR() in the loop, the compiler reloads LinearBase
	 * after every copy, as the copy might have changed it. */
	uint8_t *dst = LINEAR(TileRowOffsets[(uint16_t)y] + ViewportFirstRow + (uint32_t)(uint16_t)x * 8);
	const uint8_t *src = LINEAR(tile);
	int16_t i;

	for (i = 0; i < 8; i++) {
		memcpy(dst, src, 8);
		src += 8;
		dst += SCREEN_WIDTH;
	}
}

/* The terrain as paintTerrain last drew it, without the items over it: 40x25 tiles, each at its
 * world cell's column modulo 40 and row modulo 25, so the picture wraps round and a view that
 * scrolls finds most of its tiles already there. A tile goes in only when it differs from the one
 * there, and the picture is then copied into the viewport, cut where it wraps: that costs less
 * than drawing every tile into the viewport every frame. */
#define TERRAIN_LINES 200
static uint8_t TerrainPixels[SCREEN_WIDTH * TERRAIN_LINES];

/* Puts an 8x8 tile into the terrain picture at its tile column x and tile row y. */
void DrawTerrainTile(int32_t tile, int16_t x, int16_t y)
{
	uint8_t *dst = TerrainPixels + TileRowOffsets[(uint16_t)y] + (uint32_t)(uint16_t)x * 8;
	const uint8_t *src = LINEAR(tile);
	int16_t i;

	for (i = 0; i < 8; i++) {
		memcpy(dst, src, 8);
		src += 8;
		dst += SCREEN_WIDTH;
	}
}

/* Takes the viewport's tile at tile column x and row y into the terrain picture at its column
 * column and row row: for a cell paintTerrain leaves alone. */
void KeepTerrainTile(int16_t x, int16_t y, int16_t column, int16_t row)
{
	const uint8_t *src = LINEAR(TileRowOffsets[(uint16_t)y] + ViewportFirstRow + (uint32_t)(uint16_t)x * 8);
	uint8_t *dst = TerrainPixels + TileRowOffsets[(uint16_t)row] + (uint32_t)(uint16_t)column * 8;
	int16_t i;

	for (i = 0; i < 8; i++) {
		memcpy(dst, src, 8);
		src += SCREEN_WIDTH;
		dst += SCREEN_WIDTH;
	}
}

/* Copies the terrain picture into the viewport, the picture's tile column column and row row at
 * the viewport's top left. */
void ShowTerrain(int16_t column, int16_t row)
{
	uint8_t *dst = LINEAR(ViewportFirstRow);
	uint32_t left = (uint32_t)(uint16_t)column * 8, right = SCREEN_WIDTH - left;
	const uint8_t *src = TerrainPixels + TileRowOffsets[(uint16_t)row];
	const uint8_t *end = TerrainPixels + sizeof TerrainPixels;
	int16_t line;

	for (line = 0; line < TERRAIN_LINES; line++) {
		memcpy(dst, src + left, right);
		if (left != 0)
			memcpy(dst + right, src, left);
		dst += SCREEN_WIDTH;
		if ((src += SCREEN_WIDTH) == end)
			src = TerrainPixels;
	}
}

/* Fill a view's clip box with one color, row by row through its row table. */
void FillView(void *view, uint8_t color)
{
	View *v = (View *)view;
	uint32_t left = (uint16_t)v->clip.x0;
	uint32_t width = (uint32_t)(uint16_t)v->clip.x1 - left + 1;
	uint16_t rows = (uint16_t)(v->clip.y1 - v->clip.y0 + 1);
	int32_t rowPtr = v->rowTable + (uint32_t)(uint16_t)v->clip.y0 * 4;

	if (rows == 0)
		return;
	do {
		memset(LINEAR(LinearGet32(rowPtr) + left), color, width);
		rowPtr += 4;
	} while (--rows != 0);
}

extern "C" void ResetDrawtileGlobals(void)
{
	ViewportFirstRow = 0;
	FlatModeFlags = 0;
}
