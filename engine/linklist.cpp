//
//      IRE linked list management routines
//

#include <stdlib.h>
#ifndef _WIN32
#include <string.h>
#endif

#include "ithelib.h"
#include "core.hpp"
#include "gamedata.hpp"
#include "map.hpp"
#include "resource.hpp"
#include "linklist.hpp"
#include "object.hpp"
#include "console.hpp"
#include "init.hpp"
#include "oscli.hpp"

// defines (mostly diagnostic)

#define ML_CACHE				// You may need to disable this for testing
//#define CACHE_DOUBLECHECK
//#define ML_TELLALL

//#define LOG_MLADD
//#define CHECK_MASTERLIST
//#define PROFILE_LLADD
//#define WITH_PREJUDICE        // Clear erased object with shit (Fortify)

// variables

extern char in_editor;        // Which program we are in?
extern char *lastvrmcall;	  // VRM object was detected in
int NoDeps=0;                 // Skip dependency checks on delete

// functions

OBJECT *OB_Alloc();           // Allocate an Object
void OB_Free(OBJECT *ptr);    // Free an Object

int OB_SetDir(OBJECT *objsel,int dir, int force);     // set direction
void OB_SetSeq(OBJECT *objsel,char *name);  // change appearance
int OB_Init(OBJECT *objsel,char *name);  // (re)set an object

int LLcache_register(void *ptr);
void *LLcache_get(void *ptr);
void LLcache_update(void *ptr, void *newptr);
//int LLcache_invalidate(void *ptr, void *newptr);
int OB_ListCheck(OBJLIST *l,OBJLIST *o);
int LLcache_ALC();

int OB_Fore(OBJECT *objsel);   // Move object forwards in the list
int OB_Back(OBJECT *objsel);   // Move object back in the list

void FreePockets(OBJECT *obj);
void ShrinkSystem(OBJECT *o);

extern OBJECT *GetRawSolidObject(int x,int y, OBJECT *except);
extern TILE *GetTile(int x,int y);
extern void NPC_ReadWipe(OBJECT *o, int wflags);
extern void newUUID(char strout[37]);
static void makeUID(char strout[UUID_SIZEOF], const char *name);


extern char fullrestore;     // Be less thorough if we're only reloading
extern void Call_VRM(int i);
extern OBJECT *current_object;
extern OBJECT *victim;
OBJLIST *MasterList=NULL;

#define LLCACHE_MAX 16

static void *llcache[LLCACHE_MAX][2];

#define NPC_ReadWipe(x,y) ;

// code

/*
 *    OB_Check()  -  Verify the object pointer is valid
 */

int OB_Check(OBJECT *o)
{
OBJLIST *temp;
int ctr;

if(!o)
	return 0;	// NULL

if(o == syspocket)	// This is independently created but still valid
	return 1;

// Make sure it's in the masterlist

for(temp=MasterList;temp;temp=temp->next) {
	if(temp->ptr == o) {
		return 1;
	}
}

// Oh ffuk, it's not in the masterlist.  This could be nasty

// Wait, is it a decorative?
if(o->flags & IS_DECOR) {
		return 1;
}

for(ctr=0;ctr<CHtot;ctr++) {
	if(o == &CHlist[ctr]) {
		Bug("%x refers to a Template but isn't decorative!\n",o);
		Bug("%x is called '%s'\n",o,o->name);
		return 1;
	}
}

// Worst fears are confirmed..
Bug("FOREIGN OBJECT 0x%x DETECTED\n",o);
if(lastvrmcall)
	Bug("Was passed as a parameter from function %s\n",lastvrmcall);

ctr=0;
for(temp=ActiveList;temp;temp=temp->next) {
	if(temp->ptr == o) {
		ctr++;
	}
}

if(ctr) {
	Bug("PRESENT IN ACTIVELIST (%d times)\n",ctr);
} else {
	Bug("Not present in ActiveList\n");
}

if(probeInvalidObject) {
	Bug("Probing object:\n");
	Bug("o->name = %x:\n",o->name);
	Bug("o->name = %s:\n",o->name);
	Bug("o->personalname = %x:\n",o->personalname);
	Bug("o->personalname = %s:\n",o->personalname);
}

return 0;
}


int OB_ListCheck(OBJLIST *l,OBJLIST *o)
{
OBJLIST *temp;

if(!o)
	return 0;	// NULL

for(temp=l;temp;temp=temp->next)
	if(temp == o)
		return 1;

ilog_printf("Il Problemo: dumping to log\n");

for(temp=l;temp;temp=temp->next)
	ilog_quiet("%x,%x\n",temp,temp->ptr);

return 0;
}



/*
 *    OB_Alloc()  -  Allocate a new Object in the linked list
 */

OBJECT *OB_Alloc()
{
OBJECT *temp;

temp=(struct OBJECT *)M_get(1,sizeof(OBJECT));
temp->next=NULL;

LL_Add(&curmap->object,temp);
ML_Add(&MasterList,temp);

temp->uid[0]=0;
temp->sdir=0;
temp->sptr=0;
temp->tag=0;
temp->target.objptr = NULL;
temp->parent.objptr = NULL;
temp->activity = -1;
temp->flags |= IS_ON;

// Allocate sublayers

temp->personalname=(char *)M_get(32,sizeof(char));
temp->funcs = (FUNCS *)M_get(1,sizeof(FUNCS));
OB_Funcs(temp);

temp->stats = (STATS *)M_get(1,sizeof(STATS));
temp->maxstats = (STATS *)M_get(1,sizeof(STATS));

//temp->wield = (OBJECT **)M_get(W_SIZE,sizeof(struct OBJECT *));
temp->wield = (WIELD *)M_get(1,sizeof(struct WIELD));
temp->schedule = (SCHEDULE *)M_get(24,sizeof(SCHEDULE));
temp->user = (USEDATA *)M_get(1,sizeof(USEDATA));
temp->user->npctalk=NULL;

for(int ctr=0;ctr<24;ctr++)
	temp->schedule[ctr].active=0;

// Set up labels

temp->labels = (CHAR_LABELS *)M_get(1,sizeof(CHAR_LABELS));
temp->labels->rank=NOTHING;
temp->labels->race=NOTHING;
temp->labels->party=NOTHING;
if(in_editor)
	temp->labels->location=NULL; // Editor prefers NULL
else
	temp->labels->location=NOTHING; // Game prefers an empty string

// Return the address of the new entry
return temp;
}

