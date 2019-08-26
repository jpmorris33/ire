
/*
 *  - Objects menu tab
 */


#include <stdio.h>
#include "igui.hpp"
#include "../ithelib.h"
#include "../console.hpp"
#include "../core.hpp"
#include "../resource.hpp"
#include "../init.hpp"
#include "../linklist.hpp"
#include "../loadfile.hpp"
#include "../object.hpp"
#include "../gamedata.hpp"

#include "menusys.h"

// DEFINES

#define POCKET_X 144
#define POCKET_Y 80
#define POCKET_H 34

#define PAD_OUT(a,b) PadOut(a,b);

#ifdef WINDOWS_IS_BROKEN
#define snprintf _snprintf
#endif

struct OBJTAG
	{
	char label[48];
	OBJECT *obj;
	};
	
struct STATSENTRY
	{
	char label[128];
	long *value;
	long *max;
	};

// VARIABLES

static int sel_xoff=0;  // Offset from where we started the drag
static int sel_yoff=0;  // so that the dragging is centered on that spot
                        // i.e. we drag it by the point we first clicked on

extern int focus,OB_Id; // GUI variables

extern int MapXpos_Id,MapYpos_Id;       // Map X,Y coordinate boxes
extern char mapxstr[],mapystr[];

extern int l_proj;      // Flag, are layers being displayed?
extern int sx_proj;     // Flag, are other sprites being excluded? 1 = OFF
extern int ow_proj;     // Flag, are Owned Objects being highlighted?
extern long mapx,mapy;   // Current map position

extern OBJECT *objsel;  // Object that is currently selected.  Can be NULL
extern OBJECT *schsel;  // Object to be highlighted in Schedule editor, NULL
extern OBJECT *cutbag;

static char last_sprite[256];  // Type of the Last sprite that was created
static int  last_dir=0;
static OBJECT *pocsel=NULL;
static OBJECT *lastOwner=NULL;
int DirU_Id=-1,DirD_Id=-1,DirL_Id=-1,DirR_Id=-1;
int obj_xId=-1,obj_yId=-1,obj_name_Id=-1,obj_pname_Id=-1,obj_oname_Id=-1,obj_loc_Id=-1;
static char **UserList;

static char tag_str[]="          ";
char obj_x_str[7],obj_y_str[7],obj_name_str[17];
char obj_pname_str[33];
char obj_oname_str[33];
char obj_loc_str[33];
int tag_Id=-1,offset=0,editpocket=0;

static char none[]="-";

static char STime_str[]=" 00:00 ";
static char SVrm_str[]="                ";
static char SObj_str[]="                ";
static char SLoc_str[]="                ";
static int STime_Id=-1,SVrm_Id=-1,SObj_Id=-1,SLoc_Id=-1,setschedule=-1;

static char str32[33];
static int behave_Id=-1,use_Id=-1,kill_Id=-1,look_Id=-1,stand_Id=-1,hurt_Id=-1,init_Id=-1,wield_Id=-1,attack_Id=-1,talk_Id=-1,user1_Id=-1,user2_Id=-1;
static char edit_mode = 0;

// FUNCTIONS

extern void waitabit();
extern void Toolbar();
extern void DrawMap(int x,int y,char s_proj,char l_proj,char f_proj);
extern void gen_largemap();
static void Nothing();
static void clipmap();  // Make sure coordinates are within bounds
static void Deal_With_Contents(OBJECT *obj);
static void InsertObject(int x, int y);
static void PadOut(char *buf, int maxlen);
extern void (*EdUpdateFunc)(void);
extern void ResetStr32(char *str32);
static void SetOwnerRecursively(OBJECT *obj, OBJECT *owner);

void OB_up();           // Panning functions
void OB_down();
void OB_left();
void OB_right();

void OB_QuickInsert();  // For fast replication of known objects
void OB_SlowInsert();   // For choosing and object before creating it
void OB_Edit();         // Choose the type
void OB_Delete();       // Delete it
void OB_Pick();         // Left click on the map to pick one up
void OB_MoveThere();    // Right click on the map to move it faster

void OB_DirU();         // Point the object up
void OB_DirD();         // Point the object down
void OB_DirL();         // Point the object left
void OB_DirR();         // Point the object right

void OB_Dir(int dir);   // Set the current direction, helper

void OB_SetTag();       // Set Tag
void OB_ShowTags();      // Find an object with a certain Tag
void OB_FreeTag();      // Get next free tag
void OB_Find();         // Find an object by name
void OB_FindFirst();
void OB_FindNext();

void SelectObj(int cx,int cy);  // Do the actual picking up
void OB_Update();             // Update the direction display

extern int ReadMap(int x,int y);
extern void WriteMap(int x,int y, int tile);
extern int isSolid(int x,int y);


static void OB_SetPersonalName();
static void GetMapX();                  // Get map position
static void GetMapY();
static void GetObjX();                  // Get object position
static void GetObjY();
static void OB_debug();
static void OB_calcdist();
static void RandomObject();

static void OB_Cut();
static void OB_Kopy();
static void OB_Paste();
static void OB_Wipe();
static void OB_Wipe1();
static void OB_Uproot();

static void OB_Schedule();
static void inSchedule(int x, int y, int lines);
static void OB_SetSchedule();
static void SReset();
static void SetSTime();
static void SetSVrm();
static void SetSObject();
static void SC_MakeTarget();
static void SC_NoTarget();

static void ResetObject();

static void OB_Pocket();
static void inPocket(int x, int y, int lines);
static void PO_Next();
static void PO_Prev();
static void PO_Add();
static void PO_Del();
static void PO_Highlight();
static void PO_In();
static void PO_Out();

extern void Snap();

static void OB_Stats();
static void Finish();
//static void Resettt(); unused functions

static void EditBehaviour();
static void EditBehaviour2();
static void Behave_Update();
static void Behave_Update2();
static void SetBehave();
static void SetUse();
static void SetKill();
static void SetLook();
static void SetStand();
static void SetHurt();
static void SetWield();
static void SetAttack();
static void SetInit();
static void SetTalk();
static void SetUser1();
static void SetUser2();
static int PickVrm(char *original, char Class, char *SkipClass);

static void SetOwner();
static void OBO_MakePublic();
static void OBO_FromList();
static void OBO_MakeOwner();
static void OBO_LastOwner();
//static void FG_Animate(); unused function
static void OB_SetLocation();
static void MassSetOwner();

static int CMPTAG(const void *a,const void *b);
static void UprootProgress(int count,int max);
static int StatEdit(OBJECT *o);

// Parts of the OB_Edit browser

void GetOBFromList( int x0, int y0, char *prompt, int listsize, char **list, char *name);
void OB_Thumb(int x,int y,char *name);
//int OB_Thumb(int msg, DIALOG *d, int c);


// CODE

/*
 * GoFocal - The menu tab function.  This is called from the toolbar
 */

void OB_GoFocal()
{
int temp;               // Used to get button handles

if(focus==2)            // Make sure we don't redraw if the user clicks again
	return;

focus=2;                // This is now the focus
Toolbar();              // Build up the menu system again from scratch
IG_SetFocus(OB_Id);     // And make the button stick inwards


PAD_OUT(obj_x_str,7);
PAD_OUT(obj_y_str,7);
PAD_OUT(obj_name_str,17);
PAD_OUT(obj_pname_str,33);
PAD_OUT(obj_oname_str,33);
PAD_OUT(obj_loc_str,33);

edit_mode = 0;

// Set up Map window

IG_BlackPanel(VIEWX-1,VIEWY-1,258,258);
IG_Region(VIEWX,VIEWY,256,256,OB_Pick,NULL,OB_MoveThere);

// Write sprite drawing status

temp = IG_ToggleButton(486,72,"Exclude ON ",Nothing,NULL,NULL,&sx_proj);
IG_SetInText(temp,"Exclude OFF");

temp = IG_ToggleButton(486,100,"Rooftops OFF",Nothing,NULL,NULL,&l_proj);
IG_SetInText(temp,"Rooftops ON ");

temp = IG_ToggleButton(486,128,"OwnedObj OFF",Nothing,NULL,NULL,&ow_proj);
IG_SetInText(temp,"OwnedObj ON ");

// Set up the X and Y map position counter at the top

MapXpos_Id = IG_InputButton(24,40,mapxstr,GetMapX,NULL,NULL);
MapYpos_Id = IG_InputButton(112,40,mapystr,GetMapY,NULL,NULL);

DrawMap(mapx,mapy,1,l_proj,0);                       // Draw the map

IG_TextButton(416,104,__up,OB_up,NULL,NULL);         // Pan up button
IG_TextButton(400,128,__left,OB_left,NULL,NULL);     // Pan left button
IG_TextButton(432,128,__right,OB_right,NULL,NULL);   // Pan right button
IG_TextButton(416,150,__down,OB_down,NULL,NULL);     // Pan down button

IG_AddKey(IREKEY_UP,OB_up);                             // Pan up key binding
IG_AddKey(IREKEY_DOWN,OB_down);                         // Pan down key binding
IG_AddKey(IREKEY_LEFT,OB_left);                         // Pan left key binding
IG_AddKey(IREKEY_RIGHT,OB_right);                       // Pan right key binding

IG_AddKey(IREKEY_8_PAD,OB_DirU);
IG_AddKey(IREKEY_2_PAD,OB_DirD);
IG_AddKey(IREKEY_4_PAD,OB_DirL);
IG_AddKey(IREKEY_6_PAD,OB_DirR);

IG_AddKey(IREKEY_7_PAD,OB_DirU); // 'diagonals' for corners etc
IG_AddKey(IREKEY_9_PAD,OB_DirR);
IG_AddKey(IREKEY_3_PAD,OB_DirD);
IG_AddKey(IREKEY_1_PAD,OB_DirL);

// Create objects

IG_TextButton(300,400,"New Item",OB_SlowInsert,NULL,NULL);
IG_AddKey(IREKEY_INSERT,OB_QuickInsert);

// Modify objects

IG_TextButton(380,400,"Edit Item",OB_Edit,NULL,NULL);
IG_AddKey(IREKEY_ENTER,OB_Edit);

// Delete objects

IG_TextButton(468,400,"Delete Item",OB_Delete,NULL,NULL);
IG_AddKey(IREKEY_DEL,OB_Delete);

// Random Objects
IG_TextButton(572,400,"Random",RandomObject,NULL,NULL);
//IG_AddKey(IREKEY_DELETE,OB_Delete);

// Object direction facings

DirU_Id = IG_Tab(598,274,__up,OB_DirU,NULL,NULL);
DirD_Id = IG_Tab(598,306,__down,OB_DirD,NULL,NULL);
DirL_Id = IG_Tab(580,289,__left,OB_DirL,NULL,NULL);
DirR_Id = IG_Tab(616,289,__right,OB_DirR,NULL,NULL);
IG_Text(492,304,"DIRECTION:",ITG_BLACK);


//IG_Text(128,358,"Tag:",BLACK);
//tag_Id = IG_InputButton(160,350,tag_str,OB_GetTag,NULL,NULL);

IG_TextButton(300,432,"Show tags",OB_ShowTags,NULL,NULL);
IG_Text(388,440,"TAG:",ITG_BLACK);
tag_Id = IG_InputButton(420,432,tag_str,OB_SetTag,NULL,NULL);
IG_AddKey(IREKEY_T,OB_SetTag);
IG_TextButton(510,432,"Next free tag",OB_FreeTag,NULL,NULL);

IG_Text(300,304,"X:",ITG_BLACK);
obj_xId = IG_InputButton(324,296,obj_x_str,GetObjX,NULL,NULL);
IG_Text(400,304,"Y:",ITG_BLACK);
obj_yId = IG_InputButton(424,296,obj_y_str,GetObjY,NULL,NULL);

IG_Text(300,264,"Object type:",ITG_BLACK);
obj_name_Id = IG_InputButton(400,256,obj_name_str,OB_Edit,NULL,NULL);

IG_TextButton(300,336,"Cut",OB_Cut,NULL,NULL);
IG_TextButton(340,336,"Copy",OB_Kopy,NULL,NULL);
IG_TextButton(388,336,"Paste",OB_Paste,NULL,NULL);
IG_TextButton(444,336,"Clear",OB_Wipe1,NULL,OB_Wipe);
IG_TextButton(500,336,"Uproot",OB_Uproot,NULL,NULL);

IG_AddKey(IREKEY_C,OB_Kopy);
IG_AddKey(IREKEY_X,OB_Cut);
IG_AddKey(IREKEY_V,OB_Paste);


IG_TextButton(300,368,"Pockets",OB_Pocket,NULL,NULL);
IG_AddKey(IREKEY_O,OB_Pocket);
IG_TextButton(372,368,"Statistics",OB_Stats,NULL,NULL);
IG_TextButton(468,368,"Behaviour",EditBehaviour,NULL,NULL);
IG_AddKey(IREKEY_B,EditBehaviour);
IG_TextButton(556,368,"RESET",ResetObject,NULL,NULL);

IG_TextButton(564,336,"Schedule",OB_Schedule,NULL,NULL);

//obj_pname_Id=IG_InputButton(32,432,obj_pname_str,OB_SetPersonalName,NULL,NULL);
//IG_Text(32,420,"Individual name:",BLACK);
obj_pname_Id=IG_InputButton(32,360,obj_pname_str,OB_SetPersonalName,NULL,NULL);
IG_Text(32,348,"Individual name:",ITG_BLACK);
obj_oname_Id=IG_InputButton(32,400,obj_oname_str,SetOwner,NULL,OBO_FromList);
IG_Text(32,388,"Object is property of:",ITG_BLACK);
IG_AddKey(IREKEY_P,SetOwner);

obj_loc_Id=IG_InputButton(32,440,obj_loc_str,OB_SetLocation,NULL,NULL);
IG_Text(32,428,"Object location:",ITG_BLACK);

//IG_TextButton(552,264,"Find",OB_Find,NULL,NULL);
IG_AddKey(IREKEY_F3,OB_Find);

// Debugging: perform consistency check

IG_AddKey(IREKEY_F8,OB_calcdist);
IG_AddKey(IREKEY_F9,OB_debug);

// Set state variables

pocsel=NULL;
editpocket=0;

OB_Update();

EdUpdateFunc=NULL;
//EdUpdateFunc=FG_Animate;

waitabit();
waitabit();
}

