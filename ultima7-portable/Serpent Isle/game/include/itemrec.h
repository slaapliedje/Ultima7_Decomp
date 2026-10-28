#ifndef ITEMREC_H
#define ITEMREC_H

#include "byteorder.h"

#include "typefram.h"
#include "objref.h"
#include "iteminfo.h"

/* Where an item lies: a cell in its region on the map, or its container. */
union ItemPosition {
	struct { uint8_t x, y; } world;
	le_int16 parent;
};

/* The 8-byte record in the item buffer. Its words stay little-endian, as in the game's files,
 * so the unions and the files keep the PC's layout on any host. */
struct ItemRecord {
	le_int16 next;
	ItemPosition position;
	le_uint16 typeFrame;
	union {
		le_int16 extra;
		struct { uint8_t z, quality; } direct;
	} data;
	void setTypeFrame(TypeFrame value) { typeFrame = value.bits; }
	TypeFrame asTypeFrame() { return TypeFrame((uint16_t) typeFrame); }
	void setFlipFrame(uint16_t frame) { TypeFrame t = asTypeFrame(); t.setFlipFrame(frame); setTypeFrame(t); }
};

/* A record's first six bytes, copied to draw an item without changing it. */
struct ItemSnapshot {
	le_int16 next;
	ItemPosition position;
	le_uint16 typeFrame;
	ItemSnapshot() {}
};

/* The next item in the same list. */
inline objref objref::next() { return ptr()->next; }
inline uint16_t objref::type() { return ptr()->typeFrame & 0x3ff; }
inline uint16_t objref::frame() { return (ptr()->typeFrame & 0x7c00) >> 10; }
inline uint16_t objref::z() { return GetItemZAndStuff(this).z(); }

/* An item's type number. */
inline uint16_t GetItemType(objref item) { return item.type(); }

/* Where an item lies: 0 to 3 a map region, then off the map, deleted, contained or equipped. */
inline uint8_t GetItemKind(objref *item)
{
	ItemInfo info = GetItemZAndStuff(item);
	return (uint8_t)((uint16_t)info.flags & 7);
}
inline uint8_t IsOnMap(objref *item)
{
	ItemInfo info = GetItemZAndStuff(item);
	return info.kind() <= 3;
}
inline uint8_t IsContained(objref *item)
{
	ItemInfo info = GetItemZAndStuff(item);
	return (uint8_t)((uint16_t)info.flags & 7) >= 6;
}
inline uint8_t IsTemporary(objref *item) { return GetItemZAndStuff(item).flags & 8; }

#endif
