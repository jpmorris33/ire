/*
 *  - Background menu tab
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

extern int focus,BG_Id;
extern int MapXpos_Id,MapYpos_Id;       // Map X/Y Coordinate boxes
extern char mapxstr[],mapystr[];
extern void (*EdUpdateFunc)(void);


extern long M_core;     // Total amount of core, from memory.cc
extern int s_proj;      // Flag, are sprites being displayed?
extern int l_proj;      // Flag, are layers being displayed?

extern long mapx,mapy;   // Current map position
extern IREBITMAP *eyesore;
extern unsigned char editor_tip[]; // Animation frames

static int curoff=0;    // Current position on the tile list at the bottom
static int L=0,R=0;     // Current tile for each button, Lclick and Rclick
int L_bgtile=0;
static int cycleflag=0,cycleptr=0;
static int lastx=-666,lasty=-666;

// functions

extern void Toolbar();
extern void DrawMap(int x,int y,char s_proj,char l_proj,char f_proj);
extern void WriteMap(int x,int y,int tile);
extern int ReadMap(int x,int y);


extern int FindTilelinkNS(int n, int s);
extern int FindTilelinkEW(int e, int w);



static void Nothing();
static void clipmap();

void BGBl();
void BGBll();
void BGBr();
void BGBrr();
void BGBstart();
void BGBend();
void BG_bar();

void BG_up();
void BG_down();
void BG_left();
void BG_right();

static void GetL();                     // Get a tile for the left click
static void GetR();                     // Get a tile for the right click
static void SetR();                     // Write a right-click tile to map
static void SetL();                     // Write a left-click tile to map
static void GetEyesoreL();              // Get tile 65535 for the left click
static void GetEyesoreR();              // Get tile 65535 for the right click

static void Get_Prefab();               // Copy to clipboard
static void Put_Prefab();               // Copy to map
static void Make_Rand();                // Search and Replace
static void Tile_Rand();                // Randomize grass etc
static void Tile_RandW();               // Weighted randomize
static void Tile_BlockReplace();
static void ClearL();
static void ClearR();
static int AnalyseTile(int xx, int yy);
static void Tile_PathRoute();

static void Set_Background(int tile);   // Write a tile to the map, helper
static int Get_Background();

static void GetMapX();                  // Get map position
static void GetMapY();
static unsigned short prefab[8][8];     //
//static unsigned short blkundo[8][8];    // Undo block op

static void BG_Animate()
{
BG_bar();
DrawMap(mapx,mapy,s_proj,l_proj,0);             // Draw the map
}

// GoFocal - The menu tab function.  This is called from the toolbar

void BG_GoFocal()
{
int temp;               // Used to get button handles
char string[10];

if(focus==1)            // Make sure we don't redraw if the user clicks again
	return;

focus=1;                // This is now the focus
Toolbar();              // Build up the menu system again from scratch
IG_SetFocus(BG_Id);     // And make the button stick inwards

#ifdef __linux__
FILE *fp;
fp = fopen("prefab.dat","rb");
if(fp)
	{
	memset(prefab,0,sizeof(prefab));
	temp=fread(prefab,1,sizeof(prefab),fp);
	fclose(fp);
	}
#endif


// Set up Map window

IG_BlackPanel(VIEWX-1,VIEWY-1,258,258);
IG_Region(VIEWX,VIEWY,256,256,SetL,NULL,SetR);

// Write sprite drawing status

temp = IG_ToggleButton(486,72,"Sprites OFF",Nothing,NULL,NULL,&s_proj);
IG_SetInText(temp,"Sprites ON ");

temp = IG_ToggleButton(486,100,"Rooftops OFF",Nothing,NULL,NULL,&l_proj);
IG_SetInText(temp,"Rooftops ON ");

// Draw the VCR buttons on the Tile selector

IG_TextButton(8,410,__left,BGBl,NULL,NULL);     // Normal speed
IG_TextButton(616,410,__right,BGBr,NULL,NULL);

IG_TextButton(8,440,__left2,BGBll,NULL,BGBstart);   // Fast Forward
IG_TextButton(608,440,__right2,BGBrr,NULL,BGBend);

// Draw the contents of Left and Right buttons

IG_BlackPanel(400,192,32,32);
DrawScreenText(400,182," L");
if(L == RANDOM_TILE)
	eyesore->Draw(swapscreen,400,192);
else
	TIlist[L].form->seq[0]->image->Draw(swapscreen,400,192);

itoa(L,string,10);
DrawScreenText(400,232,string);

IG_BlackPanel(448,192,32,32);
DrawScreenText(448,182," R");
if(R == RANDOM_TILE)
     eyesore->Draw(swapscreen,448,192);
else
     TIlist[R].form->seq[0]->image->Draw(swapscreen,448,192);

itoa(R,string,10);
DrawScreenText(450,232,string);

BG_bar();                                       // Draw the Tile bar
IG_Region(32,400,576,32,GetL,NULL,GetR);        // Add clickable region

// Set up the X and Y map position counter at the top

MapXpos_Id = IG_InputButton(24,40,mapxstr,GetMapX,NULL,NULL);
MapYpos_Id = IG_InputButton(112,40,mapystr,GetMapY,NULL,NULL);

// Get and Put prefab (clipboard)

IG_TextButton(300,300,"Get prefab",Get_Prefab,NULL,NULL);
IG_TextButton(400,300,"Put prefab",Put_Prefab,NULL,NULL);
IG_TextButton(500,300,"Make Random",Make_Rand,NULL,NULL);

IG_TextButton(300,332,"Fill L tile",ClearL,NULL,NULL);
IG_TextButton(400,332,"Fill R tile",ClearR,NULL,NULL);
IG_TextButton(500,332,"Randomise tile",Tile_Rand,NULL,NULL);   // Ieahh!

IG_TextButton(300,364,"Random II",Tile_RandW,NULL,NULL);
IG_TextButton(400,364,"Tile Router",Tile_PathRoute,NULL,NULL);

IG_BlackPanel(31,340,34,34);
IG_Region(32,341,32,32,GetEyesoreL,NULL,GetEyesoreR);
eyesore->Draw(swapscreen,32,341);

temp = IG_ToggleButton(96,348,"Cycling off",Nothing,NULL,NULL,&cycleflag);
IG_SetInText(temp,"Cycling ON ");


DrawMap(mapx,mapy,s_proj,l_proj,0);             // Draw the map

EdUpdateFunc=BG_Animate;


IG_TextButton(416,104,__up,BG_up,NULL,NULL);         // Pan up button
IG_TextButton(400,128,__left,BG_left,NULL,NULL);     // Pan left button
IG_TextButton(432,128,__right,BG_right,NULL,NULL);   // Pan right button
IG_TextButton(416,150,__down,BG_down,NULL,NULL);     // Pan down button

IG_AddKey(IREKEY_UP,BG_up);                             // Pan up key binding
IG_AddKey(IREKEY_DOWN,BG_down);                         // Pan down key binding
IG_AddKey(IREKEY_LEFT,BG_left);                         // Pan left key binding
IG_AddKey(IREKEY_RIGHT,BG_right);                       // Pan right key binding

IG_AddKey(IREKEY_C,Get_Prefab);                         // get prefab key binding
IG_AddKey(IREKEY_V,Put_Prefab);                         // put prefab key binding
IG_AddKey(IREKEY_R,Tile_BlockReplace);                  // Block replace
}

void Nothing()
{
DrawMap(mapx,mapy,s_proj,l_proj,0);     // Do something anyway
return;
}

void BGBr()
{
curoff++;
if(curoff>(TItot-17))
    curoff=(TItot-17);
BG_bar();
}

void BGBl()
{
curoff--;
if(curoff<0) curoff=0;
BG_bar();
}

void BGBrr()
{
curoff+=10;
if(curoff>(TItot-17))
    curoff=(TItot-17);
if(curoff<0) curoff=0;
BG_bar();
}

void BGBll()
{
curoff-=10;
if(curoff<0) curoff=0;
BG_bar();
}

void BGBend()
{
curoff=(TItot-17);
if(curoff<0) curoff=0;
BG_bar();
}

void BGBstart()
{
curoff=0;
BG_bar();
}


void BG_bar()
{
int co;
co=17;
if(TItot<17)
    co=TItot;
IG_BlackPanel(31,399,578,34);
for(int ctr=0;ctr<co;ctr++) {
	TIlist[ctr+curoff].form->seq[editor_tip[ctr+curoff]]->image->Draw(swapscreen,33+(34*ctr),401);
	if(TIlist[ctr+curoff].form->overlay) {
		TIlist[ctr+curoff].form->overlay->image->Draw(swapscreen,33+(34*ctr),401);
	}
}

}

void GetL()
{
int co;
char string[10];

co=17;
if(TItot<17) co=TItot;
for(int ctr=0;ctr<co;ctr++) {
	if(x>33+(34*ctr) && x<65+(34*ctr)) {
		L=ctr+curoff;
		L_bgtile=L;
//		TIlist[L].form->seq[0]->image.block_put_sprite(400,192,swapscreen);
		TIlist[L].form->seq[0]->image->Draw(swapscreen,400,192);
		if(TIlist[L].form->overlay) {
			TIlist[L].form->overlay->image->Draw(swapscreen,400,192);
		}
		itoa(L,string,10);
		SetColor(ITG_LIGHTGRAY);
		DrawScreenBox(400,232,432,240);
		DrawScreenBox(384,240,512,248);
		SetColor(ITG_WHITE);
		DrawScreenText(400,232,string);
		DrawScreenText(384,240,TIlist[L].name);

		// If we're tile-cycling, reset and ensure sanity
		cycleptr=L;
		lastx=-666; // force a lastx,y reset

		return;
	}
}
}

void GetR()
{
int co;
char string[10];
co=17;
if(TItot<17)
     co=TItot;
for(int ctr=0;ctr<co;ctr++) {
	if(x>33+(34*ctr) && x<65+(34*ctr)) {
		R=ctr+curoff;
		TIlist[R].form->seq[0]->image->Draw(swapscreen,448,192);
		if(TIlist[R].form->overlay) {
			TIlist[R].form->overlay->image->Draw(swapscreen,448,192);
		}
		itoa(R,string,10);
		SetColor(ITG_LIGHTGRAY);
		DrawScreenBox(450,232,480,240);
		DrawScreenBox(384,240,512,248);
		SetColor(ITG_WHITE);
		DrawScreenText(450,232,string);
		DrawScreenText(384,240,TIlist[R].name);

		// If we're tile-cycling, ensure sanity
		if(cycleflag)
			if(cycleptr>R)
				cycleptr=R;

		return;
	}
}
}

void SetL()
{
int l;
char string[10];

// If shift is pressed, grab tile instead of drawing
if(IRE_TestShift(IRESHIFT_SHIFT)) {
	l=Get_Background();
	if(l == RANDOM_TILE) {
		GetEyesoreL();
		return;
	}

	if(l>=TItot) {
		l=TItot-1;
	}
	if(l != -1) {
		L=l;
	}
	L_bgtile=L; // Grab these too
	cycleptr=L;
	// Draw L icon
//	draw_rle_sprite(swapscreen,TIlist[L].form->seq[0]->image,400,192);
	TIlist[L].form->seq[0]->image->Draw(swapscreen,400,192);
	if(TIlist[L].form->overlay) {
		TIlist[L].form->overlay->image->Draw(swapscreen,400,192);
	}
	itoa(L,string,10);
	SetColor(ITG_LIGHTGRAY);
	DrawScreenBox(400,232,432,240);
	DrawScreenBox(384,240,512,248);
	SetColor(ITG_WHITE);
	DrawScreenText(400,232,string);
	DrawScreenText(384,240,TIlist[L].name);
	return;
}

// Write tile
Set_Background(L);
}

void SetR()
{
int r;
char string[10];

// If shift is pressed, grab tile instead of drawing
if(IRE_TestShift(IRESHIFT_SHIFT)) {
	r=Get_Background();
	if(r == RANDOM_TILE) {
		GetEyesoreR();
		return;
	}

	if(r>=TItot) {
		r=TItot-1;
	}
	if(r != -1) {
		R=r;
	}
	// Draw R icon
	TIlist[R].form->seq[0]->image->Draw(swapscreen,448,192);
	if(TIlist[R].form->overlay) {
		TIlist[R].form->overlay->image->Draw(swapscreen,448,192);
	}
	itoa(R,string,10);
	SetColor(ITG_LIGHTGRAY);
	DrawScreenBox(450,232,480,240);
	DrawScreenBox(384,240,512,248);
	SetColor(ITG_WHITE);
	DrawScreenText(450,232,string);
	DrawScreenText(384,240,TIlist[R].name);
	return;
}

// Write tile
Set_Background(R);
}

void Set_Background(int tile)
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
	{
	// Convert to map coordinates
	xxx+=mapx;
	yyy+=mapy;
	// If we are tile-cycling, only write if tile isn't same as last
	if(cycleflag)
		if(xxx == lastx && yyy == lasty)
			return;

	// Handle any tile-cycling
	if(cycleflag)
		{
		if(L==RANDOM_TILE || R==RANDOM_TILE || L>R)
			return;
		tile=cycleptr++;
		if(cycleptr>R)
			cycleptr=L;
		}

	WriteMap(xxx,yyy,tile);
	lastx=xxx;
	lasty=yyy;
	}
DrawMap(mapx,mapy,s_proj,l_proj,0);
}


int Get_Background()
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
	return ReadMap(mapx+xxx,mapy+yyy);

return -1;
}



void BG_up()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapy-=8;
else
	mapy--;
clipmap();
DrawMap(mapx,mapy,s_proj,l_proj,0);
}

void BG_down()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapy+=8;
else
	mapy++;
clipmap();
DrawMap(mapx,mapy,s_proj,l_proj,0);
}


void BG_left()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapx-=8;
else
	mapx--;
clipmap();
DrawMap(mapx,mapy,s_proj,l_proj,0);
}

void BG_right()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapx+=8;
else
	mapx++;
clipmap();
DrawMap(mapx,mapy,s_proj,l_proj,0);
}

void clipmap()
{
if(mapx<0) mapx=0;
if(mapx>curmap->w-VSW) mapx=curmap->w-VSW;
if(mapy>curmap->h-VSH) mapy=curmap->h-VSH;
if(mapy<0) mapy=0;
}

void GetMapX()
{
mapx=InputIntegerValue(-1,-1,0,curmap->w-VSW,mapx);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}

void GetMapY()
{
mapy=InputIntegerValue(-1,-1,0,curmap->h-VSH,mapy);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}

void ClearL()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                WriteMap(mapx+cx,mapy+cy,L);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}

void ClearR()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                WriteMap(mapx+cx,mapy+cy,R);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}

void Get_Prefab()
{
FILE *fp;
int cx,cy;
for(cy=0;cy<VSH;cy++)
	for(cx=0;cx<VSW;cx++)
		prefab[cx][cy]=ReadMap(mapx+cx,mapy+cy);

#ifdef __linux__
fp = fopen("prefab.dat","wb");
if(fp)
	{
	cx=fwrite(prefab,1,sizeof(prefab),fp);
	fclose(fp);
	}
#endif
}

void Put_Prefab()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                WriteMap(mapx+cx,mapy+cy,prefab[cx][cy]);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}

/*
void Tile_Rand()
{
int cx,cy,z,bad,badcount,tile_n,tile_u,tile_d,tile_l,tile_r;
long rep=0,count,max;
char str[128];

#define Z_THRESH 5

if(!Confirm(-1,-1,"This will replace all RANDOM tiles","with the tiles between the L and R mouse buttons."))
	return;

if(R<L)
	{
	Notify(-1,-1,"The tile assigned to the LEFT button must come before","the tile assigned to the RIGHT button.");
	return;
	}

if(L==RANDOM_TILE|| R==RANDOM_TILE)
	{
	Notify(-1,-1,"You must not choose the RANDOM tile as a replacement!",NULL);
	return;
	}

z=R-L;
rep=0;
max=curmap->w*curmap->h;
count=0;

DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
IG_Text(VIEWX+72,VIEWY+72,"Randomising...",ITG_WHITE);
Show();

for(cy=0;cy<curmap->h;cy++)
	for(cx=0;cx<curmap->w;cx++)
		{
		if(ReadMap(cx,cy)==RANDOM_TILE)
			{
			if(z)
				{
				if(z<Z_THRESH)
					tile_n = L+(rand()%z);
				else
					{
					badcount=0;
					tile_n = RANDOM_TILE;
					do {
						if(tile_n == RANDOM_TILE)
							tile_n = L+(rand()%z);
						// Try to prevent adjacent tiles
						if(cx>0)
							tile_l = ReadMap(cx-1,cy);
						else
							tile_l = RANDOM_TILE;
						if(cx<curmap->w)
							tile_r = ReadMap(cx+1,cy);
						else
							tile_r = RANDOM_TILE;
						if(cy>0)
							tile_u = ReadMap(cx,cy-1);
						else
							tile_u = RANDOM_TILE;
						if(cx<curmap->h)
							tile_d = ReadMap(cx,cy+1);
						else
							tile_d = RANDOM_TILE;
						bad=0;
						if(tile_l == tile_n || tile_r == tile_n || tile_u == tile_n || tile_d == tile_n)
							{
							tile_n=RANDOM_TILE;
							badcount++;
							bad=1;
							}
						if(badcount>255)
							{
							bad=0; // time out
							ilog_quiet("randomiser timed out, adjust Z_THRESH\n");
							}
						} while(bad);
					}
				}
			else
				tile_n = L;
			WriteMap(cx,cy,tile_n);
			rep++;
			}
		count++;
		DrawScreenMeter(VIEWX+66,VIEWY+96,VIEWX+190,VIEWY+112,(float)count/(float)max);
		}
sprintf(str,"%ld random tiles were found.",rep);
Notify(-1,-1,str,NULL);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}
*/

