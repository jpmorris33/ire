/*
 *      Project - Sprite and map projection code
 */

#include <string.h>

#include "core.hpp"
#include "ithelib.h"
#include "loadfile.hpp"
#include "gamedata.hpp"
#include "map.hpp"
#include "object.hpp"
#include "project.hpp"
#include "console.hpp"
#include "media.hpp"
#include "oscli.hpp"

// Defines
#define MAX_P_OVERLAY 255       // Maximum number of overlayed objects
                                // Per screen.  Somewhat unlikely..

// Various diagnostics
//#define SHOW_BLOCKLIGHTMAP
//#define SHOW_VISLIGHTMAP
//#define SHOW_GHOSTLY

struct LTAB
	{
	int x,y;
	int light;
	};

static struct POSTOVL
	{
	int x,y;
	IRESPRITE *s;
	} postoverlay[MAX_P_OVERLAY];

static struct PROJPEOPLE
	{
	int x,y;
	int cx,cy;
	OBJECT *obj;
	} people[MAX_P_OVERLAY];

static OBJECT *postfx_buf[MAX_P_OVERLAY];

// Variables

//static char Animate=0;
static unsigned char tip[65535];        // Tile Animation Pointer
static IREBITMAP **tsb;                     // Tile Scroll Buffers
static IREBITMAP *warning;
static IRELIGHTMAP *darkmap;
static IREBITMAP *tsbscreen;
static IREBITMAP *scalebmp;
static int darklevel=128;
VMINT dark_mix=0;
VMINT show_invisible=0;	// This is expose to the script engine now
static int masterclock=0;
static IRECOLOUR blanker(0,0,0);

void (*animate[8])(OBJECT *temp);
static unsigned char *rndtab[256];

    // Lookup table for the possible light paths.
    // Each node has three adjacent squares which are lit by the engine.
    // The dark routine lights up (or darkens) each square, and marks it
    // as being used, so that it cannot be brightened twice.
    // This is necessary because there will be overlap between some nodes.
    // That's necessary due to this algorithm...

static int node[8][4][2]=	{
							{ {-1,-1},{-2,-2},{-2,-1},{-1,-2}  }, // top-left
							{ {+0,-1},{+0,-2},{-1,-2},{+1,-2}  }, // top
							{ {+1,-1},{+2,-2},{+1,-2},{+2,-1}  }, // top-right
							{ {+1,+0},{+2,+0},{+2,-1},{+2,+1}  }, // right
							{ {+1,+1},{+2,+2},{+2,+1},{+1,+2}  }, // bottom-right
							{ {+0,+1},{+0,+2},{-1,+2},{+1,+2}  }, // bottom
							{ {-1,+1},{-2,+2},{-2,+1},{+1,+2}  }, // bottom-left
							{ {-1,+0},{-2,+0},{-2,-1},{-2,+1}  }  // left
							};

static IRELIGHTMAP *lightmap;       // The shape of the lightmask we'll use
static IRELIGHTMAP *lm[8][8];         // The lightmask split into tiles.
static int lmw,lmh;            // Width and height of the lightmask in tiles.
static int lmw2,lmh2;          // lmw and lmh respectively, but divided by two
static int basex,basey;
static int post_ovl=0;
static int people_ovl=0;
static int postfx=0;
static char vismap[LMSIZE][LMSIZE]; // visibility state
static char blmap[LMSIZE][LMSIZE];  // blocks light
int gamewinsize;

static OBJECT *roofsprites[MAX_P_OVERLAY]; // Objects which are on the roof
static int roofspr;

// Functions

void Init_Projector(int w, int h);
void Project_Map(int x, int y);
void Project_Sprites(int x, int y);
void Project_Roof(int x, int y);
void Hide_Unseen(int x, int y);
void SetDarkness(int d);
static void Project_sprite(OBJECT *temp, int cx, int cy, int x, int y);

static void anim_00(OBJECT *temp);
static void anim_01(OBJECT *temp);
static void anim_10(OBJECT *temp);
static void anim_11(OBJECT *temp);
void anim_random(OBJECT *temp);
void anim_unisync(OBJECT *temp);
void anim_none(OBJECT *temp);

static void init_lightmap();
static void proj_light(int x, int y, int light);
void render_lighting(int x, int y);
static void vis_floodfill(int x, int y);
static void vis_punch(int x, int y);
int isBlanked(int x, int y);
extern void CallVMnum(int func);
extern IRELIGHTMAP *load_lightmap(char *filename);

// Code

/*
 *      Init_Projector() - Set up the jump table for the animator
 *                         Create projection window
 */

