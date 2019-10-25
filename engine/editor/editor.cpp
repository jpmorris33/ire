/*
        IRE world editor

Copyright (c) 2010, IT-HE Software
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

Neither the name of IT-HE Software nor the names of its contributors may
be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.
IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


//#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __DJGPP__
#include <conio.h>
#include <dos.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif

#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "../core.hpp"     // Main data structures
#include "../ithelib.h"    // Support lib
#include "../media.hpp"      // Graphics etc
#include "../init.hpp"       // Init module
#include "../linklist.hpp" // Linked list alloc and free routines
#include "../oscli.hpp"      // Operating system command line interpreter
#include "../console.hpp"    // Text output and logger routines
#include "../cookies.h"    // Signatures for the Z1/Z2 map files
#include "../resource.hpp"   // The script file loader and search routines
#include "../loadsave.hpp" // Map IO routines
#include "../loadfile.hpp"   // Find correct file to open
#include "../sound.h"      // Include the SMTab data structure
#include "../gamedata.hpp"   // Shared data
#include "../pe/pe_api.hpp"
#include "../database.hpp"
#include "igui.hpp"

#include "../svnrevision.h"        // If this is missing, create a blank file

// Fake the SVN revision if we don't have one
#ifndef SVNREVISION
#define SVNREVISION ""
#endif



#ifdef __DJGPP__
#include <crt0.h>
int _crt0_startup_flags =_CRT0_FLAG_FILL_SBRK_MEMORY | _CRT0_FLAG_FILL_DEADBEEF;
#endif


// Defines

#undef VIEWDIST
#define VIEWDIST -8 // was -4
#define MAX_P_OVERLAY 255       // Maximum number of overlayed objects
                                // Per screen.  Somewhat unlikely..

// Variables

void (*EdUpdateFunc)(void);

int MapXpos_Id,MapYpos_Id;
int focus=666; // Default button in focus

char mapxstr[]="X: 0    ";      // Room for 5 digit number
char mapystr[]="Y: 0    ";

int AB_Id,BG_Id,OB_Id,TL_Id,BM_Id,LT_Id;

IRELIGHTMAP *lightsmap,*darkmap;
IREBITMAP *lightsbuf;
IREBITMAP *lighticon;
int lightdepth=128,preview_light=0;
unsigned char editor_tip[65535];        // Tile Animation Pointer
char imgcachedir[1024];

//short bgp_size=0;
//SPRITE *bgpool;
//BITMAP *transfer;
IREBITMAP *eyesore;
IREBITMAP *warning;
IREBITMAP *gamewin;
//IREFONT *DefaultFont;

OBJECT *objsel;         // Object selected in objects.cc
OBJECT *schsel=NULL;    // Special highlight for Schedules
OBJECT *cutbag;

//SMTab *mustab,*wavtab;  // More dummies

extern char in_editor;

int s_proj=1;           // Project Sprites flag
int l_proj=0;           // Project Layers flag (OFF BY DEFAULT)
int sx_proj=1;          // Exclude other Sprites flag (1 = OFF)
int lx_proj=1;          // Exclude other Layer objects flag (1 = OFF)
int ow_proj=0;          // Project Owned objects? (0 = OFF)
int st_proj=0;          // Project Standunder objects? (0 = OFF)

/*
char __up[]={22,0};     // The arrow icons (in the font)
char __down[]={23,0};
char __left[]={20,0};
char __left2[]={20,20,0};
char __right[]={21,0};
char __right2[]={21,21,0};
*/

char __up[]="^";     // The arrow icons (in the font)
char __down[]="v";
char __left[]={"<"};
char __left2[]={"<<"};
char __right[]={">"};
char __right2[]={">>"};

// Functions

void LoadMap(int number);
void SaveMap();
void Toolbar();
void SaveThings();
void RedrawWindow();

void Eproject_sprites();    			// Project the sprites onto the map
void Eproject_roof(int x,int y);        // Project the rooftop objects
void Eproject_light(int x,int y);       // Project static lightmap
void Eproject_dark(int x,int y);		// Preview static lightmap

