/*
 *      Object manipulation routines, world management etc
 */

#include <string.h>
#include <assert.h>

#include "ithelib.h"
#include "core.hpp"
#include "linklist.hpp"
#include "console.hpp"
#include "oscli.hpp"
#include "resource.hpp"
#include "pe/pe_api.hpp"
#include "object.hpp"
#include "init.hpp"
#include "gamedata.hpp"
#include "map.hpp"
#include "project.hpp"	// Fastrandom is in here

// defines

//#define DEBUG_SAVE
#define TOO_MUCH 1000

#define CHECK_FOR_MOVE_UP_ARSE(X,Y); if(X==Y)\
    {\
    Bug("Object '%s' tried to vanish up it's own arse\n",Y->name);\
    return;\
    }

#define CHECK_FOR_PLAYER_GETTING_LOST(X); if(X == player)\
    {\
    return;\
    }

// Structure definition for Locations
struct LOCNAME
	{
	char name[32];
	LOCNAME *next;
	} *LocList=NULL;

// variables

static char str[90];
char pending_delete=0;
char AL_dirty=0;
static int object_search_rec=0;
OBJECT *moveobject_blockage;

extern char in_editor;
extern char *lastvrmcall;
extern char debcurfunc[];

// functions

extern void CallVM(char *function);
extern void wipe_refs();
extern void ResumeSchedule(OBJECT *o);
extern int getYN(char *q);

int AddToParty(OBJECT *new_member);
TILE *GetTile(int x,int y);
int isSolid(int x,int y);
int ChooseLeader(int x);
void SubFromParty(OBJECT *member);
void gen_largemap();

void AddQuantity(OBJECT *container,char *type,int request);
int TakeQuantity(OBJECT *container,char *type,int request);
int MoveQuantity(OBJECT *src,OBJECT *dest,char *objectname,int qty);
void Qsync(OBJECT *temp);
void SetOwnerInContainer(OBJECT *obj);

int inline IsHidden(OBJECT *o);

/* Linked-list world functions */

void MoveToPocket(OBJECT *object, OBJECT *container);
void TransferToPocket(OBJECT *object, OBJECT *container);
int MoveFromPocket(OBJECT *object, OBJECT *container,int x,int y);

/* Matrix world functions */

int MoveToMap(int x, int y, OBJECT *object);
void TakeObject(int x,int y, OBJECT *container);
int MoveObject(OBJECT *object,int x,int y, int pushed);
void TransferObject(OBJECT *object,int x,int y);
OBJECT *GetObject(int x,int y);
OBJECT *GetObjectBase(int x,int y);
OBJECT *GetRawObjectBase(int x,int y);
OBJECT *GetSolidObject(int x,int y);
void ForceDropObject(int x,int y, OBJECT *object);
void FindEmptySquare(int *x,int *y);
void CreateContents(OBJECT *obj);
void MoveToTop(OBJECT *object);
void UpdatePocketCoords(OBJECT *o, int x, int y);

/* Other */

int MoveTurnObject_Core(OBJECT *objsel,OBJECT *objdst,int dx, int dy);
static int WeighObjectR(OBJECT *list);
static int GetBulkR(OBJECT *list);
static int LookInLimboFor(OBJECT *obj);
static void LookForObject(OBJECT *obj);
static void IfTriggered(OBJECT *spikes);
static OBJECT *IsTrigger(int x,int y);

OBJECT *GetFirstObject(OBJECT *cont, char *name);
OBJECT *GetFirstObjectFuzzy(OBJECT *cont, char *name);
int SumObjects(OBJECT *cont, char *name, int total);
void CheckRefs(OBJECT *o);
extern char *BestName(OBJECT *o);


// code


/*
 *    MoveToPocket()  -  Move an object into someone's pocket
 */

void MoveToPocket(OBJECT *object, OBJECT *container)
{
// Check for silliness.  This error has never happened to me, but if it
// does I thought that you should know.

if(!object || !container)
    {
    Bug("MoveToPocket(%x,%x) attempted\n",object,container);
    return;
    }

if(!(container->flags & IS_ON))   // NOOO!
    return;


CHECK_FOR_MOVE_UP_ARSE(container,object);
CHECK_FOR_PLAYER_GETTING_LOST(object);

TransferToPocket(object,container);
}

/*
 *    TransferToPocket()  -  Force an object into someone's pocket
 */

void TransferToPocket(OBJECT *object, OBJECT *container)
{
OBJECT *temp;
int ox,oy;

// Check for silliness.  This one DID occur, and we didn't check.  Now we do.
if(!object || !container)
	{
	Bug("TransferToPocket(%x,%x) attempted\n",object,container);
	return;
	}

if(object->flags & IS_DECOR)   // NO! BAD! NO DECORATIVES FOR YOU!
	{
	Bug("Attempted to move decorative to pocket\n");
	return;
	}

// Look for the object in nowhere land?

for(temp=curmap->object;temp->next;temp=temp->next)
	{
	if(temp->next==object)
		{
		// Good we found it.  We are looking at the entry before it.
		object->parent.objptr = container;	// I'm inside this object
		temp->next = object->next;         	// object no longer in list
		object->next = container->pocket.objptr;  // Get of pocket->next
		container->pocket.objptr = object;        // Object now in pocket
		SetOwnerInContainer(object);
		return;                            	// Done
		}
	}

// Is it in a pocket?
temp=object->parent.objptr;

// No?  It should be on the ground then
if(!temp)
	{
	ox = object->x;
	oy = object->y;
	TransferObject(object,0,0);
	TakeObject(0,0,container);
	object->x = ox;
	object->y = oy;
	SetOwnerInContainer(object);
	return;
	}

// It is (or claims to be) in someone's pocket

UnWield(container,object);		// Make sure it isn't wielded

LL_Remove(&temp->pocket.objptr,object);
object->next = container->pocket.objptr;	// Get of pocket->next
container->pocket.objptr = object;     	// Object now in pocket
object->parent.objptr=container;
SetOwnerInContainer(object);
return;
}


/*
 *    MoveFromPocket()  -  Move an object out of someone's pocket
 */

int MoveFromPocket(OBJECT *object, OBJECT *container, int x,int y)
{
OBJECT *temp;
// Check for silliness

if(!object || !container)
	return 0;
if(!container->pocket.objptr)
	return 0;

if(!(container->flags & IS_ON))   // NOOO!
    return 0;

// If there is something solid there, abort, unless it's a container
if(isSolid(x,y))
	{
	temp=GetSolidObject(x,y);   // Find the obstruction
	if(!temp)                   // None? Must be a tile then
		{
		return 1;
		}

	// If it isn't a table, and it isn't a chest, you can't put it there
	if(!(temp->flags & IS_TABLETOP))
		if(!(temp->flags & IS_CONTAINER))
			return 0;
	}

// Go through to be sure it is there.

for(temp = container->pocket.objptr;temp;temp=temp->next)
	if(temp == object)
		{
		LL_Remove(&container->pocket.objptr,object);
		DropObject(x,y,object);                     // Drop into the map
		return 1;
		}

return 0;
}

/*
 *    ForceFromPocket()  -  MoveFromPocket without checking for obstructions
 */

int ForceFromPocket(OBJECT *object, OBJECT *container, int x,int y)
{
OBJECT *temp;

// These checks are to prevent a crash

if(!object || !container)
	{
	return 0;
	}
if(!container->pocket.objptr)
	{
	return 0;
	}

if(!(container->flags & IS_ON))   // NOOO!
	{
	Bug("ForceFromPocket: Container is Dead!!!\n");
	return 0;
	}

for(temp = container->pocket.objptr;temp;temp=temp->next)
	if(temp == object)
		{
		LL_Remove(&container->pocket.objptr,object);
		ForceDropObject(x,y,object);                     // Drop into the map
		return 1;
		}

return 0;
}

/*
 *    MoveToMap(int x,int y,OBJECT *object)  -  Move object to the world
 */

int MoveToMap(int x, int y, OBJECT *object)
{
OBJECT *temp;

// Check for silliness.

if(!object)
	return 0;

for(temp = curmap->object;temp;temp=temp->next)
	if(temp == object)
		{
		LL_Remove(&curmap->object,object);
		ForceDropObject(x,y,object);                     // Drop into the map
		object->parent.objptr=NULL; // Make sure
		return 1;
		}
return 0;
}

/*
 *    DropObject(int x,int y,OBJECT *object)  -  Drop object at x,y
 */

int DropObject(int x,int y, OBJECT *object)
{
OBJECT *temp;
OBJECT *anchor;
int sx=0,sy=0,id;

object->next=NULL; // It will be put onto the end of the list

if(x<0 || x>=curmap->w || y<0 || y>=curmap->h)
	{
	// Terminate melodramatically
	//   sprintf(str,"DropObject(%d,%d,%s) Attempted to put outside map boundary",x,y,object->name);
	//   panic(cfa,str,"Ai! ai!  A Balrog!  A Balrog is come!  'Tis Durin's Bane!");
	return 0;
	}

UnWield(object->parent.objptr,object);	// Make sure it isn't wielded

// Find the list we want to modify

anchor=GetObjectBase(x,y);
//anchor=curmap.objmap[ytab[y]+x];

// If there's a bag, drop it inside
temp=anchor;
if(temp)
	for(;temp;temp=temp->next)
		if(temp->flags & IS_CONTAINER)
			{
			FindEmptySquare(&sx,&sy);           // Find scratch space
			curmap->objmap[MAP_POS(sx,sy)]=object;  // Dump the object there
			TakeObject(sx,sy,temp);             // Shift it into the bag
			return 1;
			}

// Modify the object's X,Y coords to sync with the matrix

object->x=x;
object->y=y;

id = getnum4char(object->name);
if(id != -1)
	{
	object->flags &= ~IS_FIXED;
	object->flags |= (CHlist[id].flags&IS_FIXED);
	}
object->parent.objptr = NULL;
if(isDecor(x,y))
	LL_Add2(&curmap->objmap[MAP_POS(x,y)],object);
else
	{
	LL_Add(&curmap->objmap[MAP_POS(x,y)],object);

	// Are we treading on anyone's toes?
//	temp=GetRawObjectBase(x,y); // GetObjectBase is better but slower
	temp=IsTrigger(x,y);
	IfTriggered(temp);
	}

gen_largemap();
return 1;
}


/*
 *    ForceDropObject(int x,int y,OBJECT *object)  -  Drop object at x,y
 */

void ForceDropObject(int x,int y, OBJECT *object)
{
int id;
OBJECT *temp;

if(!object)
	return;

if(x<0 || x>=curmap->w || y<0 || y>=curmap->h)
	return;

UnWield(object->parent.objptr,object);	// Make sure it isn't wielded

// Modify the object's X,Y coords to sync with the matrix

object->x=x;
object->y=y;

id = getnum4char(object->name);
if(id != -1)
	{
	object->flags &= ~IS_FIXED;
	object->flags |= (CHlist[id].flags&IS_FIXED);
	}
object->parent.objptr = NULL;

if(isDecor(x,y))
	{
	if(object->flags & IS_DECOR)
		Bug("Can't have two decor objects on one square at %d,%d\n",x,y);
	else
		LL_Add2(&curmap->objmap[MAP_POS(x,y)],object);
	}
else
	{
	LL_Add(&curmap->objmap[MAP_POS(x,y)],object);
	// Are we treading on anyone's toes?
//	temp=GetRawObjectBase(x,y); // GetObjectBase is better but slower
	temp=IsTrigger(x,y);
	IfTriggered(temp);
	}
gen_largemap();
}


/*
 *    TakeObject(int x,int y,OBJECT *container)  -  take object from x,y into
 */

