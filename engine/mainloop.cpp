/*
 *      IRE game runner main module
 */

// Debugging switches

//#define NO_ANIMATION  // No animation (helps profiling)
//#define DEBUG_SCHEDULE
//#define DEBUG_RESUMEACTIVITY
//#define DEBUG_LARGE_OBJECTS
//#define DEBUG_SOLID_OBJECTS
//#define DEBUG_BLOCKSLIGHT
//#define CHECK_ACTIVELIST
//#define CHECK_MASTERLIST
//#define WIELD_PARANOID	// If objects being wielded disappear, this may help

#include <stdlib.h>
#include <sys/types.h>

#if !defined(__BEOS__) && !defined(_WIN32)
#include <sys/mman.h>
#endif
#include <string.h>
#include <time.h>

#ifdef _WIN32
//#define ALLEGRO_USE_CONSOLE
#include <process.h>
#endif

#include "ithelib.h"
#include "media.hpp"
#include "core.hpp"
#include "gamedata.hpp"
#include "resource.hpp"
#include "init.hpp"
#include "oscli.hpp"
#include "console.hpp"
#include "linklist.hpp"
#include "object.hpp"
#include "library.hpp"
#include "loadsave.hpp"
#include "project.hpp"
#include "mouse.hpp"
#include "sound.h"
#include "nuspeech.hpp"
#include "map.hpp"
#include "pe/pe_api.hpp"
#include "loadfile.hpp"
#include "widgets.hpp"

// defines

#define VIEWX 16
#define VIEWY 16
#define CURSOR_CHAR "_"

// variables


extern char *darkmap,*darkrect;
extern VMINT dark_mix;
extern char AL_dirty;
extern int destroyhp;
VMINT do_lightning=0;
VMINT do_earthquake=0;
char user_input[128];

unsigned long fpsmetric=0;
time_t startmetric, stopmetric;
double timemetric;

char *savegame;

// functions

void DrawMap(int x,int y);              // Draw the map and all the things
extern void p_light(char *screen);
extern char *BestName(OBJECT *o);

extern void Call_VRM(int v);
extern char DebugVM;
int getYN(char *q);
int get_key();
int get_key_debounced();
int get_key_quiet();
int get_key_debounced_quiet();
void get_mouse_release();
int get_num(int no);

OBJECT *GetSolidObject(int x,int y);
OBJECT *GetTopObject(int x,int y);
void GetNearDirection(char *message);
void CheckTime();
void ResyncEverything();
void ResumeSchedule(OBJECT *o);
void ForceUpdateTag(int tag);
static void CheckRoof();
static void CheckTiles(int x, int y, int w, int h, int border);
static void DarkRoof();
static void GameBusy();
int DisallowMap();

int LoadGame();
void SaveGame();

static void do_change_map();
static void check_horrors(OBJECT *o);  // Look for things that scare NPCs
static void ProcessDeath_U5();
static void ProcessDeath_U6();

extern void StoreObjects();
int Restart();
extern int choose_leader(int x);
extern void reset_globals();
extern void (*lightning)(IREBITMAP *img);
void CheckSpecialKeys(int k);

// code

/*
 *  Pre-initialise game engine
 */

void initgame()
{
int ctr;

game_hour = ig_hour;
game_minute = ig_min;
game_day = ig_day;
game_month = ig_month;
game_year = ig_year;

limbo.name = "Limbo";
limbo.flags |= IS_ON;

ilog_quiet("Allocating party name registrar\n");

for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	partyname[ctr]=(char *)M_get(1,128); // Should be plenty

player = NULL;
memset(party,0,sizeof(OBJECT *) * MAX_MEMBERS);

ilog_quiet("Set up conversation system\n");
NPC_Init();
}

/*
 *      IRE runner
 */

