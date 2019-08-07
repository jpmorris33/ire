//
//		Keyword management
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

//#define LOG_FINDING		// Show all the variable finding for debugging

#include "opcodes.h"
#include "pe_api.hpp"
#include "../init.hpp"
#include "../ithelib.h"
#include "../console.hpp"
#include "../media.hpp"
#include "../sound.h"
#include "../textfile.h"

KEYWORD **klist[256];
int klistlen[256];
extern char *compilename;
extern STRUCTURE *pe_datatype;

static int FastLookup=0; // When the lists are in order, we can do bsearch

KEYWORD *add_keyword(const char *name, char type, const char *func);
KEYWORD *add_symbol_ptr(const char *name, char type, void *value);
KEYWORD *add_symbol_val(const char *name, char type, VMINT value);
int get_keyword(const char *name, char type, const char *func);
static KEYWORD **get_keywordlist(const char *name, int *start, int *end);
KEYWORD *find_keyword(const char *name, char type, const char *func);
static KEYWORD *find_keyword_slow(const char *name, char type, const char *func);
KEYWORD *find_keyword_global(const char *name, char type, const char *func);
KEYWORD *find_keyword_global_slow(const char *name, char type, const char *func);
KEYWORD *find_keyword_local(const char *name, char type, const char *func);
static KEYWORD *find_keyword_local_slow(const char *name, char type, const char *func);
KEYWORD *find_datatype_global(const char *name, const char *func);
KEYWORD *find_datatype_local(const char *name, const char *func);
KEYWORD *find_variable_global(const char *name, const char *func);
KEYWORD *find_variable_local(const char *name, const char *func);
int wipe_keywords();
static int wipe_keywords_core(int list);


static int CompareFastSort(const void *a, const void *b)
{
KEYWORD **ax=(KEYWORD **)a,**bx=(KEYWORD**)b;
return istricmp((*ax)->name,(*bx)->name);
}

static int CompareFastSearch(const void *key, const void *member)
{
KEYWORD *ptr=*(KEYWORD **)member;
/*
printf("key = %x\n",key);
printf("key = '%s'\n",key);
printf("ptr = %x\n",ptr);
printf("ptr->name = %x\n",ptr->name);
printf("ptr->name = '%s'\n",ptr->name);
//CRASH();
*/
return istricmp((char *)key,ptr->name);
}

// add_keyword - store a new label, function or variable in the list

KEYWORD *add_keyword(const char *name, char type, const char *func)
{
KEYWORD *k;
KEYWORD **newlist;
//int idno=0,kl,ctr;
static int idno=0;
int kl,ctr;


if(FastLookup)
	{
	// Error, the list is already set in stone
	#ifndef _WIN32
	// I don't have bloody time to babysit MSVC
	CRASH();
	#endif
	return NULL;
	}

// Get list representing character
kl=tolower(name[0]);

// Check for pre-existence
if(klist[kl])
	for(ctr=0;ctr<klistlen[kl];ctr++)
		{
		k=klist[kl][ctr];
		if(!k)
			break;
		if(k->local == func)
			if(type == k->type || type == '?' || k->type == '?')
				if(!istricmp(k->name,name))
					return NULL; // Duplicate entry!
//		idno++;
		}

// Add to list
klistlen[kl]++;
//idno=klistlen[kl];
idno++;

newlist = (KEYWORD **)M_resize(klist[kl],klistlen[kl]*sizeof(KEYWORD *));
k=(KEYWORD *)M_get(1,sizeof(KEYWORD));
newlist[klistlen[kl]-1]=k;
klist[kl]=newlist;

SAFE_STRCPY(k->name,name);
k->value = (void *)idno;
k->type = type;
k->preconfigured = false;

if(func)
	{
	k->local = func;
	k->refcount=0; // Check for unused stuff corrupting the localtab
	}
k->localfile=pe_localfile;
return k;
}

//
//  Sort and index the keywords
//

void compiler_sortkeywords()
{
int ctr,kl,currentindex=0;
char currentname[128];
KEYWORD *ptr;

// Sort, and re-index all the lists
for(kl=0;kl<255;kl++)
	if(klist[kl])
		{
		qsort(klist[kl],klistlen[kl],sizeof(KEYWORD*),CompareFastSort);
		// Set the base index number of the object
		SAFE_STRCPY(currentname,"");
		currentindex=0;
		for(ctr=0;ctr<klistlen[kl];ctr++)
			{
			ptr=klist[kl][ctr];
			// It's changed, from now on, use the current index
			if(istricmp(ptr->name,currentname))
				{
				currentindex=ctr;
				SAFE_STRCPY(currentname,ptr->name);
				}
			ptr->index=currentindex;

			// Also, set up the linked list, since some tasks are easier this way
			if(ctr<(klistlen[kl]-1))
				ptr->next=klist[kl][ctr+1];
			else
				ptr->next=NULL;
			}
		}

FastLookup=1;
}

// add_symbol_ptr - Create a predefined keyword

