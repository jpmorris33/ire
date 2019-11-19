//
//  Render fixed 8x8 1-bit fonts
//

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef _WIN32
	#include <stdint.h>
#else
	typedef unsigned int uint32_t;
	typedef unsigned short uint16_t;
#endif

#include "../../ithelib.h"
#include "../iregraph.hpp"
#include "fonts/ROMfont.h"

extern int _IRE_BPP;
extern char ire_NOASM;
static int shiftfactor=0;
static unsigned char *DefaultBitfont = ROMfont;
static void (*draw_bitfont_func)(unsigned char *buffer, unsigned char *fontdata, int w, unsigned int colour);

static void InitBitFont();
static void draw_bitfont_16c(unsigned char *buffer, unsigned char *fontdata, int w, unsigned int colour);
static void draw_bitfont_32c(unsigned char *buffer, unsigned char *fontdata, int w, unsigned int colour);
extern "C" void draw_bitfont_16asm(unsigned char *buffer, unsigned char *fontdata, int w, unsigned int colour);
extern "C" void draw_bitfont_32asm(unsigned char *buffer, unsigned char *fontdata, int w, unsigned int colour);


class BITFONT : IREFONT
	{
	public:
		BITFONT();
		BITFONT(const char *filename);
		~BITFONT();
		void Draw(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *string);
		void Printf(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *fmt, ...);
		int Length(const char *str);
		int Height();
		int IsColoured();
	private:
		unsigned char *fontdata;
	};


//
//  Bootstrap
//

IREFONT *MakeBitfont()
{
if(!draw_bitfont_func)
	InitBitFont();

BITFONT *fnt = new BITFONT();
return (IREFONT *)fnt;
}

	
IREFONT *MakeBitfont(const char *filename)
{
if(!draw_bitfont_func)
	InitBitFont();

BITFONT *fnt = new BITFONT(filename);
return (IREFONT *)fnt;
}

	
	
//
//	Constructors
//


BITFONT::BITFONT() : IREFONT()
{
// Create default 8x8 font

fontdata = DefaultBitfont;
}


BITFONT::BITFONT(const char *filename) : IREFONT(filename)
{
int len=0;
char rfilename[1024];
fontdata = DefaultBitfont;	// Default to 8x8 font
 
if(!filename)
	{
	printf("IREFONT: no filename\n");
	return;
	}
strncpy(rfilename,filename,1023);
rfilename[1023]=0;

fontdata = iload_file(rfilename,&len);
if(len != 2048)
	{
	M_free(fontdata);
	fontdata=NULL;
	}

if(!fontdata)
	{
	printf("BITFONT: DAT_FONT not found for %s\n",filename);
	fontdata=DefaultBitfont;
	}
}


BITFONT::~BITFONT()
{
if(fontdata && fontdata != DefaultBitfont)
	M_free(fontdata);
fontdata=NULL;
}


void BITFONT::Draw(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *string)
{
int fgcol=-1;
int bgcol=-1;
int w,h,woff;
unsigned char c;
unsigned char *buffer;

if(!dest || !string || !fontdata)
	return;

if(x<0 || y < 0)
	return;

if(fg)
	fgcol = fg->packed;	// Do not do this if the font has its own palette
if(bg)
	bgcol = bg->packed;

//printf("DRaw bitfont '%s'\n",string);

w=dest->GetW();
h=dest->GetH();
buffer = dest->GetFramebuffer();
if(!buffer)
	return;

if(y+8 > h)
	return;

// Convert Y and X to byte offset inside buffer
y *= w;
y+=x;
y <<= shiftfactor; // convert to appropriate BPP
buffer += y;
woff=w-8;

for(;*string;string++)
	{
	if(x+8 >= w)
		break;
	c = (unsigned char)*string;
	draw_bitfont_func(buffer,fontdata + (c << 3),woff,fgcol);
	x+=8;
	buffer += (8 << shiftfactor);
	}
}


void BITFONT::Printf(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *fmt, ...)
{
char linebuf[1024];
va_list ap;

va_start(ap,fmt);

#ifdef _WIN32
_vsnprintf(linebuf,1023,fmt,ap);
#else
vsnprintf(linebuf,1023,fmt,ap);
#endif

Draw(dest,x,y,fg,bg,linebuf);
va_end(ap);
}


int BITFONT::Length(const char *str)
{
if(!fontdata || !str)
	return 0;

return strlen(str)*8;
}


int BITFONT::Height()
{
if(!fontdata)
	return 0;

return 8;
}


int BITFONT::IsColoured()
{
return 0;
}

///////////
///////////
///////////

void InitBitFont()
{
switch(_IRE_BPP) {
	case 15:
	case 16:
		draw_bitfont_func=draw_bitfont_16c;
#ifndef NO_ASM
		if(!ire_NOASM)
			draw_bitfont_func=draw_bitfont_16asm;
#endif
		shiftfactor=1;
		break;
	default:
		draw_bitfont_func=draw_bitfont_32c;
#ifndef NO_ASM
		if(!ire_NOASM)
			draw_bitfont_func=draw_bitfont_32asm;
#endif
		shiftfactor=2;
		break;
	}
}

//
// C rendering backends
//

void draw_bitfont_16c(unsigned char *buffer, unsigned char *fontdata, int w, unsigned int colour)
{
uint16_t *buffer16 = (uint16_t *)buffer;
int h,b;
unsigned char c;

for(h=0;h<8;h++)
	{
	c=*fontdata;
	for(b=0;b<8;b++)
		{
		if(c & 0x80)
			*buffer16=colour;
		buffer16++;
		c<<=1;
		}

	buffer16 +=w;
	fontdata++;
	}
}



void draw_bitfont_32c(unsigned char *buffer, unsigned char *fontdata, int w, unsigned int colour)
{
uint32_t *buffer32 = (uint32_t *)buffer;
int h,b;
unsigned char c;

for(h=0;h<8;h++)
	{
	c=*fontdata;
	for(b=0;b<8;b++)
		{
		if(c & 0x80)
			*buffer32=colour;
		buffer32++;
		c<<=1;
		}

	buffer32 +=w;
	fontdata++;
	}
}