void Init_Projector(int w, int h)
{
int ctr,ctr2;
char filename[1024];
unsigned int rr,gg,bb;

// Calculate blanker colour
if(blankercol > 0)
	{
	rr=(blankercol>>16)&0xff;   // get Red
	gg=(blankercol>>8)&0xff;    // get green
	bb=blankercol&0xff;         // get blue
//	printf("bk = %x, r=%x, g=%x, b=%x\n",blankercol,rr,gg,bb);
//	blankercol = makecol((unsigned int)rr,(unsigned int)gg,(unsigned int)bb);
	blanker.Set(rr,gg,bb);
	}
else
	{
	// If no colour has been specified, assume pure black (0,0,0)
	blanker.Set(0,0,0);
	}

animate[0] = anim_00;
animate[1] = anim_01;
animate[2] = anim_10;
animate[3] = anim_11;
animate[4] = anim_random;	// Random
animate[5] = anim_unisync;	// Unisync
animate[6] = anim_none;
animate[7] = anim_none;


tsbscreen = MakeIREBITMAP(64,64);
if(!tsbscreen)
	ithe_panic("Could not allocate tile scroll buffer",NULL);

//gamewin=MakeScreen(w,h);
gamewin = MakeIREBITMAP(w,h);
if(!gamewin)
    ithe_panic("Could not allocate projection window",NULL);
gamewinsize = w*h;

//roofwin=MakeScreen(w,h);
roofwin = MakeIREBITMAP(w,h);
if(!roofwin)
    ithe_panic("Could not allocate projection window",NULL);

tsb = (IREBITMAP **)M_get(TItot,sizeof(IREBITMAP *)); // Tile Scroll Buffer

scalebmp = MakeIREBITMAP(w,h);
if(!scalebmp)
    ithe_panic("Could not allocate map scaling buffer",NULL);

if(!loadfile("warning.cel",filename))
	ithe_panic("Could not find Warning image (warning.cel)",NULL);
warning=iload_bitmap(filename);
if(!warning)
	ithe_panic("Could not load Warning image",filename);

// Build a random number table for the random animation code

for(ctr=0;ctr<256;ctr++)
    {
    rndtab[ctr]=(unsigned char *)M_get(1,512);
    if(ctr>0)
        for(ctr2=0;ctr2<512;ctr2++)
            {
            rndtab[ctr][ctr2]=rand()%ctr;
            // Prevent consecutive numbers appearing
            if(ctr2>0 && ctr>1)
                while(rndtab[ctr][ctr2]==rndtab[ctr][ctr2-1])
                    rndtab[ctr][ctr2]=rand()%ctr;
            }
    }

// Build the tile scroll buffer

for(ctr=0;ctr<TItot;ctr++)
	if(!tsb[ctr])
		{
		tsb[ctr]=MakeIREBITMAP(32,32);
		if(!tsb[ctr])
			{
			TIlist[ctr].sdx=0;
			TIlist[ctr].sdy=0;
			Bug("Could not create TSB for tile %d\n",ctr);
				return;
			}
		}

darkmap = MakeIRELIGHTMAP(VSW32,VSH32);
if(!darkmap)
	ithe_panic("Create DMP bitmap failed",NULL);

init_lightmap();
SetDarkness(darklevel); // Resync darkness level
}

/*
 *      anim_00() - no ping-pong, no loop
 */

void anim_00(OBJECT *temp)
{
OBJECT *old; // Old pointer value

temp->sptr+=temp->sdir;         // Increment the animation frame
if(temp->sptr>=temp->form->frames) // If the animation pointer > # of frames
	{
	temp->sptr=temp->form->frames-1;   // Set it to the last frame
	temp->sdir=0;              // Stop playing

	// If we're going to jump to another sequence, restart it
	if(temp->form->flags&SEQFLAG_JUMPTO)
		{
		temp->sdir=1;
		temp->sptr=0;
//		ilog_quiet("%s jump to %s\n",temp->form->name,temp->form->jumpto->name);
		temp->form = temp->form->jumpto;
		}

	// If we have a function call, do it
	if(temp->form->hookfunc > 0)
		{
		old=person;
		person = temp;
		CallVMnum(temp->form->hookfunc);
		person=old;
		}
	}
}

/*
 *      anim_01() - ping-pong, no loop
 */

void anim_01(OBJECT *temp)
{
OBJECT *old; // Old pointer value

temp->sptr+=temp->sdir;         // Increment the animation frame
if(temp->sptr>=temp->form->frames)	// if we reach upper limit
	{
	temp->sdir=-1;          // Play backwards
	temp->sptr-=2;		// This prevents 0123321001233210 etc
	}									// (should be 012321012321 etc)
else
	if(temp->sptr<0)	// if we reach -1, we've finished the backrun
		{
		temp->sdir=0;		// shut down
		temp->sptr=0;		// NEVER have a cyclic loop with 1 frame
	                        // or this will blow up on you.

		// If we're going to jump to another sequence, restart it
		if(temp->form->flags&SEQFLAG_JUMPTO)
			{
			temp->sdir=1;
			temp->sptr=0;
			temp->form = temp->form->jumpto;
			}

		// If we have a function call, do it
		if(temp->form->hookfunc > 0)
			{
			old=person;
			person = temp;
			CallVMnum(temp->form->hookfunc);
			person=old;
			}
		}

}

/*
 *      anim_10() - no ping-pong, loop
 */

