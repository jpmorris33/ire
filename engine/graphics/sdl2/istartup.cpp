// //
//  Core media initialisation
//

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_version.h>
#include "../../ithelib.h"
#include "../../oscli.hpp"
#include "../iregraph.hpp"

static char BackendVersion[256];
extern int MakeIREScreen(int bpp, int fullscreen);
extern void InvalidateIREScreen();
static Uint32 TimerShim(Uint32 interval, void *param);

const char *IRE_Backend()
{
return "SDL20";
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
ilog_printf("  Keyboard: ");
ilog_printf("OK\n");

ilog_printf("  Timer: ");
	ilog_printf("OK\n");

ilog_printf("  Mouse: ");
	ilog_printf("OK\n");

ilog_printf("  Video:\n");

bpp=32; // always

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
SDL_TimerID *id;
id=(SDL_TimerID *)M_get(1,sizeof(SDL_TimerID));
*id = SDL_AddTimer(1000/clockhz,TimerShim,(void *)timer);
return (void *)id;
}

void IRE_StopTimer(void *handle, void (*timer)())
{
SDL_TimerID *thandle=(SDL_TimerID *)handle;
SDL_RemoveTimer(*thandle);
M_free(handle);
}

Uint32 TimerShim(Uint32 interval, void *param)	{
TIMERTYPE callback=(TIMERTYPE)param;
callback();
return interval;
}
