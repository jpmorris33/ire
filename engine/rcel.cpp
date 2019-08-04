#include <stdio.h>
#include <stdlib.h>
#include <time.h>

unsigned char celbuf[320][200];

#define putsh(a,b) {putc(a&255,b);putc((a>>8)&255,b);}

main(int argc,char *argv[])
{
FILE *fp;
unsigned int ID;
unsigned int w,h;
unsigned long sizey,cx,cy;

char *filename=argv[1];

//srandom(rawclock());
srand(time(0));                 // Initialise rng

for(cy=0;cy<200;cy++)
    for(cx=0;cx<320;cx++)
	    celbuf[cx][cy]=rand()%250;


fp=fopen(filename,"wb");
if(!fp)
        {
        puts("Could not open file");
        exit(1);
        }
putsh(0x9119,fp);
putsh(320,fp);
putsh(200,fp);
putsh(0,fp);	// x coord
putsh(0,fp);	// y coord
putc(8,fp);	// 8 bits/pixel
putc(0,fp);	// 0 compression
sizey=w*h;
fwrite(&sizey,4,1,fp);
fseek(fp,32L,SEEK_SET);
fwrite(celbuf,768,1,fp);
fwrite(celbuf,64000,1,fp);
fclose(fp);
return 1;
}