//unused funtion
/*
static void FG_Animate()
{
DrawMap(mapx,mapy,1,l_proj,0);             // Draw the map
}
*/

/*
 *   Nothing - Just draw the map.  Used by toggle-buttons
 */

void Nothing()
{
DrawMap(mapx,mapy,1,l_proj,0);     // Do something anyway
return;
}

/*
 *   OB_up - Scroll upwards
 */

void OB_up()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapy-=8;
else
	mapy--;
clipmap();
DrawMap(mapx,mapy,1,l_proj,0);
}

/*
 *   OB_down - Scroll downwards
 */

void OB_down()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapy+=8;
else
	mapy++;
clipmap();
DrawMap(mapx,mapy,1,l_proj,0);
}

/*
 *   OB_left - Scroll to the left
 */

void OB_left()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapx-=8;
else
	mapx--;
clipmap();
DrawMap(mapx,mapy,1,l_proj,0);
}

/*
 *   OB_right - Scroll to the right
 */

void OB_right()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapx+=8;
else
	mapx++;
clipmap();
DrawMap(mapx,mapy,1,l_proj,0);
}

/*
 *   OB_SlowInsert - Create an object and ask the user what it should be like
 */

void OB_SlowInsert()
{
objsel=OB_Alloc();              // Allocate the object
objsel->x=mapx+(VSW/2);         // Set reasonable defaults
objsel->y=mapy+(VSH/2);
MoveToMap(objsel->x,objsel->y,objsel);  // Let there be Consistency

sel_xoff = 0;                   // If we drag and drop, no grab offset
sel_yoff = 0;

objsel->name = CHlist[0].name;             // Give it a name
objsel->form = &SQlist[CHlist[0].dir[0]];  // And a shape

OB_Edit();              // Do this to set everything up

if(objsel->flags & IS_QUANTITY)
    objsel->stats->quantity=1;          // Make sure there is at least one
}

/*
 *   OB_QuickInsert - Create an object but assume is is the same as the last
 *                    (kind of drive-thru object creation)
 */

void OB_QuickInsert() {
int click_x,click_y,char_id;
OBJECT *obtemplate;
OBJECT *a;

obtemplate=objsel;

char_id = getnum4char(last_sprite);
if(char_id == -1) {     // Is it valid?
	OB_SlowInsert();                    // No, do it the slow way
	return;
}

// First do a quick bounds check, to prevent the creation of sprites
// where the user can't actually see.

click_x = ((x-VIEWX)>>5);       // Work out the position of the mouse
click_y = ((y-VIEWY)>>5);       // in tiles

if(click_x<0 || click_y<0)      // out the top/left of the screen?
    return;                     // Yes, abort
if(click_x>VSW-1 || click_y>VSH-1)  // Out the bottom/right of the screen?
    return;                     // Yes, abort

// Now do the full calculation.

click_x = ((x-VIEWX)>>5)+(mapx); // click_x = (mouse_x_pos/32) + map_x_pos
click_y = ((y-VIEWY)>>5)+(mapy); // click_y = (mouse_y_pos/32) + map_y_pos

objsel=OB_Alloc();              // Allocate the object

objsel->x=click_x;              // Use the current position
objsel->y=click_y;
MoveToMap(objsel->x,objsel->y,objsel);  // Let there be Consistency

if(objsel->flags & IS_QUANTITY)
	objsel->stats->quantity=1;          // Make sure there is at least one

sel_xoff = 0;                   // If we drag and drop, no grab offset
sel_yoff = 0;

OB_Init(objsel,last_sprite);    // Re-evaluate the character
ShrinkDecor(objsel);			// Save memory if possible

// If we're decorative, skip the rest of the doings
if(objsel->user->edecor) {
	DrawMap(mapx,mapy,1,l_proj,0);
	return;
}

Deal_With_Contents(objsel);     // Create/destroy any contents
OB_Dir(last_dir);               // Set the direction

if(obtemplate) {
	a = objsel->stats->owner.objptr;   // Preserve pointers
	strcpy(objsel->personalname,obtemplate->personalname);
	memcpy(objsel->funcs,obtemplate->funcs,sizeof(FUNCS));
	memcpy(objsel->stats,obtemplate->stats,sizeof(STATS));
	objsel->stats->owner.objptr=a;     // Restore pointers
	if(obtemplate->labels->location) {
		SetLocation(objsel,obtemplate->labels->location);
	}
}

OB_Update();                    // Sort out the direction display

DrawMap(mapx,mapy,1,l_proj,0);
}

/*
 *   OB_Edit - Ask the user what an existing object should be like
 *             OB_SlowInsert calls this as its GUI.
 *             OB_Edit calls down to GetOBFromList to display the browser
 */

void OB_Edit()
{
char **list;
char Name[256];
int ctr;

// Make sure we have an object

if(!objsel)
	{
	Notify(-1,-1,"No sprite has been selected.","You must pick a sprite before you can modify it.");
	return;
	}

if(objsel->flags & IS_SYSTEM)
	{
	if(Confirm(-1,-1,"Can't modify system objects.","Do you want to delete it and create a new object?"))
		{
		OB_Delete();
		OB_SlowInsert();
		}
//	Notify(-1,-1,"Can't modify system objects.","You must delete it and create a new object.");
	return;
	}


// Make sure it's not a silly object

if(objsel==curmap->object)
	{
	Notify(-1,-1,"Not permitted.",NULL);
	return;
	}

ExpandDecor(objsel);	// If a shrunk decor, need to expand before changing

// Allocate a list of possible characters

list=(char **)M_get(CHtot+1,sizeof(char *));

// Set it up, convert to uppercase for Wyber's ordered selection routine.

for(ctr=0;ctr<CHtot;ctr++)
	{
	list[ctr]=CHlist[ctr].name;
	strupr(list[ctr]);              // Affects the original too
	}

strcpy(Name,objsel->name);      // Set up the default string

qsort(list,CHtot,sizeof(char*),CMP);  // Sort them for the dialog box

// This is the graphical dialog box, taken from DEU.

GetOBFromList( -1,-1, "Choose a character:", CHtot-1, list, Name);

// If the user didn't press ESC instead of choosing, modify the character

if(Name[0])     // It's ok, do it
	{
	OB_Init(objsel,Name);	// Re-evaluate the character
	ShrinkDecor(objsel);	// Save memory if possible
	Deal_With_Contents(objsel);     // Create/destroy any contents
	strcpy(last_sprite,Name);       // Keep this name for reference
	}

free(list);                             // Dispose of the list

if(!editpocket)
    {
    OB_Update();                          // Sort out the direction display
    DrawMap(mapx,mapy,1,l_proj,0);          // Update the map
    }
}

/*
 *   OB_Delete - Remove an object
 */

void OB_Delete()
{
if(!objsel)
	{
//	Notify(-1,-1,"No sprite has been selected.","You must pick a sprite before you can delete it.");
	return;
	}

if(objsel==lastOwner) {
	lastOwner = NULL;
}

if(objsel==curmap->object)                       // This can't happen
	{
	Notify(-1,-1,"Not Permitted",NULL);     // Get shirty
	return;
	}

ExpandDecor(objsel); // If it be a decorative object, normalise it first

ilog_quiet("Deleting %x\n",objsel);
ilog_quiet("(Type: %s)\n",objsel->name);
DestroyObject(objsel);                          // Destroy the object
objsel=NULL;                                    // Nothing currently selected
DrawMap(mapx,mapy,1,l_proj,0);                  // Draw the map again

sel_xoff = 0;                   // If we drag and drop, no grab offset
sel_yoff = 0;
IG_WaitForRelease();
}

/*
 *   OB_Pick - Drag and drop an object.  This calls down to SelectObj();
 */

void OB_Pick()
{
int click_x,click_y,nx,ny;

click_x = ((x-VIEWX)>>5)+(mapx); // click_x = (mouse_x_pos/32) + map_x_pos
click_y = ((y-VIEWY)>>5)+(mapy); // click_y = (mouse_y_pos/32) + map_y_pos

if(dragflag)                    // If we're dragging an object
	{
	if(!objsel)
		{
		Notify(-1,-1,"Internal error in drag'n'drop code",NULL);
		return;
		}

	nx=click_x + sel_xoff;    // Get clicked position
	ny=click_y + sel_yoff;

	if(nx<0) nx=0;     // Sanity checks.
	if(ny<0) ny=0;

//        MoveObject(objsel,nx,ny);       // Move it.
	TransferObject(objsel,nx,ny);       // Move it.

	dragflag=2;                      // Reset drag timer
	}
else                                     // If we're not dragging, or time out
	{
	SelectObj(click_x,click_y);      // Go fish

	if(objsel)                       // If an object was selected
		{
		dragflag=2;                        // We go into drag mode
		strcpy(last_sprite,objsel->name);  // We'll make more of these
		last_dir = objsel->curdir;

		objsel->form=NULL; // Force direction to change
		OB_SetDir(objsel,objsel->curdir,1);
		}
	}

DrawMap(mapx,mapy,1,l_proj,0);           // Draw the map again
OB_Update();
}

/*
 *   OB_MoveThere - Move an object to the current square without dragging it
 */

void OB_MoveThere()
{
int click_x,click_y,nx,ny;

click_x = ((x-VIEWX)>>5)+(mapx); // click_x = (mouse_x_pos/32) + map_x_pos
click_y = ((y-VIEWY)>>5)+(mapy); // click_y = (mouse_y_pos/32) + map_y_pos

if(objsel)
    {
    nx=click_x + sel_xoff;    // Get clicked position
    ny=click_y + sel_yoff;

    if(nx<0) nx=0;     // Sanity checks.
    if(ny<0) ny=0;

    gen_largemap();
//    MoveObject(objsel,nx,ny);       // Move it.
        TransferObject(objsel,nx,ny);       // Move it.
    DrawMap(mapx,mapy,1,l_proj,0);
    OB_Update();
    }
}