extern void Project_Sprites(int x, int y);


void Quit();
void Snap();
extern void AB_GoFocal();       // About box handle
extern void BG_GoFocal();       // Background handle
extern void OB_GoFocal();       // Objects
extern void TL_GoFocal();       // Roofs
extern void BM_GoFocal();       // World view (big map)
extern void LT_GoFocal();       // Static Lights
extern void gen_largemap();     // Build the large-objects map
extern void LoadResources(char *filename);
extern void EditArea(char *object);

// Code

/*
 *      Main - start up all the things, run them, stop all the things, exit
 */

int main(int argc, char *argv[])
{
const char *backend=NULL;
int ctr,ret;
char fname[1024];
char homepath[1024];

strcpy(editarea,""); // Don't go into edit_area mode by default

backend=IRE_InitBackend();      // We need this ASAP
//allegro_init();

in_editor = 1;    // Some init functions are not necessary for the editor

curmap=&worldcache[0];

#ifdef FORTIFY
remove("fortify.out");
#endif

projectname[0]=0;  // Blank the project title
projectdir[0]=0;  // Blank the project directory


M_init();         // Init memory systems

ihome(homepath);
#ifdef __SOMEUNIX__
	sprintf(newbootlog,"%s/%s",homepath,"bootlog.txt"); 
	sprintf(oldbootlog,"%s/%s",homepath,"bootlog.prv"); 
#else
	sprintf(newbootlog,"%s\\bootlog.txt",homepath); // This could be changed later in OSCLI
	sprintf(oldbootlog,"%\\bootlog.prv",homepath); // If we need it
	//strcpy(homepath,"");
#endif

ilog_start(newbootlog,oldbootlog);

// Init the compiler core so we can define constants in the ini files
compiler_init();

// Now get the config settings

if(argc>=2)
	OSCLI(argc,argv);       // input from OSCLI


if(!TryINI("editor.ini"))
	if(!TryINI("game.ini"))
		ithe_panic("Could not open editor.ini or game.ini",NULL);

/*
// Look for an editor file first
sprintf(fname,"%s%s",homepath,"editor.ini");
if(!INI_file(fname))
	INI_file("editor.ini"); // Okay, try current directory then

// Okay, let's look for the user config file
sprintf(fname,"%s%s",homepath,"game.ini");
if(!INI_file(fname))
	INI_file("game.ini"); // Okay, try current directory then
*/

// Now we need to find the main .ini file to tell us what to do.

// Do we have a project directory, passed from the commandline?
if(!projectdir[0])
	{
	// No, assume it's the current directory then
	strcpy(projectdir,"./");
	}

sprintf(fname,"%sgamedata.ini",projectdir);
ret=INI_file(fname);
if(!ret)
	ret=INI_file("gamedata.ini");

// Fix editor window size to 8,8
VSW=8;
VSH=8;

/*
if(!ret)
	{
	init_media(0);
	if(!data_choose(projectdir,fname,sizeof(fname)))
		{
		term_media();
		exit(1);
		}
	sprintf(fname,"%sgamedata.ini",projectdir);
	ret=INI_file(fname);
		if(!ret)
			{
			term_media();
			printf("Could not load file '%s'\n",fname);
			exit(1);
			}
	}
*/

// Final checks
if(!projectname[0])
	ithe_safepanic("No project was specified.","There should be a -project <projectname> line in one of the INI files.");

// Use the project directory for all file accesses
ifile_prefix=&projectdir[0];


ilog_printf("IRE Editor, Copyright (C) 2019 IT-HE Software\n");
ilog_printf("=============================================\n");
ilog_printf("\n");
ilog_printf("IRE Kernel version %s%s\n",IKV,SVNREVISION);
ilog_printf("\n");
ilog_printf("Build date: %s at %s\n",__DATE__,__TIME__);
ilog_printf("IT-HE lib version: %s\n",ithe_get_version());
ilog_printf("%s\n",backend);
ilog_printf("\n");
ilog_printf("Loading project: %s\n",projectname);
ilog_printf("\n");

#ifndef _WIN32
if(!editarea[0]) // Area Editor wants fast startup
	sleep(1);
#endif

ilog_quiet("objsize = %d bytes\n",sizeof(OBJECT));

party = (OBJECT **)M_get(MAX_MEMBERS,sizeof(OBJECT *));
partyname = (char **)M_get(MAX_MEMBERS,sizeof(char *));

// Cache these linked lists to speed up loading levels
LLcache_register(&MasterList);
LLcache_register(&ActiveList);

// Check the mapfile is there before we commit ourselves

if(!mapnumber)
	ithe_safepanic("Please specify the number of the map you want to edit.","E.G.  ed -map 1");

init_media(0); // Enter graphics mode, not for game
init_enginemedia(); // Finish graphics setup for editor

// Work out the name of the image cache directory
sprintf(imgcachedir,"cache_%s_%d",projectname,ire_bpp);
char temp[1024];
#ifdef _WIN32
	sprintf(temp,"%s\\irecache.sdb",homepath);
#else
	sprintf(temp,"%s/irecache.sdb",homepath);
#endif
if(InitSQL(temp) != 0)
	ithe_safepanic("Could not create cache database at",temp);

CheckCacheTable();
CreateCacheTable();

//DefaultFont = MakeIREFONT();	// Create the system font

gamewin = MakeIREBITMAP(256,256);
if(!gamewin)
      ithe_panic("Could not create game window",NULL);

lightsmap = MakeIRELIGHTMAP(256,256);
if(!lightsmap)
      ithe_panic("Could not create light window",NULL);

lightsbuf = MakeIREBITMAP(256,256);
if(!lightsbuf)
      ithe_panic("Could not create light window",NULL);

darkmap = MakeIRELIGHTMAP(256,256);
if(!darkmap)
	ithe_panic("Create darkmap failed",NULL);

lighticon = MakeIREBITMAP(32,32);
if(!lighticon)
      ithe_panic("Could not create light icon",NULL);

if(!loadfile("eyesore.cel",fname))
      ithe_panic("Could not find file","eyesore.cel");
eyesore = iload_bitmap(fname);
if(!eyesore)
      ithe_panic("Could not load file","eyesore.cel");

if(!loadfile("warning.cel",fname))
      ithe_panic("Could not find file","warning.cel");
warning = iload_bitmap(fname);
if(!warning)
      ithe_panic("Could not load file","warning.cel");

ilog_printf("Console started\n");

if(!fileexists("resource.txt"))
	{
	ithe_panic("Resource file 'resource.txt' not found..","Run the editor in the directory containing your .map files");
	}

ilog_printf("Load Resources\n");
LoadResources("resource.txt");

if(editarea[0])
	{
	EditArea(editarea);
	exit(1);
	}

// Make sure there are some tiles
if(TItot<1)
    ithe_panic("No tiles were defined","Check 'section: tiles' in resource.txt");

ilog_printf("VRM fixups\n");
Init_Funcs();
// We aren't going to USE them, but we need to set up the function caches,
// obj->funcs->ucache etc.

for(ctr=0;ctr<MAX_MEMBERS;ctr++)
    partyname[ctr]=(char *)M_get(1,128);        // Might be necessary

ilog_printf("Load Map\n");
mapnumber = default_mapnumber;
if(!mapnumber)
	mapnumber=1;  // Default to map 1

// Load in the game map
LoadMap(mapnumber);

cutbag = &limbo;
cutbag->name="Cut and Paste buffer";
cutbag->flags |= IS_ON;

ilog_printf("Init Random Number Generator\n");
srand(time(0));                 // Initialise rng

irecon_term();	// Don't want the console anymore, it will mess the display

// Editor routines start here

ilog_break=0; // Disable break now we're ready

IG_Init(128);                   // Set up the iGUI for max 128 buttons

Toolbar();                      // Draw the menu bar at the top
                                // And then start the iGUI event loop

swapscreen->HideMouse();
AB_GoFocal();                   // Start on the ABOUT screen
swapscreen->ShowMouse();
//AB_GoFocal();                   // Start on the ABOUT screen

EdUpdateFunc = NULL;
do
	{
	IG_Dispatch();
	if(EdUpdateFunc)
		EdUpdateFunc();

	// Make sure at least one frame has elapsed before continuing
	ResetIREClock(); // Reset the clock
	while(GetIREClock()<1)
		if(EdUpdateFunc)
			EdUpdateFunc();
	
	} while(running);       // Loop until quit

SaveMap();                      // Save the map
IG_Term();                      // Free the iGUI resources

// Shut down

TermSQL();
	
ilog_printf("Stop Editor\n");
term_media();

if(buggy)
    {
    printf("There were bugs!\n");
    printf("Check 'bootlog.txt' and search for 'BUG:'\n");
    }

printf("Thankyou for using the IRE World Editor.\n");

return 0;			// Return a value to keep compiler happy
}
// END_OF_MAIN();

