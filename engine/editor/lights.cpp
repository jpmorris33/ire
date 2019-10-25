
/*
 *  - Static Lighting menu tab
 */


#include <stdio.h>
#include "igui.hpp"
#include "../ithelib.h"
#include "../console.hpp"
#include "../core.hpp"
#include "../gamedata.hpp"
#include "menusys.h"

// defines

// variables

extern int focus,LT_Id,lightdepth;
extern int MapXpos_Id,MapYpos_Id;       // Map X/Y Coordinate boxes
extern char mapxstr[],mapystr[];
extern void (*EdUpdateFunc)(void);

extern long M_core;     // Total amount of core, from memory.cc
extern int s_proj;      // Flags, are sprites/roof?
extern int preview_light;

extern long mapx,mapy;   // Current map position
extern IREBITMAP *lighticon;

static int curoff=0;    // Current position on the tile list at the bottom
static int L=0,R=0;     // Current tile for each button, Lclick and Rclick
int L_lighttile=0;
static int Xl00_Id,Xl50_Id,Xl99_Id,XlPr_Id;

// functions

extern void Toolbar();
extern void DrawMap(int x,int y,char s_proj,char l_proj,char f_proj);
extern void WriteLight(int x,int y,int tile);
extern int ReadLight(int x,int y);
extern int GetClickXY(int mx, int my, int *xout, int *yout);


static void Nothing();
static void clipmap();
static int Get_Light();

void LTBl();
void LTBll();
void LTBr();
void LTBrr();
void LT_bar();

void LT_up();
void LT_down();
void LT_left();
void LT_right();

static void GetL();                     // Get a tile for the left click
static void GetR();                     // Get a tile for the right click
static void SetR();                     // Write a right-click tile to map
static void SetL();                     // Write a left-click tile to map

static void Get_Prefab();               // Copy to clipboard
static void Put_Prefab();               // Copy to map

static void ClearL();
static void ClearR();

// Xlucency control

static void XL_00();
static void XL_50();
static void XL_99();
static void XL_Preview();
static void XL_update();

static void Set_Light(int tile);   // Write a tile to the map, helper

static void GetMapX();                  // Get map position
static void GetMapY();

static unsigned short prefab[8][8];

//static unsigned short blkundo[8][8];    // Undo block op

// GoFocal - The menu tab function.  This is called from the toolbar

void LT_GoFocal()
{
int temp;               // Used to get button handles

if(focus==5)            // Make sure we don't redraw if the user clicks again
	return;

focus=5;                // This is now the focus
Toolbar();              // Build up the menu system again from scratch
IG_SetFocus(LT_Id);     // And make the button stick inwards

// Set up Map window

IG_BlackPanel(VIEWX-1,VIEWY-1,258,258);
IG_Region(VIEWX,VIEWY,256,256,SetL,NULL,SetR);

// Write sprite drawing status

temp = IG_ToggleButton(486,72,"Sprites OFF",Nothing,NULL,NULL,&s_proj);
IG_SetInText(temp,"Sprites ON ");

// Add transparency control

Xl00_Id = IG_Tab(486,104,"Off",XL_00,NULL,NULL);
Xl50_Id = IG_Tab(526,104,"1/2",XL_50,NULL,NULL);
Xl99_Id = IG_Tab(566,104,"On ",XL_99,NULL,NULL);
XlPr_Id = IG_Tab(486,136,"Preview Light",XL_Preview,NULL,NULL);

// Draw the VCR buttons on the Tile selector

IG_TextButton(8,410,__left,LTBl,NULL,NULL);     // Normal speed
IG_TextButton(616,410,__right,LTBr,NULL,NULL);

IG_TextButton(8,440,__left2,LTBll,NULL,NULL);   // Fast Forward
IG_TextButton(608,440,__right2,LTBrr,NULL,NULL);

// Draw the contents of Left and Right buttons

IG_BlackPanel(400,192,32,32);
DrawScreenText(400,182," L");
if(L)
    {
    LTlist[L].image->RenderRaw(lighticon);
    lighticon->Draw(swapscreen,400,192);
    }
else
     fbox2(400,192,32,32,0,swapscreen);

IG_BlackPanel(448,192,32,32);
DrawScreenText(448,182," R");
if(R)
    {
    LTlist[R].image->RenderRaw(lighticon);
    lighticon->Draw(swapscreen,448,192);
    }
else
    fbox2(448,192,32,32,0,swapscreen);

LT_bar();                                       // Draw the Tile bar
IG_Region(32,400,576,32,GetL,NULL,GetR);        // Add clickable region

// Set up the X and Y map position counter at the top

MapXpos_Id = IG_InputButton(24,40,mapxstr,GetMapX,NULL,NULL);
MapYpos_Id = IG_InputButton(112,40,mapystr,GetMapY,NULL,NULL);

IG_TextButton(300,300,"Cut screen",Get_Prefab,NULL,NULL);   // Fast Forward
IG_TextButton(400,300,"Paste screen",Put_Prefab,NULL,NULL);   // Fast Forward

IG_TextButton(300,348,"Fill L tile",ClearL,NULL,NULL);   // Fast Forward
IG_TextButton(400,348,"Fill R tile",ClearR,NULL,NULL);   // Fast Forward

XL_update();

DrawMap(mapx,mapy,s_proj,0,1);             // Draw the map

IG_TextButton(416,104,__up,LT_up,NULL,NULL);         // Pan up button
IG_TextButton(400,128,__left,LT_left,NULL,NULL);     // Pan left button
IG_TextButton(432,128,__right,LT_right,NULL,NULL);   // Pan right button
IG_TextButton(416,150,__down,LT_down,NULL,NULL);     // Pan down button

IG_AddKey(IREKEY_UP,LT_up);                             // Pan up key binding
IG_AddKey(IREKEY_DOWN,LT_down);                         // Pan down key binding
IG_AddKey(IREKEY_LEFT,LT_left);                         // Pan left key binding
IG_AddKey(IREKEY_RIGHT,LT_right);                       // Pan right key binding

IG_AddKey(IREKEY_C,Get_Prefab);                         // get prefab key binding
IG_AddKey(IREKEY_V,Put_Prefab);                         // put prefab key binding

EdUpdateFunc=NULL;

}

