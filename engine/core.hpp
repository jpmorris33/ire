/*
	IRE:		Core Data structures

	Version: 	2.0, 29/2/00

*/

#ifndef __IRCORE
#define __IRCORE

#define IKV "0.99"              // IRE Kernel Version

#include <stdlib.h>
#include "types.h"
#include "graphics/iregraph.hpp"
#include "graphics/irekeys.hpp"

#define ACT_STACK 8             // Sub-Activity stack

#define RANDOM_TILE 0xffff      // Appears as an eyesore

// SaveID values and manipulation constants
#define SAVEID_NONE 0
#define SAVEID_INVALID 0xffffffff
#define SAVEID_MAPMASK 0xfe000000
#define SAVEID_MAPBITS 7
#define SAVEID_IDMASK 0x01ffffff
#define SAVEID_IDBITS 25

#define CHAR_U 0  // Directions for the sequence cache in CHlist
#define CHAR_D 1
#define CHAR_L 2
#define CHAR_R 3

#define FP_DIAGONAL 1   // Pathfinder flags
#define FP_FINDROUTE 2

#define KILLROOF	1
#define KILLMAP		2

#define MAPSIG 0xf15e0005

#define MAX_W 8192
#define MAX_H 8192
#define MAX_MEMBERS 128

#define MAP_POS(x,y) (ytab[y]+(x))


///////////////////////////////////////////////////////////////////////////
//  Script Processing Data Structures                                    //
///////////////////////////////////////////////////////////////////////////

// NPC Flags

#define STRIPNPC	0x7fffffff
#define IS_FEMALE       0x80000001
#define KNOW_NAME       0x80000002
#define IS_HERO         0x80000004
#define CANT_EAT        0x80000008
#define CANT_DRINK      0x80000008      // Synonymous with CANT_EAT
#define IS_CRITICAL     0x80000010
#define NOT_CLOSE_DOOR  0x80000020
#define NOT_CLOSE_DOORS 0x80000020
#define IS_SYMLINK      0x80000040
#define IS_BIOLOGICAL   0x80000080
#define IS_GUARD        0x80000100
#define IS_SPAWNED      0x80000200
#define NOT_OPEN_DOOR   0x80000400
#define NOT_OPEN_DOORS  0x80000400
#define IN_BED          0x80000800
#define NO_SCHEDULE     0x80001000
#define IS_OVERDUE      0x80002000
#define IS_WIELDED      0x80004000
#define IS_ROBOT        0x80008000
#define IN_PARTY        0x80010000	// Party members aren't always considered solid

/*
typedef struct NPC_FLAGS
{
unsigned int female		:1; // Male or Female
unsigned int know_name		:1; // Do I know you?
unsigned int is_hero		:1; // Is this the hero?
unsigned int cant_eat		:1; // Can it eat and drink? (Robots can't)
unsigned int critical		:1; // Has gone critical
unsigned int no_shutdoor	:1; // Won't shut doors after themselves
unsigned int symlink		:1; // Owner is the NPC, not object (phones etc)
unsigned int biological		:1; // Affected by poison etc
unsigned int guard			:1; // Policeman
unsigned int spawned		:1; // Dynamically created (and destroyed ;-)
unsigned int no_opendoor	:1; // Can't open doors
unsigned int in_bed		:1; // In bed
unsigned int ignoreschedule	:1; // temporary monomania (e.g. for a plot event)
unsigned int scheduleoverdue	:1; // overdue, go right to bed NOW
unsigned int spare		:17;
} NPC_FLAGS;
*/

typedef struct NPC_RECORDING
{
char *page;
//struct OBJECT *player;
char *readername;		// Text, not a pointer or SaveID since since a book/npc may not exist from map to map
				// And SaveIDs may not exist at this point.
struct NPC_RECORDING *next;
} NPC_RECORDING;

typedef struct CHAR_LABELS
{
char *rank;
char *race;
char *party;
char *location;

} CHAR_LABELS;


typedef struct JOURNALENTRY
{
VMINT id;	// Journal entry ID, or -1 for a user-written entry
VMINT day;	// Day no
char date[32];	// YYYY/MM/DD HH:MM
char *name;	// For ID -1 (User), this must be freed, else pointer to journal name
char *text;	// For ID -1 (User), this must be freed, else pointer to journal text
char *title;	// For ID -1 (User), this must be freed, else pointer to journal title
char *tag;	// For ID -1 (User), this must be freed, else pointer to journal tag
VMINT status;	// User data, e.g. whether the task has been completed
struct JOURNALENTRY *next;
} JOURNALENTRY;


// Statistics