KEYWORD *add_symbol_ptr(const char *name, char type, void *value)
{
KEYWORD *k;

k = add_keyword(name,type,NULL);
if(k)
	{
	k->id = -1;
	k->value=value;
//	ilog_quiet("Adding symbol %s which points to %x\n",name,value);
	}

return k;
}

// add_symbol_val - Create a predefined keyword

KEYWORD *add_symbol_val(const char *name, char type, VMINT value)
{
KEYWORD *k;

k = add_keyword(name,type,NULL);
if(k)
	{
	k->id = -1;
	k->value=(void *)value;
	}

return k;
}

void AddScriptConstant(const char *name, VMINT value)
{
KEYWORD *k;
k=add_symbol_val(name,'n',value);
if(k) {
	k->preconfigured=true;
}
}


// return the ID of a keyword

int get_keyword(const char *name, char type, const char *func)
{
KEYWORD *k;
int idno=0;
int kl,ctr;

kl=tolower(name[0]);

if(!klist[kl])
	{
//	ilog_quiet("Nothing for '%c'\n",tolower(name[0]));
	return -1;
	}

for(ctr=0;ctr<klistlen[kl];ctr++)
	{
	k=klist[kl][ctr];
	if(k->local == func || k->local == NULL)
		if(type == k->type || type == '?' || k->type == '?')
			if(!istricmp(k->name,name))
				return idno; // Found it!
	idno++;
	}

return -1;
}

//
//  Get the first entry with this name from the relevant list
//

KEYWORD **get_keywordlist(const char *name, int *start, int *end)
{
int kl=tolower(name[0]);
KEYWORD **k;
int pos;

k=(KEYWORD **)bsearch(name,klist[kl],klistlen[kl],sizeof(KEYWORD*),CompareFastSearch);
if(!k || !*k)
	return NULL;

/*
// If what we got is not what we asked for something has gone VERY wrong
if(istricmp((*k)->name,name))
	{
	CRASH();
	}
*/

// Find base index, if possible.  This means winding back, because the
// binary search has probably found one of the 'middle' entries not
// the first entry.

pos=(*k)->index;
while(pos>0)
	{
	pos--;
	if(istricmp(klist[kl][pos]->name,name))
		{
		// Gone too far, set it back
		pos++;
		break;
		}
	};

*start=pos;
*end=klistlen[kl];

return klist[kl];
}

// return the pointer to a keyword, local or global

KEYWORD *find_keyword(const char *name, char type, const char *func)
{
KEYWORD *k,**list;
int ctr,start,end;

if(!FastLookup)
	return find_keyword_slow(name,type,func);

list = get_keywordlist(name,&start,&end);
//printf("GKL returned %d-%d = %x\n",start,end,list);

if(!list)
	return NULL;
	
for(ctr=start;ctr<end;ctr++)
	{
	k=list[ctr];

	// If the name is wrong, we've reached the end
	if(istricmp(k->name,name))
		{
//		printf("%s <> %s\n",k->name,name);
		return NULL;
		}

	if(type == k->type || type == '?' || k->type == '?')
		{
		// Check locals first..
		if(k->local == func)
			if(k->localfile == compilename || k->localfile == NULL)
				return k; // Found it!
		
		// Check globals
		if(k->local == NULL)
			{
			if(k->localfile == compilename)
				return k; // Found it!
			// Now search everywhere
			if(k->localfile == NULL)
				return k; // Found it!
			}
			
		}
	}

return NULL;
}

KEYWORD *find_keyword_slow(const char *name, char type, const char *func)
{
KEYWORD *k;
int kl,ctr;

kl=tolower(name[0]);

if(!klist[kl])
	return NULL;

for(ctr=0;ctr<klistlen[kl];ctr++)
	{
	k=klist[kl][ctr];
	if(istricmp(k->name,name))
		continue;
	
	if(type == k->type || type == '?' || k->type == '?')
		{
		// Check locals first..
		if(k->local == func)
			if(k->localfile == compilename || k->localfile == NULL)
				return k; // Found it!

			
		// Check globals
		if(k->local == NULL)
			{
			if(k->localfile == compilename)
				return k; // Found it!
			// Now search everywhere
			if(k->localfile == NULL)
				return k; // Found it!
			}
		}
	}

// Oh well

return NULL;
}

// return the pointer to a keyword local to a specific function

KEYWORD *find_keyword_local(const char *name, char type, const char *func)
{
KEYWORD *k,**list;
int ctr,start,end;

if(!func)
	return NULL;

if(!FastLookup)
	return find_keyword_local_slow(name,type,func);

list=get_keywordlist(name,&start,&end);
if(!list)
	return NULL;

for(ctr=start;ctr<end;ctr++)
	{
	k=list[ctr];
	// If the name is wrong, we've reached the end
	if(istricmp(k->name,name))
		{
		return NULL;
		}

	if(k->local == func)
		if(type == k->type || type == '?' || k->type == '?')
			if(k->localfile == compilename || k->localfile == NULL)
//				if(!istricmp(k->name,name))
					{
					return k; // Found it!
					}
	}
	
// Oh well
return NULL;
}


