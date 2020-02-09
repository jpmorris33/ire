/*
 *   JUGfile v5 packed data support
 *
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


#include "ithelib.h"


static void packDate(JUGFILE_ENTRY *fileentry, unsigned char datebuffer[6]) {
datebuffer[0] = fileentry->year & 0xff;
datebuffer[1] = (fileentry->year >> 8) & 0xff;
datebuffer[2] = fileentry->month;
datebuffer[3] = fileentry->day;
datebuffer[4] = fileentry->hour;
datebuffer[5] = fileentry->min;
}

static void unpackDate(JUGFILE_ENTRY *fileentry, unsigned char datebuffer[6]) {
fileentry->year = ((datebuffer[1]<<8) | datebuffer[0]);
fileentry->month = datebuffer[2];
fileentry->day = datebuffer[3];
fileentry->hour = datebuffer[4];
fileentry->min = datebuffer[5];
}


//
//  API to create a new jugfile
//


bool JUG5writeHeader(const char *jugfile) {
unsigned short sig = JUG_BASE|JUG_VERSION;
FILE *fp;

fp=fopen(jugfile,"wb");
if(!fp) {
	return false;
}
fwrite(&sig,1,2,fp);
fclose(fp);
return true;
}


FILE *JUG5startFileSave(const char *jugfile, JUGFILE_ENTRY *fileentry)
{
FILE *fp;
unsigned char datebuffer[6];

if(!fileentry) {
	return NULL;
}

fp=fopen(jugfile,"ab");
if(!fp) {
	return NULL;
}

fseek(fp,0L,SEEK_END);
fileentry->_headerStart = ftell(fp);


fwrite(fileentry->filename,1,JUG_MAXPATH,fp);
packDate(fileentry,datebuffer);
fwrite(datebuffer,1,6,fp);
fwrite(fileentry->spare,1,sizeof(fileentry->spare),fp);
fwrite(&fileentry->len,1,4,fp);

fileentry->_fp = fp;
fileentry->_dataStart = ftell(fp);

return fp;
}


void JUG5close(JUGFILE_ENTRY *fileentry)
{
if(!fileentry) {
	return;
}

if(fileentry->_fp) {
	fclose(fileentry->_fp);
	fileentry->_fp = NULL;
}
}

void JUG5empty(JUGFILE_ENTRY *jug) {
memset(jug,0,sizeof(JUGFILE_ENTRY));
}



//
//  API to analyse or extract a JUGfile
//

bool JUG5readHeader(const char *jugfile, JUGFILE_ENTRY *fileentry) {
unsigned short sig = JUG_BASE|JUG_VERSION;
unsigned short insig;
FILE *fp;

if(!fileentry) {
	return false;
}
fp=fopen(jugfile,"rb");
if(!fp) {
	return false;
}
fread(&insig,1,2,fp);
if(insig!=sig) {
	fclose(fp);
	return false;
}
fileentry->_fp=fp;
return true;
}

FILE *JUG5getFileInfo(JUGFILE_ENTRY *fileentry)
{
unsigned char datebuffer[6];
int readBytes=0;

if(!fileentry) {
	return NULL;
}
if(feof(fileentry->_fp)) {
	return NULL;
}

fileentry->_headerStart = ftell(fileentry->_fp);

readBytes=fread(fileentry->filename,1,JUG_MAXPATH,fileentry->_fp);
if(feof(fileentry->_fp)) {
	return NULL;
}
readBytes=fread(datebuffer,1,6,fileentry->_fp);
unpackDate(fileentry,datebuffer);
readBytes=fread(fileentry->spare,1,sizeof(fileentry->spare),fileentry->_fp);
readBytes=fread(&fileentry->len,1,4,fileentry->_fp);

fileentry->_dataStart = ftell(fileentry->_fp);

return fileentry->_fp;
}

void JUG5skipFileData(JUGFILE_ENTRY *fileentry)
{
if(!fileentry) {
	return;
}
fseek(fileentry->_fp,fileentry->len,SEEK_CUR);
}



//
//  JUG mounting/unmounting support
//

typedef struct {
const char jugfile[256];
char filename[JUG_MAXPATH+1];
long dataStart;
long length;
} JUGFILELIST;

static JUGFILELIST *jug_list;
static int jug_filecount;

// Look for the file in the list and return it to overwrite, or else the next free slot to put it in

static int findfile(const char *filename) {
int ctr=0;

for(ctr=0;ctr<jug_filecount;ctr++) {
	if(!jug_list[ctr].jugfile[0]) {
//		printf("%s: slot blank at %d\n",filename,ctr);
		return ctr;
	}
	if(!strcmp(jug_list[ctr].filename,filename)) {
//		printf("%s:found file at %d\n",filename,ctr);
		return ctr;
	}
}

//printf("%s:didn't find, returning -1\n",filename);

return -1;
}


static void addfile(const char *jugfile, JUGFILE_ENTRY *jug) {
JUGFILELIST *slot;
int slotno=findfile(jug->filename);
if(slotno < 0) {
	ithe_panic("Out of slots in jugfile loader, this should not be possible",jugfile);
}
slot = &jug_list[slotno];
strcpy(slot->jugfile, jugfile);
strcpy(slot->filename,jug->filename);
slot->dataStart=jug->_dataStart;
slot->length=jug->len;
}

//
//  Register the jugfiles with the loader and build a list of all files
//

bool jug_mountfiles(char **jugfiles, int listlen) {
JUGFILE_ENTRY jug;
FILE *fp;
int ctr,files=0;

if(jug_list) {
	ithe_panic("API misuse: cannot call jug_mountfiles twice!",NULL);
}

// Count the maximum number of slots 
for(ctr=0;ctr<listlen;ctr++) {
	JUG5empty(&jug);
	if(!JUG5readHeader(jugfiles[ctr],&jug)) {
		ithe_panic("Error opening JUGfile",jugfiles[ctr]);
	}

	do {
		fp=JUG5getFileInfo(&jug);
		if(fp) {
			JUG5skipFileData(&jug);
			files++;
		}
	} while(fp);
	JUG5close(&jug);
}

#ifdef ITHE_DEBUG
printf("jug_mountfiles: read %d files from %d JUGfiles\n",files,listlen);
#endif

jug_list = M_get(files,sizeof(JUGFILELIST));
jug_filecount = files;

// Now insert the files into the array, with later files taking priority for duplicates
for(ctr=0;ctr<listlen;ctr++) {
	JUG5empty(&jug);
	if(!JUG5readHeader(jugfiles[ctr],&jug)) {
		ithe_panic("Error opening JUGfile",jugfiles[ctr]);
	}

	do {
		fp=JUG5getFileInfo(&jug);
		if(fp) {
			addfile(jugfiles[ctr],&jug);
			JUG5skipFileData(&jug);
		}
	} while(fp);
	JUG5close(&jug);
}

// Now we need to do something clever like sorting them for faster access
return true;
}

//
//  Load in a file from the appropriate jug
//

FILE *jug_openfile(const char *filename, int32_t *length) {
FILE *fp;
int ctr;
if(!filename || !length) {
	return NULL;
}

// If none have been initialised, filecount will be 0 and it should just bail immediately

for(ctr=0;ctr<jug_filecount;ctr++) {
	if(!jug_list[ctr].jugfile[0]) {
		continue; // Blank
	}
	if(!strcmp(jug_list[ctr].filename,filename)) {
		fp=fopen(jug_list[ctr].jugfile,"rb");
		if(!fp) {
			printf("jug: couldn't open JUGFILE!\n",jug_list[ctr].jugfile);
			return NULL;
		}
		*length = jug_list[ctr].length;
		fseek(fp,jug_list[ctr].dataStart,SEEK_SET);
//		printf("jug: opened %s\n",filename);
		return fp;
	}
}
//printf("jug: couldn't open '%s'\n",filename);
return NULL;
}

//
//  Look for a file in the jug list
//

bool jug_fileexists(const char *filename) {
int ctr;
if(!filename) {
	return false;
}

// If none have been initialised, filecount will be 0 and it should just bail immediately

for(ctr=0;ctr<jug_filecount;ctr++) {
	if(!jug_list[ctr].jugfile[0]) {
		continue; // Blank
	}
	if(!strcmp(jug_list[ctr].filename,filename)) {
		return true;
	}
}
//printf("jug: couldn't find '%s'\n",filename);
return false;
}


//
//  Currently just a wrapper around fclose
//

void jug_closefile(FILE *fp) {
if(fp) {
	fclose(fp);
}
}
