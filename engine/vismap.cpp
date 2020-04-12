#include <string.h>
#include "graphics/iregraph.hpp"
#include "core.hpp"
#include "gamedata.hpp"
#include "oscli.hpp"
#include "map.hpp"
#include "object.hpp"
#include "vismap.hpp"

static IRECOLOUR blanker(0,0,0);

VisMap::VisMap() {

unsigned int rr,gg,bb;

memset(&vismap[0][0],0,(sizeof(char)*(LMSIZE*LMSIZE)));
memset(&blmap[0][0],0,(sizeof(char)*(LMSIZE*LMSIZE)));

// Calculate blanker colour
if(blankercol > 0) {
	rr=(blankercol>>16)&0xff;   // get Red
	gg=(blankercol>>8)&0xff;    // get green
	bb=blankercol&0xff;         // get blue
	blanker.Set(rr,gg,bb);
} else {
	// If no colour has been specified, assume pure black (0,0,0)
	blanker.Set(0,0,0);
}

}


//
//  Visibility calculation (player can look through windows)
//

void VisMap::calculate()
{
int ctr,ctr2;
int xmax,ymax,visx,visy;
OBJECT *o;

xmax = VSW;
ymax = VSH;

if(player) {
	visx=player->x;
	visy=player->y;
} else {
	// If there's no player, make an educated guess
	visx=mapx+VSMIDX;
	visy=mapy+VSMIDY;
}

// First, clear the visibility map (all blank)
memset(&vismap[0][0],0,(sizeof(char)*(LMSIZE*LMSIZE)));

// Mark edges as lit
for(ctr=VSH+2;ctr>0;ctr--) {
	vismap[0][ctr]=VISMAP_IS_LIT;
}
for(ctr=VSW+2;ctr>0;ctr--) {
	vismap[ctr][0]=VISMAP_IS_LIT;
}

// Now find the tiles that block light

for(ctr=0;ctr<=ymax;ctr++)
	for(ctr2=0;ctr2<=xmax;ctr2++)
		blmap[ctr2][ctr] = BlocksLight(mapx+ctr2,mapy+ctr);

// Check for windows
// North
o=GetSolidObject(visx,visy-1);
if(o)
	if(o->flags & IS_WINDOW)
		punch(visx-mapx,(visy-mapy)-1);

// West
o=GetSolidObject(visx-1,visy);
if(o)
	if(o->flags & IS_WINDOW)
		punch((visx-mapx)-1,visy-mapy);

// East
o=GetSolidObject(visx+1,visy);
if(o)
	if(o->flags & IS_WINDOW)
		punch((visx-mapx)+1,visy-mapy);

// South
o=GetSolidObject(visx,visy+1);
if(o)
	if(o->flags & IS_WINDOW)
		punch(visx-mapx,(visy-mapy)+1);

// General floodfill
floodfill(visx-mapx,visy-mapy);
}

//
//  Simplified calculation for lighting
//

void VisMap::calculateSimple()
{
int ctr,ctr2;
int xmax,ymax;
char edgemode=VISMAP_IS_DONE|VISMAP_IS_NOSPRITE;

xmax = VSW;
ymax = VSH;

if(!player) { // If there isn't a player we want to keep the old visibility map
	return;
}

// First, clear the visibility map (all blank)
memset(&vismap[0][0],0,(sizeof(char)*(LMSIZE*LMSIZE)));

// Mark edges as desired
for(ctr=VSH+2;ctr>0;ctr--) {
	vismap[0][ctr]=edgemode;
	vismap[VSH+2][ctr]=edgemode;
}
for(ctr=VSW+2;ctr>0;ctr--) {
	vismap[ctr][0]=edgemode;
	vismap[ctr][VSW+2]=edgemode;
}

// Now find the tiles that block light

for(ctr=0;ctr<=ymax;ctr++)
	for(ctr2=0;ctr2<=xmax;ctr2++)
		blmap[ctr2][ctr] = BlocksLight(mapx+ctr2,mapy+ctr);

// General floodfill
floodfill(player->x-mapx,player->y-mapy);
}


/*
 *	Remove rooms you can't see from the view (like U6)
 *	Project - Do the actual blanking
 */

void VisMap::project(IREBITMAP *dest)
{
int xctr,yctr,ctr,ctr2,xmax,ymax;

// Block out what we don't want to see

xmax = VSW;
ymax = VSH;

yctr=0;
xctr=0;
for(ctr=0;ctr<ymax;ctr++) {
	for(ctr2=0;ctr2<xmax;ctr2++) {
		if(!(vismap[ctr2+1][ctr+1]&VISMAP_IS_LIT)) {
			dest->FillRect(xctr,yctr,31,31,&blanker);
		}
		xctr+=32;
	}
	xctr=0;
	yctr+=32;
}
}




//
//  Is that square blank?
//

int VisMap::isBlanked(int x, int y)
{
int vx,vy;

vx = x-mapx;
vy = y-mapy;

if(vx<0 || vx>VSW) {
//	printf("Out of range (%d,%d)\n",vx,vy);
	return 1;
}
if(vy<0 || vy>VSH)
	return 1;

if(!vismap[vx+1][vy+1]&VISMAP_IS_LIT)
	return 1;

return 0;
}

//
//  Is that square blank, or disallows sprites?  (Other side of wall)
//

