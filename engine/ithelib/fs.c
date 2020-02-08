/*
 *   Generic filesystem routines and wrappers
 *
 *    The IFILE system also allows the possibility of loading from
 *    an uncompressed archive, but this has been disabled for now
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <string.h>
//#define __OBJECTID_DEFINED	// This is a lie to avoid conflicts with win32 sdk
#include <windows.h>
#include <direct.h>
#else
#include <strings.h>
#include <unistd.h>
#include <dirent.h>
#endif

#ifdef __APPLE__
#define _DIRENT_HAVE_D_TYPE	// It does, it just doesn't believe it
#endif

char *ifile_prefix=NULL; // No prefix by default

/*
static FILE *fpx;
#define LOGULE(x,y) {fpx=fopen("qslog","a");if(!fpx) return; fprintf(fpx,x,y); fclose(fpx);}
*/

#include "ithelib.h"

// Defines

// Variables

// Unused functions
/*
static int BSCMP(const void *key, const void *elem);
static int QSCMP(const void *elem1, const void *elem2);
*/
// Code

/*
 *  iopen: top level file open routine.
 *         Does it's own priority checking
 */


/*
 *  Read direct from file.
 */

IFILE *iopen(const char *filename)
{
FILE *fp;
IFILE *ifp;
char buffer[1024];
int32_t len=0;

fp=NULL;

// If we have a prefix, try that first

if(ifile_prefix) {
	strcpy(buffer,ifile_prefix);
	strcat(buffer,filename);

	#ifdef _WIN32
	strwinslash(buffer);
	#endif
	fp = fopen(buffer,"rb");
}

//  Nothing?  Try without any prefix
if(!fp) {
	fp = fopen(filename,"rb");
}

if(!fp) {
	fp=jug_openfile(filename,&len);
}

if(!fp) {
	ithe_panic("iopen_readfile: fopen failed on file",filename);
}
//	return NULL;

// OK, we got a result with a physical file, load it and go
ifp = (IFILE *)M_get(1,sizeof(IFILE));
if(len) {
	ifp->length = len;
} else {
	ifp->length = filelength(fileno(fp));
}

ifp->origin = ftell(fp);
ifp->fp = fp;

ifp->truename = M_get(1,strlen(filename)+1);
strcpy(ifp->truename,filename);

return ifp;
}


/*
 *  Write to a file
 */

IFILE *iopen_write(const char *filename)
{
FILE *fp;
IFILE *ifp;

// only physical files are supported for writing

fp = fopen(filename,"wb+");
if(!fp)
	ithe_panic("iopen_write: cannot create file:",filename);

// OK, we got a result with a physical file, start it and go
ifp = (IFILE *)M_get(1,sizeof(IFILE));
ifp->length = 0;
ifp->origin = 0;
ifp->fp = fp;

ifp->truename = M_get(1,strlen(filename)+1);
strcpy(ifp->truename,filename);

return ifp;
}


/*
 *  Does a file exist
 */

int iexist(const char *filename)
{
char buffer[1024];

if(!access(filename,F_OK)) {
	return 2;
}
strcpy(buffer,ifile_prefix);
strcat(buffer,filename);

if(!access(buffer,F_OK)) {
	return 1;
}

if(jug_fileexists(filename)) {
	return 2;
}

if(jug_fileexists(buffer)) {
	return 1;
}

return 0;
}

/*
 *  Get file date
 */

time_t igettime(const char *filename)
{
struct stat sb;
char buffer[1024];

if(!stat(filename,&sb))
	return sb.st_mtime;

strcpy(buffer,ifile_prefix);
strcat(buffer,filename);
if(!stat(buffer,&sb))
	return sb.st_mtime;

return 0;
}

/*
 *  Close a file
 */

void iclose(IFILE *ifp)
{
if(!ifp)
	return;
fclose(ifp->fp);
M_free(ifp->truename);
M_free(ifp);
}

