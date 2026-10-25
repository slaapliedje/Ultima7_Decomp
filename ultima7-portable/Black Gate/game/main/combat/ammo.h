#ifndef AMMO_H
#define AMMO_H

#include "byteorder.h"

#include "lowlevel.h"
#include "makemojo.h"

struct ShapeLookup;

/* One AMMO.DAT record. */
struct AmmoRecord {
	int16_t type;
	int16_t family;
	int16_t projectile;
	int8_t damage;
	U7_BITS(uint8_t, lucky : 1, autoHit : 1, returns : 1, passesBlockers : 1, removeOnStop : 1, keepOnStop : 1, explodes : 1,
		unusedFlag : 1);
	uint8_t unusedByte;
	U7_BITS(uint8_t, unusedBits : 4, damageType : 4);
	U7_BITS(uint8_t, sleep : 1, charm : 1, curse : 1, poison : 1, paralyze : 1, drainMana : 1, drainHealth : 1,
		noDamage : 1);
	char unusedTail[2];
	/* Between the file's little-endian order and the host's. */
	void swap() { SWAP_LE(type); SWAP_LE(family); SWAP_LE(projectile); }
};
static_assert(sizeof(AmmoRecord) == 13, "AmmoRecord is read from a 13-byte file record");

/* Ammunition records: 13-byte records in voodoo memory, the last one read kept in current. */
struct AmmoTable {
	int32_t base;
	char current[13];
	int32_t cached, count;
	AmmoTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	AmmoRecord *get(int32_t n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 13, 13);
			((AmmoRecord *) current)->swap();
			cached = n;
		}
		return (AmmoRecord *)current;
	}
	void read(int32_t n, AmmoRecord *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 13, 13);
		((AmmoRecord *) record)->swap();
	}
};

extern char AmmoFileName[];
extern AmmoTable AmmoRecords;
extern uint8_t AmmoCount;
extern ShapeLookup AmmoLookup;

#endif