void anim_10(OBJECT *temp)
{
temp->sptr+=temp->sdir;         // Increment the animation frame
if(temp->sptr>=temp->form->frames)      // If we reach the end,
	temp->sptr=0;           // Go back to the start
}

/*
 *      anim_11() - ping-pong, and loop
 */

void anim_11(OBJECT *temp)
{
temp->sptr+=temp->sdir;         // Increment the animation frame
if(temp->sptr>=temp->form->frames)	// if we reach upper limit
	{
	temp->sdir=-1;          // Play backwards
	temp->sptr-=2;		// prevent 0123321001233210 where it
	}					// should be 012321012321 etc
else
	if(temp->sptr<0)	// if we reach -1, finished the backward run
	{
	temp->sdir=1;		// Start playing forwards again
	temp->sptr=1;		// NEVER have a cyclic loop with 1 frame!
	}
}

/*
 *      anim_unisync() - loop synced to master clock
 */

void anim_unisync(OBJECT *temp)
{
if(temp->form->speed)
	temp->sptr=(masterclock/temp->form->speed)%temp->form->frames;
else
	temp->sptr=masterclock%temp->form->frames;
}

// Stub animation (Do Nothing)

void anim_none(OBJECT *temp)
{
temp->sptr=0;
}

/*
 *      anim_random() - random frames
 */

void anim_random(OBJECT *temp)
{
temp->sptr=rndtab[temp->form->frames][temp->sdir++];
if(temp->sdir == 512)
    temp->sdir=0;
}

/*
 *      Project_Map - Display the map on screen
 */

void Project_Map(int x,int y)
{
int xctr=0,altcode;
int yctr=0,ptr,bit,t,ctr,ctr2;

// Tile animation controller

// For each tile...
	for(ctr=0;ctr<TItot;ctr++)
		{
		// If it animates, update the tiles clock and frame when necessary
		if(TIlist[ctr].form->frames)
			{
			TIlist[ctr].tick++;
			if(TIlist[ctr].tick>=TIlist[ctr].form->speed)
				{
				tip[ctr]+=TIlist[ctr].sdir;
				if(tip[ctr]>=TIlist[ctr].form->frames)
					tip[ctr]=0;
				TIlist[ctr].tick=0;
				}
			}

		// If it scrolls, duplicate the tile in an offscreen buffer and grab
		// it at the appropriate offset to make it appear to scroll
		if(TIlist[ctr].sdx || TIlist[ctr].sdy)
			{
			TIlist[ctr].form->seq[tip[ctr]]->image->Draw(tsbscreen,0,0);
			TIlist[ctr].form->seq[tip[ctr]]->image->Draw(tsbscreen,0,32);
			TIlist[ctr].form->seq[tip[ctr]]->image->Draw(tsbscreen,32,0);
			TIlist[ctr].form->seq[tip[ctr]]->image->Draw(tsbscreen,32,32);
			tsb[ctr]->Get(tsbscreen,32-TIlist[ctr].sx,32-TIlist[ctr].sy);
			TIlist[ctr].sx = (TIlist[ctr].sx + TIlist[ctr].sdx)&31;
			TIlist[ctr].sy = (TIlist[ctr].sy + TIlist[ctr].sdy)&31;
			}
		}


// Now draw the map

yctr=0;
xctr=0;
ptr=y*curmap->w;
ptr+=x;
bit=curmap->w - VSW;

for(ctr=0;ctr<VSH;ctr++)
	{
	for(ctr2=0;ctr2<VSW;ctr2++)
		{
		t = curmap->physmap[ptr];
		if(t == RANDOM_TILE)
			warning->Draw(gamewin,xctr,yctr);
		else
			{
			if(TIlist[t].sdx || TIlist[t].sdy)
				tsb[t]->Draw(gamewin,xctr,yctr);
			else
				{
				// Transform a tile into another shape if it is on the edge
				// of the blackness, and there is a rule to handle it.

				if(TIlist[t].alternate)
					{

					// The shape of the alternate tile is in an array of
					// sixteen image pointers.
					// There are sixteen possible combinations of adjacent
					// tiles on or off (or 'rules').
					// To get the number of the combination, we add four
					// bits together, taken from the state of each adjacent
					// tile: L=1, R=2, U=4, D=8.
					// To check the state, we look at the vismap.
					// If a tile is blanked, the vismap entry will have bit 1
					// clear. The vismap is 1-based, not 0-based (outer rim!)
					// so instead of checking   -1,0 +1,0  0,-1  0,+1
					// we have to check instead  0,1 +2,+1 +1,0 +1,+2
					// The optimised method below just adds the bits.
					// The unoptimised version is easier to understand.

					// Optimised
					altcode=(vismap[ctr2][ctr+1]&1);
					altcode|=(vismap[ctr2+2][ctr+1]&1)<<1;
					altcode|=(vismap[ctr2+1][ctr]&1)<<2;
					altcode|=(vismap[ctr2+1][ctr+2]&1)<<3;
					altcode^=0xf; // invert it all

					// Blank the sprites behind the wall
					if(altcode&10) // If Down OR Right
						vismap[ctr2+1][ctr+1] |= 4; // mark bad (no sprites)

/*
					// Easier-to-read

					// Add up each direction, to get the rule number
					// Check L(-1,0), R(+1,0), U(0,-1), D(0,+1)
					// Vismap has +1,+1 added to it for the outer rim
					altcode=0;
					if(!(vismap[ctr2][ctr+1]&1)) altcode|=1; // L
					if(!(vismap[ctr2+2][ctr+1]&1))
						{
						altcode|=2; // R
						vismap[ctr2+1][ctr+1] |= 4; // mark bad (no sprites)
						}
					if(!(vismap[ctr2+1][ctr]&1)) altcode|=4; // U
					if(!(vismap[ctr2+1][ctr+2]&1))
						{
						altcode|=8; // D
						vismap[ctr2+1][ctr+1] |= 4; // mark bad (no sprites)
						}
*/

					TIlist[t].alternate[altcode]->seq[0]->image->Draw(gamewin,xctr,yctr);
					}
				else
					TIlist[t].form->seq[tip[t]]->image->Draw(gamewin,xctr,yctr);
				}
			}
#ifdef SHOW_BLOCKLIGHTMAP
		if(blmap[ctr2][ctr])
			rectfill(gamewin,xctr,yctr,xctr+33,yctr+16,makecol(128,255,127));
#endif
#ifdef SHOW_VISLIGHTMAP
		int lll;
		lll = vismap[ctr2+1][ctr+1] * 63; // four levels
		rectfill(gamewin,xctr,yctr,xctr+32,yctr+32,makecol(lll,lll,lll));
#endif
		xctr+=32;
		ptr++;
		}
	xctr=0;
	yctr+=32;
	ptr+=bit;
	}
}


