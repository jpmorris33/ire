// //
//  Core media initialisation
//

#include <SDL.h>
#include <SDL/SDL_video.h>
#include <SDL/SDL_version.h>
#include "../../ithelib.h"
#include "../../oscli.hpp"
#include "../iregraph.hpp"

static char BackendVersion[256];
extern int MakeIREScreen(int bpp, int fullscreen);
extern void InvalidateIREScreen();
static Uint32 TimerShim(Uint32 interval, void *param);

const char *IRE_Backend()
{
return "SDL12";
}

const char *IRE_InitBackend()
{
SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER);

sprintf(BackendVersion,"libSDL: %d.%d.%d",SDL_MAJOR_VERSION,SDL_MINOR_VERSION,SDL_PATCHLEVEL);
return BackendVersion;
}


int IRE_Startup(int allow_break, int videomode)
{
int bpp=0;
const SDL_VideoInfo *vm;

ilog_printf("  Keyboard: ");
SDL_EnableUNICODE( SDL_ENABLE );
ilog_printf("OK\n");

// Set allegro panic button

ilog_printf("  Timer: ");
	ilog_printf("OK\n");

ilog_printf("  Mouse: ");
	ilog_printf("OK\n");

ilog_printf("  Video:\n");
switch(videomode)
	{

	// Windowed mode

	case -2:

	// Get colour depth
	vm=SDL_GetVideoInfo();
	if(!vm)
		bpp=32;
	else	{
		bpp=vm->vfmt->BitsPerPixel;
	}
	
	break;

	// Supported video depths (fullscreen)
	case 15:
	case 16:
	case 24:
	case 32:
	bpp=videomode;
	break;

	// Anything we don't understand

	default:
	bpp=32;
	break;
	}

int fullscreen=1;
if(videomode == -2)
	fullscreen=0;

if(MakeIREScreen(bpp,fullscreen) == 0)
	return 0;

IRE_StartGFX(bpp);

return bpp;
}


void IRE_Shutdown()
{
InvalidateIREScreen(); // Tell it the framebuffer's gone
SDL_Quit();
}

typedef void (*TIMERTYPE)();


void *IRE_SetTimer(TIMERTYPE timer, int clockhz)
{
return (void *)SDL_AddTimer(1000/clockhz,TimerShim,(void *)timer);
}

void IRE_StopTimer(void *handle, void (*timer)())
{
SDL_RemoveTimer((SDL_TimerID)handle);
}

Uint32 TimerShim(Uint32 interval, void *param)	{
TIMERTYPE callback=(TIMERTYPE)param;
callback();
return interval;
}
