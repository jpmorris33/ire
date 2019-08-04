/*
 *  - Movement Path menu tab
 */

#error "This file is not used at present, but might be in future"

#include <stdio.h>
#include "igui.hpp"
#include "../memory.hpp"
#include "../console.hpp"
#include "../core.hpp"
#include "../resource.hpp"
#include "../init.hpp"
#include "../linklist.hpp"
#include "menusys.h"

// DEFINES

// VARIABLES

static int sel_xoff=0;  // Offset from where we started the drag
static int sel_yoff=0;  // so that the dragging is centered on that spot
                        // i.e. we drag it by the point we first clicked on

extern int lx_proj;
extern int focus,PA_Id; // GUI variables
static int CurPath=0;

extern int MapXpos_Id,MapYpos_Id;       // Map X,Y coordinate boxes
extern char mapxstr[],mapystr[];

extern int mapx,mapy;   // Current map position
extern SPRITE transfer; // 256x256 Transfer buffer for screen clipping

PATH_NODE *pathsel;  // Path under scrutiny

static char ll_pos_str[]="            ";
static int listpos_Id;

static char mpt_str[]="                ";
static int mpt_Id;
static char beh_str[]="                ";
static int beh_Id;

// FUNCTIONS

extern void pause();
extern void Toolbar();
extern void DrawMap(int x,int y,char s_proj,char l_proj,char f_proj);
static void Nothing();
static void clipmap();  // Make sure coordinates are within bounds

void PA_up();           // Panning functions
void PA_down();
void PA_left();
void PA_right();

void PA_Create();
void PA_Edit();
void PA_Delete();

void PA_AddNode();
void PA_MoveNode();
void PA_DelNode();

void PA_Title();
void PA_Behave();

/*
void PA_1stNode();
void PA_NextNode();
void PA_PrevNode();
*/

static void SelectObj(int cx,int cy);  // Do the actual picking up

static void PA_Update();

static void GetMapX();                  // Get map position
static void GetMapY();

// Parts of the TL_Edit browser

void GetPAFromList( int x0, int y0, char *prompt, int listsize, char **list, char *name);

// CODE

/*
 * GoFocal - The menu tab function.  This is called from the toolbar
 */

void PA_GoFocal()
{
int temp;               // Used to get button handles

if(focus==6)            // Make sure we don't redraw if the user clicks again
	return;

focus=6;                // This is now the focus
Toolbar();              // Build up the menu system again from scratch
IG_SetFocus(PA_Id);     // And make the button stick inwards

// Set up Map window

IG_BlackPanel(VIEWX-1,VIEWY-1,258,258);
IG_Region(VIEWX,VIEWY,256,256,PA_AddNode,NULL,PA_MoveNode);

// Write sprite drawing status

temp = IG_ToggleButton(486,72,"Rooftops OFF",Nothing,NULL,NULL,&lx_proj);
IG_SetInText(temp,"Rooftops ON ");

// Set up the X and Y map position counter at the top

MapXpos_Id = IG_InputButton(24,40,mapxstr,GetMapX,NULL,NULL);
MapYpos_Id = IG_InputButton(112,40,mapystr,GetMapY,NULL,NULL);

PA_Update();

IG_TextButton(416,104,__up,PA_up,NULL,NULL);         // Pan up button
IG_TextButton(400,128,__left,PA_left,NULL,NULL);     // Pan left button
IG_TextButton(432,128,__right,PA_right,NULL,NULL);   // Pan right button
IG_TextButton(416,150,__down,PA_down,NULL,NULL);     // Pan down button

IG_AddKey(UP_KEY,PA_up);                             // Pan up key binding
IG_AddKey(DN_KEY,PA_down);                           // Pan down key binding
IG_AddKey(LF_KEY,PA_left);                           // Pan left key binding
IG_AddKey(RT_KEY,PA_right);                          // Pan right key binding

// Create objects

IG_TextButton(300,256,"Create Movement Path",PA_Create,NULL,NULL);

// Modify objects

IG_TextButton(300,288,"Choose Movement Path",PA_Edit,NULL,NULL);

// Delete objects

IG_TextButton(300,320,"Delete Movement Path",PA_Delete,NULL,NULL);


DrawScreenText(64,348,"Movement path title:");
mpt_Id = IG_InputButton(64,364,mpt_str,PA_Title,NULL,NULL);
DrawScreenText(64,396,"Behaviour afterwards:");
beh_Id = IG_InputButton(64,412,beh_str,PA_Behave,NULL,NULL);

// Delete last point

IG_TextButton(300,370,"Remove last Point",PA_DelNode,NULL,NULL);
IG_AddKey(DEL,PA_DelNode);
}

