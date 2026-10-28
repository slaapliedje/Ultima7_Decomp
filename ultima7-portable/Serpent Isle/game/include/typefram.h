#ifndef TYPEFRAM_H
#define TYPEFRAM_H

#include "byteorder.h"

/* A type in the low ten bits, a frame in the next five, and a flip flag in the top bit. */
struct TypeFrame {
	uint16_t bits;
	TypeFrame() {}
	TypeFrame(uint16_t n) { bits = n; }
	TypeFrame(le_uint16 n) { bits = n; }
	TypeFrame(le_int16 n) { bits = (uint16_t) (int16_t) n; }
	TypeFrame(uint16_t type, uint16_t frame) { bits = (type & 0x3ff) | ((frame << 10) & 0x7c00); }
	uint16_t type() { return bits & 0x3ff; }
	uint16_t frame() { return (bits & 0x7c00) >> 10; }
	uint8_t flipped() { return (bits & 0x8000) == 0x8000; }
	void setType(uint16_t type) & { bits = (type & 0x3ff) | (bits & 0xfc00); }
	/* Keeps the flip flag. */
	void setFrame(uint16_t frame) & { bits = (bits & 0x83ff) | ((frame << 10) & 0x7c00); }
	/* Takes the flip flag as frame bit 5. The setters take only a TypeFrame variable, not the
	 * copy ItemRecord::asTypeFrame returns. */
	void setFlipFrame(uint16_t frame) & { bits = (bits & 0x3ff) | ((frame << 10) & 0xfc00); }
};

#endif