/*
 *    OB_Funcs - Set up the string pointers to point to the strings
 */

void OB_Funcs(OBJECT *temp)
{
temp->uidptr = &temp->uid[0];
temp->funcs->suse = &temp->funcs->use[0];
temp->funcs->stalk = &temp->funcs->talk[0];
temp->funcs->skill = &temp->funcs->kill[0];
temp->funcs->slook = &temp->funcs->look[0];
temp->funcs->sstand = &temp->funcs->stand[0];
temp->funcs->shurt = &temp->funcs->hurt[0];
temp->funcs->sstand = &temp->funcs->stand[0];
temp->funcs->sinit = &temp->funcs->init[0];
temp->funcs->sattack= &temp->funcs->attack[0];
temp->funcs->shorror = &temp->funcs->horror[0];
temp->funcs->sresurrect = &temp->funcs->resurrect[0];
temp->funcs->swield = &temp->funcs->wield[0];
temp->funcs->squantity= &temp->funcs->quantity[0];
temp->funcs->sget= &temp->funcs->get[0];
temp->funcs->suser1 = &temp->funcs->user1[0];
temp->funcs->suser2 = &temp->funcs->user2[0];
}

/*
 *    OB_Free()  -  Destroy the specified entry in the list and relink it
 *                  May take a while
 */

void OB_Free(OBJECT *ptr)
{
// We cannot free the anchor (simplifies the logic)

if(ptr == curmap->object)
	ithe_panic("OB_Free: Attempted to free OB_anchor",NULL);

// We certainly cannot do this either

if(!ptr)
	ithe_panic("OB_Free: Attempted to free NULL pointer:",NULL);

// First remove the object
LL_Remove(&curmap->object,ptr);

// Then remove any objects inside it
FreePockets(ptr);

// Then Deallocate it
LL_Kill(ptr);
}


/*
 *    LL_Add - Generalised linked-list add
 */

void LL_Add(OBJECT **base,OBJECT *object)
{
OBJECT *temp;

if(!base)
	ithe_panic("LL_Add: Attempted to add object to NULL",object->name);

if(!object)
	ithe_panic("LL_Add: Attempted to add NULL object to list",NULL);

// Terminate the object, since it will be the end of the list.
object->next = NULL;
temp =*base;

// No objects in the list?

if(!temp)
	{
	*base = object;
	return;
	}

// Only one?

if(!temp->next)
	{
	temp->next = object;
	return;
	}

// Handle for any other number of objects.


#ifdef PROFILE_LLADD
// Profiling
static int peak=0;
for(ctr=0;temp->next;temp=temp->next) ctr++;
if(++ctr>peak)
	{
	ilog_quiet("LL_Add: %d in list\n",ctr);
	peak=ctr;
	}
#else
// Release
for(;temp->next;temp=temp->next);
temp->next = object;
#endif
temp->next = object;
}

/*
 *    LL_Add2 - Generalised linked-list add (add as second from last)
 */

void LL_Add2(OBJECT **base,OBJECT *object)
{
OBJECT *temp;

if(!base)
	ithe_panic("LL_Add2: Attempted to add object to NULL",object->name);

if(!object)
	ithe_panic("LL_Add2: Attempted to add NULL object to list",NULL);

temp =*base;

// No objects in the list?

if(!temp)
	{
	*base = object;
	return;
	}

// Only one?

if(!temp->next)
	{
	*base = object;
	object->next=temp;
	temp->next = NULL;
	return;
	}

// Handle for any other number of objects.

for(;temp->next->next;temp=temp->next);
object->next=temp->next;
temp->next= object;
}

/*
 *    LL_Remove - Generalised linked-list free
 */

void LL_Remove(OBJECT **base,OBJECT *object)
{
OBJECT *temp;
char found;

if(!base)
	ithe_panic("LL_Remove: Attempted to remove object from NULL",object->name);

if(!object)
	ithe_panic("LL_Remove: Attempted to remove NULL object from list",NULL);

if(!*base)
	ithe_panic("LL_Remove: Attempted to remove object from empty list",object->name);

temp=*base;

// Look for it, to make sure it is there.

found=0;
for(;temp;temp=temp->next)
	if(temp == object)
		found = 1;

if(!found)
	{
	if(*base)
		ilog_quiet("list = %s\n",(*base)->name);
	else
		ilog_quiet("list = NULL\n");
	ithe_panic("LL_Remove: Could not find object in the list",object->name);
	}

temp=*base;

// Is it the first object?

if(temp == object)
	{
	*base = object->next;
	object->next = NULL;
	return;
	}

// Is it the second object?

if(temp->next == object)
	{
	temp->next = object->next;
	object->next = NULL;
	return;
	}

// Any other number

for(;temp->next;temp=temp->next)
	if(temp->next == object)
		{
		temp->next = object->next;
		object->next = NULL;
		return;
		}

ithe_panic("LL_Remove: Whoops!  You really messed that up Joe!",NULL);
}


/*
 *  LL_Kill - Release the memory held by an object and dependent structs.
 *            This is a low-level operation but has error checking.
 */

