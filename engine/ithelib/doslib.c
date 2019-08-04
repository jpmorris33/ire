//
//      Emulate certain DOS library functions
//

#ifndef __DJGPP__
#ifndef _WIN32

#include "ithelib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>

//#ifndef ALLEGRO // Allegro emulates the following two itself

// Make a string uppercase

void strupr(char *a)
{
/*
int len,ctr;
len =strlen(a);
for(ctr=0;ctr<len;ctr++)
    a[ctr]=toupper(a[ctr]);
*/
for(;*a;a++)
	*a=toupper(*a);
}

// Make a string lowercase

void strlwr(char *a)
{
/*
int len,ctr;
len =strlen(a);

for(ctr=0;ctr<len;ctr++)
    a[ctr]=tolower(a[ctr]);
*/
for(;*a;a++)
	*a=tolower(*a);
}

// stricmp moved to istricmp in strlib.c

//#endif

// integer to ascii

#ifndef SEER

char *itoa(int value, char *string, int radix)
{
switch(radix)
    {
    case 10:
    sprintf(string,"%d",value);
    break;

    case 16:
    sprintf(string,"%x",value);
    break;

    default:
    sprintf(string,"%d",radix);
    ithe_panic("itoa() - unsupported radix",string);
    break;
    };

return string;
}
#endif


long filelength(int fhandle)
{
struct stat s;
fstat(fhandle,&s);
return s.st_size;
}

#endif // Not Win32
#endif // Not DJGPP