/*
 *   Nothing - Just draw the map.  Used by toggle-buttons
 */

void Nothing()
{
PA_Update();
return;
}

/*
 *   TL_up - Scroll upwards
 */

void PA_up()
{
mapy--;
clipmap();
PA_Update();
}

/*
 *   TL_down - Scroll downwards
 */

void PA_down()
{
mapy++;
clipmap();
PA_Update();
}

/*
 *   TL_left - Scroll to the left
 */

void PA_left()
{
mapx--;
clipmap();
PA_Update();
}

/*
 *   TL_right - Scroll to the right
 */

void PA_right()
{
mapx++;
clipmap();
PA_Update();
}

/*
 *   PA_AddNode - Add a single node to the current path
 */

void PA_AddNode()
{
int click_x,click_y;

if(!PAlist[CurPath])
    return;

// First do a quick bounds check, to prevent the creation of sprites
// where the user can't actually see.

click_x = ((x-VIEWX)>>5);           // Work out the position of the mouse
click_y = ((y-VIEWY)>>5);           // in tiles

if(click_x<0 || click_y<0)          // out the top/left of the screen?
    return;                         // Yes, abort
if(click_x>VSW-1 || click_y>VSH-1)  // Out the bottom/right of the screen?
    return;                         // Yes, abort

// Now do the full calculation.

click_x = ((x-VIEWX)>>5)+(mapx); // click_x = (mouse_x_pos/32) + map_x_pos
click_y = ((y-VIEWY)>>5)+(mapy); // click_y = (mouse_y_pos/32) + map_y_pos

for(PATH_NODE *t=PAlist[CurPath]->path;t;t=t->next)
    if(click_x == t->x)
        if(click_y == t->y)
            return;

pathsel = PL_Add(&PAlist[CurPath]->path);
if(!pathsel)
    {
    Notify(-1,-1,"Bugger",NULL);
    return;
    }
pathsel->x=click_x;
pathsel->y=click_y;

PA_Update();
}

/*
 *   PA_MoveNode - Move a single node around
 */

void PA_MoveNode()
{
int click_x,click_y;

if(!PAlist[CurPath])
    return;

if(!pathsel)
    return;

// First do a quick bounds check, to prevent the creation of sprites
// where the user can't actually see.

click_x = ((x-VIEWX)>>5);           // Work out the position of the mouse
click_y = ((y-VIEWY)>>5);           // in tiles

if(click_x<0 || click_y<0)          // out the top/left of the screen?
    return;                         // Yes, abort
if(click_x>VSW-1 || click_y>VSH-1)  // Out the bottom/right of the screen?
    return;                         // Yes, abort

// Now do the full calculation.

click_x = ((x-VIEWX)>>5)+(mapx); // click_x = (mouse_x_pos/32) + map_x_pos
click_y = ((y-VIEWY)>>5)+(mapy); // click_y = (mouse_y_pos/32) + map_y_pos

for(PATH_NODE *t=PAlist[CurPath]->path;t;t=t->next)
    if(click_x == t->x)
        if(click_y == t->y)
            return;

pathsel->x=click_x;
pathsel->y=click_y;

PA_Update();
}

/*
 *   PA_DelNode - Delete last node
 */

void PA_DelNode()
{
PATH_NODE *tt;
pathsel = NULL;

if(!PAlist[CurPath])
    return;

PL_Del(&PAlist[CurPath]->path);
for(tt=PAlist[CurPath]->path;tt;tt=tt->next) pathsel=tt;
PA_Update();
}