void LL_Kill(OBJECT *obj)
{
if(!obj)
	ithe_panic("LL_Kill: Object is NULL",NULL);
if(obj->next)
	ithe_panic("LL_Kill: Object is part of a linked-list",obj->name);
if(obj->pocket.objptr)
	ithe_panic("LL_Kill: Object has items in pocket",obj->name);

NPC_ReadWipe(obj,1);      // Must be before STATS are removed

if(obj->stats)
	{
	M_free(obj->stats);
	obj->stats = NULL;
	}

if(obj->maxstats)
	{
	M_free(obj->maxstats);
	obj->maxstats = NULL;
	}

if(obj->funcs)
	{
	M_free(obj->funcs);
	obj->funcs = NULL;
	}

if(obj->wield)
	{
	M_free(obj->wield);
	obj->wield = NULL;
	}

if(obj->schedule)
	{
	M_free(obj->schedule);
	obj->schedule=NULL;
	}

if(obj->user)
	{
	M_free(obj->user);
	obj->user=NULL;
	}

if(obj->labels)
	{
	M_free(obj->labels);
	obj->labels=NULL;
	}

// Check all dependencies

ML_Del(&ActiveList,obj); // De-register from activity list (if present)

// Make sure it isn't somehow lurking on the map
if(obj->x>=0 && obj->y>=0 && obj->x<curmap->w && obj->y<curmap->h)
	{
	// JM: We may also need to use the 'z' coordinate to store the
	// map the object was last seen on.  Then we only try to remove it
	// from the world if it matches the map we're on.

	if(LL_Query(&curmap->objmap[MAP_POS(obj->x,obj->y)],obj))
		{
		LL_Remove(&curmap->objmap[MAP_POS(obj->x,obj->y)],obj);
		ilog_quiet("Had to remove Lurking object %s (%x) at %d,%d\n",obj->name,obj,obj->x,obj->y);
		}
	}

if(!NoDeps)
	CheckDepend(obj);
ML_Del(&MasterList,obj);

if(current_object == obj)
	current_object = NULL;
if(victim== obj)
	victim = NULL;
if(person== obj)
	person = NULL;



#ifdef WITH_PREJUDICE
memset(obj,0xff,sizeof(OBJECT)); // F--- it for good
#endif

M_free(obj);
}

/*
 *    LL_Query - Is it in the list?
 */

int LL_Query(OBJECT **base,OBJECT *object)
{
OBJECT *temp;

if(!base)
	ithe_panic("LL_Query: Attempted to query NULL list",NULL);

if(!object)
	ithe_panic("LL_Query: Attempted to query NULL object in list",NULL);


temp=*base;
for(;temp;temp=temp->next)
	if(temp == object)
		return 1;

return 0;
}


/*
 *      ML_Add - Add an object pointer to an object chain
 */

void ML_Add(OBJLIST **m,OBJECT *obj)
{
OBJLIST *t,*end,*newptr;

if(!obj)
    ithe_panic("ML_Add: Tried to add NULL to list\n",NULL);

#ifdef LOG_MLADD
if(m == &MasterList)
    ilog_quiet("Adding %x to Masterlist\n",obj);
if(m == &ActiveList)
    ilog_quiet("Adding %x to Activelist\n",obj);
#endif

// First object.

if(!*m)
    {
    *m = (OBJLIST *)M_get(1,sizeof(OBJLIST));
    (*m)->ptr = obj;
    (*m)->next = NULL;
#ifdef ML_CACHE
	LLcache_update(m,*m);
#endif
    AL_dirty = 1;
    return;
    }

// Don't add if it's already there

#ifdef CHECK_MASTERLIST
for(t = *m;t;t=t->next)
	if(t->ptr == obj)
		{
		Bug("Object %s already in list\n",obj->name);
		return;
		}
#endif

// More than one object in the list.. create it.

newptr = (OBJLIST *)M_get(1,sizeof(OBJLIST));
newptr->ptr = obj;
newptr->next = NULL;

// Check the cache.

#ifdef ML_CACHE
end=(OBJLIST *)LLcache_get(m);
if(end)
	{
#ifdef CACHE_DOUBLECHECK
	if(!OB_ListCheck(*m,end))
		{
			Bug("Cache Invalid (%x) %x:\n",end,end->ptr);
		if(m == &MasterList)
			Bug("Cache Invalid for MasterList\n");
		if(m == &ActiveList)
			Bug("Cache Invalid in ActiveList\n");
		LLcache_update(m,NULL);
		goto unlawful;
		}
#endif

	if(end->next)
		{
		Bug("Cache Expired\n");
		LLcache_update(m,NULL);
		}
	else
		{
		end->next = newptr;
		LLcache_update(m,newptr);
		AL_dirty = 1;
		return;
		}
	}
#endif

#ifdef CACHE_DOUBLECHECK
unlawful:
#endif

// Add the object to the list the slow way.

for(t = *m;t->next;t=t->next);
t->next = newptr;
#ifdef ML_CACHE
LLcache_update(m,newptr);
#endif
AL_dirty = 1;

return;
}


/*
 *    ML_Del - Free an object from an object chain
 */

void ML_Del(OBJLIST **m,OBJECT *object)
{
OBJLIST *temp,*t2;

if(!object)
    ithe_panic("ML_Del: Attempted to remove NULL object from list",NULL);

// If the list is empty, abort
if(!*m)
	return;

#ifdef LOG_MLADD
if(m == &MasterList)
    ilog_quiet("De-registering %x from Masterlist\n",object);
else
    ilog_quiet("De-registering %x from Activelist\n",object);
#endif

#ifdef ML_CACHE
LLcache_update(m,NULL);
#endif

temp=*m;

// Check
if(!temp)
	return;

// Is it the first object?

if(temp->ptr == object)
	{
	*m = temp->next;
	temp->next = NULL;
	M_free(temp);
	AL_dirty = 1;
#ifdef ML_CACHE
	LLcache_update(m,NULL);
#endif

#ifdef ML_TELLALL
	if(m == &MasterList)
		printf(">> removed first object %x from masterlist\n", object);
	else
		printf(">> removed first object %x from activelist\n", object);
	if(*m)
		printf(">> new head is %x\n", (*m)->ptr);
#endif

	return;
	}

// Check
if(!temp->next)
	return;

// Is it the second object?

if(temp->next->ptr == object)
	{
	t2=temp->next;
	temp->next = temp->next->next;
	t2->next = NULL;
	M_free(t2);
	AL_dirty = 1;
#ifdef ML_CACHE
	LLcache_update(m,NULL);
#endif

#ifdef ML_TELLALL
	if(m == &MasterList)
		printf(">> removed second object %x from masterlist\n", object);
	else
		printf(">> removed second object %x from activelist\n", object);
	if(temp->next)
		printf(">> replacement is %x\n", temp->next->ptr);
#endif
	return;
	}

// Any other number

for(;temp->next;temp=temp->next)
	if(temp->next->ptr == object)
		{
		t2 = temp->next;
		temp->next = temp->next->next;
		t2->next = NULL;
		M_free(t2);
		AL_dirty = 1;
#ifdef ML_CACHE
		LLcache_update(m,NULL);
#endif
#ifdef ML_TELLALL
		if(m == &MasterList)
			printf(">> removed other object %x from masterlist\n", object);
		else
			printf(">> removed other object %x from activelist\n", object);
		if(temp->next)
			printf(">> replacement is %x\n", temp->next->ptr);
#endif
		return;
		}

return;
//ithe_panic("ML_Del: Oh no!","The chain has become corrupted");
}

