
/*
 *    Operating System Command Line Interpreter
 */

#include <stdio.h>
#include <stdlib.h>

// MR: added
#ifndef _WIN32
#include <strings.h>
#endif

#include <string.h>
#include "media.hpp"
#include "oscli.hpp"
#include "console.hpp"
#include "ithelib.h"
#include "loadfile.hpp"
#include "cookies.h"
#include "core.hpp"

#include "init.hpp"

#define MAXJUGS 16

// Defines
#define find(xxx) OSCLI_find(ArgC,ArgV,xxx)
#define found(xxx) OSCLI_found(ArgC,ArgV,xxx)

// Variables:  These are all controlled by commandline switches

// Mapfile, filename of the current map, begins with 0xff if not defined
char projectname[]=COOKIE_MapName;
VMINT default_mapnumber=0; // default map number
char newbootlog[256];
char oldbootlog[256];
char loadingpic[256];
int  logx=0,logy=0,loglen=60;  // Graphical boot log, X,Y and length in lines
int  conx=0,cony=0,conlen=60,conwid=80;
int journalstyle=0;

char probeInvalidObject=0;
int VideoMode=-1;
char debug_nosound=0;
char no_panic=0;
char devpaths=0;
char test_console=0;
char ire_showfuncs=0;
char ire_U6partymode=0; // Ultima5 or Ultima6 style of party management?
char ire_NOASM=0;
bool ire_checkcache=false;
bool ire_recache=false;
bool use_jugfiles=false;
char show_vrm_calls=0;
int graflog=-1;        // Graphical bootlog, true or false
char quickstart=0;      // Prevent logging to disk
char mus_chan=16;       // Music channels
char sfx_chan=4;        // Sound channels
char use_light=1;       // Use lighting by default
char vis_blanking=1;    // Use visibility blanking by default
char fixwalls=1;    	// Correct visibility blanking by default
char skip_unk=0;        // Don't skip unknown keywords in the scriptfile
int EggLockout=25;
int EggDistance=11;
VMINT map_W=512,map_H=512;        // Map size
int driftlevel=20;      // Frequency change in Hz times -10 to +10
char reset_stats=0;
char reset_funcs=0;
char enable_censorware=0;
int destroyhp=-100;     // When this number is reached, object is obliterated
char show_imap=0;       // Show a map of the tracking engine
int blankercol=-1;
int imapx=384,imapy=240;
int ig_min,ig_hour,ig_day,ig_month,ig_year,ig_dow;
int turns_min=0,min_hour=60,hour_day=24,day_week=7,day_month=30,month_year=12;
VMINT speechfont=0;
int torch_bonus=0; // If lightsources are onscreen, remove total darkness
char DebugVM=0;
char in_editor; // Which program we are in (some modules are shared)
char use_hw_video=0;
static char gamespec=0;
char allow_breakout=0;
char debug_act=0;
char SkipInit=0;
char ShowRanges=0;
char bookview[256];
char bookview2[256];
char editarea[256];
char projectdir[256];
unsigned char dlink_r=0,dlink_g=80,dlink_b=190;		// Default link colour
unsigned char dtext_r=255,dtext_g=255,dtext_b=255;	// Default conv. colour
static char *juglist[MAXJUGS];

int StartingTag=0;
int vcpu_memory=0;
unsigned int FRAMERATE=12;
unsigned int MouseWait=60;
char path_rar[1024];

// Limits for the script editor

extern long spr_alloc;
extern long seq_alloc;
extern long vrm_alloc;
extern long chr_alloc;
extern long til_alloc;
extern long wav_alloc;
extern long mus_alloc;
extern long pef_alloc;
extern long tab_alloc;
extern long tli_alloc;

// Functions

static short OSCLI_find(short ArgC,char **ArgV,char *param);
static char OSCLI_found(short ArgC,char **ArgV,char *param);
void OSCLI(int ArgC,char **ArgV);
extern void AddScriptConstant(const char *name, VMINT value);
static int CopyGameINI();

// Code

// OSCLI, process a list of switches