/*
 *      Toolbar - set up the GUI components that are present in all cases
 */

void Toolbar()
{
SetColor(ITG_BLACK);
IG_KillAll();
IG_AddKey(IREKEY_ESC,Quit);           // Add key binding for ESC = Quit
IG_AddKey(IREKEY_F10,Snap);           // Add key binding for F10 = Screenshot
IG_Panel(0,0,640,32);

IG_Panel(0,32,640,480-32);
AB_Id = IG_Tab(540,4,"About",AB_GoFocal,NULL,NULL);
BM_Id = IG_Tab(4,4,"World view",BM_GoFocal,NULL,NULL);
BG_Id = IG_Tab(100,4,"Background",BG_GoFocal,NULL,NULL);
OB_Id = IG_Tab(196,4,"Sprites",OB_GoFocal,NULL,NULL);
TL_Id = IG_Tab(268,4,"Roof",TL_GoFocal,NULL,NULL);
LT_Id = IG_Tab(320,4,"Lights",LT_GoFocal,NULL,NULL);
IG_TextButton(594,4,"Quit",Quit,NULL,NULL);

// Hotkeys for the sections

IG_AddKey(IREKEY_1,BM_GoFocal);
IG_AddKey(IREKEY_2,BG_GoFocal);
IG_AddKey(IREKEY_3,OB_GoFocal);
IG_AddKey(IREKEY_4,TL_GoFocal);
IG_AddKey(IREKEY_5,LT_GoFocal);
IG_AddKey(IREKEY_0,AB_GoFocal);
IG_AddKey(IREKEY_F2,SaveThings);
}

