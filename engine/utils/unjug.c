#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 
#include <errno.h> 

#include <sys/stat.h> 

#include "../ithelib/ithelib.h"

#define BLOCKLEN 32768

void F_error(const char *e,const char *e2);
void copyfile(FILE *fpi, FILE *fpo, int32_t len);
bool doesDirExist(const char *path);
bool makePath(const char *path);

unsigned char databuf[BLOCKLEN];
char *jugname;
char *outputdir;

int main(int argc,char *argv[])
{
JUGFILE_ENTRY jug;
char outpath[1024];
char *ptr;
FILE *fpi,*fpo;
long total=0;

puts("IT-HE JUG decompiler 2.0 (C)1995-2020 IT-HE Software\n");
puts("\nUnpacks a V5 JUG file");
if(argc<3) {
	puts("Syntax: makejug <jugfile> <output directory>");
	return 1;
}

jugname=argv[1];
outputdir=argv[2];

if(doesDirExist(outputdir)) {
	F_error("Output directory already exists:",outputdir);
}


JUG5empty(&jug);

if(!JUG5readHeader(jugname,&jug)) {
	F_error("Cannot open JUGfile, or wrong version:",jugname);
}

do {
	fpi=JUG5getFileInfo(&jug);
	if(fpi) {
		strcpy(outpath,outputdir);
		strcat(outpath,"/");
		strcat(outpath,jug.filename);
		ptr=strrchr(outpath,'/');
		if(ptr) {
			*ptr=0;
			makePath(outpath);
			*ptr='/';
		}

		fpo=fopen(outpath,"wb");
		if(!fpo) {
			F_error("Failed to create output file:",outpath);
		}

		copyfile(fpi,fpo,jug.len);
		fclose(fpo);
		printf("wrote %s   [%d bytes]\n",outpath, jug.len);
		total += jug.len;
	}

} while(fpi);

JUG5close(&jug);
puts("Done.");
printf("%ld bytes total\n",total);
}

void F_error(const char *e,const char *e2)
{
printf("%s %s\n",e,e2);
exit(1);
}

void ithe_userexitfunction()
{
}

void copyfile(FILE *fpi, FILE *fpo, int32_t len) {
int32_t readbytes;
int32_t blocklen=BLOCKLEN;
int32_t left=len;
do {
	if(blocklen > left) {
		blocklen=left;
	}
	readbytes=fread(databuf,1,blocklen,fpi);
	fwrite(databuf,1,readbytes,fpo);
	left -= readbytes;
} while(left>0);
}


//
// makePath stuff based on:  https://stackoverflow.com/a/29828907
//

bool doesDirExist(const char *path)
{
#if defined(_WIN32)
	struct _stat info;
	if (_stat(path, &info) != 0) {
		return false;
	}
	return (info.st_mode & _S_IFDIR) != 0;
#else 
	struct stat info;
	if (stat(path, &info) != 0) {
		return false;
	}
	return (info.st_mode & S_IFDIR) != 0;
#endif
}


// This may need some more work to support windows

bool makePath(const char *path)
{
char * pos;
char pathcopy[1024];
#if defined(_WIN32)
	int ret = _mkdir(path);
#else
	mode_t mode = 0755;
	int ret = mkdir(path, mode);
#endif

if (ret == 0) {
	return true;
}

strcpy(pathcopy,path);

switch (errno) {
	case ENOENT:
		// parent didn't exist, try to create it
		pos = strrchr(pathcopy,'/');
		if(!pos || pos[1] == 0) {
			return false;
		}
		*pos=0; // erase the trailing slash
		if (!makePath(pathcopy)) {
			return false;
        	}
		// now, try to create again
#if defined(_WIN32)
		return 0 == _mkdir(path);
#else 
		return 0 == mkdir(path, mode);
#endif
	case EEXIST:
		// done!
		return doesDirExist(path);

	default:
	return false;
}
}
