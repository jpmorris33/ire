/*
 *  IGUI.CC - IT-HE GUI library prototype 1
 */

#include "igui.hpp"
#include "../ithelib.h"
#include "../core.hpp"
#include "../oscli.hpp"
#include "../media.hpp"
#include "menusys.h"
//#include <allegro.h>

#ifdef __DJGPP__
#define DEBUGIO stdout
#else
#define DEBUGIO stderr
#endif

#ifdef __BEOS__
#include <OS.h>
#endif

#if defined(__linux__) || defined(__DJGPP__)
#include <unistd.h>
#endif


// Defines
#define MAX_KEYBIND 128

// Variables

IRECOLOUR *ITG_LIGHTGRAY;
IRECOLOUR *ITG_LIGHTGREEN;
IRECOLOUR *ITG_DARKGRAY;
IRECOLOUR *ITG_YELLOW;
IRECOLOUR *ITG_RED;
IRECOLOUR *ITG_DARKRED;
IRECOLOUR *ITG_BLUE;
IRECOLOUR *ITG_BLACK;
IRECOLOUR *ITG_WHITE;

static int max_buttons;
static BUTTON *ButtonList;
static KEYBIND KeyList[MAX_KEYBIND];

//IREBITMAP *swapscreen;
char running,dragflag,didkill=0;
int x,y,b,z;        // Mouse X,Y and buttonlevel (z)

// Functions

void IG_Init(int max_button);
void IG_Term();
static int firstfree();

// Code

// Initialise the library

void IG_Init(int max_button)
    {
    if(max_buttons)
        return;
    InitGfx();
    swapscreen->ShowMouse();

    ITG_LIGHTGRAY=new IRECOLOUR(128,128,128);     //0 10000 10000 10000   lgray  4210
    ITG_DARKGRAY=new IRECOLOUR(96,96,96);         //0 01100 01100 01100   dgray  318c
    ITG_YELLOW=new IRECOLOUR(255,255,0);          //0 11111 11111 00000   yellow 7fe0
    ITG_RED=new IRECOLOUR(200,0,0);               //0 11001 00000 00000   red    6400
    ITG_LIGHTGREEN=new IRECOLOUR(0,200,0);        //0 00000 11001 00000   lgreen 0210

    ITG_BLUE=new IRECOLOUR(0,0,200);
    ITG_BLACK=new IRECOLOUR(0,0,0);
    ITG_WHITE=new IRECOLOUR(255,255,255);
    ITG_DARKRED=new IRECOLOUR(160,0,0);

	SetColor(ITG_BLACK);

    max_buttons = max_button;
    ButtonList = (BUTTON *)M_get(sizeof(BUTTON),max_buttons);
    running = 1;

    x = 100;
    y = 300;
    }

// Shut down the library

void IG_Term()
	{
	if(!max_buttons)
		return;

	TermGfx();

	M_free(ButtonList);
	running = 0;
	}

// Create a common-or-garden button

int IG_TextButton(int nx,int ny,char *text,FPTR l,FPTR m,FPTR r)
   {
    int ID;
    ID = firstfree();
    ButtonList[ID].textlen = strlen(text);
    ButtonList[ID].flag |= 1;

    ButtonList[ID].w=(ButtonList[ID].textlen+1)*8;
    ButtonList[ID].h=3*8;

    ButtonList[ID].x=nx;
    ButtonList[ID].y=ny;

    ButtonList[ID].x2=nx+ButtonList[ID].w;
    ButtonList[ID].y2=ny+ButtonList[ID].h;

    ButtonList[ID].state=ST_OUT;
    ButtonList[ID].type=NORMAL;

    ButtonList[ID].left_ptr=l;
    ButtonList[ID].right_ptr=r;
    ButtonList[ID].middle_ptr=m;

	IG_UpdateText(ID,text);
	IG_SetInText(ID,text);

    draw_button(ID);
    return ID;
}

// Create a button with no physical appearance.  Used for tile bars, maps etc

