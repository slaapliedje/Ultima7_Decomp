/* Linear memory: one arena, addressed by 32-bit offsets.
 * The game calls these offsets linear addresses; offset 0 is never handed out.
 * Bytes in the arena keep the little-endian order the game's files use.
 */
#ifndef ARENA_H
#define ARENA_H

#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t *LinearBase;
extern uint32_t LinearSize;

/* Allocates the arena; the far heap, extended memory and the screen all come from it.
 * Later calls do nothing. */
void InitLinearMemory(uint32_t size);

/* The game uses about 2 MB of it; a port short of memory may build with a smaller arena. */
#ifndef LINEAR_MEMORY_SIZE
#define LINEAR_MEMORY_SIZE (UINT32_C(32) << 20)
#endif

/* The 320x200 screen buffer, inside the arena. */
uint8_t *ScreenPixels(void);

/* Where InitLinearMemory put the far heap and extended (Voodoo) memory. */
extern uint32_t FarHeapArea, FarHeapAreaSize;
extern uint32_t ExtendedArea, ExtendedAreaSize;

#ifdef U7_ARENA_CHECK
/* Test builds: logs an address outside the arena and returns a stand-in for it. */
uint8_t *ArenaCheck(uint32_t address);
uint8_t *ArenaCheckAllocate(uint32_t size);
#endif

#ifdef __cplusplus
}
#endif

#ifdef U7_ARENA_CHECK
#define LINEAR(address) ArenaCheck((uint32_t)(address))
#else
#define LINEAR(address) (LinearBase + (uint32_t)(address))
#endif

static inline uint8_t LinearGet8(int32_t a) { return LINEAR(a)[0]; }
static inline uint16_t LinearGet16(int32_t a) { return LoadLE16(LINEAR(a)); }
static inline uint32_t LinearGet32(int32_t a) { return LoadLE32(LINEAR(a)); }
static inline void LinearPut8(int32_t a, uint8_t v) { LINEAR(a)[0] = v; }
static inline void LinearPut16(int32_t a, uint16_t v) { StoreLE16(LINEAR(a), v); }
static inline void LinearPut32(int32_t a, uint32_t v) { StoreLE32(LINEAR(a), v); }

#endif
