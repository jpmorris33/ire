//
//	Include a modified version of Fortify (which can't be distributed)
//

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FORTIFY
//#include "c:/fortify/fortify.h"
#include <fortify/fortify.h>
#else
#define Fortify_CheckAllMemory(); ;
#define ChkSys(x); ;
#endif

#ifdef __cplusplus
}
#endif
