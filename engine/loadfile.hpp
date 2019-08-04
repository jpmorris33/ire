//
//      Filesystem wrappers
//

#ifndef _IRE_LOADFILE
#define _IRE_LOADFILE

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "graphics/iregraph.hpp"


#ifdef __cplusplus

extern "C" char *loadfile(const char *srcname, char *destname);
extern "C" char *makefilename(const char *srcname, char *destname);
extern void adddatafile(char *df);
extern void checkdatafiles();
extern int fileexists(char *filename);
extern int filelen(char *filename);
extern char datafile_warning;
extern void mountdatafiles();
extern void clean_path(char *fn);
extern void makepath(char *fname);
extern void cpfile(char *local_path, char *abs_path, char *file_name);
extern int data_choose(char *path,char *path_file,int size);
extern void del_path_name(char *path_file,int size);

//#ifdef _IRE_IREGRAPH
extern IREBITMAP *iload_bitmap(char *filename);
extern void save_rle_sprite(IRESPRITE* spr, char *fname, long avgcol);
extern IRESPRITE* load_rle_sprite(char *fname, long *avgcol);
//#endif

#ifdef ALLEGRO_H
extern MIDI *iload_midi(char *filename);
extern SAMPLE *iload_wav(char *filename);
extern DATAFILE *iload_datafile(char *filename);
#endif

extern int iload_tempfile(char *filename, char *dest);

extern int ExportB64(const unsigned short *input, int inwords, char *out, int outlen);
extern int ImportB64(const char *input, unsigned short *out, int outlen);


typedef struct WadEntry
	{
	unsigned int start;
	char *name;
	} WadEntry;

extern void StartWad(IFILE *ofp);
extern void FinishWad(IFILE *ofp, WadEntry entries[]);
extern int GetWadEntry(IFILE *ifp, char *name);

// map file and directory-related things

extern char *makemapname(int number, int savegame, const char *extension);
extern int makesavegamedir(int savegamem, int erase);
extern void copysavegamedir(int src, int dest);


#else
extern char *loadfile(const char *srcname, char *destname);
#endif


// Macro to make a directory using the right API version
#ifdef _WIN32
	#define MAKE_DIR(a) mkdir(a)
	// We'll need the FINDFIRST stuff
	#include <direct.h>
#else
	#define MAKE_DIR(a) mkdir(a,S_IRUSR|S_IWUSR|S_IXUSR);
	#include <dirent.h>
#endif

#endif