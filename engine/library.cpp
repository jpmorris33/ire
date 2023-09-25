//
//      A library of useful high-level game functions
//      These are mostly intended to be used by the scripting system
//

#define IRE_SYSTEM_MODULE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define NO_FORTIFY
#include "library.hpp"
#include "ithelib.h"
#include "media.hpp"
#include "sound.h"
#include "loadfile.hpp"
#include "loadsave.hpp"
#include "nuspeech.hpp"
#include "linklist.hpp"
#include "object.hpp"
#include "gamedata.hpp"
#include "console.hpp"
#include "newconsole.hpp"
#include "graphics/iwidgets.hpp"
#include "map.hpp"
#include "init.hpp"
#include "oscli.hpp"
#include "mouse.hpp"

#define SOUND_DIST 4 // maximum distance without attenuation
//#define SOUND_DROP 16 // 8 volume points every square
#define SOUND_DROP 8 // 8 volume points every square
#define MAXCONSOLES 16

#define CLIPMIN(a) {if(a<0) a=0;}
#define CLIPMAX(a,b) {if(a>b) a=b;}

// Variables

extern long COtot;
extern VMINT do_lightning;
extern char pending_delete;
extern char fullrestore;
extern VMINT sf_volume;
static char *nullstr="<NULL>";
static double ire_sintab[3600];
static double ire_costab[3600];
double ire_sqrt[32768];

struct PICKLISTLIST
	{
	IREPICKLIST *win;
	int *numlist;
	int nums;
	};

static IRECONSOLE *consolelist[MAXCONSOLES];
static PICKLISTLIST Picklistlist[MAXCONSOLES];

// Master function

void Init_VRMs();

// These functions and variables are exported

extern OBJECT *OB_Alloc();
extern void OB_Free(OBJECT *a);
extern void DestroyObject(OBJECT *a);
extern void DeletePending();
extern int OB_Init(OBJECT *a,char *name);
extern int OB_SetDir(OBJECT *objsel,int dir, int force);
extern void OB_SetSeq(OBJECT *objsel,char *name);
extern TILE *GetTile(int x,int y);
extern int WeighObject(OBJECT *obj);
extern int GetBulk(OBJECT *obj);
extern int AddToParty(OBJECT *new_member);
extern void SubFromParty(OBJECT *member);
extern int get_key();
extern int get_key_debounced();
extern int Restart(void);
extern void SetDarkness(int l);
extern void spillcontents(OBJECT *bag,int x,int y);
extern int NPC_Converse(const char *file, const char *start);
extern void CheckHurt(OBJECT *list);
extern void RedrawMap();
extern int IsTileSolid(int x,int y);
extern int IsTileWater(int x,int y);

extern void Inactive(OBJECT *o);
extern void ActivityName(OBJECT *o,char *activity, OBJECT *target);
extern void ActivityNum(OBJECT *o,int activity, OBJECT *target);

extern OBJECT *GetBestPlace(int x,int y);
extern OBJECT *GetTopObject(int x,int y);
extern OBJECT *GetRawObjectBase(int x,int y);
extern void MoveToPocket(OBJECT *src,OBJECT *dest);
extern void TransferToPocket(OBJECT *src,OBJECT *dest);
extern int MoveFromPocket(OBJECT *obj,OBJECT *container,int x,int y);
extern int ForceFromPocket(OBJECT *obj,OBJECT *container,int x,int y);
extern void MoveToTop(OBJECT *obj);
extern void MoveToFloor(OBJECT *obj);

extern int MoveToMap(int x, int y, OBJECT *object);
extern void TakeObject(int x,int y, OBJECT *container);
extern int MoveObject(OBJECT *object,int x,int y,int pushed);
extern void TransferObject(OBJECT *object,int x,int y);
extern OBJECT *GetObject(int x,int y);
extern OBJECT *GetSolidObject(int x,int y);

extern OBJECT *GetSolidObject(int x,int y);
extern OBJECT *GameGetObject(int x,int y);
extern OBJECT *GetObjectBase(int x,int y);
extern void CreateContents(OBJECT *o);
extern int TakeQuantity(OBJECT *c,char *o,int q);
extern int MoveQuantity(OBJECT *s,OBJECT *d,char *o,int q);
extern void AddQuantity(OBJECT *c,char *o,int q);
extern int SumObjects(OBJECT *c,char *o,int q);
extern OBJECT *GetFirstObject(OBJECT *cont, char *name);
extern OBJECT *GetFirstObjectNR(OBJECT *cont, char *name);
extern int getYN(char *q);
extern void gen_largemap();
extern int get_num(int no);
extern int get_str();
extern char user_input[128];
extern int FindPath(OBJECT *object, OBJECT *end, int diagonal);
extern int CanRoute(OBJECT *start, OBJECT *end, int diagonal);
extern int IsOnscreen(char *pname);
extern void CheckTime();
extern void ResumeSchedule(OBJECT *o);
extern void ResyncEverything();
extern char *BestName(OBJECT *o);
extern void CallVM(char *function);
extern void CallVMnum(int pos);
extern OBJECT *find_object_with_tag(int tag,char *name);
extern int get_key_quiet();

extern void check_object(OBJECT *o, char *label);
// unused functions 
/*
static void Call_VM(char *c);
static void Call_VM_num(int n);
static void sysBug(char *msg, ...);
*/
extern char debcurfunc[];
extern void VMstacktrace();

struct OBJECT_PREF {
	OBJECT *ptr;
	int steps;
};

void set_light(int x, int y, int x2, int y2, int light);
static int CMP_sort_objpref(const void *a,const void *b);
static OBJECT *find_hostile_default(OBJECT *partymember, VMINT flags);


///////////////////////////////////

OBJECT *create_object(STRING name, int x, int y)
{
OBJECT *temp;
//boot2("***%s\n",name);
temp = OB_Alloc();
temp->curdir = temp->dir[0];
OB_Init(temp,name);
OB_SetDir(temp,CHAR_D,FORCE_SHAPE|FORCE_FRAME);
MoveToMap(x,y,temp);
CreateContents(temp);
return(temp);
}

void remove_object(OBJECT *object)
{
if(!object)
    {
    Bug("remove_object: attempt to remove NULL\n");
    redraw();
    return;
    }
CHECK_OBJECT(object);

if(object->flags & IS_DECOR)	// Mustn't remove a decor, that's bad
	return;

if(object == player)
    {
    Bug("remove_object: cannot remove the PLAYER\n");
    redraw();
    return;
    }

DeleteObject(object);
}

void delete_pending()
{
DeletePending();
}

void change_object(OBJECT *obj,char *name)
{
STATS ostats;
char resname[32];
char pname[32];
char speech[128];
char user1[128];
char user2[128];
int tcache;

CHECK_OBJECT(obj);

if(obj && name)
	{
	if(obj->flags & IS_DECOR)	// Mustn't change a decor, that's bad
		return;
	if(obj->flags & IS_SYSTEM)	// Mustn't change a system object, that's bad
		return;

	memcpy(&ostats,obj->stats,sizeof(STATS));
	strcpy(resname,obj->funcs->resurrect);
	strcpy(speech,obj->funcs->talk);
	strcpy(user1,obj->funcs->user1);
	strcpy(user2,obj->funcs->user2);
	tcache = obj->funcs->tcache;
	strcpy(pname,obj->personalname);
	OB_Init(obj,name);
	memcpy(obj->stats,&ostats,sizeof(STATS));
	strcpy(obj->funcs->resurrect,resname);
	strcpy(obj->personalname,pname);
	strcpy(obj->funcs->talk,speech);
	strcpy(obj->funcs->user1,user1);
	strcpy(obj->funcs->user2,user2);
	obj->funcs->tcache = tcache;
	AL_Add(&ActiveList,obj); // Make active (if it can be)
	}
}


void replace_object(OBJECT *obj,STRING name)
{
int k;
OBJECT *owner;

if(obj && name)
	{
	if(obj->flags & IS_DECOR)	// Mustn't change a decor, that's bad
		return;
	if(obj->flags & IS_SYSTEM)	// Mustn't change a system object, that's bad
		return;
	CHECK_OBJECT(obj);
	owner=obj->stats->owner.objptr;
	k=fullrestore;
	fullrestore=0;
	OB_Init(obj,name);
	fullrestore=k;
	obj->stats->owner.objptr=owner;
	AL_Add(&ActiveList,obj); // Make active (if it can be)
	gen_largemap();     // Recalc solid objects cache
	}
}

void set_object_direction(OBJECT *obj,int dir)
{
if(obj)
	{
	CHECK_OBJECT(obj);
	OB_SetDir(obj,dir,0);
	}
}

void set_object_sequence(OBJECT *obj,char *seq)
{
if(obj && seq)
	{
	CHECK_OBJECT(obj);
	OB_SetSeq(obj,seq);
	}
}