/*
 *    ML_Query - Count instances of an object type by name
 */

int ML_Query(OBJLIST **m,char *name)
{
OBJLIST *temp;
int count;

if(!name)
	{
	Bug("ML_Query for NULL");
	return 0;
	}

temp=*m;

count=0;
for(;temp;temp=temp->next)
	if(temp->ptr)
		if(!istricmp(temp->ptr->name,name))
			count++;
return count;
}


/*
 *    ML_InList - Is an object in this object chain?
 */

int ML_InList(OBJLIST **m,OBJECT *o)
{
OBJLIST *temp;

if(!o)
	{
	Bug("ML_Check for NULL");
	return 0;
	}

for(temp=*m;temp;temp=temp->next)
	if(temp->ptr == o)
		return 1;
return 0;
}

// Add to active list (if the object is active)

void AL_Add(OBJLIST **a,OBJECT *o)
{
// If it has an action, or any of these properties, allow it to be active.
if(o->activity > 0 || (o->flags & IS_REPEATSPIKE) || GetNPCFlag(o,IS_BIOLOGICAL) || o->stats->radius > 0) {

	// Do NOT add it twice!
	if(ML_InList(&ActiveList,o)) {
		return;
	}

	ML_Add(a,o);
	AL_dirty = 1;
}
}

// Remove from active list (only if the object is NOT active)

void AL_Del(OBJLIST **a,OBJECT *o)
{
// If it has an action, or any of these properties, don't disable it
if(o->activity > 0 || (o->flags & IS_REPEATSPIKE) || GetNPCFlag(o,IS_BIOLOGICAL) || o->stats->radius > 0) {
	return;
}

ML_Del(a,o);
}



JOURNALENTRY *J_Add(int ID, int day, const char *date)
{
char *nameptr = NULL;
char *textptr = NULL;
char *titleptr = NULL;
char *tagptr = NULL;
JOURNALENTRY *ptr;

if(!date)
	date="";

if(ID >= 0 && ID < STtot) {
	nameptr = STlist[ID].name;
	textptr = STlist[ID].data;
	titleptr = STlist[ID].title;
	tagptr = STlist[ID].tag;
}

if(!Journal) {
//printf("Journal: Add 1st entry %d\n",ID);
	ptr = (JOURNALENTRY *)M_get(1,sizeof(JOURNALENTRY));
	if(ptr) {
		ptr->id=ID;
		SAFE_STRCPY(ptr->date,date);
		if(ID != -1) {
			ptr->day = day;
			ptr->name = nameptr;
			ptr->text = textptr; 
			ptr->title = titleptr;
			ptr->tag = tagptr;
		}
		Journal=ptr;
	}
	return ptr;
}

//printf("Journal: Add next entry %d\n",ID);

if(Journal->id == ID && ID >=0)
	{
//	printf("Already present\n");
	return NULL;
	}

for(ptr = Journal;ptr->next;ptr=ptr->next) {
//	printf("Entry %d is present\n",ptr->id);
	if(ptr->id >= 0 && ptr->id == ID) {
		return NULL;  // Not Permitted
	}
}

// Check the one at the end
if(ptr->id >= 0 && ptr->id == ID) {
	return NULL;  // Not Permitted
}

//printf("Entry not alread present\n");

ptr->next = (JOURNALENTRY *)M_get(1,sizeof(JOURNALENTRY));
if(!ptr->next)
	return NULL;
ptr=ptr->next;

//printf("Added\n");

ptr->id=ID;
SAFE_STRCPY(ptr->date,date);
if(ID != -1)
	{
	ptr->day = day;
	ptr->name = nameptr;
	ptr->text = textptr; 
	ptr->title = titleptr;
	ptr->tag = tagptr;
	}

return ptr;
}


void J_Del(JOURNALENTRY *j)
{
if(!j)
	return;

if(Journal == j) {
	Journal=j->next;
	if(j->id < 0) {
		if(j->name)
			M_free(j->name);
		if(j->text)
			M_free(j->text);
		if(j->title)
			M_free(j->title);
		if(j->tag)
			M_free(j->tag);
	}
	M_free(j);
	return;
}


for(JOURNALENTRY *ptr = Journal;ptr->next;ptr=ptr->next)
	if(ptr->next == j) {
		ptr->next = j->next;
		if(j->id < 0) {
			if(j->name)
				M_free(j->name);
			if(j->text)
				M_free(j->text);
			if(j->title)
				M_free(j->title);
			if(j->tag)
				M_free(j->tag);
		}
		M_free(j);
		return;
	}

}

JOURNALENTRY *J_Find(const char *name) {
	JOURNALENTRY *ptr;

	if(!Journal) {
		return NULL;
	}

	if(!stricmp(Journal->name, name)) {
		return Journal;
	}

	for(ptr = Journal;ptr;ptr=ptr->next) {
		if(!stricmp(ptr->name, name)) {
			return ptr;
		}
	}

	return NULL;
}


void J_Free()
{
while(Journal) J_Del(Journal);
}



/* ========================== Non-core functionality ====================== */

/*
 *    OB_Init()  -  Initialize or re-initialise an Object.
 */

