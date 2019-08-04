//
//      Map manipulation routines
//

#include "core.hpp"
#include "gamedata.hpp"
#include "map.hpp"
#include "object.hpp"
#include "ithelib.h"

/*
 *      isSolid - return TRUE if the map square of interest is impassible
 */

int isSolid(int x,int y)
{
OBJECT *temp;

    if(IsTileSolid(x,y))			// Is the tile solid?
	return 1;

    if(x>=mapx && y>=mapy)                      // Check for a big object
       if(x<mapx+VSW)                           // If onscreen
          if(y<mapy+VSH)
		{
		if(solidmap[VIEWDIST+x-mapx][VIEWDIST+y-mapy])
			return 1;
		return 0;
		}

    temp = curmap->objmap[ytab[y]+x];            // Now look at object map
    for(;temp;temp=temp->next)
        if(temp->flags & IS_SOLID)
            if(temp->flags & IS_ON)
                return 1;
return 0;
}

/*
 *      GetSolidMap - return TRUE if the map square of interest is impassible
 */

OBJECT *GetSolidMap(int x,int y)
{
if(x>=mapx && y>=mapy)                      // Check for a big object
	if(x<mapx+VSW)                           // If onscreen
		if(y<mapy+VSH)
			return solidmap[VIEWDIST+x-mapx][VIEWDIST+y-mapy];
return NULL;
}

/*
 *      blocksLight - return TRUE if the map square of interest blocks light
 */

int BlocksLight(int x,int y)
{
TILE *tile;
OBJECT *object;

if(x>=curmap->w || y>=curmap->h || x<0 || y<0)
	return 1;                                   // Out of the map area

tile = GetTile(x,y);                        // Look at tiles first
if(tile->flags & DOES_BLOCKLIGHT)
	return 1;

object = GetObjectBase(x,y);

for(;object;object=object->next)
	{
	if(object->flags & DOES_BLOCKLIGHT)
		if(object->flags & IS_ON)
				return 1;
	}

return 0;
}

/*
 *      AlwaysSolid - return TRUE if the map square of interest is impassible
 *                    but FALSE if it is only impassible sometimes (doors etc)
 */

int AlwaysSolid(int x,int y)
{
TILE *tile;
OBJECT *temp;
    tile = GetTile(x,y);                        // Look at tiles first
    if(tile->flags & IS_SOLID)
       return 1;

    if(x>=mapx && y>=mapy)                      // Check for a big object
       if(x<mapx+VSW)                           // If onscreen
          if(y<mapy+VSH)
              {
              temp=largemap[VIEWDIST+x-mapx][VIEWDIST+y-mapy];
              if(temp)
                  if(temp->flags & IS_SOLID)
		      {
                      if(temp->flags & CAN_OPEN /*|| temp->flags & IS_PERSON*/)
                          return 0;
                      else
                          {
                          temp=solidmap[VIEWDIST+x-mapx][VIEWDIST+y-mapy];
                          if(temp)
                                  {
                                  if((temp->flags & CAN_OPEN)
//                                  || temp->flags.person
                                  || (temp->flags & IS_ON) == 0)
                                     return 0;
                                  else
                                     return 1;
                                  }
                              else
                                  return 0;
                          }
		      }
              }


    temp = curmap->objmap[ytab[y]+x];            // Now look at object map
    for(;temp;temp=temp->next)
        if(temp->flags & IS_SOLID)
            if(temp->flags & IS_ON)
                {
                if(temp->flags & CAN_OPEN)
                    return 0;
//                if(temp->flags & IS_PERSON)
//                    return 0;
//                if(solidmap
                return 1;
                }
return 0;
}

/*
 *      isDecor - return TRUE if the map square of interest has a Decor object
 */

int isDecor(int x,int y)
{
OBJECT *temp;

temp = curmap->objmap[ytab[y]+x];            // Now look at object map
if(temp)
	{
	for(;temp->next;temp=temp->next); // seek to end
	if(temp->flags & IS_DECOR)
		return 1;
	}
return 0;
}


/*
 *      CentreMap - Centre the map's position around an object
 */