/*
 *      Project_sprites- Display the sprites on the map
 */

void Project_Sprites(int x, int y)
{
int cx,cy,vx,vy,yoff,ok,xck;
int startx,starty,ctr;
OBJECT *temp,*oldcurrent,*oldme;

// Preserve original value of Current
oldcurrent = current_object;
oldme = person;

masterclock=GetAnimClock();

// Zero the counter
postfx = 0;
post_ovl = 0;
people_ovl = 0;
roofspr = 0; // No objects on the roof
cx=0;
cy=0;

startx = -VIEWDIST;
starty = -VIEWDIST;

if((x+startx)<0)
	startx=startx - (x+startx);
if((y+starty)<0)
	starty=starty - (y+starty);

if(x>(curmap->w-VSW))
	x=curmap->w-VSW;
if(y>(curmap->h-VSH))
	y=curmap->h-VSH;

for(vy=starty;vy<VSH;vy++)
	{
	yoff=ytab[y+vy];
	for(vx=startx;vx<VSW;vx++)
		{
		ok=1;
		// If there's darkness on this bit of map, don't draw the sprites
		if(vx>=0 && vy>=0)
			if(vismap[vx+1][vy+1]&4)
				ok=0;

		for(temp=curmap->objmap[yoff+x+vx];temp;temp=temp->next)
			{
			if(temp->flags & IS_ON)
				{
				// If a large object is partially-blanked, don't show it
				// Don't do the check if it's always drawn on top (Post-Overlay)
				if(temp->flags & IS_LARGE && (temp->form->flags&SEQFLAG_POSTOVERLAY) == 0)
					if(IsTileSolid(temp->x,temp->y))
						{
						// It looks like we have something against a wall.
						// If the opposite side of the wall is invisible,
						// don't draw it.

						if(temp->curdir == CHAR_U || temp->curdir == CHAR_D)
							{
							// Horizontal
							for(xck=0;xck<temp->mw;xck++)
								if(vx+xck>=0 && vy>=0)
									if(vismap[vx+xck+1][vy+1]&4)
										{
										ok=0; // Don't draw
										break;
										}
							}
						else
							{
							// Vertical
							for(xck=0;xck<temp->mh;xck++)
								if(vy+xck>=0 && vx>=0)
									if(vismap[vx+1][vy+xck+1]&4)
										{
										ok=0; // Don't draw
										break;
										}
							}
						}

				if(ok || temp->form->flags & SEQFLAG_WALL) // walls sprites
					{
					if(!(temp->flags & (IS_INVISIBLE|IS_SEMIVISIBLE)) || show_invisible)
						{
						cx=vx+x;
						cy=vy+y;

						// Animate the object, continously (or once, if stepped)

						if(temp->form->flags&SEQFLAG_STEPPED)
							{
							if(temp->flags & DID_STEPUPDATE)              // if updating
								{
								animate[temp->form->flags&SEQFLAG_ANIMCODE](temp);
								temp->flags &= ~DID_STEPUPDATE;           // Clear flag
								}
							}
						else
							{
							// Do the animation always, or when the timer says so
							if(temp->form->flags&SEQFLAG_ASYNC)
								{
								if(!temp->form->speed || !(masterclock % temp->form->speed))
									animate[temp->form->flags&SEQFLAG_ANIMCODE](temp);
								}
							else
								{
								temp->stats->tick++;
								if(temp->stats->tick>=temp->form->speed)
									{
									animate[temp->form->flags&SEQFLAG_ANIMCODE](temp);
									temp->stats->tick=0;
									}
								}
							}
						}

					// Draw people later (helps with large NPCs, horses etc)
					if(temp->flags & IS_PERSON)
						{
						if(people_ovl<MAX_P_OVERLAY)
							{
							people[people_ovl].x=x;
							people[people_ovl].y=y;
							people[people_ovl].cx=cx;
							people[people_ovl].cy=cy;
							people[people_ovl++].obj=temp;
							}
						}
					else
						Project_sprite(temp,cx,cy,x,y);	// Draw the sprite now

					// You may have noticed that CX and CY aren't always set
					// This doesn't really matter since if the object is not
					// visible, they won't be used anyway
						
					}
				}
			}
		}
	}

// Normalise drawing mode, may have changed after effects
//drawing_mode(DRAW_MODE_SOLID,NULL,0,0);
gamewin->SetBrushmode(ALPHA_SOLID,255);

// Now, display any post-processed stuff

// People

for(ctr=0;ctr<people_ovl;ctr++)
	Project_sprite(people[ctr].obj,people[ctr].cx,people[ctr].cy,people[ctr].x,people[ctr].y);


// Effects
for(ctr=0;ctr<postfx;ctr++)
	{
	person=current_object=postfx_buf[ctr];
	CallVMnum(-current_object->user->fx_func);
	}
// Normalise drawing mode again
gamewin->SetBrushmode(ALPHA_SOLID,255);

// And post-overlays
for(ctr=0;ctr<post_ovl;ctr++)
	postoverlay[ctr].s->Draw(gamewin,postoverlay[ctr].x,postoverlay[ctr].y);

// Restore Current and Me, which may have been changed by the effects code
current_object = oldcurrent;
person = oldme;
}

