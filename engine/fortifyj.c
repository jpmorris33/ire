//
//	Use a modified version of fortify (that can't be distributed)
//	But only if fortify is enabled in memory.hpp
//

#define __FORTIFY_C__

#include "ithelib.h"

#ifdef FORTIFY
//#include "c:/fortify/fortify2.c"
#include "../fortify/fortify2.c"
#endif