void startgame()
{
OBJECT *temp,*oplayer;
OBJECT *aptr;
OBJLIST *active,*next;
int vx,vy,ctr,turns;

turns=0;

syspocket = curmap->object;
curmap->object->flags |= IS_ON;

ilog_quiet("Init projector\n");
Init_Projector(VSW32,VSH32);
InitOrbit();

ilog_quiet("Seeking player\n");

if(!player)
	for(active=MasterList;active;active=active->next)
		if(active->ptr)
			if(!istricmp_fuzzy(active->ptr->name,"player*"))	{
				player = active->ptr;
				break;
			}

if(!player)
	ithe_panic("Could not find any players at all!","(in startgame)");

if(!party[0])	{
	party[0] = player;      // Init party
	strcpy(partyname[0],BestName(player));
	player->flags |= IS_PARTY;  // Make sure
}

//ilog_quiet("%d bytes free as we start the game:\n",coreleft());

//ilog_break=0; // Disable break, now the engine is about to start
		// This is done prior to InitProc now

ilog_quiet("Player =%x\n",player);
ilog_quiet("Player action =%d\n",player->activity);
if(player->activity > 0)
	ilog_quiet("Player action: %s\n",PElist[player->activity].name);
else
	ilog_quiet("No player action\n");

// If an alternate starting point is specified, move the player there
if(StartingTag)	{
	for(active=MasterList;active;active=active->next)
		if(active->ptr)
			if(active->ptr->tag == StartingTag)	{
				TransferObject(player,active->ptr->x,active->ptr->y);
				break;
			}
}



ClearMouseRanges();
// Create mouse range for game window
AddMouseRange(0,VIEWX,VIEWY,VSW*32,VSH*32);
// Create game grid for Mouse
SetRangeGrid(0,32,32);

//ilog_quiet("Init main game engine\n");
ilog_printf("Starting main game engine\n");

CentreMap(player);      // Centre on the player

if(IsTileWater(0,0))
	Bug("Tile at 0,0 is water.. may cause problems\n");

ilog_quiet("Activating NPC logic\n");
ResyncEverything();

ilog_quiet("Calling PE: mainproc\n");
pevm_context="Engine initialisation";
CallVM("mainproc");

S_MusicVolume(mu_volume);

// Set up mouse
ilog_quiet("Activating Mouse\n");
irecon_mouse(1);

ilog_quiet("All systems go\n");

fpsmetric=0;
time(&startmetric);

// The main loop begins here

do {
	if(show_vrm_calls)	{
		ilog_quiet("\n");
		ilog_quiet(">> Start of turn\n");
		ilog_quiet("\n");
	}

	pevm_context="main game loop (scheduler and status)";
	CallVMnum(Sysfunc_scheduler);     // Call initial function
	CallVMnum(Sysfunc_status);
	gen_largemap();

	CheckRoof();		//	show_roof=0;

	DrawMap(mapx,mapy);
	irecon_update();

	irekey = 0;
	irekey = get_key();

	// Wait, taking everything into account.
	while(GetIREClock()<FRAMERATE)
		DrawMap(mapx,mapy);
//	ilog_printf("then %d: now=%d\n",ctr,GetIREClock());
	ResetIREClock(); // Reset clock

	force_roof=0; // Don't


// Move all things

	if(show_vrm_calls)
		ilog_quiet(">> Moving Player..\n");

// Move the player first
	pevm_context="player movement";

	if(combat_mode)	{
		oplayer=player;
		for(ctr=0;ctr<MAX_MEMBERS;ctr++)
			if(party[ctr])	{
				player=party[ctr];
				current_object = NULL;//player;   // Set up parameters for the VRM
				person = player;           // Who am I?

				if(player->stats->npcflags & IS_BIOLOGICAL)	{
					pevm_context="player bio-update";
					CallVMnum(Sysfunc_updatelife);		// Only for carbon-based life
				}

				if(player->stats->npcflags & IS_ROBOT)	{
					pevm_context="player cyber-update";
					CallVMnum(Sysfunc_updaterobot);		// Only for robots
				}

				if(player->activity != -1)
					CallVMnum(player->activity);
			}
		player=oplayer;
	} else	{
		current_object = NULL;//player;   // Set up parameters for the VRM
		person = player;           // Who am I?

		// Call bio-systems update (if player IS a biological lifeform)
		if(player->stats->npcflags & IS_BIOLOGICAL)	{
			pevm_context="player bio-update";
			CallVMnum(Sysfunc_updatelife);		// Only for carbon-based life
		}

		// Call cybersystems update (if player is robotic)
		if(player->stats->npcflags & IS_ROBOT)	{
			pevm_context="player cyber-update";
			CallVMnum(Sysfunc_updaterobot);		// Only for robots
		}

		// Call player's core function
		if(player->activity != -1)
			CallVMnum(player->activity);
		else	{
			// Right, we've got a serious problem.  The player has lost control
			// Find an emergency function to maintain control of the engine
			// and allow cheat keys, restart game etc to keep working
			ctr=getnum4PE("PlayerBroken");
			if(ctr<0)
				Bug("Player doesn't know what to do\n"); // Oh shit
			else
				CallVMnum(ctr);
		}
	}

		
//	show_roof = 1; // Assume we need to show the roof

	if(show_vrm_calls)
		ilog_quiet(">> Check for mapmove..\n");

// Move to another map?  If so, set up the variables

	if(player->x > curmap->w - 8)	{
		change_map = curmap->con_e;
		change_map_y = player->y;
		change_map_x = 9;
		change_map_tag=0;
	}
	if(player->x < 8)	{
		change_map = curmap->con_w;
		change_map_y = player->y;
		change_map_x = curmap->w - 9;
		change_map_tag=0;
	}
	if(player->y > curmap->h - 8)	{
		change_map = curmap->con_s;
		change_map_y = 9;
		change_map_x = player->x;
		change_map_tag=0;
	}
	if(player->y < 8)	{
		change_map = curmap->con_n;
		change_map_y = curmap->h - 9;
		change_map_x = player->x;
		change_map_tag=0;
	}


// Reset update flags
	for(active=ActiveList;active;active=active->next)	{
		if(!(active->ptr->flags & IS_ON))	{
//			Bug("ActiveList: %s is DEAD\n",active->ptr->name);
			ML_Del(&ActiveList,active->ptr); // remove dead object
			active=ActiveList;
		}
		active->ptr->flags &= ~DID_UPDATE;
	}

#ifdef CHECK_MASTERLIST
	for(active=MasterList;active;active=active->next)	{
		if(!active->ptr)	{
			Bug("MasterList: NULL detected\n");
			ML_Del(&MasterList,active->ptr);
			active=MasterList;
		}
/*
		// If this happens we are badly ____ed (or SEER is broken again)
		if(active->ptr == (OBJECT *)0xdeadbeef)		{
			Bug("MasterList: DEAD BEEF detected\n");
			ML_Del(&MasterList,active->ptr);
			active=MasterList;
		}
		if(active->ptr == (OBJECT *)0xbeefdead)		{
			Bug("MasterList: BEEF DEAD detected\n");
			ML_Del(&MasterList,active->ptr);
			active=MasterList;
		}
*/
	}
#endif

	player->flags |= DID_UPDATE; // Player can't move twice in one go, nor can the rest of the party if we're in combat mode
	if(combat_mode)
		for(ctr=0;ctr<MAX_MEMBERS;ctr++)
			if(party[ctr])
				party[ctr]->flags |= DID_UPDATE;
  

	if(!ire_running)	// If the player quit
		break;

	CheckRoof();
	dark_mix=0;		// Reset additional darkness level
	DarkRoof();   // Add darkness if we need it

	if(show_vrm_calls)
		ilog_quiet(">> Moving things..\n");

	stopmetric=GetIREClock();

	// Now move all other objects
	active=ActiveList;
	if(active)	{
		do {
			aptr=active->ptr;  // Active may get deleted from the list,
							    // so take a copy of the object in question

			next = active->next;
			if(aptr->flags & IS_ON)
				if(!(aptr->flags & DID_UPDATE))	{	// Don't do this one again if we need to restart the update
					aptr->user->oldhp = aptr->stats->hp;
					AL_dirty=0;                     // Mark list as clean
					current_object = NULL;   // Set up parameters for the script
					person = aptr;           // Who am I?
					aptr->flags |= DID_UPDATE; // Mark object as moved

					// If it is biological, call biosystems update
					if(aptr->stats->npcflags & IS_BIOLOGICAL)
						if(aptr->stats->hp > 0)	// Must be alive
							{
							pevm_context="creature bio-update";
							CallVMnum(Sysfunc_updatelife);		// Only for carbon-based life
							}

					// If it is biological, call biosystems update
					if(aptr->stats->npcflags & IS_ROBOT)
						if(aptr->stats->hp > 0)	{// Must be alive
							pevm_context="creature cyber-update";
							CallVMnum(Sysfunc_updaterobot);		// Only for carbon-based life
						}

					// Do the move first
					if(aptr->activity>0)	{
						if(show_vrm_calls || DebugVM)	{
							ilog_quiet("    Object %s at %d,%d calling %d\n",aptr->name,aptr->x,aptr->y,aptr->activity);
							ilog_quiet("    Which is %s\n",PElist[aptr->activity].name);
						}
						pevm_context="object activity";
						CallVMnum(aptr->activity);
					}
							
					// If it is distance-triggered..
					if(aptr->stats->radius>0)	{
						if(aptr->user->counter == 0)	{
							// Is it within the distance?
							vx=player->x - aptr->x;
							vy=player->y - aptr->y;
							
							if(vx<0)
								vx=-vx;
							if(vy<0)
								vy=-vy;

							// Choose biggest
							if(vx < vy)
								vx=vy;

							if(vx == aptr->stats->radius)	{
								// Lock egg out
								aptr->user->counter=EggLockout;
								if(show_vrm_calls || DebugVM)	{
									ilog_quiet("<Radius: calling %s at %d,%d>\n",PElist[aptr->activity].name,aptr->x,aptr->y);
									if(aptr->pocket.objptr)
										ilog_quiet("<radius object contains %s>\n",aptr->pocket.objptr->name);
								}
								// Set up registers for compatability with Trigger funcs
								victim=player; // Currently only player
								current_object=aptr;
								pevm_context="radius trigger";
								CallVMnum(aptr->funcs->ucache); // Use it
								current_object=NULL;
								victim=NULL;
							}
						}
					}
						
					// We were doing the move last of all, but that caused problems with lava, swamps and so forth

					if(AL_dirty)                    // If list is dirty we need to
						next=ActiveList;            // start over.
				}

			active = next;
		} while(active);
	}
//ilog_printf("%d jticks\n",GetIREClock()-stopmetric);

//	Fortify_CheckAllMemory(); // Debugging tool.  No effect unless compiled in

	// Now, we'd better check the tiles and see if any of them have code associated
	if(show_vrm_calls)
		ilog_quiet(">> Check active tiles\n");
	CheckTiles(mapx,mapy,VSW,VSH,4);	// 4 tile border around the edge

	if(show_vrm_calls)
		ilog_quiet(">> Update clock..\n");

	pevm_context="schedule";

	turns++;
	if(turns>turns_min)	{
		// If a minute has passed, check for interesting things
		turns=0;
		game_minute++;
		CheckTime();

		// Start any schedules, and handle egg lockout timer

		for(active=MasterList;active;active=active->next)
			if(active->ptr->stats->hp>0 && (active->ptr->stats->npcflags & NO_SCHEDULE) == 0)	{
				// Are any schedules due?
				if(active->ptr->schedule)
					if((active->ptr->flags & IS_ON) && (active->ptr->flags & IS_PARTY) == 0)
						for(vx=0;vx<24;vx++)	{
							if(!active->ptr->schedule[vx].active)
								continue;
							if(active->ptr->schedule[vx].hour == game_hour)	{
								if(active->ptr->schedule[vx].minute == game_minute)	{
#ifdef DEBUG_SCHEDULE
irecon_printf("%s does %s to %s\n",active->ptr->name,active->ptr->schedule[vx].vrm,active->ptr->schedule[vx].target?active->ptr->schedule[vx].target->name:"Nothing");
#endif
									lastvrmcall="Activity Engine";
									SubAction_Wipe(active->ptr);
									ActivityNum(active->ptr,active->ptr->schedule[vx].call,active->ptr->schedule[vx].target.objptr);
									lastvrmcall="N/A";
								}
							}
						}

				// Decrement Egg Lockout counter
				if(active->ptr->stats->radius>0)
					if(active->ptr->user->counter>0)
						active->ptr->user->counter--;
/*
				// And check for horrible things, if applicable
				if(active->ptr->stats->intel >= 40) // is alive
					if(active->ptr->funcs->hrcache > 0) // can be horrified
						check_horrors(active->ptr);
*/
			}
		}

	// Check for horrible things
	for(active=MasterList;active;active=active->next)
		if(active->ptr->stats->hp>0 && (active->ptr->stats->npcflags & NO_SCHEDULE)==0)	{
			// And check for horrible things, if applicable
			if(active->ptr->stats->intel >= 40) // is alive
				if(active->ptr->funcs->hrcache > 0) // can be horrified
					check_horrors(active->ptr);
		}
			

	CheckRoof();

//
// Kill dead things
//

	pevm_context="damage/death tracking";

	if(show_vrm_calls)
		ilog_quiet(">> Admin death and destruction..\n");

	if(ire_U6partymode)
		ProcessDeath_U6();	// Use Ultima-6 style party management
	else
		ProcessDeath_U5();	// Use older party style

	if(!player)	{
		// Call the game_over code.  This may reinit the world, or just resurrect
		// the player elsewhere.
		DrawMap(mapx,mapy);
		if(show_vrm_calls)
			ilog_quiet(">> all fall down\n");
		CallVM("all_dead");
	}

	pending_delete=1;	// Force garbage collection
	DeletePending();        // Destroy And Be Free!  Destroy And Be Free!

	pevm_context="main game loop (final stuff after updates)";

	if(change_map)
		do_change_map();

/*
	// Check internal system keys

	if(irekey == IREKEY_F2)	{
		CentreMap(player);
		SaveGame();
	}

	if(irekey == IREKEY_F3)	{
		CentreMap(player);
		LoadGame();
	}

	if(irekey == IREKEY_ESC)	{
		CentreMap(player);
		if(getYN("Quit game"))
			ire_running = 0;
	}
*/

	if(irekey == IREKEY_F10)	{
		CentreMap(player);
		irecon_printf("Taking screenshot..\n");
		irecon_update();
		Show();
//		BMPshot(swapscreen);	 // Now handled by CheckSpecialKeys
	}
	CheckSpecialKeys(irekey);

	CentreMap(player);
} while(ire_running);

time(&stopmetric);

irecon_term();

ilog_quiet("Game is quit by user\n");

timemetric = difftime(stopmetric, startmetric);
ilog_quiet("Measured %f fps\n",(double)fpsmetric/timemetric);
}