int IG_Region(int nx,int ny,int w,int h,FPTR l,FPTR m,FPTR r)
   {
    int ID;
    ID = firstfree();
    ButtonList[ID].flag |= 1;
    ButtonList[ID].flag |= 2;	// Regions should have repeat

    ButtonList[ID].w=w;
    ButtonList[ID].h=h;
    ButtonList[ID].x=nx;
    ButtonList[ID].y=ny;

    ButtonList[ID].x2=nx+ButtonList[ID].w;
    ButtonList[ID].y2=ny+ButtonList[ID].h;

    ButtonList[ID].state=0;
    ButtonList[ID].type=REGION;

    ButtonList[ID].left_ptr=l;
    ButtonList[ID].right_ptr=r;
    ButtonList[ID].middle_ptr=m;

    draw_button(ID);
    return ID;
}

// Create a button that looks like an input box

int IG_InputButton(int nx,int ny,char *text,FPTR l,FPTR m,FPTR r)
    {
    int ID;
    ID = firstfree();
    ButtonList[ID].textlen = strlen(text);
    ButtonList[ID].flag |= 1;

    ButtonList[ID].w=(ButtonList[ID].textlen+1)*8;
    ButtonList[ID].h=3*8;

    ButtonList[ID].x=nx;
    ButtonList[ID].y=ny;

    ButtonList[ID].x2=nx+ButtonList[ID].w;
    ButtonList[ID].y2=ny+ButtonList[ID].h;

    ButtonList[ID].state=ST_OUT;
    ButtonList[ID].type=SUNK;

    ButtonList[ID].left_ptr=l;
    ButtonList[ID].right_ptr=r;
    ButtonList[ID].middle_ptr=m;

	IG_UpdateText(ID,text);
	IG_SetInText(ID,text);

    draw_button(ID);
    return ID;
}

// Create a toggle button that reads and writes to an INT with the flag

int IG_ToggleButton(int nx,int ny,char *text,FPTR l,FPTR m,FPTR r,int *tog)
    {
    int ID;
    ID = firstfree();
    ButtonList[ID].textlen = strlen(text);
    ButtonList[ID].flag |= 1;

    ButtonList[ID].w=(ButtonList[ID].textlen+1)*8;
    ButtonList[ID].h=3*8;

    ButtonList[ID].x=nx;
    ButtonList[ID].y=ny;

    ButtonList[ID].x2=nx+ButtonList[ID].w;
    ButtonList[ID].y2=ny+ButtonList[ID].h;

    ButtonList[ID].state=0;
    ButtonList[ID].type=TOGGLE;

    ButtonList[ID].left_ptr=l;
    ButtonList[ID].right_ptr=r;
    ButtonList[ID].middle_ptr=m;
    ButtonList[ID].toggle = tog;

	IG_UpdateText(ID,text);
	IG_SetInText(ID,text);

    draw_button(ID);
    return ID;
}

// Create a menu tab

int IG_Tab(int nx,int ny,char *text,FPTR l,FPTR m,FPTR r)
{
int ID;
ID = firstfree();
ButtonList[ID].textlen = strlen(text);
ButtonList[ID].flag |= 1;

ButtonList[ID].w=(ButtonList[ID].textlen+1)*8;
ButtonList[ID].h=3*8;

ButtonList[ID].x=nx;
ButtonList[ID].y=ny;

ButtonList[ID].x2=nx+ButtonList[ID].w;
ButtonList[ID].y2=ny+ButtonList[ID].h;

ButtonList[ID].state=ST_3;
ButtonList[ID].type=RADIOBUTTON;

ButtonList[ID].left_ptr=l;
ButtonList[ID].right_ptr=r;
ButtonList[ID].middle_ptr=m;

IG_UpdateText(ID,text);
IG_SetInText(ID,text);

draw_button(ID);
return ID;
}

// (re)set text displayed when the button is out

void IG_UpdateText(int ID,char *text)
{
if(ID<0)
	return;

if(ButtonList[ID].textlen >= (int)sizeof(ButtonList[ID].text))
	ButtonList[ID].textlen = sizeof(ButtonList[ID].text)-1;

// Safely copy string
strncpy(ButtonList[ID].text,text,ButtonList[ID].textlen);
// Force terminator
ButtonList[ID].text[ButtonList[ID].textlen]=0;

draw_button(ID);
}

// Set text displayed when the button is sunk in