int VisMap::isSpriteBlanked(int x, int y)
{
int vx,vy;

vx = x-mapx;
vy = y-mapy;

if(vx<0 || vx>VSW) {
	return 1;
}
if(vy<0 || vy>VSH)
	return 1;

if(vismap[vx+1][vy+1]&VISMAP_IS_NOSPRITE)
	return 1;
if(!vismap[vx+1][vy+1]&VISMAP_IS_LIT)
	return 1;

return 0;
}

//
//  Is that square blank, or disallows sprites?  (Other side of wall)
//

int VisMap::getNoSpriteDirect(int vx, int vy)
{
if(vx<0 || vx>VSW) {
	return 1;
}
if(vy<0 || vy>VSH) {
	return 1;
}

if(vismap[vx+1][vy+1]&VISMAP_IS_NOSPRITE) {
	return 1;
}
return 0;
}


//
//  Get blmap state (for diagnostics)
//

char VisMap::getBlMap(int x, int y)
{
if(x<0 || x>VSW)
	return 0;
if(y<0 || y>VSH)
	return 0;

return blmap[x][y];
}

//
//  Get state (for diagnostics)
//

char VisMap::get(int x, int y)
{
if(x<0 || x>VSW)
	return 0;
if(y<0 || y>VSH)
	return 0;

return vismap[x][y];
}


//
//  Get alternate tile shape for when a room is blanked off (x,y NOT PROTECTED!)
//

int VisMap::getAltCode(int x, int y) {
int altcode;
/*
The shape of the alternate tile is in an array of sixteen image pointers.
There are sixteen possible combinations of adjacent tiles on or off (or 'rules').
To get the number of the combination, we add four bits together, taken from the state of each adjacent tile:
L=1, R=2, U=4, D=8.

To check the state, we look at the vismap.  If a tile is blanked, the vismap entry will have bit 1 clear.

Note that the vismap is 1-based, not 0-based (outer rim!)
So, instead of checking   -1,0 +1,0  0,-1  0,+1 we have to check instead  0,1 +2,+1 +1,0 +1,+2

The optimised method below just adds the bits.  The unoptimised version is easier to understand.

*/

// Optimised version
altcode=(vismap[x][y+1]&VISMAP_IS_LIT);
altcode|=(vismap[x+2][y+1]&VISMAP_IS_LIT)<<1;
altcode|=(vismap[x+1][y]&VISMAP_IS_LIT)<<2;
altcode|=(vismap[x+1][y+2]&VISMAP_IS_LIT)<<3;
altcode^=0x0f; // invert it all

//// Blank the sprites behind the wall
//if(altcode&0x0a) { // If Down OR Right
//	vismap[x+1][y+1] |= VISMAP_IS_NOSPRITE; // mark bad (no sprites)
//}

return altcode;
}

/*
// Easier-to-read version

// Add up each direction, to get the rule number
// Check L(-1,0), R(+1,0), U(0,-1), D(0,+1)
// Vismap has +1,+1 added to it for the outer rim
altcode=0;
if(!(vismap[ctr2][ctr+1]&1)) altcode|=1; // L
if(!(vismap[ctr2+2][ctr+1]&1)) {
	altcode|=2; // R
	vismap[ctr2+1][ctr+1] |= 4; // mark bad (no sprites)
}
if(!(vismap[ctr2+1][ctr]&1)) altcode|=4; // U
if(!(vismap[ctr2+1][ctr+2]&1)) {
	altcode|=8; // D
	vismap[ctr2+1][ctr+1] |= 4; // mark bad (no sprites)
}
*/

//
//  Similar to getAltCode (and previously part of it), this checks whether a given tile
//  has blanks to the right, or below it.  If so, disable sprites for that tile.
//  X,Y ARE NOT PROTECTED!
//

void VisMap::checkSprites(int x,int y) {
if(!(vismap[x+2][y+1]&VISMAP_IS_LIT))	{
	vismap[x+1][y+1] |= VISMAP_IS_NOSPRITE; // mark bad (no sprites)
	return;
}
if(!(vismap[x+1][y+2]&VISMAP_IS_LIT))	{
	vismap[x+1][y+1] |= VISMAP_IS_NOSPRITE; // mark bad (no sprites)
	return;
}
}


void VisMap::clearBlMap() {
memset(&vismap[0][0],0,(sizeof(char)*(LMSIZE*LMSIZE)));
}


/*
 *  Blank rooms the player can't see
 */

void VisMap::floodfill(int x, int y)
{
// Check bounds

if(x<0 || x>VSW)
	return;
if(y<0 || y>VSH)
	return;

// Already done?
if(vismap[x+1][y+1])
	{
	vismap[x+1][y+1]=VISMAP_IS_DONE|VISMAP_IS_LIT;
	return;
	}

vismap[x+1][y+1]=VISMAP_IS_DONE|VISMAP_IS_LIT;

// If it blocks light, stop right there
if(blmap[x][y]) {
	return;
}

floodfill(x-1,y-1);
floodfill(x,y-1);
floodfill(x+1,y-1);

floodfill(x-1,y);
floodfill(x+1,y);

floodfill(x-1,y+1);
floodfill(x,y+1);
floodfill(x+1,y+1);
}

/*
 *  Punch a hole in the darkness
 */

void VisMap::punch(int x, int y)
{
if(x<0 || x>VSW)
	return;
if(y<0 || y>VSH)
	return;

vismap[x+1][y+1]=VISMAP_IS_LIT;

floodfill(x-1,y-1);
floodfill(x,y-1);
floodfill(x+1,y-1);

floodfill(x-1,y);
floodfill(x+1,y);

floodfill(x-1,y+1);
floodfill(x,y+1);
floodfill(x+1,y+1);
}