void print(char *msg, ...)
{
char temp[4096];
va_list ap;

if(!msg)
	{
	Bug("Attemped to print NULL message\n");
	va_end(ap);
	redraw();
	return;
	}

va_start(ap, msg);
vsprintf(temp,msg,ap);          // Temp is now the message
va_end(ap);

irecon_print(temp);
}

void printxy(int x,int y,char *msg, ...)
{
char temp[128];
va_list ap;

va_start(ap, msg);
vsprintf(temp,msg,ap);          // Temp now is the message
irecon_printxy(x,y,temp);
va_end(ap);
}

void TextColor(int r,int g, int b)
{
irecon_colour(r,g,b);
}

void show_object(OBJECT *z,int x,int y)
{
if(!z)
   return;
CHECK_OBJECT(z);
z->form->seq[0]->image->Draw(swapscreen,x,y);
}

OBJECT *get_first_object(int x,int y)
{
return GetObjectBase(x,y);//GetTopObject(x,y);
}

OBJECT *get_object(int x,int y)
{
return GameGetObject(x,y);
}

OBJECT *get_solid_object(int x,int y)
{
return GetSolidObject(x,y);
}

TILE *get_tile(int x,int y)
{
return GetTile(x,y);
}

void move_to_pocket(OBJECT *src,OBJECT *dest)
{
if(!src || !dest)
	return;

CHECK_OBJECT(src);
CHECK_OBJECT(dest);

if(src->flags & IS_QUANTITY)
	{
	TransferObject(src,0,0);	// Get it out of the pocket for linked-lists based on object->pocket
	AddQuantity(dest,src->name,src->stats->quantity); // Now it's gone we can do this without colliding with itself
	DeleteObject(src);
	}
else
	MoveToPocket(src,dest);
}

void transfer_to_pocket(OBJECT *src,OBJECT *dest)
{
if(!src || !dest)
	return;

CHECK_OBJECT(src);
CHECK_OBJECT(dest);

if(src->flags & IS_QUANTITY)
	{
	TransferObject(src,0,0);	// Get it out of the pocket for linked-lists based on object->pocket
	AddQuantity(dest,src->name,src->stats->quantity); // Now it's gone we can do this without colliding with itself
	DeleteObject(src);
	}
else
	TransferToPocket(src,dest);
}

int move_from_pocket(OBJECT *obj,OBJECT *container,int x,int y)
{
CHECK_OBJECT(obj);
CHECK_OBJECT(container);
if(!MoveFromPocket(obj,container,x,y))
	return 0;
return 1;
}


int force_from_pocket(OBJECT *obj,OBJECT *container,int x,int y)
{
CHECK_OBJECT(obj);
CHECK_OBJECT(container);
if(!ForceFromPocket(obj,container,x,y))
	return 0;
return 1;
}


int move_object(OBJECT *obj,int x, int y)
{
CHECK_OBJECT(obj);
if(!MoveObject(obj,x,y,0))
	return 0;
return 1;
}


int push_object(OBJECT *obj,int x, int y)
{
CHECK_OBJECT(obj);
if(!MoveObject(obj,x,y,1))
	return 0;
return 1;
}


void transfer_object(OBJECT *obj,int x, int y)
{
CHECK_OBJECT(obj);
TransferObject(obj,x,y);
}

// Move an entire pile of objects with a given tag (move tables for secrets etc)
void move_stack(OBJECT *obj,int x, int y)
{
OBJECT *o,*next;
if(!obj) {
	return;
}
int srcx=obj->x,srcy=obj->y;
if(srcx == x && srcy == y) {
	return;
}

o = GetRawObjectBase(srcx,srcy);
if(!o) {
	return;
}
// Don't move the bottom objects if they've not got the tag
// However, objects above the tagged ones should be carried along on top
if(o->tag != obj->tag) {
	for(;o;o=o->next) {
		if(o->tag == obj->tag) {
			break;
		}
	}
	if(!o) {
		return;
	}
}

for(;o;o=next) {
	next=o->next;
	// If it's already moved, skip over it
	if(o->engineflags & ENGINE_DIDMOVESTACK) {
		o=next;
		continue;
	}

	// Move it and mark it as moved
	TransferObject(o,x,y);
	o->engineflags |= ENGINE_DIDMOVESTACK;
	o=next;
}
}


void spill_contents(OBJECT *bag)
{
CHECK_OBJECT(bag);
spillcontents(bag,bag->x,bag->y);
}

void spill_contents_at(OBJECT *bag,int x, int y)
{
CHECK_OBJECT(bag);
spillcontents(bag,x,y);
}

int weigh_object(OBJECT *obj)
{
CHECK_OBJECT(obj);
return(WeighObject(obj));
}

int get_bulk(OBJECT *obj)
{
CHECK_OBJECT(obj);
return(GetBulk(obj));
}


void play_song(char *a)
{
S_PlayMusic(a);
}


void play_sound(char *a)
{
S_PlaySample(a,sf_volume);
}


void object_sound(char *a, OBJECT *b)
{
int vol,dist,dx,dy,svol;
OBJECT *temp;

// Safety check
if(!a || !b || !player)
	return;

svol = sf_volume;

if(SoundFixed)
    {
    S_PlaySample(a,sf_volume);
    return;
    }

if(!b)
    {
    S_PlaySample(a,sf_volume);
    return;
    }

CHECK_OBJECT(b);

if(!b->flags & IS_ON)
    {
    S_PlaySample(a,sf_volume);
    return;
    }

if(b->parent.objptr)
    {
    temp = b->parent.objptr;
    if(temp != player)
        {
//    if(temp->flags & IS_PERSON)
//        return;
        b->x = temp->x;
        b->y = temp->y;
        svol = sf_volume >> 2;
        svol = (svol<<1) + svol;       // Vol is now 3/4 sf_volume
        }
    }

vol = svol;
dx = player->x - b->x;
if(dx<0) dx=-dx;
dy = player->y - b->y;
if(dy<0) dy=-dy;

dist=dx;
if(dy>dx) dist=dy;

if(dist>SOUND_DIST)
	{
	dist-=SOUND_DIST;
	vol = svol - (dist*SOUND_DROP);
	if(vol<0) vol=0;
	}

S_PlaySample(a,vol);
}


void stop_song()
{
S_StopMusic();
}

int object_is_called(OBJECT *obj,char *str)
{
if(!obj) return 0;
if(!str) return 0;
CHECK_OBJECT(obj);

/*
if(!OB_Check(obj))
	{
	Bug("check_object detected a Fuck-up in object_is_called(\"%s\")\n",str);
	Bug("Detailed report follows:\n");
	CHECK_OBJECT(obj);
	ithe_panic("Fuck-up detected, check logfile",NULL);
	}
*/
if(!obj->name)
	ithe_panic("check_object","Object has NULL name");

if(!istricmp(obj->name,str))
	return 1;
return 0;
}

void redraw()
{
irecon_update();            // Redraw all text output
Show();                     // And blit the screen again
}


void get_input()
{
irekey=get_key_debounced();
}

int get_number(int no)
{
if(!get_num(no))
	return 0;
return atoi(user_input);
}

int is_solid(int x,int y)
{
return isSolid(x,y);
}


int rnd(int max)
{
if(max==0) return 0;    // Prevent division by 0
return(rand() % max);
}

int choose_member(OBJECT *x, char *player_call)
{
int ctr;

if(!x)
	return 0;
//C_printf("choose member: %s\n",x->personalname?x->personalname:x->name);
CHECK_OBJECT(x);

for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	if(party[ctr])
		Inactive(party[ctr]);

player = x;
SubAction_Wipe(player); // Erase any sub-tasks
ActivityName(player,player_call,NULL);
return 1;
}


int choose_leader(OBJECT *x, char *player_call, char *follower_call)
{
int ctr;

if(!x)
	return 0;

CHECK_OBJECT(x);
//C_printf("choose leader: %s\n",x->personalname?x->personalname:x->name);

for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	if(party[ctr])
		{
		Inactive(party[ctr]);
		if(party[ctr] != x)
			{
			SubAction_Wipe(party[ctr]); // Erase any sub-tasks
			ActivityName(party[ctr],follower_call,NULL);
			}
//		ilog_printf("setting '%s' to follow function '%s'\n",party[ctr]->name,follower_call);
		}

player = x;
SubAction_Wipe(player); // Erase any sub-tasks
ActivityName(player,player_call,NULL);
return 1;
}


int add_to_party(OBJECT *new_member)
{
CHECK_OBJECT(new_member);
return(AddToParty(new_member));
}


void remove_from_party(OBJECT *member)
{
CHECK_OBJECT(member);
SubFromParty(member);
ResumeSchedule(member);
}


