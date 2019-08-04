//
//	SDL 2.0 bitmap implementation
//

#include <stdio.h>
#include "ibitmap.hpp"
#include "../common/primitve.hpp"
#include "../common/blenders.hpp"

extern int _IRE_BPP;
extern int _IRE_MouseActivated;

static void fatline(int x, int y, IRECOLOUR *col, IREBITMAP *screen);
static int fatline_radius=1;
static Uint32 GetRGBMask(int bpp, int RGorB);
static SDL_Window *sdl_framebuffer;
static SDL_Renderer *sdl_renderer;
SDL_PixelFormat *IRE_pixelFormat=NULL;
static SDL20BMP *pixelformatimage=NULL;

#define ALPHA_OPAQUE 255


//
//  Bootstrap
//

int MakeIREScreen(int bpp, int fullscreen)
{
// Force BPP to 32
_IRE_BPP=32;

int flags=0;
if(fullscreen)
	flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
else
	flags |= SDL_WINDOW_RESIZABLE;

SDL_CreateWindowAndRenderer(640, 480, flags, &sdl_framebuffer, &sdl_renderer);
//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"linear");
SDL_RenderSetLogicalSize(sdl_renderer, 640,480);
if(!sdl_framebuffer || !sdl_framebuffer)
	return 0;
  
return 1;
}

// This must be called when SDL_Quit is called, preferably beforehand

void InvalidateIREScreen()
{
sdl_framebuffer=NULL;
}


IREBITMAP *MakeIREBITMAP(int w, int h)
{
SDL20BMP *spr = new SDL20BMP(w,h);
return (IREBITMAP *)spr;
}

IREBITMAP *MakeIREBITMAP(int w, int h, int bpp)
{
SDL20BMP *spr = new SDL20BMP(w,h,bpp);
return (IREBITMAP *)spr;
}

void IRE_SetPixelFormat(int bpp)
{
pixelformatimage = new SDL20BMP(32,32,bpp);
IRE_pixelFormat = pixelformatimage->img->format;
}


//
//	Constructor
//
	
SDL20BMP::SDL20BMP(int w, int h) : IREBITMAP(w,h)
{
BlenderAlpha=255;
BlenderMode=0;
locked=0;
width=w;
height=h;
bytesperpixel=_IRE_BPP/8;
img = SDL_CreateRGBSurface(0,w,h,_IRE_BPP,0,0,0,0);
texture = SDL_CreateTexture(sdl_renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,w,h);
if(img && texture)	{
	SDL_FillRect(img,NULL,0);
} else	{
	printf("Create failed for %dx%d\n",w,h);
	abort();
}

}

//
//	Multi-bpp constructor
//

SDL20BMP::SDL20BMP(int w, int h, int bpp) : IREBITMAP(w,h,bpp)
{
Uint32 rmask,gmask,bmask;

BlenderAlpha=255;
BlenderMode=0;
locked=0;
width=w;
height=h;
bytesperpixel=bpp/8;

//select_palette(default_palette);

rmask=GetRGBMask(bpp,0);
gmask=GetRGBMask(bpp,1);
bmask=GetRGBMask(bpp,2);

img = SDL_CreateRGBSurface(0,w,h,bpp,rmask,gmask,bmask,0);
texture = SDL_CreateTexture(sdl_renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,w,h);
if(img && texture)	{
	SDL_FillRect(img,NULL,0);
}else	{
	printf("Create failed for %dx%d, %d\n",w,h,bpp);
	abort();
	}
}

//
//  Destructor
//

SDL20BMP::~SDL20BMP()
{
if(img)
	SDL_FreeSurface(img);
img=NULL;
if(texture)
	SDL_DestroyTexture(texture);
texture=NULL;
}

//
//  Clear to a given colour
//

void SDL20BMP::Clear(IRECOLOUR *col)
{
if(!img || !col)
	return;
SDL_FillRect(img,NULL,col->packed);
}

//
//  Regular transparent drawing
//

