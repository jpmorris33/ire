//
//  Render monochrome sprite fonts
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

extern int _IRE_BPP;
extern char ire_NOASM;
static int shiftfactor=0;
static void (*draw_bytefont_func)(unsigned char *buffer, unsigned char *fontdata, int w, int h, int offset, unsigned int colour);
static void (*draw_colourfont_func)(unsigned char *buffer, unsigned char *fontdata, unsigned int *pal, int w, int h, int offset);

static void InitByteFont();
static void draw_bytefont_16c(unsigned char *buffer, unsigned char *fontdata, int w, int h, int offset, unsigned int colour);
static void draw_bytefont_32c(unsigned char *buffer, unsigned char *fontdata, int w, int h, int offset, unsigned int colour);
extern "C" void draw_bytefont_16asm(unsigned char *buffer, unsigned char *fontdata, int w, int h, int offset, unsigned int colour);
extern "C" void draw_bytefont_32asm(unsigned char *buffer, unsigned char *fontdata, int w, int h, int offset, unsigned int colour);
static void draw_colourfont_16c(unsigned char *buffer, unsigned char *fontdata, unsigned int *pal, int w, int h, int offset);
static void draw_colourfont_32c(unsigned char *buffer, unsigned char *fontdata, unsigned int *pal, int w, int h, int offset);
extern "C" void draw_colourfont_16asm(unsigned char *buffer, unsigned char *fontdata, unsigned int *pal, int w, int h, int offset);
extern "C" void draw_colourfont_32asm(unsigned char *buffer, unsigned char *fontdata, unsigned int *pal, int w, int h, int offset);

struct FONTPACKET
	{
	int w,h;
	unsigned char *data;
	};

class BYTEFONT : IREFONT
	{
	public:
		BYTEFONT();
		BYTEFONT(const char *filename);
		~BYTEFONT();
		void Draw(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *string);
		void Printf(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *fmt, ...);
		int Length(const char *str);
		int Height();
		int IsColoured();
	private:
		FONTPACKET fontchar[256];
		unsigned int pal[256];
		int fw,fh,colourfont;
	};


//
//  Bootstrap
//

IREFONT *MakeBytefont()
{
if(!draw_bytefont_func)
	InitByteFont();

BYTEFONT *fnt = new BYTEFONT();
return (IREFONT *)fnt;
}

	
IREFONT *MakeBytefont(const char *filename)
{
if(!draw_bytefont_func)
	InitByteFont();

BYTEFONT *fnt = new BYTEFONT(filename);
return (IREFONT *)fnt;
}

	
	
//
//	Constructors
//


BYTEFONT::BYTEFONT() : IREFONT()
{
// Create invalid font
fw=0;
fh=0;
memset(fontchar,0,sizeof(fontchar));
memset(pal,0,sizeof(pal));
colourfont=0;
}


BYTEFONT::BYTEFONT(const char *filename) : IREFONT(filename)
{
int len,ctr;
unsigned char r,g,b;
IRECOLOUR packcol;
IFILE *fp;

// Create invalid font to begin with
fw=0;
fh=0;
memset(fontchar,0,sizeof(fontchar));
memset(pal,0,sizeof(pal));
colourfont=0;

if(!filename)
	{
	printf("IREFONT: no filename\n");
	return;
	}

fp = iopen(filename);

unsigned int header=0;
header=igetl_i(fp);
if(header == IRE_FNT2)
	colourfont=1;
if(header != IRE_FNT1 && header != IRE_FNT2)
	{
	printf("BYTEFONT: Wrong format\n");
	return;
	}

for(ctr=0;ctr<256;ctr++)
	{
	fontchar[ctr].w=igetsh_i(fp);
	fontchar[ctr].h=igetsh_i(fp);
	if(fontchar[ctr].h > fh)
		fh=fontchar[ctr].h;
	if(fontchar[ctr].w > fw)
		fw=fontchar[ctr].w;

	len=fontchar[ctr].w * fontchar[ctr].h;
	if(len > 0)
		{
		fontchar[ctr].data = (unsigned char *)M_get(1,len);
		iread(fontchar[ctr].data,len,fp);
		}
	}

// If we have a palette, read it in and convert it to native BPP format
if(colourfont)
	for(ctr=0;ctr<256;ctr++)
		{
		r=igetc(fp);
		g=igetc(fp);
		b=igetc(fp);
		packcol.Set(r,g,b);
		pal[ctr] = packcol.packed;
		}

iclose(fp);
}


BYTEFONT::~BYTEFONT()
{
for(int ctr=0;ctr<256;ctr++)
	if(fontchar[ctr].data)
		M_free(fontchar[ctr].data);
memset(&fontchar,0,sizeof(fontchar));
fw=0;
fh=0;
}


