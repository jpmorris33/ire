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


static void packDate(struct JUGFILE_ENTRY *fileentry, unsigned char datebuffer[6]) {
datebuffer[0] = fileentry->year & 0xff;
datebuffer[1] = (fileentry->year >> 8) & 0xff;
datebuffer[2] = fileentry->month;
datebuffer[3] = fileentry->day;
datebuffer[4] = fileentry->hour;
datebuffer[5] = fileentry->min;
}

static void unpackDate(struct JUGFILE_ENTRY *fileentry, unsigned char datebuffer[6]) {
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


FILE *JUG5startFileSave(const char *jugfile, struct JUGFILE_ENTRY *fileentry)
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


void JUG5close(struct JUGFILE_ENTRY *fileentry)
{
if(!fileentry) {
	return;
}

if(fileentry->_fp) {
	fclose(fileentry->_fp);
	fileentry->_fp = NULL;
}
}

void JUG5empty(struct JUGFILE_ENTRY *jug) {
memset(jug,0,sizeof(struct JUGFILE_ENTRY));
}



//
//  API to analyse or extract a JUGfile
//

bool JUG5readHeader(const char *jugfile, struct JUGFILE_ENTRY *fileentry) {
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

FILE *JUG5getFileInfo(struct JUGFILE_ENTRY *fileentry)
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

void JUG5skipFileData(struct JUGFILE_ENTRY *fileentry)
{
if(!fileentry) {
	return;
}
fseek(fileentry->_fp,fileentry->len,SEEK_CUR);
}