int OB_Init(OBJECT *objsel,char *name)
{
int ctr;
int seq;

ctr = getnum4char(name);                // Find the character's name
if(ctr == -1)
	{
	Bug("OB_Init: Could not find object in Section: CHARACTERS\n");
	Bug("         '%s'\n",name);
	return 0;
	}
objsel->name = CHlist[ctr].name;        // Update object's name from static

// Generate a UID for it
if(!objsel->uid[0]) {
	makeUID(objsel->uid, objsel->name);
}

if(CHlist[ctr].flags & IS_DECOR)
	ithe_panic("OB_Init: Didn't catch DECOR object!",name);

// Now the object will take on the properties of the new character

// Copy the direction cache
objsel->activity=CHlist[ctr].activity;

memcpy(objsel->dir,CHlist[ctr].dir,sizeof(CHlist[ctr].dir));
strcpy(objsel->personalname,CHlist[ctr].personalname);


// Set up the stats and function systems
memcpy(objsel->stats,CHlist[ctr].stats,sizeof(STATS));
objsel->user->oldhp = objsel->stats->hp; // Keep these in step

objsel->cost = CHlist[ctr].cost;

// If it is a system object, we only want to initialise the stats.
if(objsel->flags & IS_SYSTEM)
	{
	ShrinkSystem(objsel);
	objsel->curdir=0;
	seq = objsel->dir[objsel->curdir];

	// Set up the appearance
	objsel->form = &SQlist[seq];
	objsel->hotx = objsel->form->hotx;
	objsel->hoty = objsel->form->hoty;
	objsel->desc = CHlist[ctr].desc;                // Long text description
	objsel->shortdesc = CHlist[ctr].shortdesc;      // Short text description

	objsel->flags = CHlist[ctr].flags;
	objsel->stats->npcflags = CHlist[ctr].stats->npcflags;

//	memcpy(objsel->funcs,CHlist[ctr].funcs,sizeof(FUNCS));
//	OB_Funcs(objsel);
	objsel->sptr = 0; // Just in case
	return 1;
	}

memcpy(objsel->maxstats,CHlist[ctr].maxstats,sizeof(STATS));
memcpy(objsel->funcs,CHlist[ctr].funcs,sizeof(FUNCS));
OB_Funcs(objsel);

// Labels
objsel->labels->race=CHlist[ctr].labels->race;
objsel->labels->rank=CHlist[ctr].labels->rank;
objsel->labels->party=CHlist[ctr].labels->party;

// Editor-specific things to do with decorative objects
objsel->user->edecor = CHlist[ctr].user->edecor;
// If it thinks it has already been shrunk, stop it from thinking that
if(objsel->user->edecor == 2)
	objsel->user->edecor = 1;

// Inherit the default schedule
if(objsel->schedule)
	{
	if(CHlist[ctr].schedule)
		{
		memcpy(objsel->schedule,CHlist[ctr].schedule,sizeof(SCHEDULE)*24);
/*
		ilog_quiet("%s inherits a schedule\n",objsel->name);
		for(seq=0;seq<24;seq++)
			ilog_quiet("%d:%d\n",objsel->schedule[seq].hour,objsel->schedule[seq].minute);
*/
		}
	}

// If critical maximum statistics are zero, improvise

if(objsel->maxstats->hp == 0)
	objsel->maxstats->hp = objsel->stats->hp;
if(objsel->maxstats->str == 0)
	objsel->maxstats->str = objsel->stats->str;
if(objsel->maxstats->dex == 0)
	objsel->maxstats->dex = objsel->stats->dex;
if(objsel->maxstats->intel == 0)
	objsel->maxstats->intel = objsel->stats->intel;

// Get the sequence index
objsel->curdir&=3; // Only 4 directions currently
seq = objsel->dir[objsel->curdir];

// Set up the animation stats
objsel->sdir = 1;
objsel->sptr = 0;

// Set up the appearance
objsel->form = &SQlist[seq];
objsel->hotx = objsel->form->hotx;
objsel->hoty = objsel->form->hoty;
objsel->desc = CHlist[ctr].desc;                // Long text description
objsel->shortdesc = CHlist[ctr].shortdesc;      // Short text description
objsel->light = CHlist[ctr].light;


if(ML_InList(&ActiveList,objsel))  // Make sure it isn't there yet
	ML_Del(&ActiveList,objsel);

// Set up the physics
if(!fullrestore) {
	objsel->flags = CHlist[ctr].flags;
	objsel->stats->npcflags = CHlist[ctr].stats->npcflags;
	SubAction_Wipe(objsel);
	ActivityNum(objsel,CHlist[ctr].activity,NULL); // Set default action
	NPC_ReadWipe(objsel,1);	
	if((objsel->flags & IS_TRIGGER) && (objsel->flags & IS_REPEATSPIKE)) {
		AL_Add(&ActiveList, objsel);
	}
}

objsel->flags |= IS_ON; // Make sure it's on or it will get deleted

// Set up regions according to object definition
memcpy(&objsel->hblock,&CHlist[ctr].hblock,sizeof(objsel->hblock));
memcpy(&objsel->vblock,&CHlist[ctr].vblock,sizeof(objsel->vblock));
memcpy(&objsel->harea,&CHlist[ctr].harea,sizeof(objsel->harea));
memcpy(&objsel->varea,&CHlist[ctr].varea,sizeof(objsel->varea));

Init_Areas(objsel);
CalcSize(objsel);

return 1;
}


