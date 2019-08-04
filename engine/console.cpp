//
//      Text console
//


#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "types.h"
#include "media.hpp"
#include "ithelib.h"
#include "console.hpp"
#include "newconsole.hpp"
#include "loadfile.hpp"
#include "sound.h"

// Defines

#define CONSOLE_MAX 128 // Maximum 128 lines (would be 1024 pixels high)
#define MAX_FONTS 16

// Variables

//extern FONT *font;
extern VMINT buggy;	// Were warnings detected?
IREFONT *curfont;
IREFONT *DefaultFont=NULL;
IREFONT *ire_font[MAX_FONTS];
static char *ire_fontfile[MAX_FONTS];
static int MouseActivated=0;
IRECONSOLE *MainConsole=NULL;

/*
static IREBITMAP *console_backing;
static int console_on=0,printline=0,MouseActivated=0;
static int console_x,console_y,console_w,console_h,console_w8,console_h8;
static int conpos;
*/
char *lastvrmcall=NULL;
char *blametrace=NULL;
int ilog_break=1;
static IRECOLOUR ire_textcolour(255,0,255); 
static IRECOLOUR prevcol(0,0,0); 
static IREFONT *prevfont=NULL;;

extern int mapx,mapy;

// Functions

void irecon_init(int x, int y, int w, int h);
void irecon_term();
void irecon_update();
void irecon_cls();
void irecon_clearline();
void irecon_newline();
void irecon_colour(int r, int g, int b);
void irecon_colour(unsigned int packed);
void irecon_print(const char *msg);
void irecon_printxy(int x, int y, const char *msg);
void irecon_mouse(int on);
void irecon_loadcol();
void irecon_savecol();
void Show();
extern "C" void ithe_userexitfunction();

static void ilog_console(const char *msg, ...);

// Code

void irecon_savecol()
{
prevcol.Set(ire_textcolour.packed);
prevfont = curfont;
}

void irecon_loadcol()
{
ire_textcolour.Set(prevcol.packed);
if(MainConsole)
	MainConsole->SetCol(prevcol.packed);
if(prevfont)
	curfont = prevfont;
}

void irecon_mouse(int on)
{
if(on)
	{
	if(!MouseActivated && swapscreen)
		swapscreen->ShowMouse();
	IRE_SetMouse(1);
	}
else
	{
	if(MouseActivated && swapscreen)
		swapscreen->HideMouse();
	IRE_SetMouse(0);
	}
MouseActivated=on;
}

/*
 *      Start up the console, set desired width and height
 */

void irecon_init(int x, int y, int w, int h)
{
ire_textcolour.Set(255,255,255); 
  
if(!DefaultFont)
	{
	DefaultFont = MakeBitfont();
	if(!DefaultFont)
		ithe_panic("Error creating default font",NULL);
	}

curfont=DefaultFont;

ilog_quiet("Create console %d,%d at %d,%d\n",w*8,h*8,x,y);

if(!MainConsole)
	{
	MainConsole = new IRECONSOLE;
	if(MainConsole)
		{
		MainConsole->Init(x,y,w,h,h);
		ilog_printf = ilog_console; // Use as default debugging output henceforth
		}
	else
		ithe_panic("Error creating GUI console!",NULL);
	}
}

/*
 *      Shut down the console, stop using the backing bitmap
 */

void irecon_term()
{
ilog_printf = ilog_text;

if(MainConsole)
	delete MainConsole;
MainConsole=NULL;
}

/*
 *      Redraw the console
 */

void irecon_update()
{
if(MainConsole)
	MainConsole->Update();
}

/*
 *      Clear the current line and start again
 */

void irecon_clearline()
{
if(MainConsole)
	MainConsole->ClearLine();
}

/*
 *      Clear the entire console
 */

void irecon_cls()
{
if(MainConsole)
	MainConsole->Clear();
}

/*
 *      Scroll the console for end-of-line, by moving everything up one
 */