void TakeObject(int x,int y, OBJECT *container)
{
OBJECT *temp;

if(!container)
	return;

if(x<0 || x>=curmap->w || y<0 || y>=curmap->h)
	{
	// Terminate melodramatically
	sprintf(str,"TakeObject(%d,%d) Attempted to get outside map boundary",x,y);
	ithe_panic(str,"Ai! ai!  A Balrog!  A Balrog is come!  'Tis Durin's Bane!");
	}

// Find the list we want to modify

temp=GetObject(x,y);
if(!temp)
	return;

//CHECK_FOR_MOVE_UP_ARSE(container,temp);
//CHECK_FOR_PLAYER_GETTING_LOST(temp);

LL_Remove(&curmap->objmap[MAP_POS(x,y)],temp);

temp->next=container->pocket.objptr;
container->pocket.objptr=temp;

temp->flags |= IS_FIXED;            // Object sleeps while in pocket
temp->parent.objptr = container;

gen_largemap();
}

/*
 *    MoveObject(OBJECT *object, int x,int y, int push) -  move object to x,y
 *    Performs error checking and some game functions, calls TransferObject
 */


int MoveObject(OBJECT *object,int x,int y, int pushed)
{
OBJECT *temp;
TILE *tile;
int ox,oy,blockx,blocky,blockw,blockh;
int sx=0,sy=0;
char allow=1,swap=0;

// Prevent the impossible

if(!object)
	return 0;

moveobject_blockage=NULL; // Nothing got in the way

if(object->x == x)
	if(object->y == y)
		return 1;     // Oh yes, we've done it, honest ;-)

if(x<0 || x>=curmap->w || y<0 || y>=curmap->h)
	return 0; // Out of bounds

if(object->flags & IS_FIXED) // Can't move
	return 0;

if(object->flags & IS_SYSTEM) // Can't move
	return 0;

// Do we want to use speed restriction?

if(!pushed)
	{
	// Yes, impose a movement delay
	if(object->maxstats->speed)
		{
		object->stats->speed+=object->maxstats->speed;
		if(object->stats->speed > 100)
			object->stats->speed-=100;
		else
			{
			// Abort the movement
			return 0;
			}
		}
	}

// If it's a large object, check all of it, but allow if it's only
// hitting itself.

if(object->flags & IS_LARGE)
	{
	allow=0;
	if(object->curdir > CHAR_D) //(U,D,L,R)
		{
		blockx=object->hblock[BLK_X];
		blocky=object->hblock[BLK_Y];
		blockw=object->hblock[BLK_W];
		blockh=object->hblock[BLK_H];
		}
	else
		{
		blockx=object->vblock[BLK_X];
		blocky=object->vblock[BLK_Y];
		blockw=object->vblock[BLK_W];
		blockh=object->vblock[BLK_H];
		}

	for(sy=0;sy<blockh;sy++)
		for(sx=0;sx<blockw;sx++)
			{
			if(isSolid(sx+x+blockx,sy+y+blocky))
				{
//				ilog_printf("Block at %d,%d\n",sx+x+blockx,sy+y+blocky);
				temp=GetSolidObject(sx+x+blockx,sy+y+blocky);
				tile=GetTile(sx+x+blockx,sy+y+blocky);
				if(temp)
					{
					if(temp != object)
						{
//						ilog_printf("%s Blocked by %s at %d,%d\n",object->name,temp->name,sx+x+blockx,sy+y+blocky);
//						ilog_printf("x,y = %d,%d, w,h = %d,%d\n",blockx,blocky,blockw,blockh);
						moveobject_blockage=temp; // Gotcha
						return 0;
						}
					}
				else
					{
					// GetSolidObject won't work on decoratives.
					// This will, though:
					temp=GetSolidMap(sx+x+blockx,sy+y+blocky);
					if(temp && temp != object)
						{
						moveobject_blockage=temp; // Gotcha
						return 0;
						}
					}

				// If there is a boat over water, allow anything to rest on it
				// Also drives the shore and bridges
				if(tile->flags & IS_WATER)
					if(IsBridge(sx+x+blockx,sy+y+blocky))
						{
//						ilog_printf("Bridge allow\n");
						allow=1;
						}


				if(tile->flags & IS_SOLID && !allow)
					{
//					ilog_printf("%s Blocked by tile %s at %d,%d\n",object->name,tile->name,sx+x+blockx,sy+y+blocky);
					return 0;
					}
				// Looks like an illusion caused by the LargeMap
				allow=1;
				}
			}
	if(allow)
		{
//		ilog_printf("NEWCODE: allowing move of '%s'\n",object->name);
		TransferObject(object,x,y);     // Allow the move anyway
		return 1;
		}
	}

allow=1;

// If there is something in the way, abort.
// If it turns out to be two party members, swap them over

if(isSolid(x,y))
	{
	tile=GetTile(x,y);
	temp=GetSolidObject(x,y);   // Find the obstruction

	moveobject_blockage=temp;   // This looks like the culprit
	allow=0;

	if(temp == object)                  // The object has hit itself..
		{
		TransferObject(object,x,y);     // Allow the move anyway
		moveobject_blockage=NULL;       // disregard the blockage
		return 1;
		}

	// It is legal for unliving objects and boats to enter water

	if((tile->flags & IS_WATER) && !temp)
		if(!(object->flags & IS_PERSON))       // Live people won't swim
			allow=1;

	if(temp)                    // Before we touch the flags, sanity check
		{
		// If there is a boat over water, allow anything to rest on it
		// Also drives the shore and bridges

		if(tile->flags & IS_WATER && temp->flags & IS_WATER)
			allow=1;

		// If there is a party/leader conflict, swap the party members
		swap=0;

		// The player wants to stand where a member is
		if(GetNPCFlag(temp,IN_PARTY))
			if(object==player)
				swap=1;

		// The obstruction is in the same party as me.
		// If the obstruction is a follower and I'm not (i.e. leader)
		// they get out of my way

		if(temp->labels->party != NOTHING)
			if(!istricmp(temp->labels->party,object->labels->party))  // Brethren
				{
				if(!istricmp(temp->labels->rank,"follower") && istricmp(object->labels->rank,"follower")) // leader
					swap=1;
				// Otherwise, if both same faction and rank, randomise it to prevent logjams
				if(!istricmp(temp->labels->rank,object->labels->rank))
					if((fastrandom()&3) > 1)
						swap=1;
				}



		// If we need to swap the members, do it

		if(swap)
			{
			// Store the leader's old coordinates for the follower
			ox=object->x;
			oy=object->y;
			// Find a square we can move the leaders temporarily
			FindEmptySquare(&sx,&sy);
			// Got one.   Swap the two party members
			TransferObject(object,sx,sy);
			TransferObject(temp,ox,oy);
			TransferObject(object,x,y);
			// The two party members are reversed now
			moveobject_blockage=NULL;       // disregard the blockage
				return 1;
			}

		if(temp->flags & IS_TABLETOP)            // Pushing something into a table
			{
			if(pushed)                          // If it's an actual push
				allow=1;                        // let them on the table
			else                                // Otherwise, prevent people
				if(!(object->flags & IS_PERSON))       // from walking randomly on the table
					if(object != player)        // (player not always a person.. can be boat etc)
						allow=1;
			}

		if(temp->flags & IS_CONTAINER)
			if(!(object->flags & IS_SPIKEPROOF) && !(object->flags & IS_PERSON))
				{
//				printf("Moved %s into %s\n",object->name,temp->name);
				MoveToPocket(object,temp);
				moveobject_blockage=NULL;       // disregard the blockage
				return 1;
				}
		}
	// There wasn't a solid object in that square, but is there a boat?
	else
		{
		temp=GetObjectBase(x,y);
		for(;temp;temp=temp->next)
			if(temp->flags & IS_WATER)
				{
				// Found a boat
				if(tile->flags & IS_WATER) // It is on water
					{
					allow=1;
					break;
					}

				// If this happens, we found a boat but it wasn't on water.
				// At present there aren't any other mitigating factors so we
				// might as well forget the whole idea and carry on.
				temp=NULL;
				break;
				}
		}
	// Blocked
	}

// If we're about to move into a bag (the player must not)

if(!(object->flags & IS_PERSON) && !(object->flags&IS_SPIKEPROOF))
	{
	temp=GetObject(x,y);
	if(temp)
		if(temp->flags & IS_CONTAINER)
			{
			MoveToPocket(object,temp);
			moveobject_blockage=NULL;       // disregard any blockage
			return 1;
			}
	}

if(allow)
	{
	TransferObject(object,x,y);
	moveobject_blockage=NULL;       // disregard any blockage
	}

return allow;
}


/*
 *    TransferObject(OBJECT *object, int x,int y)  -  move object to x,y
 *                                                    without solidity checks
 */

void TransferObject(OBJECT *object,int x,int y)
{
TILE *srctile,*desttile;
OBJECT *temp,*dest,*src;
char destwater,srcwater;
int id,decor;

if(!object)
	return;

if(object->flags & IS_DECOR)
	return;

if(!(object->flags & IS_ON))
	Bug("TransferObject: FYI, %s (%x) is Dead\n",object->name,object);

// Check to prevent object disappearing up it's own arse
if(object->x == x)
	if(object->y == y)
		if(!object->parent.objptr)	// If it is inside something, we want to move it outside.  Otherwise, abort as mentioned
			return;

// silently ignore object request to leave the map

if(x<0 || x>=curmap->w || y<0 || y>=curmap->h)
	return;

// Is it in someone's pocket? (i.e. not in the Real World)
if(object->parent.objptr)
	if(!ForceFromPocket(object,object->parent.objptr,0,0))
		{
		// Is it newly-created?
		if(!MoveToMap(0,0,object))
			{
			LookForObject(object);
			getYN("press Y to quit");
			sprintf(str,"move_object(%s,%ld,%ld) Object moved without using move_object",object->name,object->x,object->y);
			ithe_panic(str,"Fly!  This is a foe beyond any of you.  I must hold the narrow way.  Fly!");
//			DestroyObject();
			return;
			}
		}

// Get tile information before object moves
srctile = GetTile(object->x,object->y);
desttile = GetTile(x,y);
src = GetObjectBase(object->x,object->y);
dest = GetObjectBase(x,y);

// Check for a decor object already at the destination
decor=0;
if(dest)
	{
	for(temp=dest;temp->next;temp=temp->next); // decor WILL BE last object
	if(temp->flags & IS_DECOR)
		decor=1;
	}

srcwater=0; destwater=0;

// Remove the object from the appropriate linked list in curmap.
LL_Remove(&curmap->objmap[MAP_POS(object->x,object->y)],object);

// Keep the object's internal data consistent
object->x=x;       // Very important!
object->y=y;       // Very important!

UpdatePocketCoords(object,x,y); // Update coordinates of all objects inside

// Update the animation frame, if it's stepped
object->flags |= DID_STEPUPDATE;

// Add to the target list
if(decor)
	LL_Add2(&curmap->objmap[MAP_POS(x,y)],object);
else
	{
	LL_Add(&curmap->objmap[MAP_POS(x,y)],object);
	}

// Rebuild Large Objects table
gen_largemap();

// If the last tile was water, then say so.
if(srctile->flags & IS_WATER)
	srcwater=1;
 // If the object was resting on a boat, then say it wasn't on water
for(;src;src=src->next)
	if(src->flags & IS_WATER)
		if(src != object)       // Don't count the boat we're moving
			srcwater=0;

// If the new tile is water, then say so.
if(desttile->flags & IS_WATER)
	if(!IsBridge(x,y))	// No bridge?
		destwater=1;
 // If the object is going to rest on a boat, then say it isn't water
for(;dest;dest=dest->next)
	if(dest->flags & IS_WATER)
		destwater=0;

// If the object was in water, take away the shadow effect where necessary
if(srcwater && !destwater)
	if(object->flags & IS_SHADOW)
		{
		id = getnum4char(object->name);
		if(id == -1)
			object->flags &= ~IS_SHADOW;   // educated guess
		else
			{
			object->flags &= ~IS_SHADOW;
			object->flags |= (CHlist[id].flags & IS_SHADOW);
			}
		}

// If the object is actually going into the water, do things

if(!srcwater && destwater)
	if(!(object->flags & IS_SPIKEPROOF))
		{
		if(!(object->flags & IS_SPIKEPROOF))
			object->flags |= IS_SHADOW; // If it can't float, it sinks and becomes a shadow

		temp = current_object;
		current_object = object;
		CallVMnum(Sysfunc_splash);
		current_object = temp;
		}

// Are we treading on anyone's toes?
//temp=GetRawObjectBase(x,y); // GetObjectBase is better but slower
temp=IsTrigger(x,y);
IfTriggered(temp);

return;
}