void add_goal(OBJECT *o, int priority, char *vrm, OBJECT *target)
{
if(o && vrm)
	{
	CHECK_OBJECT(o);
#ifdef CHECK_OBJECT_STRICT
	if(target)
		{
		CHECK_OBJECT(target);
		}
#endif

	if(o->flags & IS_DECOR)	// Mustn't change a decor, that's bad
		return;
	if(o->flags & IS_SYSTEM)	// Mustn't change a system object
		return;

//	Bug("%s does %s to %x\n",o->name,vrm,target);
	SubAction_Wipe(o); // Erase any sub-tasks
	ActivityName(o,vrm,target);
	}
}

void end_goal(OBJECT *o)
{
if(o)
	{
	CHECK_OBJECT(o);
	Inactive(o);
	}
}

void wipe_goals(OBJECT *o)
{
if(o)
	{
	CHECK_OBJECT(o);
	Inactive(o);
	}
}

void kill_goals(OBJECT *o, int priority)
{
if(o)
	{
	CHECK_OBJECT(o);
	Inactive(o);
	}
}

void restart()
{
Restart();
}

void set_darkness(int l)
{
if(l<0)
	l=0;
if(l>255)
	l=255;
SetDarkness(l);
}

int talk_to(char *speechfile, char *start)
{
char filename[1024];
if(!speechfile)
    return 0;
if(!speechfile[0])
    return 0;
if(loadfile(speechfile,filename))
    return NPC_Converse(filename,start);

Bug("CONVERSATION FILE '%s' NOT FOUND\n",speechfile);
return 0;
}

void check_hurt(OBJECT *obj)
{
#ifdef CHECK_OBJECT_STRICT
if(obj)
	{
	CHECK_OBJECT(obj);
	}
#endif
CheckHurt(obj);
}

void redraw_map()
{
RedrawMap();
}

void waitfor(unsigned ms)
{
IRE_WaitFor(ms);
}

#define WAIT_SLICE 50

void waitredraw(unsigned ms)
{
unsigned int ctr=0;
ms/=WAIT_SLICE;
if(ms<1)
	ms=1;

for(ctr=0;ctr<ms;ctr++)
	{
	IRE_WaitFor(WAIT_SLICE);
	RedrawMap();
	}
//rest_callback(ms,RedrawMap);
}

// Line of sight routine, using J. Grant's interpretation of Bresenham
// From the ITG32 graphics library (1995)

int line_of_sight(int xa, int ya, int xb, int yb, int *tx, int *ty, OBJECT *ignore)
{
int t, distance,steps;
int xerr=0, yerr=0, delta_x, delta_y;
int incx, incy;
int xo,yo;

OBJECT *obj;

// Don't count the origin as a solid object
xo = xa;
yo = ya;

steps=0;

// Find the distance to go in each plane.
	delta_x = xb - xa;
	delta_y = yb - ya;

// Find out the direction
	if(delta_x > 0) incx = 1;
	else if(delta_x == 0) incx = 0;
	else incx = -1;

	if(delta_y > 0) incy = 1;
	else if(delta_y == 0) incy = 0;
	else incy = -1;

	// Find which way is were mostly going
	delta_x = abs(delta_x);
	delta_y = abs(delta_y);

	if(delta_x > delta_y) {
		distance=delta_x; 
	} else {
		distance=delta_y;
	}

	// Draw the god dam line.
	for(t = 0; t <= distance + 1; t++) {

		if(isSolid(xa,ya) && (!IsTileWater(xa,ya))) {
			if(!(xa == xo && ya == yo)) {
				if(!(xa == xb && ya == yb)) {
					obj = GetSolidObject(xa,ya);
					if(!obj) {
						if(tx) {
							*tx=xa;
						}
						if(ty) {
							*ty=ya;
						}
						return 0;
					}
					CHECK_OBJECT(obj);
					if(obj != ignore) {
						if(!(obj->flags & IS_TABLETOP)) {
							if(tx) {
								*tx=xa;
							}
							if(ty) {
								*ty=ya;
							}
							return 0;
						}
					}
				}
			}
		}
	
		xerr += delta_x;
		yerr += delta_y;

		steps++;

		if(xerr > distance) {
			xerr -= distance;
			xa += incx;
		}
		if(yerr > distance) {
			yerr -= distance;
			ya += incy;
		}
	}

if(tx) {
	*tx=xa;
}
if(ty) {
	*ty=ya;
}

return steps;
}



void move_to_top(OBJECT *object)
{
CHECK_OBJECT(object);
MoveToTop(object);
}

void move_to_floor(OBJECT *object)
{
CHECK_OBJECT(object);
MoveToFloor(object);
}

int in_pocket(OBJECT *obj)
{
CHECK_OBJECT(obj);
if(obj->pocket.objptr)
	return 1;
return 0;
}


void set_flag(OBJECT *target, VMUINT flag, int value)
{
VMUINT newflag;

if(!target)
    return;
CHECK_OBJECT(target);


if(flag & IS_NPCFLAG)
	{
	if(value)
		SetNPCFlag(target,flag);
	else
		ClearNPCFlag(target,flag);
	return;
	}

// Okay, it's a system flag

if(flag & IS_ON)
	{
	Bug("Flag IS_ON is read-only, use 'destroy' to delete objects\n");
	redraw();
	return;
	}

newflag = 0;
if(value)
	newflag = flag;

if(flag & IS_TRIGGER)
	{
	// Only allow this to be changed if there is a Stand function
	if(target->funcs && target->funcs->scache < 1)
		return; // NO
	}

if(flag & IS_DECOR)
	{
	Bug("Flag DECOR is read-only\n");
	redraw();
	return;
	}

if(flag & IS_SYSTEM)
	{
	Bug("Flag SYSTEM is read-only\n");
	redraw();
	return;
	}

/*
    default:
    Bug("Attempt to set unknown flag %08x in function '%s'\n",flag,debcurfunc);
    redraw();
    break;
*/
    
// Okay, do it
//printf("Flags for %s currently = %lx\n",target->name,target->flags);
target->flags &= ~flag;
target->flags |= newflag;
//printf("Flags for %s now %lx\n",target->name,target->flags);
}

int get_flag(OBJECT *target, VMUINT flag)
{
if(!target)
    return 0;
CHECK_OBJECT(target);

if(flag & IS_NPCFLAG)
	{
	// Okay, strip the NPCflag bit
	flag &= STRIPNPC;

	// Protection:  if there's nothing to link to, fail
	if(flag & (IS_SYMLINK & STRIPNPC))
		{
		if(target->stats->owner.objptr && GetNPCFlag(target, IS_SYMLINK))
			return 1;
		return 0;
		}

	// int may be smaller than long, so convert to a boolean
	if(GetNPCFlag(target,flag))
		return 1;
	return 0;
	}

// int may be smaller than long, so convert to a boolean
if(target->flags & flag)
	return 1;
return 0;

/*
 * Bug("Attempt to read unknown flag %08x in function '%s'\n",flag,debcurfunc);
redraw();
return 0;
*/
}

void default_flag(OBJECT *target, VMUINT flag)
{
int id,state;
if(!target)
	return;
CHECK_OBJECT(target);

if(target->flags & IS_DECOR)	// Mustn't change a decor, that's bad
	return;
if(flag & IS_ON)	// No
	return;

id = getnum4char(target->name);
if(id == -1)
	return;

if(flag & IS_NPCFLAG)
	{
	target->stats->npcflags &= ~flag;
	target->stats->npcflags |= (CHlist[id].stats->npcflags & flag);
	target->stats->npcflags &= STRIPNPC;
	return;
	}

state=0;
if(CHlist[id].flags & flag)
	state=1;
set_flag(target,flag,state);
}

int get_tileflag(TILE *target, VMUINT flag)
{
if(!target)
	return 0;

if(flag & IS_NPCFLAG)
	return 0;

if(target->flags & flag)
	return 1;
return 0;
}


int take_quantity(OBJECT *container, char *objecttype, int quantity)
{
CHECK_OBJECT(container);
return (TakeQuantity(container,objecttype,quantity));
}

void add_quantity(OBJECT *container, char *objecttype, int quantity)
{
CHECK_OBJECT(container);
AddQuantity(container,objecttype,quantity);
}

int move_quantity(OBJECT *src, OBJECT *dest, char *objecttype, int quantity)
{
if(!src)
	{
	Bug("move_quantity with NULL source\n");
	return 0;
	}
CHECK_OBJECT(src);
if(!dest)
	{
	Bug("move_quantity with NULL destination\n");
	return 0;
	}
CHECK_OBJECT(dest);
return (MoveQuantity(src,dest,objecttype,quantity));
}

int count_objects(OBJECT *container, char *objecttype)
{
CHECK_OBJECT(container);
if(container->pocket.objptr)
	return SumObjects(container->pocket.objptr,objecttype,0);
return 0;
}

