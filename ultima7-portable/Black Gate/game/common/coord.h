#ifndef COORD_H
#define COORD_H

#include "byteorder.h"

/* The world is 3072 cells square and wraps at its edges. */
#define WORLD_SIZE  3072

#ifdef __cplusplus
extern "C" {
#endif
int16_t GetGreatestDelta2D(int16_u *x1, int16_u *y1, int16_u *x2, int16_u *y2);
int16_t GetGreatestDelta3D(int16_u *x1, int16_u *y1, int16_t z1, int16_u *x2, int16_u *y2, int16_t z2);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
struct Coord;
struct CellCoord;
extern "C" int16_t CompareWorldCoords(Coord *, Coord *);

/* (v + WORLD_SIZE) % WORLD_SIZE, without the divide where it is not needed: almost every v is
 * within a world's width of the map, and a 68000 has no 32-bit divide (a call into libgcc). Out
 * there, the same % as before, negative results and all. */
inline int16_t WrapWorld(int32_t v)
{
	int32_t t = v + WORLD_SIZE;

	if (t >= 0 && t < 2 * WORLD_SIZE)
		return (int16_t) (t >= WORLD_SIZE ? t - WORLD_SIZE : t);
	return (int16_t) (t % WORLD_SIZE);
}

/* CompareWorldCoords (coord.c) for the operators below, inline: the item sort calls it thousands
 * of times a frame. */
inline int16_t WorldDelta(int16_t a, int16_t b)
{
	int16_t v = a - b;

	if (v < 0)
		v += WORLD_SIZE;
	if (v >= WORLD_SIZE / 2)
		v -= WORLD_SIZE;
	return v;
}

/* A world x or y, wrapped into 0..3071. */
struct Coord {
	int16_t value;
	Coord() {}
	Coord(int32_t n) { value = n; value = WrapWorld(value); }
	Coord &operator=(const Coord &other) { value = other.value; return *this; }
	Coord &operator=(int32_t n) { value = n; value = WrapWorld(value); return *this; }
	operator int16_t() { return value; }
	void operator+=(int32_t n) { value += n; value = WrapWorld(value); }
	void operator-=(int32_t n) { value -= n; value = WrapWorld(value); }
	void operator++() { value++; value = WrapWorld(value); }
	Coord operator+(int32_t n) { Coord c(value + n); return c; }
	Coord operator-(int32_t n) { Coord c(value - n); return c; }
	/* how far this lies past other, the short way round */
	int16_t operator-(Coord other) { return WorldDelta(value, other.value); }
	int8_t operator<(CellCoord other);
	int8_t operator<=(CellCoord other);
	int8_t operator>(CellCoord other);
	int8_t operator>=(CellCoord other);
	int8_t operator==(const Coord &other) { return value == other.value; }
	int8_t operator!=(const Coord &other) { return value != other.value; }
};

/* A coord passed by value. */
struct Loc {
	int16_t value;
	Loc() {}
	Loc(int32_t n) { value = n; }
	Loc(const Coord &c) { value = c.value; }
	operator int16_t() { return value; }
	Coord operator+(int32_t n) { return value + n; }
	Coord operator-(int32_t n) { return value - n; }
};

/* A coord argument: an int wraps in place, a coord or loc is copied. */
struct CellCoord : Coord {
	CellCoord(int32_t n) { value = n; value = WrapWorld(value); }
	CellCoord(le_int16 n) : CellCoord((int32_t) (int16_t) n) {}
	CellCoord(const Coord &c) { value = c.value; }
	CellCoord(const Loc &l) { value = l.value; }
};

/* Order along the wrapping map; the right side is copied, so a sum is wrapped, then copied. */
inline int8_t Coord::operator<(CellCoord other) { return WorldDelta(value, other.value) < 0; }
inline int8_t Coord::operator<=(CellCoord other) { return WorldDelta(value, other.value) <= 0; }
inline int8_t Coord::operator>(CellCoord other) { return WorldDelta(value, other.value) > 0; }
inline int8_t Coord::operator>=(CellCoord other) { return WorldDelta(value, other.value) >= 0; }

/* A world coordinate wrapped into 0..3071. */
inline int16_t WrapCoord(int16_t v) { v = WrapWorld(v); return v; }

/* The sign and size of a step. */
inline int16_t GetSign(int16_t d) { if (d == 0) return 0; if (d < 0) return -1; return 1; }
inline int16_t GetMagnitude(int16_t d) { return d < 0 ? -d : d; }

/* How far a lies past b, on the wrapping map. */
inline int16_t GetDelta(Coord &a, Coord b) { return WorldDelta(a.value, b.value); }
inline int16_t GetDelta(Coord &&a, Coord b) { return GetDelta(a, b); }

/* Flat and height-aware distance between two map points. */
inline int16_t GetDistance(Coord x1, Coord y1, Coord x2, Coord y2)
{
	return GetGreatestDelta2D(&x1.value, &y1.value, &x2.value, &y2.value);
}
inline int16_t GetDistance(Coord x1, Coord y1, int16_t z1, Coord x2, Coord y2, int16_t z2)
{
	return GetGreatestDelta3D(&x1.value, &y1.value, z1, &x2.value, &y2.value, z2);
}
#endif

#endif
