/*
 *  - Large view of the world
 */

#include <stdio.h>
#include "igui.hpp"
#include "../ithelib.h"
#include "../console.hpp"
#include "../core.hpp"
#include "../gamedata.hpp"
#include "../object.hpp"
#include "../oscli.hpp"
#include "../init.hpp"
#include "menusys.h"

// defines

//#define PATHBUILDER
#define MAX_PATH_SQUARES 4096
#define MicroX 512
#define MicroY 288

// variables

static int pmx=0,pmy=0,path1x=0,path1y=0;
static int BrushMode=0,ShowSolid=0,ShowEggs=0,ShowRoof=0;
static int CN_Id,CS_Id,CE_Id,CW_Id,BR_Id;
static char ConnNstr[]="None  ";
static char ConnSstr[]="None  ";
static char ConnWstr[]="None  ";
static char ConnEstr[]="None  ";

static char *Brushes[]=	{
						"-             ",
						"BRUSH  1X1    ",
						"BRUSH  2X2    ",
						"BRUSH  4X4    ",
						"BRUSH  8X8    ",
						"BRUSH 16X16   ",
						"BRUSH 32X32   ",
						"ROOF  8X8     ",
						"ROOF 32X32    ",
						"SPRAYCAN      ",
						NULL,
						};

static int BrushType[]={0,1,2,4,8,16,32,-8,-32,-1};	// MUST MATCH Brushes[]

extern int focus,BM_Id;
extern int L_bgtile,L_rooftile;
//static int PathMode=0;

extern long M_core;     // Total amount of core, from memory.cc
extern int s_proj;      // Flag, are sprites being displayed?
extern int l_proj;      // Flag, are layers being displayed?

// functions

extern void WriteRoof(int x,int y,int tile);
extern int ReadRoof(int x,int y);
extern int ReadMap(int x,int y);
extern void WriteMap(int x,int y, int tile);
extern void Toolbar();
extern void (*EdUpdateFunc)(void);
extern void DrawMap(int x,int y,char s_proj,char l_proj,char f_proj);

static void OneToOne();
static void PanMap();
static void PanUp();
static void PanDown();
static void PanLeft();
static void PanRight();
static void SetL();
static void Spray();
static void SetRoofL(int WIDF);
static void GoMap();
static void GetBrush();
static void Nothing();

static void Connections();
static void Connections_Update();
static void ConnN();
static void ConnS();
static void ConnE();
static void ConnW();
static void Finish();
#ifdef PATHBUILDER
static void PathMaker();
#endif
extern void PathMe();

// GoFocal - The menu tab function.  This is called from the toolbar

void BM_GoFocal()
{
int temp;               // Used to get button handles

if(focus==4)            // Make sure we don't redraw if the user clicks again
	return;

focus=4;                // This is now the focus
Toolbar();              // Build up the menu system again from scratch
IG_SetFocus(BM_Id);     // And make the button stick inwards

// Add Panning buttons

IG_TextButton(516,104,__up,PanUp,NULL,NULL);         // Pan up button
IG_TextButton(500,128,__left,PanLeft,NULL,NULL);     // Pan left button
IG_TextButton(532,128,__right,PanRight,NULL,NULL);   // Pan right button
IG_TextButton(516,150,__down,PanDown,NULL,NULL);     // Pan down button

BR_Id = IG_InputButton(472,256,Brushes[BrushMode],GetBrush,NULL,NULL);
IG_SetInText(BR_Id,Brushes[BrushMode]);

temp = IG_ToggleButton(472,192,"Normal View  ",Nothing,NULL,NULL,&ShowSolid);
IG_SetInText(temp,"Solid Objects");

temp = IG_ToggleButton(472,224,"Eggs off",Nothing,NULL,NULL,&ShowEggs);
IG_SetInText(temp,"Eggs on ");

temp = IG_ToggleButton(556,224,"Roof off",Nothing,NULL,NULL,&ShowRoof);
IG_SetInText(temp,"Roof on ");

// Set up Map windows

IG_BlackPanel(MicroX-2,MicroY-2,68,68);

if(curmap->w>=384 || curmap->h>=384)
    {
    IG_BlackPanel(62,62,386,386);
    PanMap();
    IG_AddKey(IREKEY_UP,PanUp);
    IG_AddKey(IREKEY_DOWN,PanDown);
    IG_AddKey(IREKEY_LEFT,PanLeft);
    IG_AddKey(IREKEY_RIGHT,PanRight);
    }
else
    OneToOne();

EdUpdateFunc=NULL;

IG_Region(64,64,384,384,SetL,NULL,GoMap);

IG_TextButton(496,384,"Connections",Connections,NULL,NULL);

#ifdef PATHBUILDER
temp = IG_TextButton(496,416,"PathMaker",PathMaker,NULL,NULL);
#endif

}

