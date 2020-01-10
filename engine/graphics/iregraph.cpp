#include <stdio.h>
#include <stdlib.h>
#include "iregraph.hpp"
#include "../ithelib.h"

extern void (*darken_bitmap)(IREBITMAP *image, unsigned int level);
extern void (*merge_blit)(unsigned char *dest,unsigned char *src, int pixels);
extern void darken_init(int bpp);
extern void IRE_SetPixelFormat(int bpp);

int _IRE_MouseActivated=0;
int _IRE_BPP=0;

//
//  General-purpose bitmaps
//

IREBITMAP::IREBITMAP(int w, int h)
{
}

IREBITMAP::IREBITMAP(int w, int h, int bpp)
{
}

IREBITMAP::~IREBITMAP()
{
}

//  Darken the bitmap by a given level

void IREBITMAP::Darken(unsigned int level)
{
if(level>255)
	level=255;
darken_bitmap(this,level);
}

// Merge anumber bitmap, with zero-based transparency into the current bitmap

void IREBITMAP::Merge(IREBITMAP *src)
{
unsigned char *fb=GetFramebuffer();
int sw,sh;
if(!src || !fb)
	return;
sw=src->GetW();
if(GetW() != sw)
	return;
sh=src->GetH();
if(GetH() < sh)
	sh=GetH();
merge_blit(fb,src->GetFramebuffer(),sw*sh);
}


//  Stub functions

void IREBITMAP::Clear(IRECOLOUR *col) {}
void IREBITMAP::Draw(IREBITMAP *dest, int x, int y) {}
void IREBITMAP::DrawAlpha(IREBITMAP *dest, int x, int y, int level) {}
void IREBITMAP::DrawShadow(IREBITMAP *dest, int x, int y, int level) {}
void IREBITMAP::DrawSolid(IREBITMAP *dest, int x, int y) {}
void IREBITMAP::DrawStretch(IREBITMAP *dest, int x, int y, int w, int h) {}
void IREBITMAP::DrawStretch(IREBITMAP *dest, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) {}
void IREBITMAP::FillRect(int x, int y, int w, int h, IRECOLOUR *col) {}
void IREBITMAP::FillRect(int x, int y, int w, int h, int r, int g, int b) {}
void IREBITMAP::Line(int x, int y, int x2, int y2, IRECOLOUR *col) {}
void IREBITMAP::Line(int x, int y, int x2, int y2, int r, int g, int b) {}
void IREBITMAP::ThickLine(int x, int y, int x2, int y2, int width, IRECOLOUR *col) {}
void IREBITMAP::ThickLine(int x, int y, int x2, int y2, int width, int r, int g, int b) {}
void IREBITMAP::Circle(int x, int y, int radius, IRECOLOUR *col) {}
void IREBITMAP::Circle(int x, int y, int radius, int r, int g, int b) {}
void IREBITMAP::Polygon(int points, int *pointlist, IRECOLOUR *col) {}
void IREBITMAP::Polygon(int points, int *pointlist, int r, int g, int b) {}
void IREBITMAP::SetBrushmode(int mode, int alphalevel) {}
void IREBITMAP::Get(IREBITMAP *src, int x, int y) {}
int IREBITMAP::GetPixel(int x, int y) {return 0;}
int IREBITMAP::GetPixel(int x, int y, IRECOLOUR *col) {return 0;}
void IREBITMAP::PutPixel(int x, int y, IRECOLOUR *col) {}
void IREBITMAP::PutPixel(int x, int y, int r, int g, int b) {}
void IREBITMAP::HLine(int x, int y, int w, IRECOLOUR *col) {};
void IREBITMAP::HLine(int x, int y, int w, int r,int g,int b) {};
void IREBITMAP::Render() {}
void IREBITMAP::ShowMouse() {}
void IREBITMAP::HideMouse() {}
unsigned char *IREBITMAP::GetFramebuffer() {return NULL;}
void IREBITMAP::ReleaseFramebuffer(unsigned char *ptr) {}
int IREBITMAP::SaveBMP(const char *filename) {return 0;}
int IREBITMAP::GetW() {return 0;}
int IREBITMAP::GetH() {return 0;}
int IREBITMAP::GetDepth() {return 0;}


//
//  fast transparent sprites (RLE in Allegro)
//

IRESPRITE::IRESPRITE(IREBITMAP *src)
{
}

IRESPRITE::IRESPRITE(const void *data, int len)
{
}

IRESPRITE::~IRESPRITE()
{
}