void IG_SetInText(int ID,char *text)
{
if(ID<0)
	return;

if(ButtonList[ID].textlen >= (int)sizeof(ButtonList[ID].text))
	ButtonList[ID].textlen = sizeof(ButtonList[ID].text)-1;

// Safely copy string
strncpy(ButtonList[ID].intext,text,ButtonList[ID].textlen);
// Force terminator
ButtonList[ID].intext[ButtonList[ID].textlen]=0;

draw_button(ID);
}

// Make this button repeat

void IG_SetRepeat(int ID)
{
if(ID<0)
	return;

ButtonList[ID].flag |= 2;
draw_button(ID);
}

// Sink a button in

void IG_SetFocus(int ID)
{
if(ID<0)
	return;

ButtonList[ID].state = ST_IN;
draw_button(ID);
}

// Pop a button out

void IG_ResetFocus(int ID)
{
if(ID<0)
	return;

ButtonList[ID].state = ST_OUT;
draw_button(ID);
}

// Draw a panel

void IG_Panel(int x,int y,int w,int h)
{
DrawScreenBox3D(x,y,x+w,y+h);
}

// Draw a hollow panel

void IG_BlackPanel(int x,int y,int w,int h)
{
DrawScreenBoxHollow(x,y,x+w,y+h);
}

// Kill all buttons and bindings

void IG_KillAll()
{
memset(&ButtonList[0],0,sizeof(BUTTON)*max_buttons);
memset(&KeyList[0],0,sizeof(KEYBIND)*MAX_KEYBIND);
didkill=1;
}

// Add a key to the key binding list

void IG_AddKey(int key,FPTR ptr)
{
int ctr;
for(ctr=0;ctr<MAX_KEYBIND;ctr++)
	if(!KeyList[ctr].key)
		{
		KeyList[ctr].key = key;
		KeyList[ctr].DoKey= ptr;
		return;
		}
}

// Draw some text on the screen

void IG_Text(int nx,int ny,char *text,IRECOLOUR *colour)
{
IRECOLOUR c(0,0,0), *d;

d=getcolor();
if(d)
	c.Set(d->packed);
SetColor(colour);
DrawScreenText(nx,ny,text);
SetColor(&c);
}



/*
 *      Dispatcher loop
 */

void IG_Dispatch()
{
int key_,ctrx,ctr;
static int x1=100,y1=300,b2;

b=0;

IRE_GetMouse(&x1,&y1,&z,&b);
x=x1-4;y=y1-4;

didkill=0;

if(IRE_KeyPressed()) {
	key_ = IRE_NextKey(NULL);
} else {
	key_ = 0;
}

if(z) {
	key_ = GetMouseZ(z);
}

if(key_) {
	for(ctrx=0;ctrx<MAX_KEYBIND;ctrx++) {
		if( key_ == KeyList[ctrx].key) {
			if(KeyList[ctrx].DoKey) {
				KeyList[ctrx].DoKey();
				b=0;
				break;
			}
		}
	}
}

Show();

if(b&&running) {
	for(ctr=0;ctr<max_buttons;ctr++) {
		if(x>=ButtonList[ctr].x&&x<=ButtonList[ctr].x2) {
			if(y>=ButtonList[ctr].y&&y<=ButtonList[ctr].y2) {
				b2=1;
				if(b==IREMOUSE_LEFT&&ButtonList[ctr].left_ptr==UNUSED) {
					b2=0;
				}
				if(b==IREMOUSE_RIGHT&&ButtonList[ctr].right_ptr==UNUSED) {
					b2=0;
				}
				if(b==IREMOUSE_MIDDLE&&ButtonList[ctr].middle_ptr==UNUSED) {
					b2=0;
				}
				if(b2) {
					if(ButtonList[ctr].type==TOGGLE) {
                        			*ButtonList[ctr].toggle = !(*ButtonList[ctr].toggle);
					} else {
						ButtonList[ctr].state=ST_IN;
					}
					draw_button(ctr);
					Show();

					if(!(ButtonList[ctr].flag & 2)) {
						IG_WaitForRelease();
					}

					if(b==IREMOUSE_LEFT) {
						ButtonList[ctr].left_ptr();
					}
					if(b==IREMOUSE_RIGHT) {
						ButtonList[ctr].right_ptr();
					}
					if(b==IREMOUSE_MIDDLE) {
						ButtonList[ctr].middle_ptr();
					}
					if(!didkill) {
						switch(ButtonList[ctr].type) {
							case NORMAL:
							case UNDEFINED:
								ButtonList[ctr].state=ST_OUT;
								break;
						}
						draw_button(ctr);
					}

//					x1 = mouse_x-4; y1 = mouse_y-4; z = mouse_b;
					IRE_GetMouse(&x1,&y1,&z,&b);
					x1-=4;
					y1-=4;
					break; // Only dispatch one event
				}
			}
		}
	}

	Show();
}
if(dragflag) {
	dragflag--;
}
}


