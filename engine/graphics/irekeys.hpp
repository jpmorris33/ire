#ifdef USE_ALLEGRO4
#include "allegro4/keylist.h"
#endif

#ifdef USE_SDL12
#include "sdl/keylist.h"
#endif

#ifdef USE_SDL20
#include "sdl2/keylist.h"
#endif

#define IREKEY_MOUSE (IREKEY_MAX+1)

#define IREKEY_SHIFTMOD 0x8000
#define IREKEY_CTRLMOD 0x10000

extern void IRE_StopKeyboard();
extern void IRE_GetKeys();
extern int IRE_TestKey(int keycode);
extern int IRE_TestShift(int shift);
extern int IRE_NextKey(int *ascii);
extern int IRE_KeyPressed(void);
extern void IRE_ClearKeyBuf(void);

// Our internal keyboard ring buffer
extern int IRE_GetBufferedKeycode();
extern void IRE_FlushBufferedKeycodes();
