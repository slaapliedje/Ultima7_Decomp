/* Byte order. The game's files, saves and arena are little-endian, as on the PC; a
 * big-endian host (the 68000 family) converts multi-byte values as they cross into native
 * structs and back. On a little-endian host every conversion here compiles to nothing.
 */
#ifndef BYTEORDER_H
#define BYTEORDER_H

#include <stddef.h>
#include <stdint.h>

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define U7_BIG_ENDIAN 1
#else
#define U7_BIG_ENDIAN 0
#endif

static inline uint16_t SwapBytes16(uint16_t v)
{
	return (uint16_t) (v << 8 | v >> 8);
}

static inline uint32_t SwapBytes32(uint32_t v)
{
	return v << 24 | (v & 0xff00) << 8 | (v >> 8 & 0xff00) | v >> 24;
}

/* Little-endian to host order, or host to little-endian: the same swap either way. */
static inline uint16_t LE16(uint16_t v) { return U7_BIG_ENDIAN ? SwapBytes16(v) : v; }
static inline uint32_t LE32(uint32_t v) { return U7_BIG_ENDIAN ? SwapBytes32(v) : v; }

/* Little-endian values at any address. */
static inline uint16_t LoadLE16(const void *p)
{
	const uint8_t *b = (const uint8_t *) p;

	return (uint16_t) (b[0] | b[1] << 8);
}

static inline uint32_t LoadLE32(const void *p)
{
	return (uint32_t) LoadLE16(p) | (uint32_t) LoadLE16((const uint8_t *) p + 2) << 16;
}

static inline void StoreLE16(void *p, uint16_t v)
{
	uint8_t *b = (uint8_t *) p;

	b[0] = (uint8_t) v;
	b[1] = (uint8_t) (v >> 8);
}

static inline void StoreLE32(void *p, uint32_t v)
{
	StoreLE16(p, (uint16_t) v);
	StoreLE16((uint8_t *) p + 2, (uint16_t) (v >> 16));
}

static inline uint32_t SwapLESized(uint32_t v, size_t size)
{
	if (!U7_BIG_ENDIAN)
		return v;
	return size == 2 ? SwapBytes16((uint16_t) v) : size == 4 ? SwapBytes32(v) : v;
}

/* Converts one struct field or variable in place, whatever its integer type; one-byte
 * fields are left alone. Works on packed fields, which C++ cannot bind to references. */
#define SWAP_LE(field) ((field) = (__typeof__(field)) SwapLESized((uint32_t) (field), sizeof(field)))

/* Bitfields: GCC fills a storage unit from its low bit on x86 and from its high bit on a
 * big-endian host. U7_BITS declares fields in the x86 layout on either, by declaring them in
 * reverse order on a big-endian host:
 *   U7_BITS(uint8_t, lucky : 1, explodes : 1, damageType : 6);
 * Each U7_BITS must fill exactly one byte, and start on a byte boundary. Up to 8 fields. */
#if U7_BIG_ENDIAN
#define U7_BITS(type, ...) type U7_REVERSE(__VA_ARGS__)
#else
#define U7_BITS(type, ...) type __VA_ARGS__
#endif

#define U7_COUNT(...) U7_COUNT_(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define U7_COUNT_(_1, _2, _3, _4, _5, _6, _7, _8, n, ...) n
#define U7_JOIN(a, b) U7_JOIN_(a, b)
#define U7_JOIN_(a, b) a##b
#define U7_REVERSE(...) U7_JOIN(U7_REVERSE_, U7_COUNT(__VA_ARGS__))(__VA_ARGS__)
#define U7_REVERSE_1(a) a
#define U7_REVERSE_2(a, b) b, a
#define U7_REVERSE_3(a, b, c) c, b, a
#define U7_REVERSE_4(a, b, c, d) d, c, b, a
#define U7_REVERSE_5(a, b, c, d, e) e, d, c, b, a
#define U7_REVERSE_6(a, b, c, d, e, f) f, e, d, c, b, a
#define U7_REVERSE_7(a, b, c, d, e, f, g) g, f, e, d, c, b, a
#define U7_REVERSE_8(a, b, c, d, e, f, g, h) h, g, f, e, d, c, b, a

#ifdef __cplusplus
/* A 16- or 32-bit value kept little-endian in memory, as the PC kept it, converted on every read
 * and write. For records that share memory with files and with unions of bytes, so they keep
 * the PC's byte layout whatever the host: the item buffer, its list heads and the NPC records.
 * Copying one copies the bytes, unconverted. */
template <class T> struct LittleEndian {
	T raw;

	operator T() const { return (T) SwapLESized((uint32_t) raw, sizeof(T)); }
	LittleEndian &operator=(T v) { raw = (T) SwapLESized((uint32_t) v, sizeof(T)); return *this; }
	LittleEndian &operator+=(T v) { return *this = (T) (*this + v); }
	LittleEndian &operator-=(T v) { return *this = (T) (*this - v); }
	LittleEndian &operator&=(T v) { return *this = (T) (*this & v); }
	LittleEndian &operator|=(T v) { return *this = (T) (*this | v); }
	LittleEndian &operator^=(T v) { return *this = (T) (*this ^ v); }
	LittleEndian &operator++() { return *this += 1; }
	LittleEndian &operator--() { return *this -= 1; }
	T operator++(int) { T old = *this; *this += 1; return old; }
	T operator--(int) { T old = *this; *this -= 1; return old; }
};
typedef LittleEndian<int16_t> le_int16;
typedef LittleEndian<uint16_t> le_uint16;
typedef LittleEndian<int32_t> le_int32;
typedef LittleEndian<uint32_t> le_uint32;
#endif

/* Converts an array of 16- or 32-bit values in place. */
static inline void SwapLE16Array(void *p, size_t count)
{
	if (U7_BIG_ENDIAN) {
		for (uint8_t *b = (uint8_t *) p; count > 0; count--, b += 2) {
			uint8_t t = b[0];

			b[0] = b[1];
			b[1] = t;
		}
	}
}

static inline void SwapLE32Array(void *p, size_t count)
{
	if (U7_BIG_ENDIAN) {
		for (uint8_t *b = (uint8_t *) p; count > 0; count--, b += 4) {
			uint8_t t0 = b[0], t1 = b[1];

			b[0] = b[3];
			b[1] = b[2];
			b[2] = t1;
			b[3] = t0;
		}
	}
}

#endif
