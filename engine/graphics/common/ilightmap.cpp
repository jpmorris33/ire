//
//	Generic lightmap implementation
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../iregraph.hpp"

extern void darken_darksprite(int x,int y, int scrw, int scrh, unsigned char *out, unsigned char *in);
extern void darken_lightsprite(int x,int y, int scrw, int scrh, unsigned char *out, unsigned char *in);
extern void (*darken)(IREBITMAP *image, IRELIGHTMAP *tab);


class CMNLIGHTMAP : public IRELIGHTMAP
	{
	public:
		CMNLIGHTMAP(int w, int h);
		~CMNLIGHTMAP();
		void Clear(unsigned char level);
		void DrawLight(IRELIGHTMAP *dest, int x, int y);
		void DrawDark(IRELIGHTMAP *dest, int x, int y);
		void DrawSolid(IRELIGHTMAP *dest, int x, int y);
		void Get(IRELIGHTMAP *src, int x, int y);
		unsigned char GetPixel(int x, int y);
		void PutPixel(int x, int y, unsigned char level);
		void Render(IREBITMAP *dest);
		void RenderRaw(IREBITMAP *dest);

		unsigned char *GetFramebuffer();
		int GetW();
		int GetH();
		
	private:
		unsigned char *image;
		int width,height;
	};

	
//
//  Bootstrap
//

IRELIGHTMAP *MakeIRELIGHTMAP(int w, int h)
{
CMNLIGHTMAP *lm = new CMNLIGHTMAP(w,h);
return (IRELIGHTMAP *)lm;
}

IRELIGHTMAP *MakeIRELIGHTMAP(int w, int h, const unsigned char *data)
{
unsigned char *dat;
CMNLIGHTMAP *lm = new CMNLIGHTMAP(w,h);
dat=NULL;
if(lm)
	dat=lm->GetFramebuffer();
if(dat)
	memcpy(dat,data,w*h);
return (IRELIGHTMAP *)lm;
}

	
	
//
//	Constructor
//
	
CMNLIGHTMAP::CMNLIGHTMAP(int w, int h) : IRELIGHTMAP(w,h)
{
image=NULL;
width=height=0;
if(w<1 || h<1)
	{
	printf("Width error: w=%d, h=%d\n",w,h);
	return;
	}
/*	
if(w&7 || h&7)
	{
	printf("Div-8 error: w=%d, h=%d\n",w,h);
	return;	// Must be divisible by 8
	}
*/
image = (unsigned char *)calloc(w,h);
if(!image)
	{
	printf("Error: calloc failed on w=%d, h=%d\n",w,h);
	return;
	}
width=w;
height=h;
}


//
//  Destructor
//

CMNLIGHTMAP::~CMNLIGHTMAP()
{
if(image)
	free(image);
image=NULL;
width=height=0;
}


//
//  Clear to a level
//

void CMNLIGHTMAP::Clear(unsigned char level)
{
if(image)
	memset(image,level,width*height);
}

//
//  Add light
//

void CMNLIGHTMAP::DrawLight(IRELIGHTMAP *dest, int x, int y)
{
if(!image)
	return;
if(width == 32 && height == 32)
	{
	darken_lightsprite(x,y,dest->GetW(),dest->GetH(), dest->GetFramebuffer(), image);
	return;
	}
	
// Fallback?
}

//
//  Darken
//

void CMNLIGHTMAP::DrawDark(IRELIGHTMAP *dest, int x, int y)
{
if(!image)
	return;
if(width == 32 && height == 32)
	{
	darken_darksprite(x,y,dest->GetW(),dest->GetH(), dest->GetFramebuffer(), image);
	return;
	}
	
// Fallback?
}

//
//  Draw direct
//

void CMNLIGHTMAP::DrawSolid(IRELIGHTMAP *dest, int x, int y)
{
unsigned char *srcptr,*destptr;
int dw,dh,yctr;
if(!dest || !image)
	return;
srcptr=image;

destptr = dest->GetFramebuffer();
if(!destptr)
	return;
dw=dest->GetW();
dh=dest->GetH();

if(x<0 || x+width > dw || y<0 || y+height>dh)
	return;

destptr += ((dw*y)+x);
for(yctr=0;yctr<height;yctr++)
	  {
	  memcpy(destptr,srcptr,width);
	  destptr+=dw;
	  srcptr+=width;
	  }
}

//
//  Get direct
//

void CMNLIGHTMAP::Get(IRELIGHTMAP *src, int x, int y)
{
unsigned char *srcptr,*destptr;
int sw,sh,yctr;
if(!src || !image)
	return;
destptr=image;

srcptr = src->GetFramebuffer();
if(!srcptr)
	return;
sw=src->GetW();
sh=src->GetH();

if(x<0 || x+width > sw || y<0 || y+height>sh)
	return;

srcptr += ((sw*y)+x);
for(yctr=0;yctr<height;yctr++)
	  {
	  memcpy(destptr,srcptr,width);
	  destptr+=width;
	  srcptr+=sw;
	  }
}

//
//  Get a pixel value
//

unsigned char CMNLIGHTMAP::GetPixel(int x, int y)
{
if(x<0 || y<0 || x >= width || y>=height)
	return 0;
if(!image)
	return 0;
return image[(y*width)+x];
}

//
//  Set a pixel value
//

void CMNLIGHTMAP::PutPixel(int x, int y, unsigned char level)
{
if(x<0 || y<0 || x >= width || y>=height)
	return;
if(!image)
	return;
image[(y*width)+x] = level;
}

//
//  Render as darkness
//

void CMNLIGHTMAP::Render(IREBITMAP *dest)
{
darken(dest,this);
}

//
//  Render as greyscale for editor previews
//

void CMNLIGHTMAP::RenderRaw(IREBITMAP *dest)
{
int x,y;
unsigned char *p=image;
if(!image || !width || !height || !dest)
	return;

for(y=0;y<height;y++)
	for(x=0;x<width;x++)
		{
		dest->PutPixel(x,y,*p,*p,*p);
		p++;
		}
}


unsigned char *CMNLIGHTMAP::GetFramebuffer()
{
return image;
}


int CMNLIGHTMAP::GetW()
{
return width;
}

int CMNLIGHTMAP::GetH()
{
return height;
}