/*
 *    TransferDecor(int x1,int y1,int x2,int y2)  -  move object from 1 to 2
 */

void TransferDecor(int x1, int y1, int x2, int y2)
{
int decor;
OBJECT *src,*dest,*temp;

if(x1<0 || x1>=curmap->w || y1<0 || y1>=curmap->h)
	return;
if(x2<0 || x2>=curmap->w || y2<0 || y2>=curmap->h)
	return;

src = GetObjectBase(x1,y1);
dest = GetObjectBase(x2,y2);

if(!src)
	return; // quietly decline

// Is there already a decor at the destination?
decor=0;
if(dest)
	{
	for(temp=dest;temp->next;temp=temp->next); // decor WILL BE last object
	if(temp->flags & IS_DECOR)
		decor=1;
	}

// Find the decor in the source (temp will point to it)

for(temp=src;temp->next;temp=temp->next);
	if(!(temp->flags & IS_DECOR))
		return; // no decor to move

// Remove the object from the source list
LL_Remove(&curmap->objmap[MAP_POS(x1,y1)],temp);

// Add it to the target list
if(decor)
	LL_Add2(&curmap->objmap[MAP_POS(x2,y2)],temp);
else
	LL_Add(&curmap->objmap[MAP_POS(x2,y2)],temp);

// Rebuild Large Objects table
gen_largemap();

return;
}


/*
 *    MoveTurnObject()  - Move and object and change direction at same time
 */

int MoveTurnObject(OBJECT *objsel,OBJECT *objdst,int dx, int dy)
{
int olddir;

olddir=objsel->curdir;  // In case we need to revert

// Is it a diagonal?
if(dx && dy)
	{
	// Find the best direction without changing polarity
	if(olddir == CHAR_U && dy > 0)
		OB_SetDir(objsel,CHAR_D,FORCE_FRAME);
	if(olddir == CHAR_D && dy < 0)
		OB_SetDir(objsel,CHAR_U,FORCE_FRAME);
	if(olddir == CHAR_L && dx > 0)
		OB_SetDir(objsel,CHAR_R,FORCE_FRAME);
	if(olddir == CHAR_R && dx < 0)
		OB_SetDir(objsel,CHAR_L,FORCE_FRAME);
	
	// Anyway, it's diagonal.  Does it work straight off?
	if(MoveObject(objsel,objsel->x+dx,objsel->y+dy,0))
		return 1;

	// No, try only doing the vertical?
	if(MoveObject(objsel,objsel->x,objsel->y+dy,0))
		return 1;
	// How about just the horizontal?
	if(MoveObject(objsel,objsel->x+dx,objsel->y,0))
		return 1;
	}

// Damn, try using the complex method instead
return(MoveTurnObject_Core(objsel,objdst,dx,dy)); // Use the old way
}

/*
 * The guts of the system, rather baroque
 */

int MoveTurnObject_Core(OBJECT *objsel,OBJECT *objdst,int dx, int dy)
{
static int looplock=0;
int ow,oh,osx,osy,ret,dir,odx,ody;
int olddir;
//char *dirn[]={"Up","Down","Left","Right"};

if(looplock>2)
	return 0;
looplock++;

olddir=objsel->curdir;  // In case we need to revert
ow = objsel->mw;
oh = objsel->mh;
odx=dx;
ody=dy;

// Work out core direction
dir = CHAR_U;

// Most complex first
if(dx > 0 && dx > dy)
	dir = CHAR_R;
if(dx < 0 && dx < dy)
	dir = CHAR_L;
if(dy > 0 && dy >= dx)
	dir = CHAR_D;
if(dy < 0 && dy <= dx)
	dir = CHAR_U;

// Simple cases override complex ones
if(!dx && dy > 0)
	dir = CHAR_D;
if(!dx && dy < 0)
	dir = CHAR_U;
if(!dy && dx > 0)
	dir = CHAR_R;
if(!dy && dx < 0)
	dir = CHAR_L;

moveobject_blockage=NULL;

// Try just changing direction
OB_SetDir(objsel,dir,FORCE_FRAME);
if(MoveObject(objsel,objsel->x+dx,objsel->y+dy,0)) // Not pushed!
	{
	looplock--;
	return 1;
	}

if(!objdst)
	{
	looplock--;
	return 0;
	}

//irecon_printf("Input: %d,%d\n",dx,dy);

// Try changing direction
if(olddir == CHAR_D || olddir == CHAR_U)
	{
	if(objdst->x > objsel->x)
		dir = CHAR_R;
	if(objdst->x < objsel->x)
		dir = CHAR_L;
	}
else
	{
	if(objdst->y > objsel->y)
		dir = CHAR_D;
	if(objdst->y < objsel->y)
		dir = CHAR_U;
	}

ret=0;

// If it's a large object, find out if the new shape would actually fit
if(objsel->flags & IS_LARGE)
	{
	// Figure out where it should go, to help large objects turning
	osx = osy = 0;
	osy = oh - ow;
//	osx = ow - oh;
/*
//	if(olddir < CHAR_L && dir == CHAR_R)
		osy = oh - ow;
//	if(olddir > CHAR_D && dir == CHAR_U)
		osx = ow - oh;
*/

/*
	if(olddir == CHAR_R && dir == CHAR_D)
		{
		osx = ow - oh;
		}

	if(olddir == CHAR_L && dir == CHAR_D)
		{
		// Do nothing
		}

	if(olddir == CHAR_R && dir == CHAR_U)
		{
		osy = -(oh - ow);
		osx = ow - oh;
		}

	if(olddir == CHAR_L && dir == CHAR_U)
		{
		osy = -(oh - ow);
		}
*/
	// Offset is only required for negative movements
	// Push it into place
	OB_SetDir(objsel,dir,FORCE_SHAPE|FORCE_FRAME);
//	irecon_printf("Want %s: Trying offset %d,%d\n",dirn[dir],osx,osy);
	ret = MoveObject(objsel,objsel->x+osx,objsel->y+osy,0);

	if(!ret)
		{
		// Shit, it didn't work.
		OB_SetDir(objsel,olddir,FORCE_SHAPE|FORCE_FRAME);

		// Try moving in a 'refined' direction
		switch(olddir)
			{
			case CHAR_U:
			dx=0;
			dy=-1;
			break;

			case CHAR_D:
			dx=0;
			dy=1;
			break;

			case CHAR_L:
			dx=-1;
			dy=0;
			break;

			case CHAR_R:
			dx=1;
			dy=0;
			break;
			};

//		irecon_printf("desparate try: offset %d,%d\n",dx,dy);
		if(!MoveObject(objsel,objsel->x+dx,objsel->y+dy,0))
			{
			dx=dy=0;
			if(olddir == CHAR_L || olddir == CHAR_R)
				dy=(qrand()%3)-1;
			else
				dx=(qrand()%3)-1;
//			irecon_printf("final chance: random offset %d,%d\n",dx,dy);
//			irecon_printf("recursing:\n");
			ret=MoveTurnObject_Core(objsel,objdst,dx,dy);
			looplock--;
			return ret;

//			return MoveObject(objsel,objsel->x+dx,objsel->y+dy,0);
			}

		looplock--;
		return 0;
		}
	}

looplock--;
return ret;
}


int MoveTurnObject_old(OBJECT *objsel,OBJECT *objdst,int dx, int dy)
{
int ow,oh,osx,osy,ret,dir;
int olddir;
char *dirn[]={"Up","Down","Left","Right"};

olddir=objsel->curdir;  // In case we need to revert
ow = objsel->mw;
oh = objsel->mh;

// Work out core direction
dir = CHAR_U;

// Most complex first
if(dx > 0 && dx > dy)
	dir = CHAR_R;
if(dx < 0 && dx < dy)
	dir = CHAR_L;
if(dy > 0 && dy >= dx)
	dir = CHAR_D;
if(dy < 0 && dy <= dx)
	dir = CHAR_U;

// Simple cases override complex ones
if(!dx && dy > 0)
	dir = CHAR_D;
if(!dx && dy < 0)
	dir = CHAR_U;
if(!dy && dx > 0)
	dir = CHAR_R;
if(!dy && dx < 0)
	dir = CHAR_L;

moveobject_blockage=NULL;

// Try just changing direction
OB_SetDir(objsel,dir,FORCE_FRAME);
if(MoveObject(objsel,objsel->x+dx,objsel->y+dy,0)) // Not pushed!
	return 1;

if(!objdst)
	return 0;

irecon_printf("Input: %d,%d\n",dx,dy);

// Try changing direction
if(olddir == CHAR_D || olddir == CHAR_U)
	{
	if(objdst->x > objsel->x)
		dir = CHAR_R;
	if(objdst->x < objsel->x)
		dir = CHAR_L;
	}
else
	{
	if(objdst->y > objsel->y)
		dir = CHAR_D;
	if(objdst->y < objsel->y)
		dir = CHAR_U;
	}

ret=0;

// If it's a large object, find out if the new shape would actually fit
if(objsel->flags & IS_LARGE)
	{
	// Figure out where it should go, to help large objects turning
	osx = osy = 0;
//	if(olddir < CHAR_L && dir == CHAR_R)
		osy = oh - ow;
//	if(olddir > CHAR_D && dir == CHAR_U)
//		osy = oh - ow;

	// Offset is only required for negative movements
	// Push it into place
	OB_SetDir(objsel,dir,FORCE_SHAPE|FORCE_FRAME);
	irecon_printf("Want %s: Trying offset %d,%d\n",dirn[dir],osx,osy);
	ret = MoveObject(objsel,objsel->x+osx,objsel->y+osy,0);

	if(!ret)
		{
		// Shit, it didn't work.
		OB_SetDir(objsel,olddir,FORCE_SHAPE|FORCE_FRAME);

		// One last try
		switch(olddir)
			{
			case CHAR_U:
			dx=0;
			dy=-1;
			break;

			case CHAR_D:
			dx=0;
			dy=1;
			break;

			case CHAR_L:
			dx=-1;
			dy=0;
			break;

			case CHAR_R:
			dx=1;
			dy=0;
			break;
			};

		irecon_printf("Final try: offset %d,%d\n",dx,dy);
		MoveObject(objsel,objsel->x+dx,objsel->y+dy,0);
		return 0;
		}
	}

return ret;
}





/*
 *    GetObjectBase(int x,int y)  -  return pointer to list at x,y
 */