void irecon_newline()
{
if(MainConsole)
	MainConsole->Newline();
}

/*
 *      Set the current text colour
 */

void irecon_colour(int r, int g, int b)
{
ire_textcolour.Set(r,g,b);
if(MainConsole)
	MainConsole->SetCol(r,g,b);
}

void irecon_colour(unsigned int packed)
{
ire_textcolour.Set(packed);
if(MainConsole)
	MainConsole->SetCol(packed);
}

void irecon_getcolour(int *r, int *b, int *g)
{
if(!r || !g || !b)
	return;
*r=ire_textcolour.r;
*g=ire_textcolour.g;
*b=ire_textcolour.b;
}



/*
 *      Set the current text colour for multicoloured fonts
 */

void irecon_colourfont()
{
}

/*
 *      Add a simple line of text (no formatting)
 */

void irecon_print(const char *msg)
{
if(MainConsole)
	MainConsole->Print(msg);
}

void irecon_printf(const char *msg, ...)
{
char linebuf[MAX_LINE_LEN];

va_list ap;

va_start(ap, msg);

#ifdef _WIN32
_vsnprintf(linebuf,MAX_LINE_LEN,msg,ap);          // Temp is now the message
#else
	#ifdef __DJGPP__
		vsprintf(linebuf,msg,ap);
	#else
		vsnprintf(linebuf,MAX_LINE_LEN,msg,ap);
	#endif
#endif

if(MainConsole)
	MainConsole->Printf(linebuf);

va_end(ap);
}


/*
 *  Write a string direct to screen, not via the console
 */

void irecon_printxy(int x,int y,const char *msg)
{
char line[MAX_LINE_LEN];
SAFE_STRCPY(line,msg);

strdeck(line,'\n');
strdeck(line,'\r');
if(curfont)
	curfont->Draw(swapscreen,x,y,&ire_textcolour,NULL,msg);
}


/*
 *  Set font to use
 */

int irecon_font(int fnum)
{
// Default to system font
curfont=DefaultFont;

// If the font is out of range, abort
if(fnum<1 || fnum>MAX_FONTS)
	{
	if(fnum != 0)
		ilog_quiet("Font %d: doesn't exist\n",fnum);
	return curfont->Height();
	}

// If the font seems to exist but doesn't, abort
if(!ire_font[fnum])
	{
	ilog_quiet("Font %d: doesn't really exist\n",fnum);
	ilog_quiet("Font %d: fname = %s\n",fnum,ire_fontfile[fnum]);
	return curfont->Height();
	}

curfont=ire_font[fnum];
return curfont->Height();
}

// Work out which font this is

int irecon_getfont()
{
int ctr;
for(ctr=0;ctr<MAX_FONTS;ctr++)
	if(ire_font[ctr] == curfont)
		return ctr;
return -1;
}


/*
 *  Master screen update function.  Copies swap screen to the physical screen.
 */

void Show()
{
if(swapscreen)
	swapscreen->Render();
}

/*
 *  Simple screen update function.  Same as Show, but without mouse support.
 */

void ShowSimple()
{
if(swapscreen)
	swapscreen->Render();
}


/*
 *      User-level logging function for the debugger
 */

void ilog_console(const char *msg, ...)
{
char buffer[MAX_LINE_LEN];

va_list ap;
va_start(ap, msg);
vsprintf(buffer,msg,ap);
va_end(ap);

if(ilog_break)
	if(IRE_TestKey(IREKEY_ESC))
		{
		ilog_break=0;
		ithe_userexitfunction();
		exit(1);
		}
ilog_quiet(buffer);
irecon_print(buffer);
irecon_update();
}


/*
 *  Register a font for later loading
 */