/*
 *      Quit - The callback handler for Quitting
 */

void Quit()
{
if(Confirm(-1,-1,"Are you sure you want to quit?",NULL))
  running = 0;
}

/*
 *      Snap - The callback handler for taking a screen dump
 */

void Snap()
{
Screenshot(swapscreen);
}


/* ================================ Map RW =============================== */

/*
 *      DrawMap - Display the map on screen
 */

void DrawMap(int x,int y,char s,char l,char f)
{
int xctr=0,ctr,ctr2,t;
int yctr=0,ptr,bit;
int mapx,mapy,Animate;

xctr=0;
yctr=0;

mapy = y;
mapx = x;

ptr=mapy*curmap->w;
ptr+=mapx;


// Find out if we're on schedule to animate everything
Animate = UpdateAnim();

// Tile animation controller

// For each tile...
if(Animate)
    for(ctr=0;ctr<TItot;ctr++)
        // If it animates, update the tiles clock and frame when necessary
        if(TIlist[ctr].form->frames)
            {
            TIlist[ctr].tick++;
            if(TIlist[ctr].tick>=TIlist[ctr].form->speed)
                {
                editor_tip[ctr]+=TIlist[ctr].sdir;
                if(editor_tip[ctr]>=TIlist[ctr].form->frames)
                    editor_tip[ctr]=0;
                TIlist[ctr].tick=0;
                }
            }

bit=curmap->w - VSW;
xctr=0;

for(ctr=0;ctr<VSH;ctr++) {
	for(ctr2=0;ctr2<VSW;ctr2++) {
		t = curmap->physmap[ptr];
		if(t == RANDOM_TILE) {
			eyesore->Draw(gamewin,xctr,yctr);
		} else {
			if(t>=TItot) {
				warning->Draw(gamewin,xctr,yctr);
			} else {
				TIlist[t].form->seq[editor_tip[t]]->image->Draw(gamewin,xctr,yctr);
				if(TIlist[t].form->overlay) {
					TIlist[t].form->overlay->image->Draw(gamewin,xctr,yctr);
				}
			}
		}

		xctr+=32;
		ptr++;
	}
	xctr=0;
	yctr+=32;
	ptr+=bit;
}

//gen_largemap();

if(s)
          Eproject_sprites();

if(f)
          Eproject_light(x,y);
if(l)
          Eproject_roof(x,y);



gamewin->Draw(swapscreen,VIEWX,VIEWY);

sprintf(mapxstr,"X: %5d",mapx);
IG_UpdateText(MapXpos_Id,mapxstr);

sprintf(mapystr,"Y: %5d",mapy);
IG_UpdateText(MapYpos_Id,mapystr);
}