/*
 *   TL_Edit - Ask the user what an existing object should be like
 *             TL_SlowInsert calls this as well as the GUI.
 *             TL_Edit calls down to GetTLFromList to display the browser
 *-/

void TL_Edit()
{
char **list;
char Name[32];

// Make sure we have an object

if(!laysel)
	{
	Notify(-1,-1,"No sprite has been selected.","You must pick a sprite before you can modify it.");
	return;
	}

// Make sure it's not a silly object

if(laysel==curmap.roof)
	{
	Notify(-1,-1,"Not permitted.",NULL);
	return;
	}

// Allocate a list of possible characters

list=(char **)M_get(SQtot+1,sizeof(char *));

// Set it up, convert to uppercase for Wyber's ordered selection routine

for(int ctr=0;ctr<SQtot;ctr++)
        {
	list[ctr]=SQlist[ctr].name;
        strupr(list[ctr]);              // Affects the original too
        }

strcpy(Name,laysel->name);      // Set up the default string

qsort(list,SQtot,sizeof(char*),CMP);  // Sort them for the dialog box

// This is the graphical dialog box, taken from DEU.

GetTLFromList( -1,-1, "Choose a character:", SQtot-1, list, Name);

// If the user didn't press ESC instead of choosing, modify the character

if(Name[0])     // It's ok, do it
	{
        L_Init(laysel,Name);           // Re-evaluate the character
        strcpy(last_sprite,Name);       // Keep this name for reference
	}

free(list);                             // Dispose of the list
PA_Update();
}


/*
 *   TL_Delete - Remove an object
 *-/

void TL_Delete()
{
if(!laysel)
	{
	Notify(-1,-1,"No sprite has been selected.","You must pick a sprite before you can delete it.");
	return;
	}

if(laysel==curmap.roof)                         // This can't happen
	{
	Notify(-1,-1,"Not Permitted",NULL);     // Get shirty
	return;
	}

L_Free(laysel);                                // Destroy the object
laysel=NULL;                                    // Nothing currently selected
PA_Update();

sel_xoff = 0;                   // If we drag and drop, no grab offset
sel_yoff = 0;
}

/*
 *   TL_Pick - Drag and drop an object.  This calls down to SelectObj();
 *-/

void TL_Pick()
{
int click_x,click_y;

click_x = ((x-VIEWX)>>5)+(mapx); // click_x = (mouse_x_pos/32) + map_x_pos
click_y = ((y-VIEWY)>>5)+(mapy); // click_y = (mouse_y_pos/32) + map_y_pos

if(dragflag)                    // If we're dragging an object
	{
        if(!laysel)
            {
            Notify(-1,-1,"Internal error in drag'n'drop code",NULL);
            return;
            }

	laysel->x=click_x + sel_xoff;    // Get clicked position
	laysel->y=click_y + sel_yoff;    // And move object to that place.

	if(laysel->x<0) laysel->x=0;     // Sanity checks.
	if(laysel->y<0) laysel->y=0;

	dragflag=2;                      // Reset drag timer
	}
else                                     // If we're not dragging, or time out
	{
        SelectObj(click_x,click_y);      // Go fish

	if(laysel)                       // If an object was selected
            {
	    dragflag=2;                        // We go into drag mode
            strcpy(last_sprite,laysel->name);  // We'll make more of these
            }
	}

PA_Update();
}

/*
 *   TL_MoveThere - Move an object to the current square without dragging it
 *-/

void TL_MoveThere()
{
int click_x,click_y;

click_x = ((x-VIEWX)>>5)+(mapx); // click_x = (mouse_x_pos/32) + map_x_pos
click_y = ((y-VIEWY)>>5)+(mapy); // click_y = (mouse_y_pos/32) + map_y_pos

if(laysel)
    {
    laysel->x=click_x + sel_xoff;    // Get clicked position
    laysel->y=click_y + sel_yoff;    // And move object to that place.

    if(laysel->x<0) laysel->x=0;     // Sanity checks.
    if(laysel->y<0) laysel->y=0;

    PA_Update();
    }
}

/*
 *   TL_GotoFirst - Move to the first object in the list
 *-/

void TL_GotoFirst()
{
if(!curmap.roof->next)                 // No objects?  Can't do it
    return;

laysel=curmap.roof->next;              // Select the first one.

mapx=laysel->x-(VSW/2);                // center the object in the view
mapy=laysel->y-(VSH/2);

clipmap();                             // Clip the view to sensible bounds

PA_Update();
}

/*
 *   TL_GotoNext - Move to the next object in the list
 *-/

void TL_GotoNext()
{
if(!laysel)                            // Abort if no object selected
    return;

if(!laysel->next)                      // Abort if no object follows
    return;

laysel=laysel->next;                   // Go to the next one

mapx=laysel->x-(VSW/2);                // center the object in the view
mapy=laysel->y-(VSH/2);

clipmap();                             // Clip the view to sensible bounds

PA_Update();
pause();
}


/* =========================== Auxiliary functions ======================== */