void OneToOne()
{
int temp,cx,cy,div,rx,ry,blockx,blocky,blockw,blockh;
OBJECT *ob;
IG_BlackPanel(62,62,386,386);

for(cy=0;cy<curmap->h;cy++)
	for(cx=0;cx<curmap->w;cx++)
		{
		temp=ReadMap(cx,cy);
		if(temp==RANDOM_TILE)
			swapscreen->PutPixel(cx+64,cy+64,ITG_BLUE);
		else
			swapscreen->PutPixel(cx+64,cy+64,TIlist[temp].form->seq[0]->thumbcol);
		}

if(ShowSolid || ShowEggs || ShowRoof)
	for(cy=0;cy<curmap->h;cy++)
		for(cx=0;cx<curmap->w;cx++)
			{
			ob=GetRawObjectBase(cx,cy);
			for(;ob;ob=ob->next)
				{
				if(ShowSolid && ob->flags&IS_SOLID)
					{
					if(ob->flags & IS_LARGE)
						{
						if(ob->curdir > CHAR_D) //(U,D,L,R) > D = L,R
							{
							blockx=ob->hblock[BLK_X];
							blocky=ob->hblock[BLK_Y];
							blockw=ob->hblock[BLK_W];
							blockh=ob->hblock[BLK_H];
							}
						else
							{
							blockx=ob->vblock[BLK_X];
							blocky=ob->vblock[BLK_Y];
							blockw=ob->vblock[BLK_W];
							blockh=ob->vblock[BLK_H];
							}
						swapscreen->FillRect(cx+64+blockx,cy+64+blocky,64+blockx+(blockw-1),64+blocky+(blockh-1),ITG_BLUE);
						}
					else
						swapscreen->PutPixel(cx+64,cy+64,ITG_BLUE);
					}
				// Show eggs
				if(ShowEggs)
					{
					if(!istricmp_fuzzy(ob->name,"egg*"))
						swapscreen->PutPixel(cx+64,cy+64,ITG_YELLOW);
					if(!istricmp_fuzzy(ob->name,"PATHMARKER"))
						swapscreen->PutPixel(cx+64,cy+64,ITG_RED);
					}
				}
			// Show roof
			if(ShowRoof)
				if(ReadRoof(cx,cy))
					swapscreen->PutPixel(cx+64,cy+64,ITG_RED);
			}


div = curmap->w>>6; //div64
rx=0;
ry=0;
for(cy=0;cy<64;cy++)
	{
	for(cx=0;cx<64;cx++)
		{
		temp = ReadMap(rx,ry);
		if(temp==RANDOM_TILE)
			swapscreen->PutPixel(MicroX+cx,MicroY+cy,ITG_BLUE);
		else
			swapscreen->PutPixel(MicroX+cx,MicroY+cy,TIlist[temp].form->seq[0]->thumbcol);
		rx+=div;
		}
	rx=0;
	ry+=div;
	}
}