/* ============================== Map Manipulation ======================== */

/*
 *      DrawMap - Display the map on screen
 */

void DrawMap(int x,int y)
{
if(x+VSW > curmap->w)
	x=curmap->w-VSW;
if(y+VSH > curmap->h)
	y=curmap->h-VSH;

if(UpdateAnim())	{
	if(vis_blanking && !blanking_off)
		Hide_Unseen_Map();

	Project_Map(x,y);


	Project_Sprites(x,y);

	if(show_roof || force_roof > 0)	{
		Project_Roof(x,y);
	}

#if 0
	static int intens=30,fallof=100;
	if(irekey == IREKEY_PLUS_PAD) intens++;
	if(irekey == IREKEY_MINUS_PAD) intens--;
	if(irekey == IREKEY_ASTERISK) fallof++;
	if(irekey == IREKEY_SLASH) fallof--;
	sprintf(user_input,"i%d, f%d\n",intens,fallof);
	swapscreen->RectFill(0,0,128,8,ire_black);
	irecon_printxy(0,0,user_input);
//	drawing_mode(DRAW_MODE_SOLID,NULL,0,0);
	ProjectCorona(128,128,64,intens,fallof,TINT_YELLOW);
#endif

	// Do the shading
	if(use_light)
		render_lighting(x,y);

	// Block invisible tiles
	if(vis_blanking && !blanking_off)
		Hide_Unseen_Project();

	// Combine the roof with the main window
	if(show_roof || force_roof > 0)	{
		//darken_blit(gamewin->GetFramebuffer(),roofwin->GetFramebuffer(),gamewinsize);
		gamewin->Merge(roofwin);
	}

	// Add a lightning effect if we need it
	if(do_lightning)	{
		if(use_light)
			lightning(gamewin);
		do_lightning--;
	}

	// visual debugger for Large Objects.   Not for production, as you will
	// see very quickly if you enable it.

	#ifdef DEBUG_LARGE_OBJECTS
	for(int vy=0;vy<VSW;vy++)
		for(int vx=0;vx<VSH;vx++)
			if(largemap[vx+VIEWDIST][vy+VIEWDIST])
				gamewin->RectFill((vx<<5),(vy<<5)+16,(vx<<5)+32,(vy<<5)+32,255,255,255);
	#endif
	#ifdef DEBUG_SOLID_OBJECTS
	for(int vy=0;vy<VSW;vy++)
		for(int vx=0;vx<VSH;vx++)
			if(solidmap[vx+VIEWDIST][vy+VIEWDIST])
				gamewin->RectFill((vx<<5),(vy<<5),(vx<<5)+32,(vy<<5)+32,255,255,255);
	#endif
	#ifdef DEBUG_BLOCKSLIGHT
	for(int vy=0;vy<VSW;vy++)
		for(int vx=0;vx<VSH;vx++)
			if(BlocksLight(vx+mapx,vy+mapy))
				gamewin->RectFill((vx<<5),(vy<<5),(vx<<5)+32,(vy<<5)+32,255,255,255);
	#endif

	// Shake the screen for an earthquake effect
	if(do_earthquake)	{
		// Push towards zero
		if(do_earthquake>0)
			do_earthquake--;
		else
			do_earthquake++;
		do_earthquake=-do_earthquake; // Invert
	}
}

// Finally, draw the thing, possibly darker than normal if we're fading
if(Fader)	{
	swapscreen->FillRect(VIEWX,VIEWY,gamewin->GetW(),gamewin->GetH(),ire_black);
	gamewin->DrawAlpha(swapscreen,VIEWX,VIEWY,255-Fader);
} else	{
	// If we're doing the earthquake bit, blit with an offset
	if(do_earthquake)	{
//		blit(gamewin,swapscreen,0,do_earthquake,VIEWX,VIEWY,gamewin->w,gamewin->h);
		gamewin->Draw(swapscreen,VIEWX,VIEWY+do_earthquake);
	} else
		gamewin->Draw(swapscreen,VIEWX,VIEWY);
}

// Debugging
if(ShowRanges)
	DrawRanges();

fpsmetric++;
}