typedef struct STATS
{
VMINT hp;
VMINT dex;
VMINT str;
VMINT intel;
VMINT weight;
VMINT quantity;
VMINT armour;
VMINT tick;                   // Animation timer
VMUINT npcflags;
VMINT damage;                 // Damage (caused by a weapon)
VMINT radius;
OBJECTID owner;
VMINT karma;                  // Have you been a good little boy? (or girl)
VMINT bulk;
VMINT range;
VMINT speed;
VMINT level;                  // skill level
VMINT alignment;
} STATS;


// Temporary data for a game in progress
// Changing this may well break savegame compatability

#define USEDATA_VERSION 256	// 256 bytes, this must always be the size in bytes

#define USEDATA_SLOTS 20
#define USEDATA_POTIONS 10
#define USEDATA_ACTSTACK ACT_STACK

typedef struct USEDATA
{
VMINT user[USEDATA_SLOTS];
VMINT poison;
VMINT unconscious;
VMINT potion[USEDATA_POTIONS]; // space for 10 potions
VMINT dx,dy;      // Used by the pathfinder
VMINT vigilante;  // If this is set, you have a license to kill people
VMINT edecor;     // Used by the Editor
VMINT counter;    // Counter.  Used by eggs or the pathfinder
VMINT experience; // Experience points
VMINT magic;      // Magic points
VMINT archive;    // Used when switching worlds
VMINT oldhp;      // Previous hp, used internally
OBJECTID arcpocket; // Used when switching worlds
OBJECTID pathgoal;  // For the long-range pathfinder

// Sub-Activities
VMINT actlist[USEDATA_ACTSTACK];
struct OBJECT *acttarget[USEDATA_ACTSTACK];
VMINT actptr,actlen;

VMINT fx_func; // Special effects function number or 0

NPC_RECORDING *npctalk;     // Used to record what you've said for continuity
NPC_RECORDING *lFlags;      // Local Flags
} USEDATA;

// Archive states for objects as they move between worlds

enum
	{
	UARC_NORMAL,
	UARC_ONCEONLY,
	UARC_SYSPOCKET,
	UARC_INPOCKET,
	UARC_SYSLIST,
	};

// A single sprite

typedef struct S_POOL
{
char *name;
char *fname;
IRESPRITE *image;
IRECOLOUR *thumbcol;

VMINT w,h;
VMUINT wxh;
//VMINT x,y;
unsigned char flags;
VMINT darkness;
} S_POOL;

// A single lightmap sprite

typedef struct L_POOL
{
char *fname;
IRELIGHTMAP *image;
} L_POOL;

// A complete animation sequence

typedef struct SEQ_POOL
{
char *name;
S_POOL **seq;
VMINT frames;
VMUINT flags;	// See SEQ_FLAGS
VMINT x,y;	// Control/collision hotspot
VMINT ox,oy;	// Overlay offset
unsigned char speed;
S_POOL *overlay;        // This is always drawn on top of the sprite
                        // but it has no physical existence
struct SEQ_POOL *jumpto;// Sequence to jump to on exit if necessary
int jumpaddr;		// Index for the jump (before pointer is resolved)
VMINT hookfunc;           // Optional script call
VMINT translucency;       // Translucency amount
} SEQ_POOL;

#define SEQFLAG_PINGPONG		0x00000001
#define SEQFLAG_LOOP			0x00000002
#define SEQFLAG_RANDOM			0x00000004
#define SEQFLAG_UNISYNC			0x00000005
#define SEQFLAG_ANIMCODE		0x00000007 // mask covering animation flags

#define SEQFLAG_STEPPED			0x00000010
#define SEQFLAG_JUMPTO			0x00000020
#define SEQFLAG_CHIMNEY			0x00000040
#define SEQFLAG_POSTOVERLAY		0x00000080
#define SEQFLAG_WALL			0x00000100
#define SEQFLAG_ASYNC			0x00000200


// Scripts

typedef struct PEM
	{
	char *name;
	char *file;
	char *code;
	VMINT codelen;
	VMINT size;
	char hidden; // local function
	char Class;
	} PEM;

// Stored data lookup items

typedef struct DT_ITEM
	{
	char *ks;
	long ki;
	char *is;
	VMINT ii;
	} DT_ITEM;

typedef struct DATATABLE
	{
	char *name;
	VMINT entries;

	char keytype;
	char listtype;

	DT_ITEM *list;
	DT_ITEM **unsorted;
	} DATATABLE;

typedef struct ST_ITEM
	{
	char *name;
	char *title;
	char *tag;
	char *data;
	} ST_ITEM;
	
//////////////////////////////////////////////////////////////////
//  Object Data Structures                                      //
//////////////////////////////////////////////////////////////////

