//
//	IRE initialisation systems
//

#include "core.hpp"

// variables
#ifndef PATH_FILES_CONF
	#define PATH_FILES_CONF "/etc/ire/"
#endif

#ifndef PATH_FILES_DAT
	#define PATH_FILES_DAT "/usr/share/games/ire/"
#endif
// functions


/* Init group */

extern void CreateCacheTable();
extern void CheckCacheTable();
extern void Init_Sprites();
extern long Init_Sprite(int pos);       // Load and start up sprites
extern long Init_RoofTile(int pos);     // Load and start up roof tiles
extern long Init_LightMap(int number);	// Load and set up light map tile
extern void Init_Things();              // Load in the Characters
extern void Init_Sequences();           // Load in the animation sequences
extern void Init_Font();                // Set up the console font
extern void Init_VRM(int id);           // Load in a single VRM
extern void Init_PE(int pos);			// Register a PE function
extern void Init_Funcs();               // Init functions for all characters1
extern void InitFuncsFor(OBJECT *o,int rebuild);// Assign VRM to CHlist.funcs entries
extern void Init_Areas(OBJECT *o);      // Set up subwidth and subheight
extern void CalcSize(OBJECT *objsel);   // Calculate object size in tiles

/* Search group */

extern SEQ_POOL *findseq(const char *name);   // Find an animation sequence
extern S_POOL *find_spr(const char *name);

extern void Init_Lookups();
// Fast versions
extern int getnum4sequence(const char *r);	// Get index of animation sequence
extern int getnum4sprite(const char *r);		// Get index of an image
extern int getnum4PE(const char *name);		// Get index of PE function
extern int getnum4char(const char *r);		// Get index of an object
extern int getnum4table(const char *r);		// Get index of data table

// Slow versions (before lookups have been built)
extern int getnum4sequence_slow(const char *r);	// Get index of animation sequence
extern int getnum4sprite_slow(const char *r);		// Get index of an image
extern int getnum4VRM(const char *name);			// Get index of VRM function
extern int getnum4PE_slow(const char *name);		// Get index of PE function
extern int getnum4char_slow(const char *r);		// Get index of an object
extern int getnum4table_slow(const char *r);		// Get index of data table
extern int getnum4tile(const char *r);
extern int getnum4tilelink(const char *r);
extern int getnum4rooftop(const char *name);
extern int getnum4light(const char *name);
extern char *getname4string(const char *data);	// Convert string pointer to name
extern char *getstring4name(const char *name);
extern char *getstringtitle4name(const char *name);
extern int getnum4string(const char *data); // String pointer to num

// Random number generator

extern void qrand_init();
extern int qrand();

// Data Table queries

DT_ITEM *GetTableName_s(const char *tablename,const char *name);
DT_ITEM *GetTableNum_s(int table,const char *name);
DT_ITEM *GetTableNum_i(int table,long num);
DT_ITEM *GetTableName_i(const char *tablename,long num);
DT_ITEM *GetTableCase(const char *tablename,const char *name);