/*
 *   SelectObj - fish for an object on the map where the user has clicked
 *-/

void SelectObj(int cx,int cy)
{

laysel = NULL;          // Assume nothing was there
sel_xoff = 0;           // No select offset
sel_yoff = 0;

for(LAYER *ttemp=curmap.roof;ttemp;ttemp=ttemp->next)
	if(cx>=ttemp->x && cx<ttemp->x+ttemp->mw)
                if(cy>=ttemp->y && cy<ttemp->y+ttemp->mh)
			{
			laysel=ttemp;
                        sel_xoff = (ttemp->x - cx);     // If we grab it by
                        sel_yoff = (ttemp->y - cy);     // the ear, record
                        }                               // where the ear was

// If laysel == NULL then we caught a fish

// Time to go
return;
}

/*
 *    PA_Update - Draw map and display nodes
 */

void PA_Update()
{
char temp[13];
int tot=1,pos;
int a,b,oa,ob;
PATH_NODE *tt;

DrawMap(mapx,mapy,1,lx_proj,0);

strcpy(mpt_str,"-               ");
strcpy(beh_str,"-               ");

if(!PAlist[CurPath])
    return;

strcpy(mpt_str,PAlist[CurPath]->name);
strcpy(beh_str,PAlist[CurPath]->func);

IG_UpdateText(mpt_Id,mpt_str);
IG_UpdateText(beh_Id,beh_str);

if(!PAlist[CurPath]->path)             // Abort, because no node was chosen
    return;

tot = 0;

// Plot the control points onscreen

for(tt=PAlist[CurPath]->path;tt;tt=tt->next,tot++)
    if(tt->x >= mapx && tt->x < (mapx+(VSW)))
        if(tt->y >= mapy && tt->y < (mapy+(VSH)))
            {
            a = ((tt->x-mapx)<<5)+VIEWX;
            b = ((tt->y-mapy)<<5)+VIEWY;
            box(a,b,a+31,b+31,RED,swapscreen);
            itoa(tot,temp,10);
            C_printxy(a,b,temp);

            if(pathsel == tt)
                box(a,b,a+31,b+31,WHITE,swapscreen);
            }

// Join the points

for(tt=PAlist[CurPath]->path;tt->next;tt=tt->next)
    {
    a = tt->x-mapx;
    b = tt->y-mapy;
    oa = tt->next->x-mapx;
    ob = tt->next->y-mapy;

    a=(a<<5)+16+VIEWX;
    b=(b<<5)+16+VIEWY;
    oa=(oa<<5)+16+VIEWX;
    ob=(ob<<5)+16+VIEWY;

    if(a<VIEWX)
        a=VIEWX;
    if(a>(VIEWX+256))
        a=VIEWX+256;
    if(oa<VIEWX)
        oa=VIEWX;
    if(oa>(VIEWX+256))
        oa=VIEWX+256;
    if(b<VIEWY)
        b=VIEWY;
    if(b>(VIEWY+256))
        b=VIEWY+256;
    if(ob<VIEWY)
        ob=VIEWY;
    if(ob>(VIEWY+256))
        ob=VIEWY+256;
    ITGline(a,b,oa,ob,WHITE,swapscreen);
    }

}

/*
 *   clipmap - clip the map's coordinates to sensible values
 */

void clipmap()
{
if(mapx<0) mapx=0;
if(mapx>curmap.w-VSW) mapx=curmap.w-VSW;
if(mapy>curmap.h-VSH) mapy=curmap.h-VSH;
if(mapy<0) mapy=0;
}


