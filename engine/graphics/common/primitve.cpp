//
//  Primitives for SDL and other libraries that need them
//
//  Taken from the IT-HE Graphics library, circa 1995-1999
//

#include <stdlib.h>

#include "primitve.hpp"

void ITGline( int xa, int ya, int xb, int yb, IRECOLOUR *colour,IREBITMAP *dest)
{
	register int t, distance;
	int  xerr=0, yerr=0, delta_x, delta_y;
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
		dest->PutPixel(xa, ya, colour);
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

void ITGlinecallback( int xa, int ya, int xb, int yb, ITGLINECALLBACK callback, IRECOLOUR *col, IREBITMAP *dest)
{
	register int t, distance;
	int  xerr=0, yerr=0, delta_x, delta_y;
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
		callback(xa,ya,col,dest);
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

// This is based on the ellipse routine and cut down

void ITGcircle(int x,int y,int r,IRECOLOUR *colour,IREBITMAP *dest)
{
	int	zx = 0;
	int	zy = r;

	int	Rsquared = r * r;		// initialize values outside
	int	TwoRsquared = 2 * Rsquared;	//  of loops

	int	d;
	int	dx,dy;


	d = Rsquared - Rsquared*r + Rsquared/4L;
	dx = 0;
	dy = TwoRsquared * r;

	while (dx<dy)
	{
	dest->PutPixel(x+zx, y+zy, colour);
	dest->PutPixel(x-zx, y+zy, colour);
	dest->PutPixel(x+zx, y-zy, colour);
	dest->PutPixel(x-zx, y-zy, colour);

	  if (d > 0L)
	  {
	    --zy;
	    dy -= TwoRsquared;
	    d -= dy;
	  }

	  ++zx;
	  dx += TwoRsquared;
	  d += Rsquared + dx;
	}


	d += (3L*(Rsquared-Rsquared)/2L - (dx+dy)) / 2L;

	while (zy>=0)
	{
	dest->PutPixel(x+zx, y+zy, colour);
	dest->PutPixel(x-zx, y+zy, colour);
	dest->PutPixel(x+zx, y-zy, colour);
	dest->PutPixel(x-zx, y-zy, colour);

	  if (d < 0L)
	  {
	    ++zx;
	    dx += TwoRsquared;
	    d += dx;
	  }

	  --zy;
	  dy -= TwoRsquared;
	  d += Rsquared - dy;
	}
}


void ITGcirclefill(int x,int y,int r,IRECOLOUR *colour,IREBITMAP *dest)
{
	int	zx = 0;
	int	zy = r;

	int	Rsquared = r * r;		// initialize values outside
	int	TwoRsquared = 2 * Rsquared;	//  of loops

	int	d;
	int	dx,dy;


	d = Rsquared - Rsquared*r + Rsquared/4L;
	dx = 0;
	dy = TwoRsquared * r;

	while (dx<dy) {
		dest->HLine(x-zx,y+zy,zx+zx,colour);
		if(y+zy != y-zy) {
			// Prevent overdraw (e.g. on additive blender)
			dest->HLine(x-zx,y-zy,zx+zx,colour);
		}

		if (d > 0L) {
			--zy;
			dy -= TwoRsquared;
		d -= dy;
	  	}

		++zx;
		dx += TwoRsquared;
		d += Rsquared + dx;
	}


	d += (3L*(Rsquared-Rsquared)/2L - (dx+dy)) / 2L;

	while (zy>=0) {
		dest->HLine(x-zx,y+zy,zx+zx,colour);
		if(y+zy != y-zy) {
			// Prevent overdraw (e.g. on additive blender)
			dest->HLine(x-zx,y-zy,zx+zx,colour);
		}

		if (d < 0L) {
			++zx;
			dx += TwoRsquared;
			d += dx;
		}

		--zy;
		dy -= TwoRsquared;
		d += Rsquared - dy;
	}
}

//
//  This looks kind of pretty actually
//
#if 0
void ITGborkedcirclefill(int x,int y,int r,IRECOLOUR *colour,IREBITMAP *dest)
	int	zx = 0;
	int	zy = r;

	int	Rsquared = r * r;		// initialize values outside
	int	TwoRsquared = 2 * Rsquared;	//  of loops

	int	d;
	int	dx,dy;


	d = Rsquared - Rsquared*r + Rsquared/4L;
	dx = 0;
	dy = TwoRsquared * r;

	while (dx<dy) {
		dest->HLine(x-zx,y+zy,zx+zx,colour);
		dest->HLine(x-zx,y-zy,zx+zx,colour);

		if (d > 0L) {
			--zy;
			dy -= TwoRsquared;
		d -= dy;
	  	}

		++zx;
		dx += TwoRsquared;
		d += Rsquared + dx;
	}


	d += (3L*(Rsquared-Rsquared)/2L - (dx+dy)) / 2L;

	while (zy>=0) {
		dest->HLine(x-zx,y+zy,zx+zx,colour);
		dest->HLine(x-zx,y-zy,zx+zx,colour);

		if (d < 0L) {
			++zx;
			dx += TwoRsquared;
			d += dx;
		}

		--zy;
		dy -= TwoRsquared;
		d += Rsquared - dy;
	}
}
#endif
