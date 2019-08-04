//
//	Allegro 4.x sprite implementation
//

#include <stdio.h>
#include "ibitmap.hpp"
#include "../common/blenders.hpp"

#define HEADER_SDL12 0x314c4453 // SDL1

class SDL12SPR : public IRESPRITE
	{
	public:
		SDL12SPR(IREBITMAP *src);
		SDL12SPR(const void *data, int len);
		~SDL12SPR();
		void Draw(IREBITMAP *dest, int x, int y);
		void DrawAddition(IREBITMAP *dest, int x, int y, int level);
		void DrawAlpha(IREBITMAP *dest, int x, int y, int level);
		void DrawDissolve(IREBITMAP *dest, int x, int y, int level);
		void DrawShadow(IREBITMAP *dest, int x, int y, int level);
		void DrawSolid(IREBITMAP *dest, int x, int y);
		void DrawInverted(IREBITMAP *dest, int x, int y);
		void *SaveBuffer(int *len);
		void FreeSaveBuffer(void *buffer);
                unsigned int GetAverageCol();
                void SetAverageCol(unsigned int acw);
		int GetW();
		int GetH();
		
	private:
		SDL_Surface *img;
		SDL_Rect rec;
		unsigned int avcol;
		Uint32 chromakey;
		unsigned char *originalpixels;
		BLENDERFUNC AddProc;
		BLENDERFUNC AlphaProc;
		BLENDERFUNC DissolveProc;
		BLENDERFUNC InvertProc;
	};

	
//
//  Bootstrap
//

IRESPRITE *MakeIRESPRITE(IREBITMAP *bmp)
{
SDL12SPR *spr = new SDL12SPR(bmp);
return (IRESPRITE *)spr;
}

IRESPRITE *MakeIRESPRITE(const void *data, int len)
{
unsigned int header=0;
if(!data || len<4)
	return NULL;
memcpy(&header,data,4);
if(header != HEADER_SDL12)
	return NULL;

SDL12SPR *spr = new SDL12SPR(data,len);
return (IRESPRITE *)spr;
}


// Read a 32-bit word

static  unsigned int getwptr(unsigned char **ptr)	{
	unsigned char *bptr=*ptr;
	unsigned char byte;
	unsigned int out=0;
	
	byte=(*bptr++)&0xff;
	out = byte;
	byte=(*bptr++)&0xff;
	out |= byte<<8;
	byte=(*bptr++)&0xff;
	out |= byte<<16;
	byte=(*bptr++)&0xff;
	out |= byte<<24;
	*ptr=bptr;
	
	return out;
}

// Write a 32-bit word

static  void putwptr(unsigned char **ptr, unsigned int word)	{
	unsigned char *bptr=*ptr;
	*bptr++ = (word&0xff);
	*bptr++ = ((word>>8)&0xff);
	*bptr++ = ((word>>16)&0xff);
	*bptr++ = ((word>>24)&0xff);

	*ptr=bptr;
}


	
//
//	Constructor
//
	
SDL12SPR::SDL12SPR(IREBITMAP *src) : IRESPRITE(src)
{
if(!src)
	return;

avcol=0;
originalpixels=NULL;

SDL12BMP *srcptr=(SDL12BMP *)src;
// First duplicate the image
img = SDL_ConvertSurface(srcptr->img,srcptr->img->format,srcptr->img->flags);

// Now convert to RLE format
chromakey=SDL_MapRGB(img->format,ire_transparent->r,ire_transparent->g,ire_transparent->b);
SDL_SetColorKey(img,SDL_SRCCOLORKEY|SDL_RLEACCEL,chromakey);

// Take a copy of the bitmap data for the complex sprite effects
SDL_LockSurface(img);
int len = img->w*img->h*srcptr->img->format->BytesPerPixel;
originalpixels = (unsigned char *)calloc(1,len);
if(!originalpixels)
	return;
memcpy(originalpixels,img->pixels,len);
SDL_UnlockSurface(img);
// Okay, sorted

AddProc=BlenderGetOp(ALPHA_ADD,srcptr->img->format->BitsPerPixel);
AlphaProc=BlenderGetOp(ALPHA_TRANS,srcptr->img->format->BitsPerPixel);
DissolveProc=BlenderGetOp(ALPHA_DISSOLVE,srcptr->img->format->BitsPerPixel);
InvertProc=BlenderGetOp(ALPHA_INVERT,srcptr->img->format->BitsPerPixel);

rec.w=img->w;
rec.h=img->h;
}


// Constructor to build from cache data

SDL12SPR::SDL12SPR(const void *datafile, int len) : IRESPRITE(datafile,len)
{
unsigned char *dptr=(unsigned char *)datafile; // Dodgy, but we aren't actually altering it
unsigned int sz=0,w,h,bpp,pitch,rmask,gmask,bmask,amask;
Uint32 chrom=0;
if(!datafile)
	return;

img=NULL;
// header, image size, w, h, bpp, pitch, rmask, gmask, bmask, amask, chromakey, avcol,pixels

sz = getwptr(&dptr);
if(sz != HEADER_SDL12)
	return;

sz = getwptr(&dptr);	// pixels
w = getwptr(&dptr);
h = getwptr(&dptr);
bpp = getwptr(&dptr);
pitch = getwptr(&dptr);
rmask = getwptr(&dptr);
gmask = getwptr(&dptr);
bmask = getwptr(&dptr);
amask = getwptr(&dptr);
chrom = getwptr(&dptr);
avcol = getwptr(&dptr);

// We need to keep a copy of the original pixel data
originalpixels = (unsigned char *)calloc(1,sz);
if(!originalpixels)
	return;
memcpy(originalpixels,dptr,sz);

img=SDL_CreateRGBSurfaceFrom(originalpixels,w,h,bpp,pitch,rmask,gmask,bmask,amask);
SDL_SetColorKey(img,SDL_SRCCOLORKEY|SDL_RLEACCEL,chrom);

AddProc=BlenderGetOp(ALPHA_ADD,bpp);
AlphaProc=BlenderGetOp(ALPHA_TRANS,bpp);
DissolveProc=BlenderGetOp(ALPHA_DISSOLVE,bpp);
InvertProc=BlenderGetOp(ALPHA_INVERT,bpp);

rec.w=w;
rec.h=h;
}