OBJECT *OB_Copy(OBJECT *obj)
{
OBJECT *objsel;

if(!obj)
	return NULL;

// A decor?  This is outside my jurisdiction
if(obj->flags & IS_DECOR)
	return OB_Decor(obj->name);

objsel = OB_Alloc();
OB_Init(objsel,obj->name);

// Set up the stats and function systems
memcpy(objsel->stats,obj->stats,sizeof(STATS));
memcpy(objsel->maxstats,obj->maxstats,sizeof(STATS));
memcpy(objsel->funcs,obj->funcs,sizeof(FUNCS));
OB_Funcs(objsel);

// Labels
objsel->labels->race=obj->labels->race;
objsel->labels->rank=obj->labels->rank;
objsel->labels->party=obj->labels->party;

objsel->user->edecor = obj->user->edecor;


// Set up the appearance
objsel->curdir = obj->curdir;
objsel->form = obj->form;
objsel->hotx = objsel->form->hotx;
objsel->hoty = objsel->form->hoty;
objsel->sdir = 1;
objsel->sptr = 0;

objsel->desc = obj->desc;                // Long text description
objsel->shortdesc = obj->shortdesc;      // Short text description

objsel->tag = obj->tag;

// Set up the physics
if(!fullrestore)
	{
	objsel->flags = obj->flags;
	objsel->stats->npcflags = obj->stats->npcflags;
	objsel->light = obj->light;
	SubAction_Wipe(objsel);
	ActivityNum(objsel,obj->activity,NULL); // Set default action
	NPC_ReadWipe(objsel,1);
	}

return objsel;
}



/*
 *    OB_Decor()  -  Create a decorative object.
 *                   You can have millions of these, but they must ALWAYS
 *                   be the top object in the list.
*/

OBJECT *OB_Decor(char *name)
{
int ctr;

ctr = getnum4char(name);                // Find the character's name
if(ctr == -1)
	return NULL;
//	ithe_panic("OB_Decor: Could not find object in Section: CHARACTERS!",name);

return OB_Decor_num(ctr);
}

/*
 *    OB_Decor_num()  -  Create a decorative object by number.
*/

OBJECT *OB_Decor_num(int ctr)
{
OBJECT *objsel;
int seq;

if(!(CHlist[ctr].flags & IS_DECOR))
	return NULL;

CHlist[ctr].next = NULL;
CHlist[ctr].pocket.objptr = NULL;

objsel = &CHlist[ctr];

// Get the sequence index
seq = objsel->dir[0];

// Set up the animation stats
objsel->sdir = 1;
objsel->sptr = 0;

// Set up the appearance
objsel->form = &SQlist[seq];
objsel->hotx = objsel->form->hotx;
objsel->hoty = objsel->form->hoty;

objsel->flags |= IS_ON;

// Set up regions according to object definition
memcpy(&objsel->hblock,&CHlist[ctr].hblock,sizeof(objsel->hblock));
memcpy(&objsel->vblock,&CHlist[ctr].vblock,sizeof(objsel->vblock));
memcpy(&objsel->harea,&CHlist[ctr].harea,sizeof(objsel->harea));
memcpy(&objsel->varea,&CHlist[ctr].varea,sizeof(objsel->varea));

Init_Areas(objsel);
CalcSize(objsel);

return objsel;
}

/*
 *    OB_SetDir()  -  Change the direction that a character is facing
 *                    objsel is the pointer in the list, dir the direction
 *                    it is facing.  See CHAR_L CHAR_R etc..
 */

int OB_SetDir(OBJECT *objsel,int dir, int flags)
{
int seq,cx,cy,w,h,x,y,nochange;
OBJECT *temp;
TILE *tile;
int olddir;

if(dir>3)
	{
	Bug("Attempt to set %s to face invalid direction %d!\n",objsel->name,dir);
	return 0;
	}

nochange=0;

if(objsel->curdir == dir)   // abort if we're already the candidate
	if(objsel->form)        // and it does exist
		if(flags & FORCE_FRAME)
			nochange=1;		// This is necessary for the cow and dragons

// Get the sequence index
seq = objsel->dir[dir];
olddir=objsel->curdir;  // In case we need to revert
objsel->curdir = dir;

// If object has been changed to another shape with SetSequence, force change
if(objsel->engineflags & ENGINE_DIDSETSEQUENCE)
	{
	nochange=0;
	objsel->engineflags &= ~ENGINE_DIDSETSEQUENCE;
	}

// If it's a large object, find out if the new shape would actually fit
if(!(flags & FORCE_SHAPE))           // Sometimes we won't want to do this
	if(objsel->flags & IS_LARGE)
		{
		// Find the polarity of the new object
		if(dir < CHAR_L)  //(U,D,L,R)
			{
			w = objsel->vblock[BLK_W];
			h = objsel->vblock[BLK_H];
			x = objsel->vblock[BLK_X];
			y = objsel->vblock[BLK_Y];
			}
		else
			{
			w = objsel->hblock[BLK_W];
			h = objsel->hblock[BLK_H];
			x = objsel->hblock[BLK_X];
			y = objsel->hblock[BLK_Y];
			}

		// Now make sure it doesn't hit something else
		// If it hits itself that's OK
		for(cx=0;cx<w;cx++)
			for(cy=0;cy<h;cy++)
				{
				temp = GetRawSolidObject(cx+x+objsel->x,cy+y+objsel->y,objsel);
				if(temp)
					{
					objsel->curdir = olddir; // Obstruction
					return 0;
					}
				tile=GetTile(cx+x+objsel->x,cy+y+objsel->y);
				if(tile->flags & IS_SOLID)
					{
					objsel->curdir = olddir; // Obstruction
					return 0;
					}
				}
		}

// Set up new direction up

if(!nochange)
	{
	objsel->form = &SQlist[seq];

	// Set up the animation stats
	objsel->sdir = 1;
	if(objsel->sptr >= objsel->form->frames)
		objsel->sptr = 0;

	objsel->hotx = objsel->form->hotx;
	objsel->hoty = objsel->form->hoty;
	//objsel->w    = objsel->form->seq[0]->w;
	//objsel->h    = objsel->form->seq[0]->h;
	}
CalcSize(objsel);

return 1; // Success
}



