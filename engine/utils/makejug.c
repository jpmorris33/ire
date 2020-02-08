#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 

#include <sys/stat.h> 

#include "../ithelib/ithelib.h"

#define BLOCKLEN 32768

void F_error(const char *e,const char *e2);
bool traverse(const char *directory, bool append);
void copyfile(FILE *fpi, FILE *fpo, int32_t len);
bool statFile(FILE *fp, const char *filename, JUGFILE_ENTRY *jug);

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


if(!JUG5writeHeader(jugname)) {
	F_error("Cannot open new JUGfile:",jugname);
}

if(!traverse(basedir, false)) {
	F_error("Could not open directory:",basedir);
}

puts("Done.");
}

void copyfile(FILE *fpi, FILE *fpo, int32_t len) {
int32_t readbytes;
int32_t count=0;
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
JUGFILE_ENTRY jug;

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

		if(strlen(path2) >= JUG_MAXPATH) {
			F_error("Path exceeds 224 characters:",path2);
		}

		printf("open directory '%s'\n",path2);

		traverse(path2,true);
	} else {

		printf("%s%s\n",path,filenames[ctr]);

		strcat(path2, filenames[ctr]);
		if(strlen(path2) >= JUG_MAXPATH) {
			F_error("Path exceeds 224 characters:",path2);
		}

		strcpy(path3,basedir);
		strcat(path3,"/");
		strcat(path3,path2);
		fpi=fopen(path3,"rb");
		if(!fpi) {
			F_error("Error opening input file:",path3);
		}
		JUG5empty(&jug);
		statFile(fpi,path2,&jug);
		fpo=JUG5startFileSave(jugname,&jug);
		if(!fpo) {
			F_error("Error opening output file:",jugname);
		}
		copyfile(fpi,fpo,jug.len);
		JUG5close(&jug);
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


bool  statFile(FILE *fp, const char *filename, JUGFILE_ENTRY *jug) {
struct stat sb;
struct tm *t;
if(fstat(fileno(fp), &sb)) {
	return false;
}

jug->len = sb.st_size;

t = localtime(&sb.st_mtime);

jug->year = t->tm_year + 1900;
jug->month = t->tm_mon + 1;
jug->day = t->tm_mday;
jug->hour = t->tm_hour;
jug->min = t->tm_min;

memset(&jug->filename,0,sizeof(jug->filename));
strncpy(jug->filename,filename,JUG_MAXPATH);

}