OBJECT *find_object_with_tag(int tag,char *name)
{
OBJLIST *t;

for(t=MasterList;t;t=t->next)
    {
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
    }

return NULL;
}

int get_yn(char *question)
{
return(getYN(question));
}


void set_user_flag(char *a, int s)
{
Set_tFlag(a,s);
}

int get_user_flag(char *a)
{
return(Get_tFlag(a));
}


void set_local(OBJECT *plyr, OBJECT *obj, char *a, int s)
{
char *pname = NPC_MakeName(plyr);
NPC_set_lFlag(obj,a,pname,s);
M_free(pname);
}

int get_local(OBJECT *plyr, OBJECT *obj, char *a)
{
int r;
char *pname = NPC_MakeName(plyr);
r=NPC_get_lFlag(obj,a,pname);
M_free(pname);
return r;
}

int move_forward(OBJECT *a)
{
if(!a)
    return 0;
CHECK_OBJECT(a);

a->engineflags |= ENGINE_STEPUPDATED; // Force animation

if(a->curdir == CHAR_U)
    return MoveObject(a,a->x,a->y-1,0);
if(a->curdir == CHAR_D)
    return MoveObject(a,a->x,a->y+1,0);
if(a->curdir == CHAR_L)
    return MoveObject(a,a->x-1,a->y,0);
if(a->curdir == CHAR_R)
    return MoveObject(a,a->x+1,a->y,0);
return 0;
}

int move_backward(OBJECT *a)
{
if(!a)
    return 0;
CHECK_OBJECT(a);

a->engineflags |= ENGINE_STEPUPDATED; // Force animation

if(a->curdir == CHAR_U)
    return MoveObject(a,a->x,a->y+1,0);
if(a->curdir == CHAR_D)
    return MoveObject(a,a->x,a->y-1,0);
if(a->curdir == CHAR_L)
    return MoveObject(a,a->x+1,a->y,0);
if(a->curdir == CHAR_R)
    return MoveObject(a,a->x-1,a->y,0);
return 0;
}

int turn_l(OBJECT *a)
{
if(!a)
    return 0;
CHECK_OBJECT(a);

switch(a->curdir)
    {
    case CHAR_U:
    OB_SetDir(a,CHAR_L,FORCE_SHAPE);
    break;

    case CHAR_L:
    OB_SetDir(a,CHAR_D,FORCE_SHAPE);
    break;

    case CHAR_D:
    OB_SetDir(a,CHAR_R,FORCE_SHAPE);
    break;

    case CHAR_R:
    OB_SetDir(a,CHAR_U,FORCE_SHAPE);
    break;
    }
return a->curdir;
}

int turn_r(OBJECT *a)
{
if(!a)
    return 0;
CHECK_OBJECT(a);

switch(a->curdir)
    {
    case CHAR_U:
    OB_SetDir(a,CHAR_R,FORCE_SHAPE);
    break;

    case CHAR_R:
    OB_SetDir(a,CHAR_D,FORCE_SHAPE);
    break;

    case CHAR_D:
    OB_SetDir(a,CHAR_L,FORCE_SHAPE);
    break;

    case CHAR_L:
    OB_SetDir(a,CHAR_U,FORCE_SHAPE);
    break;
    }
return a->curdir;
}

void wait_for_animation(OBJECT *obj)
{
if(!obj)
	return;
CHECK_OBJECT(obj);

if(obj->form->flags&6)  // Ignore if looped or stepped
	return;             // (Otherwise it will never come back)

if(obj->parent.objptr)       // Objects in pockets do not animate
	return;

for(;obj->sdir;RedrawMap());
}

void ___lightning(int ticks)
{
do_lightning=ticks;
}

int is_tile_solid(int x,int y)
{
return IsTileSolid(x,y);
}

int is_tile_water(int x,int y)
{
return IsTileWater(x,y);
}

void move_party_to_object(OBJECT *o)
{
if(!o)
	return;
CHECK_OBJECT(o);
for(int ctr=0;ctr<MAX_MEMBERS;ctr++)
    if(party[ctr])
        TransferToPocket(party[ctr],o);
}

void move_party_from_object(OBJECT *o, int x, int y)
{
if(!o)
	return;
CHECK_OBJECT(o);
for(int ctr=0;ctr<MAX_MEMBERS;ctr++)
	if(party[ctr])
		ForceFromPocket(party[ctr],o,x,y);
}


int move_towards(OBJECT *object, OBJECT *end)
{
char *s1,*s2;

if(!object || !end)
    {
    s1="Null";
    s2="Null";

    if(object)
        s1=BestName(object);
    if(end)
        s2=BestName(end);
    Bug("FindPath (%s,%s)\n",s1,s2);
    return 0;
    }

CHECK_OBJECT(object);
CHECK_OBJECT(end);

if(object->flags & IS_DECOR)
	return 0;
if(end->flags & IS_DECOR)
	return 0;
if(object->flags & IS_SYSTEM)
	return 0;

/*
if(!istricmp(object->name,"plate_broken"))
	ithe_panic("Oh no not again","plate is posessed");
*/
return FindPath(object,end,1);
}

int move_towards_4(OBJECT *object, OBJECT *end)
{
char *s1,*s2;

if(!object || !end)
    {
    s1="Null";
    s2="Null";

    if(object)
        s1=BestName(object);
    if(end)
        s2=BestName(end);
    Bug("FindPath (%s,%s)\n",s1,s2);
    return 0;
    }

CHECK_OBJECT(object);
CHECK_OBJECT(end);

if(object->flags & IS_DECOR)
	return 0;
if(end->flags & IS_DECOR)
	return 0;
if(object->flags & IS_SYSTEM)
	return 0;

return FindPath(object,end,0);
}

int move_thick(OBJECT *object, OBJECT *end)
{
char *s1,*s2;

if(!object || !end)
    {
    s1="Null";
    s2="Null";

    if(object)
        s1=object->name;
    if(end)
        s2=end->name;
    Bug("MoveThick (%s,%s)\n",s1,s2);
    return 0;
    }

CHECK_OBJECT(object);
CHECK_OBJECT(end);

if(object->flags & IS_DECOR)
	return 0;
if(end->flags & IS_DECOR)
	return 0;
if(object->flags & IS_SYSTEM)
	return 0;

object->user->dx=0;
if(object->x < end->x)
    object->user->dx=1;
if(object->x > end->x)
    object->user->dx=-1;
object->user->dy=0;
if(object->y < end->y)
    object->user->dy=1;
if(object->y > end->y)
    object->user->dy=-1;

if(object->user->dy<0)
    OB_SetDir(object,CHAR_U,FORCE_SHAPE);
if(object->user->dy>0)
    OB_SetDir(object,CHAR_D,FORCE_SHAPE);
if(object->user->dx<0)
    OB_SetDir(object,CHAR_L,FORCE_SHAPE);
if(object->user->dx>0)
    OB_SetDir(object,CHAR_R,FORCE_SHAPE);
MoveObject(object,object->x+object->user->dx,object->y+object->user->dy,0);
object->engineflags |= ENGINE_STEPUPDATED; // Force animation
return 1;
}

int character_onscreen(char *pname)
{
return IsOnscreen(pname);
}

void scroll_things()
{
int ctr,x,y;
for(ctr=0;ctr<TItot;ctr++)
	{
	x = TIlist[ctr].name[1];
	if(x<'A' || x>'Z')
		break;
	x =((x-'A')%3)-1;
	y = TIlist[ctr].name[1];
	if(y<'A' || y>'Z')
		break;
	y =((y-'A')%5)-2;
	TIlist[ctr].sdx=x;
	TIlist[ctr].sdy=y;
	}
}

void scroll_tile(char *name, int x, int y)
{
int ctr;

for(ctr=0;ctr<TItot;ctr++)
	if(!istricmp(TIlist[ctr].name,name))
		{
		scroll_tile_number(ctr,x,y);
		return;
		}
Bug("No such tile '%s'\n",name);
}

void scroll_tile_number(int no, int x, int y)
{
if(no<0 || no>=TItot)
	{
	Bug("No such tile number %d\n",no);
	return;
	}

if(x<0)
	x=32+x;
if(y<0)
	y=32+y;

TIlist[no].sdx=x;
TIlist[no].sdy=y;

// If zero, reset the scroll offsets, since the user probably wants it to
// go back to normal

if(x == 0 && y == 0)
	{
	TIlist[no].sx=0;
	TIlist[no].sy=0;
	}
}


void scroll_tile_reset(int no)
{
if(no<0 || no>=TItot)
	{
	Bug("No such tile number %d\n",no);
	return;
	}

TIlist[no].sdx=TIlist[no].odx;
TIlist[no].sdy=TIlist[no].ody;
TIlist[no].sx=0;
TIlist[no].sy=0;
}