int OB_TurnDir(OBJECT *objsel,int dir)
{
int ow,oh,osx,osy,ret;
int olddir;
//int w,h,x,y;
//char *dirn[]={"Up","Down","Left","Right"};

if(dir>3)
	{
	Bug("Attempt to set %s to face invalid direction %d!\n",objsel->name,dir);
	return 0;
	}

olddir=objsel->curdir;  // In case we need to revert
objsel->curdir = dir;

ow = objsel->mw;
oh = objsel->mh;

// Try just changing direction
if(OB_SetDir(objsel,dir,0))
	return 1;

ret=0;

// If it's a large object, find out if the new shape would actually fit
if(objsel->flags & IS_LARGE)
	{
/*
	// Find the polarity of the new object
	if(dir < CHAR_L)  //(U,D,L,R)
		{
		w = objsel->vblock[BLK_W];
		h = objsel->vblock[BLK_H];
		x = objsel->vblock[BLK_X];
		y = objsel->vblock[BLK_Y];
		}
	else
		{
		w = objsel->hblock[BLK_W];
		h = objsel->hblock[BLK_H];
		x = objsel->hblock[BLK_X];
		y = objsel->hblock[BLK_Y];
		}
*/

	// Figure out where it should go, to help large objects turning
	osx = osy = 0;
//	if(olddir < CHAR_L && dir == CHAR_R)
		osy = oh - ow;
//	if(olddir > CHAR_D && dir == CHAR_U)
//		osy = oh - ow;

	// Push it into place
	OB_SetDir(objsel,dir,1);
//	irecon_printf("Want %s: Trying offset %d,%d\n",dirn[dir],osx,osy);
	ret = MoveObject(objsel,objsel->x+osx,objsel->y+osy,1);

	if(!ret)
		{
		// Shit, it didn't work
		OB_SetDir(objsel,olddir,1);
		return 0;
		}
	}

return ret;
}



/*
 *    OB_SetSeq()-  Temporarily set the object's display frame to something
 *                  unusual.  The name is the SEQUENCE we select
 */

void OB_SetSeq(OBJECT *objsel,char *seqname)
{
int seq;

if(!objsel)
    {
    Bug("SetSequence passed NULL object\n");
    return;
    }

// Get the sequence index
seq = getnum4sequence(seqname);
if(seq == -1)
	{
	Bug("Could not find sequence %s in SECTION:SEQUENCES\n",seqname);
	return;
	}

objsel->form = &SQlist[seq];
objsel->hotx = objsel->form->hotx;
objsel->hoty = objsel->form->hoty;
objsel->w    = objsel->form->seq[0]->w;
objsel->h    = objsel->form->seq[0]->h;

// Set up the animation stats
objsel->sdir = 1;
objsel->sptr = 0;

// We've just done setsequence, we need to take this into account when
// we next do setdir()
objsel->engineflags |= ENGINE_DIDSETSEQUENCE;

}

/*
 *    OB_Back()  -  Move an object backwards in the list
 */

int OB_Back(OBJECT *objsel)
{
OBJECT *temp;

if(!curmap->object->next)
	return 1;               // Not enough sprites to do this
if(!curmap->object->next->next)
	return 1;               // Not enough sprites to do this

if(!objsel)
	return 2;               // No sprite selected

if(objsel==curmap->object)
	return 3;               // Not permitted

// Look for a link to the selected object two objects away.

for(OBJECT *tt=curmap->object;tt->next->next;tt=tt->next)
	if(tt->next->next == objsel)
		{
		temp=objsel->next;              // Preserve original link
		objsel->next=tt->next;          // link into next part of list
		tt->next=objsel;                // link into the list itself
		objsel->next->next=temp;        // Restore old link
		return 0;                       // Exit
		}
return 0;
}

/*
 *    OB_Fore()  -  Move an object forward in the list
 */

int OB_Fore(OBJECT *objsel)
{
OBJECT *temp;

if(!curmap->object->next)
	return 1;               // Not enough sprites to do this
if(!curmap->object->next->next)
	return 1;               // Not enough sprites to do this

if(!objsel)
	return 2;               // No sprite selected

if(objsel==curmap->object)
	return 3;               // Not permitted

// Look for a link to the selected object two objects away.

for(OBJECT *tt=curmap->object;tt->next->next;tt=tt->next)
	if(tt->next == objsel)
		{
		temp=tt->next->next->next;      // Preserve original link
		tt->next=tt->next->next;        // Change first link
		tt->next->next=objsel;          // Change second link
		objsel->next=temp;              // Change third link
		return 0;
		}
return 0;
}


/*
 * OB_Previous - Pointer to previous object
 */

OBJECT *OB_Previous(OBJECT *a)
{
OBJECT *temp;
for(temp=curmap->object;temp->next;temp=temp->next)
    if(temp->next==a)
        return(temp);
ithe_panic("OB_Previous: Ah shit","You really messed that up Joe!");
return NULL;
}


void FreePockets(OBJECT *obj)
{
OBJECT *temp,*next;

// Sanity check
if(!obj)
	ithe_panic("FreePockets: NULL pointer encountered",NULL);

// Silently ignore object without pocket
if(!obj->pocket.objptr)
	return;

temp = obj->pocket.objptr;
while(temp)
	{
	next = temp->next;
	LL_Remove(&obj->pocket.objptr,temp);       // Unlink the object
	if(temp->pocket.objptr)                    // If it has objects inside
		FreePockets(temp); 	            // Recurse inside it
	LL_Kill(temp);                  	    // Free the memory
	// Go to next object
	temp = next;
	};
}

int LLcache_register(void *ptr)
{
int ctr;

for(ctr=0;ctr<LLCACHE_MAX;ctr++)
	if(llcache[ctr][0] == NULL)
		{
		llcache[ctr][0]=ptr;
		llcache[ctr][1]=NULL;
		return ctr;
		}
Bug("LLcache_register: Failed - Too many caches.\n");
return -1;
}

void *LLcache_get(void *ptr)
{
int ctr;

for(ctr=0;ctr<LLCACHE_MAX;ctr++)
	if(llcache[ctr][0] == ptr)
		return llcache[ctr][1];
return NULL;
}

void LLcache_update(void *ptr, void *newptr)
{
int ctr;

for(ctr=0;ctr<LLCACHE_MAX;ctr++)
	if(llcache[ctr][0] == ptr)
		{
//		ilog_printf("updating %d (%x)\n",ctr,newptr);
		llcache[ctr][1]=newptr;
		return;
		}
Bug("LLcache_upd: couldn't update %x : no such cache (%x)\n",newptr,ptr);
}