/*
 *   OB_SetTag - Set the TAG Id for an object
 *

void OB_SetTag()
{
int num;

if(!objsel)     // Abort if no object selected
    return;

num=InputIntegerValue(-1,-1,0,65535,objsel->tag);
objsel->tag = num;
OB_Update();

}
*/

/*
 *   OB_FindTag1 - Find an object with a certain tag
 */

void OB_FindTag1()
{
OBJECT *temp;
int tag=0;

// Get the tag to search for

tag=InputIntegerValue(-1,-1,-2100000000,2100000000,tag);

temp = FindTag(tag,NULL);

// Scan the linked list for that tag

if(!temp)
    {
    Notify(-1,-1,"Could not find an object using that tag number.",NULL);
    return;
    }

//if(InPocket(temp))
if(temp->parent.objptr)
    {
    Notify(-1,-1,"That item is inside a container.",NULL);
	objsel = temp->parent.objptr;
	// Look for the outermost container (in case it's nested)
	while(objsel->parent.objptr) objsel=objsel->parent.objptr;
    }
else
    objsel = temp;               // We've found one

mapx=objsel->x-(VSW/2);      // center the object in the view
mapy=objsel->y-(VSH/2);

clipmap();                     // Clip the view to sensible bounds
OB_Update();                   // Update tag number
DrawMap(mapx,mapy,1,l_proj,0); // Project
IG_WaitForRelease();
return;                        // Leave
}


/*
 *   OB_ShowTags - Show all tags in use
 */

void OB_ShowTags()
{
OBJLIST *t;
int tags,ctr;
OBJTAG *tag;
char **labels;
char buf[1024];

// Count tags in existence
tags=0;
for(t=MasterList;t;t=t->next)
	if(t->ptr && t->ptr->flags&IS_ON && t->ptr->tag != 0)
		tags++;

// Allocate sufficient memory
tag=(OBJTAG *)M_get(tags+1,sizeof(OBJTAG));
labels=(char **)M_get(tags+1,sizeof(char *));

// Now go through and get the tags into a list..
tags=0;
for(t=MasterList;t;t=t->next)
	if(t->ptr && t->ptr->flags&IS_ON && t->ptr->tag != 0)
		{
		sprintf(buf,"%ld - %s (%ld,%ld)",t->ptr->tag,t->ptr->name,t->ptr->x,t->ptr->y);
		//sprintf(buf,"%ld(%s) ",t->ptr->tag,t->ptr->name);
		strupr(buf);
		strncpy(tag[tags].label,buf,47);
		tag[tags].obj=t->ptr;
		tags++;
		}

// Sort them by number
qsort(tag,tags,sizeof(OBJTAG),CMPTAG);  // Sort them for the dialog box

// Build a string list to give to the menu
for(ctr=0;ctr<tags;ctr++)
	labels[ctr]=tag[ctr].label;

if(!labels[tags-1])	// Hack, don't have time to figure out what's wrong
	tags--;

strcpy(buf,"\0");
InputNameFromList32( -1,-1, "Choose an object", tags, labels, buf);

// If we got something, find it
if(strcmp(buf,""))
	{
	for(ctr=0;ctr<tags;ctr++)
		if(!strcmp(tag[ctr].label,buf))
			{
			// If any man knows any reason why this pointer should not be
			// the chosen object let him speak now or forever hold his piece
			if(!tag[ctr].obj) // "The pointer isn't valid!"
				break;
			if(tag[ctr].obj->parent.objptr) // "The pointer isn't in the real world!"
				{
				Notify(-1,-1,"That item is inside a container.",NULL);
				break;
				}
			// I now delare you to be the true chosen object..
			objsel=tag[ctr].obj;
			break;
			}
//	return;
	}

// Tidy up
M_free(tag);
M_free(labels);

if(!objsel)
	return;

// Resync position

mapx=objsel->x-(VSW/2);      // center the object in the view
mapy=objsel->y-(VSH/2);

clipmap();                     // Clip the view to sensible bounds
OB_Update();                   // Update tag number
DrawMap(mapx,mapy,1,l_proj,0); // Project
IG_WaitForRelease();
return;                        // Leave
}


/*
 *   OB_FreeTag - Find first free tag
 */

void OB_FreeTag()
{
OBJECT *temp;
char stemp[32];

// Take a number

for(int ctr=0;ctr<2100000000;ctr++)
    {
    temp = FindTag(ctr,NULL);
    if(!temp)
            {
            sprintf(stemp,"tag: %d",ctr);
            Notify(-1,-1,"The next free tag is:",stemp);
            return;
            }

    // If not we keep going round and round until we find one
    }

// Proclaim the impossible

Notify(-1,-1,"Probable System Fault","There are over 2 billion tags.  Are they really all used?");
}

/*
 *   OB_SetTag - Set the tag
 */

void OB_SetTag()
{
if(!objsel)
    {
    Notify(-1,-1,"You need to select an object to do this",NULL);
    return;
    }

// Get the new tag number

objsel->tag=InputIntegerValue(-1,-1,-2100000000,2100000000,objsel->tag);

// Update the display

OB_Update();
DrawMap(mapx,mapy,1,l_proj,0);
IG_WaitForRelease();
}

void ResetObject()
{
if(!objsel)
    return;

if(objsel->user->edecor)
	return;

if(!Confirm(-1,-1,"This will destroy any individual settings for this object.","Do you wish to do this?"))
    return;

OB_Init(objsel,objsel->name);
OB_Update();
DrawMap(mapx,mapy,1,l_proj,0);
IG_WaitForRelease();
}

/*
 *   OB_SetLocation - Pick a location or add a new one
 */

void OB_SetLocation()
{
int num;
char **labels;
char buf[1024];

if(!objsel)
	return;

num=0;
GetLocationList(&labels,&num);

qsort(labels,num,sizeof(char *),CMP);

strcpy(buf,"\0");
InputNameFromListWithAdd( -1,-1, "Enter a location name", num, labels, buf);

// If we got something, find it
if(strcmp(buf,""))
	SetLocation(objsel,buf);

if(num>0)
	M_free(labels);            // Don't free list if it was empty

OB_Update();                   // Update location
IG_WaitForRelease();
return;                        // Leave
}


/* =========================== Auxiliary functions ======================== */

/*
 *   SelectObj - fish for an object on the map where the user has clicked
 */

void SelectObj(int cx,int cy)
{
objsel = NULL;          // Assume nothing was there
sel_xoff = 0;           // No select offset
sel_yoff = 0;

gen_largemap();
objsel = GetObject(cx,cy);      // Go fishing

if(objsel)                      // Caught a fish
    {
    if(objsel->flags&IS_LARGE)             // If it's a big fish, find out
        {                               // what part of the object was
        sel_xoff = objsel->x-cx;        // grabbed, for drag'n'drop purposes
        sel_yoff = objsel->y-cy;
        }
    }

OB_Update();                    // Sort out the direction display
//DrawMap(mapx,mapy,1,l_proj,0);  // Redraw the map with the changes

// Time to go
return;
}

/*
 *    OB_Update - update direction arrows and that stuff
 */

void OB_Update()
{
if(edit_mode == 1)
    return;

IG_ResetFocus(DirU_Id); // Pull all the buttons out by default
IG_ResetFocus(DirD_Id);
IG_ResetFocus(DirL_Id);
IG_ResetFocus(DirR_Id);

if(!objsel)             // Abort, because no sprite was chosen
	return;
if(!(objsel->flags&IS_ON))
	return;

// Now find the right button to push in

switch(objsel->curdir)
    {
    case CHAR_U:
    IG_SetFocus(DirU_Id); // Up is stuck in
    break;

    case CHAR_D:
    IG_SetFocus(DirD_Id); // Down
    break;

    case CHAR_L:
    IG_SetFocus(DirL_Id); // Left
    break;

    case CHAR_R:
    IG_SetFocus(DirR_Id); // Right
    break;
    };

snprintf(tag_str,9,"%-9ld",objsel->tag);
tag_str[8]=0;
IG_UpdateText(tag_Id,tag_str);

snprintf(obj_x_str,7,"%-6ld",objsel->x);
obj_x_str[6]=0;
IG_UpdateText(obj_xId,obj_x_str);

snprintf(obj_y_str,7,"%-6ld",objsel->y);
obj_y_str[6]=0;
IG_UpdateText(obj_yId,obj_y_str);

snprintf(obj_name_str,17,"%-16s",objsel->name);
obj_name_str[16]=0;
IG_UpdateText(obj_name_Id,obj_name_str);

SAFE_STRCPY(obj_pname_str,objsel->personalname);
PAD_OUT(obj_pname_str,33);
IG_UpdateText(obj_pname_Id,obj_pname_str);

if(!objsel->stats->owner.objptr)
	{
	SAFE_STRCPY(obj_oname_str,"everyone");
	}
else
	if(!objsel->stats->owner.objptr->personalname)
		{
		SAFE_STRCPY(obj_oname_str,"nameless owner!");
		}
	else
		{
		SAFE_STRCPY(obj_oname_str,objsel->stats->owner.objptr->personalname);
		}

PAD_OUT(obj_oname_str,33);
IG_UpdateText(obj_oname_Id,obj_oname_str);

if(objsel->labels->location)
	{
	SAFE_STRCPY(obj_loc_str,objsel->labels->location);
	}
else
	strcpy(obj_loc_str,"-");
PAD_OUT(obj_loc_str,33);
IG_UpdateText(obj_loc_Id,obj_loc_str);
}

/*
 *   clipmap - clip the map's coordinates to sensible values
 */

void clipmap()
{
if(mapx<0) mapx=0;
if(mapx>curmap->w-VSW) mapx=curmap->w-VSW;
if(mapy>curmap->h-VSH) mapy=curmap->h-VSH;
if(mapy<0) mapy=0;
}

/*
 *   GetOBFromList - show thumbnails of the sprites you can choose from.
 *                   Called by OB_Edit, calls down to OB_Thumb
 */

void GetOBFromList( int x0, int y0, char *prompt, int listsize, char **list, char *name)
{
UserList=list;
InputNameFromListWithFunc( x0, y0, prompt, listsize, list, 15, name, 257, 257, OB_Thumb);
}


/*
 *   OB_Thumb - show thumbnails of the sprites you can choose from.
 *              Called by GetOBFromList
 */

void OB_Thumb(int x,int y,char *name)
{
int num;
fbox2(x,y,256,256,ITG_BLUE,swapscreen);

num = getnum4char(name);
if(num != -1)
        {
        fbox2(0,0,256,256,ITG_BLUE,gamewin);
        SQlist[CHlist[num].dir[0]].seq[0]->image->Draw(gamewin,0,0);
        gamewin->Draw(swapscreen,x,y);
        }
else
	IG_Text(x+4,y+32,"Not found",ITG_WHITE);
}


/*
 * OB_Dir - code to set up the object's direction, from the four defaults
 *          (up, down, left, right)
 */

void OB_Dir(int d)
{
IG_ResetFocus(DirU_Id); // All of them are stuck out,
IG_ResetFocus(DirD_Id); // but after we return the appropriate function
IG_ResetFocus(DirL_Id); // will make the right one sink in
IG_ResetFocus(DirR_Id); // If no was sprite selected, this is what we want

if(!objsel)             // Safety valve
    return;

if(objsel->curdir == d) // Are we already pointing upwards?
    return;             // If so, abort

OB_SetDir(objsel,d,1);    // Set the direction, reset visual form
last_dir = objsel->curdir;

DrawMap(mapx,mapy,1,l_proj,0);  // Redraw the map with the changes
}

/*
 *    OB_DirU - Make the object face Upwards
 */

void OB_DirU()
{
OB_Dir(CHAR_U);
if(!objsel)
    {
    Notify(-1,-1,"No sprite has been selected.","You must pick a sprite before you can modify it.");
    return;
    }
IG_SetFocus(DirU_Id);
IG_WaitForRelease();
}

