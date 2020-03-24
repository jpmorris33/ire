/*
 *      Game state information
 */

#include "core.hpp"
#include "media.hpp"
#include "gamedata.hpp"

#define MAX_WORLDS 4

OBJECT *syspocket;          // Storage area for GUI objects
OBJECT *player;             // The player
OBJECT *victim;             // Global variable
OBJECT *person;             // Current object being moved
OBJECT *current_object;     // Global variable
OBJECT *fx_obj;
TILE *current_tile;         // Global variable
OBJECT **party;
char **partyname;
WORLD *curmap;               // pointer to current world
WORLD worldcache[MAX_WORLDS];
OBJECT limbo;              // This is where things are placed between worlds

long game_minute=0;     // The script engine only supports integer (not char)
long game_hour=0;
long game_day=0;
long game_month=0;
long game_year=0;
long days_passed=0;
long day_of_week=0;

long mapx=0,mapy=0,mapx2=0,mapy2=0; // Coordinates for Seer
long new_x,new_y;           // Seer parameters
long Fader=0;               // Lighting control
long SoundFixed=0;          // Stop sounds fading with distance if 1
long irekey;                // Key pressed by user
long show_roof=1;           // Project the roof? (Yes)
long force_roof=0;          // Roof override (0 = none, -1 = hide, 1 = show)
char fullrestore=0;        // Do we revert all objects to their defaults?
long ytab[MAX_H];           // Fast access to parts of the map
long buggy=0;               // Were warnings detected?
long combat_mode=0;
long ire_running=1;
long blanking_off=0;	   // Display everything, visible or no?

// Map transition variables
long change_map=0,change_map_x,change_map_y,change_map_tag=0;

// Global vars, intended for use with the conversation engine
VMINT pe_usernum1,pe_usernum2,pe_usernum3,pe_usernum4,pe_usernum5;
char *pe_userstr1=NULL, *pe_userstr2=NULL, *pe_userstr3=NULL, *pe_userstr4=NULL, *pe_userstr5=NULL;

// Map window info

int VSW=8; // Tiles
int VSH=8;
int VSA=64;
int VSMIDX;
int VSMIDY;
int VSW32; // Pixels
int VSH32;
int VSA32;

OBJECT *largemap[LMSIZE][LMSIZE];  // Large Object caches
OBJECT *solidmap[LMSIZE][LMSIZE];
OBJECT *bridgemap[LMSIZE][LMSIZE];
int decorxmap[LMSIZE][LMSIZE];
int decorymap[LMSIZE][LMSIZE];
OBJLIST *ActiveList=NULL;  // List of all active objects

// Cached ID numbers for 'system' VRMs

long Sysfunc_scheduler = -1, Sysfunc_status = -1, Sysfunc_follower = -1;
long Sysfunc_splash = -1, Sysfunc_trackstop = -1, Sysfunc_updatelife = -1;
long Sysfunc_erase = -1, Sysfunc_wakeup = -1, Sysfunc_updaterobot = -1;
long Sysfunc_levelup = -1;
long mapnumber=1,fx_func;

VMINT exptab[EXPTAB_MAX];
VMINT exptab_max;

// Resource data info

struct S_POOL    *SPlist;              // Sprite list
struct SEQ_POOL  *SQlist;              // Sequence list
struct OBJECT    *CHlist;              // Main list of characters
struct TILE      *TIlist;              // Map tile
struct S_POOL    *RTlist;              // Roof-tile list
struct L_POOL    *LTlist;              // Lightmap list
struct DATATABLE *DTlist;              // Data tables list
struct TILELINK  *TLlist;              // Map tile connections
struct ST_ITEM   *STlist;              // Map tile connections
struct JOURNALENTRY *Journal=NULL;

IRECURSOR *MouseOverPtr=NULL;             // New mouse pointer
IRECURSOR *DefaultCursor=NULL;

char **pe_files;			// List of user script files (not funcs)
extern struct SMTab *wavtab;          // Sound table
extern struct SMTab *mustab;          // Music table

long SPtot;                     // Total number of sprite
long SQtot;                     // Total number of sequence
long COtot;                     // Total number of code
long CHtot;                     // Total number of characters
long TItot;                     // Total number of tiles
long RTtot;                     // Total number of roof tiles
long LTtot;                     // Total number of Lights
long PEtot;			            // Total number of PissEasy functions
long DTtot;			            // Total number of Data Tables
long TLtot;			            // Total number of Tile Links
long STtot;			            // Total number of Strings


long spr_alloc=0;
long seq_alloc=0;
long vrm_alloc=0;
long chr_alloc=0;
long til_alloc=0;
long mus_alloc=0;
long wav_alloc=0;
long rft_alloc=256;
long lgt_alloc=256;
long pef_alloc=0;
long tab_alloc=0;
long tli_alloc=0;
long str_alloc=0;

// Special Effects Engine registers

long tfx_sx,tfx_sy,tfx_radius,tfx_drift,tfx_speed;
long tfx_dx,tfx_dy;
long tfx_Brushsize,tfx_Alpha,tfx_intensity,tfx_falloff;
long tfx_picdelay1=0,tfx_picdelay2=0,tfx_picdelay3=0;

/*
 *  Save game variables
 */

void VM_SaveRegs(OBJREGS *o)
{
o->player = player;
o->current = current_object;
o->me = person;
o->victim = victim;
}

/*
 *  Restore system variables
 */

void VM_RestoreRegs(OBJREGS *o)
{
player = o->player;
current_object = o->current;
person = o->me;
victim = o->victim;
}