void Nothing()
{
DrawMap(mapx,mapy,s_proj,0,1);
return;
}

void LTBr()
{
curoff++;
if(curoff>(LTtot-17))
    curoff=(LTtot-17);
if(curoff<0) curoff=0;
LT_bar();
}

void LTBl()
{
curoff--;
if(curoff<0) curoff=0;
LT_bar();
}

void LTBrr()
{
curoff+=10;
if(curoff>(LTtot-17))
    curoff=(LTtot-17);
if(curoff<0) curoff=0;
LT_bar();
}

void LTBll()
{
curoff-=10;
if(curoff<0) curoff=0;
LT_bar();
}


void LT_bar()
{
int co,ctr;
co=17;
if(LTtot<17)
    co=LTtot;
IG_BlackPanel(31,399,578,34);

for(ctr=0;ctr<co;ctr++)
   if(ctr+curoff < 1)
        fbox2(33,401,32,32,0,swapscreen);
   else
	{
	LTlist[ctr+curoff].image->RenderRaw(lighticon);
	lighticon->Draw(swapscreen,33+(34*ctr),401);
	}
}

void GetL()
{
int co;
co=17;
if(LTtot<17) co=LTtot;
for(int ctr=0;ctr<co;ctr++)
	if(x>33+(34*ctr) && x<65+(34*ctr))
		{
		L=ctr+curoff;
		L_lighttile = L; // For big-map painting
                if(!L)
                    fbox2(400,192,32,32,0,swapscreen);
                else
			{
			LTlist[L].image->RenderRaw(lighticon);
			lighticon->Draw(swapscreen,400,192);
			}
		return;
		}
}

void GetR()
{
int co;
co=17;
if(LTtot<17)
     co=LTtot;
for(int ctr=0;ctr<co;ctr++)
	if(x>33+(34*ctr) && x<65+(34*ctr))
		{
		R=ctr+curoff;
                if(!R)
                    fbox2(448,192,32,32,0,swapscreen);
                else
			{
			LTlist[R].image->RenderRaw(lighticon);
			lighticon->Draw(swapscreen,448,192);
			}
		return;
		}
}

void SetL()
{
int l;

// If shift is pressed, grab tile instead of drawing
if(IRE_TestShift(IRESHIFT_SHIFT)) {
	l=Get_Light();
	if(l>=LTtot) {
		l=LTtot-1;
	}
	if(l != -1) {
		L=l;
	}
	IG_BlackPanel(400,192,32,32);
	if(L) {
		LTlist[L].image->RenderRaw(lighticon);
		lighticon->Draw(swapscreen,400,192);
	}
	return;
}

Set_Light(L);
}

