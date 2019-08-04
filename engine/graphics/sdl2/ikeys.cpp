//
//	SDL keyboard implementation
//

#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include "../irekeys.hpp"

extern int IRE_ReadKeyBuffer();
extern void IRE_FlushKeyBuffer();
static inline int IsModifier(int keycode);
static int getAsciiForKey(int k, int m);

#define SDLK_LAST 4096
#define STRIP_TO_FIT (SDLK_LAST-1)

static char keymap[SDLK_LAST];
static int lastkey=0;
static char lastascii=0;

// Is it a modifier, rather than something that should return a proper keycode?

int IsModifier(int keycode)
{
switch(keycode)	{
	case IREKEY_LSHIFT:
	case IREKEY_RSHIFT:
	case IREKEY_LCONTROL:
	case IREKEY_RCONTROL:
	case IREKEY_ALT:
	case IREKEY_ALTGR:
	case IREKEY_CAPSLOCK:
	case IREKEY_NUMLOCK:
	case IREKEY_SCRLOCK:
		return 1;
	default:
		return 0;
	}
}

void IRE_GetKeys()	{
	SDL_Event sdlEvent;
	int k,m;

	if( SDL_PollEvent( &sdlEvent ) )	{
		k=sdlEvent.key.keysym.sym & STRIP_TO_FIT;
		m=sdlEvent.key.keysym.mod;

		if(sdlEvent.type == SDL_KEYDOWN)	{
			keymap[k]=1;
			if(!IsModifier(k))
				lastkey=k;
			lastascii = getAsciiForKey(k,m);
		}
		if(sdlEvent.type == SDL_KEYUP)	{
			keymap[k]=0;
			if(lastkey == k)	{
				lastkey=0;
				lastascii=0;
			}
		}
	}
}

int IRE_TestKey(int keycode)
{
IRE_GetKeys();
if(keycode > 0 && keycode < IREKEY_MAX)
	return keymap[keycode];
return 0;
}

int IRE_TestShift(int shift)
{
// Get the current modifier state
int m = SDL_GetModState();

// Translate modifiers to IRE's internal format
int im=0;

if(m&KMOD_SHIFT)
	im |= IRESHIFT_SHIFT;
if(m&KMOD_CTRL)
	im |= IRESHIFT_CONTROL;
if(m&KMOD_ALT)
	im |= IRESHIFT_ALT;

// Is it the one(s) we want?
if(im & shift)
	return 1;

// No
return 0;
}

#define REPEATMAX 700

int IRE_NextKey(int *ascii)
{
int a;
while(!lastkey)	{
	IRE_GetKeys();
	usleep(500);
}

if(ascii)
	*ascii = lastascii;
a=lastkey;

for(int ctr=0;ctr<REPEATMAX;ctr++)	{
	if(!lastkey)	{
		lastkey=lastascii=0;
		break;
		}
	IRE_GetKeys();
	usleep(150); // Was 50
	}

return a;
}

int IRE_KeyPressed()
{
IRE_GetKeys();
return lastkey;
}

void IRE_ClearKeyBuf()
{
memset(keymap,0,sizeof(keymap));
lastkey=lastascii=0;
}

// Buffered keyboard input


int IRE_GetBufferedKeycode()
{
static int overrun=0;
int a;

a=IRE_KeyPressed();
if(!a)
	return 0;

// We're bypassing the internal keybuffer so add the shift modifiers ourselves
if(IRE_TestShift(IRESHIFT_SHIFT))
	a|=IREKEY_SHIFTMOD;
if(IRE_TestShift(IRESHIFT_CONTROL))
	a|=IREKEY_CTRLMOD;

for(int ctr=0;ctr<REPEATMAX;ctr++)	{
	if(!IRE_KeyPressed())	{
		IRE_ClearKeyBuf();
		overrun=0; // Key released
		return a;
		}
	usleep(50);
	}

// If the player is holding keys down, stall them for a bit to try and prevent
// overshooting.  This was tweaked until it worked nicely.
	
overrun++;

if(overrun > 1)
	overrun=0;

if(overrun)
	return 0;

return a;
}

void IRE_FlushBufferedKeycodes()
{
}


int getAsciiForKey(int k, int m)	{
	return 0;
}