/*
 *	Check for active tiles 
 */

void CheckTiles(int x, int y, int w, int h, int border)	{
  
int xctr=0;
int yctr=0,ptr,bit,ctr,ctr2;
int x2,y2;

unsigned short *tptr;
unsigned short tn=0;
TILE *t;
OBJECT *o,*oldvic;

// Figure out the absolute coordinates, with the border

x2=x+w+border;
y2=y+h+border;

x-= border;
y-= border;

// Clip them

if(x<0)
	x=0;
if(y<0)
	y=0;
if(x2>=curmap->w)
	x2=curmap->w-1;
if(y2>=curmap->h)
	y2=curmap->h-1;

// Now get the width and height after clipping
w=x2-x;
h=y2-y;

// Now work out the offset into the array
ptr=y*curmap->w;
ptr+=x;

// Work out the skip factor
bit=curmap->w - w;

tptr = &curmap->physmap[ptr];

oldvic=victim;

for(yctr=0;yctr<h;yctr++)	{
	for(xctr=0;xctr<w;xctr++,tptr++)	{
		tn = *tptr;
		if(tn >= TItot)
			continue;	// Invalid, possible the selector tile from the editor
		t=&TIlist[tn];
		if(t->standfunc > 0)	{
			// Got one!  Now we need to find out if anything is above it
			o=GetObjectBase(x+xctr,y+yctr);
			if(!o)
				continue;	// Nothing there
				
			// Affect all the objects above it.  We will probably need a flag to toggle this behaviour in the tile.  Lava needs it, others may not want it at all
				
			for(;o;o=o->next)	{
				if((o->flags & IS_ON) && (!(o->flags & IS_SYSTEM)) && (!(o->flags & IS_DECOR)))	{
					victim=o;
					
//					printf("run at %d,%d do func %s on %s\n",x+xctr,y+yctr,PElist[t->standfunc].name,o->name);
					
					pevm_context="Active Tile";
					CallVMnum(t->standfunc);
					victim=NULL;
				}
			}
		}
	}
	tptr += bit;
}

victim=oldvic;
}



/*
 *      getYN - Ask a question, return TRUE if the answer is Y
 */

int getYN(char *q)
{
irecon_printf("%s- Are you sure?  ",q);
irecon_update();
Show();
if(get_key() != IREKEY_Y)
	{
	irecon_printf("No.\n");
	irecon_update();
	Show();
	return 0;
	}
else
	{
	irecon_printf("Yes.\n");
	irecon_update();
	Show();
	return 1;
	}
}

/*
 *      get_key - Get a key from the user, animate things in the background
 */

int get_key()
{
int input=0;

// If we're in STILL mode (no animation) draw once then never again
#ifdef NO_ANIMATION
	DrawMap(mapx,mapy);
	Show();
#endif

do  {
#ifndef NO_ANIMATION
	DrawMap(mapx,mapy);
	Show();
	IRE_WaitFor(0); // Yield timeslice
#endif

	IRE_GetMouse();
	CheckMouseRanges();
	if(MouseID != -1)
		{
		waitredraw(MouseWait);
		input=IREKEY_MAX+1; // Not a valid key, a mouse click
		}
	else
		{
		input = GetKey();
		CheckSpecialKeys(input);
		}
	} while(!input);
return input;
}

/*
 *  As above, but don't draw the game window
 */

int get_key_quiet()
{
int input=0;

do  {
	if(ShowRanges)
		DrawRanges();
	Show();
	IRE_WaitFor(0); // Yield timeslice
	IRE_GetMouse();
	CheckMouseRanges();
	if(MouseID != -1)
		{
//		rest_callback(100,poll_mouse);
		IRE_WaitFor(MouseWait);
		input=IREKEY_MAX+1; // Not a valid key, a mouse click
		}
	else
		input = GetKey();
	} while(!input);
return input;
}

/*
 *      get_key_debounced - Get a key, wait for it to be released
 */

int get_key_debounced()
{
int input=0;

//FlushKeys();
input = get_key();
CheckSpecialKeys(input);
FlushKeys();

return input;
}

/*
 *  As above, but don't draw the game window
 */

int get_key_debounced_quiet()
{
int input=0;

input = get_key_quiet();
CheckSpecialKeys(input);
FlushKeys();

return input;
}


void get_mouse_release()
{
int b;
do
	{
	DrawMap(mapx,mapy);
	Show();
	IRE_WaitFor(0); // Yield timeslice
//	S_PollMusic();

	if(!IRE_GetMouse(&b)) // No mouse
		return;
	} while(b);
return;
}



/*
 *      get_num - User types in a number, animate things in the background
 */

int get_num(int no)
{
int input=0;
int ascii=0;
int bufpos=0;
char buf[128];

itoa(no,buf,10);
bufpos=strlen(buf);
buf[bufpos]=0;
do  {
	DrawMap(mapx,mapy);
	irecon_clearline();
	if(buf[0] != 0)
		irecon_print(buf);         // Printing an empty line makes a newline
	irecon_print(CURSOR_CHAR);
	irecon_update();
	Show();
	IRE_WaitFor(0); // Yield timeslice
//	S_PollMusic();
	if(IRE_KeyPressed())
		{
		input = IRE_NextKey(&ascii);
		CheckSpecialKeys(input);
		if(ascii >= '0' && ascii <= '9')
			{
			if(bufpos<conwid)
				{
				buf[bufpos++]=ascii;
				buf[bufpos]=0;
				}
			}
		if(input == IREKEY_DEL || input == IREKEY_BACKSPACE)
			if(bufpos>0)
				{
				bufpos--;
				buf[bufpos]=0;
				}
		}
	else
		input=0;
	} while(input != IREKEY_ESC && input != IREKEY_ENTER);
	irecon_clearline();
irecon_print(buf);
irecon_print("\n");
if(input == IREKEY_ESC)
	return 0;
strcpy(user_input,buf);
return 1;
}

