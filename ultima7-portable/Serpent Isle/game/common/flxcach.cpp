/* Serpent Isle SI.EXE, resident segment 54 (file offsets 0x0218ea to 0x021ac5, 475 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "vooalloc.h"
#include "oops.h"
#include "flxcach.h"

void FlexSpans::alloc(int16_t bytes)
{
	size = bytes;
	block = AllocateVoodooMemory(&VoodooXmsBlock, bytes);
	if (block == 0)
		ReportOutOfVoodooMemory();
}

/* The table keeps the file's little-endian entries; get and put convert. */
void FlexSpans::get(uint16_t i, void *dst)
{
	CopyLinearToFar(dst, block + i * sizeof(FlexEntry), sizeof(FlexEntry));
	((FlexEntry *) dst)->swap();
}

void FlexSpans::put(uint16_t i, void *src)
{
	FlexEntry image = *(FlexEntry *) src;

	image.swap();
	CopyFarToLinear(block + i * sizeof(FlexEntry), &image, sizeof(FlexEntry));
}

uint8_t CachedFlex::getEntry(int16_t i, FlexEntry *entry)
{
	if (!table.loaded()) {
		FlexEntry all;    /* the whole table, read as one entry */

		table.alloc((int16_t) hdr.count * sizeof(FlexEntry));
		all.offset = hdr.valid() ? INT32_C(128) : INT32_C(0);
		all.size = (int16_t) hdr.count * sizeof(FlexEntry);
		readEntryToVoodoo(&all, table.block, 0);
	}
	table.get(i, entry);
	return !entry->empty();
}

void CachedFlex::store(int16_t i, FlexEntry *entry)
{
	table.put(i, entry);
}

void CachedFlex::writeEntry(int16_t i, FlexEntry *entry)
{
	store(i, entry);
	FlexWriter::writeEntry(i, entry);
}

CachedFlex::~CachedFlex()
{
	close();
}
