#include "u7port.h"
#include "arena.h"
#include "byteorder.h"
#include "view.h"
#include "shapedraw.h"

/* A shape starts with its size and a table of 32-bit frame offsets; the first offset
 * also marks where the table ends. A frame starts with four 16-bit extents from its
 * hot spot, then its spans, each:
 *
 *   header    length * 2, plus 1 if run-encoded; a length of 0 ends the frame
 *   offsets   along the span, then across it, from the hot spot
 *   pixels    raw bytes, or runs of count * 2 plus 1 for a fill of the byte after it
 *
 * Spans are drawn straight from the shape, run by run, with the pixel mode and the
 * direction fixed per frame: most runs are a few pixels long, so a library call or a
 * mode test per run or per pixel costs more than the pixels themselves.
 */
#define SHAPE_FIRST_FRAME       4
#define FRAME_HEADER_SIZE       8

/* Runs shorter than this are drawn with an inline loop, longer ones with memcpy or memset.
 * The loops must stay loops: GCC would otherwise turn them back into library calls. */
#define SHORT_RUN               32
#define NO_LIBCALLS             __attribute__((optimize("no-tree-loop-distribute-patterns")))

/* Draws count shape pixels from pixels, step bytes apart on the screen. */
template <enum ShapePixelMode Mode, int32_t Step>
static inline NO_LIBCALLS __attribute__((always_inline)) void PutPixels(uint8_t *screen, const uint8_t *pixels, int32_t count,
	const uint8_t *table)
{
	if (Mode == SHAPE_COPY && Step == 1 && count >= SHORT_RUN) {
		memcpy(screen, pixels, (size_t)count);
		return;
	}
	for (; count > 0; count--, screen += Step, pixels++) {
		uint8_t mix;

		switch (Mode) {
		case SHAPE_COPY:
			*screen = *pixels;
			break;
		case SHAPE_TRANSLATE:
			*screen = table[*screen];
			break;
		case SHAPE_BLEND:
			/* 0xff keeps the pixel solid; otherwise it picks the 256-byte
			 * table, counted from the table's start, that recolors the screen. */
			mix = table[*pixels];
			*screen = mix == 0xff ? *pixels : table[mix * 256 + *screen];
			break;
		}
	}
}

/* Draws count shape pixels that all have the value pixel. */
template <enum ShapePixelMode Mode, int32_t Step>
static inline NO_LIBCALLS __attribute__((always_inline)) void PutFill(uint8_t *screen, uint8_t pixel, int32_t count,
	const uint8_t *table)
{
	if (Mode == SHAPE_BLEND) {
		uint8_t mix = table[pixel];

		if (mix != 0xff) {
			const uint8_t *recolor = table + mix * 256;

			for (; count > 0; count--, screen += Step)
				*screen = recolor[*screen];
			return;
		}
	} else if (Mode == SHAPE_TRANSLATE) {
		for (; count > 0; count--, screen += Step)
			*screen = table[*screen];
		return;
	}
	if (Step == 1 && count >= SHORT_RUN) {
		memset(screen, pixel, (size_t)count);
		return;
	}
	for (; count > 0; count--, screen += Step)
		*screen = pixel;
}

/* Returns the address just past a span of length pixels starting at source. */
static inline const uint8_t *SkipSpan(const uint8_t *source, int32_t length, bool runEncoded)
{
	if (!runEncoded)
		return source + length;
	while (length > 0) {
		uint8_t run = *source++;

		length -= run >> 1;
		source += run & 1 ? 1 : run >> 1;
	}
	return source;
}

