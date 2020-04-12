/*
 *      Media System Initialisation
 *
 *      Start up multimedia devices and input
 */

#include <stdlib.h>
#include <string.h>
#include "ithelib.h"
#include "console.hpp"
#include "oscli.hpp"
#include "media.hpp"
#include "loadfile.hpp"
#include "mouse.hpp"
#include "sound.h"

// Defines

#define CURSOR_CHAR "_"

#ifdef _WIN32
	#define WINDOWS_IS_BROKEN
#endif

// Variables
IREBITMAP *swapscreen;
IRECOLOUR *ire_transparent,*ire_black,*tfx_Colour;
int ire_bpp,ire_bytespp;
unsigned char ire_transparent_r,ire_transparent_g,ire_transparent_b;

static IREBITMAP *pic;
static IREBITMAP *oldpic;
static char media_on=0,media_clock=0;


// Functions

extern void RFS_getescape();
extern void CheckSpecialKeys(int k);

static int rmbuf();
static void BMPshot(IREBITMAP *savescreen);
static void PNGshot(IREBITMAP *savescreen);


// Code

/*
 * Timer code and it's volatile data
 */

static void *timerhandle;
static volatile int ready_for_tick=0;   // Timer clock
static volatile unsigned int ire_uticks=0;
static volatile unsigned int ire_animticks=0;
static volatile unsigned int ire_divisor=0;
static void ire_animation_clock(void)
{
//printf("Timer microticks = %d\n",ire_uticks);
ire_uticks++;
ire_divisor++;
// Divide the animation clock by four
if(ire_divisor >= 4)
	{
	ire_animticks++;
	ready_for_tick=1;
	ire_divisor=0;
	}
}

/*
 * UpdateAnim() - returns true if it's time to update the animations
 */

int UpdateAnim()
{
if(ready_for_tick)
	{
	ready_for_tick = 0;
	return 1;
	}
return 0;
}


unsigned int GetIREClock()
{
return ire_uticks;
}

void ResetIREClock()
{
ire_uticks=0;

if(in_editor)
	return;
// Sync animation clock
ready_for_tick=0;
ire_divisor=0;
ire_animticks=0;
}

unsigned int GetAnimClock()
{
return ire_animticks;
}

void init_media(int game)
{
if(media_on)
	return;
media_on=1;

ilog_printf("Starting up:\n");

if(game && (!debug_nosound))
	{
	ilog_printf("  Sound: ");
	if(S_Init())
		ilog_printf("OK\n");
	else
		ilog_printf("FAILED\n");
	}

ire_bpp=IRE_Startup(allow_breakout,VideoMode);

// Work out bytes per pixel
ire_bytespp=ire_bpp/8;
if(ire_bpp==15)
	ire_bytespp=2; // hack for 15bpp

ilog_quiet("Primary media init succesful\n");
ilog_printf("\n");
}


//
//  Secondary media initialisation
//

void init_enginemedia()
{
char picpath[1024];
	
ilog_quiet("  Video:\n");
ilog_quiet("  Transparency\n");

ire_transparent = new IRECOLOUR(255,0,255);
ire_black = new IRECOLOUR(0,0,0);
tfx_Colour = new IRECOLOUR(0,0,0);

ire_transparent_r = ire_transparent->r;
ire_transparent_g = ire_transparent->g;
ire_transparent_b = ire_transparent->b;

if(!BlenderInit(ire_transparent,ire_bpp))
	{
	IRE_StopGFX();
	ilog_quiet("Out of memory in BlenderInit\n");
	printf("Out of memory in BlenderInit\n");
	exit(1);
	}


ilog_quiet("  Text mode\n");

swapscreen=MakeIREBITMAP(640,480);
if(!swapscreen)
	{
	IRE_StopGFX();
	ilog_quiet("Can't create swapscreen\n");
	printf("Can't create swapscreen\n");
	exit(1);
	}
swapscreen->Render();

oldpic=MakeIREBITMAP(640,480);
if(!oldpic)
	{
	IRE_StopGFX();
	ilog_quiet("Can't create save/restore bitmap\n");
	printf("Can't create save/restore bitmap\n");
	exit(1);
	}

if(loadingpic[0] == 0)
	{
	// None specified
	swapscreen->Clear(ire_black);
	Show();
	}
else
	{
	loadfile(loadingpic,picpath);
	pic = iload_bitmap(picpath);
	if(!pic)
		{
		ilog_quiet("  Oh dear\n");
		IRE_StopGFX();
		ilog_quiet("Can't load backing picture '%s'\n",picpath);
		printf("Can't load backing picture '%s'\n",picpath);
		exit(1);
		}
	swapscreen->Clear(ire_black);
	pic->Draw(swapscreen,0,0);
	delete pic;
	}

ilog_quiet("  Start console\n");
irecon_init(logx,logy,80,loglen);

ilog_quiet("  Locks\n");

media_clock=0;
// Ok, let's do it
ilog_quiet("  Clock: ");
timerhandle = IRE_SetTimer(ire_animation_clock,140); // 140Hz timebase
if(timerhandle)
	ilog_quiet("Ok\n");
else
	{
	exit(1);
	}
media_clock=1;

ilog_quiet("Media INIT successful\n");
}

