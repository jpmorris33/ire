//
//	Allegro 4.x mouse pointer
//

#include <stdio.h>
#include "ibitmap.hpp"

class A4CURSOR : public IRECURSOR
	{
	public:
		A4CURSOR();
		A4CURSOR(IREBITMAP *src, int hotx, int hoty);
		~A4CURSOR();
		void Set();
		
	private:
		int active;
		BITMAP *img;
	};

	
IRECURSOR *MakeIRECURSOR()
{
A4CURSOR *spr = new A4CURSOR();
return (IRECURSOR *)spr;
}

IRECURSOR *MakeIRECURSOR(IREBITMAP *bmp, int hotx, int hoty)
{
A4CURSOR *spr = new A4CURSOR(bmp,hotx,hoty);
return (IRECURSOR *)spr;
}


//
//	Constructor, create a default system cursor
//

A4CURSOR::A4CURSOR() : IRECURSOR()
{
img=NULL;
active=0;
}

//
//	Constructor, create a cursor from a bitmap
//

A4CURSOR::A4CURSOR(IREBITMAP *src, int hotx, int hoty) : IRECURSOR(src,hotx,hoty)
{
if(!src)
	return;

A4BMP *srcptr=(A4BMP *)src;
img = create_bitmap(srcptr->img->w,srcptr->img->h);
if(img)
	blit(srcptr->img,img,0,0,0,0,srcptr->img->w,srcptr->img->h);
active=0;
}

//
//	Cleanup
//

A4CURSOR::~A4CURSOR()
{
if(active)
	set_mouse_sprite(NULL);
if(img)
	destroy_bitmap(img);
}

//
//	Use this cursor
//

void A4CURSOR::Set()
{
set_mouse_sprite(img);	// NULL is acceptable
active=1;
}