/*
 *      RedrawMap - Redraw the map and update
 */

void RedrawMap()
{
CentreMap(player);
DrawMap(mapx,mapy);
Show();
IRE_WaitFor(0); // Yield timeslice
}


/*
 *      Restart - Restart the game totally
 */

int Restart()
{
char *fname;
OBJLIST *active,*ptr;
int ctr,newmap;

// Safety valve
if(!curmap->object)
	return 0;

GameBusy();

ilog_quiet("Destroy temporary savegame\n");
makesavegamedir(9999,1);

// Set time

game_hour = ig_hour;
game_minute = ig_min;
game_day = ig_day;
game_month = ig_month;
game_year = ig_year;

// Destroy in-game variables

irecon_printf("Erase..\n");
Wipe_tFlags();
wipe_z1();

// Change map if neccessary
newmap=0;

if(mapnumber != default_mapnumber)
	{
	mapnumber = default_mapnumber;
	newmap=1;

	pending_delete=1;	// Force garbage collection
	DeletePending();

	// Archive objects to Limbo
	while(curmap->object->next) OB_Free(curmap->object->next);

	// Now start deleting everything
	wipe_z1();

	// Erase the activelist
	while(ActiveList) ML_Del(&ActiveList,ActiveList->ptr); // erase list

	// Read in the physical map
	load_map(mapnumber);
	syspocket = curmap->object; // make sure we don't trash the game!
	}

// Reset tile scrolling
for(ctr=0;ctr<TItot;ctr++)
	scroll_tile_reset(ctr);

// Reset everything
irecon_printf("Reload..\n");

fullrestore = 0;
fname=makemapname(mapnumber,0,".mz1");
load_z1(fname);
// Load roof and lights
load_z2(mapnumber);
load_z3(mapnumber);
fullrestore = 0;

ilog_printf("\n");
ilog_printf("Rerun Object Inits\n");

for(ptr=MasterList;ptr;ptr=ptr->next)
	if((ptr->ptr->flags & DID_INIT) == 0)
		{
		if(ptr->ptr->funcs->icache != -1)
			{
			current_object = ptr->ptr;
			person = ptr->ptr;
			new_x = person->x;
			new_y = person->y;
			CallVMnum(ptr->ptr->funcs->icache);
			}
		ptr->ptr->flags |= DID_INIT;
		}

irecon_printf("Reset..\n");

// Find player

player = NULL;
for(active=MasterList;active;active=active->next)
	if(active->ptr)
		if(!istricmp_fuzzy(active->ptr->name,"player*"))
			{
			player = active->ptr;
			break;
			}

if(!player)
    ithe_panic("Cannot find the player anymore",NULL);

reset_globals();

ResyncEverything();

CallVM("mainproc");
CallVM("loadproc");

for(ctr=0;ctr<MAX_MEMBERS-1;ctr++)
	{
	party[ctr]=0;
	partyname[ctr][0]='\0';
	}
party[0]=player;
strcpy(partyname[0],BestName(player));
player->flags |= IS_PARTY;

return 1;
}

void CheckTime()
{
//printf("checktime was %04d/%02d/%02d %02d:%02d\n",game_year,game_month,game_day,game_hour,game_minute);
//printf("checktime %d m/h %d h/d %d d/m %d m/y igyear %d\n",min_hour,hour_day,day_month,month_year,ig_year);
  
if(game_minute >= min_hour)
	{
	game_hour += game_minute/min_hour;
	game_minute %= min_hour;
	}

if(game_hour >= hour_day)
	{
	game_day += game_hour/hour_day;
	game_hour %= hour_day;
	}

if(game_day >= day_month)
	{
	game_month += game_day/day_month;
	game_day %= day_month;
	}

if(game_month >= month_year)
	{
	game_year += game_month/month_year;
	game_month %= month_year;
	}

//printf("checktime now %04d/%02d/%02d %02d:%02d\n",game_year,game_month,game_day,game_hour,game_minute);
days_passed = ((game_year - ig_year) * month_year) * day_month;  // Year days
days_passed += (game_month - ig_month) * day_month; // Month days
days_passed += (game_day - ig_day); // days
//printf("days passed = %d\n",days_passed);

day_of_week = days_passed % day_week;
}

void ResyncEverything()
{
OBJLIST *t;
for(t=MasterList;t;t=t->next)
	{
	SubAction_Wipe(t->ptr); // Kill any sub-activities
	ResumeSchedule(t->ptr);
	}
}


/*
 *      Make an NPC do whatever they were doing before they were distracted
 */

