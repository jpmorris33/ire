/*
 *   NPC link checker 1.4
 *
 *   This is a quick hack, not the epitomy of optimisation or coding style
 *   Should be portable though
 *
 *   This program is in the public domain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined( __linux__) || defined(__BEOS__) || defined(__APPLE__)
#define NO_STRICMP
#endif

char **pageindex;
int pages;


#ifdef NO_STRICMP
int stricmp(char *a,char *b)
{
for(;;)
	{
	if(*a != *b)
		if(tolower(*a) != tolower(*b))
			return (int)tolower(*a) - (int)tolower(*b);
	if(!*a)
		return 0;
	a++;
	b++;
	}
return 0;
}
#endif



int find_page(char *page)
{
int ctr;

// Check for intrinsics
if(!stricmp(page,"exit"))
	return 1;

// Check page index
for(ctr=0;ctr<pages;ctr++)
	{
	if(!stricmp(page,pageindex[ctr]))
		{
//		printf("%s: found link %s [%d/%d]\n",page,pageindex[ctr],ctr,pages);
		return 1;
		}
	}
return 0;
}

void process_file(char *fname)
{
FILE *fp;
char buffer[1024];
int ctr,line,errors,links=0,llink=0,rlink=0;
char *p,*ptr1,*ptr2,*bufptr;
char curpage[128];

printf("Checking %s\n",fname);
strcpy(curpage,"<unknown>");

fp=fopen(fname,"r");
if(!fp)
	{
	printf("Cannot open file %s\n",fname);
	return;
	}

errors=0;

// First, count pages

pages=0;
do
	{
	bufptr=fgets(buffer,1024,fp);
	if(!bufptr)
		break;
	if(strstr(buffer,"[page=\""))
		pages++;
	} while(!feof(fp));

if(!pages)
	{
	printf("Warning: No pages defined!\n");
	fclose(fp);
	return;
	}

// Now allocate room for the pages

pageindex=(char **)calloc(pages,sizeof(char *));
if(!pageindex)
	{
	printf("Could not allocate page index, quitting\n");
	exit(1);
	}

for(ctr=0;ctr<pages;ctr++)
	{
	pageindex[ctr]=(char *)malloc(128);
	if(!pageindex[ctr])
		{
		printf("Out of memory, quitting\n");
		exit(1);
		}
	}

// Back to start
fseek(fp,0L,SEEK_SET);

// Now read in the page index

ctr=0;
line=0;
do
	{
	bufptr=fgets(buffer,1024,fp);
	if(!bufptr)
		break;
	line++;
	if(strstr(buffer,"[page=\""))
		{
		// Find opening quote
		ptr1=strchr(buffer,'\"');
		ptr1++;
		// Find ending quote
		ptr2=strrchr(ptr1,'\"');
		if(!ptr2)
			{
			printf("No ending quote in page declaration at line %d\n",line);
			errors++;
			continue;
			}
		*ptr2=0; // Kill the ending quote
		strcpy(pageindex[ctr],ptr1);
		ctr++;
		}
	} while(!feof(fp));


// Back to start again
fseek(fp,0L,SEEK_SET);

// Now find the links

line=0;
do
	{
	bufptr=fgets(buffer,1024,fp);
	if(!bufptr)
		break;
	line++;

	// If it's a page declaration, reset links count
	if(strstr(buffer,"[page=\""))
		{
		links=0;
		// Get current page name
		ptr1=strchr(buffer,'\"');
		ptr1++;
		strcpy(curpage,ptr1);
		ptr1=strchr(curpage,'\"');
		if(ptr1) *ptr1=0; // Kill ending quote
		}

	// Mark if there are L&R links
	if(strstr(buffer,"[left=\""))
		llink=1;
	if(strstr(buffer,"[right=\""))
		rlink=1;

	// If end of page and no links, there's a problem
	if(strstr(buffer,"[endpage]"))
		{
		if(!links)
			{
			printf("Page '%s' has no links at line %d\n",curpage,line);
			errors++;
			}

		if(llink && !rlink)
			printf("Warning: Page '%s' has only a left link at line %d\n",curpage,line);
		llink=0;rlink=0; // Reset L&R flags
		strcpy(curpage,"<unknown>");
		}

	// Check for links
	p = NULL;

	#define CHECK_LINKS(CHK) \
	if(!p)\
		{\
		p=strstr(buffer,CHK);\
		if(p)\
			{\
			links++;\
			}\
		}

	CHECK_LINKS("[goto=\"");
	CHECK_LINKS("[linkto=\"");
	CHECK_LINKS("[nextpage=\"");
	CHECK_LINKS("[append=\"");
	CHECK_LINKS("[left=\"");
	CHECK_LINKS("[right=\"");
//	CHECK_LINKS("[random_page=\""); // Don't check destination

	// Random Pages are a special case
	if(!p)
		{
		p=strstr(buffer,"random_page=\"");
		if(p)
			{
			links++;
			p=NULL; // prevent checking (hack)
			}
		}

	// Check quotes
	if(p)
		{
		// Find opening quote
		ptr1=strchr(p,'\"');
		ptr1++;
		// Find ending quote
		ptr2=strrchr(ptr1,'\"');
		if(!ptr2)
			{
			printf("No ending quote in link at line %d in page '%s'\n",line,curpage);
			errors++;
			continue;
			}
		*ptr2=0; // Kill the ending quote
		if(!find_page(ptr1))
			{
			printf("Page '%s' not found in link at line %d in page '%s'\n",ptr1,line,curpage);
			errors++;
			}
//		else
//			printf("%s: found link %s\n",p,ptr1);
		}
	} while(!feof(fp));

if(!find_page("start"))
	printf("Warning: No start page in this file\n");


if(errors)
	printf("%d errors found\n",errors);
//else
//	printf("No errors\n");


// Free everything and quit

for(ctr=0;ctr<pages;ctr++)
	free(pageindex[ctr]);
free(pageindex);
}


int main(int argc,char *argv[])
{
int ctr;

if(argc<2)
	{
	printf("NPC link checker 1.4\n");
	printf("Syntax: checknpc <filenames>\n");
	exit(1);
	}

for(ctr=1;ctr<argc;ctr++)
	process_file(argv[ctr]);
printf("Done\n");
return 0;
}