void SDL20BMP::Draw(IREBITMAP *dest, int x, int y)
{
if(!img || !dest)
	return;
SDL_Rect rec;
rec.x=x;
rec.y=y;
rec.w=width;
rec.h=height;
SDL_SetColorKey(img,SDL_TRUE,ire_transparent->packed);
SDL_SetSurfaceBlendMode(img,SDL_BLENDMODE_NONE);
SDL20BMP *destptr=(SDL20BMP *)dest;
SDL_BlitSurface(img,NULL,destptr->img,&rec);
}

//
//  Alpha-blended drawing
//

void SDL20BMP::DrawAlpha(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest)
	return;
SDL_Rect rec;
rec.x=x;
rec.y=y;
rec.w=width;
rec.h=height;
SDL_SetColorKey(img,SDL_TRUE,ire_transparent->packed);
SDL_SetSurfaceAlphaMod(img,level);
SDL20BMP *destptr=(SDL20BMP *)dest;
SDL_SetSurfaceBlendMode(img,SDL_BLENDMODE_BLEND);
SDL_BlitSurface(img,NULL,destptr->img,&rec);
}

//
//  Shadow drawing - need to implement
//

void SDL20BMP::DrawShadow(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest)
	return;
SDL_Rect rec;
rec.x=x;
rec.y=y;
rec.w=width;
rec.h=height;
SDL_SetColorKey(img,SDL_TRUE,ire_transparent->packed);
SDL_SetSurfaceAlphaMod(img,level);
SDL20BMP *destptr=(SDL20BMP *)dest;
SDL_SetSurfaceBlendMode(img,SDL_BLENDMODE_BLEND);
SDL_BlitSurface(img,NULL,destptr->img,&rec);
//SDL_SetSurfaceAlphaMod(img,ALPHA_OPAQUE);
}

//
//  Non-transparent drawing
//

void 
SDL20BMP::DrawSolid(IREBITMAP *dest, int x, int y)
{
if(!img || !dest)
	return;
SDL_Rect rec;
rec.x=x;
rec.y=y;
rec.w=width;
rec.h=height;
SDL_SetColorKey(img,0,0);

SDL20BMP *destptr=(SDL20BMP *)dest;
SDL_BlitSurface(img,NULL,destptr->img,&rec);
}

//
//  Stretch-blit - need to implement myself
//

void SDL20BMP::DrawStretch(IREBITMAP *dest, int x, int y, int w, int h)
{
if(!img || !dest)
	return;
SDL_Rect rec;
rec.x=x;
rec.y=y;
rec.w=width;
rec.h=height;
SDL_SetColorKey(img,SDL_TRUE,ire_transparent->packed);

SDL20BMP *destptr=(SDL20BMP *)dest;
SDL_BlitSurface(img,NULL,destptr->img,&rec);
}


//
//  Fillrect
//

void SDL20BMP::FillRect(int x, int y, int w, int h, IRECOLOUR *col)
{
if(!img || !col || h<=0 || w<=0)
	return;

 w++; // Hack, but it seems to be too small if we don't
 h++;

if(BlenderAlpha == 255)	{
	SDL_Rect rec;
	rec.x=x;
	rec.y=y;
	rec.w=w;
	rec.h=h;
	SDL_FillRect(img,&rec,col->packed);
} else	{
	int ya=y;
	for(int hc=h;hc>0;hc--)	{
		HLine(x,ya++,w,col);
	}
		
}
}

void SDL20BMP::FillRect(int x, int y, int w, int h, int r, int g, int b)
{
if(!img || h<=0 || w<=0)
	return;

w++; // Hack, but it seems to be too small if we don't
 h++;

if(BlenderAlpha == 255)	{
	SDL_Rect rec;
	rec.x=x;
	rec.y=y;
	rec.w=w;
	rec.h=h;
	SDL_FillRect(img,&rec,SDL_MapRGB(IRE_pixelFormat,r,g,b));
} else	{
	int ya=y;
	IRECOLOUR col;
	col.Set(r,g,b);
	for(int hc=h;hc>0;hc--)	{
		HLine(x,ya++,w,&col);
	}
		
}
}

//
//  Circle
//

void SDL20BMP::Circle(int x, int y, int radius, IRECOLOUR *col)
{
if(!img || !col)
	return;
ITGcirclefill(x,y,radius,col,this);
}