void ResumeSchedule(OBJECT *o)
{
int i,h,ctr;
int activity;
OBJECT *target;

#ifdef DEBUG_RESUMEACTIVITY
#define DEBUG_RESACT_PRINT(x,y) ilog_printf(x,y);
#else
#define DEBUG_RESACT_PRINT(x,y) ;
#endif


// Make sure it's valid

if(!o)
	{
	DEBUG_RESACT_PRINT("ResumeSchedule was passed %x\n",NULL);
	return;
	}

	DEBUG_RESACT_PRINT("Resuming for %s\n",BestName(o));

if((o->flags & IS_ON) == 0)
	{
	DEBUG_RESACT_PRINT("%s is switched off\n",BestName(o));
	return;
	}

if(o->flags & IS_PARTY)
	{
	DEBUG_RESACT_PRINT("%s is of the party\n",BestName(o));
	return;
	}

if(o->stats->hp<=0)
	{
	DEBUG_RESACT_PRINT("%s is dead\n",BestName(o));
	return;
	}

if(!o->schedule)
	{
	// We don't have a schedule.  Was there a default activity?
	i=getnum4char(o->name);
	if(i<0)
		{
		Bug("Gurk!!  Object '%s' doesn't exist in ResumeSchedule\n",o->name);
		return;
		}
	if(CHlist[i].activity>0)
		{
		ActivityNum(o,CHlist[i].activity,NULL);
		return;
		}
	// Carry on.  It's possible there's a subactivity.  If not, stop then
	}

// First, are there any pending sub-activities
if(o->user->actlist[0]>0)
	{
	// Get the activity
	SubAction_Pop(o,&activity,&target);

	DEBUG_RESACT_PRINT("%s does ",BestName(o));
	DEBUG_RESACT_PRINT(" subaction %s",PElist[activity].name);
	DEBUG_RESACT_PRINT(" to %s\n",BestName(target));

	// Do the activity
	ActivityNum(o,activity,target);
	return;
	}

if(o->stats->npcflags & NO_SCHEDULE)
	{
	DEBUG_RESACT_PRINT("%s is not accepting new schedules\n",BestName(o));
	return;
	}


if(!o->schedule)
	{
	// Bang, you're dead
//	DEBUG_RESACT_PRINT("%s has no schedule\n",BestName(o));
	return;
	}

i = -1;
h = -1;

// Look for events which took place before GameHour today
// REMEMBER: There are 24 slots, but each slot does NOT correspond to an hour!

DEBUG_RESACT_PRINT("%s: trying today\n",o->name);
for(ctr=0;ctr<24;ctr++)
	{
	// Reject known bad cases
	if(!o->schedule[ctr].active)
		continue;

#ifdef DEBUG_RESUMEACTIVITY
		ilog_quiet("Event found at %d:%02d [%s]\n",o->schedule[ctr].hour,o->schedule[ctr].minute,o->schedule[ctr].vrm);
#endif
		
	if(o->schedule[ctr].hour > game_hour)
		continue;
	if(o->schedule[ctr].hour == game_hour && o->schedule[ctr].minute > game_minute)
		continue;
	if(o->schedule[ctr].hour >= h)
		{
	#ifdef DEBUG_RESUMEACTIVITY
		ilog_quiet("using event found at %d:%02d [%s]\n",o->schedule[ctr].hour,o->schedule[ctr].minute,o->schedule[ctr].vrm);
	#endif
		i = ctr;
		h = o->schedule[ctr].hour;
		}
	}

// Found nothing?  Try yesterday (before game_hour)

if(h == -1)
	{
	DEBUG_RESACT_PRINT("%s: trying yesterday\n",o->name);
	for(ctr=0;ctr<24;ctr++)
		{
		// Reject known bad cases
		if(!o->schedule[ctr].active)
			continue;
		if(o->schedule[ctr].hour < game_hour)
			continue;
		#ifdef DEBUG_RESUMEACTIVITY
			ilog_quiet("Event found at %d:%02d [%s]\n",o->schedule[ctr].hour,o->schedule[ctr].minute,o->schedule[ctr].vrm);
		#endif
		if(o->schedule[ctr].hour >= h)
			{
			i = ctr;
			h = o->schedule[ctr].hour;
			}
		}
	}

if(i>=0)
	{
	if(o->schedule[i].call == o->activity && o->schedule[i].target.objptr == o->target.objptr)
		{
//		ilog_quiet("%s trying to resume current task\n",o->name);
		return;
		}
	SubAction_Wipe(o);
	ActivityNum(o,o->schedule[i].call,o->schedule[i].target.objptr);
	#ifdef DEBUG_RESUMEACTIVITY
	ilog_quiet("%s resumes doing %s (%d) to %s\n",o->name,o->schedule[i].vrm,o->schedule[i].call,o->schedule[i].target.objptr?o->schedule[i].target.objptr->name:"Nothing");
	#endif
	}
else
	{
	// Okay, so we have absolutely no idea.  Was there a default activity?
	i=getnum4char(o->name);
	if(i<0)
		{
		Bug("Gurk!!  Object '%s' doesn't exist in ResumeSchedule\n",o->name);
		return;
		}
	if(CHlist[i].activity>0)
		ActivityNum(o,CHlist[i].activity,NULL);
//	else
//		DEBUG_RESACT_PRINT("%s couldn't decide what to do\n",BestName(o));
	}
}


/*
 *        Update all objects with the specified tag (for special effects)
 */

void ForceUpdateTag(int tag)
{
OBJLIST *active,*next;


// Reset update flag

for(active=ActiveList;active;active=active->next)
	active->ptr->flags &= ~DID_UPDATE;

// Move all objects with this tag
active=ActiveList;
if(active)
	do {
		next = active->next;
		if(active->ptr->flags & IS_ON)
			if(active->ptr->tag == tag)
				if(!(active->ptr->flags & DID_UPDATE)) // Don't do this one again if we need
					{                               // to restart the update
					active->ptr->user->oldhp = active->ptr->stats->hp;
					AL_dirty=0;                     // Mark list as clean
					current_object = NULL;//active->ptr;   // Set up parameters for the VRM
					person = active->ptr;           // Who am I?
					active->ptr->flags |= DID_UPDATE; // Mark object as moved
					if(active->ptr->stats->npcflags & IS_BIOLOGICAL)
						if(active->ptr->stats->hp > 0)	// Must be alive
							CallVMnum(Sysfunc_updatelife);		// Only for carbon-based life
					if(active->ptr->stats->npcflags & IS_ROBOT)
						if(active->ptr->stats->hp > 0)	// Must be alive
							CallVMnum(Sysfunc_updaterobot);		// Only for robotic life
					CallVMnum(active->ptr->activity);
					if(AL_dirty)                    // If list is dirty we need to
						next=ActiveList;            // start over.
					}
			active = next;
			} while(active);

// Delete Objects

DeletePending();
AL_dirty=1;                     // Mark list as clean
}

/*
 *        Change the map to another
 */

void do_change_map()
{
char *fname;
OBJLIST *ptr;

GameBusy();

pending_delete=1;	// Force garbage collection
DeletePending();

// We did archive the limbo objects here...
save_objects(mapnumber);

// Write current map space to savegame 9999
fname=makemapname(mapnumber,9999,".mz1");
if(fname)
	{
	MZ1_SavingGame=1; // Store every tiny detail
	save_z1(fname);
	MZ1_SavingGame=0;
	}

// Write NPC usedata to file (AFTER z1 has assigned new numbers)
fname=makemapname(mapnumber,9999,".ms");
if(fname)
	{
	ilog_quiet("Write '%s'\n",fname);
	save_ms(fname);
	}

// Get light state as well
save_lightstate(mapnumber,9999);

// Now start deleting everything
mapnumber=change_map;
wipe_z1();

// Erase the activelist
while(ActiveList) ML_Del(&ActiveList,ActiveList->ptr); // erase list

erase_curmap();

// Read in the physical world
load_map(change_map);
syspocket = curmap->object; // make sure we don't trash the game!

// Read in savegame 9999 map if it exists
fname=makemapname(mapnumber,9999,".mz1");
if(fname && fileexists(fname))
	{
	fullrestore=1;	// Ensure every tiny detail is restored
	load_z1(fname);
	fullrestore=0;
	}
else
	{
	// Read in the master file if it doesn't
	fname=makemapname(mapnumber,0,".mz1");
	if(fname)
		{
		// Just read it in normally (DON'T use 'fullrestore')
		load_z1(fname);
		}
	}

load_z2(mapnumber);
load_z3(mapnumber);

// Restore usedata if it exists
fname=makemapname(mapnumber,9999,".ms");
if(fname && fileexists(fname))
	{
/*
ilog_quiet("Dump mastlist\n");
for(ptr=MasterList;ptr;ptr=ptr->next)
	if(ptr->ptr)
		ilog_quiet(">%s : %d\n",ptr->ptr->name,ptr->ptr->save_id);
ilog_quiet("Done\n");
*/
	
	// Restore only what we need, not the time and party state
	ilog_quiet("Restore '%s'\n",fname);
	MZ1_SavingGame=1;
	load_ms(fname);
	MZ1_SavingGame=0;
	}

// Restore archived objects from limbo
restore_objects();



// Get light state (if exists)
load_lightstate(mapnumber,9999);

// Run inits (as necessary)
for(ptr=MasterList;ptr;ptr=ptr->next)
	{
	if(!(ptr->ptr->flags & DID_INIT))
		{
		if(ptr->ptr->funcs->icache != -1)
			{
			current_object = ptr->ptr;
			person = ptr->ptr;
			new_x = person->x;
			new_y = person->y;
			CallVMnum(ptr->ptr->funcs->icache);
			}
		ptr->ptr->flags |= DID_INIT;
		}
		
#ifdef WIELD_PARANOID
	if(ptr->ptr->stats && ptr->ptr->stats->npcflags & IS_WIELDED)
		{
//		printf("Check wield for %s\n",ptr->ptr->name);
		if(!FindWieldPoint(ptr->ptr,NULL))
			{
			printf("!!! semi-wielded object %s found!\n",ptr->ptr->name);
			ptr->ptr->stats->npcflags &= ~IS_WIELDED;
			}
		}
#endif		
	}

CallVM("loadproc");

ResyncEverything();

// Okay, done
change_map=0;
}


