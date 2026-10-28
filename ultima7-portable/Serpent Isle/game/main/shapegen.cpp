/* Serpent Isle SI.EXE, overlay segment 334 (file offsets 0x09cb30 to 0x09cd9e, 622 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "byteorder.h"
#include "lowlevel.h"
#include "u7manage.h"

/* A view's clip corners, each x and y read as one long. */
struct View {
	int16_t id;
	int32_t rowTable;
	int32_t topLeft;
	int32_t bottomRight;
};

/* A corner read as one long: on the PC x is its low word; on a big-endian host the halves
 * come out the other way round. */
static int32_t Corner(int32_t v)
{
	return U7_BIG_ENDIAN ? (int32_t) ((uint32_t) v << 16 | (uint32_t) v >> 16) : v;
}

/* Builds a new shape block from the image in view source: EncodeFrame first measures (no
 * destination), then writes the data after an 8-byte header holding its length and the offset 8. */
int16_t MakeShapeFromImage(int16_t source)
{
	View *view;
	uint16_t size;
	int16_t block;
	int32_t dest;

	view = gShapeManager.lockView(source);
	size = EncodeFrame(view, Corner(view->topLeft), Corner(view->bottomRight), Corner(view->topLeft), INT32_C(0),
		0xff, 101);
	block = gShapeManager.allocateBlock(size + 100, 0x7fff, 0);
	view = gShapeManager.lockView(source);
	dest = gShapeManager.get(block) + 8;
	view = gShapeManager.lockView(source);
	EncodeFrame(view, Corner(view->topLeft), Corner(view->bottomRight), Corner(view->topLeft), dest, 0xff, 101);
	PokeLong(gShapeManager.get(block), size + INT32_C(8));
	PokeLong(gShapeManager.get(block) + 4, INT32_C(8));
	return block;
}