void SDL20BMP::Circle(int x, int y, int radius, int r, int g, int b)
{
if(!img)
	return;
IRECOLOUR col;
col.Set(r,g,b);
ITGcirclefill(x,y,radius,&col,this);
}

//
//  Line
//

void SDL20BMP::Line(int x, int y, int x2, int y2, IRECOLOUR *col)
{
if(!img || !col)
	return;
ITGline(x,y,x2,y2,col,this);
}

void SDL20BMP::Line(int x, int y, int x2, int y2, int r, int g, int b)
{
if(!img)
	return;
IRECOLOUR col;
col.Set(r,g,b);
ITGline(x,y,x2,y2,&col,this);
}

//
//  Thick line
//

void SDL20BMP::ThickLine(int x, int y, int x2, int y2, int radius, IRECOLOUR *col)
{
if(!img || !col)
	return;
fatline_radius=radius;
ITGlinecallback(x,y,x2,y2,fatline,col,this);
}

void SDL20BMP::ThickLine(int x, int y, int x2, int y2, int radius, int r, int g, int b)
{
IRECOLOUR col;
col.Set(r,g,b);
ThickLine(x,y,x2,y2,radius,&col);
}


//
//  Polygon
//

void SDL20BMP::Polygon(int count, int *vertices, IRECOLOUR *col)
{
if(!img || !vertices || count < 2 || !col)
	return;

int oldx=vertices[0];
int oldy=vertices[1];
for(int ctr=2;ctr<count;ctr+=2)	{
	ITGline(oldx,oldy,vertices[ctr],vertices[ctr+1],col,this);
	oldx=vertices[ctr];
	oldy=vertices[ctr+1];
	}
}

void SDL20BMP::Polygon(int count, int *vertices, int r, int g, int b)
{
IRECOLOUR col;
col.Set(r,g,b);
Polygon(count,vertices,&col);
}


void SDL20BMP::SetBrushmode(int mode, int alpha)
{
BlenderMode = mode;
BlenderAlpha = alpha;
SetBlender();
BlenderSetAlpha(alpha);
}

void SDL20BMP::SetBlender()
{
if(!img || !img->format)
	return;
BlenderSetMode(BlenderMode,img->format->BitsPerPixel);
}


//
//  Acquire from an existing bitmap
//

void SDL20BMP::Get(IREBITMAP *src, int x, int y)
{
if(!img || !src)
	return;
SDL_Rect rec;
rec.x=x;
rec.y=y;
rec.w=width;
rec.h=height;
SDL_SetColorKey(img,SDL_FALSE,0);

SDL20BMP *srcptr=(SDL20BMP *)src;
SDL_BlitSurface(srcptr->img,&rec,img,NULL);
}

//
//  Get a pixel
//