OBJECT *GetObjectBase(int x,int y)
{
OBJECT *anchor,*a2;
char ok;

if(x<0 || x>=curmap->w || y<0 || y>=curmap->h)
	{
	return NULL;
//   sprintf(str,"GetObjectBase(%d,%d,%s) Attempted outside map boundary",x,y);
//   panic(cfa,str,"Ai! ai!  A Balrog!  A Balrog is come!  'Tis Durin's Bane!");
	}

// Find the list

anchor=curmap->objmap[ytab[y]+x];
a2=NULL;

// Find it in the Large Object list if possible
if(x>=mapx && x<=(mapx+VSW))
	if(y>=mapy && y<=(mapy+VSH))
		{
		a2=largemap[VIEWDIST+x-mapx][VIEWDIST+y-mapy];
		// If there isn't anything there, it presumably means that the
		// object has an Active Area that doesn't include the base
		// (But we must ONLY do this if Anchor found a Large Object!)
		if(anchor)
			if(anchor->flags & IS_LARGE && !(anchor->flags & IS_DECOR))
				if(!a2)
					anchor=anchor->next; // Get rid of the original pointer
		}

// If there wasn't anything at the original spot, but there is from the
// largemap, use that.
if(!anchor)
	anchor=a2;

// Prefer large object over System object (fixes room blanking problem with cursor over doors)
if(anchor)
	if(anchor->flags & IS_SYSTEM)
		if(a2)
			anchor=a2;

// Validate the chosen object (make sure it's not pending-delete)

do	{
	ok=1;
	if(!anchor)			// No object, return failure
		return NULL;

	if(!(anchor->flags & IS_ON))	// Object unsuitable, choose another
		{
		ok=0;
		anchor=anchor->next;
		}
	} while(!ok);

return anchor;
}


/*
 *    GetRawObjectBase(int x,int y)  -  return pointer to list at x,y
 *                                      Excludes large objects.
 */

OBJECT *GetRawObjectBase(int x,int y)
{
OBJECT *anchor;

// Find the list we want to modify

if(x<0 || x>=curmap->w || y<0 || y>=curmap->h)
	{
	return NULL;
//   sprintf(str,"GetObjectBase(%d,%d,%s) Attempted outside map boundary",x,y);
//   panic(cfa,str,"Ai! ai!  A Balrog!  A Balrog is come!  'Tis Durin's Bane!");
	}

anchor=curmap->objmap[ytab[y]+x];
return anchor;
}


/*
 *    GetObject(int x,int y)  -  return pointer to object at x,y
 */

OBJECT *GetObject(int x,int y)
{
OBJECT *temp,*prev;
OBJECT *anchor_and_large;
OBJECT *anchor_small;

// Get two separate opinions of what is in that square.
// Find out where they differ and thereby make plain the truth.
// A little bit like the way RGB is coded into just two signals in a PAL tv

anchor_and_large=GetObjectBase(x,y);
anchor_small=GetRawObjectBase(x,y);

// If this square's list is empty, say so

if(!anchor_and_large)              // No large or small objects found
	return anchor_and_large;       // Might already be in EAX

if(!anchor_small)                  // No small objects here
	return anchor_and_large;       // Return any large object

// Find the last object
prev = NULL;
for(temp=anchor_and_large;temp->next;temp=temp->next)
	{
	if(temp->flags & IS_ON)                          // Store last real object
		prev=temp;
	};    // Just seek

// If the candidate is not real regress

if(!(temp->flags & IS_ON))
	temp = prev;

return temp;
}


/*
 *    GameGetObject(int x,int y)  -  return pointer to object at x,y
 *                                   if it is not 'hidden'
 */

OBJECT *GameGetObject(int x,int y)
{
OBJECT *temp,*prev;
OBJECT *anchor_and_large;
OBJECT *anchor_small;

// Get two separate opinions of what is in that square.
// Find out where they differ and thereby make plain the truth.
// A little bit like the way RGB is coded into just two signals in a PAL tv

anchor_and_large=GetObjectBase(x,y);
anchor_small=GetRawObjectBase(x,y);

// If the small object is hidden, we'll need to find something more suitable
if(IsHidden(anchor_small))
	{
	// Find the first 'real' object
	prev = NULL;
	for(temp=anchor_small;temp;temp=temp->next)
		if(!IsHidden(temp))
			{
			prev=temp; // Got one
			break;
			}
	// Use this instead (or use nothing)
	anchor_small=prev;
	}

// If this square's list is empty, say so

if(!anchor_and_large)              // No large or small objects found
    return anchor_and_large;       // Might already be in EAX

if(!anchor_small)                  // No small objects here
    return anchor_and_large;       // Return any large object

CHECK_OBJECT(anchor_and_large);
CHECK_OBJECT(anchor_small);

// Find the last object
// JM: 07/01/2006: now searching the small object list. why were we
// searching the large object list which is usually a different square?!?
prev = NULL;
for(temp=anchor_small;temp->next;temp=temp->next)
	{
	if(!IsHidden(temp))
		prev=temp;                         // Store last real object
	};    // Just seek

// If the candidate is not real, regress

CHECK_OBJECT(temp);

// If candidate is hidden or invisible, regress
if(IsHidden(temp))
	temp = prev;

// Only choose the decor as a last resort
if(temp)
	if(temp->flags & IS_DECOR)
		{
		// If there's nothing else, keep the decor and don't regress
		if(prev)
			temp = prev;
		}

CHECK_OBJECT(temp);
return temp;
}


/*
 *      GetSolidObject - return a pointer to the last solid object
 *                       Based on GameGetObject
 */

OBJECT *GetSolidObject(int x,int y)
{
OBJECT *temp,*prev;
OBJECT *anchor_and_large;
OBJECT *anchor_small;

// Get two separate opinions of what is in that square.
// Find out where they differ and thereby make plain the truth.
// This is tricker than GameGetObject.. we have to find the solid ones

temp=GetObjectBase(x,y);
for(anchor_and_large=NULL;temp;temp=temp->next)
	if(temp->flags & IS_SOLID)
		{
		anchor_and_large=temp;
		break;
		}
anchor_small=GetRawSolidObject(x,y,NULL);

// If this square's list is empty, say so

if(!anchor_and_large)              // No large or small objects found
	return anchor_and_large;       // NULL might already be in EAX

if(!anchor_small)                  // No small objects here
	{
	if(LargeIntersect(anchor_and_large,x,y))
		return anchor_and_large;       // Return any large object
	}

CHECK_OBJECT(anchor_and_large);
CHECK_OBJECT(anchor_small);

// Find the last object
prev = NULL;
for(temp=anchor_and_large;temp->next;temp=temp->next)
	{
	if(temp->flags & IS_ON && !(temp->flags & IS_SYSTEM) && temp->flags & IS_SOLID
	&& ((temp->flags & IS_INVISSHADOW) != IS_INVISSHADOW))
		prev=temp;                         // Store last real object
	};    // Just seek

// If the candidate is not real, regress

CHECK_OBJECT(temp);

if(!(temp->flags & IS_ON))
	temp = prev;

// If candidate is invisible, regress

if(temp->flags & IS_SYSTEM || ((temp->flags&IS_INVISSHADOW) ==IS_INVISSHADOW))
	temp = prev;

// If candidate isn't solid, regress

if(!(temp->flags & IS_SOLID))
	temp = prev;

// If candidate isn't solid in this region, regress..
if(!LargeIntersect(temp,x,y))
	temp = prev;

CHECK_OBJECT(temp);

return temp;
}

/*
 *      GetRawSolidObject - return a pointer to the first solid object
 *                          Except is an object to skip, (e.g. yourself)
 *                          or NULL if none is desired
 */

OBJECT *GetRawSolidObject(int x,int y, OBJECT *except)
{
OBJECT *temp;

temp=GetRawObjectBase(x,y);

if(!temp)
    return temp;

for(;temp;temp=temp->next)
	if(temp->flags & IS_SOLID)
		if(temp->flags & IS_ON)
			if(!(temp->flags & IS_SYSTEM) && (temp->flags&IS_INVISSHADOW) !=IS_INVISSHADOW)
				if(temp != except)		// Is it The-One-We-Don't-Want?
					return temp;
return NULL;
}



/*
 *    IsBridge(int x,int y)  -  return true if square contains a bridge
 */

int IsBridge(int x,int y)
{
OBJECT *anchor,*a2;

if(x<0 || x>=curmap->w || y<0 || y>=curmap->h)
	return 0;

// Find the list

anchor=curmap->objmap[ytab[y]+x];
a2=NULL;

// Find it in the Large Object list if possible
if(x>=mapx && x<=(mapx+VSW))
	if(y>=mapy && y<=(mapy+VSH))
		{
		a2=bridgemap[VIEWDIST+x-mapx][VIEWDIST+y-mapy];
		if(a2 && a2->flags & IS_ON)
			return 1;
		}

for(;anchor;anchor=anchor->next)
	if(anchor->flags & IS_WATER)
		if(anchor->flags & IS_ON)
			return 1;

return 0;
}


/*
 *    IsTrigger(int x,int y)  -  return pointer if square contains a trigger
 */

OBJECT *IsTrigger(int x,int y)
{
OBJECT *anchor,*a2;

if(x<0 || x>=curmap->w || y<0 || y>=curmap->h)
	return NULL;

// Find the list

anchor=curmap->objmap[ytab[y]+x];
a2=NULL;

// Find it in the Large Object list if possible
if(x>=mapx && x<=(mapx+VSW))
	if(y>=mapy && y<=(mapy+VSH))
		{
		a2=largemap[VIEWDIST+x-mapx][VIEWDIST+y-mapy];
		if(a2 && a2->flags & IS_ON && a2->flags & IS_TRIGGER)
			return a2;
		}

if(!anchor)
	return NULL;

a2=NULL;

while(anchor)
	{
	if(anchor->flags & IS_TRIGGER && anchor->flags & IS_ON)
		a2=anchor;
	anchor=anchor->next;
	};


return a2;
}


/*
 *      WeighObject - calculate how much weight is in the list
 *                    calls WeighObjectR
 */

int WeighObject(OBJECT *obj)
{
int w=0;
if(!obj)
     return 0;
if(!(obj->flags & IS_ON))
    return 0;

if(!obj->stats || !obj->maxstats)
	return 0;

if(obj->stats->quantity > 1)
	if(obj->maxstats->weight > 0)
		return obj->stats->quantity*obj->maxstats->weight;

w = obj->stats->weight;
if(obj->pocket.objptr)
	{
	object_search_rec=0;
	return w+WeighObjectR(obj->pocket.objptr);
	}

return w;
}

/*
 *      WeighObjectR - recursive engine for WeightObject
 */

int WeighObjectR(OBJECT *list)
{
OBJECT *temp;
int w=0;
static int complain=0;

if(!list->stats || !list->maxstats)
	return 0;

if(list->stats->quantity > 1)
	if(list->maxstats->weight > 0)
		return list->stats->quantity*list->maxstats->weight;


// Defence against objects inside their asses

if(object_search_rec>TOO_MUCH)
	{
	if(!complain)
		{
		Bug("OH NO!  Too much recursion in WeighObject!\n");
		complain=1;
		}
	return 0;
	}
object_search_rec++;
complain=0;

// Okay, do the thing

for(temp = list;temp;temp=temp->next)
    if(temp->flags & IS_ON)
        {
        w+=temp->stats->weight;
        if(temp->pocket.objptr)
            w+=WeighObjectR(temp->pocket.objptr);
        }
return w;
}

/*
 *      GetBulk - calculate how much bulk is in the list
 *                calls GetBulkR
 */

int GetBulk(OBJECT *obj)
{
int w=0;
if(!obj)
     return 0;
if(!(obj->flags & IS_ON))
    return 0;

w = obj->stats->bulk;
if(obj->pocket.objptr)
    return w+GetBulkR(obj->pocket.objptr);

return w;
}

/*
 *      GetBulkR - recursive engine for GetBulk
 */

int GetBulkR(OBJECT *list)
{
OBJECT *temp;
int w=0;

for(temp = list;temp;temp=temp->next)
    if(temp->flags & IS_ON)
        {
        w+=temp->stats->bulk;
        if(temp->pocket.objptr)
            w+=GetBulkR(temp->pocket.objptr);
        }
return w;
}


/*
 *      AddToParty - append a new member to the party array
 */