/*
 *		Do the actual sprite projection
 */

void Project_sprite(OBJECT *temp, int cx, int cy, int x, int y)
{
SEQ_POOL *project;

// Now we plot the object with the CLIP method
// <<5 multiplies by 32, converting the map coordinates into pixels.

if(!(temp->flags & (IS_INVISIBLE|IS_SEMIVISIBLE)) || show_invisible)
	{
	// Project the sprite
	project = temp->form;
	if(temp->flags & IS_TRANSLUCENT)
		{
		project->seq[temp->sptr]->image->DrawAlpha(gamewin,((cx-x)<<5),((cy-y)<<5),project->translucency);
		}
	else
		if(temp->flags & IS_SHADOW && !show_invisible)
			{
			project->seq[temp->sptr]->image->DrawShadow(gamewin,((cx-x)<<5),((cy-y)<<5),48);
			}
		else
			{
	#ifdef SHOW_GHOSTLY
			project->seq[temp->sptr]->image->DrawAlpha(gamewin,((cx-x)<<5),((cy-y)<<5),64);
	#else
			project->seq[temp->sptr]->image->Draw(gamewin,((cx-x)<<5),((cy-y)<<5));
	#endif
			}

	// If there is an overlay sprite, find out if it is for now
	// or for later.  If it's for now, display it, else queue it

	if(temp->form->overlay)
		{
		if(temp->form->flags&SEQFLAG_POSTOVERLAY)
			{
			if(post_ovl<MAX_P_OVERLAY)
				{
				postoverlay[post_ovl].x=(cx-x)<<5;
				postoverlay[post_ovl].y=(cy-y)<<5;
				postoverlay[post_ovl++].s=temp->form->overlay->image;
				}
			}
		else
			project->overlay->image->Draw(gamewin,((cx-x)<<5)+project->ox,((cy-y)<<5)+project->oy);
		}
	}
//else	// Rooftop objects are invisible
	{
	if(temp->form->flags&SEQFLAG_CHIMNEY) // It's a rooftop object
		if(roofspr<MAX_P_OVERLAY)
			roofsprites[roofspr++]=temp;
	}

// Special effects don't matter if you're visible or not
if(temp->user)
	{
	if(temp->user->fx_func != 0)
		{
		if(temp->user->fx_func > 0)
			{
			person=current_object=temp;
			CallVMnum(temp->user->fx_func);
			}
		else
			{
			if(postfx<MAX_P_OVERLAY)
				postfx_buf[postfx++]=temp;
			}
		}
	}
}



/*
 *      Project_roof - Display the roof objects on the map
 */

