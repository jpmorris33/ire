//
//      Mouse and button support in the game
//

#include <string.h>
#include "media.hpp"
#include "core.hpp"
#include "console.hpp"
#include "mouse.hpp"
#include "init.hpp"
#include "gamedata.hpp"

// Defines

// Variables

MOUSERANGE MouseRange[MAX_MBUTTONS]; // Mouse 'buttons'
static MOUSERANGE MouseBackup[MAX_MBUTTONS]; // Backup copy of the ranges
int MouseOver,MouseRanges,MouseBackups;
long MouseID=-1,MouseGridX=-1,MouseGridY=-1;
long MouseMapX=-1,MouseMapY=-1;

static void DrawMouseRect(IREBITMAP *dest, int x1,int y1, int x2, int y2, IRECOLOUR *col);
static void CheckMouseRange(int mx, int my, int mousebuttonflag);


// Funcs

/*
 *  Mouse Support
 */


void WaitForMouseRelease()
{
int b;
do
	{
//	S_PollMusic();
	if(!IRE_GetMouse(&b)) // No mouse
		return;
	} while(b);
return;
}

// Reset button system

void ClearMouseRanges()
{
MouseRanges=0;
MouseOver=0;
}

// MouseOver?

void AddMouseRange(int ID, int x, int y, int w, int h)
{
if(MouseRanges>=MAX_MBUTTONS)
	return;
if(MouseRanges<0)
	MouseRanges=0;

MouseRange[MouseRanges].left = x;
MouseRange[MouseRanges].top = y;
MouseRange[MouseRanges].right = x+w;
MouseRange[MouseRanges].bottom= y+h;
MouseRange[MouseRanges].id = ID;
MouseRange[MouseRanges].flags = RANGE_LBUTTON; // Assume left button
MouseRange[MouseRanges].pointer= 0; // No mouse over
MouseRange[MouseRanges].normal = NULL;
MouseRange[MouseRanges].in = NULL;
MouseRanges++;
}

// Is it a grid?

void SetRangeGrid(int ID, int w, int h)
{
int ctr;
for(ctr=0;ctr<MouseRanges;ctr++)
	if(MouseRange[ctr].id == ID)
		{
		MouseRange[ctr].flags |= RANGE_GRID;
		// Safety checks (these will be divisors later on)
		if(w<1)
			w=1;
		if(h<1)
			h=1;
		MouseRange[ctr].gw = w;
		MouseRange[ctr].gh = h;
		return;
		}
}

// Is it a button?

void SetRangeButton(int ID, char *normal, char *in)
{
int ctr,n,i;

n=getnum4sprite(normal);
i=getnum4sprite(in);

// If we can't find the normal button, scream
if(n == -1)
	{
	Bug("Set button %d: Can't find sprite %s\n",ID,normal);
	return;
	}

// If the 'in' button can't be found, use the default
if(i == -1)
	i=n;

for(ctr=0;ctr<MouseRanges;ctr++)
	if(MouseRange[ctr].id == ID)
		{
		MouseRange[ctr].flags |= RANGE_BUTTON;
		MouseRange[ctr].normal = SPlist[n].image;
		MouseRange[ctr].in = SPlist[i].image;
		MouseRange[ctr].right = MouseRange[ctr].left+SPlist[n].w;
		MouseRange[ctr].bottom= MouseRange[ctr].top+SPlist[n].h;
		return;
		}
}

// Make it right-clickable

void RightClick(int ID)
{
int ctr;

for(ctr=0;ctr<MouseRanges;ctr++)
	if(MouseRange[ctr].id == ID)
		{
		MouseRange[ctr].flags &= ~RANGE_LBUTTON;
		MouseRange[ctr].flags |= RANGE_RBUTTON;
		return;
		}
}


// MouseOver?

void SetRangePointer(int ID, int pointerno)
{
int ctr;
for(ctr=0;ctr<MouseRanges;ctr++)
	if(MouseRange[ctr].id == ID)
		{
		MouseRange[ctr].pointer = pointerno;
		MouseOver=1; // We now have a MouseOver event
		return;
		}
}

// Check mouse range

