//
//      String manipulation functions
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <strings.h>
#endif
#include <ctype.h>

#include "ithelib.h"

#define MAXBUFS 25 // May need increasing later

static char localbufarray[MAXBUFS][MAX_LINE_LEN];

char NOTHING[]="\0";

static void overlapped_strcpy(char *out,char *in);


/*
 *      isXspace - is the character under scrutiny whitespace or not?
 *                 Two versions: export and inline
 */

char isxspace(unsigned char input)
{
if(!input)
	return 0;
return input<=32?1:0;
}

static __inline char iisxspace(unsigned char input)
{
if(!input)
	return 0;
return input<=32?1:0;
}


/*
 *      rest - A LISP string processing routine.
 *             rest returns all but the first word of a string.
 *             e.g. rest("1 2 3") returns "2 3"
 */

const char *strrest(const char *input)
{
short ctr,len,ptr,ptr2,ptr3;

if(input==NULL)
	return NOTHING;

len=strlen(input);
ptr=-1;
for(ctr=0;ctr<len;ctr++)
	if(!iisxspace(input[ctr]))
		{
		ptr=ctr;
		break;
		}
if(ptr==-1)
	return NOTHING;

ptr2=-1;
for(ctr=ptr;ctr<len;ctr++)
	if(iisxspace(input[ctr]))
		{
		ptr2=ctr;
		break;
		}

if(ptr2==-1)
	return NOTHING;

ptr3=-1;
for(ctr=ptr2;ctr<len;ctr++)
	if(!iisxspace(input[ctr]))
		{
		ptr3=ctr;
		break;
		}
if(ptr3==-1)
	return NOTHING;

return &input[ptr3];
}

/*
 *      first - A LISP string processing routine.
 *              first returns only the first word of a string.
 *              e.g. first("1 2 3") returns "1"
 *
 *              first returns a copy of the string, it does not modify the
 *              original string.  Use hardfirst() if you want this to happen.
 */

char *strfirst(const char *input)
{
char *tbuf;
char *p,*q,ok;

tbuf = strLocalBuf();

// Simple cases first

if(input==NULL)
	return NOTHING;
if(input==NOTHING)
	return NOTHING;
if(input[0]=='\0' || input[0]=='\n')
	return NOTHING;
if(strlen(input) == 0)
	return NOTHING;

// Then the rest

strcpy(tbuf,input);
p=strchr(tbuf,' ');
if(p)
	*p=0;

p=tbuf;

// Now we need to know if there's anything useful there

if(*p == 0)
	return NOTHING;  // No there isn't

// Examine each character

ok=0;
for(q=p;*q;q++)
	if(!iisxspace(*q))
		ok=1;

if(!ok)
	return NOTHING;  // Only whitespace

if(*p == 0)
	return NOTHING;  // Nothing there

if(strlen(p) == 0)
	return NOTHING;

return p;
}

/*
 *      hardfirst - Like 'first' but modifies the original string.
 */

char *hardfirst(char *line)
{
char *ptr;
char *out;

ptr = strfirst(line);

// got the first item, now search for it to find the address

out = strstr(line,ptr);
if(!out)
	ithe_panic("Oh no!  The string vanished",ptr);

ptr = (char *)strrest(out);        // Got beginning of second item
if(ptr != NOTHING)
	*(ptr-1) = 0;       // Wind back, and punch a hole

// We now have the name, but first we must strip off
// all the whitespace or the stricmp will fail later

strstrip(out);

return out;
}

/*
 *      last - return last item
 *             e.g. last("1 2 3") returns "3"
 */

char *strlast(const char *a)
{
return(strgetword(a,strcount(a)));
}

/*
 *      count - Return the number of words in the sentence
 */

int strcount(const char *a)
{
int rc;
char tvb[MAX_LINE_LEN];
char tvb2[MAX_LINE_LEN];
const char *p;

// First look for an empty string (0 words)

if(!a[0])
	return 0;

for(p=a;iisxspace(*p);p++);
if(!*p)
	return 0;

// Now do the rest of the job

strcpy(tvb,a);
rc = 0;
do	{
	p = strrest(tvb);
	strcpy(tvb2,p);
	strcpy(tvb,tvb2);
	rc++;
	} while(p != NOTHING);
return rc;
}

/*
 *  word - Return the word at the specified position
 *             e.g. word("a b c",2) = 'b'
 */

char *strgetword(const char *a, int pos)
{
char tvb[MAX_LINE_LEN];
char *p;
int rc=1;

if(!a)
	return NOTHING;

// Hack for 1

if(pos == 1)
	return(strfirst(a));

// If it starts with whitespace, frig it otherwise the whitespace will actually count as the first one
if(iisxspace(*a))
	rc=0; // Compensate for this

// This for the rest..

strcpy(tvb,a);
for(p=&tvb[0];*p;p++)
	if(iisxspace(*p))
		{
		*p=0;
		if(!iisxspace(*(p+1))) // Must be just 1 space to count
			rc++;
		if(rc == pos)
			return strfirst(++p);
		}
return NOTHING;
}

