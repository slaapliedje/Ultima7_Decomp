#ifndef U7MAP_H
#define U7MAP_H

struct MapCoordsSaver;

struct Coord;

void ScreenToWorldCoords(int16_t x, int16_t y, int16_u *tx, int16_u *ty, int8_t snap);
void WorldCoordsToScreen(Coord tx, Coord ty, int16_u *x, int16_u *y);

extern MapCoordsSaver MapCoordsFile;
extern char *MapCoordFileName;

#endif
