//
//
//

#include <allegro.h>
#include "../irekeys.hpp"

extern int IRE_ReadKeyBuffer();
extern void IRE_FlushKeyBuffer();

void IRE_StopKeyboard()
{
remove_keyboard();
}

void IRE_GetKeys()
{
poll_keyboard();
}


int IRE_TestKey(int keycode)
{
poll_keyboard();
if(keycode > 0 && keycode < IREKEY_MAX)
	return key[keycode];
return 0;
}

int IRE_TestShift(int shift)
{
if(key_shifts & shift)
	return 1;
return 0;
}

int IRE_NextKey(int *ascii)
{
poll_keyboard();
int k =readkey();
if(ascii)
	*ascii = k&0xff; // ASCII code
return (k>>8)&0xff; // Scancode
}

int IRE_KeyPressed()
{
return keypressed();
}

void IRE_ClearKeyBuf()
{
clear_keybuf();
}


int IRE_GetBufferedKeycode()
{
return IRE_ReadKeyBuffer();
}

void IRE_FlushBufferedKeycodes()
{
IRE_FlushKeyBuffer();
}