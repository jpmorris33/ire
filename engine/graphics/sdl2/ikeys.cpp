//
//	SDL keyboard implementation
//

#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include "../iregraph.hpp"
#include "../irekeys.hpp"

extern int IRE_ReadKeyBuffer();
extern void IRE_FlushKeyBuffer();
extern "C" void ithe_panic(const char *a, const char *b);
static inline int IsModifier(int keycode);
static int getAsciiForKey(int k, int m);
static int XlateMousebuttons(int input);

#define SDLK_LAST 4096
#define STRIP_TO_FIT (SDLK_LAST-1)

static char keymap[SDLK_LAST];
static int lastkey=0;
static char lastascii=0;
static char asciitab[SDLK_LAST];
static char shiftascii[256];
static int mousex,mousey,mousez;

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
	int k,m,y;

	if( SDL_PollEvent( &sdlEvent ) )	{
		k=sdlEvent.key.keysym.scancode & STRIP_TO_FIT;
		m=sdlEvent.key.keysym.mod;

		if(sdlEvent.type == SDL_KEYDOWN)	{
			keymap[k]=1;
			if(!IsModifier(k)) {
				lastkey=k;
			}
			lastascii = getAsciiForKey(k,m);

			if(keymap[IREKEY_LCONTROL] && keymap[IREKEY_PAUSE]) {
				ithe_panic("User Break","User requested emergency quit");
			}

//			printf("Keydown %d\n", k);
		}
		if(sdlEvent.type == SDL_KEYUP)	{
			keymap[k]=0;
			if(lastkey == k)	{
				lastkey=0;
				lastascii=0;
			}
		}

		if(sdlEvent.type == SDL_MOUSEMOTION)	{
			mousex = sdlEvent.motion.x;
			mousey = sdlEvent.motion.y;
		}

		if(sdlEvent.type == SDL_MOUSEWHEEL)	{
			y=sdlEvent.wheel.y;
			if(sdlEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
				y=-y;
			}
			mousez=-y;
		}

	}
}

int IRE_TestKey(int keycode)
{
IRE_GetKeys();
if(keycode > 0 && keycode < IREKEY_MAX) {
	return keymap[keycode];
}
return 0;
}

int IRE_TestShift(int shift)
{
// Get the current modifier state
int m = SDL_GetModState();

// Translate modifiers to IRE's internal format
int im=0;

if(m&KMOD_SHIFT) {
	im |= IRESHIFT_SHIFT;
}
if(m&KMOD_CTRL) {
	im |= IRESHIFT_CONTROL;
}
if(m&KMOD_ALT) {
	im |= IRESHIFT_ALT;
}

// Is it the one(s) we want?
if(im & shift)
	return 1;

// No
return 0;
}

#define REPEATMAX 75
#define  BUFFERREPEAT 10

