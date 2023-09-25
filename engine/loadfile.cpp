//
//      File loader
//

#include <stdio.h>
#include "ithelib.h"
#include "loadfile.hpp"
#include "oscli.hpp"

// Defines

#define WAD_SIGNATURE 0x44415750

// Variables

char savegamedest[1024];

// Functions

#ifdef _WIN32
static void winslash(char *name);
#endif
static int blank_savegame(const char *srcdir, const char *name);
static int copy_savegame(const char *srcdir, const char *name);

// Code

	
char *loadfile(const char *srcname, char *destname)
{
// Normalise filename
strcpy(destname,srcname);
strlwr(destname);
strslash(destname);

// Check it
if(iexist(destname))
	return destname;
return NULL;
}


char *makefilename(const char *srcname, char *destname)
{
// Normalise filename
strcpy(destname,srcname);
strlwr(destname);
strslash(destname);
return destname;
}


int fileexists(char *name)
{
//char fn[1024];
if(!name)
	ithe_panic("fileexists: NULL filename given",NULL);

return(iexist(name));
}

/*
 *		Convert slashes to windows format
 */
#ifdef _WIN32
void winslash(char *name)
{
char *a;
do	{
	a=strchr(name,'/');
	if(a)
		*a='\\';
//      	*a='|';
	} while(a);
}
#endif
/*
 *      Convert a path from Absolute to Relative format
 */

void clean_path(char *fn)
{
char filename[1024];
char pathname[1024];

char *ptr;
int len;

strcpy(pathname,"/");
strcat(pathname,projectname);
strcat(pathname,"/");

strcpy(filename,"/");
strcat(filename,fn);

strslash(pathname);
strslash(filename);
strlwr(pathname);
strlwr(filename);

// OK, everything should be ready.

ptr = strstr(filename,pathname);
if(!ptr)
	return; // It's clean

// It's not clean.  Clean it.

ptr += strlen(pathname);
ptr--; // Remove initial '/'

len = (int)(ptr-filename);

strcpy(filename,&fn[len]);
strcpy(fn,filename);
}

/*
 *  .WAD manipulation functions.  May have a 40-byte signature in front of the
 *       data, so extra logic to work around this in loader.  New files should
 *       be regular PWADs
 */

void StartWad(IFILE *ofp)
{
iputl_i(WAD_SIGNATURE,ofp);// PWAD
iputl_i(0L,ofp);// Entries, patched later
iputl_i(0L,ofp);// Offset, ditto
}

/*
 *	Append the directory to the file to make a true .WAD
 */

void FinishWad(IFILE *ofp, WadEntry entries[])
{
unsigned int directory,num;
directory = itell(ofp);

// Write the entries

for(num=0;entries[num].name;num++)
	{
	iputl_i(entries[num].start,ofp); // (was start-40 for header)
	iputl_i(entries[num+1].start-entries[num].start,ofp);
	iwrite((unsigned char *)entries[num].name,8,ofp);
	}

// Back up the WAD and write the initial entries.

iseek(ofp,4,SEEK_SET);	// was 44 for signature
iputl_i(num,ofp);
iputl_i(directory,ofp); // was -40 to cancel the signature

// Finish

iclose(ofp);
}

int GetWadEntry(IFILE *ifp, char *name)
{
unsigned int sig,num,off,ctr;
char buf[]="nothing!";
int offset=0; // Assume normal PWAD for now

iseek(ifp,0,SEEK_SET);
sig = igetl_i(ifp);
if(sig != WAD_SIGNATURE)
	{
	// Is it an old one with the signature?
	iseek(ifp,40,SEEK_SET);
	sig = igetl_i(ifp);
	if(sig == WAD_SIGNATURE)
		offset=40;	// Yes, set offset to 40
	else
		return 0;
	}

num = igetl_i(ifp);
off = igetl_i(ifp);

iseek(ifp,off+offset,SEEK_SET); // Cancel out any signature
for(ctr=0;ctr<num;ctr++)
	{
	off = igetl_i(ifp);
	igetl_i(ifp); // skip length
	iread((unsigned char *)buf,8,ifp);
	if(!strcmp(buf,name))
		{
		iseek(ifp,off+offset,SEEK_SET); // Cancel out the signature
		return 1;
		}
	}

return 0;
}