void PanMap()
{
int temp,cx,cy,cw,ch,div,rx,ry,blockx,blocky,blockw,blockh;
OBJECT *ob;

if(pmx+384 > curmap->w)
	pmx = (curmap->w-384);
if(pmy+384 > curmap->h)
	pmy = (curmap->h-384);
if(pmx < 0)
	pmx = 0;
if(pmy < 0)
	pmy = 0;

cw=curmap->w;
if(cw>384)
	cw=384;
ch=curmap->h;
if(ch>384)
	ch=384;

for(cy=0;cy<ch;cy++)
	for(cx=0;cx<cw;cx++)
		{
		temp=ReadMap(cx+pmx,cy+pmy);
		if(temp==RANDOM_TILE)
			swapscreen->PutPixel(cx+64,cy+64,ITG_BLUE);
		else
			swapscreen->PutPixel(cx+64,cy+64,TIlist[temp].form->seq[0]->thumbcol);
		}

if(ShowSolid || ShowEggs || ShowRoof)
	for(cy=0;cy<ch;cy++)
		for(cx=0;cx<cw;cx++)
			{
			ob=GetRawObjectBase(cx+pmx,cy+pmy);
			for(;ob;ob=ob->next)
				{
				if(ShowSolid && ob->flags & IS_SOLID)
					{
					if(ob->flags & IS_LARGE)
						{
						if(ob->curdir > CHAR_D) //(U,D,L,R) > D = L,R
							{
							blockx=ob->hblock[BLK_X];
							blocky=ob->hblock[BLK_Y];
							blockw=ob->hblock[BLK_W];
							blockh=ob->hblock[BLK_H];
							}
						else
							{
							blockx=ob->vblock[BLK_X];
							blocky=ob->vblock[BLK_Y];
							blockw=ob->vblock[BLK_W];
							blockh=ob->vblock[BLK_H];
							}
//						swapscreen->FillRect(cx+64+blockx,cy+64+blocky,64+blockx+(blockw-1),64+blocky+(blockh-1),ITG_BLUE);
						swapscreen->FillRect(cx+64+blockx,cy+64+blocky,(blockw-1),(blockh-1),ITG_BLUE);
						}
					else
						swapscreen->PutPixel(cx+64,cy+64,ITG_BLUE);
					}
				// Show eggs
				if(ShowEggs)
					{
					if(!istricmp_fuzzy(ob->name,"egg*"))
						swapscreen->PutPixel(cx+64,cy+64,ITG_YELLOW);
					if(!istricmp_fuzzy(ob->name,"pathmarker"))
						swapscreen->PutPixel(cx+64,cy+64,ITG_RED);
					}
				}
			// Show roof
			if(ShowRoof)
				if(ReadRoof(cx+pmx,cy+pmy))
					swapscreen->PutPixel(cx+64,cy+64,ITG_RED);
			}

// Path point

if(path1x)
	if(path1y)
		if(path1x-pmx>0)
			if(path1y-pmy>0)
				if((path1x-pmx)+64<384)
					if((path1y-pmy)+64<384)
						{
						swapscreen->Circle((path1x-pmx)+64,(path1y-pmy)+64,3,ITG_RED);
						}

div = curmap->w>>6; // div64
rx=0;
ry=0;
for(cy=0;cy<64;cy++)
	{
	for(cx=0;cx<64;cx++)
		{
		temp = ReadMap(rx,ry);
		if(temp==RANDOM_TILE)
			swapscreen->PutPixel(MicroX+cx,MicroY+cy,ITG_BLUE);
		else
			swapscreen->PutPixel(MicroX+cx,MicroY+cy,TIlist[temp].form->seq[0]->thumbcol);
		rx+=div;
		}
	rx=0;
	ry+=div;
	}
}

void PanUp()
{
pmy-=16;
PanMap();
}

void PanDown()
{
pmy+=16;
PanMap();
}

void PanLeft()
{
pmx-=16;
PanMap();
}

void PanRight()
{
pmx+=16;
PanMap();
}

