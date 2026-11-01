/* Black Gate U7.EXE, overlay segment 254 (file offsets 0x0787f0 to 0x078c9b, 1195 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include <string.h>
#include "plat.h"
#include "dosio.h"
#include "easyfile.h"
#include "flex.h"
#include "datanode.h"
#include "u7sound.h"
#include "colbuf.h"
#include "debug.h"
#include "oops.h"
#include "itemovr1.h"
#include "itemovr2.h"
#include "loadreg.h"
#include "sysusage.h"
#include "u7manage.h"
#include "u7npc.h"
#include "savegame.h"
#include "worldpal.h"
#include "chkfile.h"
#include "fileutil.h"
#include "gtimer.h"
#include "init.h"
#include "npcref.h"
#include "party.h"
#include "u7ibuf.h"

/* The version stamped in a saved game's header. */
#define SAVE_VERSION    300

/* A saved game: a Flex file of the files in the game directory. */
struct SaveGameFile : FlexWriter {
	int16_t pending, next;
	int16_t *slots;
	SaveGameFile();
	~SaveGameFile();
	void extract(char *name, char *dir);
	void save(char *dir, char *name, char *title, int16_t version, int16_t count);
};

GameFiles SaveGameFiles;

uint8_t SaveLoadActive = 0;
char *const InitGameFileName = "INITGAME.DAT";
char *const SaveGameFileName = "SAVEGAME.DAT";
char *const SaveFileNameFormat = "GAME%02d.U7";
char *const NPCFileName = "NPC.DAT";

/* Free bytes on the current drive. */
int32_t GetDiskFreeBytes(void)
{
	uint32_t bytes = plat_disk_free();

	/* DOS never reported more than 2 GB free. */
	return bytes > INT32_MAX ? INT32_MAX : (int32_t) bytes;
}

/* Free space on the current drive, in units of 400 KB. */
int16_t GameFiles::getFreeSaveSpace()
{
	return GetDiskFreeBytes() / (400 * INT32_C(1024));
}

/* Saves every registered object into dir, through refresh when flag is set. */
void GameFiles::saveAll(char *dir, int8_t flag)
{
	int16_t unusedMode = 0x9200;
	DataNode *p = SaveNodes;

	SaveLoadActive = 1;
	while (p != 0) {
		DebugPrintf("Saving \"%s\"\n", p->getName());
		if (flag)
			p->refresh(dir);
		else
			p->save(dir);
		p = p->next;
	}
	SaveLoadActive = 0;
}

/* Loads every registered object from dir. */
void GameFiles::loadAll(char *dir)
{
	int16_t unusedMode = 0x9100;
	DataNode *p = SaveNodes;

	SaveLoadActive = 1;
	while (p != 0) {
		DebugPrintf("Loading \"%s\"\n", p->getName());
		p->load(dir);
		p = p->next;
	}
	SaveLoadActive = 0;
}

/* Starts a new game: unpacks INITGAME.DAT into the game directory and loads it. */
int8_t MakeNewGame(void)
{
	SaveGameFile save;

	LogMemoryUsage("Making new game");
	plat_dir_create(GamedatPath);
	save.extract(BuildPath(StaticPath, InitGameFileName, 0), GamedatPath);
	LogMemoryUsage("Loading Npcs");
	LoadNpcs(BuildPath(GamedatPath, NPCFileName, 0));
	if (!FindAvatarNpc())
		ReportInvalidSaveGame();
	plat_file_remove(BuildPath(GamedatPath, NPCFileName, 0));
	LogMemoryUsage("Loading Regions");
	ForgetSavedRegions();
	LoadGameArgs();
	RestoreAvatarMana();
	LoadShapesInUse();
	return 1;
}

/* Port addition: a game converted from an Exult saved game by tools/exult2u7.py. Its saved game
 * holds what INITGAME.DAT holds, the NPCs in NPC.DAT and the regions, and IMPORT.DAT: in
 * little-endian words, its version (1), flags (1: the avatar can't move, 2: Armageddon is
 * done), the clock's ticks and days, each usecode timer's ticks and days, then the party's size
 * and its NPC numbers. It is built at start-up, as a new game is. */
char *const ImportFileName = "IMPORT.DAT";
extern GameTimer UsecodeTimers[];

static uint8_t IsImportedGame(void)
{
	return FileExists(BuildPath(GamedatPath, ImportFileName, 0));
}