/*
 *      Strip - Shorten a string to remove any trailing spaces
 */

void strstrip(char *tbuf)
{
char *tptr;

// Sanity checks first

if(tbuf == NOTHING || tbuf == NULL)
	return;                     // There is nothing there!  abort

// Check for too short string

if(strlen(tbuf) <= 1)
	return;                     // There is nothing there!  abort

// Check for just a lot of spaces

if(strcount(tbuf) == 0)      // how many words?
	return;                     // There is nothing there!  abort

// Find first non-whitespace character
for(tptr = tbuf;iisxspace(*tptr);tptr++);

// Make sure there was something

if(!*tptr)
	return;

// Tptr is now the beginning of the 'pure' string
overlapped_strcpy(tbuf,tptr);

// Now we need to find the whitespace character at the end of the string,
// if there is one

// Wind to end

for(tptr = tbuf;*tptr;tptr++);

// Find first non-whitespace character

--tptr;

// Ok, we're at the end of the string.  If there's no trailing space, we're
// fine and don't need do do any more work

if(!iisxspace(*tptr))
	return;

// Find the first character that's not a space

for(;iisxspace(*tptr);tptr--);

// Hit it

*(++tptr)=0;
}


/*
 *      Getnumber - Return the numeric value of a string
 *                  Support for constants can be re-added later if needed
 */

int strgetnumber(const char *number)
{
int ctr;

ctr=atoi(number);
return ctr;
}

/*
 *      Isnumber - is the string a number?
 */

int strisnumber(const char *thing)
{
int ctr,Xc;

Xc=strlen(thing);
if(Xc<1)
	return 0; // NO!

for(ctr=0;ctr<Xc;ctr++)
	if((thing[ctr]<'0'||thing[ctr]>'9')&&thing[ctr]!='-'&&thing[ctr]!='+')
		return 0L;
return 1;
}

/*
 *      slash - Convert path slashes to UNIX format
 */

void strslash(char *a)
{
for(;*a;a++)
	if(*a == '\\')
		*a='/';
}

/*
 *      slash - Convert path slashes to UNIX format
 */

void strwinslash(char *a)
{
for(;*a;a++)
	if(*a == '/')
		*a='\\';
}


/*
 * We can't return the address of a local buffer because it will change, so
 * we use this to provide scratch space instead.  The buffer it returns is
 * one of many so we don't need to worry about recursive calls overwriting
 * each other's scratch space
 */

char *strLocalBuf()
{
static int bufctr=0;
char *b;

bufctr++;
if(bufctr>=MAXBUFS)
	bufctr=0;
b=&localbufarray[bufctr][0];

*b=0; // Blank it for the function
return b;
}

/*
 *      istricmp - efficient case-insensitive string compare
 */

int istricmp(const char *a, const char *b)
{
if(!a || !b)
	return 666;

for(;;)
	{
	if((unsigned char)*a != (unsigned char)*b)
		if(tolower((unsigned char)*a) != tolower((unsigned char)*b))
			return (int)tolower((unsigned char)*a) - (int)tolower((unsigned char)*b);
	if(!*a)
		return 0;
	a++;
	b++;
	}

ithe_panic("Error in ithelib.a","666 in istricmp");
return 666;
}

/*
 *      istricmp_fuzzy - efficient case-insensitive fuzzy string compare
 */

int istricmp_fuzzy(const char *a, const char *b)
{
if(!a || !b)
	return 666;

for(;;)
	{
	if((unsigned char)*a != (unsigned char)*b)
		if(tolower((unsigned char)*a) != tolower((unsigned char)*b))
			{
			// '*' matches all
			if(*a=='*' || *b=='*')
				return 0;
			return (int)tolower((unsigned char)*a) - (int)tolower((unsigned char)*b);
			}
	if(!*a)
		return 0;
	a++;
	b++;
	}

ithe_panic("Error in ithelib.a","666 in istricmp_fuzzy");
return 666;
}

/*
 *   strdeck(string,char) - Find all occurences of a character and "deck 'em"
 *                          i.e. turn them into spaces
 */

void strdeck(char *line,char c)
{
char *p;
do
	{
	p=strchr(line,c);
	if(p)
		*p=' ';
	} while(p);
}

//
//  This is to shut valgrind up
//

void overlapped_strcpy(char *out,char *in)
{
int ctr,len;
if(!out || !in)
	return;

len=strlen(in)+1;

for(ctr=0;ctr<len;ctr++)
	{
	*out=*in;
	out++;
	in++;
	}
}



char *stradd(char *old, const char *extra)
{
int extralen,oldlen;
char *newstring;

if(!extra)
	return NULL;
extralen = strlen(extra);

if(!old)
	{
	newstring = (char *)M_get(1,extralen+1);
	if(newstring)
		strcpy(newstring,extra);
	return newstring;
	}
	
oldlen = strlen(old);
newstring = (char *)M_resize(old, oldlen+extralen+1);
if(newstring)
	{
	strcpy(newstring + oldlen,extra);
	return newstring;
	}

return old;
}