/* unused function
void sysBug(char *msg, ...)
{
char temp[4096];
va_list ap;

if(!msg)
	{
	Bug("Attemped to Bug-print NULL message\n");
	va_end(ap);
	redraw();
	return;
	}

va_start(ap, msg);
vsprintf(temp,msg,ap);          // Temp is now the message
va_end(ap);

Bug(temp);
}
*/
OBJECT *search_container(OBJECT *container,char *name)
{
if(!container)
	return NULL;
CHECK_OBJECT(container);
if(container->pocket.objptr)
	return GetFirstObject(container->pocket.objptr, name);
return NULL;
}

void vrmfade_in()
{
for(Fader=255;Fader>0;Fader-=8)
	{
	RedrawMap();
	IRE_WaitFor(1);
//	S_PollMusic(); // Music is now polled by thread
	}
Fader=0;
}

void vrmfade_out()
{
for(Fader=0;Fader<255;Fader+=8)
	{
	RedrawMap();
	IRE_WaitFor(1);
//	S_PollMusic(); // Music is now polled by thread
	}
Fader=255;
}

void check_time()
{
CheckTime();
}

//
// Convert date and time to packed format
//

VMINT pack_time(VMINT year, VMINT month, VMINT day, VMINT hour) {
return (year * 1000000) + (month * 10000) + (day * 100) + hour;
}

//
// Split packed date time to component parts
//

void unpack_time(VMINT packed, VMINT *year, VMINT *month, VMINT *day, VMINT *hour) {
if(year) {
	*year = packed / 1000000;
}
packed %= 1000000;
if(month) {
	*month = packed / 10000;
}
packed %= 10000;
if(day) {
	*day = packed / 100;
}
packed %= 100;
if(hour) {
	*hour = packed;
}
}

//
// Convert date to packed format
//

VMINT pack_date(VMINT year, VMINT month, VMINT day) {
return (year * 10000) + (month * 100) + day;
}

//
// Split packed date to component parts
//

void unpack_date(VMINT packed, VMINT *year, VMINT *month, VMINT *day) {
if(year) {
	*year = packed / 10000;
}
packed %= 10000;
if(month) {
	*month = packed / 100;
}
packed %= 100;
if(day) {
	*day = packed;
}
}

//
//  Check and adjust a packed datetime integer
//

void check_time(VMINT *timeptr) {
VMINT hour=0,day=0,month=0,year=0;
if(!timeptr) {
	CheckTime();
	return;
}

unpack_time(*timeptr,&year,&month,&day,&hour);

if(hour >= hour_day) {
	day += hour/hour_day;
	hour %= hour_day;
}

if(day >= day_month) {
	month += day/day_month;
	day %= day_month;
}

if(month >= month_year) {
	year += month/month_year;
	month %= month_year;
}

*timeptr = pack_time(year,month,day,hour);
}

//
//  Check and adjust packed time without date
//

void check_date(VMINT *timeptr) {
if(!timeptr) {
	CheckTime();
	return;
}

// Convert to datetime by adding a fake hour
VMINT datetime = (*timeptr) * 100;
check_time(&datetime);
*timeptr = datetime/100;
}



OBJECT *find_nearest(OBJECT *o, char *type)
{
#define INF 2000000000
char *s1,*s2;
OBJECT *temp;
int x,y,mix,miy,max,may,steps;

// the three possible cases, best of each
OBJECT_PREF mine_ob;
OBJECT_PREF public_ob;
OBJECT_PREF private_ob;
mine_ob.steps = public_ob.steps = private_ob.steps = INF;
mine_ob.ptr = public_ob.ptr = private_ob.ptr = NULL;

if(!o || !type) {
	s1="Null";
	s2="Null";

	if(o) {
		s1=o->name;
	}
	if(type) {
		s2=type;
	}
	Bug("find_nearest(%s,%s)\n",s1,s2);
	return 0;
}

CHECK_OBJECT(o);

// Now build the search area and make sure it is within the bounds of the map

mix = o->x - 16;
miy = o->y - 16;
CLIPMIN(mix);
CLIPMIN(miy);

max=mix+32;
may=miy+32;

CLIPMAX(max,curmap->w);
CLIPMAX(may,curmap->h);

// Ok, start searching

for(y=miy;y<may;y++) {
    for(x=mix;x<max;x++) {
        for(temp=GetRawObjectBase(x,y);temp;temp=temp->next) {
            if(temp->flags & IS_ON) {
                if(!istricmp(temp->name,type)) {
                    steps = CanRoute(o,temp,1);
                    if(steps >= 0) {
						// best case, mine
                        if(temp->stats->owner.objptr == o) {
                            if(steps<mine_ob.steps) {
                                mine_ob.steps = steps;
                                mine_ob.ptr = temp;
                            }
                        } else {
                        	if(temp->stats->owner.objptr == NULL || GetNPCFlag(temp, IS_SPAWNED)) {
							    // next best case, public (spawned from an egg counts too)
                        	    if(steps<public_ob.steps) {
	                                public_ob.steps = steps;
                                	public_ob.ptr = temp;
                            	}
                        	} else {
								// worst case, someone else's
	                            if(steps<private_ob.steps) {
	                                private_ob.steps = steps;
                                	private_ob.ptr = temp;
                                }
							}
                        }
                    }
                }
			}
		}
	}
}

// Now we should have the best possible case for each of the three types

if(mine_ob.ptr) {
	return mine_ob.ptr;
}

if(public_ob.ptr) {
	return public_ob.ptr;
}

return private_ob.ptr;
}

// Find up to n of the nearest objects of the type requested

void find_nearby(OBJECT *o, char *type, OBJECT **list, int listsize)
{
char *s1,*s2;
OBJECT *temp;
int x,y,mix,miy,max,may,ptr;

if(!o || !type || !list) {
	s1="Null";
	s2="Null";

	if(o)
		s1=o->name;
	if(type)
		s2=type;
	Bug("find_nearby(%s,%s,%x,%d)\n",s1,s2,list,listsize);
	return;
}

// Blank the list
memset(list,0,listsize*sizeof(OBJECT *));

CHECK_OBJECT(o);

// Now build the search area and make sure it is within the bounds of the map

mix = o->x - 16;
miy = o->y - 16;
CLIPMIN(mix);
CLIPMIN(miy);

max=mix+32;
may=miy+32;
CLIPMAX(max,curmap->w);
CLIPMAX(may,curmap->h);

// Ok, start searching, and keep adding to the list if we can get there

ptr=0;
for(y=miy;y<may;y++)
	for(x=mix;x<max;x++)
		for(temp=GetRawObjectBase(x,y);temp;temp=temp->next)
			if(temp->flags & IS_ON)
				if(!istricmp_fuzzy(temp->name,type))
					if(CanRoute(o,temp,1) >= 0)
						if(ptr<listsize)
							list[ptr++]=temp;

return;
}

// Find up to n of the nearest objects matching the flag requested (e.g. IS_PERSON)

void find_nearby_flag(OBJECT *o, VMINT flag, OBJECT **list, int listsize)
{
OBJECT *temp;
int x,y,mix,miy,max,may,ctr,dist;
OBJECT_PREF steps[256];

if(!o || !list) {
	return;
}

flag |= IS_ON;	// Ensure it's on

// Blank the list
memset(list,0,listsize*sizeof(OBJECT *));

if(listsize > 255) {
	listsize = 255;
}

// Initialise the pool
for(ctr=0;ctr<listsize;ctr++) {
	steps[ctr].ptr=NULL;
	steps[ctr].steps = INF;
}

CHECK_OBJECT(o);

// Now build the search area and make sure it is within the bounds of the map

mix = o->x - 16;
miy = o->y - 16;
CLIPMIN(mix);
CLIPMIN(miy);

max=mix+32;
may=miy+32;
CLIPMAX(max,curmap->w);
CLIPMAX(may,curmap->h);

// Ok, start searching, and keep adding to the list if we can get there

ctr=0;
for(y=miy;y<may;y++) {
	for(x=mix;x<max;x++) {
		for(temp=GetRawObjectBase(x,y);temp;temp=temp->next) {
			if(temp == o) {
				continue;
			}
			if((temp->flags & flag) == flag) {
				dist= CanRoute(o,temp,1);
				if(dist >= 0) {
					if(ctr<listsize) {
						steps[ctr].ptr = temp;
						steps[ctr].steps = dist;
						ctr++;
					}
				}
			}
		}
	}
}

// Sort them shortest-first
qsort(steps,ctr,sizeof(OBJECT_PREF),CMP_sort_objpref);

for(y=0;y<ctr;y++) {
	list[y]=steps[y].ptr;
}

return;
}

