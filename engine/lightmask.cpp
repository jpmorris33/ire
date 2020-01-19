#include <string.h>
#include "ithelib.h"
#include "graphics/iregraph.hpp"
#include "core.hpp"
#include "loadfile.hpp"
#include "gamedata.hpp"
#include "map.hpp"
#include "object.hpp"
#include "lightmask.hpp"
#include "vismap.hpp"

extern IRELIGHTMAP *load_lightmap(char *filename);
extern VisMap lightingmap;

LightMask::LightMask() {
lightmap=NULL;
memset(tile,0,MASK_MAXWIDTH*MASK_MAXHEIGHT*sizeof(IRELIGHTMAP *));
}

LightMask::~LightMask() {
int x,y;
if(lightmap) {
	delete lightmap;
	lightmap=NULL;
}

for(x=0;x<=w;x++) {
	for(y=0;y<=h;y++) {
		if(tile[x][y]) {
			delete tile[x][y];
			tile[x][y]=NULL;
		}
        }
}
}


void LightMask::init() {
int x,y;
char filename[1024];

if(lightmap) {
	return;
}

// Load in the light mask
if(!loadfile("sprites/lightmap.cel",filename))
	ithe_panic("Could not find lightmask '%s'",filename);
lightmap=load_lightmap(filename);

// Find size of the light mask in tiles

w = lightmap->GetW()/32;
h = lightmap->GetH()/32;

// Correct the value (since it rounds down)

if((w * 32) < lightmap->GetW())
    w++;
if((h * 32) < lightmap->GetH())
    h++;

if(w>MASK_MAXWIDTH || h>MASK_MAXHEIGHT) {
	ithe_panic("Lightmap is bigger than 288x288", NULL);
}

// This will be the centre point to fill from
w2=w/2;
h2=h/2;

// Break the lightmap into tiles
for(x=0;x<=w;x++) {
	for(y=0;y<=h;y++) {
		tile[x][y]=MakeIRELIGHTMAP(32,32);
		if(!tile[x][y]) {
			ithe_panic("Could not create lightmap tile","Out of memory?");
		}
		tile[x][y]->Get(lightmap,x<<5,y<<5);
        }
}

}



//
//  Visibility calculation
//

void LightMask::calculate(int x, int y)
{
//Get top-left corner of mask relative to view window
maskx=x-w2;
masky=y-h2;

// Get map coordinates of the top-left corner of the mask
xoffset=(x+mapx)-w2;
yoffset=(y+mapy)-h2;

// First, clear the visibility map (all blank)
memset(&map[0][0],0,(sizeof(char)*(MASK_MAXWIDTH*MASK_MAXHEIGHT)));

floodfill(w2,h2);
}


/*
 *	Project the lightmask on the map
 */

void LightMask::projectLight(int x, int y, IRELIGHTMAP *dest)
{
int ctr,ctr2,xctr,yctr;

// Convert x,y to pixel positions of the top-left corner of the mask
x=(x-w2)<<5;
y=(y-h2)<<5;
// Now it's the top-left corner

xctr=x;
yctr=y;
for(ctr=0;ctr<=h;ctr++) {
	for(ctr2=0;ctr2<=w;ctr2++) {
		if(map[ctr2][ctr]) {
			tile[ctr2][ctr]->DrawLight(dest,xctr,yctr);
		}
		xctr+=32;
	}
	xctr=x;
	yctr+=32;
}
}

/*
 *	Project the lightmask on the map
 */

void LightMask::projectDark(int x, int y, IRELIGHTMAP *dest)
{
int ctr,ctr2,xctr,yctr;

// Convert x,y to pixel positions of the top-left corner of the mask
x=(x-w2)<<5;
y=(y-h2)<<5;
// Now it's the top-left corner

xctr=x;
yctr=y;
for(ctr=0;ctr<=h;ctr++) {
	for(ctr2=0;ctr2<=w;ctr2++) {
		if(map[ctr2][ctr]) {
			tile[ctr2][ctr]->DrawDark(dest,xctr,yctr);
		}
		xctr+=32;
	}
	xctr=x;
	yctr+=32;
}
}


void LightMask::floodfill(int x, int y)
{
// Check bounds

if(x<0 || x>w)
	return;
if(y<0 || y>h)
	return;

// Already done?
if(map[x][y]) {
	return;
}

// If it's not allowed to display wall sprites, no go
if(lightingmap.isSpriteBlanked(x+xoffset,y+yoffset)) {
	return;
}

map[x][y]=1;

// If it blocks light, try filling right and down only
if(BlocksLight(x+xoffset,y+yoffset)) {
	if(!blocked(x+1,y))
		floodfill(x+1,y);
	if(!blocked(x,y+1))
		floodfill(x,y+1);
	return;
}

map[x][y]=1;

// Otherwise just spread

floodfill(x,y-1);

floodfill(x-1,y);
floodfill(x+1,y);

floodfill(x,y+1);
}


int LightMask::blocked(int x, int y) {
if(BlocksLight(x+xoffset,y+yoffset)) {
	return 1;
}
//printf("Check lightmap at %d,%d = %d\n",x+xoffset,y+yoffset,lightingmap.isSpriteBlanked(x+xoffset,y+yoffset));
if(lightingmap.isSpriteBlanked(x+xoffset,y+yoffset)) {
	return 1;
}
return 0;
}