int IRE_NextKey(int *ascii)
{
int a;
while(!lastkey)	{
	IRE_GetKeys();
	SDL_Delay(1);
//	usleep(500);
}

if(ascii) {
	*ascii = lastascii;
}
a=lastkey;

for(int ctr=0;ctr<REPEATMAX;ctr++)	{
	if(!lastkey)	{
		lastkey=lastascii=0;
		break;
		}
	IRE_GetKeys();
	SDL_Delay(1);
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

// Buffered keyboard input (Player movement etc)


int IRE_GetBufferedKeycode()
{
static int overrun=0;
int a;

//return IRE_ReadKeyBuffer();

a=IRE_KeyPressed();
if(!a) {
	return 0;
}

// We're bypassing the internal keybuffer so add the shift modifiers ourselves
if(IRE_TestShift(IRESHIFT_SHIFT)) {
	a|=IREKEY_SHIFTMOD;
}
if(IRE_TestShift(IRESHIFT_CONTROL)) {
	a|=IREKEY_CTRLMOD;
}

for(int ctr=0;ctr<BUFFERREPEAT;ctr++)	{
	if(!IRE_KeyPressed())	{
		IRE_ClearKeyBuf();
		overrun=0; // Key released
//	printf(">> SDL2: return key %d\n", a);
		return a;
		}
//	usleep(50);
	SDL_Delay(1);
	}

// If the player is holding keys down, stall them for a bit to try and prevent
// overshooting.  This was tweaked until it worked nicely.
	
overrun++;

if(overrun > 2) {
	overrun=0;
}

if(overrun) {
	return 0;
}

return a;
}

void IRE_FlushBufferedKeycodes()
{
//IRE_FlushKeyBuffer();
lastkey = 0;
}

//
//  This is not a good approach and should really be replaced with
//  SDL_StartTextInput or similar.  But it gets the editor going again.
//

int getAsciiForKey(int k, int m)	{
	int shift=0, ascii, shifted=0;

	if(k<0 || k>=SDLK_LAST) {
		return 0;
	}

	if(m&KMOD_SHIFT) {
		shift=32;
	}

	ascii = asciitab[k];
	if(shift && ascii) {
		shifted = shiftascii[ascii];
		if(shifted) {
			return shifted;
		}
	}

	return ascii;
}


void IRESDL2_InitKeys() {

asciitab[IREKEY_0] = '0';
asciitab[IREKEY_1] = '1';
asciitab[IREKEY_2] = '2';
asciitab[IREKEY_3] = '3';
asciitab[IREKEY_4] = '4';
asciitab[IREKEY_5] = '5';
asciitab[IREKEY_6] = '6';
asciitab[IREKEY_7] = '7';
asciitab[IREKEY_8] = '8';
asciitab[IREKEY_9] = '9';

asciitab[IREKEY_A] = 'a';
asciitab[IREKEY_B] = 'b';
asciitab[IREKEY_C] = 'c';
asciitab[IREKEY_D] = 'd';
asciitab[IREKEY_E] = 'e';
asciitab[IREKEY_F] = 'f';
asciitab[IREKEY_G] = 'g';
asciitab[IREKEY_H] = 'h';
asciitab[IREKEY_I] = 'i';
asciitab[IREKEY_J] = 'j';
asciitab[IREKEY_K] = 'k';
asciitab[IREKEY_L] = 'l';
asciitab[IREKEY_M] = 'm';
asciitab[IREKEY_N] = 'n';
asciitab[IREKEY_O] = 'o';
asciitab[IREKEY_P] = 'p';
asciitab[IREKEY_Q] = 'q';
asciitab[IREKEY_R] = 'r';
asciitab[IREKEY_S] = 's';
asciitab[IREKEY_T] = 't';
asciitab[IREKEY_U] = 'u';
asciitab[IREKEY_V] = 'v';
asciitab[IREKEY_W] = 'w';
asciitab[IREKEY_X] = 'x';
asciitab[IREKEY_Y] = 'y';
asciitab[IREKEY_Z] = 'z';
asciitab[IREKEY_BACKSPACE] = '\b';
asciitab[IREKEY_ENTER] = 13;
asciitab[IREKEY_ESC] = 27;

asciitab[IREKEY_SPACE] = ' ';
asciitab[IREKEY_MINUS] = '-';
asciitab[IREKEY_EQUALS] = '=';
asciitab[IREKEY_SLASH] = '/';
asciitab[IREKEY_SLASH_PAD] = '/';
asciitab[IREKEY_BACKSLASH] = '\\';
asciitab[IREKEY_BACKSLASH2] = '\\';
asciitab[IREKEY_COLON] = ';';
asciitab[IREKEY_DOT] = '.';
asciitab[IREKEY_COMMA] = ',';
asciitab[IREKEY_QUOTE] = '\'';
asciitab[IREKEY_QUOTE] = '\'';
asciitab[IREKEY_OPENBRACE] = '[';
asciitab[IREKEY_CLOSEBRACE] = ']';

shiftascii['0']=')';
shiftascii['1']='!';
shiftascii['2']='\"';
shiftascii['3']='#';
shiftascii['4']='$';
shiftascii['5']='%';
shiftascii['6']='^';
shiftascii['7']='&';
shiftascii['8']='*';
shiftascii['9']='(';

shiftascii['-']='_';
shiftascii['=']='+';
shiftascii[',']='<';
shiftascii['/']='?';
shiftascii['[']='{';
shiftascii[']']='}';
shiftascii[';']=':';
shiftascii['\'']='@';
shiftascii['\\']='|';

for(int ctr='a';ctr<='z';ctr++) {
	shiftascii[ctr]=ctr-32;
}
}


int IRE_GetMouse()
{
SDL_PumpEvents();
return 1;
}


int IRE_GetMouse(int *b)
{
SDL_PumpEvents();
int mousestate = SDL_GetMouseState(NULL,NULL);
if(b) {
	*b=XlateMousebuttons(mousestate);
}
return 1;
}


int IRE_GetMouse(int *x, int *y, int *z, int *b)
{
SDL_PumpEvents();
IRE_GetKeys();
int mousestate = SDL_GetMouseState(NULL,NULL);
if(x) {
	*x=mousex;
}
if(y) {
	*y=mousey;
}
if(z) {
	*z=mousez;
	if(mousez) {
		mousez=0;
	}
}
if(b) {
	*b=XlateMousebuttons(mousestate);
}
return 1;
}


void IRE_ShowMouse()
{
SDL_ShowCursor(1);
}

void IRE_HideMouse()
{
SDL_ShowCursor(0);
}


int XlateMousebuttons(int input)
{
int output=0;
//if(input != 0)
//	printf("mousebutton = %d\n",input);
if(input & 1) {
	output |= IREMOUSE_LEFT;
}
if(input & 2) {
	output |= IREMOUSE_LEFT|IREMOUSE_RIGHT;
}
if(input & 4) {
	output |= IREMOUSE_RIGHT;
}
//if(output != 0)
//	printf("mousebutton now = %d\n",output);
return output;
}


