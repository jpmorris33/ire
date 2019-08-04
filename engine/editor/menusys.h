/*

	Doom Editor Utility, by Brendon Wyber and Rapha‰l Quinet.

	You are allowed to use any parts of this code in another program, as
	long as you give credits to the authors in the documentation and in
	the program itself.  Read the file README.1ST for more information.

	This program comes with absolutely no warranty.

	DEU.H - Main doom defines.
*/

/* the includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
//#include <graphics.h>
//#include <malloc.h>
#include "../graphics/iregraph.hpp"
#include "../console.hpp"

/*
#ifdef __cplusplus
extern "C" {
#endif
*/

extern IREBITMAP *swapscreen;
extern IREBITMAP *gamewin;

typedef int Bool;               /* Boolean data: true or false */

/*
	the macros and constants
*/

/*
	the interfile global variables
*/

/* from deu.c */
//extern Bool  SwapButtons;	/* swap right and middle mouse buttons */

/* from gfx.c */
extern int GfxMode;		/* current graphics mode, or 0 for text */
extern int Scale;		/* scale to draw map 20 to 1 */
extern int OrigX;		/* the X origin */
extern int OrigY;		/* the Y origin */
extern int PointerX;		/* X position of pointer */
extern int PointerY;		/* Y position of pointer */
extern int ScrMaxX;		/* maximum X screen coord */
extern int ScrMaxY;		/* maximum Y screen coord */
extern int ScrCenterX;		/* X coord of screen center */
extern int ScrCenterY;		/* Y coord of screen center */
//extern  CURCOL;

/* from mouse.c */
extern Bool UseMouse;		/* is there a mouse driver? */

/*
	the function prototypes
*/

/* from deu.c */
//int main( int, char *[]);
void *GetMemory( size_t);
void *ResizeMemory( void *, size_t);
void *GetFarMemory( unsigned long size);
void *ResizeFarMemory( void *old, unsigned long size);

/* from gfx.c */
void InitGfx( void);
void TermGfx( void);
void ClearScreen( void);
void SetColor(IRECOLOUR *col);
void DrawScreenLine( int, int, int, int);
void DrawScreenBox( int, int, int, int);
void DrawScreenBox3D( int, int, int, int);
void DrawSunkBox3D( int, int, int, int);
void DrawScreenBoxHollow( int, int, int, int);
void DrawScreenMeter( int, int, int, int, float);
void DrawScreenText( int, int, char *, ...);
void DrawScreenChar( int Xstart, int Ystart, char msg);
void Box(int a,int b,int c,int d);

//void DrawPointer( void);

void InputNameFromListWithFunc( int, int, char *, int, char **, int, char *, int, int, void (*hookfunc)(int, int, char *));
void InputNameFromList( int, int, char *, int, char **, char *);
void InputNameFromList32( int x0, int y0, char *prompt, int listsize, char **list, char *name);
void InputNameFromListWithAdd( int x0, int y0, char *prompt, int listsize, char **list, char *name);
void GetThumbFromList( int x0, int y0, char *prompt, int listsize, char **list, char *name);
int InputInteger( int, int, int *, int, int);
int InputIntegerValue( int, int, int, int, int);
int InputIntegerValueP( int x0, int y0, int minv, int maxv, int defv, char *promptstr);
int InputIntegerValuePI( int x0, int y0, int minv, int maxv, int defv, char *promptstr);
void InputFileName( int, int, char *, int, char *);
void InputString( int, int, char *, int, char *);
void InputIString( int, int, char *, int, char *);
Bool Confirm( int, int, char *, char *);
int Notify( int, int, char *, char *);
void System(int, int, char *,char *,void (*)());
void DisplayMessage( int, int, char *, ...);
void DrawScreenMeter( int Xstart, int Ystart, int Xend, int Yend, float value);


void DisplayMessage( int x0, int y0, char *msg, ...);
void setfillstyle(int a,int b);
//void setcolor( int);
IRECOLOUR *getcolor(void);
#define fbox2(x,y,w,h,c,screen) screen->FillRect(x,y,w,h,c)

void ClipBox(int x,int y,int w,int h,IRECOLOUR *colour,IREBITMAP *screen);
void ClipCircle(int x,int y,int r,IRECOLOUR *colour,IREBITMAP *screen);

// These colours value depends on the bpp level.

// They are defined indirectly because otherwise the compiler will argue
// with me and try to impose it's corrupted colours over the proper ones.
// (They are defined in conio.h)

extern IRECOLOUR *ITG_LIGHTGRAY;
extern IRECOLOUR *ITG_LIGHTGREEN;
extern IRECOLOUR *ITG_DARKGRAY;
extern IRECOLOUR *ITG_YELLOW;
extern IRECOLOUR *ITG_RED;
extern IRECOLOUR *ITG_DARKRED;
extern IRECOLOUR *ITG_BLUE;
extern IRECOLOUR *ITG_BLACK;
extern IRECOLOUR *ITG_WHITE;

#define KEY_DELETE KEY_DEL

#define INS 82
#define DEL 83
#define ENTER 0x0d

extern char __up[];
extern char __down[];
extern char __left[];
extern char __right[];

extern char __left2[];  // Double Left
extern char __right2[]; // Double Right

#define IGUI_INVALID -2147483647
#define IGUI_MIN -2147483646
#define IGUI_MAX 2147483646

/* end of file */
/*
#ifdef __cplusplus
}
#endif
*/