// Scanning table for NPC's vision for each of the 8 directions.
// Each block is a single 'line', containing the coordinates necessary
// to scan outwards from the NPC in a cone-shape, e.g:
//
//       9
//      4A
//     15B
//    *26C
//     37D
//      8E
//       F
//
// ..where the '*' is the NPC looking east, and the numbers are the scan
// in order, i.e. '1' is first, 'F' is last.
// The table contains the vectors needed to make this happen.

static int vision_cone[4][15][2]=
	{
	// Scan North
	{{+0,-1},{-1,-1},{+1,-1},
	 {-2,-2},{-1,-2},{+0,-2},{+1,-2},{+2,-2},
	 {-3,-3},{-2,-3},{-1,-3},{+0,-3},{+1,-3},{+2,-3},{+3,-3}},
	// Scan South
	{{+0,+1},{-1,+1},{+1,+1},
	 {-2,+2},{-1,+2},{+0,+2},{+1,+2},{+2,+2},
	 {-3,+3},{-2,+3},{-1,+3},{+0,+3},{+1,+3},{+2,+3},{+3,+3}},
	// Scan West
	{{-1,-1},{-1,+0},{-1,+1},
	 {-2,-2},{-2,-1},{-2,+0},{-2,+1},{-2,+2},
	 {-3,-3},{-3,-2},{-3,-1},{-3,+0},{-3,+1},{-3,+2},{-3,+3}},
	// Scan East
	{{+1,-1},{+1,+0},{+1,+1},
	 {+2,-2},{+2,-1},{+2,+0},{+2,+1},{+2,+2},
	 {+3,-3},{+3,-2},{+3,-1},{+3,+0},{+3,+1},{+3,+2},{+3,+3}},
	};


void check_horrors(OBJECT *o)
{
int ctr,vx,vy;
OBJECT *temp;
OBJREGS regs;

if(o->x <=0 || o->y<=0) // Out of range
	return;

if(!o)
	return;

for(ctr=0;ctr<15;ctr++)
	{
	vx=o->x+vision_cone[o->curdir][ctr][0];
	vy=o->y+vision_cone[o->curdir][ctr][1];
	if(vx<0 || vx>curmap->w)
		break;
	if(vy<0 || vy>curmap->h)
		break;
	if(line_of_sight(o->x,o->y,vx,vy))
		{
		temp=GameGetObject(vx,vy);
		for(;temp;temp=temp->next)
			if(temp)
				if(temp->flags & IS_HORRIBLE) // We saw something nasty
					{
					VM_SaveRegs(&regs);
					person=o;
					current_object=temp;
					CallVMnum(o->funcs->hrcache);
					VM_RestoreRegs(&regs);
					return;
					}
		}
	}

return;
}

/*
 *  Do we show the rooftops or not?
 */

void CheckRoof()
{
// Check roof
show_roof=1;

if(curmap->roof[(player->y*curmap->w)+player->x])
	{
	// There is a roof above our heads.  Take the roof away?
	if(RTlist[curmap->roof[(player->y*curmap->w)+player->x]].flags&KILLROOF)
		show_roof=0;
	}

/*
// Look for windows

	temp=GetRawObjectBase(player->x,player->y-1);
	if(temp)
		if(temp->flags & IS_WINDOW)
			show_roof=0;

	temp=GetRawObjectBase(player->x,player->y+1);
	if(temp)
		if(temp->flags & IS_WINDOW)
			show_roof=0;

	temp=GetRawObjectBase(player->x-1,player->y);
	if(temp)
		if(temp->flags & IS_WINDOW)
			show_roof=0;

	temp=GetRawObjectBase(player->x+1,player->y);
	if(temp)
		if(temp->flags & IS_WINDOW)
			show_roof=0;
*/

// The script language has the final say
if(force_roof)
	{
	if(force_roof>0)
		show_roof=1;
	else
		show_roof=0;
	}
}


void DarkRoof()
{
if(curmap->roof[(player->y*curmap->w)+player->x])
	{
	// Darken the world if applicable
	dark_mix+=RTlist[curmap->roof[(player->y*curmap->w)+player->x]].darkness;
	}
}


int DisallowMap()
{
if(curmap->roof[(player->y*curmap->w)+player->x])
	return RTlist[curmap->roof[(player->y*curmap->w)+player->x]].flags & KILLMAP;
return 0;
}


void GameBusy()
{
if(!gamewin)
	return;

// Darken the screen to show the game's 'thinking'
swapscreen->FillRect(VIEWX,VIEWY,gamewin->GetW(),gamewin->GetH(),ire_black);
gamewin->DrawShadow(swapscreen,VIEWX,VIEWY,128);
Show();
}


void SaveGame(int savegame_no, const char *title)
{
char msg[128];
memset(msg,0,sizeof(msg));
if(!title)
	title="Uh";
SAFE_STRCPY(msg,title);

printf("Make savegame dir\n");	
makesavegamedir(savegame_no,1); // Don't erase the other files or we'll lose the non-current map's data
copysavegamedir(9999,savegame_no); // Copy scratch files to savegame

// Write the control file
write_sgheader(savegame_no,mapnumber,msg);

irecon_printf("Saving..\n");
irecon_update();
GameBusy();

savegame=makemapname(mapnumber,savegame_no,".mz1");
MZ1_SavingGame=1; // Store every tiny detail
save_z1(savegame);
MZ1_SavingGame=0;

// Save light state
save_lightstate(mapnumber,savegame_no);

savegame=makemapname(0,savegame_no,".msc");
save_ms(savegame);
}





void SaveGame()
{
int savegame_no,ctr;
time_t time_data;
char msg[128];

memset(msg,0,sizeof(msg));

savegame_no=GetLoadSaveSlot(1);

if(savegame_no > 0)
	{
	savegame=read_sgheader(savegame_no,&ctr);

	if(savegame)
		strcpy(msg,savegame);
	else
		{
		time(&time_data);
		strcpy(msg,ctime(&time_data));

		// ctime() likes to add a CR and stuff.. remove it
		strdeck(msg,0x0d);
		strdeck(msg,0x0a);
		strstrip(msg);
		}

	if(GetTextBox(msg,"Enter a description",40))
		SaveGame(savegame_no,msg);
	}
}


