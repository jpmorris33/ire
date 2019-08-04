//
//      loadsave.hpp - Map IO for starting the game, saving and loading
//

#define MAX_SAVES 1000 // This many savegame slots

// Functions

extern void LoadMap(int mapno);
extern void load_map(int mapno);
extern void save_map(int mapno);
extern void erase_curmap();

extern void load_z1(char *filename);
extern void save_z1(char *filename);
extern void load_z1A(char *filename);
extern void save_z1A(char *filename);
extern void wipe_z1();

extern void load_z2(int mapno);
extern void save_z2(int mapno);

extern void load_z3(int mapno);
extern void save_z3(int mapno);

extern void load_ms(char *filename);
extern void save_ms(char *filename);

extern void load_lightstate(int mapno, int sgnum);
extern void save_lightstate(int mapno, int sgnum);

extern void write_sgheader(int sgnum, int world, char *title);
extern char *read_sgheader(int sgnum, int *world);

extern void save_objects(int mapnum);
extern void restore_objects();

extern bool LoadGame(int savegame_no);
extern void SaveGame(int savegame_no, const char *title);

// Are we storing every tiny detail (savegame) or just the layout?
extern int MZ1_SavingGame;
// If we're loading the game at the moment, stop things like scripts running
extern int MZ1_LoadingGame;

extern unsigned int MakeSaveID(unsigned int map, unsigned int id);
extern unsigned int GetSaveID_ID(unsigned int saveid);
extern unsigned int GetSaveID_Map(unsigned int saveid);
