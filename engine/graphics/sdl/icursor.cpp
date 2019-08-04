//
//	Allegro 4.x mouse pointer
//

#include <stdio.h>
#include "ibitmap.hpp"

static SDL_Cursor *SurfaceToCursor(SDL_Surface *image, int hx, int hy);
static Uint32 getpixel(SDL_Surface *img, int x, int y);

class SDL12CURSOR : public IRECURSOR
	{
	public:
		SDL12CURSOR();
		SDL12CURSOR(IREBITMAP *src, int hotx, int hoty);
		~SDL12CURSOR();
		void Set();
		
	private:
		int defaultcursor;
		SDL_Cursor *img;
	};


IRECURSOR *MakeIRECURSOR()
{
SDL12CURSOR *spr = new SDL12CURSOR();
return (IRECURSOR *)spr;
}
	
IRECURSOR *MakeIRECURSOR(IREBITMAP *bmp, int hotx, int hoty)
{
SDL12CURSOR *spr = new SDL12CURSOR(bmp,hotx,hoty);
return (IRECURSOR *)spr;
}


//
//	Constructor, default cursor
//
	
SDL12CURSOR::SDL12CURSOR() : IRECURSOR()
{
img=SDL_GetCursor();
defaultcursor=1;  // Don't free it
}


//
//	Constructor
//
	
SDL12CURSOR::SDL12CURSOR(IREBITMAP *src, int hotx, int hoty) : IRECURSOR(src,hotx,hoty)
{
if(!src)
	return;

SDL12BMP *srcptr=(SDL12BMP *)src;
img = SurfaceToCursor(srcptr->img,hotx,hoty);
defaultcursor=0;
}

//
//	Cleanup
//

SDL12CURSOR::~SDL12CURSOR()
{
if(img && !defaultcursor)
	SDL_FreeCursor(img);
}

//
//	Use this cursor
//

void SDL12CURSOR::Set()
{
if(img)
	SDL_SetCursor(img);
}



//
//	Surface->Cursor converter from the SDL mailing list, written by Andre de Leiradella, 2003
//
//	http://lists.libsdl.org/pipermail/sdl-libsdl.org/2003-April/034907.html
//

SDL_Cursor *SurfaceToCursor(SDL_Surface *image, int hx, int hy) {
	int w, x, y;
	Uint8 *data, *mask, *d, *m, r, g, b;
	Uint32 color;
	SDL_Cursor *cursor;

	w = (image->w + 7) / 8;
//	data = (Uint8 *)alloca(w * image->h * 2);
	data = (Uint8 *)malloc(w * image->h * 2);	// JM: Use malloc instead, more cross-platform
	if (data == NULL)
		return NULL;
	memset(data, 0, w * image->h * 2);
	mask = data + w * image->h;
	if (SDL_MUSTLOCK(image))
		SDL_LockSurface(image);
	for (y = 0; y < image->h; y++) {
		d = data + y * w;
		m = mask + y * w;
		for (x = 0; x < image->w; x++) {
			color = getpixel(image, x, y);
			if ((image->flags & SDL_SRCCOLORKEY) == 0 || color != image->format->colorkey) {
				SDL_GetRGB(color, image->format, &r, &g, &b);
				color = (r + g + b) / 3;
				m[x / 8] |= 128 >> (x & 7);
				if (color < 128)
					d[x / 8] |= 128 >> (x & 7);
			}
		}
	}
	if (SDL_MUSTLOCK(image))
		SDL_UnlockSurface(image);
	cursor = SDL_CreateCursor(data, mask, w, image->h, hx, hy);
	// JM: Now free the allocated data since we're not using alloca anymore
	free(data);
	return cursor;
}

//
//	JM: Support routine for SurfaceToCursor
//

Uint32 getpixel(SDL_Surface *img, int x, int y)
{
Uint32 out=0;
if(!img)
	return 0;
if(x<0||y<0||x>=img->w||y>=img->h)
	return 0;
int bpp=img->format->BytesPerPixel;
unsigned char *ptr=(unsigned char *)img->pixels;
unsigned char *dest=(unsigned char *)&out;

ptr += (img->pitch *y);
ptr += (x*bpp);

if(bpp == 4)
	return *(Uint32 *)ptr;

if(bpp == 2)
	return *(Uint16 *)ptr;

for(int ctr=bpp;ctr>0;ctr--)
	*dest++=*ptr++;

return out;
}