OBJECT *find_hostile(OBJECT *partymember, VMINT flags) {

	if(!partymember) {
		partymember = player;
	}
	if(!partymember) {
		return NULL;
	}

	int algorithm = flags & HOSTILE_ALGO_MASK;
	switch(algorithm) {
/*
		case HOSTILE_NEAREST:
			return find_hostile_nearest(partymember, flags);
		case HOSTILE_FURTHEST:
			return find_hostile_furthest(partymember, flags);
		case HOSTILE_STRONGEST:
			return find_hostile_strongest(partymember, flags);
		case HOSTILE_WEAKEST:
			return find_hostile_strongest(partymember, flags);
*/
		default:
			return find_hostile_default(partymember, flags);
	}
}

OBJECT *find_hostile_default(OBJECT *partymember, VMINT flags) {
	OBJECT *temp;
	int x,y,mix,miy,max,may;

	// Now build the search area and make sure it is within the bounds of the map

	mix = partymember->x - 16;
	miy = partymember->y - 16;
	CLIPMIN(mix);
	CLIPMIN(miy);

	max=mix+32;
	may=miy+32;
	CLIPMAX(max,curmap->w);
	CLIPMAX(may,curmap->h);

	// Just grab the first matching object, if any

	for(y=miy;y<may;y++) {
		for(x=mix;x<max;x++) {
			for(temp=GetRawObjectBase(x,y);temp;temp=temp->next) {
				if((temp->flags & IS_ON) && (temp->engineflags & ENGINE_HOSTILE)) {
					if(flags & HOSTILE_UNIQUE) {
						if(temp->engineflags & ENGINE_HOSTILEMARK) {
							continue; // Someone's already claimed it
						}
						temp->engineflags |= ENGINE_HOSTILEMARK; // Claim it
					}
					return temp;
				}
			}
		}
	}

	return NULL;
}

void find_hostiles(OBJECT *partymember, OBJECT **list, int listsize, VMINT flags){

	OBJECT *obj;

	if(!list) {
		Bug("find_hostiles() called without list\n");
		return;
	}
	if(listsize<1) {
		return;
	}

	// Blank the list
	memset(list,0,listsize*sizeof(OBJECT *));

	for(int ctr=0;ctr<listsize;ctr++) {
		obj = find_hostile(partymember, flags | HOSTILE_UNIQUE);	// Doesn't make sense without unique enemy tracking
		if(!obj) {
			// Ran out of hostiles
			return;
		}

		list[ctr]=obj;
	}
}



// Find objects in a rectangle marked by two objects

void find_between(OBJECT *o1, OBJECT *o2, char *type, OBJECT **list, int listsize)
{
const char *s1,*s2,*s3;
OBJECT *temp;
int x,y,mix,miy,max,may,ptr;

if(!o1 || !o2 || !type || !list) {
	s1="Null";
	s2="Null";
	s3="Null";

	if(o1)
		s1=o1->name;
	if(o2)
		s2=o2->name;
	if(type)
		s3=type;
	Bug("find_between(%s,%s,%s,%x,%d)\n",s1,s2,s3,list,listsize);
	return;
}

// Blank the list
memset(list,0,listsize*sizeof(OBJECT *));

CHECK_OBJECT(o);

// Now build the search area and make sure it is within the bounds of the map

mix = o1->x;
miy = o1->y;
max = o2->x;
may = o2->y;

if(mix > max) {
	max=mix;
	mix=o2->x;
}
if(miy > may) {
	may=miy;
	miy=o2->y;
}

CLIPMIN(mix);
CLIPMIN(miy);
CLIPMIN(max);
CLIPMIN(may);

CLIPMAX(mix,(curmap->w-1));
CLIPMAX(miy,(curmap->h-1));
CLIPMAX(max,(curmap->w-1));
CLIPMAX(may,(curmap->h-1));

// Ok, start searching, and keep adding to the list.  Doesn't matter if we can route there or not

ptr=0;
for(y=miy;y<=may;y++)
	for(x=mix;x<=max;x++)
		for(temp=GetRawObjectBase(x,y);temp;temp=temp->next)
			if(temp->flags & IS_ON)
				if(!istricmp_fuzzy(temp->name,type))
					if(ptr<listsize)
						list[ptr++]=temp;

return;
}




static int CMP_sort_objpref(const void *a,const void *b)
{
return ((OBJECT_PREF *)a)->steps - ((OBJECT_PREF *)b)->steps;
}



// Find object by tag (searching nearby for speed)

OBJECT *find_neartag(OBJECT *o, char *type, int tag)
{
OBJECT *temp;
int x,y,mix,miy,max,may;

if(!o)
	{
	Bug("find_neartag(NULL,%s,%d)\n",NULL,type,tag);
	return NULL;
	}

CHECK_OBJECT(o);

// Now build the search area and make sure it is within the bounds of the map

mix = o->x - 16;
miy = o->y - 16;
if(mix<0)
	mix=0;
if(miy<0)
	miy=0;
max=mix+32;
may=miy+32;

if(max>curmap->w)
	max=curmap->w;
if(may>curmap->h)
	may=curmap->h;

// Search

for(y=miy;y<may;y++)
	for(x=mix;x<max;x++)
		for(temp=GetRawObjectBase(x,y);temp;temp=temp->next)
			if(temp->tag == tag)
				{
				if(!type)
					return temp;
				else
					if(!istricmp_fuzzy(temp->name,type))
						return temp;
				}
return NULL;
}


// Find a path marker

OBJECT *find_pathmarker(OBJECT *o, char *name)
{
char *s1,*s2;
OBJECT *temp;
int x,y,mix,miy,max,may;

if(!o || !name)
	{
	s1="Null";
	s2="Null";
	if(o)
		s1=o->name;
	if(name)
		s2=name;
	Bug("find_pathmarker(%s,%s)\n",s1,s2);
	return NULL;
	}

CHECK_OBJECT(o);

// Now build the search area and make sure it is within the bounds of the map

mix = o->x - 32;
miy = o->y - 32;
if(mix<0)
	mix=0;
if(miy<0)
	miy=0;
max=mix+64;
may=miy+64;

if(max>curmap->w)
	max=curmap->w;
if(may>curmap->h)
	may=curmap->h;

// Ok, start searching.

for(y=miy;y<may;y++)
	for(x=mix;x<max;x++)
		for(temp=GetRawObjectBase(x,y);temp;temp=temp->next)
			if(temp->flags & IS_ON)
				if(!istricmp(temp->name,"pathmarker"))
					if(!istricmp(temp->labels->location,name))
						if(CanRoute(o,temp,1) >= 0)
							return temp;

return NULL;
}


/*
 *      return a string with the optimal description of the object
 */

char *best_name(OBJECT *o)
{
if(!o)
	return nullstr;
CHECK_OBJECT(o);
return BestName(o);
}

/*
 *      count_active_objects() - diagnostic data
 */

int count_active_objects()
{
OBJLIST *t;
int ctr=0;

for(t=ActiveList;t;t=t->next) ctr++;
return ctr;
}



int object_onscreen(OBJECT *a)
{
if(!a)
      {
      Bug("Tried to find out if NULL is onscreen\n");
      return 0;
      }
CHECK_OBJECT(a);

// Is it inside the window?

if(a->x >=mapx)
    if(a->x <= mapx2)
        if(a->y >=mapy)
            if(a->y <= mapy2)
                return 1;    // Yes

return 0; // No
}

void resume_schedule(OBJECT *o)
{
if(!o)
	return;
CHECK_OBJECT(o);
ResumeSchedule(o);
}

void resync_everything()
{
ResyncEverything();
}

OBJECT *get_object_below(OBJECT *o)
{
OBJECT *temp;
if(!o)
    {
    Bug("get_object_below(NULL);\n");
    return NULL;
    }
CHECK_OBJECT(o);

if(o->parent.objptr)   // Don't bother
    return NULL;

temp = GetObjectBase(o->x,o->y);
if(!temp)
    {
    Bug("get_object_below found nothing: map corrupt?\n");
    return NULL;
    }

if(temp == o)   // Nothing below it
    return NULL;

for(;temp->next;temp=temp->next)
    if(temp->next == o)
        return temp;

//Bug("Ooh bugger!\n");
return NULL;
}

void check_object(OBJECT *target, char *label)
{
if(!target)
	return;

if(!OB_Check(target))
	{
	Bug("check_object detected a F___-up in PE-space (\"%s\")\n",label);
	ithe_panic("F___-up detected, check logfile",NULL);
	}
}

void set_light(int x, int y, int x2, int y2, int light)
{
int ctr,w;
if(x<0 || y<0 || x2>curmap->w || y2>curmap->h)
	{
	Bug("Tried to Set Light (%d,%d-%d,%d) outside map (%d,%d-%d,%d)\n",x,y,x2,y2,0,0,curmap->w,curmap->h);
	return;
	}
w=(x2-x)+1;
if(w<0)
	{
	Bug("Bogus light range (%d,%d to %d,%d)\n",x,y,x2,y2);
	return;
	}

for(ctr=y;ctr<=y2;ctr++)
	memset(&curmap->lightst[(ctr*curmap->w)+x],light,w);
}