int SDL20BMP::GetPixel(int x, int y)
{
Uint32 out=0;
if(!img)
	return -1;
if(x<0||y<0||x>=width||y>=height)
	return -1;
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

int SDL20BMP::GetPixel(int x, int y, IRECOLOUR *col)
{
if(!col)
	return 0;

int c = GetPixel(x,y);
if(c==-1)
	return 0;
col->Set(c);
return 1;
}

//
//  Set a pixel
//

void SDL20BMP::PutPixel(int x, int y, IRECOLOUR *col)
{
if(!img)
	return;
if(x<0||y<0||x>=width||y>=height)
	return;
int bpp=img->format->BytesPerPixel;
unsigned char *ptr=(unsigned char *)img->pixels;
unsigned char *src=(unsigned char *)&col->packed;

ptr += (img->pitch *y);
ptr += (x*bpp);

if(BlenderOp)	{
	BlenderOp(ptr,src);
	return;
	}

for(int ctr=bpp;ctr>0;ctr--)
	*ptr++=*src++;
}

void SDL20BMP::PutPixel(int x, int y, int r, int g, int b)
{
Uint32 packed;
if(!img)
	return;
if(x<0||y<0||x>=width||y>=height)
	return;
int bpp=img->format->BytesPerPixel;
unsigned char *ptr=(unsigned char *)img->pixels;
unsigned char *src=(unsigned char *)&packed;

ptr += (img->pitch *y);
ptr += (x*bpp);
packed=SDL_MapRGB(IRE_pixelFormat,r,g,b);

if(BlenderOp)	{
	BlenderOp(ptr,src);
	return;
	}

for(int ctr=bpp;ctr>0;ctr--)
	*ptr++=*src++;
}

//
//	For optimising rectangles etc
//

void SDL20BMP::HLine(int x, int y, int w, IRECOLOUR *col)
{
if(!img)
	return;
if(x<0||y<0||x>=width||y>=height)
	return;
if(x+w >= width)
	w=width-x;

int ctr;
int bpp=img->format->BytesPerPixel;
unsigned char *ptr=(unsigned char *)img->pixels;
unsigned char *src=(unsigned char *)&col->packed;

ptr += (img->pitch *y);
ptr += (x*bpp);

if(BlenderOp)	{
	for(ctr=w;ctr>0;ctr--)	{
		BlenderOp(ptr,src);
		ptr+=bpp;
		}
	return;
	}

unsigned char *oldsrc=src;
for(ctr=w;ctr>0;ctr--)	{
	src=oldsrc;
	for(int ctr2=bpp;ctr2>0;ctr2--)
		*ptr++=*src++;
	}
}

void SDL20BMP::HLine(int x, int y, int w, int r, int g, int b)	{
IRECOLOUR col;
col.Set(r,g,b);
HLine(x,y,w,r,g,b);
}


//
//  Render to physical screen at 0,0 - this is intended for the swapscreen bitmap
//

void SDL20BMP::Render()
{
if(!img || !sdl_framebuffer)
	return;
/*
if(_IRE_MouseActivated)
	{
	SDL_ShowCursor(0);
	}
*/	

SDL_UpdateTexture(texture,NULL,img->pixels,img->pitch);
SDL_RenderCopy(sdl_renderer,texture,NULL,NULL);
SDL_RenderPresent(sdl_renderer);

/*
if(_IRE_MouseActivated)
	{
	SDL_ShowCursor(1);
	}
*/	
}

//
//
//

void SDL20BMP::ShowMouse()
{
SDL_ShowCursor(1);
}

void SDL20BMP::HideMouse()
{
SDL_ShowCursor(0);
}

int SDL20BMP::SaveBMP(const char *filename)
{
return(SDL_SaveBMP(img,filename) == 0);
}


//
//  Get framebuffer
//

unsigned char *SDL20BMP::GetFramebuffer()
{
if(!img)
	return NULL;
if(!locked && SDL_MUSTLOCK(img))	{
	if(SDL_LockSurface(img) == 0)
		locked=1;
	}

return (unsigned char *)img->pixels;
}


void SDL20BMP::ReleaseFramebuffer(unsigned char *ptr)
{
if(locked)
	SDL_UnlockSurface(img);
locked=0;
}



int SDL20BMP::GetW()
{
return width;
}


int SDL20BMP::GetH()
{
return height;
}


int SDL20BMP::GetDepth()
{
if(!img)
	return 0;
if(!IRE_pixelFormat)
	return 0;
return IRE_pixelFormat->BitsPerPixel;
}


//
//	Helper function
//

void fatline(int x, int y, IRECOLOUR *col, IREBITMAP *screen)
{
ITGcircle(x,y,fatline_radius,col,screen);
}


Uint32 GetRGBMask(int bpp, int RGorB)
{
Uint32 mask[4][3]	=
	{
	{0x00007c00,0x000003e0,0x0000001f},	// RGB 15
	{0x0000f800,0x000007e0,0x0000001f},	// RGB 16
	{0x00ff0000,0x0000ff00,0x000000ff},	// RGB 24
	{0x00ff0000,0x0000ff00,0x000000ff}	// RGB 32
	};
	
if(RGorB <0 || RGorB > 2)
	return 0;
switch(bpp)	{
	case 15:
		return mask[0][RGorB];
	case 16:
		return mask[1][RGorB];
	case 24:
		return mask[2][RGorB];
	case 32:
		return mask[3][RGorB];
	}
	
return 0;
}