/*
 *    OB_DirD - Make the object face Down
 */

void OB_DirD()
{
OB_Dir(CHAR_D);
if(!objsel)
    {
    Notify(-1,-1,"No sprite has been selected.","You must pick a sprite before you can modify it.");
    return;
    }
IG_SetFocus(DirD_Id);
IG_WaitForRelease();
}

/*
 *    OB_DirL - Make the object face Left
 */

void OB_DirL()
{
OB_Dir(CHAR_L);
if(!objsel)
    {
    Notify(-1,-1,"No sprite has been selected.","You must pick a sprite before you can modify it.");
    return;
    }
IG_SetFocus(DirL_Id);
IG_WaitForRelease();
}

/*
 *    OB_DirR - Make the object face Right
 */

void OB_DirR()
{
OB_Dir(CHAR_R);
if(!objsel)
    {
    Notify(-1,-1,"No sprite has been selected.","You must pick a sprite before you can modify it.");
    return;
    }
IG_SetFocus(DirR_Id);
IG_WaitForRelease();
}

/*
 *    GetMapX - Ask the user where on the map they want to be
 */

void GetMapX()
{
mapx=InputIntegerValue(-1,-1,0,curmap->w-VSW,mapx);
DrawMap(mapx,mapy,1,l_proj,0);
IG_WaitForRelease();
}

/*
 *    GetMapY - Ask the user where on the map they want to be
 */

void GetMapY()
{
mapy=InputIntegerValue(-1,-1,0,curmap->h-VSH,mapy);
DrawMap(mapx,mapy,1,l_proj,0);
IG_WaitForRelease();
}

/*
 *    GetObjX - Ask the user where on the map they want to be
 */

void GetObjX()
{
int newx;

if(objsel)
    {
    newx=InputIntegerValue(-1,-1,0,curmap->w-VSW,objsel->x);
//    MoveObject(objsel,newx,objsel->y);
        TransferObject(objsel,newx,objsel->y);
    }
DrawMap(mapx,mapy,1,l_proj,0);
OB_Update();
IG_WaitForRelease();
}

/*
 *    GetObjY - Ask the user where on the map they want to be
 */

void GetObjY()
{
int newy;
if(objsel)
    {
    newy=InputIntegerValue(-1,-1,0,curmap->h-VSH,objsel->y);
//    MoveObject(objsel,objsel->x,newy);
	TransferObject(objsel,objsel->x,newy);
    }
DrawMap(mapx,mapy,1,l_proj,0);
OB_Update();
IG_WaitForRelease();
}

void RandomObject()
{
int cx,cy,thresh;

if(!Confirm(-1,-1,"This will copy the current object","over all the random tiles on the map."))
	return;

thresh=InputIntegerValueP(-1,-1,0,100,5,"Enter Density");
if(thresh<0)
	return;

DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
IG_Text(VIEWX+72,VIEWY+72,"Processing..",ITG_WHITE);
Show();


for(cy=0;cy<curmap->h;cy++)
	{
	DrawScreenMeter(VIEWX+66,VIEWY+96,VIEWX+190,VIEWY+112,(float)cy/(float)curmap->h);
/*
	itoa(cy,str,10);
	DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
	IG_Text(VIEWX+72,VIEWY+72,str,ITG_WHITE);
	Show();
*/
	for(cx=0;cx<curmap->w;cx++)
		if(ReadMap(cx,cy)==RANDOM_TILE)
			if(!isSolid(cx,cy))
				{
				if(rand() % 100 <= thresh)
					{
					InsertObject(cx,cy);
					WriteMap(cx,cy,0);
					}
				}
	}
DrawMap(mapx,mapy,1,l_proj,0);
}


/*
 *    OB_debug - Check map consistency
 */

void OB_debug()
{
char good=1;
int vx,vy,yoff;
OBJECT *temp;
int objects=0,large=0;
char string1[128];
char string2[128];

for(vy=0;vy<curmap->h;vy++)
    {
    yoff=ytab[vy];
    for(vx=0;vx<curmap->w;vx++)
        {
        for(temp=curmap->objmap[yoff+vx];temp;temp=temp->next)
            if(temp)                            // Safety check
                {
                objects++;
                if(temp->x != vx)
                    good=0;
                if(temp->y != vy)
                    good=0;
                if(temp->flags&IS_LARGE)
                    large++;
                }
        }
    }
if(good)
    Notify(-1,-1,"Map OK.","All object locations consistent.");
else
    Notify(-1,-1,"Map Consistency Failed.","Some object locations inconsistent.");

if(curmap->object->next)
    Notify(-1,-1,"Unlinked objects detected",NULL);

sprintf(string1,"There are %d objects\n",objects);
sprintf(string2,"Of these %d are large objects\n",large);
Notify(-1,-1,string1,string2);
}

/*
 *    OB_calcdist - Calculate distance from object to owner
 */

void OB_calcdist()
{
char string1[128];
int tx,ty;

if(!objsel)
	return;
if(!objsel->stats->owner.objptr)
	return;

tx=objsel->x-objsel->stats->owner.objptr->x;
ty=objsel->y-objsel->stats->owner.objptr->y;
if(tx<0) tx=-tx;
if(ty<0) ty=-ty;


sprintf(string1,"X/Y distances to owner are %d,%d",tx,ty);
Notify(-1,-1,string1,NULL);
}


void OB_Wipe1()
{
Notify(-1,-1,"To wipe the current screen of objects,","click this control with the RIGHT mouse button.");
}

void OB_Wipe()
{
int cx,cy;
OBJECT *temp;

DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
IG_Text(VIEWX+72,VIEWY+72,"Processing..",ITG_WHITE);
Show();

for(cy=0;cy<VSH;cy++)
	{
	DrawScreenMeter(VIEWX+66,VIEWY+96,VIEWX+190,VIEWY+112,(float)cy/(float)VSH);
	for(cx=0;cx<VSW;cx++)
		{
		do	{
			temp=GetRawObjectBase(cx+mapx,cy+mapy);
			if(temp)
				DestroyObject(temp);
			} while(temp);
		}
	}

objsel=NULL;
DrawMap(mapx,mapy,1,l_proj,0);
OB_Update();
}

void OB_Uproot()
{
int cx,cy;
OBJECT *temp;

if(!Confirm(-1,-1,"This will destroy all objects above all Random Tiles",NULL))
	return;

DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
IG_Text(VIEWX+72,VIEWY+72,"Processing..",ITG_WHITE);
Show();

for(cy=0;cy<curmap->h;cy++)
	{
	for(cx=0;cx<curmap->w;cx++)
		{
		if(curmap->physmap[MAP_POS(cx,cy)] == RANDOM_TILE)
			{
			do	{
				temp=GetRawObjectBase(cx,cy);
				if(temp)
					{
					temp->flags &= ~IS_ON; // Mark for erasure
					temp=temp->next;
					}
				} while(temp);
			}
		}
	}

// SANCTIFY
MassCheckDepend();
NoDeps=1; // This is dangerous (pointer corruption) if left switched off

pending_delete=1;	// Force garbage collection
DeletePendingProgress(UprootProgress);
NoDeps=0; // Switch dependency checking on again

objsel=NULL;
DrawMap(mapx,mapy,1,l_proj,0);
OB_Update();
}

void UprootProgress(int count,int max)
{
if(max>0)
	DrawScreenMeter(VIEWX+66,VIEWY+96,VIEWX+190,VIEWY+112,(float)count/(float)max);
}

void OB_Cut()
{
int cx,cy;
OBJECT *temp;

// Clear the Cut+Paste buffer

// Previously we did something complex to try and handle the
// dependencies, but that no longer works.
// Instead, mark all the top-level objects as deleted and force a purge
// This will handle the dependencies much more quickly and cleanly
for(temp=cutbag->pocket.objptr;temp;temp=temp->next)
	if(temp)
		temp->flags &= ~IS_ON;
MassCheckDepend();

cutbag->tag=0; // Cut

DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
IG_Text(VIEWX+72,VIEWY+72,"Processing..",ITG_WHITE);
Show();

// Now populate it

for(cy=0;cy<VSH;cy++)
	{
	DrawScreenMeter(VIEWX+66,VIEWY+96,VIEWX+190,VIEWY+112,(float)cy/(float)VSH);
	for(cx=0;cx<VSW;cx++)
		{
		do	{
			temp=GetRawObjectBase(cx+mapx,cy+mapy);
			if(temp)
				{
				TransferToPocket(temp,cutbag);
				temp->x=cx;
				temp->y=cy;
				}
			} while(temp);
		}
	}
DrawMap(mapx,mapy,1,l_proj,0);
OB_Update();
}

void OB_Paste()
{
OBJECT *temp,*t2;

if(!cutbag->pocket.objptr)
	{
	ilog_quiet("Paste: Nothing in pocket\n");
	return;				// Nothing to do
	}

// Paste the stuff

if(cutbag->tag == 0) // Move originals out
	{
	do
		{
		if(cutbag->pocket.objptr)
			{
			// Wind to end, to unpack in reverse order
			for(temp=cutbag->pocket.objptr;temp->next;temp=temp->next);
			ForceFromPocket(temp,cutbag,temp->x+mapx,temp->y+mapy);
			}
		} while(cutbag->pocket.objptr);
	}

if(cutbag->tag == 1) // Make copies of the copies
	{
	for(temp=cutbag->pocket.objptr;temp;temp=temp->next)
		{
		t2=OB_Copy(temp);
		MoveToMap(temp->x+mapx,temp->y+mapy,t2);
//		TransferObject(t2,temp->x+mapx,temp->y+mapy);
		}
	}

DrawMap(mapx,mapy,1,l_proj,0);
OB_Update();
}


void OB_Kopy()
{
int cx,cy;
OBJECT *temp,*temp2;

// Clear the Cut+Paste buffer

do {
	if(cutbag->pocket.objptr)
		{
		temp=cutbag->pocket.objptr;
		LL_Remove(&cutbag->pocket.objptr,temp);
		FreePockets(temp);
		LL_Kill(temp);
		}
	} while(cutbag->pocket.objptr);

cutbag->tag=1; // Copy

DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
IG_Text(VIEWX+72,VIEWY+72,"Processing..",ITG_WHITE);
Show();

// Now populate it

for(cy=0;cy<VSH;cy++)
	{
	DrawScreenMeter(VIEWX+66,VIEWY+96,VIEWX+190,VIEWY+112,(float)cy/(float)VSH);
	for(cx=0;cx<VSW;cx++)
		{
		temp=GetRawObjectBase(cx+mapx,cy+mapy);
		for(;temp;temp=temp->next)
			{
			temp2=OB_Copy(temp);
			TransferToPocket(temp2,cutbag);
			temp2->x=cx;
			temp2->y=cy;
			}
		}
	}
DrawMap(mapx,mapy,1,l_proj,0);
OB_Update();
}


void Deal_With_Contents(OBJECT *obj)
{
OBJECT *temp;

if(obj->user->edecor)	// Not for decoratives
	return;
if(obj->flags & IS_SYSTEM)
	return;

if(!obj->pocket.objptr)
    {
    for(int ictr=0;ictr<objsel->funcs->contents;ictr++)
        {
        if(objsel->funcs->contains[ictr])
            {
            temp = OB_Alloc();
            temp->curdir = temp->dir[0];
            OB_Init(temp,objsel->funcs->contains[ictr]);
            OB_SetDir(temp,CHAR_D,1);
            MoveToPocket(temp,objsel);
            }
        }
    }
else
    FreePockets(obj);
}