bool LoadGame(int savegame_no)
{
OBJECT *temp;
int mapno,ctr;

if(savegame_no == -666)
	return Restart();

if(savegame_no > 0)
	{
	// Read the savegame title and current world number
	savegame=read_sgheader(savegame_no,&mapno);
	if(savegame)
		{
		// Do we have a functional world? (reload from the menu won't)
		if(curmap->object)
			{
			irecon_printf("Please wait..\n");
			irecon_update();
			GameBusy();
			ilog_quiet("\nRELOAD\n\n");

			// Erase limbo
			for(temp=limbo.next;temp;temp=temp->next) temp->flags &= ~IS_ON;
			MassCheckDepend();

			wipe_z1();
			}

		fullrestore = 1;

		mapnumber=mapno;
		// Read in the physical map
		if(curmap->object)
			erase_curmap();
		load_map(mapnumber);
		syspocket = curmap->object; // make sure we don't trash the game!

		savegame=makemapname(mapno,savegame_no,".mz1");
		load_z1(savegame);

		// Load roof and lights
		load_z2(mapno);
		load_z3(mapno);

		// Reset tiles in case there is no TILEANIM field in .msc file
		for(ctr=0;ctr<TItot;ctr++)
			scroll_tile_reset(ctr);

		// Now load misc state file
		savegame=makemapname(0,savegame_no,".msc");
		load_ms(savegame);

		load_lightstate(mapno,savegame_no);

		// Destroy the scratch files and copy the savegame over them
		makesavegamedir(9999,1);
		copysavegamedir(savegame_no,9999);

		fullrestore = 0;
		CallVM("mainproc");
		CallVM("loadproc");

		// Ensure the light, rooftops etc are probably recalced
		CheckRoof();
		dark_mix=0;
		DarkRoof();
		}
	else
		{
		irecon_printf("Could not find savegame '%s'\n",savegame);
		return false;
		}
	ilog_quiet("RELOAD COMPLETE\n");
	return true;
	}

return false;
}



int LoadGame()
{
int savegame_no=GetLoadSaveSlot(0);
return LoadGame(savegame_no);
}


//
//  Project the map at a certain scale to the game window
//

int ZoomMap(int zoom, int roof)
{
if(zoom<1)
	zoom=1;
/*
if(zoom>32)
	zoom=32;
*/
MicroMap(mapx-(((zoom-1)*VSW)/2),mapy-(((zoom-1)*VSH)/2),zoom,roof);
//draw_sprite(swapscreen,gamewin,VIEWX,VIEWY);
gamewin->Draw(swapscreen,VIEWX,VIEWY);

return 1;
}

//
//  Ultima5-style death management (If player dies, find another volunteer)
//

void ProcessDeath_U5()
{
int ctr;
char non_vehicle_in_party = 0;

if(player->stats->hp < 0)
	CallVMnum(Sysfunc_status);			// show the player their dead HP

// Make sure we have at least one intelligent being in the party otherwise
// there might be just the car or boat they were in when they died

non_vehicle_in_party = 0;    // Assume worst-case scenario

for(ctr=0;ctr<MAX_MEMBERS-1;ctr++)
	if(party[ctr])
		if(party[ctr]->stats->hp > 0)            // Still living
			if(party[ctr]->stats->intel > 0)     // Not a vehicle
				non_vehicle_in_party=1;

// Any party members who are found to be dead get their membership revoked

for(ctr=0;ctr<MAX_MEMBERS-1;ctr++)
	if(party[ctr])
		if(party[ctr]->stats->hp <1)
			{
			irecon_printf("%s is dead!\n",partyname[ctr]);
			if(player == party[ctr])         // Sync map before player
				CentreMap(player);        // gets erased
			SubFromParty(party[ctr]);
			ctr=0;	// Move the counter back, since there will now be a hole
					// that SubFromParty has removed so we need to try again
			}

// Is there no-one at the helm of our vehicle?

if(non_vehicle_in_party == 0)
	if(player)
		if(player->stats->intel < 1)
			{
			CentreMap(player);
			SubFromParty(player);
			player=NULL;
			}
}

//
//  Ultima6-style death management (hero dies: it's all over)
//

void ProcessDeath_U6()
{
int ctr;

if(player->stats->hp < 0)
	CallVMnum(Sysfunc_status);			// show the player their dead HP

// Any party members who are found to be dead get their membership revoked
for(ctr=0;ctr<MAX_MEMBERS-1;ctr++)
	if(party[ctr])
		if(party[ctr]->stats->hp <1)
			{
			irecon_printf("%s is dead!\n",partyname[ctr]);
			if(player == party[ctr])	// Sync map before wiping player
				{
				CentreMap(player);
				player=NULL;
				}
//			ilog_printf("rm %s\n",partyname[ctr]);
			SubFromParty(party[ctr]);
			ctr=0;	// Move the counter back, since there will now be a hole
					// that SubFromParty has removed so we need to try again
			}

for(ctr=0;ctr<MAX_MEMBERS-1;ctr++)
	if(party[ctr])
		if(party[ctr]->stats->npcflags & IS_HERO)
			if(party[ctr]->stats->hp > 0)
				{
				if(!player)
					player=party[ctr];	// Current player's dead, use leader
				return;
				}

// We don't have a hero.  No hero, no game.
player=NULL;
}


void Cheat_gimme()	{
	OBJECT *temp;
	irecon_cls();
	irecon_printf("Create Object.. Enter name:\n");
	strcpy(user_input,"");
	if(irecon_getinput(user_input,32))	{
		if(getnum4char_slow(user_input)>0)	{
			temp = OB_Alloc();
			temp->curdir = temp->dir[0];
			OB_Init(temp,user_input);
			OB_SetDir(temp,CHAR_D,FORCE_SHAPE);
			MoveToMap(player->x,player->y,temp);
			CreateContents(temp);
		} else	{
			irecon_printf("unknown object\n");
		}
	}
}

void Cheat_setflag()	{

	irecon_cls();
	irecon_printf("Set flag:\n");
	strcpy(user_input,"");
	if(irecon_getinput(user_input,32))	{
		int ctr=get_user_flag(user_input);
		irecon_printf("Flag '%s' was %d, now %d\n",user_input,ctr,!ctr);
		set_user_flag(user_input,!ctr);
	}
}

void CheckSpecialKeys(int k)	{
if(k == IREKEY_F10)
	BMPshot(swapscreen);
//if(k == IREKEY_F10 | IREKEY_CTRLMOD)
//	CRASH();
if(k == (IREKEY_F10 | IREKEY_CTRLMOD))
	ithe_panic("User Break","User requested emergency quit");
if(IRE_TestKey(IREKEY_PAUSE))
	if(IRE_TestShift(IRESHIFT_CONTROL))
		ithe_panic("User Break","User requested emergency quit");
}