/*
 *      WriteMap - update a tile on the map
 */

void WriteMap(int x,int y,int tile)
{
int ptr;
ptr=y*curmap->w;
ptr+=x;
curmap->physmap[ptr]=tile;
}

/*
 *      ReadMap - get a tile from the map
 */

int ReadMap(int x,int y)
{
int ptr;
ptr=y*curmap->w;
ptr+=x;
return(curmap->physmap[ptr]);
}

/*
 *      WriteRoof - update a tile on the map
 */

void WriteRoof(int x,int y,int tile)
{
int ptr;
ptr=y*curmap->w;
ptr+=x;
curmap->roof[ptr]=tile;
}

/*
 *      ReadMap - get a tile from the map
 */

int ReadRoof(int x,int y)
{
int ptr;
ptr=y*curmap->w;
ptr+=x;
return(curmap->roof[ptr]);
}

/*
 *      WriteRoof - update a tile on the map
 */

void WriteLight(int x,int y,int tile)
{
int ptr;
ptr=y*curmap->w;
ptr+=x;
curmap->light[ptr]=tile;
}

/*
 *      ReadMap - get a tile from the map
 */

int ReadLight(int x,int y)
{
int ptr;
ptr=y*curmap->w;
ptr+=x;
return(curmap->light[ptr]);
}

/*
 *      Eproject_sprites- Display the sprites on the map
 */

