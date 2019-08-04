//
//  Automatic path generator (experimental and unused)
//

#if 0

static TL_DATA *WorkOutTile(int dx, int dy, int tile);
static TL_DATA *ChooseNewTile(int dx, int dy, int tile);
static int CheckVector(int dx,int dy, int ox, int oy);
static void Roll(int *dx,int *dy, int odx, int ody);



typedef struct PATHLIST
	{
	int x;
	int y;
	} PATHLIST;


void PathMe()
{
int ctr,bail,ok;
int hx,hy,dx,dy,xa,ya,xb,yb,odx,ody;
TL_DATA *tdata;
int tile,newtile,octr;
PATHLIST *occupied;

tile=1;
newtile=1;

occupied=(PATHLIST *)M_get(MAX_PATH_SQUARES,sizeof(PATHLIST));
octr=0;

odx=0;
ody=0;

// Mark start

if(path1x == 0 && path1y == 0)
	{
	path1x=(x-64)+pmx;
	path1y=(y-64)+pmy;
	PanMap();
	return;
	}

// Prevent multiple clicks

if(path1x == (x-64)+pmx)
	if(path1y == (y-64)+pmy)
		return;

PathMode=0;

if(!Confirm(-1,-1,"Do path?",NULL))
	return;

xa=path1x;
ya=path1y;

for(ctr=0;ctr<100;ctr++)
	{
//	ilog_quiet("Get vector\n");
//	Roll(&dx,&dy,odx,ody);
	bail=0;
	do {

		try_again:
		bail++;
		if(bail>50)
			{
			path1x=0;
			path1y=0;
			PanMap();
			ilog_quiet("Bailed Out\n");
			Notify(-1,-1,"Error: Got stuck making path.. timed out",NULL);
			return;
			}

		// Get vector
		Roll(&dx,&dy,odx,ody);

		// Make sure we didn't backtrack
		for(ctr=0;ctr<octr;ctr++)
			if(occupied[ctr].x == xa+dx && occupied[ctr].y == ya+dy)
				{
				ilog_quiet("Tried to backtrack\n");
				goto try_again;
				}

		ilog_quiet("Going %d,%d, choose new tile (was %s)\n",dx,dy,TLlist[tile].name);

		tdata = ChooseNewTile(dx,dy,tile);
		if(tdata)
			{
			tile=tdata->tile;
			ilog_quiet("Got %d (%s) for tile\n",tile,TLlist[tile].name);
			}
		else
			{
			ilog_quiet("No valid tiles\n");
			goto try_again;
			}

		odx=dx;
		ody=dy;
		} while(!tdata);

	xa += dx;
	ya += dy;

	// Add square to occupied list
	occupied[octr].x=xa;
	occupied[octr].y=ya;
	octr++;
	if(octr>MAX_PATH_SQUARES)
		{
		path1x=0;
		path1y=0;
		PanMap();
		ilog_quiet("Bailed Out\n");
		Notify(-1,-1,"Error: path too long",NULL);
		return;
		}

	WriteMap(xa,ya,TLlist[tile].tile);
//	DrawMap(mapx,mapy,s_proj,l_proj,0);             // Draw the map
	rest(5);
	PanMap();
	Show();
	}

path1x=0;
path1y=0;

PanMap();

}

/*
// Simple direction algorithm

void Roll(int *dx,int *dy, int odx, int ody)
{
int ddx,ddy;

do {
	ddx=(rand()%3)-1;
	ddy=(rand()%2);
	} while(!CheckVector(ddx,ddy,odx,ody));

*dx=ddx;
*dy=ddy;
}
*/

// Biased direction finder
// 'Thirty-two beans' algorithm

void Roll(int *dx,int *dy, int odx, int ody)
{
int beans,ddx,ddy;

ddx=0;
ddy=0;

do {
	beans=rand()%32;

/*
	// W
	if(beans < 2)
		{
		ddy=0;
		ddx=-1;
		}
*/
	// SW
	if(beans > 1 && beans < 7)
		{
		ddy=1;
		ddx=-1;
		}

	// S
	if(beans > 6 && beans < 25)
		{
		ddy=1;
		ddx=0;
		}

	// SE
	if(beans > 24 && beans < 30)
		{
		ddy=1;
		ddx=1;
		}

	// E
	if(beans > 29 && beans < 32)
		{
		ddy=0;
		ddx=1;
		}

	} while(!CheckVector(ddx,ddy,odx,ody));
*dx=ddx;
*dy=ddy;
}

