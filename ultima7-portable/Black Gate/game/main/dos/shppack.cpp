#include "u7port.h"
#include "arena.h"

extern "C" int32_t PackShapeFrames(int32_t shape, int32_t keep);

/* Packs a shape in linear memory down to the frames whose bits are set in the dword at keep:
 * dropped frames' bytes are closed up and their table entries zeroed. The shape starts with
 * its size, then a table of frame offsets. Returns the new size.
 *
 * The original closed up each dropped frame by moving everything after it; this moves each run
 * of kept frames once, to the same place. The shape comes out the same; only the bytes past its
 * new end, which the cache takes back, are left otherwise. */
int32_t PackShapeFrames(int32_t shape, int32_t keep)
{
	uint32_t bits = LinearGet32(keep);
	uint32_t total = LinearGet32(shape);
	int32_t frames = (int32_t) (LinearGet32(shape + 4) >> 2) - 1;
	int32_t entry = shape + 4;
	uint32_t removed = 0, runFrom = 0, runSize = 0, start, end;
	int32_t i;

	if (frames <= 1)
		return (int32_t) total;
	for (i = 0; i < frames; i++, entry += 4, bits >>= 1) {
		start = LinearGet32(entry);
		end = i + 1 < frames ? LinearGet32(entry + 4) : total;
		if (bits & 1) {
			if (removed != 0) {
				LinearPut32(entry, start - removed);
				if (runSize == 0)
					runFrom = start;
				runSize += end - start;
			}
			continue;
		}
		/* a dropped frame: the kept run before it goes down by what was dropped before that */
		if (runSize != 0) {
			memmove(LINEAR(shape + runFrom - removed), LINEAR(shape + runFrom), runSize);
			runSize = 0;
		}
		LinearPut32(entry, 0);
		removed += end - start;
	}
	if (runSize != 0)
		memmove(LINEAR(shape + runFrom - removed), LINEAR(shape + runFrom), runSize);
	LinearPut32(shape, total - removed);
	return (int32_t) (total - removed);
}