void OB_Pocket()
{
if(!objsel)
    {
    Notify(-1,-1,"No sprite has been selected.","You must pick a sprite before you can modify it.");
    return;
    }

if(objsel->flags & IS_SYSTEM)
	return;

focus=0;
IG_KillAll();

DrawScreenBox3D(128,64,512,416);

inPocket(POCKET_X,POCKET_Y,POCKET_H);
IG_Region(POCKET_X,POCKET_Y,128,288,PO_Highlight,NULL,NULL);

IG_TextButton(304,80,__up,PO_Prev,NULL,NULL);
IG_TextButton(304,344,__down,PO_Next,NULL,NULL);

IG_TextButton(384,80, "Create object",PO_Add,NULL,NULL);
IG_TextButton(384,112,"Remove object",PO_Del,NULL,NULL);
IG_TextButton(384,144,"Move outside ",PO_Out,NULL,NULL);
IG_TextButton(384,176,"Bring inside ",PO_In,NULL,NULL);
IG_TextButton(384,384,"Finish",OB_GoFocal,NULL,NULL);

IG_AddKey(IREKEY_ESC,OB_GoFocal);     // ESC to quit this menu
IG_AddKey(IREKEY_F10,Snap);           // Add key binding for shift-F10

editpocket=1;
pocsel=NULL;
}

void PO_Prev()
{
offset--;
if(offset<0)
    offset=0;
inPocket(POCKET_X,POCKET_Y,POCKET_H);
}

void PO_Next()
{
offset++;
inPocket(POCKET_X,POCKET_Y,POCKET_H);
}

void PO_Add()
{
OBJECT *temp,*ooe;

if(!objsel)
    {
    Notify(-1,-1,"System error","This should not be able to happen");
    return;
    }

if(objsel->flags & IS_SYSTEM)
	return;

temp=OB_Alloc();              // Allocate the object

MoveToPocket(temp,objsel);

temp->name = CHlist[0].name;             // Give it a name
temp->form = &SQlist[CHlist[0].dir[0]];  // And a shape

// Because OB_Edit is designed to be a callback, with no parameters,
// it uses the global OBJSEL.  This time we want to call it with TEMP,
// so we need to do some shuffling.

ooe = objsel;
objsel = temp;

OB_Edit();              // Do this to set everything up

objsel = ooe;           // Restore objsel to normal

//temp->flags.seekstate=temp->flags.fixed;
temp->flags |= IS_FIXED;

if(temp->flags & IS_QUANTITY)
    temp->stats->quantity=1;          // Make sure there is at least one

inPocket(POCKET_X,POCKET_Y,POCKET_H);
IG_WaitForRelease();
}

void PO_Del()
{
if(!pocsel)
    {
    Notify(-1,-1,"No object has been selected","First highlight the object you want to remove.");
    return;
    }

if(!objsel)
    {
    Notify(-1,-1,"System error","This should not be able to happen");
    return;
    }

if(objsel->flags & IS_SYSTEM)
	return;

LL_Remove(&objsel->pocket.objptr,pocsel);
pocsel=NULL;
inPocket(POCKET_X,POCKET_Y,POCKET_H);
IG_WaitForRelease();
}

void PO_Out()
{
if(!objsel)
    {
    Notify(-1,-1,"System error","This should not be able to happen");
    return;
    }
if(!pocsel)
    {
    Notify(-1,-1,"No object has been selected","First highlight the object you want to leave the container.");
    return;
    }

if(objsel->flags & IS_SYSTEM)
	return;

ForceFromPocket(pocsel,objsel,objsel->x,objsel->y);
pocsel=NULL;
inPocket(POCKET_X,POCKET_Y,POCKET_H);
IG_WaitForRelease();
}

void PO_In()
{
OBJECT *temp;
if(!objsel)
    {
    Notify(-1,-1,"System error","This should not be able to happen");
    return;
    }

if(objsel->flags & IS_SYSTEM)
	return;

if(!Confirm(-1,-1,"This will get the object UNDERNEATH the container.","Do you wish to proceed?"))
    return;

temp = GetRawObjectBase(objsel->x,objsel->y);

if(temp == objsel)
    {
    Notify(-1,-1,"There are no objects beneath the container to pick up!",NULL);
    return;
    }

for(;temp->next;temp=temp->next)
    if(temp->next == objsel)
        {
        MoveToPocket(temp,objsel);
        pocsel=NULL;
        inPocket(POCKET_X,POCKET_Y,POCKET_H);
        IG_WaitForRelease();
        return;
        }

Notify(-1,-1,"Something went wrong.",NULL);
IG_WaitForRelease();
}

void inPocket(int x, int y, int lines)
{
OBJECT *temp;
int pos=0,line=0;

DrawScreenBoxHollow(x,y,x+(16*8)+16,y+(lines*8)+16);

for(temp=objsel->pocket.objptr;temp;temp=temp->next)
    {
    if(pos>=offset && line<lines)
        if(temp)
            {
            IG_Text(x+8,y+8,temp->name,ITG_WHITE);
            if(temp == pocsel)
                IG_Text(x+8,y+8,temp->name,ITG_RED);
            y+=8;
            line++;
            }
    pos++;
    }

if(line == 0 && offset == 0)
    IG_Text(x+8,y+8,"EMPTY",ITG_WHITE);
}


void PO_Highlight()
{
OBJECT *temp;
int pos=0,line=0,lines=0;
int sx,sy,cy;

sx = POCKET_X;
sy = POCKET_Y;
lines = POCKET_H;
pocsel=NULL;

cy = y-8;

for(temp=objsel->pocket.objptr;temp;temp=temp->next)
    {
    if(pos>=offset && line<lines)
        if(temp)
            {
            if(cy>=sy && cy <=(sy+8))
                pocsel=temp;
            sy+=8;
            line++;
            }
    pos++;
    }

inPocket(POCKET_X,POCKET_Y,POCKET_H);
IG_WaitForRelease();
}

void OB_Schedule()
{
schsel=NULL;
if(!objsel)
    {
    Notify(-1,-1,"No sprite has been selected.","You must pick a sprite before you can modify it.");
    return;
    }

if(objsel->flags & IS_SYSTEM)
	return;

focus=0;
IG_KillAll();

DrawScreenBox3D(96,64,544,432);

setschedule=-1;
inSchedule(POCKET_X,POCKET_Y,24);
IG_Region(POCKET_X,POCKET_Y,192,288,OB_SetSchedule,NULL,NULL);
IG_TextButton(384,POCKET_Y+32,"Finish",OB_GoFocal,NULL,NULL);

/*
STime_Id=IG_InputButton(128,392,STime_str,SetSTime,NULL,NULL);
SPri_Id=IG_InputButton(200,392,SPri_str,SetSPriority,NULL,NULL);
SVrm_Id=IG_InputButton(248,392,SVrm_str,SetSVrm,NULL,NULL);
SObj_Id=IG_InputButton(384,392,SObj_str,SetSObject,NULL,NULL);
*/

DrawSunkBox3D(368,POCKET_Y+80,520,POCKET_Y+280);
IG_TextButton(376,POCKET_Y+88,"Clear entry",SReset,NULL,NULL);
IG_Text(376,POCKET_Y+120,"Time:",ITG_WHITE);
STime_Id=IG_InputButton(376,POCKET_Y+128,STime_str,SetSTime,NULL,NULL);
//IG_Text(376,POCKET_Y+200,"VRM called:",ITG_WHITE);
//SVrm_Id=IG_InputButton(376,POCKET_Y+208,SVrm_str,SetSVrm,NULL,NULL);
IG_Text(376,POCKET_Y+160,"VRM called:",ITG_WHITE);
SVrm_Id=IG_InputButton(376,POCKET_Y+168,SVrm_str,SetSVrm,NULL,NULL);
IG_Text(376,POCKET_Y+200,"Target:",ITG_WHITE);
SObj_Id=IG_InputButton(376,POCKET_Y+208,SObj_str,SetSObject,NULL,NULL);
IG_Text(376,POCKET_Y+240,"Location:",ITG_WHITE);
SLoc_Id=IG_InputButton(376,POCKET_Y+248,SLoc_str,NULL,NULL,NULL);

IG_AddKey(IREKEY_ESC,OB_GoFocal);     // ESC to quit this menu
IG_AddKey(IREKEY_F10,Snap);           // Add key binding for shift-F10

editpocket=1;
IG_WaitForRelease();
}

void inSchedule(int x, int y, int lines)
{
char str[32];
int ctr;
IRECOLOUR *col = ITG_WHITE;

if(!objsel)
    return;

DrawScreenBoxHollow(x,y,x+(24*8)+16,y+(lines*12)+16);

for(ctr=0;ctr<24;ctr++)
	{
	if(ctr == setschedule)
		col = ITG_RED;
	else
		col = ITG_WHITE;

	if(!objsel->schedule[ctr].active)
		IG_Text(x+8,y+8,"-",col);
	else
		{
		snprintf(str,31,"%2d:%2d %8s",objsel->schedule[ctr].hour,objsel->schedule[ctr].minute,objsel->schedule[ctr].vrm);
		str[31]=0;
		ilog_quiet("%x:%s\n",objsel,objsel->name);
		IG_Text(x+8,y+8,str,col);
		}
	y+=12;
	}

if(setschedule < 0)
	return;

memset(STime_str,0,sizeof(STime_str));
if(!objsel->schedule[setschedule].active)
	{
	strcpy(STime_str," 00:00 ");
	strcpy(objsel->schedule[setschedule].vrm,"-");
	objsel->schedule[setschedule].target.objptr=NULL;
	}
else
	snprintf(STime_str,sizeof(STime_str)-1," %02d:%02d ",objsel->schedule[setschedule].hour,objsel->schedule[setschedule].minute);

// Assume location blank
strcpy(SLoc_str,"-               ");

strncpy(SVrm_str,objsel->schedule[setschedule].vrm,sizeof(SVrm_str)-1);
PAD_OUT(SVrm_str,17);
if(objsel->schedule[setschedule].target.objptr)
	{
	strncpy(SObj_str,objsel->schedule[setschedule].target.objptr->name,sizeof(SObj_str)-1);
	PAD_OUT(SObj_str,17);
	// Update location if there is one
	if(objsel->schedule[setschedule].target.objptr->labels->location)
		{
		strncpy(SLoc_str,objsel->schedule[setschedule].target.objptr->labels->location,sizeof(SLoc_str)-1);
		PAD_OUT(SLoc_str,17);
		}
	}
else
	strcpy(SObj_str,"Nothing         ");

IG_UpdateText(STime_Id,STime_str);
IG_UpdateText(SVrm_Id,SVrm_str);
IG_UpdateText(SObj_Id,SObj_str);
IG_UpdateText(SLoc_Id,SLoc_str);
}

void OB_SetSchedule()
{
int ctr;
int sx=POCKET_X+8;
int sy=POCKET_Y+8;

for(ctr=0;ctr<24;ctr++)
    {
    if(x>sx && x<sx+192)
        if(y>sy && y<(sy+12))
            {
            setschedule = ctr;
            inSchedule(POCKET_X,POCKET_Y,24);
            return;
            }
    sy+=12;
    }
IG_WaitForRelease();
}

void SReset()
{
if(setschedule < 0)
	return;
objsel->schedule[setschedule].active = 0;
inSchedule(POCKET_X,POCKET_Y,24);
IG_WaitForRelease();
}

void SetSTime()
{
char timestr[16];
int h,m;
if(setschedule < 0)
	return;

if(!objsel->schedule[setschedule].active)
	objsel->schedule[setschedule].hour=0;
sprintf(timestr,"%02d:%02d",objsel->schedule[setschedule].hour,objsel->schedule[setschedule].minute);
InputString(-1,-1,"Enter the time (HH:MM)",6,timestr);
sscanf(timestr,"%02d:%02d",&h,&m);

objsel->schedule[setschedule].hour=h;
objsel->schedule[setschedule].minute=m;
objsel->schedule[setschedule].active=1;
inSchedule(POCKET_X,POCKET_Y,24);
IG_WaitForRelease();
}

void SetSVrm()
{
int x;
if(setschedule < 0)
	return;

x = PickVrm(objsel->schedule[setschedule].vrm,'A',"P");
if(x==-1)
	strcpy(objsel->schedule[setschedule].vrm,"\0");
else
	{
	SAFE_STRCPY(objsel->schedule[setschedule].vrm,PElist[x].name);
	}

objsel->schedule[setschedule].call = x;
inSchedule(POCKET_X,POCKET_Y,24);
IG_WaitForRelease();
}

/*
 *
 *      EDIT STATISTICS
 *
 */