int AddToParty(OBJECT *new_member)
{
int ctr;

// Are they already there?
for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	if(party[ctr] == new_member)
		return ctr;

//ilog_quiet("PARTY: new member: %s\n",new_member->name);

for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	if(!party[ctr])
		{
		party[ctr] = new_member;
		strcpy(partyname[ctr],BestName(new_member));
//ilog_quiet("addtoparty (%d)\n",follower);
		SubAction_Wipe(new_member);
		ActivityNum(new_member,Sysfunc_follower,NULL);
		SetNPCFlag(new_member,IN_PARTY);
		return ctr;
		}
return -1;
}


/*
 *      SubFromParty - remove a party member
 */

void SubFromParty(OBJECT *member)
{
int ctr,ctr2,pos;

if(!member)
    return;

//ilog_quiet("PARTY: sub member: %s\n",member->name);

pos=-1;
for(ctr=0;ctr<MAX_MEMBERS;ctr++)
    if(party[ctr] == member)
        pos=ctr;

if(pos == -1)
    return;

if(member->stats->hp>0)
	{
	ClearNPCFlag(member, IN_PARTY);
	ResumeSchedule(member);
    }
else
	Inactive(member);  // Prevent the dead guy from following you about

// If the leader is leaving, call an election

if(player == member)
     {
     player = NULL;
     for(ctr2=0;ctr2<MAX_MEMBERS-1;ctr2++)
         if(party[ctr2])
             if(party[ctr2]->stats->hp>0)
                 ChooseLeader(ctr2);
     }

party[pos]=NULL;
partyname[pos][0]='\0';

   // sort the party to remove any holes

for(ctr=0;ctr<MAX_MEMBERS-1;ctr++)
	{
restart:
	if(!party[ctr])
		for(ctr2=ctr;ctr2<MAX_MEMBERS-1;ctr2++)
			if(party[ctr2])
				{
				party[ctr] = party[ctr2];
				party[ctr2] = NULL;
				goto restart; // A 'better' way would be more convoluted
				}
	}

}


/*
 *      ChooseLeader- Set the party leader
 */

int ChooseLeader(int x)
{
if(!party[x])
    {
    Bug("Election: party member %d not present\n",x);
    return 0;
    }

//ilog_quiet("PARTY: ChooseLeader: %s\n",party[x]->name);

for(int ctr=0;ctr<MAX_MEMBERS;ctr++)
	if(party[ctr])
		if(party[ctr]->stats->hp > 0)
			{
			party[ctr]->activity=-1; // disable them
			AL_Del(&ActiveList,party[ctr]);
			}
player = party[x];
SubAction_Wipe(player); // Erase any sub-tasks
ActivityName(player,"player_action",NULL);

return 1;
}


/*
 *      FindEmptySquare- Find some scratch space for certain map operations
 */

void FindEmptySquare(int *x,int *y)
{
int sx,sy;

for(sy=0;sy<curmap->h;sy++)
    for(sx=0;sx<curmap->w;sx++)
        if(!curmap->objmap[ytab[sy]+sx])
            {
            *x=sx;
            *y=sy;
            return;
            }
ithe_panic("FindEmptySquare failed","Map probably corrupted");
}


/*
 *      gen_largemap- Build the table of objects larger than 1 square
 */

void gen_largemap()
{
int x,y,xctr,yctr;
int sx,sy,ex,ey,bx,by,xo,yo;
int blockx,blocky,blockw,blockh;

OBJECT *temp;

sx=mapx-VIEWDIST;
sy=mapy-VIEWDIST; // was -4
ex=mapx+VSW+4;
ey=mapy+VSH+4;
xo=0;
yo=0;
if(sx<0)
	{
	xo=0-sx;
	sx=0;
	}
if(sy<0)
	{
	yo=0-sy;
	sy=0;
	}
if(ex>=curmap->w)
	ex=curmap->w-(VSW+4);
if(ey>=curmap->h)
	ey=curmap->h-(VSH+4);

// Clear the grids
memset(&largemap[0][0],0,(sizeof(OBJECT *)*(LMSIZE*LMSIZE)));
memset(&solidmap[0][0],0,(sizeof(OBJECT *)*(LMSIZE*LMSIZE)));
memset(&bridgemap[0][0],0,(sizeof(OBJECT *)*(LMSIZE*LMSIZE)));
memset(&decorxmap[0][0],0,(sizeof(int)*(LMSIZE*LMSIZE)));
memset(&decorymap[0][0],0,(sizeof(int)*(LMSIZE*LMSIZE)));

by=yo;
bx=xo;
for(y=sy;y<ey;y++)
	{
	for(x=sx;x<ex;x++)
		{
		temp = curmap->objmap[ytab[y]+x];
		for(;temp;temp=temp->next)
			{
			if(temp->flags & IS_ON)
				{
				// Is it a decorative?
				if(temp->flags & IS_DECOR)
					{
					for(xctr=0;xctr<temp->mw;xctr++)
						for(yctr=0;yctr<temp->mw;yctr++)
							{
							decorxmap[bx+xctr][by+yctr]=x;
							decorymap[bx+xctr][by+yctr]=y;
							}
					}

				// Next, is it large?
				if(temp->flags & IS_LARGE)       // Found a large object?
					{
					if(temp->flags & IS_SOLID)   // Is it solid? If so, find out where
						{
						if(temp->curdir > CHAR_D) //(U,D,L,R) > D = L,R
							{
							blockx=temp->hblock[BLK_X];
							blocky=temp->hblock[BLK_Y];
							blockw=temp->hblock[BLK_W];
							blockh=temp->hblock[BLK_H];
							}
						else
							{
							blockx=temp->vblock[BLK_X];
							blocky=temp->vblock[BLK_Y];
							blockw=temp->vblock[BLK_W];
							blockh=temp->vblock[BLK_H];
							}

						// Bounds check
						if(bx+blockx+blockw > LMSIZE)
							blockw = LMSIZE-(bx+blockx);
						if(by+blocky+blockh > LMSIZE)
							blockh = LMSIZE-(by+blocky);

						// Reset border size
						temp->w = blockw<<5;
						temp->h = blockh<<5;

						// Colour in the squares where the solid parts are

						for(xctr=0;xctr<blockw;xctr++)
							for(yctr=0;yctr<blockh;yctr++)
								{
								solidmap[bx+xctr+blockx][by+yctr+blocky]=temp;
								// If it's solid it must exist here too
								largemap[bx+blockx+xctr][by+blocky+yctr]=temp;
								}
						}

					// Fill in the active area of the object

					if(temp->curdir > CHAR_D) //(U,D,L,R) > D = L,R
						{
						blockx=temp->harea[BLK_X];
						blocky=temp->harea[BLK_Y];
						blockw=temp->harea[BLK_W];
						blockh=temp->harea[BLK_H];
						}
					else
						{
						blockx=temp->varea[BLK_X];
						blocky=temp->varea[BLK_Y];
						blockw=temp->varea[BLK_W];
						blockh=temp->varea[BLK_H];
						}

					// Bounds check
					if(bx+blockx+blockw > LMSIZE)
						blockw = LMSIZE-(bx+blockx);
					if(by+blocky+blockh > LMSIZE)
						blockh = LMSIZE-(by+blocky);

					largemap[bx][by]=NULL; // Delete core section
					for(xctr=0;xctr<blockw;xctr++)
						for(yctr=0;yctr<blockh;yctr++)        // For the height,
							largemap[bx+blockx+xctr][by+blocky+yctr]=temp;

					// Is it a bridge
					if(temp->flags & IS_WATER)
						for(xctr=0;xctr<blockw;xctr++)
							for(yctr=0;yctr<blockh;yctr++)        // For the height,
								bridgemap[bx+blockx+xctr][by+blocky+yctr]=temp;
					}
				else
					{
					// Not a Large Object
					if(temp->flags & IS_SOLID)           // If it's small and solid
						solidmap[bx][by]=temp;  // Fill in the single square
					if(temp->flags & IS_WATER)
						bridgemap[bx][by]=temp;  // Fill in the single square
					}
				}
			}
		bx++;
		}
	bx=xo;
	by++;
	}

}

/*
 *      DestroyObject - Perform the complex task of destroying an arbitrary
 *                      object without harming the fabric of the universe
 */

void DestroyObject(OBJECT *obj)
{
OBJECT *temp,*pocket;
char str[256];
int ctr;

if(!obj)                // No!
	{
	ithe_panic("DestroyObject: Attempted to destroy NULL",NULL);
	return;
	}

if(obj->flags & IS_DECOR)
	return;

// If it's a decorative in the editor, preprocess it before doing anything
if(obj->user->edecor)
	ExpandDecor(obj);

// Be careful when deleting the player
if(obj == player)
	player=NULL;

// Delete it from the wielding list if it's there
if(player)
	for(ctr=0;ctr<WIELDMAX;ctr++)
		{
		if(GetWielded(player,ctr) == obj)
			SetWielded(player,ctr,NULL);
		}

// First see if it is where it seems to be

if(!obj->parent.objptr)
	{
	if(OB_CheckPos(obj))
		{
#ifdef DEBUG_SAVE
		if(obj->x == 0 && obj->y == 0)
			{
			ilog_quiet("destroy %x (%s) - stuff at 0,0 is:\n",obj,obj->name);
			for(temp=curmap->objmap[MAP_POS(obj->x,obj->y)];temp;temp=temp->next)
				ilog_quiet("temp = %x (%s).  Next = %x\n",temp,temp->name,temp->next);
			}
#endif

		LL_Remove(&curmap->objmap[MAP_POS(obj->x,obj->y)],obj);
		FreePockets(obj);

		LL_Kill(obj);
		return;
		}
	}

// Next look in the pockets, unless we are doing a bulk erase (NoDeps)

pocket = obj->parent.objptr;
if(pocket && !NoDeps)
	{
#ifdef DEBUG_SAVE
	ilog_quiet("DelFromParent %s (%x) ",obj->name,obj);
	ilog_quiet("inside %s %x\n",pocket->name,pocket);
#endif
	obj->parent.objptr=NULL;
	// Remove it from the system
	LL_Remove(&pocket->pocket.objptr,obj);
	FreePockets(obj);
	LL_Kill(obj);
	return;
	}

// In case of non-fatal bug, look for it in the linked list buffer

for(temp = curmap->object;temp;temp=temp->next)
	if(temp == obj)
		{
		Bug("Removing loose object %s\n",obj->name);
		LL_Remove(&curmap->object,obj);
		FreePockets(obj);
		LL_Kill(obj);
		return;
		}

// Oh dear

sprintf(str,"obj %s (%p) at %ld,%ld (id %d), next=%p, flags=%lx",obj->name,obj,obj->x,obj->y,obj->save_id,obj->next,obj->flags);
/*
Bug("DestroyObject: Object has no physical existence.");
Bug(str);
*/
ithe_panic("DestroyObject: Object has no physical existence",str);
}


/*
 *      LookForObject - Diagnostic message.  Give location of object
 */

void LookForObject(OBJECT *obj)
{
OBJECT *temp;
char str[256];
int ctr;

if(!obj)                // No!
	{
	ilog_printf("Object is Null\n");
	return;
	}

ilog_printf("Looking for object %s\n",obj->name);

if(obj->flags & IS_DECOR)
	{
	ilog_printf("Object is Decorative\n");
	return;
	}

// Delete it from the wielding list if it's there
if(player)
	for(ctr=0;ctr<WIELDMAX;ctr++)
		if(GetWielded(player,ctr) == obj)
			{
			ilog_printf("Object is wielded by player\n");
			}

// First see if it is where it seems to be

if(!obj->parent.objptr)
	{
	ilog_printf("Object has no parent\n");

	if(OB_CheckPos(obj))
		{
		ilog_printf("Object is where it should be (%ld,%ld)\n",obj->x,obj->y);
		return;
		}
	}


ilog_printf("Object is inside %s (%x)\n",obj->parent.objptr->name,obj->parent.objptr);

// In case of non-fatal bug, look for it in the linked list buffer

for(temp = curmap->object;temp;temp=temp->next)
	if(temp == obj)
		{
		ilog_printf("Object is in the LLB\n");
		return;
		}

if(LookInLimboFor(obj))
	{
	ilog_printf("Object is in limbo\n");
	return;
	}

// Oh dear

ilog_printf("Object could not be found\n");

sprintf(str,"obj %s (%p) at %ld,%ld (id %d), next=%p, flags=%lx",obj->name,obj,obj->x,obj->y,obj->save_id,obj->next,obj->flags);
/*
Bug("DestroyObject: Object has no physical existence.");
Bug(str);
*/
ilog_quiet("%s\n",str);
//ithe_panic("DestroyObject: Object has no physical existence",str);
}



