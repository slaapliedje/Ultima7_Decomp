#ifndef EGG_H
#define EGG_H

#include "byteorder.h"

/* Two bytes of an egg's data, read as a word or byte by byte; the word is little-endian, as in
 * the item buffer (egg.c, in C, uses only the bitfields). */
union EggData {
#ifdef __cplusplus
	le_uint16 word;
#else
	uint16_t word;
#endif
	struct { uint8_t first, second; } bytes;
};

/* Packed trigger metadata, chance and two payload words: an egg's extra record in the item
 * buffer, in the PC's layout. */
struct EggRecord {
	U7_BITS(uint16_t, kind : 4, criteria : 3, nocturnal : 1);
	U7_BITS(uint16_t, once : 1, hatched : 1, distance : 5, repeat : 1);
	uint8_t probability;
	union EggData data1;
	uint8_t unusedByte;
	union EggData data2;
};

#ifdef __cplusplus
extern "C" {
#endif
int16_t Egg_rollChance(struct EggRecord *egg);
void Egg_setHatched(int16_t *ref);
void Egg_clearHatched(int16_t *ref);
void Egg_setCriteriaAndDistance(int16_t *ref, uint8_t criteria, uint8_t distance);
#ifdef __cplusplus
}
#endif

#endif