void IRESPRITE::Draw(IREBITMAP *dest, int x, int y) {}
void IRESPRITE::DrawSolid(IREBITMAP *dest, int x, int y) {}
void IRESPRITE::DrawAddition(IREBITMAP *dest, int x, int y, int level) {}
void IRESPRITE::DrawAlpha(IREBITMAP *dest, int x, int y, int level) {}
void IRESPRITE::DrawDissolve(IREBITMAP *dest, int x, int y, int level) {}
void IRESPRITE::DrawShadow(IREBITMAP *dest, int x, int y, int level) {}
void IRESPRITE::DrawInverted(IREBITMAP *dest, int x, int y) {}
void *IRESPRITE::SaveBuffer(int *len) {return NULL;}
void IRESPRITE::FreeSaveBuffer(void *data) {}
unsigned int IRESPRITE::GetAverageCol() {return 0;}
void IRESPRITE::SetAverageCol(unsigned int col) {}
int IRESPRITE::GetW() {return 0;}
int IRESPRITE::GetH() {return 0;}

//
//  Mouse cursor
//

IRECURSOR::IRECURSOR()
{
}

IRECURSOR::IRECURSOR(IREBITMAP *src, int hotx, int hoty)
{
}

IRECURSOR::~IRECURSOR()
{
}

void IRECURSOR::Set() {}


//
//  Lightmaps
//

IRELIGHTMAP::IRELIGHTMAP(int x, int y)
{
}

IRELIGHTMAP::~IRELIGHTMAP()
{
}

void IRELIGHTMAP::Clear(unsigned char level) {}
void IRELIGHTMAP::DrawLight(IRELIGHTMAP *dest, int x, int y) {}
void IRELIGHTMAP::DrawDark(IRELIGHTMAP *dest, int x, int y) {}
void IRELIGHTMAP::DrawSolid(IRELIGHTMAP *dest, int x, int y) {}
void IRELIGHTMAP::DrawCorona(int x, int y, int radius, int intensity, int falloff, bool darken) {}
void IRELIGHTMAP::Get(IRELIGHTMAP *src, int x, int y) {}
unsigned char IRELIGHTMAP::GetPixel(int x, int y) {return 0;}
void IRELIGHTMAP::PutPixel(int x, int y, unsigned char level) {}
void IRELIGHTMAP::Render(IREBITMAP *dest) {}
void IRELIGHTMAP::RenderRaw(IREBITMAP *dest) {}
unsigned char *IRELIGHTMAP::GetFramebuffer() {return NULL;}
int IRELIGHTMAP::GetW() {return 0;}
int IRELIGHTMAP::GetH() {return 0;}

//
//  Fonts
//

IREFONT::IREFONT()
{
}

IREFONT::IREFONT(const char *fname)
{
}

IREFONT::~IREFONT()
{
}

//void IREFONT::Draw(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *string);
void IREFONT::Printf(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *fmt, ...) {}
int IREFONT::Length(const char *str) {return 0;}
int IREFONT::Height() {return 0;}
int IREFONT::IsColoured() {return 0;}


//
//  Other functions
//

void IRE_StartGFX(int bpp)
{
_IRE_BPP=bpp;
IRE_SetPixelFormat(bpp);
darken_init(_IRE_BPP);
}


void IRE_SetMouse(int status)
{
_IRE_MouseActivated=status;
}


//
//  May need this for implementing translucent lines and stuff
//

/*
void ITGline( int xa, int ya, int xb, int yb, int colour,void * bitmap)
{
	int t, distance;
	int xerr=0, yerr=0, delta_x, delta_y;
	int incx, incy;

	// Find the distance to go in each plane.
	delta_x = xb - xa;
	delta_y = yb - ya;

	// Find out the direction
	if(delta_x > 0) incx = 1;
	else if(delta_x == 0) incx = 0;
	else incx = -1;

	if(delta_y > 0) incy = 1;
	else if(delta_y == 0) incy = 0;
	else incy = -1;

	// Find which way is were mostly going
	delta_x = abs(delta_x);
	delta_y = abs(delta_y);
	if(delta_x > delta_y) distance=delta_x;
	else distance=delta_y;

	// Draw the god dam line.
	for(t = 0; t <= distance + 1; t++) {
		dot(xa, ya, colour,bitmap);
		xerr += delta_x;
		yerr += delta_y;
		if(xerr > distance) {
			xerr -= distance;
			xa += incx;
		}
		if(yerr > distance) {
			yerr -= distance;
			ya += incy;
		}
	}
}
*/