// Work out savegame dir (and save it)

char *getsavegamedir(int savegame)
{
char temp[2048];
char home[1024];
char *out;
out=strLocalBuf(); // Get local string (with garbage collection)

//ilog_quiet("getsavegamedir on slot %d\n",savegame);

ihome(home);
#ifdef __SOMEUNIX__
	snprintf(temp,2047,"%s/saves/%s/slot%02d/",home,projectname,savegame);
#else
	sprintf(temp,"%s/iresaves\\%s\\slot%02d\\",home,projectname,savegame);
	#ifndef _WIN32
	strslash(temp);
	#endif
#endif

strcpy(out,temp);
return out;
}


// File mangling for mapfiles, work out the full path and filename

char *makemapname(int number, int savegame, const char *extension)
{
char *temp,*t2;

temp=strLocalBuf();

// Savegame 0 means the actual level data (master copy) not a savegame,
// (savegames are modified copies of the original).

if(!savegame) {
	const char *projdir=use_jugfiles?"":projectdir;
	sprintf(temp,"%s%s%04d%s",projdir,projectname,number,extension);
} else {
	t2=getsavegamedir(savegame);
	if(!t2)
		return NULL;
	sprintf(temp,"%s%s%04d%s",t2,projectname,number,extension);
	#ifdef _WIN32
	strwinslash(temp);
	#else
	strslash(temp);
	#endif
}

return temp;
}

// Make savegame dir (somewhat involved)

int makesavegamedir(int savegame, int erase)
{
char home[1024];
char *temp=strLocalBuf();
ihome(home);

if(savegame < 0)	{
	sprintf(temp,"%d",savegame);
	ithe_panic("read_sgheader called with bad savegame number",temp);
}

//ilog_quiet("makesavegamedir for slot %d\n",savegame);
	
#ifdef __SOMEUNIX__
	sprintf(temp,"%s/saves",home);
	MAKE_DIR(temp);
	sprintf(temp,"%s/saves/%s",home,projectname);
	MAKE_DIR(temp);
	sprintf(temp,"%s/saves/%s/slot%02d",home,projectname,savegame);
	MAKE_DIR(temp);

	// Empty it
	sprintf(temp,"%s/saves/%s/slot%02d/",home,projectname,savegame);

#else
	sprintf(temp,"%s\\iresaves",home);
	MAKE_DIR(temp);
	sprintf(temp,"%s\\iresaves\\%s",home,projectname);
	#ifndef _WIN32
	strslash(temp);
	#endif
	MAKE_DIR(temp);
	sprintf(temp,"%s\\iresaves\\%s\\slot%02d",home,projectname,savegame);
	#ifndef _WIN32
	strslash(temp);
	#endif
	MAKE_DIR(temp);

	sprintf(temp,"%s\\iresaves\\%s\\slot%02d\\",home,projectname,savegame);
#endif

if(!erase)
	return 1; // Fine
	
int listlen=0;
char **list = igetdir(temp,&listlen,0);
if(list)
	for(int ctr=0;ctr<listlen;ctr++)
		if(list[ctr])
			blank_savegame(temp,list[ctr]);
ifreedir(list,listlen);

return 1;
}


/*
 * helper function
 */
static int blank_savegame(const char *srcdir, const char *name)
{
char buf[512];
strcpy(buf,srcdir);
strcat(buf,name);
ilog_quiet("Erase: %s\n",buf);
remove(buf);
return 0;
}