void Eproject_sprites()
{
int x,y;
int cx,cy,vx,vy,yoff;
int startx,starty,ctr;
OBJECT *temp;
SEQ_POOL *project;
OBJECT *postoverlay[MAX_P_OVERLAY];
int post_ovl=0;

startx = VIEWDIST;
starty = VIEWDIST;

x=mapx;
y=mapy;

if((x+startx)<0)
	startx=0;
if((y+starty)<0)
	starty=0;

for(vy=starty;vy<VSH;vy++)
	{
	yoff=ytab[y+vy];
	for(vx=startx;vx<VSW;vx++)
		{
		for(temp=curmap->objmap[yoff+x+vx];temp;temp=temp->next)
			if(sx_proj || (objsel && !strcmp(temp->name,objsel->name)))
				{
				cx=vx+x;
				cy=vy+y;

				// Then we plot the object with the CLIP method
				// <<5 multiplies by 32, converting the map coordinates into pixels.

				// Project the sprite
				project = temp->form;
				if(temp->flags & IS_TRANSLUCENT)
					{
					project->seq[temp->sptr]->image->DrawAlpha(gamewin,((cx-x)<<5),((cy-y)<<5),project->translucency);
					}
				else
					if(temp->flags&IS_SHADOW && !(temp->flags&IS_INVISIBLE)) // Invisible is 'Hidden'
						{
						project->seq[temp->sptr]->image->DrawShadow(gamewin,((cx-x)<<5),((cy-y)<<5),48);
						}
					else
						project->seq[temp->sptr]->image->Draw(gamewin,((cx-x)<<5),((cy-y)<<5));

                // If there is an overlay sprite, find out if it is for now
                // or for later.  If it's for now, display it, else queue it

				if(temp->form->overlay)
					{
					if(temp->form->flags&64)
						{
						if(post_ovl<MAX_P_OVERLAY)
							postoverlay[post_ovl++]=temp;
						}
					else
						project->overlay->image->Draw(gamewin,((cx-x)<<5)+project->ox,((cy-y)<<5)+project->oy);
					}

				// If the object is the selected object, highlight it.
				if(objsel == temp)
					ClipBox((cx-mapx)<<5,(cy-mapy)<<5,temp->w-1,temp->h-1,ITG_WHITE,gamewin);

				// If the object is the selected object, show any radius
				if(objsel == temp)
					if(temp->stats->radius)
						ClipCircle(((cx-mapx)<<5)+16,((cy-mapy)<<5)+16,temp->stats->radius<<5,ITG_RED,gamewin);

				// If the object is the Owner of the current object, highlight in red.
				if(objsel)
					if(objsel->stats->owner.objptr == temp)
						ClipBox((cx-mapx)<<5,(cy-mapy)<<5,temp->w-1,temp->h-1,ITG_RED,gamewin);

				// If the object is the current Schedule Target, highlight in blue.
				if(schsel == temp)
					ClipBox((cx-mapx)<<5,(cy-mapy)<<5,temp->w-1,temp->h-1,ITG_BLUE,gamewin);

				// If we're showing all 'Owned' objects
				if(ow_proj && temp->stats->owner.objptr)
					{
					//set_invert_blender(128,128,255,255);
					//draw_trans_rle_sprite(gamewin,project->seq[temp->sptr]->image,((cx-x)<<5),((cy-y)<<5));
					project->seq[temp->sptr]->image->DrawInverted(gamewin,((cx-x)<<5),((cy-y)<<5));
					}
				}
		}
	}
// Now, display any post-processed overlays

for(ctr=0;ctr<post_ovl;ctr++)
	{
	temp = postoverlay[ctr];
	temp->form->overlay->image->Draw(gamewin,(temp->x-x)<<5,(temp->y-y)<<5);
	}

}

/*
 *      Eproject_roof- Display the roof objects on the map
 */

void Eproject_roof(int x,int y)
{
int xctr=0,ctr,ctr2;
int xctr2=0,ptr,bit;
int mapx,mapy;

xctr=0;
xctr2=0;

mapy = y;
mapx = x;

ptr=mapy*curmap->w;
ptr+=mapx;

bit=curmap->w - VSW;
xctr=0;
for(ctr=0;ctr<VSH;ctr++)
	{
	for(ctr2=0;ctr2<VSW;ctr2++)
		{
		if(curmap->roof[ptr])
			if(curmap->roof[ptr]<RTtot)
				{
				if(st_proj && !(RTlist[curmap->roof[ptr]].flags & KILLROOF))
					{
					RTlist[curmap->roof[ptr]].image->DrawAlpha(gamewin,xctr,xctr2,192);
					//textout_ex((BITMAP *)IRE_GetBitmap(gamewin),font,"S",xctr,xctr2,ITG_BLUE->packed,-1);
					DefaultFont->Draw(gamewin,xctr,xctr2,ITG_BLUE,NULL,"S");
					}
				else
					{
					RTlist[curmap->roof[ptr]].image->Draw(gamewin,xctr,xctr2);
					//draw_rle_sprite(gamewin,RTlist[curmap->roof[ptr]].image,xctr,xctr2);
					}

				}
		xctr+=32;
		ptr++;
		}
	xctr=0;
	xctr2+=32;
	ptr+=bit;
	}

return;
}

