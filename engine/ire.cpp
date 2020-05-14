/*
        IRE game engine

Copyright (c) 2011, IT-HE Software
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

#ifndef _WIN32
	#include <unistd.h>
	#ifdef __DJGPP__
		#include <conio.h>
	#endif
#else
	#include <conio.h>
	#define ALLEGRO_USE_CONSOLE
#endif


//#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>

#include "core.hpp"
#include "console.hpp"
#include "newconsole.hpp"
#include "media.hpp"
#include "ithelib.h"
#include "oscli.hpp"      // Operating System Command Line Interpreter
#include "cookies.h"
#include "gamedata.hpp"
#include "loadfile.hpp"
#include "library.hpp"
#include "sound.h"
#include "init.hpp"
#include "linklist.hpp"
#include "database.hpp"
#include "pe/pe_api.hpp"
#include "svnrevision.h"	// If this is missing, create a blank file

// Fake the SVN revision if we don't have one
#ifndef SVNREVISION
#define SVNREVISION ""
#endif

extern void PathInit();
extern void PathTerm();

/*
 * Defines
 */

/*
 * Variables
 */

IREBITMAP *gamewin=NULL;        // Projection window
IREBITMAP *roofwin;        // Projection window
char imgcachedir[1024];
extern char *pevm_context;
extern IRECONSOLE *MainConsole;

/*
 * Functions
 */

void RFS_getescape();
static void read_config();
static void write_config();

static void BookViewer();
static void UnitTests();
static void RunGame();

extern void LoadResources(char *filename);
extern void InitVM();
extern void CallVM(char *func);
extern void initgame();
extern void startgame();
extern void LoadMap(int mapno);

#ifdef __DJGPP__
#include <crt0.h>
int _crt0_startup_flags =_CRT0_FLAG_FILL_SBRK_MEMORY | _CRT0_FLAG_FILL_DEADBEEF | _CRT0_FLAG_LOCK_MEMORY;
#endif

// Code