void copysavegamedir(int src, int dest)
{
char *srcdir;
char *destdir;

destdir=getsavegamedir(dest);
strcpy(savegamedest,destdir);  // Copy destination to a global

srcdir=getsavegamedir(src);

ilog_quiet("Try to copy: %s %s\n",srcdir,savegamedest);

//for_each_file_ex(srcdir,0,FA_DIREC,copy_savegame,0);
int listlen=0;
char **list = igetdir(srcdir,&listlen,0);
if(list)
	for(int ctr=0;ctr<listlen;ctr++)
		if(list[ctr])
			copy_savegame(srcdir,list[ctr]);
		
ifreedir(list,listlen);
}


/*
 * helper function to copy a single file to a certain directory
 */

static int copy_savegame(const char *srcdir, const char *name)
{
FILE *fi,*fo;
char fname[512];
unsigned char buf[4096]; // 4k read buffer
int bytes,total;

strcpy(fname,srcdir);
strcat(fname,name);

fi=fopen(fname,"rb");
if(!fi)
	{
	ilog_quiet("COPY: sourcename: can't open '%s'\n",fname);
	return 0;
	}

strcpy(fname,savegamedest);
strcat(fname,name);

fo=fopen(fname,"wb");
if(!fo)
	{
	fclose(fi);
	ilog_quiet("COPY: destname: can't create '%s'\n",fname);
	return 0;
	}

total=0;
do
	{
	bytes=fread(buf,1,4096,fi);
	total+=bytes;
	fwrite(buf,1,bytes,fo);
	} while(bytes==4096);

fclose(fi);
fclose(fo);

ilog_quiet("COPY: '%s' to '%s' : %d bytes copied\n",name,fname,total);
return 0;
}

//
//  Utility functions to create a list of all files in a given directory
//


#ifdef _WIN32
void makepath(char *fname)
{
char buffer[1024];
char *p,*pp;

strcpy(buffer,fname);
winslash(buffer); // Normalise slashes
p=strrchr(buffer,'\\');
if(p)
	*p=0; // Chop off the filename

MAKE_DIR(buffer);

pp=&buffer[0];
do
	{
	p=strchr(pp,'\\');
	if(p)
		{
		*p=0; // Pop the slash out
		MAKE_DIR(buffer);
		*p='\\'; // Put it back
		pp=++p; // Look for the next one?
		}
	} while(p);
}
#else
void makepath(char *fname)
{
char buffer[1024];
char *p,*pp;

strcpy(buffer,fname);
strslash(buffer); // Normalise slashes
p=strrchr(buffer,'/');
if(p)
	*p=0; // Chop off the filename

MAKE_DIR(buffer);

pp=&buffer[0];
do
	{
	p=strchr(pp,'/');
	if(p)
		{
		*p=0; // Pop the slash out
		MAKE_DIR(buffer);
		*p='/'; // Put it back
		pp=++p; // Look for the next one?
		}
	} while(p);

}
#endif



//
//  Convert 16-bit words into 18-bit ASCII triplets.  inwords is number of words, not bytes
//

int ExportB64(const unsigned short *input, int inwords, char *out, int outlen)
{
int len,ctr;
char *outptr;
unsigned short i;

if(!input || inwords < 1 || !out)
	return 0;

len=(inwords*3)+1;
if(outlen < len)
	return 0;

outptr=out;
for(ctr=0;ctr<inwords;ctr++)
	{
	i=*input++;
	*outptr++=((i&0x3f)+'0');
	i>>=6;
	*outptr++=((i&0x3f)+'0');
	i>>=6;
	*outptr++=((i&0x3f)+'0');
	}
*outptr=0;

return 1;
}

//
//  Convert a set of 18-bit ASCII triplets back into 16-bit binary words, returns no of words output
//

int ImportB64(const char *input, unsigned short *out, int outlen)
{
int len,ctr;
unsigned short i;

if(!input || !out)
	return 0;

// Find out how many words we'll be outputting
len=strlen(input);
len /=3;

if(outlen < len)
	return 0;

for(ctr=0;ctr<len;ctr++)
	{
	i=((*input++)-'0')&0x3f;
	i|=((((*input++)-'0')&0x3f)<<6);
	i|=((((*input++)-'0')&0x3f)<<12);
	*out++=i;
	}

return len;
}