void Project_Roof(int x, int y)
{
int xctr=0,yctr,ptr,bit,ctr,ctr2;
OBJECT *temp;

xctr=0;
yctr=0;

// This must be black for framebuffer overlaying later
//clear_to_color(roofwin,ire_black);
roofwin->Clear(ire_black);

ptr=y*curmap->w;
ptr+=x;

bit=curmap->w - VSW;
xctr=0;
for(ctr=0;ctr<VSH;ctr++)
	{
	for(ctr2=0;ctr2<VSW;ctr2++)
		{
		if(curmap->roof[ptr])
			if(curmap->roof[ptr]<RTtot)
				{
//				draw_rle_sprite(roofwin,RTlist[curmap->roof[ptr]].image,xctr,yctr);
				RTlist[curmap->roof[ptr]].image->Draw(roofwin,xctr,yctr);
				}
		xctr+=32;
		ptr++;
		}
	xctr=0;
	yctr+=32;
	ptr+=bit;
	}

// Now, project the rooftop sprites (chimney etc)

for(ctr=0;ctr<roofspr;ctr++)
	{
	temp = roofsprites[ctr];
//	draw_rle_sprite(roofwin,temp->form->seq[0]->image,(temp->x-x)<<5,(temp->y-y)<<5);
	temp->form->seq[0]->image->Draw(roofwin,(temp->x-x)<<5,(temp->y-y)<<5);
	}
}

//
//  Project a scaled map
//

void MicroMap(int x, int y, int divisor, int roof)
{
int w,h,w2,h2,x2,y2;

w=VSW*32;
h=VSH*32;
w2=w/divisor;
h2=h/divisor;

gamewin->Clear(ire_black);
roofwin->Clear(ire_black);
gamewin->Clear(new IRECOLOUR(128,0,255));
roofwin->Clear(new IRECOLOUR(128,0,255));
for(y2=0;y2<divisor;y2++)
	for(x2=0;x2<divisor;x2++) {
		if(x+(x2*VSW) > 0)
			if(y+(y2*VSH) > 0)
				if(x+(x2*VSW)+VSW+4 < curmap->w)
					if(y+(y2*VSH)+VSH+4 < curmap->h) {
						// Rebuild the map details
						mapx=x+(x2*VSW);
						mapy=y+(y2*VSH);
						gen_largemap();

						Project_Map(x+(x2*VSW),y+(y2*VSH));
						Project_Sprites(x+(x2*VSW),y+(y2*VSH));
						if(roof) {
							Project_Roof(x+(x2*VSW),y+(y2*VSH));
							gamewin->Merge(roofwin);
						}
						gamewin->DrawStretch(scalebmp,0,0,w,h,w2*x2,h2*y2,w2,h2);
					}
	}

CentreMap(player);
gen_largemap();

scalebmp->Draw(gamewin,0,0);
}

/*
 *      MicroMap - Display the map on screen
 */

/*
void MicroMap1(int x,int y)
{
int xctr=0;
int yctr=0,ptr,bit,t,ctr,ctr2,w,h;

yctr=0;
xctr=0;
ptr=y*curmap->w;
ptr+=x;
bit=curmap->w - VSW;

w=VSW*4;
h=VSH*4;

for(ctr=0;ctr<h;ctr++)
	{
	for(ctr2=0;ctr2<w;ctr2++)
		{
		t = curmap->physmap[MAP_POS(x+ctr2,y+ctr)];
		draw_rle_sprite(scalebmp,TIlist[t].form->seq[0]->image,0,0);
		stretch_blit(scalebmp,gamewin,0,0,32,32,xctr,yctr,8,8);
		xctr+=8;
		}
	xctr=0;
	yctr+=8;
	ptr+=bit;
	}
}
*/

//////////////

/*
 *      Hide_Unseen - Remove rooms you can't see from the view (like U6)
 *                    Map - Do the calculations
 */

void Hide_Unseen_Map()
{
int ctr,ctr2;
int xoff,yoff,xmax,ymax;
OBJECT *o;

xoff = mapx-1;
yoff = mapy-1;
xmax = VSW;
ymax = VSH;

if(player) // If there isn't a player we want to keep the old visibility map
	{
	// First, clear the visibility map (all blank)
	memset(&vismap[0][0],0,(sizeof(char)*(LMSIZE*LMSIZE)));

	// Mark edges as lit
	for(ctr=VSH+2;ctr>0;ctr--)
		vismap[0][ctr]=1;
	for(ctr=VSW+2;ctr>0;ctr--)
		vismap[ctr][0]=1;

	// Now find the tiles that block light

	for(ctr=0;ctr<=ymax;ctr++)
		for(ctr2=0;ctr2<=xmax;ctr2++)
			blmap[ctr2][ctr] = BlocksLight(mapx+ctr2,mapy+ctr);

	// Check for windows
	// North
	o=GetSolidObject(player->x,player->y-1);
	if(o)
		if(o->flags & IS_WINDOW)
			vis_punch(player->x-mapx,(player->y-mapy)-1);

	// West
	o=GetSolidObject(player->x-1,player->y);
	if(o)
		if(o->flags & IS_WINDOW)
			vis_punch((player->x-mapx)-1,player->y-mapy);

	// East
	o=GetSolidObject(player->x+1,player->y);
	if(o)
		if(o->flags & IS_WINDOW)
			vis_punch((player->x-mapx)+1,player->y-mapy);

	// South
	o=GetSolidObject(player->x,player->y+1);
	if(o)
		if(o->flags & IS_WINDOW)
			vis_punch(player->x-mapx,(player->y-mapy)+1);

	// General floodfill
	vis_floodfill(player->x-mapx,player->y-mapy);
	}
}