void iseek(IFILE *ifp, int offset, int whence)
{
if(!ifp)
	return;

switch(whence)
	{
	case SEEK_SET:
	fseek(ifp->fp,offset+ifp->origin,SEEK_SET);
	break;

	case SEEK_END:
	fseek(ifp->fp,ifp->origin+ifp->length+offset,SEEK_SET);
	break;

	default:
	fseek(ifp->fp,offset,SEEK_CUR);
	break;
	}
}

unsigned char igetc(IFILE *ifp)
{
return fgetc(ifp->fp);
}

unsigned short igetsh_reverse(IFILE *ifp)
{
unsigned char a,b;
a=igetc(ifp);
b=igetc(ifp);
return ((a<<8) + b);
}

unsigned short igetsh_native(IFILE *ifp)
{
unsigned short s=0;
iread((void *)&s,2,ifp);
return s;
}

long igetl_reverse(IFILE *ifp)
{
unsigned short a=0,b=0;
a=igetsh_reverse(ifp);
b=igetsh_reverse(ifp);
return (long)((a<<16) + b);
}

long igetl_native(IFILE *ifp)
{
long a=0;
iread((void *)&a,4,ifp);
return a;
//return getw(ifp->fp);
}


unsigned long igetlu_reverse(IFILE *ifp)
{
unsigned short a,b;
a=igetsh_reverse(ifp);
b=igetsh_reverse(ifp);
return (unsigned long)((a<<16) + b);
}

unsigned long igetlu_native(IFILE *ifp)
{
return (unsigned long)igetl_native(ifp);
//return getw(ifp->fp);
}

uqword igetll_reverse(IFILE *ifp)
{
return (((uqword)igetl_reverse(ifp) << 32) + igetl_reverse(ifp));
}

uqword igetll_native(IFILE *ifp)
{
uqword q=0;
iread((void *)&q,8,ifp);
return q;
}

int iread(unsigned char *buf, int l, IFILE *ifp)
{
return fread(buf,1,l,ifp->fp);
}

void iputc(unsigned char c, IFILE *ifp)
{
fputc(c,ifp->fp);
}

void iputsh_reverse(unsigned short s, IFILE *ifp)
{
iputc((s>>8)&0xff,ifp);
iputc(s&0xff,ifp);
}

void iputsh_native(unsigned short s, IFILE *ifp)
{
iwrite((void *)&s,2,ifp);
}

void iputl_reverse(long l, IFILE *ifp)
{
iputsh_reverse((l>>16)&0xffff,ifp);
iputsh_reverse(l&0xffff,ifp);
}

void iputl_native(long l, IFILE *ifp)
{
putw(l,ifp->fp);
}

void iputlu_reverse(unsigned long l, IFILE *ifp)
{
iputsh_reverse((l>>16)&0xffff,ifp);
iputsh_reverse(l&0xffff,ifp);
}

void iputlu_native(unsigned long l, IFILE *ifp)
{
putw(l,ifp->fp);
}

void iputll_reverse(uqword q, IFILE *ifp)
{
iputl_reverse((q>>32)&0xffffffff,ifp);
iputl_reverse(q&0xffffffff,ifp);
}

void iputll_native(uqword q, IFILE *ifp)
{
iwrite((void *)&q,8,ifp);
}

int iwrite(unsigned char *buf, int l, IFILE *ifp)
{
return fwrite(buf,1,l,ifp->fp);
}

unsigned int itell(IFILE *ifp)
{
return ftell(ifp->fp)-ifp->origin;
}

unsigned int ifilelength(IFILE *ifp)
{
return ifp->length;
}

int ieof(IFILE *ifp)
{
if(ftell(ifp->fp) >= ifp->origin+ifp->length)
	return 1;
return 0;
}

// Get a home subdirectory for config settings