void find_objects_with_tag(VMINT tag,char *name, VMINT *num, OBJECT **obj)
{
OBJLIST *t;
int ctr;

if(!obj)
	return;
if(!num)
	return;
if(*num<1)
	return;

ctr=0;
for(t=MasterList;t;t=t->next)
	if(t->ptr->tag == tag)
		if(name)
			if(!istricmp(name,t->ptr->name))
				if(ctr < *num)
					obj[ctr++]=t->ptr;
*num=ctr;
return;
}
// unused functions 
/*
void Call_VM(char *c)
{
CallVM(c);
}

void Call_VM_num(int n)
{
CallVMnum(n);
}
*/

void InitOrbit()
{
int ctr=0;
for(ctr=0;ctr<3600;ctr++)
	{
	ire_sintab[ctr] = sin(((double)(ctr)/3600.0)*6.28);
	ire_costab[ctr] = cos(((double)(ctr)/3600.0)*6.28);
	}
for(ctr=0;ctr<32768;ctr++)
	ire_sqrt[ctr]=sqrt((double)ctr);

}

void CalcOrbit(VMINT *angle, VMINT radius, VMINT drift, VMINT speed, VMINT *x, VMINT *y, VMINT ox, VMINT oy)
{
double fx,fy;
VMINT turb;

// Calculate path turbulence

turb = 0;
if(drift > 0)
	turb = qrand()%drift;
if(drift < 0)
	turb = -(qrand()%(-drift));

// Update angle
*angle += speed;
*angle %= 3600;

// Translate around origin
fx = ire_sintab[*angle] * (double)(radius + turb);
fy = ire_costab[*angle] * (double)(radius + turb);

// Write finished coordinates
*x = ox + (VMINT)fx;
*y = oy + (VMINT)fy;
}

// Project a corona effect

#define INC_COLOUR(cl,val)  {cl+=val;	if(cl>255) cl=255;}

void ProjectCoronaOld(int xc,int yc, int w, int h,int intensity, int falloff)
{
int x,y,x1,y1,w1,h1;
int r,g,b,inc;
double xa,yb;
IRECOLOUR pixel;

// Get half width
w1=(w>>1);
h1=(h>>1);
// Calculate new top-left (from centre position)
x1=xc-w1;
y1=yc-h1;

for(y=0;y<h;y++)
	for(x=0;x<w;x++)
		{
		gamewin->GetPixel(x+x1,y+y1,&pixel);
		r=pixel.r;
		g=pixel.g;
		b=pixel.b;
		xa=(double)w1-(double)(x);
		yb=(double)h1-(double)(y);
		inc=(int)((double)intensity-(((double)falloff/100.0)*(xa*xa)+(yb*yb)));
		if(inc<0) inc=0;
		if(inc)
			{
			INC_COLOUR(r,inc);
			INC_COLOUR(g,inc);
			INC_COLOUR(b,inc);
			}
		gamewin->PutPixel(x+x1,y+y1,r,g,b);
		}
}

void ProjectCorona(VMINT xc,VMINT yc, VMINT radius,VMINT intensity, VMINT falloff, VMINT tint)
{
VMINT x,y,x1,y1,diam,xa,yb;
VMINT r,g,b,inc,xayb;
IRECOLOUR pixel;

// Get diameter
diam=radius<<1;

// Calculate new top-left (from centre position)
x1=xc-radius;
y1=yc-radius;

for(y=0;y<diam;y++)
	for(x=0;x<diam;x++)
		{
		xa=radius-x;
		yb=radius-y;
		// Calculate the squared value and clip it for the SQRT table
		xayb=(xa*xa)+(yb*yb);
		if(xayb>32768)
			xayb=32768;
		if(xayb<0)
			xayb=0;
		inc=intensity-(int)(((float)falloff/100.0)*ire_sqrt[xayb]);
		if(inc>0)
			{
			// Only do the pixel stuff if we need to
			gamewin->GetPixel(x+x1,y+y1,&pixel);
			r=pixel.r;
			g=pixel.g;
			b=pixel.b;
			if(tint&TINT_RED)
				INC_COLOUR(r,inc);
			if(tint&TINT_GREEN)
				INC_COLOUR(g,inc);
			if(tint&TINT_BLUE)
				INC_COLOUR(b,inc);
//			putpixel(gamewin,x+x1,y+y1,makecol(r,g,b));
			gamewin->PutPixel(x+x1,y+y1,r,g,b);
			}
		}
}


#define DEC_COLOUR(cl,val)  {cl-=val;	if(cl<0) cl=0;}

void ProjectBlackCorona(VMINT xc,VMINT yc, VMINT radius,VMINT intensity, VMINT falloff, VMINT tint)
{
VMINT x,y,x1,y1,diam,xa,yb;
VMINT r,g,b,inc,xayb;
IRECOLOUR pixel;

// Get diameter
diam=radius<<1;

// Calculate new top-left (from centre position)
x1=xc-radius;
y1=yc-radius;

for(y=0;y<diam;y++)
	for(x=0;x<diam;x++)
		{
		xa=radius-x;
		yb=radius-y;
		// Calculate the squared value and clip it for the SQRT table
		xayb=(xa*xa)+(yb*yb);
		if(xayb>32768)
			xayb=32768;
		if(xayb<0)
			xayb=0;
		inc=intensity-(int)(((float)falloff/100.0)*ire_sqrt[xayb]);
		if(inc>0)
			{
			// Only do the pixel stuff if we need to
			gamewin->GetPixel(x+x1,y+y1,&pixel);
			r=pixel.r;
			g=pixel.g;
			b=pixel.b;
			if(tint&TINT_RED)
				DEC_COLOUR(r,inc);
			if(tint&TINT_GREEN)
				DEC_COLOUR(g,inc);
			if(tint&TINT_BLUE)
				DEC_COLOUR(b,inc);
			gamewin->PutPixel(x+x1,y+y1,r,g,b);
			}
		}
}



void InitConsoles()
{
memset(consolelist,0,sizeof(consolelist));

InitPicklists();
}


void TermConsoles()
{
int ctr;
for(ctr=0;ctr<MAXCONSOLES;ctr++)
	if(consolelist[ctr])
		delete consolelist[ctr];
memset(consolelist,0,sizeof(consolelist));
	
TermPicklists();
}


int CreateConsole(int x, int y, int wpix, int hpix)
{
if(x<0 || y<0 || wpix < 8 || hpix < 8)
	return 0;
  
for(int ctr=0;ctr<MAXCONSOLES;ctr++)
	if(!consolelist[ctr]) {
		consolelist[ctr] = new IRECONSOLE;
		if(consolelist[ctr]) {
			if(consolelist[ctr]->Init(x,y,wpix/8,hpix/8,0))
				return ctr+1;
			delete consolelist[ctr];
			consolelist[ctr]=NULL;
			}
		}
return 0;
}


void DeleteConsole(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

delete consolelist[console];
consolelist[console]=NULL;
}

void ClearConsole(int console) {
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

consolelist[console]->Clear();
}

void SetConsoleColour(int console, unsigned int packed)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

consolelist[console]->SetCol(packed);
}


void AddConsoleLine(int console, const char *line)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

consolelist[console]->AddLine(line);
}


void AddConsoleLineWrap(int console, const char *line)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

consolelist[console]->Printf(line);
consolelist[console]->Newline();
}


void DrawConsole(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

consolelist[console]->Update();
}


void ScrollConsole(int console, int dir)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

consolelist[console]->ScrollRel(dir);
}


void SetConsoleLine(int console, int line)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

consolelist[console]->SetPos(line);
}


void SetConsolePercent(int console, int percent)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

consolelist[console]->SetPos(percent);
}

void SetConsoleSelect(int console, int line)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

consolelist[console]->SelectLine(line);
}

void SetConsoleSelect(int console, const char *line)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

consolelist[console]->SelectText(line);
}


int GetConsoleLine(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return 0;
if(!consolelist[console])
	return 0;

return consolelist[console]->GetPos();
}


int GetConsolePercent(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return 0;
if(!consolelist[console])
	return 0;

return consolelist[console]->GetPercent();
}

int GetConsoleSelect(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return 0;
if(!consolelist[console])
	return 0;

return consolelist[console]->GetSelect();
}

const char *GetConsoleSelectS(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return 0;
if(!consolelist[console])
	return 0;

return consolelist[console]->GetSelectText();
}


int GetConsoleLines(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return 0;
if(!consolelist[console])
	return 0;

return consolelist[console]->GetLines();
}


int GetConsoleRows(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return 0;
if(!consolelist[console])
	return 0;

return consolelist[console]->GetRows();
}