void SetL()
{
int hx,hy,xx,yy,nodraw=0,div;
static int lastx=666,lasty=666;
IRECOLOUR *col;
int WIDF;

if(!BrushMode)
	return;

WIDF=BrushType[BrushMode]; // Width of the brush

if(WIDF < 1) // Is it a special brush?
	{
	if(WIDF == -1)
		{
		Spray();
		return;
		}

	// If it isn't the spraycan, it's probably a roof paint job
	SetRoofL(-WIDF);
	return;
	}


hx = (x-64)+pmx;
hy = (y-64)+pmy;

nodraw=0;
if(hx==lastx && hy==lasty)
	nodraw=1;
lastx=hx;lasty=hy;

if(L_bgtile==RANDOM_TILE)
	col=ITG_BLUE;
else
	col=TIlist[L_bgtile].form->seq[0]->thumbcol;

if(hx>(curmap->w-WIDF))
	hx=(curmap->w-WIDF);
if(hy>(curmap->h-WIDF))
	hy=(curmap->h-WIDF);

for(xx=0;xx<WIDF;xx++)
	for(yy=0;yy<WIDF;yy++)
		{
		WriteMap(hx+xx,hy+yy,L_bgtile);
		if(x+xx<448 && y+yy<448)
			swapscreen->PutPixel(x+xx,y+yy,col);
		}
div=curmap->w>>6; // divide by 64
swapscreen->PutPixel(MicroX+(hx/div),MicroY+(hy/div),col);
}

void GoMap()
{
mapx = (x-64)+pmx;
mapy = (y-64)+pmy;
}

void Spray()
{
int hx,hy,xx,yy,nodraw=0;
IRECOLOUR *col;
static int lastx=666,lasty=666;

hx = (x-64)+pmx;
hy = (y-64)+pmy;

nodraw=0;
if(hx==lastx && hy==lasty)
	nodraw=1;
lastx=hx;lasty=hy;

if(L_bgtile==RANDOM_TILE)
	col=ITG_BLUE;
else
	col = TIlist[L_bgtile].form->seq[0]->thumbcol;

if(hx>(curmap->w-8))
	hx=(curmap->w-8);
if(hy>(curmap->h-8))
	hy=(curmap->h-8);

for(xx=0;xx<8;xx++)
	for(yy=0;yy<8;yy++)
		{
		if(rand()%100>95)
			{
			WriteMap(hx+xx,hy+yy,L_bgtile);
			if(x+xx<448 && y+yy<448)
				swapscreen->PutPixel(x+xx,y+yy,col);
			}
		}
//div=curmap->w>>6; // divide by 64
//putpixel(swapscreen,MicroX+(hx/div),MicroY+(hy/div),col);
}

void SetRoofL(int WIDF)
{
int hx,hy,xx,yy,nodraw=0,temp;
static int lastx=666,lasty=666;

hx = (x-64)+pmx;
hy = (y-64)+pmy;

nodraw=0;
if(hx==lastx && hy==lasty)
	nodraw=1;
lastx=hx;lasty=hy;

if(hx>(curmap->w-WIDF))
	hx=(curmap->w-WIDF);
if(hy>(curmap->h-WIDF))
	hy=(curmap->h-WIDF);

for(xx=0;xx<WIDF;xx++)
	for(yy=0;yy<WIDF;yy++)
		{
		WriteRoof(hx+xx,hy+yy,L_rooftile);
		if(x+xx<448 && y+yy<448)
			{
			if(L_rooftile)
				swapscreen->PutPixel(x+xx,y+yy,ITG_RED);
			else
				{
				temp = ReadMap(hx+xx,hy+yy);
				if(temp==RANDOM_TILE)
					swapscreen->PutPixel(x+xx,y+y,ITG_BLUE);
				else
					swapscreen->PutPixel(x+xx,y+yy,TIlist[temp].form->seq[0]->thumbcol);
				}
			}
		}
//div=curmap->w>>6; // divide by 64
//putpixel(swapscreen,MicroX+(hx/div),MicroY+(hy/div),col);
}

void Nothing()
{
PanMap();
}