int main(int argc, char *argv[])
{
char fname[1024];
char homepath[1024];
int ret;
const char *backend;
unsigned char *jugid;
int jugidlen;

#ifdef FORTIFY
remove("fortify.out");
#endif

curmap = &worldcache[0]; // For multiple map support later
curmap->object=NULL;

backend=IRE_InitBackend();	// We need this ASAP

projectname[0]=0;  // Blank the project title
projectdir[0]=0;  // Blank the project title

loadingpic[0]=0; // Blank the loading screen

M_init();         // Init memory systems

party = (OBJECT **)M_get(MAX_MEMBERS,sizeof(OBJECT *));
partyname = (char **)M_get(MAX_MEMBERS,sizeof(char *));

// Cache these linked lists to speed up loading levels
LLcache_register(&MasterList);
LLcache_register(&ActiveList);

	ihome(homepath);
#ifdef __SOMEUNIX__
	sprintf(newbootlog,"%s/%s",homepath,"bootlog.txt"); 
	sprintf(oldbootlog,"%s/%s",homepath,"bootlog.prv"); 
#else
	sprintf(newbootlog,"%s\\bootlog.txt",homepath); // This could be changed later in OSCLI
	sprintf(oldbootlog,"%\\bootlog.prv",homepath); // If we need it
#endif

ilog_start(newbootlog,oldbootlog);

// Write the bootlog to both screen and file

ilog_printf("IRE game runtime, Copyright (C) 2020 IT-HE Software\n");
ilog_printf("IRE engine version: %s%s\n",IKV,SVNREVISION);
ilog_printf("Build date: %s at %s\n",__DATE__,__TIME__);
ilog_printf("IT-HE lib version: %s\n",ithe_get_version());
ilog_printf("%s\n",backend);
//ilog_printf("home: %s\n",homepath);

// Start up the script compiler.  Doing this here will allow constants to
// be declared in the .ini files.
compiler_init();

// Now get the config settings (this may print stuff if RARfiles are added)

if(argc>=2)
	OSCLI(argc,argv);       // input from the OS commandline interpreter

// Okay, let's look for the user config file
/*
sprintf(fname,"%s%s",homepath,"game.ini");
if(!INI_file(fname))
	INI_file("game.ini"); // Okay, try current directory then
*/

TryINI("jug.ini"); // Optional

if(!TryINI("game.ini"))
	ithe_panic("Could not open game.ini",NULL);

// Okay, give assembler status
#ifdef NO_ASM
	ilog_printf("Assembler: Using C instead\n");
#else
	if(ire_NOASM)
		ilog_printf("Assembler: Disabled\n");
	else
		ilog_printf("Assembler: %s\n",ASM);
#endif

ilog_printf("\n");

// Now we need to find the main .ini file to tell us what to do.

// Do we have a project directory, passed from the commandline?
ret=0;
if(!projectdir[0]) {
	// No, assume it's the current directory then
	strcpy(projectdir,"./");
}

sprintf(fname,"%sgamedata.ini",projectdir);
ret=INI_file(fname);
if(!ret) {
	ret=INI_file("gamedata.ini");
}

// Final checks
if(!projectname[0]) {
	ithe_safepanic("Can't find gamedata.ini","There should be a -project <project> line in one of the INI files.");
}

// Use the project directory for all file accesses
ifile_prefix=&projectdir[0];


#ifndef _WIN32
if(!bookview[0])  { // Book viewer wants fast startup
	sleep(1);    // Wait a little so the user can see the boot prompt
}
#endif

init_media(1); // Enter graphics mode now, for graphical popup

ilog_printf("Loading game: %s\n",projectname);
if(fileexists("jug.id")) {
	jugid = iload_file("jug.id",&jugidlen);
	if(jugid) {
		ilog_printf("%s",(const char *)jugid);
		M_free(jugid);
	}
}

ilog_printf("\n");
#ifndef _WIN32
sleep(1);
#endif

// Now do the map data calculations
VSA=VSW*VSH;		// Viewscreen area
VSMIDX=VSW/2;		// Viewscreen X middle
VSMIDY=VSH/2;		// Viewscreen Y middle
VSW32=VSW*32;		// Viewscreen width in tiles
VSH32=VSH*32;		// Viewscreen height in tiles
VSA32=VSW32*VSH32;	// Viewscreen area in tiles

PathInit();	// Preallocate data for the pathfinder

// Check the mapfile is there before we commit ourselves
ilog_printf("\n");

if(!mapnumber)
	ithe_safepanic("No map file was specified, or invalid map number.","There should be a -map <n> line in one of the INI files.");

// Load in all the fonts, so we can go graphical..
irecon_loadfonts();

init_enginemedia(); // Finish graphics setup for game

// Work out the name of the image cache directory
sprintf(imgcachedir,"cache_%s_%d",projectname,ire_bpp);
char temp[1024];
#ifdef _WIN32
	sprintf(temp,"%s\\irecache.sdb",homepath);
#else
	ret=snprintf(temp,sizeof(temp),"%s/irecache.sdb",homepath);
	if(ret > 0) {
		temp[ret]=0;
	}
#endif
if(InitSQL(temp) != 0) {
	ithe_safepanic("Could not create cache database at",temp);
}

CheckCacheTable();

CreateCacheTable();

InitConsoles();

ilog_printf("Console started\n");
ilog_printf("\n");

// Play some startup music if any
if(fileexists("startup.ogg"))
	StreamSong_start("startup.ogg");

if(!fileexists("resource.txt")) {
	if(fileexists("main.txt")) {
		ithe_panic("Resource file not found..","Please rename your main.txt file to resource.txt");
	} else {
		ithe_panic("Resource file not found..","File resource.txt is missing!");
	}
}

ilog_printf("Load Resources\n");
LoadResources("resource.txt");

DefaultCursor = MakeIRECURSOR();

// Look for a mouse pointer (optional)
if(loadfile("mouseptr.cel",fname)) {
	IREBITMAP *tempspr=iload_bitmap(fname);
	if(!tempspr) {
		// Stop if the promise couldn't be delivered
		ithe_panic("Could not load mouse pointer",fname);
	}
	MouseOverPtr=MakeIRECURSOR(tempspr,0,0);
	delete tempspr;
}

ilog_printf("Init Random Number Generator\n");
srand(time(0));                 // Initialise rng
qrand_init();

J_Free(); // Free journal

TermConsoles();

TermSQL();

irecon_term();

if(bookview[0]) {
	BookViewer();
} else {
	if(unit_tests) {
		UnitTests();
	} else {
		RunGame();
	}
}

PathTerm();

term_media();

if(buggy) {
	printf("There were bugs in the script code.\n");
	printf("Check 'bootlog.txt' and search for 'BUG:'\n");
}

return 0;			// Return a value to keep compiler happy
}

//END_OF_MAIN();

/*
 *  Run the game
 */

