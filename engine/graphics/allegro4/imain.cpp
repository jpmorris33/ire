//
//	Other allegro-related routines
//
#include <stdio.h>
#include "ibitmap.hpp"

static int lastz=0;

IRECOLOUR::IRECOLOUR()
{
Set(0);
}

IRECOLOUR::IRECOLOUR(int red, int green, int blue)
{
Set(red,green,blue);
}

IRECOLOUR::IRECOLOUR(unsigned int pack)
{
Set(pack);
}

void IRECOLOUR::Set(int red, int green, int blue)
{
r=red&0xff;
g=green&0xff;
b=blue&0xff;
packed=makecol(r,g,b);
}

void IRECOLOUR::Set(unsigned int pack)
{
packed=pack;
r=getr(pack);
g=getg(pack);
b=getb(pack);
}

//
//	Nasty, low-level hack for compatibility while reworking the engine
//	Remove this ASAP when it is no longer needed!
//

void *IRE_GetBitmap(IREBITMAP *bmp)
{
A4BMP *bmpptr=(A4BMP *)bmp;
if(!bmp)
	return NULL;
return bmpptr->img;
}




void IRE_StopGFX()
{
allegro_exit();
}


void IRE_WaitFor(int ms)
{
rest(ms);
}



int IRE_GetMouse()
{
if(poll_mouse() < 0)
	return 0;
//lastz = mouse_z;
return 1;
}


int IRE_GetMouse(int *b)
{
if(poll_mouse() < 0)
	return 0;	// No mouse
if(b)
	*b=mouse_b;
//lastz = mouse_z;
return 1;
}


int IRE_GetMouse(int *x, int *y, int *z, int *b)
{
if(poll_mouse() < 0)
	return 0;	// No mouse
if(x)
	*x=mouse_x;
if(y)
	*y=mouse_y;
if(z)
	{
	*z=lastz-mouse_z;
	lastz = mouse_z;
	}
if(b)
	*b=mouse_b;
return 1;
}



void IRE_ShowMouse()
{
show_mouse(screen);
}

void IRE_HideMouse()
{
show_mouse(NULL);
}