void OB_Stats()
{
if(!objsel)
    return;

StatEdit(objsel);
}


void OB_SetPersonalName()
{
if(!objsel)
    return;
InputIString(-1,-1,"Enter the character's individual name:",32,objsel->personalname);
OB_Update();
IG_WaitForRelease();
}

/*
 *
 *      EDIT BEHAVIOUR
 *
 */

//behave

/*
char contains[8][32];
char contents;
// Up to 8 objects can be put in the object when it's first created
*/


void EditBehaviour()
{
if(!objsel)
    {
    IG_WaitForRelease();
    return;
    }

if(objsel->flags & IS_SYSTEM)
	{
    IG_WaitForRelease();
	return;
	}

focus=0;
IG_KillAll();
ResetStr32(str32);

DrawScreenBox3D(32,128,608,400);

SetColor(ITG_WHITE);
DrawScreenText(48,160,"Initial behaviour:");
behave_Id =    IG_InputButton(224,152,str32,SetBehave,NULL,NULL);
DrawScreenText(48,192,"When used:");
use_Id =   IG_InputButton(224,184,str32,SetUse,NULL,NULL);
DrawScreenText(48,224,"When killed:");
kill_Id =   IG_InputButton(224,216,str32,SetKill,NULL,NULL);
DrawScreenText(48,256,"When looked at:");
look_Id = IG_InputButton(224,248,str32,SetLook,NULL,NULL);

DrawScreenText(48,288,"When stood on:");
stand_Id = IG_InputButton(224,280,str32,SetStand,NULL,NULL);
DrawScreenText(48,320,"When hurt:");
hurt_Id = IG_InputButton(224,312,str32,SetHurt,NULL,NULL);
DrawScreenText(48,352,"When created:");
init_Id = IG_InputButton(224,344,str32,SetInit,NULL,NULL);

IG_TextButton(512,152,"Finish",Finish,NULL,NULL);
IG_TextButton(512,344,"Page 2",EditBehaviour2,NULL,NULL);
IG_AddKey(IREKEY_ESC,OB_GoFocal);     // ESC to quit this menu (not save changes)
IG_AddKey(IREKEY_F10,Snap);           // Add key binding for shift-F10

Behave_Update();
}

void EditBehaviour2()
{
focus=0;
IG_KillAll();
ResetStr32(str32);

DrawScreenBox3D(32,128,608,400);

SetColor(ITG_WHITE);

DrawScreenText(48,160,"When wielded:");
wield_Id =    IG_InputButton(224,152,str32,SetWield,NULL,NULL);
DrawScreenText(48,192,"Attack script:");
attack_Id =   IG_InputButton(224,184,str32,SetAttack,NULL,NULL);
DrawScreenText(48,224,"User string 1:");
user1_Id =   IG_InputButton(224,216,str32,SetUser1,NULL,NULL);
DrawScreenText(48,256,"User string 2:");
user2_Id = IG_InputButton(224,248,str32,SetUser2,NULL,NULL);
/*

DrawScreenText(48,288,"When stood on:");
stand_Id = IG_InputButton(224,280,str32,SetStand,NULL,NULL);
DrawScreenText(48,320,"When hurt:");
hurt_Id = IG_InputButton(224,312,str32,SetHurt,NULL,NULL);
*/
DrawScreenText(48,352,"Speech File:");
talk_Id = IG_InputButton(224,344,str32,SetTalk,NULL,NULL);


IG_TextButton(512,344,"Page 1",EditBehaviour,NULL,NULL);
IG_TextButton(512,152,"Finish",Finish,NULL,NULL);
IG_AddKey(IREKEY_ESC,OB_GoFocal);     // ESC to quit this menu (not save changes)
IG_AddKey(IREKEY_F10,Snap);           // Add key binding for shift-F10

Behave_Update2();
}



void Behave_Update()
{
if(objsel->activity== -1)
	IG_UpdateText(behave_Id,none);
else
	IG_UpdateText(behave_Id,PElist[objsel->activity].name);

if(objsel->funcs->ucache == -1)
	IG_UpdateText(use_Id,none);
else
	IG_UpdateText(use_Id,PElist[objsel->funcs->ucache].name);

if(objsel->funcs->kcache == -1)
    IG_UpdateText(kill_Id,none);
else
    IG_UpdateText(kill_Id,PElist[objsel->funcs->kcache].name);

if(objsel->funcs->lcache == -1)
    IG_UpdateText(look_Id,none);
else
    IG_UpdateText(look_Id,PElist[objsel->funcs->lcache].name);

if(objsel->funcs->scache == -1)
    IG_UpdateText(stand_Id,none);
else
    IG_UpdateText(stand_Id,PElist[objsel->funcs->scache].name);

if(objsel->funcs->hcache == -1)
    IG_UpdateText(hurt_Id,none);
else
    IG_UpdateText(hurt_Id,PElist[objsel->funcs->hcache].name);

if(objsel->funcs->icache == -1)
    IG_UpdateText(init_Id,none);
else
    IG_UpdateText(init_Id,PElist[objsel->funcs->icache].name);
}

void Behave_Update2()
{

if(objsel->funcs->wcache == -1)
    IG_UpdateText(wield_Id,none);
else
    IG_UpdateText(wield_Id,PElist[objsel->funcs->wcache].name);

if(objsel->funcs->acache == -1)
    IG_UpdateText(attack_Id,none);
else
    IG_UpdateText(attack_Id,PElist[objsel->funcs->acache].name);

IG_UpdateText(user1_Id,objsel->funcs->user1);
IG_UpdateText(user2_Id,objsel->funcs->user2);


IG_UpdateText(talk_Id,objsel->funcs->talk);
}




void SetBehave()
{
if(objsel->activity == -1)
	objsel->activity = PickVrm(NULL,'A',"P");
else
	objsel->activity = PickVrm(PElist[objsel->activity].name,'A',"P");

Behave_Update();
IG_WaitForRelease();
}

void SetUse()
{
objsel->funcs->ucache = PickVrm(objsel->funcs->use,0,"AP");
if(objsel->funcs->ucache != -1)
    strcpy(objsel->funcs->use, PElist[objsel->funcs->ucache].name);

Behave_Update();
IG_WaitForRelease();
}

void SetKill()
{
objsel->funcs->kcache = PickVrm(objsel->funcs->kill,0,"AP");
if(objsel->funcs->kcache != -1)
    strcpy(objsel->funcs->kill, PElist[objsel->funcs->kcache].name);

Behave_Update();
IG_WaitForRelease();
}

void SetLook()
{
objsel->funcs->lcache = PickVrm(objsel->funcs->look,0,"AP");
if(objsel->funcs->lcache != -1)
    strcpy(objsel->funcs->look, PElist[objsel->funcs->lcache].name);

Behave_Update();
IG_WaitForRelease();
}

void SetStand()
{
objsel->funcs->scache = PickVrm(objsel->funcs->stand,0,"AP");
if(objsel->funcs->scache != -1)
    strcpy(objsel->funcs->stand, PElist[objsel->funcs->scache].name);

Behave_Update();
IG_WaitForRelease();
}

void SetHurt()
{
objsel->funcs->hcache = PickVrm(objsel->funcs->hurt,0,"AP");
if(objsel->funcs->hcache != -1)
    strcpy(objsel->funcs->hurt, PElist[objsel->funcs->hcache].name);

Behave_Update();
IG_WaitForRelease();
}

void SetWield()
{
objsel->funcs->wcache = PickVrm(objsel->funcs->wield,0,"AP");
if(objsel->funcs->wcache != -1)
    strcpy(objsel->funcs->wield, PElist[objsel->funcs->wcache].name);

Behave_Update2();
IG_WaitForRelease();
}

void SetAttack()
{
objsel->funcs->acache = PickVrm(objsel->funcs->attack,0,"AP");
if(objsel->funcs->acache != -1)
	strcpy(objsel->funcs->attack, PElist[objsel->funcs->acache].name);

Behave_Update2();
IG_WaitForRelease();
}

void SetUser1()
{
char tmp[32];

strcpy(tmp,objsel->funcs->user1);
InputIString(-1,-1,"Enter the user string:",32,tmp);
if(stricmp(objsel->funcs->user1,tmp))
	strcpy(objsel->funcs->user1, tmp);
Behave_Update2();
IG_WaitForRelease();
}

void SetUser2()
{
char tmp[32];

strcpy(tmp,objsel->funcs->user2);
InputIString(-1,-1,"Enter the user string:",32,tmp);
if(stricmp(objsel->funcs->user2,tmp))
	strcpy(objsel->funcs->user2, tmp);
Behave_Update2();
IG_WaitForRelease();
}

void SetInit()
{
objsel->funcs->icache = PickVrm(objsel->funcs->init,0,"AP");
if(objsel->funcs->icache != -1)
    strcpy(objsel->funcs->init,PElist[objsel->funcs->icache].name);

Behave_Update();
IG_WaitForRelease();
}

void SetTalk()
{
char before[128];
char newname[128];
char filename[1024];

strcpy(before,objsel->funcs->talk);
strcpy(newname,objsel->funcs->talk);

//InputString(-1,-1,"Enter the file with the character's speech (or '-' for none)",32,objsel->funcs->talk);
//InputString(-1,-1,"Enter the file with the character's speech (or '-' for none)",32,objsel->funcs->talk);
InputFileName(-1,-1,"Pick a conversation file",128,newname);

if(newname[0] == 0)
	strcpy(objsel->funcs->talk,before);
else
	strcpy(objsel->funcs->talk,newname);

if(!stricmp(objsel->funcs->talk,"-"))
	objsel->funcs->tcache=-1;
else
	{
	objsel->funcs->tcache=1;
//	printf("Checking file '%s'\n",objsel->funcs->talk);
	if(!loadfile(objsel->funcs->talk,filename))
		{
		strcpy(objsel->funcs->talk,"-"); // Nothing
		objsel->funcs->tcache=-1;
		}
//	printf("Checking file done\n");
	}

Behave_Update2();
IG_WaitForRelease();
}

void Finish()
{
IG_WaitForRelease();
OB_GoFocal();
}

int PickVrm(char *original, char Class, char *SkipClass)
{
char name[128];
char **mlist;
long num=0,ctr=0;

mlist = (char **)M_get(PEtot+1,sizeof(char *));

// If a null pointer
if(!original)
     original="-";

// If an empty string
if(!original[0])
     original="-";

mlist[num++]="-";
for(ctr=0;ctr<PEtot;ctr++)
	if(PElist[ctr].name)
		if(PElist[ctr].Class == Class || !Class)
			if(!SkipClass || !PElist[ctr].Class || !strchr(SkipClass,PElist[ctr].Class))
				{
				mlist[num]=PElist[ctr].name;
				strupr(mlist[num++]);
				}

qsort(mlist,num,sizeof(char *),CMP);

strcpy(name,original);
strupr(name);

//InputNameFromListWithFunc(-1,-1,"Choose a function to call:",num,mlist,5,name,0,0,NULL);
InputNameFromList(-1,-1,"Choose a function to call:",num,mlist,name);

if(!name[0])
    strcpy(name,original);

M_free(mlist);

for(ctr=0;ctr<PEtot;ctr++)
	if(PElist[ctr].name != NULL)
		if(PElist[ctr].Class == Class || !Class)
			if(!stricmp(PElist[ctr].name,name))
				return ctr;
return -1;
}

/*
 * SetOwner - Enter a special mode to set the object's owner
 */