/*
 *    DelDecor(int x,int y,OBJECT *o)  -  delete a DECOR object
 */

void DelDecor(int x,int y, OBJECT *o)
{
OBJECT *temp;
int found;

if(!o)
	{
//	ilog_quiet("DD: No obj\n");
	return;
	}

if(x<0 || x>=curmap->w || y<0 || y>=curmap->h)
	{
//	ilog_quiet("DD: oob\n");
	return;
	}

// Verify it is here
found=0;
for(temp=curmap->objmap[MAP_POS(x,y)];temp;temp=temp->next)
	if(temp == o)
		found=1;

if(!found)
	{
//	ilog_quiet("DD: Not there\n");
	return;
	}

LL_Remove(&curmap->objmap[MAP_POS(x,y)],o);
gen_largemap();

//ilog_quiet("DD: Done\n");
}


/*
 *    GetDecor(int *x,int *y,OBJECT *o)  -  get exact coords for a DECOR obj
 */

int GetDecor(int *x,int *y, OBJECT *o)
{
int ix,iy;

if(!o)
	return 0;

if(!x || !y)
	return 0;

ix=*x;
iy=*y;

if(ix<0 || ix>=curmap->w || iy<0 || iy>=curmap->h)
	return 0;

gen_largemap();

if(ix>=mapx && iy>=mapy)                      // Check for a big object
	{
	if(ix<mapx+VSW)                           // If onscreen
		{
		if(iy<mapy+VSH)
			{
			*x= decorxmap[VIEWDIST+ix-mapx][VIEWDIST+iy-mapy];
			*y= decorymap[VIEWDIST+ix-mapx][VIEWDIST+iy-mapy];
			return 1;
			}
		}
	}

*x=0;
*y=0;

return 0;
}


/*
 *      CreateContents - Create the default contents of an item
 */

void CreateContents(OBJECT *objsel)
{
OBJECT *temp;
OBJREGS oldvars;

if(objsel->flags & IS_DECOR)
	return;

for(int ictr=0;ictr<objsel->funcs->contents;ictr++)
	if(objsel->funcs->contains[ictr])
		{
		temp = OB_Alloc();
		temp->curdir = temp->dir[0];
		OB_Init(temp,objsel->funcs->contains[ictr]);
		OB_SetDir(temp,CHAR_D,FORCE_SHAPE);
		MoveToPocket(temp,objsel);
		// Prevent a living creature inside an egg from being active
		if(GetNPCFlag(temp,IS_BIOLOGICAL))
			AL_Del(&ActiveList,temp);
		}

// Call initial script if in the game

if(!in_editor)
	if(!(objsel->flags & DID_INIT))
		{
		if(objsel->funcs->icache != -1)
			{
			VM_SaveRegs(&oldvars);
			current_object = objsel;
			person = objsel;
			CallVMnum(objsel->funcs->icache);
			VM_RestoreRegs(&oldvars);
			}
		objsel->flags |= DID_INIT;
		}

}

/*
 *      MoveToTop - Move specified object to the top of the pile
 */

void MoveToTop(OBJECT *object)
{
int ox,oy;
// Move object to the top of the heap by moving it to 0,0 and back

if(object->parent.objptr)		// Don't bother for pocketed object
	return;

ox = object->x;
oy = object->y;
TransferObject(object,0,0);
TransferObject(object,ox,oy);
}

/*
 *      MoveToFloor - Move specified object to above the last FIXED object
 *                    CAUTION: does low-level linked-list manipulation
 */

void MoveToFloor(OBJECT *object)
{
OBJECT *temp,*after;

// Move object to the top of the heap by moving it to 0,0 and back

if(object->parent.objptr)		// Don't bother for pocketed object
	return;

// Disassociate from the map
LL_Remove(&curmap->objmap[MAP_POS(object->x,object->y)],object);

after=NULL;
temp = GetRawObjectBase(object->x,object->y);
for(;temp;temp=temp->next)
   if(temp->flags & IS_FIXED && !(temp->flags & IS_DECOR))
        after=temp;

// If there are no fixed objects, stick it on the front.
// Also, if the object itself is fixed (cures the blood in doorway problems)
if(!after || object->flags & IS_FIXED)
    {
    object->next = GetRawObjectBase(object->x,object->y);
    curmap->objmap[MAP_POS(object->x,object->y)] = object;
    return;
    }

// Otherwise stick it after the object we've chosen

object->next = after->next;
after->next = object;
}

/*
 *      TakeQuantity - Subtract an amount from a container with several
 *                     of these objects in, e.g. player pays 100 gold coins,
 *                     the coins may be scattered througout his pockets.
 */

int TakeQuantity(OBJECT *container,char *type,int request)
{
int total,found;
OBJECT *temp;
int o;

// First, see if the object can support quantity values.
o = getnum4char(type);
if(o == -1)
    return 0;

if(CHlist[o].flags & IS_QUANTITY)
	o=1;
else
	o=0;

total = SumObjects(container->pocket.objptr, type, 0);

if(!request)
	request = 1;

if(total<request)
	return 0;

found=0;
do {
	temp = GetFirstObjectFuzzy(container->pocket.objptr,type);
	if(!temp)
		{
		Bug("take_quantity: financial irregularities detected in object %s\n",container->name);
		return 0;
		}

   // If the object doesn't support Quantities, we change it so it does
   // and set the quantity of the object to 1 temporarily, since it will
   // be destroyed later, anyway

	if(!o)
		temp->stats->quantity = 1;

   // If less than requested sum, take it all and destroy it.

	if(temp->stats->quantity <= request)
		{
		found += temp->stats->quantity;
		DeleteObject(temp);
		}
	else
		{
		temp->stats->quantity -=request; // Otherwise just take what we need
		found=request;
		}


	Qsync(temp);  // Call the Quantity Change function if present

	} while(found<request);

return 1;
}

/*
 *      AddQuantity - Add a number of objects to the container.
 *                    E.g. pay the player 100 gold coins, and group them
 *                    together by adding to the first pile we come across.
 *                    If we try to add 0, create a new object regardless
 */

void AddQuantity(OBJECT *container,char *type,int request)
{
OBJECT *temp;
int o;

// First, see if the object can support quantity values.
o = getnum4char(type);
if(o == -1)
    {
    Bug("Cannot create unknown object %s\n",type);
    return;
    }

if(CHlist[o].flags & IS_QUANTITY)
	o=1;
else
	o=0;

temp = GetFirstObject(container->pocket.objptr,type);
if(request == 0)
    temp = NULL;

// If the object exists
if(temp && o > 0)
	{
	temp->stats->quantity+=request;
	Qsync(temp);  // Call the Quantity Change function if present
	return;
	}

// If it supports multiply quantites, just create one and set the quantity

if(o)
	{
	temp = OB_Alloc();
	OB_Init(temp,type);
	temp->stats->quantity=request;
	MoveToPocket(temp,container);
	Qsync(temp);  // Call the Quantity Change function if present
	return;
	}

// If not, make several

for(o=0;o<request;o++)
	{
	temp = OB_Alloc();
	OB_Init(temp,type);
	CreateContents(temp);
	MoveToPocket(temp,container);
	}

return;
}

/*
 *      MoveQuantity - Transfer an amount between containers with several
 *                     of these objects in, e.g. player pays 100 gold coins,
 *                     the coins may be scattered througout his pockets.
 */

int MoveQuantity(OBJECT *src,OBJECT *dest,char *type,int request)
{
int total,found;
OBJECT *temp;
int o,ctr;

// First, see if the object can support quantity values.
temp = GetFirstObjectFuzzy(src->pocket.objptr,type);
if(!temp)
	{
//	printf("Movequantity: not present in source\n");
	return 0;	// There aren't any to study
	}

if(temp->flags & IS_QUANTITY)
	o=1;
else
	o=0;

total = SumObjects(src->pocket.objptr, type, 0);

if(!request)
	request = 1;

if(total<request)
	{
//	printf("MoveQuantity: not enough\n");
	return 0;
	}

// If the objects cannot be piles, trasfer them one at a time

if(!o)
	{
//	printf("MoveQuantity: moving objects\n");

	for(ctr=0;ctr<request;ctr++)
		{
		temp = GetFirstObjectFuzzy(src->pocket.objptr,type);
		if(!temp)
			{
			Bug("move_quantity: financial irregularities detected in object %s\n",src->name);
			return 0;
			}

//		printf("Moving %s to %s\n",temp->name,dest->name);
		ForceFromPocket(temp,src,0,0);
		TransferToPocket(temp,dest);
		}
	return 1;
	}

// If they can, delete the specified amount in the src and create in the dest

//printf("MoveQuantity: doing it mathematically\n");

found=0;
do
	{
	temp = GetFirstObject(src->pocket.objptr,type);
	if(!temp)
		{
		Bug("move_quantity: financial irregularities detected in object %s\n",src->name);
		return 0;
		}

	// If less than requested sum, take it all and destroy it.

	if(temp->stats->quantity <= request)
		{
		found += temp->stats->quantity;
		DeleteObject(temp);
		AddQuantity(dest,type,request);
		}
	else
		{
		temp->stats->quantity -=request; // Otherwise just take what we need
		found=request;
		AddQuantity(dest,type,request);
		Qsync(temp);  // Call the Quantity Change function if present
		}


	} while(found<request);

return 1;
}

/*
 * Set up the Object's current activity by number
 */

void ActivityNum(OBJECT *o,int activity, OBJECT *target)
{
OBJREGS objvars;

if(activity < 0)
	return;

if(!o)
	{
	ilog_quiet("Tried to assign activity %d to NULL\n",activity);
	ithe_panic("Tried to assign activity to NULL",NULL);
	}

if(debug_act)
	ilog_quiet("%s does %s to %s\n",o->name,PElist[activity].name,target?target->name:"Nothing");

if(GetNPCFlag(o,IN_BED))
	{
	VM_SaveRegs(&objvars);
	person = o;
	current_object = o;
	CallVMnum(Sysfunc_wakeup);
	VM_RestoreRegs(&objvars);
	}

o->activity = activity;
o->target.objptr = target;
o->user->vigilante=0; // Reset vigilante flag

// Activate the object if it is not a creature inside an egg
if(!(o->parent.objptr && GetNPCFlag(o, IS_BIOLOGICAL)))
	if(!ML_InList(&ActiveList,o))
		{
		AL_Add(&ActiveList,o);
		}
}

/*
 * Set up the Object's current activity by name
 */

void ActivityName(OBJECT *o,char *activity, OBJECT *target)
{
if(!o)
	{
	Bug("Tried to assign activity %s to NULL\n",activity);
	return;
	}

o->activity = getnum4PE(activity);
if(o->activity <0)
	{
	Bug("Couldn't find activity script called '%s' for %s\n",activity,BestName(o));
	Inactive(o); // Can't do it.  Stop the object
	return;
	}

if(o->flags & IS_DECOR)
	return;

o->target.objptr = target;
o->user->vigilante=0; // Reset vigilante flag

// Activate the object if it is not a creature inside an egg
if(!(o->parent.objptr && GetNPCFlag(o, IS_BIOLOGICAL)))
	if(!ML_InList(&ActiveList,o))
		{
		AL_Add(&ActiveList,o);
		}
}