void RunGame()
{
if(test_console)
	{
	swapscreen->Clear(ire_black);
	irecon_init(24,386,592/8,72/8);
	long ctr=0;
	time_t t1,t2;
	for(ctr=0;ctr<8;ctr++)
		irecon_printf("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\n");
	
	ctr=0;
	t1=time(NULL);
	do	{
		MainConsole->FastUpdate(); // No swapscreen rendering
		ctr++;
		t2=time(NULL);
		} while(t2-t1 < 60);
	irecon_cls();
	char msg[128];
	sprintf(msg,"%ld console ops/sec\n",ctr/60);
	ithe_panic(msg,NULL);
	}

// Ensure we have a valid console for this bit
swapscreen->Clear(ire_black);
irecon_init(conx,cony,conwid,conlen);

ilog_printf("Load Sound and Music\n");
S_Load();

read_config();

ilog_printf("VRM fixups\n");
Init_Funcs();                   // Assign the CHlist.func to appropriate VRM

ilog_printf("Starting PEVM microkernel\n");
InitVM();

ilog_quiet("Creating temporary savegame\n");
makesavegamedir(9999,1);

ilog_quiet("Engine pre-init\n");
initgame();
ire_running=1; // Ensure game is running

ilog_quiet("Calling PE: initproc\n");
pevm_context="Pre-game startup";

// Tear down the old console and build a new one, or not
irecon_term();
swapscreen->Clear(ire_black);
ilog_break=0;	//disable ESC, now the engine is officially running

if(!SkipInit)
	{
	// Call the init procedure
	CallVM("sys_initproc");
	}
else
	{
	// Do some initialisation ourselves
	LoadBacking(swapscreen,"backings/panel.png");
	}

// Create the console anyway.  If we already have, it will be ignored
irecon_init(conx,cony,conwid,conlen);

// If user quit, don't launch the game core
if(!ire_running)
	return;

if(ire_running != -1) // If -1, we loaded an existing game
	{
	ilog_printf("Load Map\n");
	mapnumber = default_mapnumber;
	LoadMap(mapnumber);                   // Load in the game map
	}

LoadBacking(swapscreen,"backings/panel.png");

// Okay, start the game up

ilog_printf("Start Game\n");

startgame();                     // Do things
write_config();
ilog_printf("Stop Game\n");
}

/*
 *  Run the engine as a book viewer
 */

void BookViewer()
{
irecon_init(conx,cony,conwid,conlen);

// Fake the entries that the engine might use for books
player=OB_Alloc();
victim=OB_Alloc();
person=player;
current_object=victim;

if(bookview2[0])
	talk_to(bookview,bookview2);
else
	talk_to(bookview,NULL);
}


/*
 *  Run some script tests
 */

void UnitTests()
{
irecon_init(conx,cony,conwid,conlen);

// Fake the entries that the engine might use
player=OB_Alloc();
victim=OB_Alloc();
person=player;
current_object=victim;

ilog_printf("VRM fixups\n");
Init_Funcs();                   // Assign the CHlist.func to appropriate VRM

ilog_printf("Starting PEVM microkernel\n");
InitVM();

CallVM("sys_unittests");

// If this did happen, the program would have quit by now
printf("No test failures!\n");
}




/*
 *      read_config - get previous volume levels from file
 */

void read_config()
{
char string[128];
FILE *config;

// Make filename to store volume settings
ihome(string);
strcat(string,"/volume.cfg");

config = fopen(string,"rb");
if(!config)
	return;

mu_volume = getw(config);
sf_volume = getw(config);

S_MusicVolume(mu_volume);    // Update sound engine
S_SoundVolume(sf_volume);

fclose(config);
}

/*
 *      write_config - write current volume levels to file
 */

void write_config()
{
char string[1024];
FILE *config;

// Make filename to store volume settings
ihome(string);
strcat(string,"/volume.cfg");

config = fopen(string,"wb");
if(!config)
	return;

putw(mu_volume,config);
putw(sf_volume,config);

fclose(config);
}


void RFS_getescape()
{
while(IRE_NextKey(NULL) != IREKEY_ESC);
}


/*
 *      LoadBacking - load a picture into a pre-existing bitmap
 */

void LoadBacking(IREBITMAP *img,char *fname)
{
char filename[1024];
IREBITMAP *x;

if(!loadfile(fname,filename))
	{
	Bug("Cannot open image file '%s'\n",fname);
	return;
	}

x = iload_bitmap(filename);
if(!x)
	{
	Bug("Cannot load image file '%s': unsupported format?\n",filename);
	return;
	}
  
x->DrawSolid(img,0,0);  
delete x;
}