void ConsoleJournal(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

//printf("Replay journal\n");

JOURNALENTRY *ptr;
for(ptr=Journal;ptr;ptr=ptr->next)
	{
// 	printf("Jrnl: %s\n",ptr->text);
	if(ptr->title)
		consolelist[console]->Printf("%s - %s\n",ptr->date,ptr->title);
	else
		consolelist[console]->Printf("%s\n",ptr->date);
	consolelist[console]->Newline();
	consolelist[console]->Printf(ptr->text);
	consolelist[console]->Newline();
	consolelist[console]->Newline();
	}
}


void ConsoleJournalDay(int console, int day)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

//printf("Replay journal\n");

JOURNALENTRY *ptr;
for(ptr=Journal;ptr;ptr=ptr->next)
	if(ptr->day == day)
		{
// 	printf("Jrnl: %s\n",ptr->text);
		if(ptr->title)
			consolelist[console]->Printf("%s - %s\n",ptr->date,ptr->title);
		else
			consolelist[console]->Printf("%s\n",ptr->date);
		consolelist[console]->Newline();
		consolelist[console]->Printf(ptr->text);
		consolelist[console]->Newline();
		consolelist[console]->Newline();
		}
}

void ConsoleJournalTasks(int console, int mode) {
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!consolelist[console])
	return;

JOURNALENTRY *ptr;
for(ptr=Journal;ptr;ptr=ptr->next) {
	if(!ptr->tag || stricmp(ptr->tag, "task")) {
		continue;
	}
	if(mode == JOURNAL_TODO_ONLY || mode == JOURNAL_TODO_HEADER_ONLY) {
		if(ptr->status != 0) {
			continue;
		}
	}
	if(mode == JOURNAL_DONE_ONLY || mode == JOURNAL_DONE_HEADER_ONLY) {
		if(ptr->status == 0) {
			continue;
		}
	}

	if(ptr->title)
		consolelist[console]->Printf("* %s\n",ptr->title);
	else
		consolelist[console]->Printf("!!! TASK '%s' HAS NO TITLE!\n",ptr->name);
	consolelist[console]->Newline();
	if(mode != JOURNAL_TODO_HEADER_ONLY && mode != JOURNAL_DONE_HEADER_ONLY) {
		consolelist[console]->Printf(ptr->text);
		consolelist[console]->Newline();
	}
	consolelist[console]->Newline();
}
}



void make_journaldate(char *date)
{
char *dayword = getstring4name("str_Day");
if(!date)
	return;

if(!dayword)
	dayword = "DAY";

switch(journalstyle)
	{
	case 1:
		snprintf(date,32,"%04d/%02d/%02d",(int)game_year,(int)game_month,(int)game_day);
		break;
	case 2:
		snprintf(date,32,"%04d/%02d/%02d %02d:%02d",(int)game_year,(int)game_month,(int)game_day,(int)game_hour,(int)game_minute);
		break;
	  
	default:
		snprintf(date,32,"%s %d",dayword,(int)days_passed+1);
		break;
	}
}



void InitPicklists()
{
memset(Picklistlist,0,sizeof(Picklistlist));
}


void TermPicklists()
{
int ctr;
for(ctr=0;ctr<MAXCONSOLES;ctr++)
	{
	if(Picklistlist[ctr].win)
		delete Picklistlist[ctr].win;
	if(Picklistlist[ctr].numlist)
		M_free(Picklistlist[ctr].numlist);
	}
memset(Picklistlist,0,sizeof(Picklistlist));
}


int CreatePicklist(int x, int y, int wpix, int hpix)
{
if(x<0 || y<0 || wpix < 8 || hpix < 8)
	return 0;
  
for(int ctr=0;ctr<MAXCONSOLES;ctr++)
	if(!Picklistlist[ctr].win) {
		Picklistlist[ctr].win = new IREPICKLIST;
		if(Picklistlist[ctr].win) {
			if(Picklistlist[ctr].win->Init(x,y,wpix,hpix,NULL,0))
				return ctr+1;
			delete Picklistlist[ctr].win;
			Picklistlist[ctr].win=NULL;
			}
		}
return 0;
}


void DeletePicklist(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!Picklistlist[console].win)
	return;

delete Picklistlist[console].win;
Picklistlist[console].win=NULL;

if(Picklistlist[console].numlist)
	M_free(Picklistlist[console].numlist);
Picklistlist[console].numlist=NULL;
Picklistlist[console].nums=0;
}


void SetPicklistColour(int console, unsigned int packed)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!Picklistlist[console].win)
	return;

IRECOLOUR newcol;
newcol.Set(packed);
Picklistlist[console].win->SetColour(&newcol);
}


void PollPicklist(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
IREPICKLIST *picklist = Picklistlist[console].win;
if(!picklist)
	return;

int mb,z;
int key=-1;

picklist->Update();

z=0;
IRE_GetMouse(NULL,NULL,&z,NULL);
if(z<0)
	key=IREKEY_UP;
if(z>0)
	key=IREKEY_DOWN;

if(IRE_KeyPressed() || key != -1)
	{
	irekey=get_key_quiet();
	if(key != -1)
		irekey=key; // Get the scrollwheel key
	picklist->Process(irekey);
	swapscreen->HideMouse();
	swapscreen->Render();
	swapscreen->ShowMouse();
	}
else
	{
	IRE_WaitFor(1);  // Yield
	swapscreen->HideMouse();
	swapscreen->Render();
	swapscreen->ShowMouse();
		
	// Check mouse ranges
	IRE_GetMouse();
	CheckMouseRanges();
	if(MouseID != -1)
		{
		IRE_WaitFor(MouseWait);
		irekey=IREKEY_MOUSE;
		}
		
	// Now process the list
	if(IRE_GetMouse(NULL,NULL,NULL,&mb))
		if(mb&IREMOUSE_LEFT)
			picklist->Process(-1);
	}
}


void AddPicklistLine(int console, int id, const char *line)
{
int *newnum;
	
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!Picklistlist[console].win)
	return;

// Add the line to the picklist
Picklistlist[console].win->AddLine(line);

// Add the corresponding ID number to the internal store

//printf("listnum = %d, add new ID %d.  Name = %s\n",Picklistlist[console].nums,id,line);
newnum = (int *)M_resize(Picklistlist[console].numlist,(Picklistlist[console].nums+1)*sizeof(int));
if(newnum)
	{
	Picklistlist[console].numlist=newnum;
	newnum[Picklistlist[console].nums]=id;
	Picklistlist[console].nums++;
	}
}


void AddPicklistSave(int console, int savegame)
{
const char *title;
int blank;

// ilog_quiet("AddPicklistSave add %d\n",savegame);
	
if(savegame < 1)
	return;

title = read_sgheader(savegame,&blank);
if(!title)
	return;

// ilog_quiet("AddPicklistSave read header ok, adding to list\n");

AddPicklistLine(console,savegame,title);
}


int GetPicklistItem(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return 0;
if(!Picklistlist[console].win)
	return 0;

return Picklistlist[console].win->GetItem();
}


int GetPicklistId(int console)
{
int pos;
	
console--;

ilog_quiet("GetPicklistID console = %d\n",console);

if(console <0 || console >= MAXCONSOLES)
	 return -1;

ilog_quiet("GetPicklistID number okay, check window %p\n",Picklistlist[console].win);

if(!Picklistlist[console].win)
	return -1;

ilog_quiet("GetPicklistID number okay, check numberlist %p\n",Picklistlist[console].numlist);

if(!Picklistlist[console].numlist)
	return -1;

pos=Picklistlist[console].win->GetItem();
if(pos >= 0 && pos < Picklistlist[console].nums)
	return Picklistlist[console].numlist[pos];

ilog_quiet("GetPicklistID pos %d out of range, bailing\n",pos);

return -1;
}


const char *GetPicklistText(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return 0;
if(!Picklistlist[console].win)
	return 0;

return Picklistlist[console].win->GetSelectText();
}


void SetPicklistLine(int console, int line)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!Picklistlist[console].win)
	return;

Picklistlist[console].win->SetPos(line);
}


void SetPicklistPercent(int console, int percent)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return;
if(!Picklistlist[console].win)
	return;

Picklistlist[console].win->SetPercent(percent);
}


int GetPicklistLine(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return 0;
if(!Picklistlist[console].win)
	return 0;

return Picklistlist[console].win->GetPos();
}


int GetPicklistPercent(int console)
{
console--;
if(console <0 || console >= MAXCONSOLES)
	 return 0;
if(!Picklistlist[console].win)
	return 0;

return Picklistlist[console].win->GetPercent();
}



int FindLastSave()
{
int blank,listtotal;
listtotal=0;

for(int ctr=1;ctr<MAX_SAVES;ctr++)
	if(read_sgheader(ctr,&blank))
		listtotal=ctr;

ilog_quiet("FindLastSave returns %d\n",listtotal);

return listtotal;
}