void OSCLI(int ArgC,char **ArgV)
{
int cpt,rgb;
char *pos;
char *value;

if(found("version") || found("v"))
	exit(1);

if(found("?") || found("h") || found("help"))
	{
	printf("\n");
	puts("-list [listfile]    Execute a list of commands (like @ in doom)");
	puts("-game [path]        Path to look for a game project in");
	puts("-window		  Run in a window rather than fullscreen");
	puts("-mapfile [number]   Load a different map number");
	puts("-nopanic            no fancy error messages");
	puts("-debugVM            Allow debugging keys in the PE Virtual Machine");
	puts("-debugAct           Debug activity system");
	puts("-videomode          Set number of bpp, or 0 to ask user (recommended)");
	puts("-nolight            Don't do any lighting (for slow systems)");
	puts("-skipunknowns       Ignore unknown keywords in the script, don't stop");
	puts("-show_imap          Display a map of the pathfinder engine");
	puts("-showfuncs          Write all script function names to IREFUNCS.TXT and stop");
	puts("-noasm              Disable assembler routines");
	puts("-checkcache	  Check image cache is up-to-date (slightly slower)");
	puts("-recache		  Rebuild image cache");
	puts("-jug <file[s]>      Load game data from one of more .jug files");
if(in_editor)
		{
		puts("-reset_stats        Reset all objects in the editor to their default statistics");
		puts("-reset_funcs        Reset all objects in the editor to their default functions");
		puts("-area <object>      Calculate size or area of a large object");
		}
else
		{
		puts("-showranges         Show mouse regions in red for mouse debugging");
		puts("-book <filename>    View a book or conversation (for book editing)");
		puts("-noinit             Skip 'initproc' (to bypass startup menu)");
		puts("-testconsole        Font routines performance test (long!)");
		}
//	puts("-L                  Display license");
	puts("-?                  This screen");
	puts("\nTo cause a stack trace on a Panic, set SUPERPANIC=1 in your environment space");
//	puts("\nTo send ilog_quiet to the printer set LPTDEBUG=1 in your environment space");
	exit(1);
	}

if(found("L"))
	{
	puts("The IRE game engine and editor are supplied under the terms");
	puts("of the BSD license, which means you can do pretty much what");
	puts("you like with them.  For full details, see the 'license.txt'");
	puts("file which should have been included in the package.");
	puts("\n");
	puts("The game data and artwork may have a different license.");
	exit(0);
	}

cpt=find("map");
if(cpt>=0&&!default_mapnumber)
	if(ArgV[cpt+1])
		default_mapnumber = atoi(ArgV[cpt+1]);

cpt=find("edit");
if(cpt>=0)
	if(ArgV[cpt+1])
		default_mapnumber = atoi(ArgV[cpt+1]);

cpt=find("framerate");
if(cpt>=0)
	if(ArgV[cpt+1])
		FRAMERATE = atoi(ArgV[cpt+1]);

cpt=find("mousewait");
if(cpt>=0)
	if(ArgV[cpt+1])
		MouseWait = atoi(ArgV[cpt+1]);

if(found("nopanic"))
    no_panic = 1;

if(found("skipunknowns"))
    skip_unk= 1;

if(found("noinit"))
    SkipInit = 1;

if(found("debugvrm") || found("debugvrms"))
    show_vrm_calls= 1;

if(found("debugvm"))
    DebugVM= 1;

if(found("debugact"))
    debug_act = 1;

if(found("nolight"))
    use_light = 0;

if(found("noblanking"))
    vis_blanking = 0;

if(found("nofixwalls"))
    fixwalls = 0;

if(found("nosound"))
    debug_nosound = 1;

if(found("break"))
    allow_breakout = 1;

if(found("reset_stats"))
    reset_stats = 1;
if(found("reset_funcs"))
    reset_funcs = 1;

if(found("censorware"))
	enable_censorware = 1;

if(found("hardware_video"))
	use_hw_video = 1;

if(found("showranges"))
	ShowRanges=1;

if(found("testconsole"))
    test_console = 1;

if(found("noasm"))
    ire_NOASM = 1;

if(found("recache"))
    ire_recache = true;

if(found("checkcache"))
    ire_checkcache = true;

// Get the project title
cpt=find("project");
if(cpt>=0) {
	if(ArgV[cpt+1]) {
		SAFE_STRCPY(projectname,ArgV[cpt+1]);
	}
}

// Look for JUGfiles and mount them on the VFS layer if found
if(!juglist[0]) {
	cpt=find("jug");
	if(cpt>=0) {
		int jugctr=0;
		for(int ctr=cpt+1;ctr<ArgC;ctr++) {
			if(ArgV[ctr]) {
				juglist[jugctr++] = ArgV[ctr];
			}
			if(jugctr >= MAXJUGS) {
				jugctr=MAXJUGS-1;
				break;
			}
		}

		if(jugctr > 0) {
			jug_mountfiles(juglist,jugctr);
			use_jugfiles=true;
		}
	}
}


if(!gamespec)
	{
	cpt=find("game");
	if(cpt>=0)
		if(ArgV[cpt+1])
			{
			SAFE_STRCPY(projectdir,ArgV[cpt+1]);
			strslash(projectdir);
			// There must be a slash at the end
			if(projectdir[strlen(projectdir)-1] != '/')
				strcat(projectdir,"/");
			gamespec=1;
			}
	}

if(VideoMode==-1)
	{
	cpt=find("VideoMode");
	if(cpt>=0)
		if(ArgV[cpt+1])
			VideoMode=atoi(ArgV[cpt+1]);
	}

// Shorthand for emergency start (handy for GDB under X11)

if(found("v0"))
	VideoMode=0;

// Shorthand for Windowed mode (X11 or GDI only at present)
if(found("vx") || found("window") || found("windowed"))
	VideoMode=-2;

cpt=find("console_x");
if(cpt>=0)
	{
	if(ArgV[cpt+1])
		conx=atoi(ArgV[cpt+1]);
	}

cpt=find("console_y");
if(cpt>=0)
    {
    if(ArgV[cpt+1])
  	cony=atoi(ArgV[cpt+1]);
    }

cpt=find("console_w");
if(cpt>=0)
    {
    if(ArgV[cpt+1])
  	conwid=atoi(ArgV[cpt+1]);
    else
        conwid = 900;
    if(conwid > 80)
        ithe_panic("Usage: -console_w <width>","Width must be in characters, 2 to 80.  It is NOT in pixels.");
    }

cpt=find("console_h");
if(cpt>=0)
    {
    if(ArgV[cpt+1])
  	conlen=atoi(ArgV[cpt+1])-1;
    else
        conlen = 900;   // Not valid, therefore print the message below.
    if(conlen > 60)
        ithe_panic("Usage: -console_h <height>","Height must be in lines, 2 to 60.  It is NOT in pixels.");
    }

cpt = find("speechfont");
if(cpt>=0) {
	speechfont = atol(ArgV[cpt+1]);
}

cpt = find("torch_bonus");
if(cpt>=0) {
	torch_bonus = atol(ArgV[cpt+1]);
}

cpt=find("loadfont");
if(cpt>=0)
	if(ArgV[cpt+1])
		irecon_registerfont(ArgV[cpt+2],atoi(ArgV[cpt+1]));

cpt=find("driftlevel");
if(cpt>=0)
    {
    if(ArgV[cpt+1])
  	driftlevel=atoi(ArgV[cpt+1]);
    }

cpt=find("map_width");
if(cpt>=0)
    {
    if(ArgV[cpt+1])
  	map_W=atoi(ArgV[cpt+1]);
    }

cpt=find("map_height");
if(cpt>=0)
    {
    if(ArgV[cpt+1])
  	map_H=atoi(ArgV[cpt+1]);
    }

cpt=find("view_w");
if(cpt>=0)
	{
	if(ArgV[cpt+1])
		VSW=atoi(ArgV[cpt+1]);
	}

cpt=find("view_h");
if(cpt>=0)
	{
	if(ArgV[cpt+1])
		VSH=atoi(ArgV[cpt+1]);
	}

cpt=find("loadingpic");
if(cpt>=0 && graflog ==-1)
	{
	if(ArgV[cpt+1])
		{
		SAFE_STRCPY(loadingpic,ArgV[cpt+1]);
		}
	else
		ithe_safepanic("Usage: -loadingpic MYFILE.PCX","The picture can be PCX, or JPEG");
	}

cpt=find("loadingpic_console");
if(cpt>=0)
	{
	if(ArgV[cpt+1])
		logx=atoi(ArgV[cpt+1]);
	if(ArgV[cpt+2])
		logy=atoi(ArgV[cpt+2]);
	if(ArgV[cpt+3])
		loglen=atoi(ArgV[cpt+3]);
	if(logx>128)
		logx=4;
	if(logy+(loglen*8)>480)
		logy=4;
	if(loglen>60)
                loglen=8;
	}

// Book viewer utility (game program)

cpt=find("book");
if(cpt>=0 && in_editor == 0)
	{
	if(ArgV[cpt+1])
		{
		SAFE_STRCPY(bookview,ArgV[cpt+1]);
		}
	else
		ithe_safepanic("Usage: -book path/bookfile.txt",NULL);

	if(ArgV[cpt+2])
		{
		SAFE_STRCPY(bookview2,ArgV[cpt+2]);
		}
	}

// Area calculator utility (editor program)

cpt=find("area");
if(cpt>=0 && in_editor == 1)
	{
	if(ArgV[cpt+1])
		strcpy(editarea,ArgV[cpt+1]);
	else
		ithe_safepanic("Usage: -area object","The object must be an entry in Section: Characters");
	}


if(found("quickstart"))
    quickstart=1;

if(found("u6party") || found("newparty") || found("u6partymode") || found("newpartymode"))
    ire_U6partymode=1;

if(found("showfuncs"))
    ire_showfuncs=1;

if(found("show_imap"))
    show_imap = 1;
cpt=find("imapxy");
if(cpt>=0)
	{
	if(ArgV[cpt+1])
		imapx=atol(ArgV[cpt+1]);
	if(ArgV[cpt+2])
		imapy=atol(ArgV[cpt+2]);
	}

if(found("probe_ivo"))
    probeInvalidObject= 1;

cpt=find("destroy_hp");
if(cpt>=0)
if(ArgV[cpt+1])
  	destroyhp = atoi(ArgV[cpt+1]);

cpt=find("journal_style");
if(cpt>=0)
if(ArgV[cpt+1])
  	journalstyle = atoi(ArgV[cpt+1]);

// Set VCPU memory size

cpt = find("vcpu_memory");
if(cpt>=0)
	vcpu_memory = atoi(ArgV[cpt+1]);


// Set limits for the script editor

cpt = find("max_sprites");
if(cpt>=0)
    spr_alloc = atol(ArgV[cpt+1]);

cpt = find("max_sequences");
if(cpt>=0)
    seq_alloc = atol(ArgV[cpt+1]);

cpt = find("max_vrms");
if(cpt>=0)
    vrm_alloc = atol(ArgV[cpt+1]);

cpt = find("max_characters");
if(cpt>=0)
    chr_alloc = atol(ArgV[cpt+1]);

cpt = find("max_tiles");
if(cpt>=0)
    til_alloc = atol(ArgV[cpt+1]);

cpt = find("max_wavs");
if(cpt>=0)
    wav_alloc = atol(ArgV[cpt+1]);

cpt = find("max_music");
if(cpt<0)
	cpt = find("max_mods");
if(cpt>=0)
    mus_alloc = atol(ArgV[cpt+1]);

cpt = find("max_pe_files");
if(cpt>=0)
    pef_alloc = atol(ArgV[cpt+1]);

cpt = find("max_tables");
if(cpt>=0)
    tab_alloc = atol(ArgV[cpt+1]);

cpt = find("max_tilelinks");
if(cpt>=0)
    tli_alloc = atol(ArgV[cpt+1]);

// Starting date

cpt = find("start_minute");
if(cpt>=0)
    ig_min = atol(ArgV[cpt+1]);

cpt = find("start_hour");
if(cpt>=0)
    ig_hour = atol(ArgV[cpt+1]);

cpt = find("start_day");
if(cpt>=0)
    ig_day = atol(ArgV[cpt+1]);

cpt = find("start_month");
if(cpt>=0)
    ig_month = atol(ArgV[cpt+1]);

cpt = find("start_year");
if(cpt>=0)
    ig_year = atol(ArgV[cpt+1]);

cpt = find("start_day_of_month");
if(cpt>=0)
    ig_year = atol(ArgV[cpt+1]);

// Calendar definitions

cpt = find("turns_per_minute");
if(cpt>=0)
    turns_min = atol(ArgV[cpt+1]);

cpt = find("minutes_per_hour");
if(cpt>=0)
    min_hour = atol(ArgV[cpt+1]);

cpt = find("hours_per_day");
if(cpt>=0)
    hour_day = atol(ArgV[cpt+1]);

cpt = find("days_per_week");
if(cpt>=0)
    day_week = atol(ArgV[cpt+1]);

cpt = find("days_per_month");
if(cpt>=0)
    day_month = atol(ArgV[cpt+1]);

cpt = find("months_per_year");
if(cpt>=0)
    month_year = atol(ArgV[cpt+1]);

cpt=find("egglockout");
if(cpt>=0)
	{
	if(ArgV[cpt+1])
		EggLockout=atoi(ArgV[cpt+1]);
	}

cpt=find("eggdistance");
if(cpt>=0)
	{
	if(ArgV[cpt+1])
		EggDistance=atoi(ArgV[cpt+1]);
	}

// Set default link colour

cpt = find("linkcol");
if(cpt>=0)
	{
	pos=strchr(ArgV[cpt+1],'#');
	if(pos)
		{
		pos++;
		sscanf(pos,"%x",&rgb);   // Got it in binary.
		dlink_r=(rgb>>16)&0xff;   // get Red
		dlink_g=(rgb>>8)&0xff;    // get green
		dlink_b=rgb&0xff;         // get blue
		}
	}

// Set default conversation text colour

cpt = find("textcol");
if(cpt>=0)
	{
	pos=strchr(ArgV[cpt+1],'#');
	if(pos)
		{
		pos++;
		sscanf(pos,"%x",&rgb);   // Got it in binary.
		dtext_r=(rgb>>16)&0xff;   // get Red
		dtext_g=(rgb>>8)&0xff;    // get green
		dtext_b=rgb&0xff;         // get blue
		}
	}

// Set default blanker colour

cpt = find("blankcol");
if(cpt>=0)
	{
	pos=strchr(ArgV[cpt+1],'#');
	if(pos)
		{
		pos++;
		sscanf(pos,"%x",&blankercol);   // Got it in binary.
		}
	}

// Set a script constant

cpt = find("set");
if(cpt>=0)
	{
	value=ArgV[cpt+2];
	if(value)
		if(value[0] == '=')
			value=ArgV[cpt+3];
	if(value && ArgV[cpt+1])
		{
		rgb=atol(value);
		AddScriptConstant(ArgV[cpt+1],rgb);
		}

	}

// Find a tag and start there (handy for development)

cpt = find("startingtag");
if(cpt>=0)
	{
	if(ArgV[cpt+1])
		StartingTag=atol(ArgV[cpt+1]);
	}


// It is of vital importance that -include/list comes last, or the INI parser will
// fall over, when the globals ArgV and ArgC are clobbered.

cpt=find("include");
if(cpt>=0)
	if(ArgV[cpt+1])
		{
		INI_file(ArgV[cpt+1]);
		return;
		}

cpt=find("list");
if(cpt>=0)
	if(ArgV[cpt+1])
		INI_file(ArgV[cpt+1]);
return;
}