/*
 *      Hide_Unseen - Remove rooms you can't see from the view (like U6)
 *                    Project - Do the actual blanking
 */

void Hide_Unseen_Project()
{
int xctr,yctr,ctr,ctr2,xmax,ymax;

// Block out what we don't want to see

xmax = VSW;
ymax = VSH;

yctr=0;
xctr=0;
for(ctr=0;ctr<ymax;ctr++)
	{
	for(ctr2=0;ctr2<xmax;ctr2++)
		{
		if(!(vismap[ctr2+1][ctr+1]&1))
			gamewin->FillRect(xctr,yctr,31,31,&blanker);
		xctr+=32;
		}
	xctr=0;
	yctr+=32;
	}
}

int isBlanked(int x, int y)
{
int vx,vy;

vx = x-mapx;
vy = y-mapy;

if(vx<0 || vx>VSW)
	return 1;
if(vy<0 || vy>VSH)
	return 1;

if(!vismap[vx+1][vy+1]&1)
	return 1;

return 0;
}


void SetDarkness(int d)
{
darklevel=(unsigned char)d;
if(ire_bpp == 8 && darklevel>30)
	darklevel=30;
}

int GetDarkness()
{
return darklevel;
}



void init_lightmap()
{
int x,y;
char filename[1024];
IRECOLOUR rcol(0,0,0);
// Load in the light mask

if(!loadfile("sprites/lightmap.cel",filename))
	ithe_panic("Could not find lightmask '%s'",filename);
lightmap=load_lightmap(filename);

// Put the light mask where we can get_sprite it

darkmap->Clear(0);
lightmap->DrawSolid(darkmap,0,0);

// Find size of the light mask in tiles

lmw = lightmap->GetW()/32;
lmh = lightmap->GetH()/32;

// Correct the value (since it rounds down)

if((lmw * 32) < lightmap->GetW())
    lmw++;
if((lmh * 32) < lightmap->GetH())
    lmh++;

lmw2=lmw/2;
lmh2=lmh/2;

// Break the lightmap into tiles

for(x=0;x<=lmw;x++)
    for(y=0;y<=lmh;y++)
        {
        lm[x][y]=MakeIRELIGHTMAP(32,32);
	if(!lm[x][y])
		ithe_panic("Could not create lightmap tile","Out of memory?");
	
	lm[x][y]->Get(darkmap,x<<5,y<<5);
/*	
        for(cx=0;cx<32;cx++)
            for(cy=0;cy<32;cy++)
                {
			darkmap->GetPixel(cx+(x<<5),cy+(y<<5),&rcol);
			lm[x][y]->PutPixel(cx,cy,&rcol);
                }
*/                
        }
}

/*
 *      render the lighting
 */

void render_lighting(int x, int y)
{
int cx,cy,ex,ey,ctr,nx,ny;
int startx,starty;
OBJECT *temp;
LTAB light[256];
int lights;
unsigned char *lptr;
int offset,id;
int darkness=0;

darkness = darklevel + dark_mix;
if(darkness>255)
	darkness=255;

darkmap->Clear(darkness);
memset(&blmap[0][0],0,(sizeof(char)*(LMSIZE*LMSIZE)));

// Find all lightsources

lights=0;

basex=x;
basey=y;

ey = y+VSH+3;
ex = x+VSW+3;
startx = x-3;
starty = y-3;

if(startx<0) startx=0;
if(starty<0) starty=0;
if(ex>=curmap->w)
    ex=curmap->w;
if(ey>=curmap->h)
    ey=curmap->h;

for(cy=starty;cy<ey;cy++)
for(cx=startx;cx<ex;cx++)
    {
    temp = GetRawObjectBase(cx,cy);
    for(;temp;temp=temp->next)              // Traverse the list
        if(temp->flags & IS_ON && lights<255)
            if(temp->light)
                {
                // if the object is in the screen region
                if((cx+lmw >= x) && (cx-lmw < ex))
                    if((cy+lmh >= y) && (cy <= ey))
                        {
                        nx = cx-x;
                        ny = cy-y;
                        if(nx+lmw>0 && ny+lmh>0)
                            if(nx-lmw2<VSW && ny-lmh2<VSH)
                                {
                                light[lights].light=temp->light;
                                light[lights].x=nx;
                                light[lights].y=ny;
                                lights++;
                                }
                        }
                }
    }

// Ok, we're ready.  Preserve the default colourmap

// First project any static lighting

offset = curmap->w-VSW;
lptr = &curmap->lightst[(y*curmap->w)+x];

nx=0;ny=0;
for(cy=0;cy<VSH;cy++)
	{
	for(cx=0;cx<VSW;cx++)
		{
		if(*lptr++)
			{
			id=curmap->light[((cy+y)*curmap->w)+cx+x];
			if(id && id < LTtot)
				LTlist[id].image->DrawLight(darkmap,nx,ny);
			}
		nx+=32;
		}
	lptr+=offset;
	nx=0;
	ny+=32;
	}

// Project darksources first

for(ctr=0;ctr<lights;ctr++)
	if(light[ctr].light<0)
		proj_light(light[ctr].x,light[ctr].y,0);

// Then project lightsources

for(ctr=0;ctr<lights;ctr++)
	if(light[ctr].light>0)
		proj_light(light[ctr].x,light[ctr].y,1);

// Now use the darkness map to shade the game window
darkmap->Render(gamewin);
// And the roof (which is rendered separately to avoid contamination)
roofwin->Darken(darkness);
}