/*
 * Wipe the Object's current activity
 */

void Inactive(OBJECT *o)
{
if(!o)
	ithe_panic("Tried to delete activity from NULL",NULL);

//ilog_quiet("InActivate %s\n",o->name);

o->activity = -1;
o->target.objptr = NULL;
o->user->vigilante=0; // Reset vigilante flag
AL_Del(&ActiveList,o);
}



/*
 *  LookInLimboFor - Is it in the inter-map storage area?
 */

int LookInLimboFor(OBJECT *obj)
{
OBJECT *temp;

for(temp = &limbo;temp;temp=temp->next)
	if(temp == obj)
		return 1;

return 0;
}


/*
 *     SumObjects - Return the total of a multiple object, e.g. money.
 *                  Recursive.
 */

int SumObjects(OBJECT *cont, char *name, int total)
{
for(OBJECT *temp = cont;temp;temp=temp->next)
    {
    if(temp->flags & IS_ON)
      if(!istricmp_fuzzy(temp->name,name))
        {
        if(temp->flags & IS_QUANTITY)
            total+=temp->stats->quantity;
        else
            total++;
        }
    if(temp->pocket.objptr)
        total+=SumObjects(temp->pocket.objptr,name,0); // 0, NOT total!
    }
return total;
}


/*
 *     EraseContents - Erase contents of an object.  Recursive.
 */

void EraseContents(OBJECT *cont)
{
if(!cont)
	return;

for(OBJECT *temp = cont;temp;temp=temp->next)
	{
	EraseContents(temp->pocket.objptr);
	DeleteObject(temp);
	}
}


/*
 *     GetFirstObject - Find the first instance of a specified object.
 *                      Recursive.
 */

OBJECT *GetFirstObject(OBJECT *cont, char *name)
{
OBJECT *search;

for(OBJECT *temp = cont;temp;temp=temp->next)
	{
	if(temp->flags & IS_ON)
		if(!istricmp(temp->name,name))
			return temp;
	if(temp->pocket.objptr)
		{
		search=GetFirstObject(temp->pocket.objptr,name);
		if(search)
			return search;
		}
	}
return NULL;
}

/*
 *     GetFirstObjectFuzzy - Find the first instance of an object which
 *                           matches the first few characters of the name.
 *                           Recursive.
 */

OBJECT *GetFirstObjectFuzzy(OBJECT *cont, char *name)
{
OBJECT *search;

for(OBJECT *temp = cont;temp;temp=temp->next)
	{
	if(temp->flags & IS_ON)
		{
//		printf("GetFirstObjectFuzzy: %s vs %s\n",temp->name,name);
		if(!istricmp_fuzzy(temp->name,name))
			{
//			printf("GetFirstObjectFuzzy: matched\n");
			return temp;
			}
		}
	if(temp->pocket.objptr)
		{
		search=GetFirstObjectFuzzy(temp->pocket.objptr,name);
		if(search)
			return search;
		}
	}
return NULL;
}

/*
 *      CheckDepend - Remove all references to a pointer
 */

void CheckDepend(OBJECT *ptr)
{

if(!ptr)
	return;

// If it is evil and must be cleansed, setting the kill bit won't matter
ptr->flags &= ~IS_ON;

// Delete from activelist if necessary
ML_Del(&ActiveList,ptr);

// Stalinist purge
MassCheckDepend();

return;
}


/*
 *      MassCheckDepend - Purge all pending-delete objects
 */

void MassCheckDepend()
{
int ctr;
OBJLIST *t;

// All the objects in the entire universe

t = MasterList;
if(!t)
	return;

for(t=MasterList;t;t=t->next)
	if(!t->ptr)
		{
		Bug("NULL in masterlist at %x\n",t);
		ML_Del(&MasterList,NULL);
		t=MasterList;
		}
	else
		CheckRefs(t->ptr);

// All globally-defined objects

if(!in_editor)
	wipe_refs();

// If a party member was boiled to vapour kick 'em out of the party

if(!in_editor)
	for(ctr=0;ctr<MAX_MEMBERS;ctr++)
		if(party[ctr])
			if(!(party[ctr]->flags & IS_ON))
				party[ctr] = NULL;

return;
}


void CheckRefsPtr(void **o)
{
OBJECT *ptr;
// Convert address of object pointer into an object pointer

if(!o)
	return; // It doesn't exist
	
ptr = *(OBJECT **)o;
if(ptr)
	{
	if((ptr->flags & IS_ON) == 0)
		{
		// Destroy the object
		ilog_quiet("CREV: destroy bad global %s (%x)\n",ptr->name,ptr);
		*o=NULL;
		return;
		}
	}

CheckRefs(ptr);

return;
}


/*
 *      CheckRefs - Purge any pending-delete pointers in this object
 */

void CheckRefs(OBJECT *o)
{
int ctr;

if(!o)
	return;

// Check enemy
if(o->enemy.objptr)
	if((o->enemy.objptr->flags & IS_ON) == 0)
		o->enemy.objptr=NULL;

// Check owner
if(o->stats)
	if(o->stats->owner.objptr)
		if((o->stats->owner.objptr->flags & IS_ON) == 0)
			o->stats->owner.objptr=NULL;

// If the object is inside a Suspect, drop it

if(o->parent.objptr)
	if((o->parent.objptr->flags & IS_ON) == 0)
		{
#ifdef DEBUG_SAVE
		ilog_quiet("CKR: emergency drop object %s (%x) ID%ld from %s (%p) at %ld,%ld\n",o->name,o,o->save_id,o->parent->name,o->parent,o->parent->x,o->parent->y);
#endif
		LL_Remove(&o->parent.objptr->pocket.objptr,o);
		ForceDropObject(0,0,o);
//printf("CheckRefs on %s\n",o->name);
		o->flags &= ~IS_ON; // And make its days numbered too
		// Delete from activelist if necessary
		ML_Del(&ActiveList,o);
		}

// Check activity target
if(o->target.objptr)
	if((o->target.objptr->flags & IS_ON) == 0)
		o->target.objptr=NULL;

// Check schedule
if(o->schedule)
	for(ctr=0;ctr<24;ctr++)
		if(o->schedule[ctr].active)
			if(o->schedule[ctr].target.objptr)
				if((o->schedule[ctr].target.objptr->flags & IS_ON) == 0)
					o->schedule[ctr].active = 0;

// Check sub-task stack
if(o->user)
	for(ctr=0;ctr<ACT_STACK;ctr++)
		if(o->user->acttarget[ctr])
			if((o->user->acttarget[ctr]->flags & IS_ON) == 0)
				o->user->acttarget[ctr] = NULL;

if(o->user)
	{
	// Is the path goal it?
	if(o->user->pathgoal.objptr)
		if((o->user->pathgoal.objptr->flags & IS_ON) == 0)
			o->user->pathgoal.objptr=NULL;
	}

return;
}




/*
 *      DeletePending() - Remove all Pending-Delete objects (Garbage collect)
 */

void DeletePending()
{
OBJLIST *t;
if(!pending_delete)
	return;

t = MasterList;
if(!t)
	return;

do
	{
	if(!(t->ptr->flags & IS_ON))
		{
		DestroyObject(t->ptr);  // Free the object
		t=MasterList;           // Restart the check
		}
	if(t)
		t=t->next;
	} while(t);

pending_delete=0;             // None
}

/*
 *      DeletePending with progress callback
 */

void DeletePendingProgress(void (*func)(int count, int max))
{
OBJLIST *t;
int count,max;

if(!pending_delete)
	return;
if(!MasterList)
    return;

// First see how many there are

max=0;
for(t = MasterList;t;t=t->next)
	if(!(t->ptr->flags&IS_ON))
		max++;

// Then delete them, counting as we go

count=0;
t = MasterList;
do
	{
	if(!(t->ptr->flags&IS_ON))
		{
		count++;
		DestroyObject(t->ptr);  // Free the object
		t=MasterList;           // Restart the check
		func(count,max);
		}
	if(t)
		t=t->next;
	} while(t);

pending_delete=0;             // None
}


/*
 *        Is something standing on spikes or something?
 */

void IfTriggered(OBJECT *spikes)
{
OBJECT *p,*op;
OBJREGS oldvars;
int w,h,x,y,x1,y1;

if(!spikes)
	return;

if(spikes->x == 0 && spikes->y == 0)
	return;

// Seek to end of list
if(spikes->next && spikes->next->next)
	for(;spikes->next->next;spikes=spikes->next);
	
if(!(spikes->flags & IS_TRIGGER))
	return;

p = spikes;
op = spikes;

w=spikes->mw;
h=spikes->mh;

if(w==1 && h==1)
	{
	if(spikes->next)
		if(!spikes->next->parent.objptr) // If not in a pocket
			{
			if(!(spikes->next->flags & IS_SPIKEPROOF))
				{
				VM_SaveRegs(&oldvars);
				victim = spikes->next;
				current_object = p;
				spikes = victim;
				if(p->funcs->scache)
					CallVMnum(p->funcs->scache);
				spikes = p;
				VM_RestoreRegs(&oldvars);
				}
			}
	}
else
	{
	if(spikes->curdir > CHAR_D) //(U,D,L,R)
		{
		x1=spikes->harea[BLK_X];
		y1=spikes->harea[BLK_Y];
		w=spikes->harea[BLK_W];
		h=spikes->harea[BLK_H];
		}
	else
		{
		x1=spikes->varea[BLK_X];
		y1=spikes->varea[BLK_Y];
		w=spikes->varea[BLK_W];
		h=spikes->varea[BLK_H];
		}

	for(y=0;y<h;y++)
		for(x=0;x<w;x++)
			{
			p=GetObjectBase(x1+x+spikes->x,y1+y+spikes->y);
			if(p == spikes)
				p = spikes->next;
			if(p)
				if(!p->parent.objptr) // if not in a pocket
					if(!(p->flags & IS_SPIKEPROOF))
						{
						VM_SaveRegs(&oldvars);
						victim = p;
						current_object = spikes;
						spikes = victim;
						CallVMnum(op->funcs->scache);
						spikes = op;
						VM_RestoreRegs(&oldvars);
						}
			}
	}
}


/*
 *      IsOnscreen() - is an NPC on screen?
 */

int IsOnscreen(char *pname)
{
OBJECT *temp;

for(int y=0;y<VSH;y++)
    for(int x=0;x<VSW;x++)
        for(temp = GetObjectBase(x+mapx,y+mapy);temp;temp=temp->next)
            if(temp->personalname)
		{
//		ilog_quiet("is_onscreen: found '%s', seeking '%s'\n",temp->personalname,pname);
                if(!istricmp(temp->personalname,pname))
                    return 1;
		}
return 0;
}

/*
 *  Update the coordinates of all objects in this one's pocket
 */

void UpdatePocketCoords(OBJECT *o, int x, int y)
{
OBJECT *temp;
for(temp=o->pocket.objptr;temp;temp=temp->next)
	{
	temp->x=x;
	temp->y=y;
	if(temp->pocket.objptr)
		UpdatePocketCoords(temp,x,y);
	}
}


char *BestName(OBJECT *o)
{
// If it is Not, say so

if(!o)
	return "Nothing";

// If it has a personal name, and you know it, use it

if(o->personalname)
	if(o->personalname[0] != '\0')
		if(GetNPCFlag(o, KNOW_NAME))
			if(istricmp(o->personalname,"-"))
				return o->personalname;

/*
// If it is you, say as much

if(o == player)
     return "you";
*/

// Oh well.  Use the short description

return o->shortdesc;
}


/*
 *      Spillcontents - If a container is destroyed it bursts
 */