void ihome(char *path)
{
char *home;
char temp[1024];
#ifdef _WIN32
	char hometemp[1024];
	char *home1;
	char *home2;
#endif

home=getenv("HOME");
if(!home)	{
	home=".";

	#ifdef _WIN32
		home1=getenv("HOMEDRIVE");
		home2=getenv("HOMEPATH");

		if(home1 && home2)	{
			strcpy(hometemp,home1);
			strcat(hometemp,home2);
			home=hometemp;
		}
	#endif
	}

#if defined(__SOMEUNIX__) || defined(__DJGPP__)
	sprintf(temp,"%s/.ire",home);
	mkdir(temp,S_IRUSR|S_IWUSR|S_IXUSR);
	strcat(temp,"/");
#else
	sprintf(temp,"%s/ire",home);
	mkdir(temp);
	strcat(temp,"/");
	#ifdef _WIN32
		strwinslash(temp);
	#endif
#endif

strcpy(path,temp);
}


unsigned char *iload_file(char *filename, int *outlen)
{
unsigned char *buf;                        // conversion buffer
IFILE *fp;
int len;

if(!outlen)
	return NULL;

fp = iopen(filename);
len=ifilelength(fp);

buf=(unsigned char *)M_get(1,len);              // Allocate room
iread(buf,len,fp);
iclose(fp);

*outlen=len;
return buf;
}


//
//	Make a nice list of files and optionally directories as well (tagged with a '*')
//

#ifdef _WIN32