#define GETRANDOM(x)  if(x##t > 0) {t=x[rand()%(x##t+1)];} else {t=x[0];}
// Equivalent to:
/*
	if(TLlist[tile].nt > 0)
		t=TLlist[tile].n[rand()%(TLlist[tile].nt+1]);
	else
		t=TLlist[tile].n[0];
*/


//

// Work out the appropriate tile for this direction

TL_DATA *ChooseNewTile(int dx, int dy, int tile)
{
int t,r,bail;
TL_DATA *tl;

if(tile<0)
	return NULL;

// Check Cardinal directions

// Up
if(dx == 0 && dy < 0)
	{
	if(!TLlist[tile].n)
		return NULL;
	if(!TLlist[tile].n[0].tile)
		return NULL;

	// If only one choice, use it

	if(TLlist[tile].nt == 0)
		return &TLlist[tile].n[0];

	// otherwise, decide

	tl = NULL;
	bail=0;
	do {
		r = rand()%(TLlist[tile].nt);
		tl = &TLlist[tile].n[r];
		if(TLlist[tl->tile].nt != 0)
			{
			ilog_quiet("N Roll again (%s)\n",TLlist[tl->tile].name);
			tl=NULL;
			}
		bail++;
		if(bail>50)
			return NULL;
		} while(!tl);
	return tl;
	}

// Down
if(dx == 0 && dy > 0)
	{
	if(!TLlist[tile].s)
		return NULL;
	if(!TLlist[tile].s[0].tile)
		return NULL;

	// If only one choice, use it

	if(TLlist[tile].st == 0)
		return &TLlist[tile].s[0];

	// otherwise, decide

	r = rand()%(TLlist[tile].st);
	tl = &TLlist[tile].s[r];
	return tl;
	}

// Left
if(dx < 0 && dy == 0)
	{
	if(!TLlist[tile].w)
		return NULL;
	if(!TLlist[tile].w[0].tile)
		return NULL;

	// If only one choice, use it

	if(TLlist[tile].wt == 0)
		return &TLlist[tile].w[0];

	// otherwise, decide

	tl = NULL;
	bail=0;
	do {
		r = rand()%(TLlist[tile].wt);
		tl = &TLlist[tile].w[r];
		if(TLlist[tl->tile].nt != 0)
			{
			ilog_quiet("W Roll again (%s)\n",TLlist[tl->tile].name);
			tl=NULL;
			}
		bail++;
		if(bail>50)
			return NULL;
		} while(!tl);
	return tl;
	}

// Right
if(dx > 0 && dy == 0)
	{
	if(!TLlist[tile].e)
		return NULL;
	if(!TLlist[tile].e[0].tile)
		return NULL;

	// If only one choice, use it

	if(TLlist[tile].et == 0)
		return &TLlist[tile].e[0];

	// otherwise, decide

	tl = NULL;
	bail=0;
	do {
		r = rand()%(TLlist[tile].et);
		tl = &TLlist[tile].e[r];
		if(TLlist[tl->tile].nt != 0)
			{
			ilog_quiet("E Roll again (%s)\n",TLlist[tl->tile].name);
			tl=NULL;
			}
		bail++;
		if(bail>50)
			return NULL;
		} while(!tl);
	return tl;
	}

// Now the other four
/*
// NW
if(dx < 0 && dy < 0)
	{
	if(!TLlist[tile].nw)
		return NULL;
	if(!TLlist[tile].nw[0].tile)
		return NULL;

	// If only one choice, use it

	if(TLlist[tile].nwt == 0)
		return &TLlist[tile].nw[0];

	// otherwise, decide

	r = rand()%(TLlist[tile].nwt);
	tl = &TLlist[tile].nw[r];
	return tl;
	}

// NE
if(dx > 0 && dy < 0)
	{
	if(!TLlist[tile].ne)
		return NULL;
	if(!TLlist[tile].ne[0].tile)
		return NULL;

	// If only one choice, use it

	if(TLlist[tile].net == 0)
		return &TLlist[tile].ne[0];

	// otherwise, decide

	r = rand()%(TLlist[tile].net);
	tl = &TLlist[tile].ne[r];
	return tl;
	}
*/

// SW
if(dx < 0 && dy > 0)
	{
	if(!TLlist[tile].sw)
		return NULL;
	if(!TLlist[tile].sw[0].tile)
		return NULL;

	// If only one choice, use it

	if(TLlist[tile].swt == 0)
		return &TLlist[tile].sw[0];

	// otherwise, decide

	r = rand()%(TLlist[tile].swt);
	tl = &TLlist[tile].sw[r];
	return tl;
	}

// SE
if(dx > 0 && dy > 0)
	{
	if(!TLlist[tile].se)
		return NULL;
	if(!TLlist[tile].se[0].tile)
		return NULL;

	// If only one choice, use it

	if(TLlist[tile].set == 0)
		return &TLlist[tile].se[0];

	// otherwise, decide

	r = rand()%(TLlist[tile].set);
	tl = &TLlist[tile].se[r];
	return tl;
	}

return NULL;
}