#if 0
int LLcache_invalidate(void *ptr, void *newptr)
{
int ctr,diag=0;

for(ctr=0;ctr<LLCACHE_MAX;ctr++)
	if(llcache[ctr][0] == ptr)
		{
		diag=1;
		if(llcache[ctr][1] == newptr)
			{
			ilog_printf("invalidating %d (%x)\n",ctr,newptr);
			llcache[ctr][1]=NULL;
			return 1;
			}
		}

Bug("LLcache_inv: couldn't invalidate %x\n",newptr);
if(!diag)
	Bug("Because I did not find the cache\n");
return 0;
}
#endif

int LLcache_ALC(char *fname, int line)
{
if(!OB_ListCheck(ActiveList,(OBJLIST *)llcache[1][1]))
	{
	ilog_printf("LL consistency check failed at: %s:%d\n",fname,line);
	return 0;
	}
return 1;
}

static char DecorPersonal[32]="Decorative Object - Don't Edit";

/*
 * Conserve memory by freeing substructures and using the template ones
 */

void ShrinkDecor(OBJECT *o)
{
int char_id;

if(!o)
	return;

if(in_editor != 1)		// Only in map editor
	return;

if(!o->user->edecor)	// Not decorative
	return;

if(o->user->edecor == 2)	// Already changed
	return;

char_id = getnum4char(o->name);

if(char_id == -1)
	return;

M_free(o->personalname);
o->personalname = &DecorPersonal[0];

M_free(o->funcs);
o->funcs = CHlist[char_id].funcs;

M_free(o->stats);
o->stats = CHlist[char_id].stats;

M_free(o->maxstats);
o->maxstats = CHlist[char_id].maxstats;

M_free(o->wield);
o->wield = CHlist[char_id].wield;

M_free(o->schedule);
o->schedule = CHlist[char_id].schedule;

M_free(o->labels);
o->labels = CHlist[char_id].labels;

// FInally, the User data structure
M_free(o->user);
o->user = CHlist[char_id].user;

// Mark it as pointing to the Master data structure
// (Yes, this DOES modify the master, but it really doesn't matter)
o->user->edecor = 2;
}

/*
 * Undo the 'shrinking', e.g. if we turn it into a real object again
 */

void ExpandDecor(OBJECT *o)
{
if(!o)
	return;

if(in_editor != 1)		// Only in map editor
	return;

if(o->user->edecor != 2)	// Already changed
	return;

o->personalname = (char *)M_get(1,32);

o->funcs = (FUNCS *)M_get(1,sizeof(FUNCS));
OB_Funcs(o);

o->stats = (STATS *)M_get(1,sizeof(STATS));
o->maxstats = (STATS *)M_get(1,sizeof(STATS));
//o->wield = (OBJECT **)M_get(W_SIZE,sizeof(struct OBJECT *));
o->wield = (WIELD *)M_get(1,sizeof(struct WIELD));
o->schedule = (SCHEDULE *)M_get(24,sizeof(SCHEDULE));
o->user = (USEDATA *)M_get(1,sizeof(USEDATA));
o->user->npctalk=NULL;

for(int ctr=0;ctr<24;ctr++)
	o->schedule[ctr].active=0;

// Set up labels

o->labels = (CHAR_LABELS *)M_get(1,sizeof(CHAR_LABELS));
o->labels->rank=NOTHING;
o->labels->race=NOTHING;
o->labels->party=NOTHING;
}

/*
 * Conserve memory by freeing substructures and using the template ones
 */

void ShrinkSystem(OBJECT *o)
{
if(!o)
	return;

if(o->flags & IS_SYSTEM) // Already changed
	return;

// We only need STATS for System Objects.

M_free(o->funcs);
o->funcs = NULL;

M_free(o->maxstats);
o->maxstats = NULL;

M_free(o->wield);
o->wield = NULL;

M_free(o->schedule);
o->schedule = NULL;

M_free(o->labels);
o->labels = NULL;

// FInally, the User data structure
M_free(o->user);
o->user = NULL;
}

//
//  String management
//

USERSTRING *STR_New(int length)
{
USERSTRING *u;
u=(USERSTRING *)M_get(1,sizeof(USERSTRING));
u->len=length;
u->ptr=(char *)M_get(1,length+1); // empty but fully-sized

return u;
}


void STR_Add(USERSTRING *u, const char *str)
{
int inlen,curlen;

if(!u)
	ithe_panic("Bad userstring",NULL);

if(!str)
	return; // Done it..

// Now get enough space, merge the strings and replace the original

inlen=strlen(str);
curlen=strlen(u->ptr);
if(inlen+curlen > u->len)
	{
	curlen = u->len - curlen; // Work out how much is left
	strncat(u->ptr,str,curlen); // Copy that much
	u->ptr[u->len]=0;
	}
else
	strcat(u->ptr,str);

/*
// Check, for the paranoid in me
if(strlen(u->ptr) > u->len)
	{
	CRASH();
	}
*/
}

//
//  Set the string contents
//

void STR_Set(USERSTRING *u, const char *str)
{
if(!u)
	ithe_panic("Bad userstring",NULL);

if(!str)
	return; // Done it..

u->ptr[0]=0;		// Reset to empty
STR_Add(u,str);		// Append to the emptiness
}

void STR_Free(USERSTRING *u)
{
if(!u)
	return; // Already done..

if(u->ptr)
	M_free(u->ptr);
M_free(u);
}



void makeUID(char strout[UUID_SIZEOF], const char *name) {
memset(strout,0,UUID_SIZEOF);
strncpy(strout,name,4);
// If the name is shorter than 4 characters, add underscores
while(strlen(strout)<4) {
	strcat(strout,"_");
}
strupr(strout); // Make the prefix uppercase

newUUID(&strout[4]);  // Create the GUID

if(strlen(strout) != UUID_LEN) {
	ithe_panic("Internal error: Invalid UID", strout);
}
}