#define IS_ON           0x00000001	// Active?
#define CAN_OPEN        0x00000002	// Can this obstruction be cleared?
#define IS_WINDOW       0x00000004	// Is it a window?
#define IS_SOLID        0x00000008	// Is it solid to people?
#define IS_FRAGILE      0x00000010	// Will it break if dropped?
#define IS_TRIGGER      0x00000020	// Is it a trigger?
#define IS_INVISIBLE    0x00000040	// Only appears in the editor?
#define IS_SEMIVISIBLE 	0x00000080	// Player can still see even though invisible
#define IS_FIXED        0x00000100	// Is it nailed to the ground?
#define IS_CONTAINER    0x00000200	// Is it a container?
#define IS_TRANSLUCENT  0x00000400	// Is it translucent?
#define IS_LARGE        0x00000800	// Bigger than one tile?
#define IS_SPIKEPROOF   0x00001000	// Doesn't set off triggers
#define CAN_WIELD       0x00002000	// Can it be wielded?
#define IS_REPEATSPIKE	0x00004000	// Is the trigger one-shot or repeats every turn?
//#define SPARE		0x00008000	
#define DOES_BLOCKLIGHT 0x00010000	// Does it block light?
#define IS_TABLETOP     0x00020000	// Can you drop things on it even though it's solid?
#define DID_INIT        0x00040000	// Have we run the INIT code?
//#define SPARE         0x00080000	
#define IS_PERSON       0x00100000	// Is it sentient?
#define IS_HORRIBLE     0x00200000	// Will the very sight of it upset NPCs?
#define IS_HORROR       0x00200000	// (As above)
#define IS_SHOCKING     0x00200000	// (As above)
#define IS_QUANTITY     0x00400000	// Is it stackable?
#define IS_BOAT         0x00800000	// Can it travel on water?
#define IS_WATER        0x00800000	// Is it water?
#define IS_SHADOW       0x01000000	// Is it just a dark patch?
#define IS_DECOR        0x02000000	// Is it a special decorative object?
#define IS_SYSTEM       0x04000000	// Hidden from player (markers triggers etc)
	
#define IS_NPCFLAG      0x80000000	// RESERVED

#define IS_INVISSHADOW  0x01000040	// Combination flag

//
//  Engine internal state flags (transient, not exposed, probably don't need to be saved either)
//

#define ENGINE_STEPUPDATED	0x00000001	// Was just animated (used by per-turn animation code)
#define ENGINE_DIDSETSEQUENCE	0x00000002	// Has setsequence just been used?
#define ENGINE_DIDUPDATE	0x00000004	// Have we just moved (avoids runaway)
#define ENGINE_DIDACTIVETILE	0x00000008	// Going to use this to improve lava handling for offscreen NPCs
#define ENGINE_DIDSPIKE		0x00000010	// Have we updated repeating spikes?

typedef struct FUNCS            // This is called when you...
{
char use[32];           // USE it
VMINT  ucache;
char *suse;             // This is a pointer to the string array, for PEscript

char talk[128];          // TALK to it
VMINT  tcache;
char *stalk;

char kill[32];          // KILL/BREAK it
VMINT  kcache;
char *skill;

char look[32];          // LOOK at it
VMINT  lcache;
char *slook;

char stand[32];         // STAND on it
VMINT  scache;
char *sstand;

char hurt[32];          // HURT/DAMAGE it
VMINT  hcache;
char *shurt;

char init[32];          // Called when object first created
VMINT  icache;
char *sinit;

// The ?cache number succeeding each entry is the index in the array of VRMs
// to the one that will be called, and is used for optimisation.

char contains[8][32];
VMINT contents;
// Up to 8 objects can be put in the object when it's first created

char resurrect[32];          // Object it becomes when resurrected
char *sresurrect;

char wield[32];          // Called when object is wielded/removed
VMINT  wcache;
char *swield;

char attack[32];          // Attack with it
VMINT  acache;
char *sattack;

char user1[32];
char *suser1;

char user2[32];
char *suser2;

char horror[32];          // NPC sees something truly awful
VMINT  hrcache;
char *shorror;

char quantity[32];          // Quantity is changed
VMINT  qcache;
char *squantity;

char get[32];          // Picked up
VMINT  gcache;
char *sget;

} FUNCS;

// Tiles

typedef struct TILE
{
char *name;
VMUINT flags;
SEQ_POOL *form;		// Direct access to animation
char seqname[32];	// Name of animation (in editor)
VMINT sptr;			// Animation counter
VMINT cost;			// movement cost
VMINT sdir,tick;		// animation info
VMINT sdx,sdy;		// Scroll delta x,y
VMINT sx,sy;			// scroll coordinates x,y
VMINT odx,ody;		// Original scroll delta
VMINT standfunc;		// Function to be called if stood
char *desc;			// description
SEQ_POOL **alternate;// Alternate tiles (for blanked corners) or NULL
} TILE;

// AI objects

typedef struct SCHEDULE
{
char active,ok,hour,minute;

char vrm[32];
VMINT call;
OBJECTID target;
} SCHEDULE;


