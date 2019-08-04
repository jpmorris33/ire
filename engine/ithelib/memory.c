/*
 *      Memory management, including optional bounds-checker (FORTIFY)
 */

#define ILIB_MMOD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ithelib.h"

// Variables

char log_mem_usage=0;

#ifdef ITHE_DEBUG
#define MAX_PTRS 250000
static unsigned long len[MAX_PTRS];
static void *ptr[MAX_PTRS];
static int mctr;
static char msg[128];
#endif

// Functions

static void memerror(char *msg,long mem);

// Code

/*
 *      M_init - very little at the moment
 */

void M_init()
{
remove("fortify.out");  // Remove any debugging file for a new one if needed
#ifdef ITHE_DEBUG
memset(len,0,MAX_PTRS);
#endif
}

/*
 *      M_term - Not needed at present
 */

void M_term()
{
}

/*
 *      M_get - allocate a block of memory
 */
#ifndef FORTIFY
void *M_get(unsigned int quantity,long wanted)
{
void *temp;
long qty;

qty=quantity*wanted;

if(quantity<1)
	memerror("M_get() - Silly quantity requested: ",quantity);
if(wanted<1)
	memerror("M_get() - Silly amount requested: ",wanted);

temp=calloc(quantity+1,wanted);
if(!temp)
	memerror("M_get() - Out of memory requesting: ",qty+wanted);

memset(temp,0,qty); // Zero cool

#ifdef ITHE_DEBUG
for(mctr=0;mctr<MAX_PTRS;mctr++)
	if(len[mctr] == 0)
		{
		len[mctr]=qty;
		ptr[mctr]=temp;
		return temp;
		}
ithe_panic("Internal error in M_get - Too many pointers",NULL);
#endif

return temp;
}

/*
 *      M_resize - reallocate a block of memory
 */
void *M_resize(void *old,long qty)
{
void *temp;

#ifdef ITHE_DEBUG
if(!old)
	return M_get(1,qty);
#endif

if(qty<1)
	memerror("M_resize() - Silly quantity requested: ",qty);

temp=realloc(old,qty);
if(!temp)
	memerror("M_resize() - Out of memory requesting: ",qty);

#ifdef ITHE_DEBUG
// Modify list
for(mctr=0;mctr<MAX_PTRS;mctr++)
	if(ptr[mctr] == old)
		{
		len[mctr]=qty;
		ptr[mctr]=temp;
		return temp;
		}
#endif

return temp;
}

void *M_get_log(unsigned int quantity,long wanted,int l,char *f)
{
void *temp;
temp = M_get(quantity,wanted);
ilog_quiet("Alloc %d [%p] at line %d in %s\n",quantity*wanted,temp,l,f);
return temp;
}


void M_free_log(void *m,int l,char *f)
{
ilog_quiet("Free [%p] at line %d in %s\n",m,l,f);
M_free(m);
}

/*
 *      M_free - free a block of memory
 */

void M_free(void *M_bank)
{

if(M_bank)
    free(M_bank);
else
    ithe_panic("Attempting to free a NULL pointer",NULL);

#ifdef ITHE_DEBUG
for(mctr=0;mctr<MAX_PTRS;mctr++)
	if(ptr[mctr] == M_bank)
		{
		len[mctr]=0;
		ptr[mctr]=NULL;
		return;
		}
sprintf(msg,"0x%p",M_bank);
ithe_panic("M_free - Pointer not allocated by M_get",msg);
#endif

M_bank = NULL;

}

#endif

int M_chk(void *M_bank)
{
#ifdef ITHE_DEBUG
for(mctr=0;mctr<MAX_PTRS;mctr++)
	{
	// Is it in the list directly?
	if(ptr[mctr] == M_bank)
		return 1;
	// Is it within this object's scope?
	if((unsigned long)M_bank > (unsigned long)ptr[mctr])
		if((unsigned long)M_bank <= (unsigned long)ptr[mctr]+len[mctr])
			return 1;
	}
CRASH();
//sprintf(msg,"0x%p",M_bank);
//ithe_panic("M_free - Pointer not allocated by M_get",msg);
#endif

return 1;
}

static void memerror(char *msg,long mem)
{
char buf[64];
sprintf(buf,"%ld bytes",mem);
ithe_panic(msg,buf);
}