void Connections()
{
focus=0;
IG_KillAll();

DrawScreenBox3D(128,128,384,320);

SetColor(ITG_WHITE);
DrawScreenText(144,160,"Map to the North");
CN_Id = IG_InputButton(280,152,ConnNstr,ConnN,NULL,NULL);
DrawScreenText(144,192,"Map to the South:");
CS_Id = IG_InputButton(280,184,ConnSstr,ConnS,NULL,NULL);
DrawScreenText(144,224,"Map to the East:");
CE_Id = IG_InputButton(280,216,ConnEstr,ConnE,NULL,NULL);
DrawScreenText(144,256,"Map to the West:");
CW_Id = IG_InputButton(280,248,ConnWstr,ConnW,NULL,NULL);

IG_TextButton(160,288,"Finish",Finish,NULL,NULL);
IG_AddKey(IREKEY_ESC,Finish);     // ESC to quit this menu
//IG_AddKey(KEY_F10,Snap);           // Add key binding for shift-F10

Connections_Update();
IG_WaitForRelease();
}

void Connections_Update()
{
if(curmap->con_n)
	sprintf(ConnNstr,"%6d",curmap->con_n);
else
	strcpy(ConnNstr,"None  ");
IG_UpdateText(CN_Id,ConnNstr);

if(curmap->con_s)
	sprintf(ConnSstr,"%6d",curmap->con_s);
else
	strcpy(ConnSstr,"None  ");
IG_UpdateText(CS_Id,ConnSstr);

if(curmap->con_e)
	sprintf(ConnEstr,"%6d",curmap->con_e);
else
	strcpy(ConnEstr,"None  ");
IG_UpdateText(CE_Id,ConnEstr);

if(curmap->con_w)
	sprintf(ConnWstr,"%6d",curmap->con_w);
else
	strcpy(ConnWstr,"None  ");
IG_UpdateText(CW_Id,ConnWstr);

IG_WaitForRelease();
}

void ConnN()
{
int ret;

ret=0;
ret=InputIntegerValueP(-1,-1,0,9999,curmap->con_n,"Enter map number ");
if(ret<0)
	return;

if(ret==mapnumber)
	if(!Confirm(-1,-1,"That's this map.","Are you sure you want to link this map to itself?"))
		return;

curmap->con_n=ret;
Connections_Update();
}

void ConnS()
{
int ret;

ret=0;
ret=InputIntegerValueP(-1,-1,0,9999,curmap->con_s,"Enter map number ");
if(ret<0)
	return;

if(ret==mapnumber)
	if(!Confirm(-1,-1,"That's this map.","Are you sure you want to link this map to itself?"))
		return;

curmap->con_s=ret;
Connections_Update();
}

void ConnE()
{
int ret;

ret=0;
ret=InputIntegerValueP(-1,-1,0,9999,curmap->con_e,"Enter map number ");
if(ret<0)
	return;

if(ret==mapnumber)
	if(!Confirm(-1,-1,"That's this map.","Are you sure you want to link this map to itself?"))
		return;

curmap->con_e=ret;
Connections_Update();
}

void ConnW()
{
int ret;

ret=0;
ret=InputIntegerValueP(-1,-1,0,9999,curmap->con_w,"Enter map number ");
if(ret<0)
	return;

if(ret==mapnumber)
	if(!Confirm(-1,-1,"That's this map.","Are you sure you want to link this map to itself?"))
		return;

curmap->con_w=ret;
Connections_Update();

}

void Finish()
{
IG_WaitForRelease();
BM_GoFocal();
}

#ifdef PATHBUILDER
void PathMaker()
{
//PathMode=1;
}
#endif

void GetBrush()
{
int ctr;
char brush[256];

for(ctr=0;Brushes[ctr];ctr++);

strcpy(brush,Brushes[BrushMode]);
InputNameFromList(-1,-1, "Choose a brush type:", ctr, Brushes, brush);

for(ctr=0;Brushes[ctr];ctr++)
	if(!strcmp(brush,Brushes[ctr]))
		{
		BrushMode=ctr;
		IG_UpdateText(BR_Id,Brushes[BrushMode]);
		return;
		}
}


