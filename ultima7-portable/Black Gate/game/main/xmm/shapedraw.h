#ifndef SHAPEDRAW_H
#define SHAPEDRAW_H

struct View;

/* What a shape pixel does to the view pixel under it. */
enum ShapePixelMode {
	SHAPE_COPY,		/* replace it */
	SHAPE_TRANSLATE,	/* recolor it through a 256-byte table */
	SHAPE_BLEND		/* mix it through a translucency table */
};

/* Draw one frame of a run-length shape at x, y, clipped to the view's clip box.
 * Flipped swaps x and y, so each span runs down a column.
 * The table is a linear address, used by translate and blend.
 */
void DrawShapeFrame(struct View *view, int16_t x, int16_t y, int32_t shape, int16_t frameNum,
	bool flipped, enum ShapePixelMode mode, int32_t table);

/* A backend that draws shapes itself (U7_SHAPE_SPANS_BACKEND, the Jaguar's GPU) may still be
 * drawing when DrawShapeFrame returns, while SHAPE_SPANS_ASYNC is on: the world's items, whose
 * drawing needs nothing but shapes. SHAPE_SPANS_FINISH waits for it, before anything changes
 * what it reads: a shape the cache may move or drop, the translation tables. */
#ifdef U7_SHAPE_SPANS_BACKEND
#ifdef __cplusplus
extern "C" {
#endif
void shape_spans_finish(void);
void shape_spans_async(int on);
#ifdef __cplusplus
}
#endif
#define SHAPE_SPANS_FINISH() shape_spans_finish()
#define SHAPE_SPANS_ASYNC(on) shape_spans_async(on)
#else
#define SHAPE_SPANS_FINISH() ((void) 0)
#define SHAPE_SPANS_ASYNC(on) ((void) 0)
#endif

#endif