void term_media()
{
if(!media_on)
	{
	ilog_quiet("Media not active, can't shut down\n");
	return;
	}
media_on=0;

ilog_quiet("Shut down Media:\n");

ilog_quiet("  Clock:\n");
if(media_clock)
	{
	IRE_StopTimer(timerhandle,ire_animation_clock);
	media_clock=0;
	}

ilog_quiet("  Gfx:\n");
KillGFX();

BlenderTerm();

ilog_quiet("  Keyboard:\n");

ilog_quiet("  Graphics backend:\n");

S_Term();

ilog_quiet("Media shutdown completed\n");
}

/*
 *      Kill the graphics and take us back to text mode
 */

void KillGFX()
{
static int c=0;
if(c)
    return;
c=1;
ilog_quiet("Kill GFX:\n");
IRE_Shutdown();
#ifdef __linux__
//c=system("reset");	// This was really for SVGAlib, probably don't need it now
#endif

ilog_quiet("Irecon-Term\n");
irecon_term();
}



int sizeof_sprite(IREBITMAP *bmp)
{
int cd=0;

if(!bmp)
	return 0;

switch(bmp->GetDepth())
	{
	case 8:
	cd=1;
	break;

	case 15:
	case 16:
	cd=2;
	break;

	case 24:
	cd=3;
	break;

	case 32:
	cd=4;
	break;

	default:
	ithe_panic("Unknown colour depth in object",NULL);
	break;
	};

return bmp->GetW()*bmp->GetH()*cd;
}



/*
 *      Get a key, if one is pending, else return 0
 */

int GetKey()
{
int z,shift;
IRE_GetMouse(NULL,NULL,&z,NULL);
if(z) {
	if(z<0) {
		return IREKEY_MOUSEUP;
	} else {
		return IREKEY_MOUSEDOWN;
	}
}

return IRE_GetBufferedKeycode();
}

int GetMouseZ(int z)
{
int shift;
shift=IRE_TestShift(IRESHIFT_SHIFT);
if(z<0) {
	if(shift) {
		return IREKEY_LEFT;
	}
	return IREKEY_UP;
}
if(z>0) {
	if(shift) {
		return IREKEY_RIGHT;
	}
	return IREKEY_DOWN;
}
return 0;
}


/*
 *      Wait for a key
 */

int WaitForKey()
{
int k,z;
do {
	k = IRE_GetBufferedKeycode();
	// look for a mouse click?

	IRE_GetMouse(NULL,NULL,&z,NULL);
	if(z<0) {
		return IREKEY_MOUSEUP;
	}
	if(z>0) {
		return IREKEY_MOUSEDOWN;
	}
	CheckMouseRanges();
	if(MouseID != -1) {
		return IREKEY_MOUSE;
	}

} while(!k);

return k;
}

unsigned char WaitForAscii()
{
int k;
do
	{
	IRE_NextKey(&k);
//	S_PollMusic();
	} while(!k);

return k;
}

// Flush the internal keybuffer

void FlushKeys()
{
IRE_FlushBufferedKeycodes();
}


/*
 *      Plot - draw the little row of dots
 *
 *             Call Plot(number); to set the maximum length of the row
 *             Then call Plot(0); each cycle of the loop to draw a dot
 */


void Plot(int max)
{
static int mpos,ctr=0;
ctr++;
if(max)
	{
	mpos=max/40;
	if(mpos<1)
		mpos=1;
	ctr=0;
	}
if(ctr>mpos)
	{
	ilog_printf(".");
	ctr=0;
	}
}


void Screenshot(IREBITMAP *savescreen) {
// PNG support is a pain in the butt, but this will help later
BMPshot(savescreen);
}


/*
 *      BMPShot - Take a screenshot in .BMP format
 */

void BMPshot(IREBITMAP *savescreen)
{
char name[1024];
char home[1024];
int ctr=0;
FILE *fp;
ihome(home);

if(!savescreen)
	return;

if(!home[0])
	strcpy(home,".");

do
	{
	sprintf(name,"%s/ire%05d.bmp",home,ctr++);
	fp=fopen(name,"rb");
	if(fp)
		fclose(fp);  // Check residual value in loop
	} while(fp);

// Create file or bail out
savescreen->SaveBMP(name);
}

#if 0
/*
 *      PNGShot - Take a screenshot in .PNG format
 */

void PNGshot(IREBITMAP *savescreen)
{
char name[1024];
char home[1024];
int ctr=0;
FILE *fp;
ihome(home);

if(!savescreen)
	return;

if(!home[0])
	strcpy(home,".");

do
	{
	sprintf(name,"%s/ire%05d.png",home,ctr++);
	fp=fopen(name,"rb");
	if(fp)
		fclose(fp);  // Check residual value in loop
	} while(fp);

// Create file or bail out
savescreen->SavePNG(name);
}
#endif


/*
 *      Get a string
 */

int GetStringInput(char *ptr,int len)
{
int input=0;
int bufpos=0;
char buf[128];
int ascii=0;

strcpy(buf,ptr);
bufpos=strlen(buf);
buf[bufpos]=0;
do  {
	irecon_clearline();
	if(buf[0] != 0)
		irecon_print(buf);         // Printing an empty line makes a newline
	irecon_print(CURSOR_CHAR);
	irecon_update();
	Show();
	if(IRE_KeyPressed())
		{
		input = IRE_NextKey(&ascii);
 		CheckSpecialKeys(input);
		if(ascii >= ' ' && ascii <= 'z')
			{
			if(bufpos<conwid && bufpos<len)
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
strcpy(ptr,buf);
return 1;
}

void SaveScreen()
{
oldpic->Get(swapscreen,0,0);
}

void RestoreScreen()
{
oldpic->Draw(swapscreen,0,0);
}