// Find the first unused button

int firstfree()
{
int ctr;
for(ctr=0;ctr<max_buttons;ctr++)
	if(!(ButtonList[ctr].flag & 1))
		return ctr;
return -1;
}

// Draw each type of button

void draw_button(int no)
{
if(no<0)
	return;

switch(ButtonList[no].type)
	{
	case NORMAL:
	case RADIOBUTTON:
	if(ButtonList[no].state==ST_OUT)
		{
		SetColor(ITG_BLACK);
		DrawScreenBox3D(ButtonList[no].x,ButtonList[no].y,ButtonList[no].x2,ButtonList[no].y2);
		DrawScreenText(ButtonList[no].x+4,ButtonList[no].y+8,ButtonList[no].text);
		}

	if(ButtonList[no].state==ST_IN)
		{
		SetColor(ITG_BLACK);
		DrawSunkBox3D(ButtonList[no].x,ButtonList[no].y,ButtonList[no].x2,ButtonList[no].y2);
		DrawScreenText(ButtonList[no].x+6,ButtonList[no].y+10,ButtonList[no].intext);
		}

	if(ButtonList[no].state==ST_3)
		{
		SetColor(ITG_BLACK);
		DrawScreenBox3D(ButtonList[no].x,ButtonList[no].y,ButtonList[no].x2,ButtonList[no].y2);
		DrawScreenText(ButtonList[no].x+4,ButtonList[no].y+8,ButtonList[no].text);
		}
	break;

	case TOGGLE:
	DrawScreenBoxHollow(ButtonList[no].x,ButtonList[no].y,ButtonList[no].x2,ButtonList[no].y2);
	SetColor(ITG_WHITE);

	if(!ButtonList[no].toggle)
		ithe_panic("draw_button: Toggle Button has a NULL toggle pointer!",ButtonList[no].text);

	if(*ButtonList[no].toggle)
		DrawScreenText(ButtonList[no].x+4,ButtonList[no].y+8,ButtonList[no].intext);
	else
		DrawScreenText(ButtonList[no].x+4,ButtonList[no].y+8,ButtonList[no].text);
	break;

	case SUNK:
	DrawScreenBoxHollow(ButtonList[no].x,ButtonList[no].y,ButtonList[no].x2,ButtonList[no].y2);

	// Is the button enabled?
	if(ButtonList[no].left_ptr || ButtonList[no].middle_ptr || ButtonList[no].right_ptr)
		SetColor(ITG_WHITE);
	else
		SetColor(ITG_LIGHTGRAY);
	DrawScreenText(ButtonList[no].x+4,ButtonList[no].y+8,ButtonList[no].text);
	break;

	case REGION:
	break;
	}
SetColor(ITG_WHITE);
}


int CMP(const void *a,const void *b)
{
return strcmp(*((char **)a),*((char **)b));
}

void waitabit()
{
IRE_WaitFor(50);
}

void IG_WaitForRelease()
{
int x,y,b=1,timeout;
timeout=254;
while(b && timeout>0) {
	IRE_GetMouse(&x,&y,NULL,&b);
	x-=4;y-=4;
//    x = mouse_x-4; y = mouse_y-4; z = mouse_b;
	waitabit();
	timeout--;
}

if(timeout < 1) {
	fprintf(DEBUGIO,"IRE GUI: Timed out waiting for mouse to respond (gui/igui.cpp line %d)\n",__LINE__);
	fprintf(DEBUGIO,"Mouse says %d\n",b);
}
}