void Tile_Rand()
{
int cx,cy,z,tile_n;

if(!Confirm(-1,-1,"This will replace all RANDOM tiles","with the tiles between the L and R mouse buttons."))
	return;

if(R<L)
	{
	Notify(-1,-1,"The tile assigned to the LEFT button must come before","the tile assigned to the RIGHT button.");
	return;
	}

if(L==RANDOM_TILE|| R==RANDOM_TILE)
	{
	Notify(-1,-1,"You can not choose the RANDOM tile as a replacement.",NULL);
	return;
	}

z=(R-L)+1;

DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
IG_Text(VIEWX+72,VIEWY+72,"Randomising...",ITG_WHITE);
Show();

for(cy=0;cy<curmap->h;cy++)
	for(cx=0;cx<curmap->w;cx++)
		{
		if(ReadMap(cx,cy)==RANDOM_TILE)
			{
			if(z)
				tile_n = L+(rand()%z);
			else
				tile_n = L;
			// Thin out for hedges etc
			if(cx>1 && cx<curmap->w-1)
				if(cy>1 && cy<curmap->h-1)
					{
					if(ReadMap(cx,cy-1) == tile_n)
						tile_n=L;
					if(ReadMap(cx-1,cy) == tile_n)
						tile_n=L;
					if(ReadMap(cx+1,cy) == tile_n)
						tile_n=L;
					if(ReadMap(cx,cy+1) == tile_n)
						tile_n=L;
					}
			WriteMap(cx,cy,tile_n);
			}
		}
//sprintf(str,"%ld random tiles were found.",rep);
//Notify(-1,-1,str,NULL);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}


void Tile_RandW()
{
int cx,cy,z,tile_n;
long max,thresh;

if(!Confirm(-1,-1,"This will replace all RANDOM tiles","with the tiles between the L and R mouse buttons."))
	return;

if(R<L)
	{
	Notify(-1,-1,"The tile assigned to the LEFT button must come before","the tile assigned to the RIGHT button.");
	return;
	}

if(L==RANDOM_TILE|| R==RANDOM_TILE)
	{
	Notify(-1,-1,"You can not choose the RANDOM tile as a replacement.",NULL);
	return;
	}

//max=InputIntegerValueP(-1,-1,0,100,10,"Enter Range value");
max=100;
thresh=InputIntegerValueP(-1,-1,0,100,5,"Enter Density");
if(thresh<0)
	return;

z=(R-L)+1;

DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
IG_Text(VIEWX+72,VIEWY+72,"Randomising...",ITG_WHITE);
Show();

for(cy=0;cy<curmap->h;cy++)
	for(cx=0;cx<curmap->w;cx++)
		{
		if(ReadMap(cx,cy)==RANDOM_TILE)
			{
			if(z)
				tile_n = L+(rand()%z);
			else
				tile_n = L;

			if(rand()%max > thresh)
				tile_n=L;
			else
				{
				// Thin out for hedges etc
				if(cx>1 && cx<curmap->w-1)
					if(cy>1 && cy<curmap->h-1)
						{
						if(ReadMap(cx,cy-1) == tile_n)
							tile_n=L;
						if(ReadMap(cx-1,cy) == tile_n)
							tile_n=L;
						if(ReadMap(cx+1,cy) == tile_n)
							tile_n=L;
						if(ReadMap(cx,cy+1) == tile_n)
							tile_n=L;
						}
				}
			WriteMap(cx,cy,tile_n);
			}
		}
//sprintf(str,"%ld random tiles were found.",rep);
//Notify(-1,-1,str,NULL);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}


void Make_Rand()
{
int cx,cy,z;
long rep=0;
char str[128];

if(!Confirm(-1,-1,"This will replace all tiles between the L and R mouse buttons","with the RANDOM tile."))
    return;

if(R<L)
    {
    Notify(-1,-1,"The tile assigned to the LEFT button must come before","the tile assigned to the RIGHT button.");
    return;
    }

if(L==RANDOM_TILE|| R==RANDOM_TILE)
    {
    Notify(-1,-1,"You cannot search for the RANDOM tile.",NULL);
    return;
    }


z=R-L;

for(cy=0;cy<curmap->h;cy++)
    for(cx=0;cx<curmap->w;cx++)
        {
        z=ReadMap(cx,cy);
        if(z>=L && z<=R)
            {
            WriteMap(cx,cy,RANDOM_TILE);
            rep++;
            }
        }
sprintf(str,"%ld tiles were replaced.",rep);
Notify(-1,-1,str,NULL);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}

void GetEyesoreL()
{
L=RANDOM_TILE;
L_bgtile=L;
eyesore->Draw(swapscreen,400,192);
}

void GetEyesoreR()
{
R=RANDOM_TILE;
eyesore->Draw(swapscreen,448,192);
}


void Tile_BlockReplace()
{
static int cx,cy,cw,ch;
int vx,vy;

if(!Confirm(-1,-1,"This will replace all L tiles with R tiles","between two given coordinates."))
	return;

vx=cx;
cx=InputIntegerValueP(-1,-1,0,curmap->w,cx,"Enter starting X coordinate");
if(cx<0)
	{
	cx=vx;
	return;
	}

vy=cy;
cy=InputIntegerValueP(-1,-1,0,curmap->h,cy,"Enter starting Y coordinate");
if(cy<0)
	{
	cy=vy;
	return;
	}

vx=cw;
cw=InputIntegerValueP(-1,-1,cx,curmap->w,cw,"Enter finishing X coordinate");
if(cw<0)
	{
	cw=vx;
	return;
	}

vy=ch;
ch=InputIntegerValueP(-1,-1,cy,curmap->h,ch,"Enter finishing Y coordinate");
if(ch<0)
	{
	ch=vy;
	return;
	}

DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
IG_Text(VIEWX+72,VIEWY+72,"Processing...",ITG_WHITE);
Show();

for(vy=cy;vy<ch;vy++)
	for(vx=cx;vx<cw;vx++)
		if(ReadMap(vx,vy)==L)
			WriteMap(vx,vy,R);
//sprintf(str,"%ld random tiles were found.",rep);
//Notify(-1,-1,str,NULL);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}



void Tile_PathRoute()
{
int cx,cy,xx,yy;

if(!Confirm(-1,-1,"This will trace a shore outline from the random tiles on the map.",""))
	return;

DrawScreenBox3D(VIEWX+64,VIEWY+64,VIEWX+192,VIEWY+128);
IG_Text(VIEWX+72,VIEWY+72,"Processing...",ITG_WHITE);
Show();

xx=-1;
yy=-1;
for(cy=0;cy<VSH;cy++)
	for(cx=0;cx<VSW;cx++)
		{
		if(ReadMap(cx+mapx,cy+mapy)==RANDOM_TILE)
			{
			xx=cx+mapx;
			yy=cy+mapy;
			AnalyseTile(xx,yy);
			break;
 			}
		}

		
//sprintf(str,"%ld random tiles were found.",rep);
//Notify(-1,-1,str,NULL);
DrawMap(mapx,mapy,s_proj,l_proj,0);
}


int AnalyseTile(int xx, int yy)
{
int N,S,E,W;
int state,link;

if(ReadMap(xx,yy) != RANDOM_TILE)
	return 0;

unsigned int cardinal=0;

state=0;
link=-1;
  
if(xx > 0 && yy > 0 && xx<curmap->w && yy<curmap->h)
	{
	N=S=E=W=-1;
	
	N=ReadMap(xx,yy-1);
	S=ReadMap(xx,yy+1);
	E=ReadMap(xx+1,yy);
	W=ReadMap(xx-1,yy);

	cardinal=0;
	if(N == RANDOM_TILE)
		cardinal |= 1;
	if(S == RANDOM_TILE)
		cardinal |= 2;
	if(E == RANDOM_TILE)
		cardinal |= 4;
	if(W == RANDOM_TILE)
		cardinal |= 8;
	
	switch(cardinal)
		{
		case 1:
		case 2:
		case 3:
			state=0;
			break;
		case 4:
		case 8:
		case 12:
			state=1;
			break;
		}


	// Now we've figured out if it's vertical (0) or horizontal (1)
	// Work out what the tiles are on the non-random axis

	if(state == 0)
		link = FindTilelinkEW(E,W);
	if(state == 1)
		link = FindTilelinkNS(N,S);

	if(link	> 0)
		{
		WriteMap(xx,yy,TLlist[link].tile);
		if(N == RANDOM_TILE)
			AnalyseTile(xx,yy-1);
		if(S == RANDOM_TILE)
			AnalyseTile(xx,yy+1);
		if(E == RANDOM_TILE)
			AnalyseTile(xx+1,yy);
		if(W == RANDOM_TILE)
			AnalyseTile(xx-1,yy);
		}

	}

return 1;
}