void SetR()
{
int r;

// If shift is pressed, grab tile instead of drawing
if(IRE_TestShift(IRESHIFT_SHIFT)) {
	r=Get_Light();
	if(r>=LTtot) {
		r=LTtot-1;
	}
	if(r != -1) {
		R=r;
	}
	IG_BlackPanel(448,192,32,32);
	if(R) {
		LTlist[R].image->RenderRaw(lighticon);
		lighticon->Draw(swapscreen,448,192);
	}
	return;
}

Set_Light(R);
}

void Set_Light(int tile)
{
int t,xx,yy,yyy,xxx;

yyy=-1;
xxx=-1;
for(xx=0;xx<VSW;xx++)
	{
	t=32*xx+(VIEWX-1);
	if(x>=t &&x<(t+32))
		xxx=xx;
	}

for(yy=0;yy<VSH;yy++)
	{
	t=32*yy+(VIEWY-1);
	if(y>=t && y<(t+32))
		yyy=yy;
	}

if(xxx!=-1 && yyy!=-1)
	WriteLight(mapx+xxx,mapy+yyy,tile);
DrawMap(mapx,mapy,s_proj,0,1);
}

// Xlucency

void XL_00()
{
lightdepth=0;
preview_light=0;
XL_update();
}

void XL_50()
{
lightdepth=128;
preview_light=0;
XL_update();
}

void XL_99()
{
lightdepth=255;
preview_light=0;
XL_update();
}

void XL_Preview()
{
lightdepth=0;
preview_light=1;
XL_update();
}

void XL_update()
{
IG_ResetFocus(Xl00_Id); // Pull all the buttons out by default
IG_ResetFocus(Xl50_Id);
IG_ResetFocus(Xl99_Id);
IG_ResetFocus(XlPr_Id);

// Now find the right button to push in

if(preview_light)
	{
	IG_SetFocus(XlPr_Id);
	DrawMap(mapx,mapy,s_proj,0,1);
	return;
	}

switch(lightdepth)
    {
    case 0:
    IG_SetFocus(Xl00_Id);
    break;

    case 128:
    IG_SetFocus(Xl50_Id);
    break;

    case 255:
    IG_SetFocus(Xl99_Id);
    break;

	default:
    IG_SetFocus(Xl50_Id);
	lightdepth=128;
    break;
    };
DrawMap(mapx,mapy,s_proj,0,1);
}

void LT_up() {
if(IRE_TestShift(IRESHIFT_SHIFT))
        mapy-=8;
else
	mapy--;
clipmap();
DrawMap(mapx,mapy,s_proj,0,1);
}

void LT_down() {
if(IRE_TestShift(IRESHIFT_SHIFT))
        mapy+=8;
else
	mapy++;
clipmap();
DrawMap(mapx,mapy,s_proj,0,1);
}


void LT_left() {
if(IRE_TestShift(IRESHIFT_SHIFT))
        mapx-=8;
else
	mapx--;

clipmap();
DrawMap(mapx,mapy,s_proj,0,1);
}

void LT_right() {
if(IRE_TestShift(IRESHIFT_SHIFT))
        mapx+=8;
else
	mapx++;
clipmap();
DrawMap(mapx,mapy,s_proj,0,1);
}

void ClearL()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                WriteLight(mapx+cx,mapy+cy,L);
DrawMap(mapx,mapy,s_proj,0,1);
}

void ClearR()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                WriteLight(mapx+cx,mapy+cy,R);
DrawMap(mapx,mapy,s_proj,0,1);
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
 *    GetMapX - Ask the user where on the map they want to be
 */

void GetMapX()
{
mapx=InputIntegerValue(-1,-1,0,curmap->w-VSW,mapx);
DrawMap(mapx,mapy,s_proj,0,1);
}

/*
 *    GetMapY - Ask the user where on the map they want to be
 */

void GetMapY()
{
mapy=InputIntegerValue(-1,-1,0,curmap->h-VSH,mapy);
DrawMap(mapx,mapy,s_proj,0,1);
}


void Get_Prefab()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                prefab[cx][cy]=ReadLight(mapx+cx,mapy+cy);
//DrawMap(mapx,mapy,s_proj,l_proj,0);
}

void Put_Prefab()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                WriteLight(mapx+cx,mapy+cy,prefab[cx][cy]);
DrawMap(mapx,mapy,s_proj,0,1);
}


int Get_Light()
{
int xpos,ypos;
if(GetClickXY(x,y,&xpos,&ypos)) {
	return ReadLight(xpos,ypos);
}
return -1;
}