KEYWORD *find_keyword_local_slow(const char *name, char type, const char *func)
{
KEYWORD *k;
int kl,ctr;

if(!func)
	return NULL;

kl=tolower(name[0]);
if(!klist[kl])
	return NULL;

for(ctr=0;ctr<klistlen[kl];ctr++)
	{
	k=klist[kl][ctr];
	if(k->local == func)
		if(type == k->type || type == '?' || k->type == '?')
			if(k->localfile == compilename || k->localfile == NULL)
				if(!istricmp(k->name,name))
					return k; // Found it!
	}
	
// Oh well

return NULL;
}


// find the pointer to a keyword regardless of scope

KEYWORD *find_keyword_global(const char *name, char type, const char *func)
{
KEYWORD *k,**list;
int ctr,start,end;

if(!FastLookup)
	return find_keyword_global_slow(name,type,func);

list = get_keywordlist(name,&start,&end);
if(!list)
	return NULL;

// Look in this file alone first
for(ctr=start;ctr<end;ctr++)
	{
	k=list[ctr];

	// If the name is wrong, we've gone too far, so stop
	if(istricmp(k->name,name))
		break;

	if(type == k->type || type == '?' || k->type == '?')
		if(k->local == func || k->local == NULL || func == NULL)
			if(k->localfile == compilename)
				return k; // Found it!
	}

// Now look in all files

for(ctr=start;ctr<end;ctr++)
	{
	k=list[ctr];
	// If the name is wrong, we've gone too far, so stop
	if(istricmp(k->name,name))
		break;
		
	if(type == k->type || type == '?' || k->type == '?')
		if(k->local == func || k->local == NULL || func == NULL)
			if(k->localfile==NULL)
				if(!istricmp(k->name,name))
					return k; // Found it!
	}
return NULL;
}


KEYWORD *find_keyword_global_slow(const char *name, char type, const char *func)
{
KEYWORD *k;
int kl,ctr;

kl=tolower(name[0]);

if(!klist[kl])
	return NULL;

// Look in this file alone first
for(ctr=0;ctr<klistlen[kl];ctr++)
	{
	k=klist[kl][ctr];
	if(type == k->type || type == '?' || k->type == '?')
		if(k->local == func || k->local == NULL || func == NULL)
			if(k->localfile == compilename)
				if(!istricmp(k->name,name))
					return k; // Found it!
	}

// Now look in all files

for(ctr=0;ctr<klistlen[kl];ctr++)
	{
	k=klist[kl][ctr];
	if(type == k->type || type == '?' || k->type == '?')
		if(k->local == func || k->local == NULL || func == NULL)
			if(k->localfile==NULL)
				if(!istricmp(k->name,name))
					return k; // Found it!
	}
return NULL;
}


// return the pointer to a keyword local to a specific function

KEYWORD *find_datatype_local(const char *name, const char *func)
{
KEYWORD *k;
int ctr;

for(ctr=0;pe_datatypes[ctr];ctr++)
	{
	k = find_keyword_local(name, pe_datatypes[ctr]->type,func);
	if(k)
		{
		pe_datatype=pe_datatypes[ctr];
		return k;
		}
	}
pe_datatype=NULL;
return NULL;
}

KEYWORD *find_datatype_global(const char *name, const char *func)
{
KEYWORD *k;
int ctr;

for(ctr=0;pe_datatypes[ctr];ctr++)
	{
	k = find_keyword_global(name, pe_datatypes[ctr]->type,func);
	if(k)
		{
		pe_datatype=pe_datatypes[ctr];
		return k;
		}
	}
pe_datatype=NULL;
return NULL;
}

KEYWORD *find_variable_local(const char *name, const char *func)
{
KEYWORD *k;
int ctr;

for(ctr=0;pe_vartypes[ctr];ctr++)
	{
	k = find_keyword_local(name, pe_vartypes[ctr],func);
	if(k)
		return k;
	}
return NULL;
}

KEYWORD *find_variable_global(const char *name, const char *func)
{
KEYWORD *k;
int ctr;

for(ctr=0;pe_vartypes[ctr];ctr++)
	{
	k = find_keyword_global(name, pe_vartypes[ctr],func);
	if(k)
		return k;
	}
return NULL;
}


// wipe_keywords

int wipe_keywords()
{
int ctr;
for(ctr=0;ctr<255;ctr++)
	wipe_keywords_core(ctr);
return 1;
}

static int wipe_keywords_core(int list)
{
int ctr;

if(!klist[list])      // Haven't started!
	return 0;

// Free everything array-style since it's easier than the linked-list way

for(ctr=0;ctr<klistlen[list];ctr++)
	if(klist[list][ctr])
		M_free(klist[list][ctr]);
M_free(klist[list]);
klist[list]=NULL;

return 1;
}

