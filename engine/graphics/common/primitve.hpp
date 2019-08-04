//
//  Primitives for SDL and other libraries that need them
//

#include "../iregraph.hpp"

typedef void(*ITGLINECALLBACK)(int x, int y, IRECOLOUR *col, IREBITMAP *dest);

extern void ITGline( int xa, int ya, int xb, int yb, IRECOLOUR *colour,IREBITMAP *dest);
extern void ITGcircle(int x, int y, int r, IRECOLOUR *col,IREBITMAP *dest);
extern void ITGcirclefill(int x, int y, int r, IRECOLOUR *col,IREBITMAP *dest);
extern void ITGlinecallback( int xa, int ya, int xb, int yb, ITGLINECALLBACK callback, IRECOLOUR *col, IREBITMAP *dest);
