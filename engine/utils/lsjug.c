#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ithelib/ithelib.h"

typedef int32_t INT32;


void F_error(char *e,char *e2);

int main(int argc,char *argv[])
{
struct JUGFILE_ENTRY jug;
char *jugname;
FILE *fp;
long total = 0;

puts("IT-HE JUG listing util 2.0 (C)1995-2020 IT-HE Software\n");
puts("\nShows the contents of a V5 JUG file");
if(argc<2) {
	puts("Syntax: lsjug <jugfile>");
	return 1;
}

jugname=argv[1];

JUG5empty(&jug);

if(!JUG5readHeader(jugname,&jug)) {
	F_error("Cannot open JUGfile, or wrong version:",jugname);
}

do {
	fp=JUG5getFileInfo(&jug);
	if(fp) {
		printf("%s   [%d bytes]\n",jug.filename, jug.len);
		JUG5skipFileData(&jug);
		total += jug.len;
	}

} while(fp);

JUG5close(&jug);
puts("Done.");
printf("%ld bytes total\n",total);
}

void F_error(char *e,char *e2)
{
printf("%s %s\n",e,e2);
exit(1);
}

void ithe_userexitfunction()
{
}