/* Flipped spans run down a column, a whole screen row per pixel, whatever the view's row table. */
template <enum ShapePixelMode Mode, bool Flipped>
static NO_LIBCALLS void DrawSpans(const struct View *view, int16_t x, int16_t y, const uint8_t *source,
	const uint8_t *table)
{
	enum { step = Flipped ? SCREEN_WIDTH : 1 };
	const int16_t alongMin = Flipped ? view->clip.y0 : view->clip.x0;
	const int16_t alongMax = Flipped ? view->clip.y1 : view->clip.x1;
	const int16_t acrossMin = Flipped ? view->clip.x0 : view->clip.y0;
	const int16_t acrossMax = Flipped ? view->clip.x1 : view->clip.y1;
	const int16_t alongBase = Flipped ? y : x, acrossBase = Flipped ? x : y;
	const int32_t rowTable = view->rowTable;

	for (;;) {
		uint16_t header = LoadLE16(source);
		int32_t length = header >> 1;
		bool runEncoded = header & 1;
		int16_t along, across;
		int32_t first, last, pos;
		uint8_t *screen;

		if (length == 0)
			return;
		along = (int16_t)(alongBase + (int16_t)LoadLE16(source + 2));
		across = (int16_t)(acrossBase + (int16_t)LoadLE16(source + 4));
		source += 6;

		if (across < acrossMin || across > acrossMax || along > alongMax
			|| (int16_t)(along + length - 1) < alongMin) {
			source = SkipSpan(source, length, runEncoded);
			continue;
		}
		/* The visible pixels of the span, counted from its start. */
		first = alongMin > along ? alongMin - along : 0;
		last = alongMax - along < length - 1 ? alongMax - along : length - 1;
		if (first > last) {
			source = SkipSpan(source, length, runEncoded);
			continue;
		}

		/* screen is where pixel first of the span goes */
		if (Flipped)
			screen = LINEAR(LinearGet32(rowTable + (along + first) * 4) + across);
		else
			screen = LINEAR(LinearGet32(rowTable + across * 4) + along + first);

		if (!runEncoded) {
			PutPixels<Mode, step>(screen, source + first, last - first + 1, table);
			source += length;
			continue;
		}
		for (pos = 0; pos < length;) {
			uint8_t run = *source++;
			int32_t count = run >> 1;
			int32_t from = pos > first ? pos : first;
			int32_t to = pos + count - 1 < last ? pos + count - 1 : last;

			if (run & 1) {
				if (from <= to)
					PutFill<Mode, step>(screen + (from - first) * step, *source, to - from + 1, table);
				source++;
			} else {
				if (from <= to)
					PutPixels<Mode, step>(screen + (from - first) * step, source + (from - pos),
						to - from + 1, table);
				source += count;
			}
			pos += count;
		}
	}
}

#ifdef U7_SHAPE_SPANS_68K
/* The Atari backend's 68030 version of DrawSpans<SHAPE_COPY, false>, the most used. */
extern "C" void shape_spans_copy(const uint8_t *source, int32_t x, int32_t y, const int16_t clip[4],
	const uint8_t *rowTable, uint8_t *linearBase);
#endif

void DrawShapeFrame(struct View *view, int16_t x, int16_t y, int32_t shape, int16_t frameNum,
	bool flipped, enum ShapePixelMode mode, int32_t table)
{
	uint16_t entry = (uint16_t)((frameNum + 1) * 4);
	const uint8_t *tableBytes = LINEAR(table);
	const uint8_t *source;

	if (LinearGet16(shape + SHAPE_FIRST_FRAME) < entry)
		return;
	source = LINEAR(shape + (int32_t)LinearGet32(shape + entry) + FRAME_HEADER_SIZE);
	switch (mode) {
	case SHAPE_COPY:
		if (flipped)
			DrawSpans<SHAPE_COPY, true>(view, x, y, source, tableBytes);
		else
#ifdef U7_SHAPE_SPANS_68K
			shape_spans_copy(source, x, y, &view->clip.x0, LINEAR(view->rowTable), LinearBase);
#else
			DrawSpans<SHAPE_COPY, false>(view, x, y, source, tableBytes);
#endif
		break;
	case SHAPE_TRANSLATE:
		if (flipped)
			DrawSpans<SHAPE_TRANSLATE, true>(view, x, y, source, tableBytes);
		else
			DrawSpans<SHAPE_TRANSLATE, false>(view, x, y, source, tableBytes);
		break;
	case SHAPE_BLEND:
		if (flipped)
			DrawSpans<SHAPE_BLEND, true>(view, x, y, source, tableBytes);
		else
			DrawSpans<SHAPE_BLEND, false>(view, x, y, source, tableBytes);
		break;
	}
}

extern "C" void ResetShapedrawGlobals(void)
{
}