void SetOwner()
{
if(!objsel)
	{
//	IG_WaitForRelease();
	if(Confirm(-1,-1,"No object selected.","Set owner for entire screen?"))
		MassSetOwner();
	return;
	}

focus=0;                // This is now the focus
IG_KillAll();
IG_Panel(0,32,640,480-32);

IG_BlackPanel(VIEWX-1,VIEWY-1,258,258);
IG_Region(VIEWX,VIEWY,256,256,OBO_MakeOwner,NULL,NULL);

IG_Text(300,64,"SET OWNER OF THE OBJECT",ITG_WHITE);
IG_Text(300,72,"Click on the character who will own it",ITG_WHITE);

// Set up the X and Y map position counter at the top

MapXpos_Id = IG_InputButton(24,40,mapxstr,GetMapX,NULL,NULL);
MapYpos_Id = IG_InputButton(112,40,mapystr,GetMapY,NULL,NULL);

DrawMap(mapx,mapy,1,l_proj,0);                       // Draw the map

IG_TextButton(416,104,__up,OB_up,NULL,NULL);         // Pan up button
IG_TextButton(400,128,__left,OB_left,NULL,NULL);     // Pan left button
IG_TextButton(432,128,__right,OB_right,NULL,NULL);   // Pan right button
IG_TextButton(416,150,__down,OB_down,NULL,NULL);     // Pan down button

IG_AddKey(IREKEY_UP,OB_up);                             // Pan up key binding
IG_AddKey(IREKEY_DOWN,OB_down);                         // Pan down key binding
IG_AddKey(IREKEY_LEFT,OB_left);                         // Pan left key binding
IG_AddKey(IREKEY_RIGHT,OB_right);                       // Pan right key binding

IG_TextButton(300,300,"Make Public Property",OBO_MakePublic,NULL,NULL);

IG_TextButton(300,332," Choose from a list ",OBO_FromList,NULL,NULL);

IG_TextButton(300,364,"  Reuse last owner  ",OBO_LastOwner,NULL,NULL);
IG_AddKey(IREKEY_R,OBO_LastOwner);

IG_TextButton(300,396,"  Cancel and Abort  ",OB_GoFocal,NULL,NULL);
IG_AddKey(IREKEY_ESC,OB_GoFocal);

IG_Text(VIEWX+4,356,"Previous owner:",ITG_WHITE);
IG_BlackPanel(VIEWX,368,256,16);
if(lastOwner) {
	IG_Text(VIEWX+4,372,lastOwner->personalname,ITG_WHITE);
} else {
	IG_Text(VIEWX+4,372,"No previous owner",ITG_WHITE);
}


edit_mode = 1;

IG_WaitForRelease();
}

void OBO_MakePublic()
{
lastOwner = NULL;
SetOwnerRecursively(objsel, NULL);
OB_GoFocal();
IG_WaitForRelease();
}

void OBO_LastOwner()
{
SetOwnerRecursively(objsel, lastOwner);
OB_GoFocal();
IG_WaitForRelease();
}

/*
 *      Find all 'probably sentient' objects and present a list of p-names
 */

void OBO_FromList()
{
OBJECT *a;
OBJLIST *o;
char *original;
char name[128];
char **mlist;
long num=0;
int x,y;

if(!objsel)
    {
    IG_WaitForRelease();
    return;
    }

// Find the name of the original owner, for a starting point

if(!objsel->stats->owner.objptr)
    original="-";
else
    if(!objsel->stats->owner.objptr->personalname)
        original="-";
    else
        original=objsel->stats->owner.objptr->personalname;

// Count the eligible candidates, which are either sentients or very strange.

/*
for(x=0;x<curmap->w;x++)
    for(y=0;y<curmap->h;y++)
        for(a=GetObjectBase(x,y);a;a=a->next)
            if(a->flags.person && a->personalname)
                num++;
*/

for(o=MasterList;o;o=o->next)
	if(o->ptr)
		if(o->ptr->flags&IS_PERSON && o->ptr->personalname)
			num++;

// Allocate space for them

mlist = (char **)M_get(num+1,sizeof(char *));

// Set up entry '-' (public property)

num=0;
mlist[num]=(char *)M_get(1,4);
strcpy(mlist[num++],"-");

// Gather them into the system and convert to lowercase

num=1;
/*
for(x=0;x<curmap->w;x++)
    for(y=0;y<curmap->h;y++)
        for(a=GetObjectBase(x,y);a;a=a->next)
            if(a->flags.person)
                if(a->personalname)
*/
for(o=MasterList;o;o=o->next)
	if(o->ptr)
		if(o->ptr->flags&IS_PERSON && o->ptr->personalname)
			if(strlen(o->ptr->personalname)>1)
				{
				mlist[num]=(char *)M_get(1,16);
				strcpy(mlist[num],o->ptr->personalname);
				strupr(mlist[num++]); // For the DEU system
				}

// Sort for the DEU menu system

qsort(mlist,num,sizeof(char *),CMP);

strcpy(name,original);
strupr(name);

// Do the querying

InputNameFromListWithFunc(-1,-1,"Choose the being who owns the object:",num-1,mlist,5,name,0,0,NULL);

// Set back to original owner if user pressed ESC

if(!name[0])
    strcpy(name,original);

// Free the menu, we won't need it anymore.

for(long ctr=0;ctr<num;ctr++)
    M_free(mlist[ctr]);
M_free(mlist);

// If it's just "-", then make it public property and finish

if(!stricmp(name,"-"))
    {
    lastOwner = NULL;
    SetOwnerRecursively(objsel, NULL);
    OB_GoFocal();
    IG_WaitForRelease();
    return;
    }

// If not, search all eligible candidates again, and look for that name.
// Set the pointer when we've got it, and then finish

for(x=0;x<curmap->w;x++)
    for(y=0;y<curmap->h;y++)
        for(a=GetObjectBase(x,y);a;a=a->next)
            if(a->personalname && a->flags&IS_PERSON)
                if(!stricmp(name,a->personalname))
                    {
		    lastOwner = a;
		    SetOwnerRecursively(objsel, a);
                    OB_GoFocal();
                    return;
                    }

// Oh dear.  I wonder what went wrong?
// Set it to be public to avoid nasty accident

Notify(-1,-1,"Oh bugger!",NULL);
lastOwner = NULL;
SetOwnerRecursively(objsel, NULL);
OB_GoFocal();
IG_WaitForRelease();
}

/*
 *   OBO_MakeOwner - Pick an object, based on OB_Pick
 */

void OBO_MakeOwner()
{
int click_x,click_y;
OBJECT *a;

a = objsel;

click_x = ((x-VIEWX)>>5)+(mapx); // click_x = (mouse_x_pos/32) + map_x_pos
click_y = ((y-VIEWY)>>5)+(mapy); // click_y = (mouse_y_pos/32) + map_y_pos

SelectObj(click_x,click_y);      // Go fish

if(objsel) {                       // If an object was selected
    last_dir = objsel->curdir;
    objsel->form=NULL; // Force direction to change
    OB_SetDir(objsel,objsel->curdir,1);

    if(!objsel->personalname) {
        if(Confirm(-1,-1,"The chosen object doesn't have an individual name!","Are you sure you want to do this?")) {
	   lastOwner = objsel;
	   SetOwnerRecursively(a,objsel);
        }
    } else {
	lastOwner = objsel;
	SetOwnerRecursively(a,objsel);
    }
}

objsel = a;

OB_GoFocal();
IG_WaitForRelease();
}

void MassSetOwner()
{
OBJECT *a,*owner=NULL;
OBJLIST *o;
static char name[128];
char **mlist;
long num=0;
int x,y;

for(o=MasterList;o;o=o->next)
	if(o->ptr)
		if(o->ptr->flags&IS_PERSON && o->ptr->personalname)
			num++;

// Allocate space for them

mlist = (char **)M_get(num+1,sizeof(char *));

// Set up entry '-' (public property)

num=0;
mlist[num]=(char *)M_get(1,4);
strcpy(mlist[num++],"-");

// Gather them into the system and convert to lowercase

num=1;
for(o=MasterList;o;o=o->next)
	if(o->ptr)
		if(o->ptr->flags&IS_PERSON && o->ptr->personalname)
			if(strlen(o->ptr->personalname)>1)
				{
				mlist[num]=(char *)M_get(1,16);
				strcpy(mlist[num],o->ptr->personalname);
				strupr(mlist[num++]); // For the DEU system
				}

// Sort for the DEU menu system

qsort(mlist,num,sizeof(char *),CMP);

//strcpy(name,"-");
strupr(name);

// Do the querying

InputNameFromListWithFunc(-1,-1,"Choose the being who owns the object:",num-1,mlist,5,name,0,0,NULL);

// Free the menu, we won't need it anymore.

for(long ctr=0;ctr<num;ctr++)
	M_free(mlist[ctr]);
M_free(mlist);

// User hit ESC?
if(!name[0])
	{
	IG_WaitForRelease();
	return;
	}

// If it's just "-", then make them public
if(!stricmp(name,"-"))
	owner = NULL;

// If not, search all eligible candidates again, and look for that name.
// Set the pointer when we've got it, and then do the thing

for(x=0;x<curmap->w;x++)
	for(y=0;y<curmap->h;y++)
		for(a=GetObjectBase(x,y);a;a=a->next)
			if(a->personalname && a->flags&IS_PERSON)
				if(!stricmp(name,a->personalname))
					{
					owner = a;
					lastOwner = a;
					break;
					}

// Now set the owners

for(y=0;y<VSH;y++) {
//	DrawScreenMeter(VIEWX+66,VIEWY+96,VIEWX+190,VIEWY+112,(float)cy/(float)VSH);
	for(x=0;x<VSW;x++) {
		for(a=GetRawObjectBase(x+mapx,y+mapy);a;a=a->next) {
			if(a) {
				SetOwnerRecursively(a,owner);
			}
		}
	}
}

//objsel=NULL;
DrawMap(mapx,mapy,1,l_proj,0);
OB_Update();
IG_WaitForRelease();
}




/*
 * SetSTarget - Enter a special mode to choose a target object for schedule
 */

void SetSObject()
{
if(!objsel)
    return;

if(setschedule<0)
	return;

focus=0;                // This is now the focus
//Toolbar();              // Build up the menu system again from scratch
IG_KillAll();
IG_Panel(0,32,640,480-32);


IG_BlackPanel(VIEWX-1,VIEWY-1,258,258);
IG_Region(VIEWX,VIEWY,256,256,SC_MakeTarget,NULL,NULL);

IG_Text(300,64,"SET THE SCHEDULE ACTIVITY TARGET",ITG_WHITE);
IG_Text(300,72,"Click on the object they will focus on",ITG_WHITE);

// Set up the X and Y map position counter at the top

MapXpos_Id = IG_InputButton(24,40,mapxstr,GetMapX,NULL,NULL);
MapYpos_Id = IG_InputButton(112,40,mapystr,GetMapY,NULL,NULL);

schsel = objsel->schedule[setschedule].target.objptr;
if(schsel)
	{
	mapx = schsel->x-4;
	mapy = schsel->y-4;
	clipmap();
	}
DrawMap(mapx,mapy,1,l_proj,0);                       // Draw the map

IG_TextButton(416,104,__up,OB_up,NULL,NULL);         // Pan up button
IG_TextButton(400,128,__left,OB_left,NULL,NULL);     // Pan left button
IG_TextButton(432,128,__right,OB_right,NULL,NULL);   // Pan right button
IG_TextButton(416,150,__down,OB_down,NULL,NULL);     // Pan down button

IG_AddKey(IREKEY_UP,OB_up);                             // Pan up key binding
IG_AddKey(IREKEY_DOWN,OB_down);                         // Pan down key binding
IG_AddKey(IREKEY_LEFT,OB_left);                         // Pan left key binding
IG_AddKey(IREKEY_RIGHT,OB_right);                       // Pan right key binding

IG_TextButton(300,300,"  Clear Target  ",SC_NoTarget,NULL,NULL);
IG_TextButton(300,332,"Cancel and Abort",OB_Schedule,NULL,NULL);
IG_AddKey(IREKEY_ESC,OB_Schedule);

edit_mode = 1;

//waitabit();
//waitabit();
IG_WaitForRelease();
}

/*
 *   SC_MakeTarget - Pick an object, based on OB_Pick
 */

