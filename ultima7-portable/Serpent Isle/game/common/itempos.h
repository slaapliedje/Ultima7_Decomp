#ifndef ITEMPOS_H
#define ITEMPOS_H

#include "itemrec.h"
#include "maps.h"

extern uint8_t LoadedRegions[4];
void Item_getXAndY(objref &ref, int16_t *x, int16_t *y);

/* x, y and z at once, reading the item's placement byte only once. item.cpp's, inline: the item
 * sort and the occlusion mask ask for it for each item in view, every frame. */
inline void Item_getXYZ(objref &ref, int16_t *x, int16_t *y, int16_t *z)
{
	ItemInfo info = GetItemZAndStuff(&ref);

	*z = info.z();
	if (info.kind() <= 3) {
		/* on the map, as everything in the chunk lists is */
		ItemRecord *record = ref.ptr();
		uint8_t region = LoadedRegions[info.kind()];

		*x = record->position.world.x + RegionX[region].value;
		*y = record->position.world.y + RegionY[region].value;
	} else
		Item_getXAndY(ref, x, y);
}

#endif