void CheckMouseRanges()
{
int mx,my,mb,ctr;
static IRECURSOR *mptr=NULL,*optr=NULL;

MouseID=-1;

//ilog_printf("Check Mouse Range\n");

// Grab the mouse coordinates (they are volatile may change)
if(!IRE_GetMouse(&mx,&my,NULL,&mb))
	return; // No mouse

// Nothing selected
MouseID=-1;
MouseGridX=-1;
MouseGridY=-1;

DrawButtons();

mptr=NULL; // Assume default pointer

// Do we need to check all the ranges for a mouse-over event?

if(MouseOver && MouseOverPtr) {
	for(ctr=0;ctr<MouseRanges;ctr++) {
		if(MouseRange[ctr].pointer) {
			if(mx > MouseRange[ctr].left && mx < MouseRange[ctr].right) {
				if(my > MouseRange[ctr].top && my < MouseRange[ctr].bottom) {
					mptr=MouseOverPtr;
				}
			}
		}
	}
}

// Check left button click
if(mb & IREMOUSE_LEFT) {
	CheckMouseRange(mx, my, RANGE_LBUTTON);
}

if(mb & IREMOUSE_RIGHT) {
	CheckMouseRange(mx, my, RANGE_RBUTTON);
}

// Choose new pointer if it has changed
if(mptr != optr) {
	if(mptr) {
		mptr->Set();
	} else {
		DefaultCursor->Set();
	}
optr=mptr;
}

}

void CheckMouseRange(int mx, int my, int mousebuttonflag) {
	int a;
	for(int ctr=0;ctr<MouseRanges;ctr++) {
		if(MouseRange[ctr].flags & mousebuttonflag) {
			// printf("Check mouse range %d,%d vs %d,%d-%d,%d\n", mx, my, MouseRange[ctr].left, MouseRange[ctr].top, MouseRange[ctr].right, MouseRange[ctr].bottom);
			if(mx > MouseRange[ctr].left && mx < MouseRange[ctr].right) {
				if(my > MouseRange[ctr].top && my < MouseRange[ctr].bottom) {
					// If it is a grid, calculate selected cell
					if(MouseRange[ctr].flags & RANGE_GRID) {
						a=mx-MouseRange[ctr].left; // Get grid offset
						MouseGridX=a/MouseRange[ctr].gw;
						a=my-MouseRange[ctr].top; // Get grid offset
						MouseGridY=a/MouseRange[ctr].gh;
						MouseMapX=MouseGridX+mapx;
						a=my-MouseRange[ctr].top; // Get grid offset
						MouseMapY=MouseGridY+mapy;
					}
					// return the corresponding ID
					MouseID=MouseRange[ctr].id;
					// printf("Return mouseId %d\n", MouseID);
					return;
				}
			}
		}
	}

}

void DrawButtons()
{
int ctr;

for(ctr=0;ctr<MouseRanges;ctr++)
	{
	if(MouseRange[ctr].flags & RANGE_BUTTON)
		{
		if(MouseRange[ctr].flags & RANGE_BUTTONIN)
			MouseRange[ctr].in->Draw(swapscreen,MouseRange[ctr].left,MouseRange[ctr].top);
		else
			MouseRange[ctr].normal->Draw(swapscreen,MouseRange[ctr].left,MouseRange[ctr].top);
		}
	}
}

// For debugging only

void DrawRanges()
{
int ctr;
IRECOLOUR red(255,0,0);

for(ctr=0;ctr<MouseRanges;ctr++)
	DrawMouseRect(swapscreen,MouseRange[ctr].left,MouseRange[ctr].top,MouseRange[ctr].right,MouseRange[ctr].bottom,&red);
}

void DrawMouseRect(IREBITMAP *dest, int x1,int y1, int x2, int y2, IRECOLOUR *col)
{
dest->Line(x1,y1,x2,y1,col);
dest->Line(x1,y1,x1,y2,col);
dest->Line(x2,y1,x2,y2,col);
dest->Line(x1,y2,x2,y2,col);
}

// Is it a grid?

void PushButton(int ID, int state)
{
int ctr;
for(ctr=0;ctr<MouseRanges;ctr++)
	if(MouseRange[ctr].id == ID)
		{
		if(!(MouseRange[ctr].flags & RANGE_BUTTON))
			return;
		if(state)
			MouseRange[ctr].flags |= RANGE_BUTTONIN;
		else
			MouseRange[ctr].flags &= ~RANGE_BUTTONIN;
		return;
		}
}

/*
 *  Save Ranges
 */

void SaveRanges()
{
memcpy(MouseBackup,MouseRange,sizeof(MouseRange));
MouseBackups=MouseRanges;
}

/*
 *  Save Ranges
 */

void RestoreRanges()
{
memcpy(MouseRange,MouseBackup,sizeof(MouseRange));
MouseRanges=MouseBackups;
}