short OSCLI_find(short ArgC,char **ArgV,char *param)
{
short ctr;
char tp[1024];

strcpy(tp,"-");
strcat(tp,param);
//ilog_quiet("Look for %s\n",tp);
for(ctr=0;ctr<ArgC;ctr++) // was <=
	if(ArgV[ctr])
		if(istricmp(tp,ArgV[ctr])==0)
			return(ctr);

strcpy(tp,"--");
strcat(tp,param);
for(ctr=0;ctr<ArgC;ctr++)
	if(ArgV[ctr])
		if(istricmp(tp,ArgV[ctr])==0)
			return(ctr);

#if defined(__DJGPP__) || defined(_WIN32) // Only for the DOS families
strcpy(tp,"/");
strcat(tp,param);
for(ctr=0;ctr<ArgC;ctr++)
	if(ArgV[ctr])
		if(istricmp(tp,ArgV[ctr])==0)
			return(ctr);
#endif

return -1;
}

char OSCLI_found(short ArgC,char **ArgV,char *param)
{
if(OSCLI_find(ArgC,ArgV,param) >= 0)
	return 1;
return 0;
}

//
//  Parse a .INI file
//
/*
int INI_file(char *ini)
{
int words,len,ctr;
char *word[64];
char *line;
char *pos;
FILE *fp;
int32_t filelen=0;
char *buffer;
char *end;

fp=fopen(ini,"rb");
if(fp) {
	fseek(fp,0L,SEEK_END);
	filelen = ftell(fp);
	fseek(fp,0L,SEEK_SET);

	printf("OPENED FILE '%s'\n",ini);
} else {
	printf("OPENING JUG '%s'\n",ini);
	fp=jug_openfile(ini,&filelen);
	}
if(!fp) {
	printf("COULDN'T OPEN '%s'\n",ini);
	return 0;
}

buffer = (char *)M_get(1,filelen+1);
words=fread(buffer,1,filelen,fp);
fclose(fp);

printf("Read %d bytes\n",words);


printf("file content: [%s]\n",buffer);


line=buffer;
pos=line;
end=buffer + filelen;
do {
	pos=strchr(line,0x0a);
	if(pos) {
		*pos=0;
	} else {
		break;
	}
	while(line[0] >0 && line[0]<32) {
		line++;
	}
	if(!line[0]) {
		line=pos+1;
		continue;
	}
	printf("line=[%s]\n",line);

	if(line[0]!='#'&&line[0]!=';'&&line[0]!='%') {
		words=0;
		word[words++]=&line[0];
		len=strlen(line);                     
		for(ctr=0;ctr<len&&words<15;ctr++) {
			if(line[ctr]==' '||line[ctr]=='\t') {
				line[ctr]=0;
				word[words++]=&line[ctr+1];
			} else {
				if(line[ctr]<' ') {
					line[ctr]=0;
				}
			}
			OSCLI(words,word);
		}
	}
	printf("OK\n");
	line=pos+1;

} while(line<end);

M_free(buffer);
return 1;
}
*/


