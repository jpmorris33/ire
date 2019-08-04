//
//	Allegro 4.x font implementation - DEPRECATED, we now use IRE's own bitfonts and bytefonts
//


#include <stdio.h>
#include <string.h>
#include "ibitmap.hpp"

class A4FONT : IREFONT
	{
	public:
		A4FONT();
		A4FONT(const char *filename);
		~A4FONT();
		void Draw(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *string);
		void Printf(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *fmt, ...);
		int Length(const char *str);
		int Height();
		int IsColoured();
	private:
		DATAFILE *df;
		FONT *fontdata;
		RGB *fontpal;
	};


//
//  Bootstrap
//

IREFONT *MakeOldfont()
{
A4FONT *fnt = new A4FONT();
return (IREFONT *)fnt;
}

	
IREFONT *MakeOldfont(const char *filename)
{
A4FONT *fnt = new A4FONT(filename);
return (IREFONT *)fnt;
}

	
	
//
//	Constructors
//


A4FONT::A4FONT() : IREFONT()
{
// Create default 8x8 font

df = NULL;
fontdata = font;
fontpal = NULL;
}


A4FONT::A4FONT(const char *filename) : IREFONT(filename)
{
int ctr;

df = NULL;
fontdata = font;	// Default to 8x8 font
fontpal = NULL;
 
if(!filename)
	{
	printf("IREFONT: no filename\n");
	return;
	}

df = load_datafile(filename);
if(df)
	{
	for(ctr=0;df[ctr].type != DAT_END;ctr++)
		{
		// Look for the font
		if(df[ctr].type == DAT_FONT)
			fontdata=(FONT *)df[ctr].dat;
		// Look for the palette, if the font has one associated with it
		if(df[ctr].type == DAT_PALETTE)
			fontpal=(RGB *)df[ctr].dat;
		}
	}
else
	{
	printf("IREFONT: could not load  %s\n",filename);
	return;
	}
	
if(!fontdata)
	{
	printf("IREFONT: DAT_FONT not found for %s\n",filename);
	fontpal=NULL;
	fontdata=font;
	}
}


A4FONT::~A4FONT()
{
if(df)
	unload_datafile(df);
}


void A4FONT::Draw(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *string)
{
int fgcol=-1;
int bgcol=-1;

if(!dest || !string || !fontdata)
	return;

if(fg && !fontpal)
	fgcol = fg->packed;	// Do not do this if the font has its own palette
if(bg)
	bgcol = bg->packed;

if(fontpal)
	set_palette(fontpal);
textout_ex(((A4BMP *)dest)->img,fontdata,string,x,y,fgcol,bgcol);
}


void A4FONT::Printf(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *fmt, ...)
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


int A4FONT::Length(const char *str)
{
if(!fontdata || !str)
	return 0;

return text_length(fontdata,str);
}


int A4FONT::Height()
{
if(!fontdata)
	return 0;

return text_height(fontdata);
}


int A4FONT::IsColoured()
{
if(!fontpal)
	return 0;

return 1;
}