static int IsItADirectory(WIN32_FIND_DATA *d)
{
if(d->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	return 1;
return 0;
}

char **igetdir(const char *dir, int *items, int flags)
{
const char *dotdot = "[..]";
int count=0;
char **out;
char spec[1024];
char *filename;
WIN32_FIND_DATA d;
HANDLE e;

if(!dir || !items)
	return NULL;

strcpy(spec,dir);
filename=&spec[strlen(spec)-1];
if(spec == '/')
	strcat(spec,"*.*");
else
	strcat(spec,"/*.*");

//printf("scan for %s\n",spec);

e=FindFirstFile(spec,&d);

if(e == INVALID_HANDLE_VALUE)	{
	if(GetLastError() != ERROR_FILE_NOT_FOUND)
		return NULL;

	// No files present
	if((flags & IGETDIR_SHOWDIRS) && (flags & IGETDIR_DOTS))	{
		// Make a list containing just [..]
		out=(char **)M_get(1,sizeof(char *)*1);
		out[0]=M_get(1,strlen(dotdot)+1);
		strcat(out[0],dotdot);
		*items=1;
		return out;
		}

	// Okay, nothing there
	return NULL;
	}

do	{
	if(IsItADirectory(&d))	{
		if(flags & IGETDIR_SHOWDIRS)
			count++;
		}
	else
		count++;
	} while(FindNextFile(e,&d));
FindClose(e);

if(!count) {
	return NULL;
}

out=(char **)M_get(1,sizeof(char *)*count);
if(!out)	{
	// Actually, M_get will kill the program if it fails...
	return NULL;
	}

count=0;

// First pass, look for directories
if(flags & IGETDIR_SHOWDIRS)	{
	e=FindFirstFile(spec,&d);
	for(;;)	{
		if(e == INVALID_HANDLE_VALUE)
			break;
	
		if(!IsItADirectory(&d))	{
			// We don't like your kind around here
			if(!FindNextFile(e,&d))
				break;
			continue;
			}
			
		// Okay, add it to the list
		filename=d.cFileName;
//	printf("founddir '%s'\n",filename);

		out[count] = (char *)M_get(1,strlen(filename)+3);	// length + '[',']' and NULL
		if(!out[count])
			break;
		strcpy(out[count],"["); // Mark that it's a directory
		strcat(out[count],filename);
		strcat(out[count],"]");
		count++;
		if(!FindNextFile(e,&d))
			break;
		}
	FindClose(e);	
	}

// Second pass, now look for files
e=FindFirstFile(spec,&d);
for(;;)	{
	if(e == INVALID_HANDLE_VALUE)
		break;

	if(IsItADirectory(&d))	{
		// We don't like your kind around here
		if(!FindNextFile(e,&d))
			break;
		continue;
		}
	
	// Okay, add it to the list
	filename=d.cFileName;

//	printf("found '%s'\n",filename);

	out[count] = (char *)M_get(1,strlen(filename)+1);	// length and NULL
	if(!out[count])
		break;
	strcpy(out[count],filename);
	count++;
	if(!FindNextFile(e,&d))
		break;
	}

//printf("found %d items\n",count);

FindClose(e);
*items=count;
return out;
}

#else

static int IsItADirectory(struct dirent *e)
{
#ifdef _DIRENT_HAVE_D_TYPE
	if(e->d_type == DT_DIR)
		 return 1;
#else
	// Do it the hard way, by statting it
	SAFE_STRCPY(path,dir);
	strcat(path,"/");
	strcat(path,e->d_name);
	stat(path,&sb);
	// Still need to finish this
	#error TBD
#endif

return 0;
}

char **igetdir(const char *dir, int *items, int flags)
{
const char *dotdot = "[..]";
int count=0;
char **out;
DIR *d;
struct dirent *e;
struct stat sb;
char path[1024];

if(!dir || !items)
	return NULL;

d=opendir(dir);

if(!d)
	return NULL;

do	{
	e=readdir(d);
	if(!e)
		break;
	
	if(IsItADirectory(e))	{
		if(flags & IGETDIR_SHOWDIRS)
			count++;
		}
	else
		count++;
	} while(e);

if(count == 0)	{
	closedir(d);

	if((flags & IGETDIR_SHOWDIRS) && (flags & IGETDIR_DOTS))	{
		// Make a list containing just [..]
		out=(char **)M_get(1,sizeof(char *)*1);
		out[0]=M_get(1,strlen(dotdot)+1);
		strcat(out[0],dotdot);
		*items=1;
		return out;
		}

	return NULL;
	}

out=(char **)M_get(1,sizeof(char *)*count);
if(!out)	{
	closedir(d);
	return NULL;
	}

count=0;

// First pass, look for directories
if(flags & IGETDIR_SHOWDIRS)	{
	rewinddir(d);
	for(;;)	{
		e=readdir(d);
		if(!e)
			break;

		if(!IsItADirectory(e))
			 continue;  // We don't like your kind around here
			
		// Okay, add it to the list
		out[count] = (char *)M_get(1,strlen(e->d_name)+3);	// length + '[',']' and NULL
		if(!out[count])
			break;
		strcpy(out[count],"["); // Mark that it's a directory
		strcat(out[count],e->d_name);
		strcat(out[count],"]");
		count++;
		}
		
	}

// Second pass, now look for files
rewinddir(d);
for(;;)	{
	e=readdir(d);
	if(!e)
		break;

	if(IsItADirectory(e))
		 continue;  // We don't like your kind around here
		
	// Okay, add it to the list
	out[count] = (char *)M_get(1,strlen(e->d_name)+1);	// length and NULL
	if(!out[count])
		break;
	strcpy(out[count],e->d_name);
	count++;
	}

closedir(d);
*items=count;
return out;
}

#endif


void ifreedir(char **list, int items)
{
int ctr;
if(!list)
	return;

for(ctr=0;ctr<items;ctr++)
	if(list[ctr])
		 M_free(list[ctr]);
M_free(list);
}




/*
 *
 *
 *  Internal functions (not for human consumption)
 *
 *
 */

#ifdef __BEOS__		// BEOS is not well
int getw(FILE *fp)
{
int w;
fread(&w,1,4,fp);
return w;
}

void putw(int w, FILE *fp)
{
fwrite(&w,1,4,fp);
}
#endif