void BYTEFONT::Draw(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *string)
{
int fgcol=-1;
int bgcol=-1;
int w,h,c;
unsigned char *buffer;
FONTPACKET *ch;

if(!dest || !string)
	return;

if(x<0 || y < 0)
	return;

if(fg)
	fgcol = fg->packed;
if(bg)
	bgcol = bg->packed;

w=dest->GetW();
h=dest->GetH();
buffer = dest->GetFramebuffer();
if(!buffer)
	return;

if(y+fh > h)
	return;

// Convert Y and X to byte offset inside buffer
y *= w;
y+=x;
y <<= shiftfactor; // convert to appropriate BPP
buffer += y;


if(colourfont)
	{
	for(;*string;string++)
		{
		c=(*string) & 0xff;
		ch=&fontchar[c];
		if(x+ch->w >= w)
			break;
		draw_colourfont_func(buffer,ch->data,pal,ch->w,ch->h,w-ch->w);
		x+=ch->w;
		buffer += (ch->w << shiftfactor);  // Shuffle buffer along
		}
	}
else
	{
	for(;*string;string++)
		{
		c=(*string) & 0xff;
		ch=&fontchar[c];
		if(x+ch->w >= w)
			break;
		draw_bytefont_func(buffer,ch->data,ch->w,ch->h,w-ch->w,fgcol);
		x+=ch->w;
		buffer += (ch->w << shiftfactor);
		}
	}
}


void BYTEFONT::Printf(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *fmt, ...)
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


int BYTEFONT::Length(const char *str)
{
int w,c;
if(!str)
	return 0;
w=0;
for(;*str;str++)
	{
	c=(*str)&0xff;
	w+=fontchar[c].w;
	}

return w;
}


int BYTEFONT::Height()
{
return fh;
}


int BYTEFONT::IsColoured()
{
return colourfont;
}

///////////
///////////
///////////

void InitByteFont()
{
switch(_IRE_BPP) {
	case 15:
	case 16:
		draw_bytefont_func=draw_bytefont_16c;
		draw_colourfont_func=draw_colourfont_16c;
#ifndef NO_ASM
		if(!ire_NOASM)
			{
			draw_bytefont_func=draw_bytefont_16asm;
			draw_colourfont_func=draw_colourfont_16asm;
			}
#endif
		shiftfactor=1;
		break;
	default:
		draw_bytefont_func=draw_bytefont_32c;
		draw_colourfont_func=draw_colourfont_32c;
#ifndef NO_ASM
		if(!ire_NOASM)
			{
			draw_bytefont_func=draw_bytefont_32asm;
			draw_colourfont_func=draw_colourfont_32asm;
			}
#endif
		
		shiftfactor=2;
		break;
	}
}

//
// C rendering backends
//

void draw_bytefont_16c(unsigned char *buffer, unsigned char *fontdata, int w, int h, int offset, unsigned int colour)
{
uint16_t *buffer16 = (uint16_t *)buffer;
int yctr,xctr;

for(yctr=0;yctr<h;yctr++)
	{
	for(xctr=0;xctr<w;xctr++)
		{
		if(*fontdata++)
			*buffer16=colour;
		buffer16++;
		}
	buffer16 +=offset;
	}
}


void draw_bytefont_32c(unsigned char *buffer, unsigned char *fontdata, int w, int h, int offset, unsigned int colour)
{
uint32_t *buffer32 = (uint32_t *)buffer;
int yctr,xctr;

for(yctr=0;yctr<h;yctr++)
	{
	for(xctr=0;xctr<w;xctr++)
		{
		if(*fontdata++)
			*buffer32=colour;
		buffer32++;
		}
	buffer32 +=offset;
	}
}


void draw_colourfont_16c(unsigned char *buffer, unsigned char *fontdata, unsigned int *pal, int w, int h, int offset)
{
uint16_t *buffer16 = (uint16_t *)buffer;
int yctr,xctr;
unsigned char c;

for(yctr=0;yctr<h;yctr++)
	{
	for(xctr=0;xctr<w;xctr++)
		{
		c=*fontdata++;
		if(c)
			*buffer16=(uint16_t)pal[c];
		buffer16++;
		}
	buffer16 +=offset;
	}
}


void draw_colourfont_32c(unsigned char *buffer, unsigned char *fontdata, unsigned int *pal, int w, int h, int offset)
{
uint32_t *buffer32 = (uint32_t *)buffer;
int yctr,xctr;
unsigned char c;

for(yctr=0;yctr<h;yctr++)
	{
	for(xctr=0;xctr<w;xctr++)
		{
		c=*fontdata++;
		if(c)
			*buffer32=pal[c];
		buffer32++;
		}
	buffer32 +=offset;
	}
}

