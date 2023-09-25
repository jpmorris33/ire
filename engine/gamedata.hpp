/*
 *      Game state information
 */

#ifndef __IRE_GAMEDATA__
#define __IRE_GAMEDATA__


#define LMSIZE 32
#define VIEWDIST 10 // was 4

// Pathfinder results
#define PATH_FINISHED 0
#define PATH_WAITING -1
#define PATH_BLOCKED -2
#define EXPTAB_MAX 256

extern OBJECT *syspocket;          // Storage area for GUI objects
extern OBJECT *player;             // The player
extern OBJECT *victim;             // Global variable
extern OBJECT *person;             // Current person being moved
extern OBJECT *current_object;     // Global variable
extern OBJECT *fx_obj;             // Object special effect is centred around
extern TILE *current_tile;         // Global variable
extern OBJECT **party;
extern char **partyname;
extern WORLD *curmap;               // pointer to current world
extern WORLD worldcache[];
extern JOURNALENTRY *Journal;

extern long game_minute;     // Script engine doesn't support char
extern long game_hour;
extern long game_day;
extern long game_month;
extern long game_year;
extern long days_passed;
extern long day_of_week;

extern long mapx,mapy,mapx2,mapy2; // Coordinates for Seer
extern long new_x,new_y;           // Seer parameters
extern long Fader;                 // Lighting control
extern long SoundFixed;            // Stop sounds fading with distance if 1
extern long irekey;                // Key pressed by user
extern long show_roof;             // Project the roof? (Yes)
extern long force_roof;            // Force the roof to appear? (-1,0,1)
extern char fullrestore;          // Do we revert all objects to their defaults?
extern long ytab[MAX_H];           // Fast access to parts of the map
extern long buggy;                 // Were warnings detected?
extern long ire_running;
extern long combat_mode;
extern VMINT mu_volume,sf_volume;	  // Audio levels
extern long blanking_off;

extern long change_map,change_map_x,change_map_y,change_map_tag;

// Global vars, intended for use with the conversation engine
extern VMINT pe_usernum1,pe_usernum2,pe_usernum3,pe_usernum4,pe_usernum5;
extern char *pe_userstr1,*pe_userstr2,*pe_userstr3,*pe_userstr4,*pe_userstr5;


extern OBJECT *largemap[LMSIZE][LMSIZE];  // Large Object caches
extern OBJECT *solidmap[LMSIZE][LMSIZE];
extern OBJECT *bridgemap[LMSIZE][LMSIZE];
extern int decorxmap[LMSIZE][LMSIZE];
extern int decorymap[LMSIZE][LMSIZE];
extern OBJLIST *ActiveList;       // List of all active objects
extern OBJECT limbo;
extern VMINT exptab[EXPTAB_MAX];
extern VMINT exptab_max;

extern IRECURSOR *MouseOverPtr; // New mouse pointer
extern IRECURSOR *DefaultCursor;

// Cached ID numbers for 'system' VRMs

extern long Sysfunc_scheduler, Sysfunc_status, Sysfunc_follower;
extern long Sysfunc_splash, Sysfunc_trackstop, Sysfunc_updatelife;
extern long Sysfunc_erase, Sysfunc_wakeup, Sysfunc_updaterobot;
extern long Sysfunc_levelup, Sysfunc_playerbroken, Sysfunc_combat;
extern long mapnumber,fx_func;

// Resource Data

extern SEQ_POOL *SQlist; // Sequences in the SQlist
extern long SQtot;       // no. of sequences
extern OBJECT *CHlist;   // Array of characters
extern long CHtot;       // no. of characters
extern S_POOL *SPlist;   // Array of sprites
extern long SPtot;       // number of sprites
extern TILE *TIlist;   // Array of sprites
extern long TItot;       // number of sprites
extern S_POOL *RTlist;   // Array of sprites
extern long RTtot;       // number of sprites
extern L_POOL *LTlist;   // Array of sprites
extern long LTtot;       // number of sprites
extern long PEtot;	 // number of user functions
extern PEM *PElist;	// List of user functions
extern char **pe_files;	 // List of user script files (not funcs)
extern DATATABLE *DTlist;   // Array of lookups
extern long DTtot;       // number of lookups
extern TILELINK *TLlist;   // Array of tile links
extern long TLtot;       // number of tile links
extern ST_ITEM *STlist;   // Array of strings
extern long STtot;       // number of strings

extern long spr_alloc;
extern long seq_alloc;
extern long vrm_alloc;
extern long chr_alloc;
extern long til_alloc;
extern long mus_alloc;
extern long wav_alloc;
extern long rft_alloc;
extern long lgt_alloc;
extern long pef_alloc;
extern long tab_alloc;
extern long tli_alloc;
extern long str_alloc;

// Special Effects registers

extern long tfx_sx,tfx_sy;
extern long tfx_dx,tfx_dy;
extern long tfx_Brushsize,tfx_Alpha,tfx_intensity,tfx_falloff;
extern long tfx_picdelay1,tfx_picdelay2,tfx_picdelay3;
extern long tfx_radius,tfx_drift,tfx_speed;
extern IRECOLOUR *tfx_Colour;

extern void VM_RestoreRegs(OBJREGS *o);
extern void VM_SaveRegs(OBJREGS *o);


inline int GetNPCFlag(OBJECT *o, unsigned int flag) {
flag &= STRIPNPC;
return o->stats->npcflags & flag;
}

inline void SetNPCFlag(OBJECT *o, unsigned int flag) {
flag &= STRIPNPC;
o->stats->npcflags |= flag;
}

inline void ClearNPCFlag(OBJECT *o, unsigned int flag) {
flag &= STRIPNPC;
o->stats->npcflags &= ~flag;
}


#endif