void CentreMap(OBJECT *focus)
{
if(!focus)
   return;

if(focus->flags & IS_LARGE)
    {
    focus->mw = focus->w>>5;
    if(focus->w & 0xf || !focus->mw)
        focus->mw++;

    focus->mh = focus->h>>5;
    if(focus->h & 0xf || !focus->mh)
        focus->mh++;

    mapx = (focus->x-VSMIDX)+(focus->mw>>1);
    mapy = (focus->y-VSMIDY)+(focus->mh>>1);
    }
else
    {
    mapx = focus->x-VSMIDX;
    mapy = focus->y-VSMIDY;
    }

if(mapx<0)
    mapx=0;
if(mapy<0)
    mapy=0;

mapx2 = mapx+VSW-1;
mapy2 = mapy+VSH-1;
}


/*
 *      GetTile - return a pointer to the kind of tile in this position
 */

TILE *GetTile(int x,int y)
{
unsigned int i;
i = curmap->physmap[MAP_POS(x,y)];
if(i == RANDOM_TILE)    // Oh God!
	return &TIlist[0];	//Resort to trickery
return &TIlist[i];
}

/*
 *      IsTileSolid - return whether the tile in this square is solid
 */

int IsTileSolid(int x,int y)
{
TILE *t;
OBJECT *temp;
if(x>=curmap->w)
	return 1;
if(y>=curmap->h)
	return 1;
// Examine the tile
//t = &TIlist[curmap->physmap[MAP_POS(x,y)]];
t = GetTile(x,y);
// If it's water, look for a bridge or boat over it
if(t->flags & IS_WATER)
	{
	for(temp=GetObject(x,y);temp;temp=temp->next)
		if(temp->flags & IS_WATER)
			return 0;
	}

// Otherwise..

if(t->flags & IS_SOLID)
	return 1;
return 0;
}


/*
 *      GetTileCost - return movement cost for the tile at x,y
 */

int GetTileCost(int x,int y)
{
TILE *t;
OBJECT *temp;

if(x>=curmap->w)
	return -1; // Wall
if(y>=curmap->h)
	return -1; // Wall

// Examine the tile.  If it's water, look for a bridge over it
t = GetTile(x,y);
if(t->flags & IS_WATER)
	{
	for(temp=GetObject(x,y);temp;temp=temp->next)
		if(temp->flags & IS_WATER)
			return t->cost;
	}

// Is it solid?
if(t->flags & IS_SOLID)
	return -1;

// Otherwise..
return t->cost;
}


/*
 *      IsTileWater - return whether the tile in this square is water
 */

int IsTileWater(int x,int y)
{
if(TIlist[curmap->physmap[MAP_POS(x,y)]].flags & IS_WATER)
	return 1;
return 0;
}

/*
 *		Check if the given X,Y coordinates intersect a given large object
 */

int LargeIntersect(OBJECT *temp, int x, int y)
{
int blockx,blocky,blockw,blockh;

if(!temp)
	return 0;

if(!(temp->flags & IS_SOLID))
	return 0;

if(temp->curdir > CHAR_D) //(U,D,L,R) > D = L,R
	{
	blockx=temp->hblock[BLK_X];
	blocky=temp->hblock[BLK_Y];
	blockw=temp->hblock[BLK_W]-1;
	blockh=temp->hblock[BLK_H]-1;
	}
else
	{
	blockx=temp->vblock[BLK_X];
	blocky=temp->vblock[BLK_Y];
	blockw=temp->vblock[BLK_W]-1;
	blockh=temp->vblock[BLK_H]-1;
	}
// Convert solid region to map coordinates
blockx+=temp->x;
blocky+=temp->y;
blockw+=blockx;
blockh+=blocky;

// Only return the large object if it matches the solid area
if(x>=blockx && x<=blockw)
	if(y>=blocky && y<=blockh)
		{
//		ilog_printf("%s %d,%d intersects %d-%d,%d-%d\n",temp->name,x,y,blockx,blockw,blocky,blockh);
		return 1;
		}

return 0;
}