int INI_file(char *ini)
{
int words,len,ctr;
char linebuf[1024];
char *word[64];
char *line;
FILE *fp;
int32_t filelen=0;
long limit=0;

fp=fopen(ini,"r");
if(!fp) {
	fp=jug_openfile(ini,&filelen);
	if(!fp) {
		return 0;
	}
}

if(filelen) {
	limit=ftell(fp) + filelen;
}

do	{

	// For an INI file in a jug
	if(limit) {
		if(ftell(fp) > limit) {
			break;
		}
	}
	line=fgets(linebuf,1023,fp);

//	printf("line = [%s]\n",line);

	if(line)
		if(line[0]!='#'&&line[0]!=';'&&line[0]!='%')
			{
			words=0;
			word[words++]=&line[0];
			len=strlen(line);                     
			for(ctr=0;ctr<len&&words<15;ctr++)
				if(line[ctr]==' '||line[ctr]=='\t')
					{
					line[ctr]=0;
					word[words++]=&line[ctr+1];
					}
				else
					if(line[ctr]<' ')
						line[ctr]=0;
			OSCLI(words,word);
			}
	} while(line);
fclose(fp);

return 1;
}



//
//  Look for an INI file locally or in a shared area
//

int TryINI(char *filename)
{
char conffile[1024];
char confpath[1024];

// Look in the current directory first (for development)
if(INI_file(filename))
	return 1;

ihome(confpath);
strcpy(conffile,confpath);	
strcat(conffile,filename);	

// Look for the INI file in the home directory
if(INI_file(conffile))
	return 1;

// Try snagging one from the game directory
if(CopyGameINI())
	if(INI_file(conffile))
		return 1;

// Look in the system config directory
#ifdef __SOMEUNIX__
strcpy(conffile,PATH_FILES_CONF);
strcat(conffile,filename);
if(INI_file(conffile))
	return 1;
#endif

// Damn
return 0;
}

//
//	If the INI file doesn't exist, try and copy one from the game directory into ~/.ire
//

int CopyGameINI()
{
char srcpath[1024];
char homedir[1024];
char buffer[1024];
FILE *fpi;
FILE *fpo;
int len;

ihome(homedir);
MAKE_DIR(homedir); // make sure
strcat(homedir,"game.ini");

SAFE_STRCPY(srcpath,projectdir);
strcat(srcpath,"game.ini");

printf("Looking for config file in %s\n",srcpath);

fpi=fopen(srcpath,"rb");
if(!fpi)
	return 0;

printf("Creating config file %s\n",homedir);

fpo=fopen(homedir,"wb");
if(!fpo)	{
	fclose(fpi);
	return 0;
	}

do {
	len=fread(buffer,1,1024,fpi);
//	printf("read %d bytes\n",len);
	if(len > 0)
		fwrite(buffer,1,len,fpo);
	} while(len>0);

fclose(fpi);
fclose(fpo);
return 1;
}