void PA_Create()
{
for(int ctr=0;ctr<MAX_PATHS;ctr++)
if(!PAlist[ctr])
    {
    CurPath=ctr;
    PAlist[ctr] = M_get(1,sizeof(PATHLIST));
    PAlist[ctr]->path=NULL;
    sprintf(PAlist[ctr]->name,"PATH_%d",ctr);
    InputString(-1,-1,"Enter a description of the path:",16,PAlist[ctr]->name);
    PA_Update();
    return;
    }
}

void PA_Edit()
{
char **list;
char Name[32];
int tot,cur,ctr;

tot=0;

for(ctr=0;ctr<MAX_PATHS;ctr++)
   if(PAlist[ctr])
       tot++;

if(!tot)
    {
    Notify(-1,-1,"No Movement Paths have been created yet.",NULL);
    return;
    }

list=(char **)M_get(tot,sizeof(char *));

// Set it up, convert to uppercase for Wyber's ordered selection routine

cur=0;
for(ctr=0;ctr<MAX_PATHS;ctr++)
        if(PAlist[ctr])
            {
            list[cur]=PAlist[ctr]->name;
            strupr(list[cur++]);              // Affects the original too
            }

if(PAlist[CurPath])
    strcpy(Name,PAlist[CurPath]->name);      // Set up the default string
else
    strcpy(Name,list[0]);

qsort(list,tot,sizeof(char*),CMP);  // Sort them for the dialog box

// This is the graphical dialog box, taken from DEU.

InputNameFromList( -1,-1, "Choose a Movement Path:", tot-1, list, Name);

// If the user didn't press ESC instead of choosing, modify the character

if(Name[0])     // It's ok, do it
    for(ctr=0;ctr<MAX_PATHS;ctr++)
        if(PAlist[ctr])
            if(!stricmp(PAlist[ctr]->name,Name))
                CurPath=ctr;

free(list);                             // Dispose of the list
PA_Update();
}


void PA_Delete()
{
if(!PAlist[CurPath])
    return;

if(!Notify(-1,-1,"Really delete this movement path?",NULL))
    return;

// Delete until deleted

for(;PL_Del(&PAlist[CurPath]->path););

M_free(PAlist[CurPath]);
PAlist[CurPath]=NULL;
}

void PA_Title()
{
if(!PAlist[CurPath])
    return;
InputString(-1,-1,"Enter a description of the path:",16,PAlist[CurPath]->name);
PA_Update();
}

void PA_Behave()
{
char **list;
char Name[32];

if(!PAlist[CurPath])
    return;

list=(char **)M_get(COtot+1,sizeof(char *));

// Set it up, convert to uppercase for Wyber's ordered selection routine

list[0]="-";
for(int ctr=0;ctr<COtot;ctr++)
        {
	list[ctr+1]=COlist[ctr].name;
        strupr(list[ctr]);              // Affects the original too
        }

strcpy(Name,PAlist[CurPath]->func);      // Set up the default string

qsort(list,COtot,sizeof(char*),CMP);  // Sort them for the dialog box

// This is the graphical dialog box, taken from DEU.

InputNameFromList( -1,-1, "Choose a function to call when finished:", COtot, list, Name);

// If the user didn't press ESC instead of choosing, modify the character

if(Name[0])     // It's ok, do it
        strcpy(PAlist[CurPath]->func,Name);

free(list);                             // Dispose of the list
PA_Update();
}

/*
 *   GetPAFromList - Show the list of paths
 */

void GetPAFromList( int x0, int y0, char *prompt, int listsize, char **list, char *name)
{
//InputNameFromListWithFunc( x0, y0, prompt, listsize, list, 5, name, 257, 257, TL_Thumb);
}


/*
 *    GetMapX - Ask the user where on the map they want to be
 */

void GetMapX()
{
mapx=InputIntegerValue(-1,-1,0,curmap.w-VSW,mapx);
DrawMap(mapx,mapy,1,lx_proj,0);
}

/*
 *    GetMapY - Ask the user where on the map they want to be
 */

void GetMapY()
{
mapy=InputIntegerValue(-1,-1,0,curmap.h-VSH,mapy);
DrawMap(mapx,mapy,1,lx_proj,0);
}