void spillcontents(OBJECT *t,int x,int y)
{
OBJECT *temp;
if(!t)
	return;

if(t->flags & IS_DECOR)
	return;

for(int ctr=0;ctr<WIELDMAX;ctr++)
	{
	temp=GetWielded(t,ctr);
	if(temp)
		{
		ClearNPCFlag(temp, IS_WIELDED);
		temp->flags &= ~IS_INVISIBLE; // Make absolutely sure - may be true for old savegames
		// IS_WIELDED and IS_INVISIBLE were at one point the same flag.
		SetWielded(t,ctr,NULL);
		}
	}

while(t->pocket.objptr)
	{
	temp=t->pocket.objptr;
	t->pocket.objptr=t->pocket.objptr->next;
	ForceDropObject(x,y,temp);
	};
}

//
//  Make the specified objects not be wielded anymore (NULL dest = unwield EVERYTHING)
//

void UnWield(OBJECT *container, OBJECT *dest)
{
OBJECT *temp;
if(!container || !container->wield)
	return;

for(int ctr=0;ctr<WIELDMAX;ctr++)
	{
	temp=GetWielded(container,ctr);
	if(temp)
		if(temp == dest || !dest)
			{
			ClearNPCFlag(temp, IS_WIELDED);
			temp->flags &= ~IS_INVISIBLE;	// Make absolutely sure, e.g. for old savegames
			SetWielded(container,ctr,NULL);
			}
	}

}


/*
 *      check_hurt - Poll the things to see if they've been harmed or killed
 */

void CheckHurt(OBJECT *start)
{
OBJECT *temp,*oldcurrent,*next=NULL;

// Check for things that are hurt

if(!start)
	{
	ilog_quiet("CheckHurt NULL\n");
    return;
	}

oldcurrent=current_object;

for(temp=start;temp;temp=next)
    {
    next=temp->next;

	if(temp->flags & IS_DECOR) // STOP
		return;

    if(temp->flags & IS_ON)
        {
	// If object has been healed, keep oldhp in sync
        if(temp->stats->hp > temp->user->oldhp)
		temp->user->oldhp=temp->stats->hp;
	  
        if(temp->stats->hp < temp->user->oldhp)
            {
            if(temp->stats->hp>0)
                if(temp->funcs->hcache != -1)
                    {
                    current_object = temp;
                    CallVMnum(temp->funcs->hcache);
                    current_object = oldcurrent;
                    }
            temp->user->oldhp=temp->stats->hp;
            }
            

// "They're all dead.  They just don't know it yet." - Eric Draven, The Crow

        if(temp->stats->hp < 1)
            {
			UnWield(temp,NULL);	// Unwield all wielded stuff
            if(temp->funcs->kcache != -1)      // If it can be killed..
                {
                current_object = temp;
                CallVMnum(temp->funcs->kcache); // ..then kill it.
                if(current_object)
                    temp->user->oldhp=temp->stats->hp;
                current_object = oldcurrent;
                }
            else                                  // If it cannot be killed
            if(temp->stats->hp < destroyhp)       // and it is very dead..
                {                                 // ..it shall be Made Not
                current_object = temp;
                CallVMnum(Sysfunc_erase);       // wipe it out
                if(oldcurrent == temp)
                    oldcurrent = NULL;
                if(victim == temp)
                    victim = NULL;
                current_object = oldcurrent;
                }
            }
        }
    }
}

/*
 *      find_id - Look for an ID number on the map and return the objptr
 */

OBJECT *find_id(unsigned int id)
{
OBJLIST *t;
for(t=MasterList;t;t=t->next)
	if(t->ptr)
		if(t->ptr->save_id == id)
			return t->ptr;

return NULL;
}


/*
 *      FindTag - Look for a tag on the map and return the object
 */

OBJECT *FindTag(int tag,char *name)
{
OBJLIST *t;

for(t=MasterList;t;t=t->next)
    if(t->ptr->tag == tag)
	{
        if(name)
            {
            if(!istricmp(name,t->ptr->name))
                return t->ptr;
            }
        else
            return t->ptr;
	}

return NULL;
}


/*
 *      OB_CheckPos - Check Object is where it seems to be
 */

int OB_CheckPos(OBJECT *o)
{
OBJECT *t;

// If it doesn't exist, quit
if(!o)
	return 0;

// If the object is inside another, do a recursive scan
if(o->parent.objptr)
	{
//	ilog_quiet("%s: recurse into %s\n",o->name,o->parent->name);
	return OB_CheckPos(o->parent.objptr);
	}

if(o->x<0 || o->x>curmap->w)
	return 0;
if(o->y<0 || o->y>curmap->h)
	return 0;

t=curmap->objmap[MAP_POS(o->x,o->y)];
for(;t;t=t->next)
	if(t == o)
		return 1;

return 0;
}

// Add a Sub-Activity to the object's queue

void SubAction_Push(OBJECT *o,int activity,OBJECT *target)
{
int ctr;

// Find an empty slot

for(ctr=0;ctr<ACT_STACK;ctr++)
	if(o->user->actlist[ctr] <= 0) // Nothing here
		{
		o->user->actlist[ctr]=activity;
		o->user->acttarget[ctr]=target;
		return;
		}
}

// Take a Sub-Activity from the object's queue

void SubAction_Pop(OBJECT *o,int *activity,OBJECT **target)
{
int ctr;

// Blank in case queue empty
*activity=0;
*target=NULL;

// Anything in queue?
if(o->user->actlist[0]>0)
	{
	// Yes, take it
	*activity=o->user->actlist[0];
	*target=o->user->acttarget[0];

	// Now move the queue along
	for(ctr=1;ctr<ACT_STACK;ctr++)
		{
		o->user->actlist[ctr-1]=o->user->actlist[ctr];
		o->user->acttarget[ctr-1]=o->user->acttarget[ctr];
		}

	// Blank the one at the end
	o->user->actlist[ACT_STACK-1]=0;
	}
}

// Delete all sub-activities for this object

void SubAction_Wipe(OBJECT *o)
{
int ctr;

// Find an empty slot

for(ctr=0;ctr<ACT_STACK;ctr++)
	{
	o->user->actlist[ctr]=0;
	o->user->acttarget[ctr]=NULL;
	return;
	}
}

/*
 *  Mark an object for deletion
 */

void DeleteObject(OBJECT *o)
{
if(!o)
	return;

if(o->flags & IS_DECOR)
	return;

if(o->pocket.objptr)
	spillcontents(o,o->x,o->y);

// Delete from activelist if necessary
ML_Del(&ActiveList,o);

o->flags &= ~IS_ON;
pending_delete=1;
}

// Empty location string
char *NoLocation="-";


/*
 *   Add a location name to an object and cache it if necessary
 */

void SetLocation(OBJECT *obj, char *loc)
{
LOCNAME *lptr;

if(!obj || !obj->labels) // Ouch.
	return;

// Is it 'none'?
if(!istricmp(loc,"-") || strlen(loc)<1)
	{
	obj->labels->location=NULL;
	return;
	}

// If it's there, use it
for(lptr=LocList;lptr;lptr=lptr->next)
	if(!istricmp(loc,lptr->name))
		{
		obj->labels->location = lptr->name;
		return;
		}

// It isn't there, so we must add it to the list.

// No locations in the list, make it the first entry
if(!LocList)
	{
	lptr=(LOCNAME *)M_get(1,sizeof(LOCNAME));
	lptr->next=NULL;
	strncpy(lptr->name,loc,sizeof(lptr->name)-1);
	LocList=lptr;

	// Don't forget to set up the location
	obj->labels->location=lptr->name;
	return;
	}

// Add it to the end
for(lptr=LocList;lptr->next;lptr=lptr->next);
//assert(!lptr->next);

lptr->next=(LOCNAME *)M_get(1,sizeof(LOCNAME));
lptr->next->next=NULL;
strncpy(lptr->next->name,loc,sizeof(lptr->next->name)-1);

// Don't forget to set up the location
obj->labels->location=lptr->next->name;
}


/*
 *  Turn the location list into a list of strings suitable for sorting
 *  First parameter is list pointer, second will be set to no. of items
 *  The caller must free the list pointer afterwards.
 */

void GetLocationList(char ***list,int *num)
{
int n;
LOCNAME *lptr;

n=1;// Reserve 'none'
for(lptr=LocList;lptr;lptr=lptr->next) n++;
*num=n;

*list=(char **)M_get(n,sizeof(char *));

n=0;
(*list)[n++]=NoLocation;
for(lptr=LocList;lptr;lptr=lptr->next)
	(*list)[n++]=lptr->name;
}


void Qsync(OBJECT *temp)
{
OBJECT *oldcur;

if(temp && temp->funcs->qcache >= 0)
	{
	oldcur = current_object;
	current_object = temp;
	CallVMnum(temp->funcs->qcache);
	current_object = oldcur;
	}
}

//
//  Is the object real or hidden
//

int IsHidden(OBJECT *o)
{
if(!o)
	return 1;
if(!(o->flags & IS_ON))
	return 1;
if(o->flags & IS_SYSTEM)
	return 1;
if((o->flags&IS_INVISSHADOW) == IS_INVISSHADOW)
	return 1;
return 0;
}

//
//  Convert a numeric wield slot to a pointer
//

OBJECT **GetWieldPtr(OBJECT *container, int handle)
{
WIELD *w;
  
if(!container || !container->wield)
	return NULL;
w=container->wield;

switch(handle)
	{
	case WIELD_HEAD:
	      return &w->head;
	case WIELD_NECK:
	      return &w->neck;
	case WIELD_BODY:
	      return &w->body;
	case WIELD_LEGS:
	      return &w->legs;
	case WIELD_FEET:
	      return &w->feet;
	case WIELD_ARMS:
	      return &w->arms;
	case WIELD_LHAND:
	      return &w->l_hand;
	case WIELD_RHAND:
	      return &w->r_hand;
	case WIELD_LFINGER:
	      return &w->l_finger;
	case WIELD_RFINGER:
	      return &w->r_finger;
	case WIELD_SPARE1:
	      return &w->spare1;
	case WIELD_SPARE2:
	      return &w->spare2;
	case WIELD_SPARE3:
	      return &w->spare3;
	case WIELD_SPARE4:
	      return &w->spare4;
	      
	}

return NULL;
}

//
//  Return a wielded object in a given slot
//

OBJECT *GetWielded(OBJECT *container, int handle)
{
OBJECT **p=GetWieldPtr(container,handle);
if(!p)
	return NULL;

return *p;
}

//
//  Wield an object in a given slot
//

int SetWielded(OBJECT *container, int handle, OBJECT *item)
{
OBJECT **p=GetWieldPtr(container,handle);
if(!p)
	return 0;
*p=item;
return 1;
}

//
//  Find the point at which a given object is wielded
//

OBJECT **FindWieldPoint(OBJECT *object, int *handle)
{
OBJECT **point;
OBJECT *parent;

if(!object)
	return NULL;
  
parent = object->parent.objptr;
if(!parent)
	return NULL;

for(int ctr=0;ctr<WIELDMAX;ctr++)
	{
	point=GetWieldPtr(parent,ctr);
	if(point && *point ==object)
		{
		if(handle)
			*handle=ctr;
		return point;
		}
	}
	
return NULL;
}

void SetOwnerInContainer(OBJECT *obj)
{
if(!obj || !obj->parent.objptr || !obj->stats)
	return;
if(obj->parent.objptr->flags & IS_PERSON)
	{
	// Simple case
	obj->stats->owner.objptr = obj->parent.objptr;
	return;
	}

if(obj->parent.objptr->stats && obj->parent.objptr->stats->owner.objptr)
	{
	// Set it to the same owner as the container
	obj->stats->owner.objptr = obj->parent.objptr->stats->owner.objptr;
	return;
	}

// Not sure what to do now
}