static void ImportGame(void)
{
	DataFile f;
	int16_t i, size, number, flags;
	uint8_t attackMode;
	objref member;

	LoadNpcs(BuildPath(GamedatPath, NPCFileName, 0));
	if (!FindAvatarNpc())
		ReportInvalidSaveGame();
	plat_file_remove(BuildPath(GamedatPath, NPCFileName, 0));
	ForgetSavedRegions();
	if (f.open(BuildPath(GamedatPath, ImportFileName, 0), FILE_OPEN) != 1 || f.readWord() != 1)
		ReportInvalidSaveGame();
	flags = f.readWord();
	AvatarDontMove = (flags & 1) != 0;
	ArmageddonDone = (flags & 2) != 0;
	GameTime.ticks = f.readWord();
	GameTime.days = f.readWord();
	for (i = 0; i < 12; i++) {
		UsecodeTimers[i].ticks = f.readWord();
		UsecodeTimers[i].days = f.readWord();
	}
	AddToParty(AvatarRef, 0);
	size = f.readWord();
	for (i = 0; i < size; i++) {
		number = f.readWord();
		GetNpcIbo(&member, number);
		if (member.valid()) {
			/* joining sets the follow schedule, and the attack mode it keeps */
			attackMode = GetNpcBufferForIbo(&member)->defaultAttackMode;
			AddToParty(member, 0);
			GetNpcBufferForIbo(&member)->defaultAttackMode = attackMode;
		}
	}
	f.close();
	plat_file_remove(BuildPath(GamedatPath, ImportFileName, 0));
	LoadShapesInUse();
}

/* Loads the game from the game directory. */
void GameFiles::loadGame()
{
	loadAll(GamedatPath);
	if (!FindAvatarNpc())
		ReportInvalidSaveGame();
	ReloadRegionTerrain();
	GameScreen.restore();
}

/* Whether the game directory exists. */
uint8_t GameFiles::hasGameDirectory()
{
	uint8_t exists = 0;

	exists = plat_dir_exists(GamedatPath) != 0;
	return exists;
}

/* The file name of saved game n. */
char *GameFiles::getSaveFileName(int16_t n)
{
	return BuildNumberedPath("", SaveFileNameFormat, n, 0);
}

/* The title of saved game n, or "" when there is none. */
void GameFiles::getSaveTitle(int16_t n, char *title)
{
	Flex f;

	if (!f.open(getSaveFileName(n)))
		strcpy(title, "");
	else {
		f.hdr.getTitle(title);
		if (f.hdr.saveVersion != SAVE_VERSION)
			strcpy(title, "");
		f.close();
	}
}

/* Restores saved game n (-1: the game directory as it stands), starting a new game when there is
 * no game directory. */
void GameFiles::restoreGame(int16_t n)
{
	StopMusic();
	if (n != -1) {
		SaveGameFile save;
		save.extract(getSaveFileName(n), GamedatPath);
		if (IsImportedGame())
			QuitToDos();        /* to start again and build it */
	}
	if (!hasGameDirectory()) {
		MakeNewGame();
		saveGame(-1, 0, 1);
	} else if (IsImportedGame()) {
		ImportGame();
		saveGame(-1, 0, 1);
	}
	loadGame();
	PlayMusic(CurrentMusic);
}

/* Saves the game into the game directory and, unless n is -1, packs it as saved game n under
 * title. */
void GameFiles::saveGame(int16_t n, char *title, int8_t flag)
{
	saveAll(GamedatPath, flag);
	if (n != -1) {
		SaveGameFile save;
		save.save(GamedatPath, getSaveFileName(n), title, SAVE_VERSION, 300);
	}
}

/* Deletes the game directory and the files in it. */
void GameFiles::deleteGameDirectory()
{
	uint8_t done;
	plat_find ff;

	if (hasGameDirectory()) {
		done = !plat_find_first(BuildPath(GamedatPath, "*.*", 0), &ff);
		while (!done) {
			plat_file_remove(BuildPath(GamedatPath, ff.name, 0));
			done = !plat_find_next(&ff);
		}
		plat_dir_remove(GamedatPath);
	}
}

extern "C" void ResetSavegameGlobals(void)
{
	memset(&SaveGameFiles, 0, sizeof(SaveGameFiles));
	SaveLoadActive = 0;
}