// Get the Dot Product Vector to make sure we aren't travelling in
// the opposite direction to what we were just now (prevents weird shit)

int CheckVector(int dx,int dy, int ox, int oy)
{
int dpx,dpy;

dpx = dx*ox;
dpy = dy*oy;

if(dpx+dpy < 0)
	return 0;
return 1;

/*
if(ox < 0 && oy == 0)
	{
	if(dx > 0)
		return 0;
	}

if(ox < 0 && oy == 0)
	{
	if(dx > 0)
		return 0;
	}
*/
}

// Work out the appropriate tile for this direction

TL_DATA *WorkOutTile(int dx, int dy, int tile)
{
int t,r;

ilog_quiet("getting %d\n",tile);
if(tile<0)
	return NULL;

// Check Cardinal directions

// Up
if(dx == 0 && dy < 0)
	{
	if(!TLlist[tile].n)
		return NULL;

	if(TLlist[tile].nt > 0)
		return &TLlist[tile].n[rand()%(TLlist[tile].nt)];
	else
		return &TLlist[tile].n[0];
	}

// Down
if(dx == 0 && dy > 0)
	{
	if(!TLlist[tile].s)
		return NULL;

	if(TLlist[tile].st > 0)
		return &TLlist[tile].s[rand()%(TLlist[tile].st)];
	else
		return &TLlist[tile].s[0];
	}

// Left
if(dx < 0 && dy == 0)
	{
	if(!TLlist[tile].w)
		return NULL;
	if(TLlist[tile].wt > 0)
		return &TLlist[tile].w[rand()%(TLlist[tile].wt)];
	else
		return &TLlist[tile].w[0];
	}

// Right
if(dx > 0 && dy == 0)
	{
	if(!TLlist[tile].e)
		return NULL;
	if(TLlist[tile].et > 0)
		return &TLlist[tile].e[rand()%(TLlist[tile].et)];
	else
		return &TLlist[tile].e[0];
	}

// Now the other four

// NW
if(dx < 0 && dy < 0)
	{
	if(!TLlist[tile].nw)
		return NULL;
	if(TLlist[tile].nwt > 0)
		return &TLlist[tile].nw[rand()%(TLlist[tile].nwt)];
	else
		return &TLlist[tile].nw[0];
	}

// NE
if(dx > 0 && dy < 0)
	{
	if(!TLlist[tile].ne)
		return NULL;
	if(TLlist[tile].net > 0)
		return &TLlist[tile].ne[rand()%(TLlist[tile].net)];
	else
		return &TLlist[tile].ne[0];
	}

// SW
if(dx < 0 && dy > 0)
	{
	if(!TLlist[tile].sw)
		return NULL;
	if(TLlist[tile].swt > 0)
		return &TLlist[tile].sw[rand()%(TLlist[tile].swt)];
	else
		return &TLlist[tile].sw[0];
	}

// SE
if(dx > 0 && dy > 0)
	{
	if(!TLlist[tile].se)
		return NULL;
	if(TLlist[tile].set > 0)
		return &TLlist[tile].se[rand()%(TLlist[tile].set)];
	else
		return &TLlist[tile].se[0];
	}

return NULL;
}

#endif