void irecon_registerfont(const char *filename, int no)
{
int len;

// Too many fonts
if(no>MAX_FONTS)
	{
	printf("Error loading font %d - max font no is %d\n",no,MAX_FONTS);
	return;
	}

// System font!
if(no<1)
	{
//	printf("Error loading font %d - font 0 is reserved\n",no);
	return;
	}

len=strlen(filename)+1;
ire_fontfile[no]=(char *)M_get(1,len);
strcpy(ire_fontfile[no],filename);

//printf("Registered font %d:%s\n",no,filename);
return;
}

/*
 *  Load in a font
 */

int irecon_loadfont(int no)
{
char outname[1024];
  
// Too many fonts
if(no>MAX_FONTS)
	{
	ilog_quiet("Error loading font %d - max font no is %d\n",no,MAX_FONTS);
	return 0;
	}

// System font!
if(no<1)
	{
	ilog_quiet("Error loading font %d - font 0 is reserved\n",no);
	return 0;
	}

// Font not registered!
if(!ire_fontfile[no])
	{
	ilog_quiet("Error loading font %d - internal error\n",no);
	return 0;
	}

// Already used
if(ire_font[no])
	{
	ilog_quiet("Error loading font %d:%s - already loaded\n",no,ire_fontfile[no]);
	return 0;
	}

unsigned int header;
int len=0;
unsigned char *fontdata = iload_file(ire_fontfile[no],&len);
memcpy(&header,fontdata,4);

// Try as a bytefont first
if(header == IRE_FNT1 || header == IRE_FNT2)
	{
	ire_font[no] = MakeBytefont(ire_fontfile[no]);
	if(ire_font[no])
		return 1;
	return 0;
	}

// Now try a bitfont
if(len == 2048)
	{
	ire_font[no] = MakeBitfont(ire_fontfile[no]);
	if(ire_font[no])
		return 1;
	return 0;
	}

return 0;
}

/*
 *  Load all fonts in
 */

void irecon_loadfonts()
{
int ctr;
for(ctr=0;ctr<MAX_FONTS;ctr++)
	if(ire_fontfile[ctr])
		{
		ilog_quiet("Font %d: %s\n",ctr,ire_fontfile[ctr]);
		irecon_loadfont(ctr);
		}
}


/*
 *      get_num - User types in a number, animate things in the background
 */

int irecon_getinput(char *buffer, int maxlen)
{
int input=0;
int ascii=0;
int bufpos=0;
char buf[128];

if(maxlen>127)
	maxlen=127;

strcpy(buf,buffer);
bufpos=strlen(buf);
buf[bufpos]=0;
do  {
	irecon_clearline();
	if(buf[0] != 0)
		irecon_print(buf);         // Printing an empty line makes a newline
	irecon_print("_"); // Cursor
	irecon_update();
	Show();
	if(IRE_KeyPressed())
		{
		input = IRE_NextKey(&ascii);
		if(isprint(ascii)) // If printable
			{
			if(bufpos<maxlen)
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
strcpy(buffer,buf);
return 1;
}

/*
 *      Report a minor error
 */

void Bug(const char *msg, ...)
{
char buffer[MAX_LINE_LEN];

va_list ap;
va_start(ap, msg);
vsprintf(buffer,msg,ap);
va_end(ap);

ilog_quiet("BUG: %s",buffer);

IRECOLOUR colour;
colour.Set(ire_textcolour.packed);
irecon_colour(200,0,0);
irecon_print("BUG: ");
irecon_print(buffer);
irecon_colour(colour.packed);

irecon_update();

buggy=1;
}


// Function needed for ithelib

void ithe_userexitfunction()
{
static int c=0;
if(c)
	{
	ilog_quiet("iUser Exit: already done\n");
	return;
	}

ilog_quiet("iUser Exit\n");

c=1;
ilog_quiet("Stopping graphics\n");
KillGFX();
ilog_quiet("Graphics stopped\n");
S_Term();
ilog_quiet("Sound stopped\n");
if(lastvrmcall)
	ilog_quiet("Crashed in VRM %s\n",lastvrmcall);
if(blametrace)
	ilog_quiet("%s was called by %s\n",lastvrmcall,blametrace);
}