/*
 *      Project a single light source
 */

void proj_light(int x, int y, int light)
{
int xx,yy,cx,cy,ctr,lx,ly;
char dlm[8][8];
int xoffset,yoffset;

#define STARTX 0

xoffset = STARTX;//+(1-rnd(2));
yoffset = STARTX;//+(1-rnd(2));

// Light central hub square
for(yy=0;yy<3;yy++)
	for(xx=0;xx<3;xx++)
		{
		lx=((x+xx-1)<<5)+xoffset;
		ly=((y+yy-1)<<5)+yoffset;
		if(light)
			lm[xx+1][yy+1]->DrawLight(darkmap,lx,ly);
		else
			lm[xx+1][yy+1]->DrawDark(darkmap,lx,ly);
		dlm[xx+1][yy+1]=1;
		}

// Clear the already-lit array
memset(dlm,0,64);

cx = x+basex;
cy = y+basey;

for(ctr=0;ctr<8;ctr++)
	{
	// Store for faster access
	xx = (node[ctr][0][0]);
	yy = (node[ctr][0][1]);

	if(!BlocksLight(cx+xx,cy+yy))
		{
		xx = (node[ctr][1][0])+2;
		yy = (node[ctr][1][1])+2;
	
		if(!dlm[xx][yy])
			{
			lx=((x+xx-2)<<5)+xoffset;
			ly=((y+yy-2)<<5)+yoffset;
			if(light)
				lm[xx][yy]->DrawLight(darkmap,lx,ly);
			else
				lm[xx][yy]->DrawDark(darkmap,lx,ly);
			dlm[xx][yy]=1;
			}

		xx = (node[ctr][2][0])+2;
		yy = (node[ctr][2][1])+2;
		if(!dlm[xx][yy])
			{
			lx=((x+xx-2)<<5)+xoffset;
			ly=((y+yy-2)<<5)+yoffset;
			if(light)
				lm[xx][yy]->DrawLight(darkmap,lx,ly);
			else
				lm[xx][yy]->DrawDark(darkmap,lx,ly);
			dlm[xx][yy]=1;
			}

		xx = (node[ctr][3][0])+2;
		yy = (node[ctr][3][1])+2;
		if(!dlm[xx][yy])
			{
			lx=((x+xx-2)<<5)+xoffset;
			ly=((y+yy-2)<<5)+yoffset;
			if(light)
				lm[xx][yy]->DrawLight(darkmap,lx,ly);
			else
				lm[xx][yy]->DrawDark(darkmap,lx,ly);
			dlm[xx][yy]=1;
			}
		}
	}
}



/*
 *  Blank rooms the player can't see
 */

void vis_floodfill(int x, int y)
{
// Check bounds

if(x<0 || x>VSW)
	return;
if(y<0 || y>VSH)
	return;

// Already done?
if(vismap[x+1][y+1])
	{
	vismap[x+1][y+1]=3;
	return;
	}

// If it blocks light, stop right there
if(blmap[x][y])
	{
	vismap[x+1][y+1]=3; // Mark as done, and lit (was 2, walls were 'dark')
	return;
	}
else
	vismap[x+1][y+1]=3;  // was 1

vis_floodfill(x-1,y-1);
vis_floodfill(x,y-1);
vis_floodfill(x+1,y-1);

vis_floodfill(x-1,y);
vis_floodfill(x+1,y);

vis_floodfill(x-1,y+1);
vis_floodfill(x,y+1);
vis_floodfill(x+1,y+1);
}

/*
 *  Punch a hole in the darkness
 */

void vis_punch(int x, int y)
{
if(x<0 || x>VSW)
	return;
if(y<0 || y>VSH)
	return;

vismap[x+1][y+1]=1;

vis_floodfill(x-1,y-1);
vis_floodfill(x,y-1);
vis_floodfill(x+1,y-1);

vis_floodfill(x-1,y);
vis_floodfill(x+1,y);

vis_floodfill(x-1,y+1);
vis_floodfill(x,y+1);
vis_floodfill(x+1,y+1);
}

//
//  General-purpose fast random function
//

int fastrandom()
{
static int frndpos=0;
static int findex=0;
frndpos++;
if(frndpos > 511)
	{
	frndpos=0;
	findex++;
	if(findex > 255)
		findex=0;
	}
return rndtab[findex][frndpos];
}