//
//  Destructor
//

SDL12SPR::~SDL12SPR()
{
if(img)
	SDL_FreeSurface(img);
img=NULL;

if(originalpixels)
	free(originalpixels);
originalpixels=NULL;
}

//
//  Regular transparent drawing
//

void SDL12SPR::Draw(IREBITMAP *dest, int x, int y)
{
if(!img || !dest)
	return;

rec.x=x;
rec.y=y;
SDL12BMP *destptr=(SDL12BMP *)dest;
SDL_BlitSurface(img,NULL,destptr->img,&rec);
}

//
//  Additive blended drawing
//

void SDL12SPR::DrawAddition(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest || !originalpixels)
	return;
if(level == 0)
	return;
if(level == 255)	{
	Draw(dest,x,y);
	return;
}

rec.x=x;
rec.y=y;
SDL12BMP *destptr=(SDL12BMP *)dest;
SDL_BlitSurface(img,NULL,destptr->img,&rec);
}

//
//  Alpha-blended drawing
//

void SDL12SPR::DrawAlpha(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest || !originalpixels)
	return;
if(level == 0)
	return;
if(level == 255)	{
	Draw(dest,x,y);
	return;
}

rec.x=x;
rec.y=y;
SDL12BMP *destptr=(SDL12BMP *)dest;
SDL_SetAlpha(img,SDL_SRCALPHA,level);
SDL_BlitSurface(img,NULL,destptr->img,&rec);
SDL_SetAlpha(img,SDL_SRCALPHA,SDL_ALPHA_OPAQUE);
}

//
//  Dissolved drawing
//

void SDL12SPR::DrawDissolve(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest || !originalpixels)
	return;
if(level == 0)
	return;
if(level == 255)	{
	Draw(dest,x,y);
	return;
}

rec.x=x;
rec.y=y;
SDL12BMP *destptr=(SDL12BMP *)dest;
SDL_BlitSurface(img,NULL,destptr->img,&rec);
}

//
//  Shadow drawing - replace with a burn or subtractive blender if possible
//

void SDL12SPR::DrawShadow(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest || !originalpixels)
	return;
if(level == 0)
	return;
if(level == 255)	{
	Draw(dest,x,y);
	return;
}

rec.x=x;
rec.y=y;
SDL12BMP *destptr=(SDL12BMP *)dest;
SDL_SetAlpha(img,SDL_SRCALPHA,level);
SDL_BlitSurface(img,NULL,destptr->img,&rec);
SDL_SetAlpha(img,SDL_SRCALPHA,SDL_ALPHA_OPAQUE);
}

//
//  Non-transparent drawing
//

void SDL12SPR::DrawSolid(IREBITMAP *dest, int x, int y)
{
if(!img || !dest)
	return;
rec.x=x;
rec.y=y;
SDL12BMP *destptr=(SDL12BMP *)dest;
SDL_FillRect(destptr->img,&rec,ire_transparent->packed);
SDL_BlitSurface(img,NULL,destptr->img,&rec);
}

//
//  Inversed drawing
//

void SDL12SPR::DrawInverted(IREBITMAP *dest, int x, int y)
{
if(!img || !dest || !originalpixels)
	return;

rec.x=x;
rec.y=y;
SDL12BMP *destptr=(SDL12BMP *)dest;
SDL_BlitSurface(img,NULL,destptr->img,&rec);
}




unsigned int SDL12SPR::GetAverageCol()
{
return avcol;
}

void SDL12SPR::SetAverageCol(unsigned int acw)
{
avcol=acw;
}

void *SDL12SPR::SaveBuffer(int *len)
{
unsigned char *outbuffer,*ptr;
unsigned int sz=0,pixels;
if(!len|| !img)
	return 0;

pixels = img->w*img->h*img->format->BytesPerPixel;
sz = 4+4+4+4+4+4+4+4+4+4+4+4+(pixels); // header, image size, w, h, bpp, pitch, rmask, gmask, bmask, amask, chromakey, avcol, pixels
*len=sz;
outbuffer = (unsigned char *)calloc(1,sz);
if(!outbuffer)
	return 0;
ptr=outbuffer;

SDL_LockSurface(img);

putwptr(&ptr,HEADER_SDL12);
putwptr(&ptr,pixels);
putwptr(&ptr,img->w);
putwptr(&ptr,img->h);
putwptr(&ptr,img->format->BitsPerPixel);
putwptr(&ptr,img->pitch);
putwptr(&ptr,img->format->Rmask);
putwptr(&ptr,img->format->Gmask);
putwptr(&ptr,img->format->Bmask);
putwptr(&ptr,img->format->Amask);
putwptr(&ptr,chromakey);
putwptr(&ptr,avcol);
memcpy(ptr,img->pixels,pixels);

SDL_UnlockSurface(img);

return outbuffer;
}

void SDL12SPR::FreeSaveBuffer(void *buffer)
{
if(buffer)
	free(buffer);
}


int SDL12SPR::GetW()
{
if(!img)
	return 0;
return img->w;
}

int SDL12SPR::GetH()
{
if(!img)
	return 0;
return img->h;
}