void SC_MakeTarget()
{
int click_x,click_y;
OBJECT *a;

if(setschedule < 0)
	return;

a = objsel;

click_x = ((x-VIEWX)>>5)+(mapx); // click_x = (mouse_x_pos/32) + map_x_pos
click_y = ((y-VIEWY)>>5)+(mapy); // click_y = (mouse_y_pos/32) + map_y_pos

SelectObj(click_x,click_y);      // Go fish

if(objsel)                       // If an object was selected
    {
    last_dir = objsel->curdir;
    objsel->form=NULL; // Force direction to change
    OB_SetDir(objsel,objsel->curdir,1);
    a->schedule[setschedule].target.objptr = objsel;
    }
else
    {
    a->schedule[setschedule].target.objptr = NULL;
    }

objsel = a;

OB_Schedule();
IG_WaitForRelease();
}

void SC_NoTarget()
{
if(setschedule < 0)
	return;

objsel->schedule[setschedule].target.objptr=NULL;
schsel=NULL;
IG_WaitForRelease();
}


void InsertObject(int x, int y)
{
OBJECT *obtemplate;
OBJECT *a;
int char_id;

obtemplate=objsel;

char_id = getnum4char(last_sprite);
if(char_id == -1)     // Is it valid?
    {
    OB_SlowInsert();                    // No, do it the slow way
    return;
    }

objsel=OB_Alloc();              // Allocate the object

objsel->x=x;
objsel->y=y;
MoveToMap(objsel->x,objsel->y,objsel);  // Let there be Consistency

if(objsel->flags&IS_QUANTITY)
	objsel->stats->quantity=1;          // Make sure there is at least one

sel_xoff = 0;                   // If we drag and drop, no grab offset
sel_yoff = 0;

OB_Init(objsel,last_sprite);    // Re-evaluate the character
ShrinkDecor(objsel);			// Save memory if possible

// If we're decorative, skip the rest of the doings
if(objsel->user->edecor)
	return;

Deal_With_Contents(objsel);     // Create/destroy any contents
OB_Dir(last_dir);               // Set the direction

if(obtemplate)
	{
	a = objsel->stats->owner.objptr;   // Preserve pointers
	strcpy(objsel->personalname,obtemplate->personalname);
	memcpy(objsel->funcs,obtemplate->funcs,sizeof(FUNCS));
	memcpy(objsel->stats,obtemplate->stats,sizeof(STATS));
	objsel->stats->owner.objptr=a;     // Restore pointers
	}
}



/*
 *   OB_Find - Search for an object
 */

void OB_Find()
{
if(!objsel)
	OB_FindFirst();
else
	OB_FindNext();
}


void OB_FindFirst()
{
char **list;
char Name[256];
int ctr;
OBJECT *temp;

// Allocate a list of possible characters

list=(char **)M_get(CHtot+1,sizeof(char *));

// Set it up, convert to uppercase for Wyber's ordered selection routine.

for(ctr=0;ctr<CHtot;ctr++)
	{
	list[ctr]=CHlist[ctr].name;
	strupr(list[ctr]);              // Affects the original too
	}

strcpy(Name,list[0]);      // Set up the default string

qsort(list,CHtot,sizeof(char*),CMP);  // Sort them for the dialog box

// This is the graphical dialog box, taken from DEU.

GetOBFromList( -1,-1, "Choose a character:", CHtot-1, list, Name);

free(list);                             // Dispose of the list

// If the user didn't press ESC instead of choosing, modify the character

if(Name[0])     // It's ok, do it
	{
	for(int y=0;y<curmap->h-VSH;y++)
		for(int x=0;x<curmap->w-VSW;x++)
			for(temp = GetObjectBase(x,y);temp;temp=temp->next)
				if(temp)
					if(!stricmp(temp->name,Name))
						{
						mapx=x-(VSW/2);
						mapy=y-(VSH/2);
						if(mapx<0) mapx=0;
						if(mapy<0) mapy=0;
						objsel=temp;
						OB_Update();                          // Sort out the direction display
						DrawMap(mapx,mapy,1,l_proj,0);          // Update the map
						return;
						}
	}

Notify(-1,-1,"Nothing found.",NULL);
}

void OB_FindNext()
{
OBJECT *temp;
int x,y;

x=objsel->x;
y=objsel->y;

// Step past initial position
x++;
if(x>=curmap->w)
	{
	x=0;
	y++;
	}

while (y<curmap->h)
	{
	for(temp = GetObjectBase(x,y);temp;temp=temp->next)
		if(temp)
			if(!stricmp(temp->name,objsel->name))
				{
				mapx=x-(VSW/2);
				mapy=y-(VSH/2);
				if(mapx<0) mapx=0;
				if(mapy<0) mapy=0;
				objsel=temp;
				OB_Update();                          // Sort out the direction display
				DrawMap(mapx,mapy,1,l_proj,0);          // Update the map
				return;
				}

	x++;
	if(x>=curmap->w)
		{
		x=0;
		y++;
		}
	}

Notify(-1,-1,"Nothing found.",NULL);
}


int CMPTAG(const void *a,const void *b)
{
/*  
int ret = (*(OBJTAG *)a).obj->tag - (*(OBJTAG *)b).obj->tag;
if(ret == 0)
	return stricmp((*(OBJTAG *)a).obj->name,(*(OBJTAG *)b).obj->name);
return ret;
*/  

//printf("Sort = %s vs %s = return %d\n",(*(OBJTAG *)a).obj->name,(*(OBJTAG *)b).obj->name,stricmp((*(OBJTAG *)a).obj->name,(*(OBJTAG *)b).obj->name));
return stricmp((*(OBJTAG *)a).label,(*(OBJTAG *)b).label);
}


void ResetStr32(char *str32)
{
int temp;
for(temp=0;temp<32;temp++)
	str32[temp]=' ';
str32[32]=0;
}


void PadOut(char *buf, int maxlen)
{
int len,spare;
buf[maxlen-1]=0;
len = strlen(buf);
spare = maxlen-len;
if(spare > 0)
	memset(&buf[len],' ',spare);
buf[maxlen-1]=0;
}




/*
 *   Get savegame slot number from an Allegro dialog
 */
#define MAX_STATS 64

// Declare the stuff we'll need

static char *STL_StatList[MAX_STATS];
static int STL_ListLen=0;
static int EditStat(char *text, long *n1, long *n2);

STATSENTRY stats[MAX_STATS];
static int statpos=0;

// Do it

#define ADD_STAT1(a,b) {sprintf(stats[statpos].label,a,b);stats[statpos].value=& b;stats[statpos].max=NULL;statpos++;}
#define ADD_STAT2(a,b,c) {sprintf(stats[statpos].label,a,b,c);stats[statpos].value=& b;stats[statpos].max=&c;statpos++;}

int StatEdit(OBJECT *o)
{
int ctr,ret,ret2;
char name[128];

for(;;)
	{
	statpos=0;

	// Must be in alphabetical order!

	if(o->flags & IS_SYSTEM)
		{
		// No maxstats for System objects
		ADD_STAT1("Alignment       %ld",o->stats->alignment);
		ADD_STAT1("Armour          %ld",o->stats->armour);
		ADD_STAT1("Attack range    %ld",o->stats->range);
		ADD_STAT1("Bulk            %ld",o->stats->bulk);
		ADD_STAT1("Damage          %ld",o->stats->damage);
		ADD_STAT1("Dexterity       %ld",o->stats->dex);
		ADD_STAT1("Egg Radius      %ld",o->stats->radius);
		ADD_STAT1("Health          %ld",o->stats->hp);
		ADD_STAT1("Intelligence    %ld",o->stats->intel);
		ADD_STAT1("Karma           %ld",o->stats->karma);
		ADD_STAT1("Movement speed  %ld",o->maxstats->speed);
		ADD_STAT1("Quantity        %ld",o->stats->quantity);
		ADD_STAT1("Skill level     %ld",o->stats->level);
		ADD_STAT1("Strength        %ld",o->stats->str);
		ADD_STAT1("Weight          %ld",o->stats->weight);
		}
	else
		{
		ADD_STAT1("Alignment       %ld",o->stats->alignment);
		ADD_STAT2("Armour          %ld/%ld",o->stats->armour,o->maxstats->armour);
		ADD_STAT1("Attack range    %ld",o->stats->range);
		ADD_STAT2("Bulk            %ld/%ld",o->stats->bulk,o->maxstats->bulk);
		ADD_STAT2("Damage          %ld/%ld",o->stats->damage,o->maxstats->damage);
		ADD_STAT2("Dexterity       %ld/%ld",o->stats->dex,o->maxstats->dex);
		ADD_STAT1("Egg Radius      %ld",o->stats->radius);
		ADD_STAT2("Health          %ld/%ld",o->stats->hp,o->maxstats->hp);
		ADD_STAT2("Intelligence    %ld/%ld",o->stats->intel,o->maxstats->intel);
		ADD_STAT2("Karma           %ld/%ld",o->stats->karma,o->maxstats->karma);
		ADD_STAT1("Movement speed  %ld",o->maxstats->speed);
		ADD_STAT2("Quantity        %ld/%ld",o->stats->quantity,o->maxstats->quantity);
		ADD_STAT2("Skill level     %ld/%ld",o->stats->level,o->maxstats->level);
		ADD_STAT2("Strength        %ld/%ld",o->stats->str,o->maxstats->str);
		ADD_STAT1("Weight          %ld",o->stats->weight);
		}

	for(ctr=0;ctr<statpos;ctr++)
		STL_StatList[ctr] = stats[ctr].label;
	STL_ListLen = statpos;

	strcpy(name,STL_StatList[0]);
//	strupr(name);

	//InputNameFromListWithFunc(-1,-1,"Choose a function to call:",num,mlist,5,name,0,0,NULL);
	InputNameFromList(-1,-1,"Edit character stats:",STL_ListLen,STL_StatList,name);

	if(!name[0])
	    break;	// Pressed ESC
	
	for(ctr=0;ctr<statpos;ctr++)
		if(!stricmp(name,stats[ctr].label))
			EditStat(stats[ctr].label,stats[ctr].value,stats[ctr].max);
	};

return 1; // Return slot number
}


int EditStat(char *text, long *n1, long *n2)
{
int ret;
char prompt[1024];
char number1[64];
char number2[64];
char buffer[128];
char oldbuffer[128];
char *p,*firstword,*secondword;
long v1,v2;

if(!text || !n1)
	return 0;

// Trim the tabulation from the heading
strcpy(prompt,text);
p = strstr(prompt,"  ");
if(p)
	*p=0;

if(!n2)		{
	sprintf(buffer," %ld",*n1);
	strcat(prompt,buffer);
	ret=InputIntegerValuePI(-1,-1, IGUI_MIN, IGUI_MAX,*n1,prompt);
	if(ret == IGUI_INVALID)
		return 0;
	*n1=ret;
	return 1;
	}

// Reference
sprintf(buffer," %ld/%ld",*n1,*n2);
strcat(prompt,buffer);
	
for(;;)		{
	sprintf(buffer,"%ld/%ld",*n1,*n2);
	strcpy(oldbuffer,buffer);
	InputString(-1,-1,prompt, 127, buffer);
	if(!strcmp(buffer,oldbuffer))
		return 0;	// Cancelled

	p=strchr(buffer,'/');
	if(!p || strrchr(buffer,'/') != p)
		continue;	// Wrong, do it again

	// Okay, so we have exactly one /, min and max should be on either side of it
	*p=0;
	p++; // Start of second value

	firstword = hardfirst(buffer);
	secondword = hardfirst(p);

	if(!strisnumber(firstword))
		continue;

	if(!strisnumber(secondword))
		continue;

	v1=atol(firstword);
	v2=atol(secondword);
	if(v1>v2)
		continue; // Someone doesn't understand the idea of min and max
		
	// Okay, it's good.  Break the loop
	break;
	}

*n1=v1;
*n2=v2;
return 1;
}


void SetOwnerRecursively(OBJECT *obj, OBJECT *owner)
{
if(!obj) {
	return;
}
obj->stats->owner.objptr = owner;

if(!obj->pocket.objptr) {
	return;
}

for(obj=obj->pocket.objptr;obj;obj=obj->next) {
	obj->stats->owner.objptr = owner;
	if(obj->pocket.objptr) {
		SetOwnerRecursively(obj, owner);
	}
}

}
