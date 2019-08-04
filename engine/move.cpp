//
//      IRE Map Section Mover
//

#include <stdio.h>
#include <string.h>

#include "core.hpp"
#include "ithelib.h"
#include "cookies.h"

extern "C" void read_WORLD(WORLD *curmap, IFILE *fp);
extern "C" void write_WORLD(WORLD *curmap, IFILE *fp);
extern "C" void read_OBJECT(OBJECT *o, IFILE *fp);
extern "C" void write_OBJECT(OBJECT *o, IFILE *fp);
int GetWadEntry(IFILE *fp,char *name);

char filename1[256];
char filename2[256];
char filename3[256];
char filename4[256];
char sigtemp[40];
char *cfa;

int main(int argc, char *argv[])
{
IFILE *fp,*fp2;
char *ptr;
WORLD curmap,newmap;
int mem,map_H,map_W,temp_int,key,cx,cy,xoff,yoff,total,size;
int maxx,minx,maxy,miny;
//unsigned char vbuf[4096]; // God Forbid
OBJECT o;

printf("IRE Map Section Mover 1.0\n");
printf("=========================\n\n");

if(argc<2)
    {
    printf("mover <mapfile>\n");
    exit(1);
    }

strcpy(filename1,argv[1]);
ptr = strrchr(filename1,'.');
if(ptr)
       {
       if(!istricmp(ptr+1,"map"))
           *ptr=0;
       printf("Filename base is '%s'\n\n",filename1);
       }

printf("Enter the X coord for the top-left corner:\n");
fflush(stdin);
scanf("%d",&minx);
printf("Enter the Y coord for the top-left corner:\n");
fflush(stdin);
scanf("%d",&miny);
printf("Enter the X coord for the bottom-right corner:\n");
fflush(stdin);
scanf("%d",&maxx);
printf("Enter the Y coord for the bottom-right corner:\n");
fflush(stdin);
scanf("%d",&maxy);

printf("\n\nMoving all objects in region %d,%d-%d,%d\n\n",minx,miny,maxx,maxy);

printf("Enter the X offset to apply to all objects (- is left, + is right):\n");
fflush(stdin);
scanf("%d",&xoff);
printf("Enter the Y offset to apply to all objects (- is up, + is down):\n");
scanf("%d",&yoff);

printf("Enter the file to save to (without extension):\n");
scanf("%s",filename3);

strcpy(filename2,filename1);
strcat(filename2,".map");

fp = iopen(filename2);
if(!fp)
    {
    printf("Could not load file '%s'\n",filename2);
    exit(1);
    }

iread((unsigned char *)sigtemp,40,fp);
if(strcmp(sigtemp,COOKIE_MAP))
    {
    printf("Unknown file format in map file '%s'\n",filename2);
    exit(1);
    }

read_WORLD(&curmap,fp);
if((unsigned)curmap.sig != MAPSIG)
    {
    printf("Current map version is 0x%x, but this is 0x%x\n",MAPSIG,temp_int);
    exit(1);
    }

curmap.physmap = (unsigned short *)M_get(curmap.w * curmap.h,sizeof(short));
iread((unsigned char *)curmap.physmap,curmap.w*curmap.h*sizeof(short),fp);
map_W = curmap.w;
map_H = curmap.h;
iclose(fp);


printf("Copying Objects..\n");
strcpy(filename2,filename1);
strcat(filename2,".mz1");
strcpy(filename4,filename3);
strcat(filename4,".mz1");

fp=iopen(filename2);
if(!fp)
       {
       printf("Can't open file '%s'\n",filename2);
       exit(1);
       }
fp2=iopen_write(filename4);
if(!fp2)
       {
       printf("Can't write to file '%s'\n",filename4);
       exit(1);
       }

for(;!ieof(fp);iputc(igetc(fp),fp2));

printf("Patching copy..\n");
cx=GetWadEntry(fp2,"OBJECTS ");
if(!cx)
      printf("Bummer.\n");
else
      {
      total=igetl_i(fp2);
      size=igetl_i(fp2);
      for(cx=0;cx<total;cx++)
              {
              // Get current position
              temp_int = itell(fp2);
              // Read it
              read_OBJECT(&o,fp2);
              iread((unsigned char *)sigtemp,32,fp2);

              if(o.x <= maxx && o.x >= minx)
                 if(o.y <= maxy && o.y >= miny)
                    {
                    // Patch it
                    o.x = o.x + xoff;
                    o.y = o.y + yoff;
                    if(o.x < 0)
                           o.x = 65535;
                    if(o.y < 0)
                           o.y = 65535;
                    }
              // Back up

              iseek(fp2,temp_int,SEEK_SET);
              // Rewrite
              write_OBJECT(&o,fp2);
              iread((unsigned char *)sigtemp,32,fp2); // skip name
              }
      }

iclose(fp);
iclose(fp2);

printf("All done!\n");
}

END_OF_MAIN();


void ithe_userexitfunction()
{
}

int GetWadEntry(IFILE *ifp, char *name)
{
unsigned int sig,num,off,ctr;
char buf[]="nothing!";

iseek(ifp,40,SEEK_SET);
sig = igetl_i(ifp);
if(sig != 0x44415750)
       {
       printf("sig = %x\n",sig);
	ithe_panic("Incorrect WAD substructure in file",ifp->truename);
        }

num = igetl_i(ifp);
off = igetl_i(ifp);

iseek(ifp,off+40,SEEK_SET); // Cancel out the signature
for(ctr=0;ctr<num;ctr++)
	{
	off = igetl_i(ifp);
	igetl_i(ifp); // skip length
	iread((unsigned char *)buf,8,ifp);
//	ilog_quiet("'%s'%s'\n",buf,name);
	if(!strcmp(buf,name))
		{
		iseek(ifp,off+40,SEEK_SET); // Cancel out the signature
		return 1;
		}
	}

return 0;
}
