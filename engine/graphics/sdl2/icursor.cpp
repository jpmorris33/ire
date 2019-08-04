//
//	SDL2 mouse pointer
//

#include <stdio.h>
#include "ibitmap.hpp"

class SDL20CURSOR : public IRECURSOR
	{
	public:
		SDL20CURSOR();
		SDL20CURSOR(IREBITMAP *src, int hotx, int hoty);
		~SDL20CURSOR();
		void Set();
		
	private:
		int defaultcursor;
		SDL_Cursor *img;
	};


IRECURSOR *MakeIRECURSOR()
{
SDL20CURSOR *spr = new SDL20CURSOR();
return (IRECURSOR *)spr;
}
	
IRECURSOR *MakeIRECURSOR(IREBITMAP *bmp, int hotx, int hoty)
{
SDL20CURSOR *spr = new SDL20CURSOR(bmp,hotx,hoty);
return (IRECURSOR *)spr;
}


//
//	Constructor, default cursor
//
	
SDL20CURSOR::SDL20CURSOR() : IRECURSOR()
{
img=SDL_GetCursor();
defaultcursor=1;  // Don't free it
}


//
//	Constructor
//
	
SDL20CURSOR::SDL20CURSOR(IREBITMAP *src, int hotx, int hoty) : IRECURSOR(src,hotx,hoty)
{
// This is a total cheat, create the default SDL hand cursor instead of
// actually loading in the bitmap.  If the game needs to support multiple
// cursors eventually, e.g. direction arrows for the game window, this will
// need to be done properly.

img = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
defaultcursor=0;
}

//
//	Cleanup
//

SDL20CURSOR::~SDL20CURSOR()
{
if(img && !defaultcursor)
	SDL_FreeCursor(img);
}

//
//	Use this cursor
//

void SDL20CURSOR::Set()
{
if(img)
	SDL_SetCursor(img);
}