typedef struct WIELD
{
struct OBJECT *head;
struct OBJECT *neck;
struct OBJECT *body;
struct OBJECT *legs;
struct OBJECT *feet;
struct OBJECT *arms;
struct OBJECT *l_hand;
struct OBJECT *r_hand;
struct OBJECT *l_finger;
struct OBJECT *r_finger;
struct OBJECT *spare1;
struct OBJECT *spare2;
struct OBJECT *spare3;
struct OBJECT *spare4;
} WIELD;

enum	{
	WIELD_HEAD,
	WIELD_NECK,
	WIELD_BODY,
	WIELD_LEGS,
	WIELD_FEET,
	WIELD_ARMS,
	WIELD_LHAND,
	WIELD_RHAND,
	WIELD_LFINGER,
	WIELD_RFINGER,
	WIELD_SPARE1,
	WIELD_SPARE2,
	WIELD_SPARE3,
	WIELD_SPARE4,
	
	WIELDMAX};



// Moving object entity

typedef struct OBJECT
{
char *name;
char uid[41];
VMUINT flags;
VMUINT engineflags;
VMINT w,h,mw,mh;
VMINT x,y,z;
char *personalname;
SCHEDULE *schedule;
SEQ_POOL *form;
VMINT sptr,SPARE,sdir; // animation info
STATS *maxstats;    // Statistics structure
STATS *stats;       // Current stats
FUNCS *funcs;       // VRM Function table
VMINT curdir;
short dir[4];       // u,d,l,r
char *desc;         // Examine Description
char *shortdesc;    // Short Description
OBJECTID target; // Current target object (behaviour goal, if any)
VMINT   tag;          // Tags are used for triggering things
VMINT   activity;     // Current activity VRM
struct USEDATA *user;      // User data for a game in progress
unsigned char hblock[4]; // xywh
unsigned char vblock[4]; // xywh
unsigned char harea[4]; // xywh
unsigned char varea[4]; // xywh
VMINT hotx,hoty;          // attack hotspot
unsigned int save_id;     // Assigned object number (during saving and loading)
VMINT light;        // Light it casts
VMINT cost;	    // movement cost (for barriers and traps)
CHAR_LABELS *labels;    // Tags and labels for the object that don't change.
OBJECTID enemy;
//struct OBJECT **wield;	// Christ, Joe, what were you thinking!?
struct WIELD *wield;
OBJECTID pocket;
struct OBJECT *next;
OBJECTID parent;
} OBJECT;


typedef struct OBJREGS
{
OBJECT *player;
OBJECT *current;
OBJECT *me;
OBJECT *victim;
} OBJREGS;


#define BLK_X 0
#define BLK_Y 1
#define BLK_W 2
#define BLK_H 3

// World structure

typedef struct WORLD
{
unsigned int sig;			// Signature
int w,h;
unsigned short *physmap;	// Physical world
OBJECT *object;				// Linked list of objects
unsigned char *roof;		// Rooftop
OBJECT **objmap;			// Object Map
unsigned char *light;		// Static Lighting
unsigned char *lightst;
int con_n,con_s,con_e,con_w;// map connections
int *temp[25];
} WORLD;

typedef struct OBJLIST
{
OBJECT *ptr;
struct OBJLIST *next;
} OBJLIST;

typedef struct PEVM
	{
	OBJECT *reg[4];		// Local object registers
	char *name;			// Name of function (for debugging)
	char *file;			// Filename of function (for debugging)
	VMTYPE *code;	// code and data
	int size;			// length of code and data
	int codelen;		// length of code and data
	VMTYPE *ip;	// instruction pointer
	struct PEVM *next;	// for use as a linked-list (vmstack etc)
	} PEVM;

// Tile Linker for automatic map structure generation

typedef struct TL_DATA
	{
	VMINT tile;
	VMINT becomes;
	} TL_DATA;

typedef struct TILELINK
	{
	char *name;
	VMINT tile;
	TL_DATA *n;
	TL_DATA *s;
	TL_DATA *e;
	TL_DATA *w;
	TL_DATA *ne;
	TL_DATA *nw;
	TL_DATA *se;
	TL_DATA *sw;
	VMINT nt;
	VMINT st;
	VMINT et;
	VMINT wt;
	VMINT net;
	VMINT nwt;
	VMINT set;
	VMINT swt;
	} TILELINK;

typedef struct USERSTRING
{
VMINT len;
char *ptr;
} USERSTRING;


// Externals

extern OBJLIST *MasterList;

extern int gamewinsize;
extern IREBITMAP *gamewin;
extern IREBITMAP *roofwin;
extern int VSW; // Tiles
extern int VSH;
extern int VSA;
extern int VSMIDX;
extern int VSMIDY;
extern int VSW32; // Pixels
extern int VSH32;
extern int VSA32;

extern char imgcachedir[];
#endif