/*
 *      Eproject_light - Display the static lightmap objects on the map
 */

void Eproject_light(int x,int y)
{
int xctr=0,ctr,ctr2;
int xctr2=0,ptr,bit;
int mapx,mapy;

if(preview_light)
	{
	Eproject_dark(x,y);
	return;
	}

lightsbuf->Clear(ire_transparent);
lightsmap->Clear(0);

xctr=0;
xctr2=0;

mapy = y;
mapx = x;

ptr=mapy*curmap->w;
ptr+=mapx;

bit=curmap->w - VSW;
xctr=0;
for(ctr=0;ctr<VSH;ctr++)
	{
	for(ctr2=0;ctr2<VSW;ctr2++)
		{
		if(curmap->light[ptr])
			if(curmap->light[ptr]<LTtot)
				LTlist[curmap->light[ptr]].image->DrawSolid(lightsmap,xctr,xctr2);
		xctr+=32;
		ptr++;
		}
	xctr=0;
	xctr2+=32;
	ptr+=bit;
	}

lightsmap->RenderRaw(lightsbuf);
lightsbuf->DrawAlpha(gamewin,0,0,lightdepth);
return;
}

// Preview static lighting

void Eproject_dark(int x,int y)
{
int cx,cy,nx,ny,id;

darkmap->Clear(125);

nx=0;ny=0;
for(cy=0;cy<VSH;cy++)
	{
	for(cx=0;cx<VSW;cx++)
		{
		id=curmap->light[((cy+y)*curmap->w)+cx+x];
		if(id)
			LTlist[id].image->DrawLight(darkmap,nx,ny);
		nx+=32;
		}
	nx=0;
	ny+=32;
	}

darkmap->Render(gamewin);
}

/*
 *      SaveMap- Write the map to disk, calls functions in loadsave.cc
 */

void SaveMap()
{
char *ptr,*ptr2;
int ret;

ret=0;
ret=InputIntegerValueP(-1,-1,1,9999,mapnumber,"Save as map number ");
if(ret<0)
	return;

mapnumber=ret;

//	Write tilemap (uses mapname)
save_map(mapnumber);

//	Write Object positions (needs explicit name)
ptr=makemapname(mapnumber,0,".mz1");
ptr2=makemapname(mapnumber,0,".bak");
if(ptr)
	{
	// Back up original first
	if(ptr2)
		{
		unlink(ptr2); // Remove existing backup
		rename(ptr,ptr2); // rename original to backup
		}
	MZ1_SavingGame=0; // Don't store every tiny detail
	save_z1(ptr);
	}

//	Write Roofmap (uses mapname)
save_z2(mapnumber);

//	Write Lightmap (uses mapname)
save_z3(mapnumber);
}

void SaveThings()
{
SaveMap();
RedrawWindow();
}

void RedrawWindow()
{
int r = focus;
focus = 666;

switch (r)
	{
	case 0:
	AB_GoFocal();
	break;

	case 1:
	BG_GoFocal();
	break;

	case 2:
	OB_GoFocal();
	break;

	case 3:
	TL_GoFocal();
	break;

	case 4:
	BM_GoFocal();
	break;
	};
}

void RFS_getescape()
{
while(IRE_NextKey(NULL) != IREKEY_ESC);
}


int GetClickXY(int mx, int my, int *xout, int *yout)
{
int t,pos,gotx,goty;

*xout=*yout=0;
gotx=goty=-1;

for(pos=0;pos<VSW;pos++) {
	t=32*pos+(VIEWX-1);
	if(mx>=t && mx<(t+32)) {
		gotx=pos;
		break;
	}
}

for(pos=0;pos<VSH;pos++) {
	t=32*pos+(VIEWY-1);
	if(my>=t && my<(t+32)) {
		goty=pos;
		break;
	}
}

if(gotx!=-1 && goty!=-1) {
	*xout = mapx+gotx;
	*yout = mapy+goty;
	return 1;
}

return 0;
}
