#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 

#include "../ithelib/ithelib.h"

#define BLOCKLEN 32768
typedef int32_t INT32;

void F_error(const char *e,const char *e2);
bool JUG5create(const char *jugfile);
FILE *JUG5save(const char *jugfile,const char *entryname, INT32 len);
INT32 getfilelength(FILE *fp);
bool traverse(const char *directory, bool append);
void copyfile(FILE *fpi, FILE *fpo, INT32 len);

unsigned char databuf[BLOCKLEN];
char *jugname=NULL;
char basedir[1024];

int main(int argc,char *argv[])
{
puts("IT-HE JUG compiler 2.0 (C)1995-2020 IT-HE Software");
puts("\nTakes a directory and produces a JUG file from it.");
puts("\n");
if(argc<2) {
	puts("Syntax: makejug <directory> [JUG file]");
	puts("If no JUG file is specified, OUTPUT.JUG is created.\n");
	return 1;
}

if(argc>2) {
	jugname=argv[2];
} else {
	jugname="output.jug";
}

strcpy(basedir,argv[1]);


if(!JUG5create(jugname)) {
	F_error("Cannot open new JUGfile:",jugname);
}

if(!traverse(basedir, false)) {
	F_error("Could not open directory:",basedir);
}

puts("Done.");
}

bool JUG5create(const char *jugfile) {
unsigned short sig = JUG_BASE|JUG_VERSION;
FILE *fp;

fp=fopen(jugname,"wb");
if(!fp) {
	return false;
}
fwrite(&sig,1,2,fp);
fclose(fp);
return true;
}


FILE *JUG5save(const char *jugfile,const char *entryname,INT32 len)
{
FILE *fp;

fp=fopen(jugfile,"ab");
if(!fp) {
	return NULL;
}

fseek(fp,0L,SEEK_END);
fwrite(entryname,1,252,fp);
fwrite(&len,1,4,fp);
return fp;
}



INT32 getfilelength(FILE *fp) {
long oldpos = ftell(fp);
long len =0;
fseek(fp,0L,SEEK_END);
len=ftell(fp);
fseek(fp,oldpos,SEEK_SET);
return (INT32)len;
}

void copyfile(FILE *fpi, FILE *fpo, INT32 len) {
INT32 readbytes;
INT32 count=0;
do {
	readbytes=fread(databuf,1,BLOCKLEN,fpi);
	fwrite(databuf,1,readbytes,fpo);
	count += readbytes;
} while(readbytes == BLOCKLEN);
}


bool traverse(const char *directory, bool append) {
char path[1024];
char path2[2048];
char path3[3072];
char **filenames;
int items,ctr;
FILE *fpi,*fpo;
INT32 filelen;

strcpy(path,"");
strcpy(path2,"");
if(append) {
	strcat(path,directory);
	strcat(path,"/");
	strcat(path2,basedir);
	strcat(path2,"/");
}

strcat(path2,directory);

filenames = igetdir(path2, &items, IGETDIR_SHOWDIRS);
if(!filenames) {
	printf("No files in '%s'\n",path2);
	return false;
}

printf("Open path '%s', got %d items\n",path,items);

for(ctr=0;ctr<items;ctr++) {
	// Ignore the internal directory entries
	if(!strcmp(filenames[ctr],"[.]")) {
		continue;
	}
	if(!strcmp(filenames[ctr],"[..]")) {
		continue;
	}

	strcpy(path2,path);
	if(filenames[ctr][0]=='[') {
		strcat(path2,&filenames[ctr][1]);
		strdeck(path2,']');
		strstrip(path2);

		if(strlen(path2) >= 252) {
			F_error("Path exceeds 252 characters:",path2);
		}

		printf("open directory '%s'\n",path2);

		traverse(path2,true);
	} else {

		printf("%s%s\n",path,filenames[ctr]);

		strcat(path2, filenames[ctr]);
		if(strlen(path2) >= 252) {
			F_error("Path exceeds 252 characters:",path2);
		}

		strcpy(path3,basedir);
		strcat(path3,"/");
		strcat(path3,path2);
		fpi=fopen(path3,"rb");
		if(!fpi) {
			F_error("Error opening input file:",path3);
		}
		filelen = getfilelength(fpi);
		fpo=JUG5save(jugname,path2,filelen);
		if(!fpo) {
			F_error("Error opening output file:",jugname);
		}
		copyfile(fpi,fpo,filelen);
		fclose(fpo);
		fclose(fpi);
	}
}


ifreedir(filenames,items);
return true;
}



void ithe_userexitfunction()
{
}

void F_error(const char *e,const char *e2)
{
printf("%s %s\n",e,e2);
exit(1);
}
