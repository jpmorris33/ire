//
//      IRE Map Resizer
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
//int GetWadEntry(IFILE *fp,char *name);

char filename1[256];
char filename2[256];
char filename3[256];
char filename4[256];
char sigtemp[256];
char *cfa,*retptr;

int main(int argc, char *argv[])
{
IFILE *fp;
FILE *fpi,*fpo;
char *ptr;
WORLD curmap,newmap;
int mem,map_H,map_W,key,cx,cy,xoff,yoff,nx,ny;
char buf[1024];
char *word;
int copy,ret;

printf("IRE Map Resizer 1.6\n");
printf("===================\n\n");

printf("ERROR: This program needs to be updated to cope with the current map formats\n");
exit(1);


if(argc<2)
    {
    printf("resize <mapfile>\n");
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


strcpy(filename2,filename1);
strcat(filename2,".map");

fp = iopen(filename2);
if(!fp)
    {
    printf("Could not load file '%s'\n",filename2);
    exit(1);
    }

iread((unsigned char *)sigtemp,8,fp);
if(strncmp(sigtemp,COOKIE_MAPFILE,4))
	{
	printf("Unknown or obsolete file format in map file '%s'\n",filename2);
	exit(1);
	}

read_WORLD(&curmap,fp);
if((unsigned)curmap.sig != MAPSIG)
    {
    printf("Current map version is 0x%x, but this is 0x%x\n",MAPSIG,curmap.sig);
    exit(1);
    }

curmap.physmap = (unsigned short *)M_get(curmap.w * curmap.h,sizeof(short));
iread((unsigned char *)curmap.physmap,curmap.w*curmap.h*sizeof(short),fp);
map_W = curmap.w;
map_H = curmap.h;
iclose(fp);

printf("Map loaded.\n");
mem = map_W * map_H * (sizeof(short)+sizeof(char)+sizeof(char)+sizeof(char)+sizeof(OBJECT
*))/1024; printf("\n\n");
printf("The map is currently %dx%d, and uses %dKB of memory.\n",map_W,map_H,mem);


printf("Enter the width of the map:\n");
fflush(stdin);
ret=scanf("%d",&map_W);
printf("Enter the height of the map:\n");
ret=scanf("%d",&map_H);
newmap.w = map_W;
newmap.h = map_H;

try_again:
mem = map_W * map_H * (sizeof(short)+sizeof(char)+sizeof(char)+sizeof(char)+sizeof(OBJECT *))/1024; printf("\n\n\n");
printf("The new map will be %dx%d, and require %dKB of memory.\n",map_W,map_H,mem);
printf("Is this okay? (Y/N)\n");
do {
   key=getchar();
   } while(key < ' ');
if(key!='y' && key!='Y')
    {
    printf("Enter the width of the map:\n");
    fflush(stdin);
    ret=scanf("%d",&map_W);
    printf("Enter the height of the map:\n");
    ret=scanf("%d",&map_H);
    newmap.w = map_W;
    newmap.h = map_H;
    goto try_again;
    }

newmap.sig=MAPSIG;

printf("Enter the X offset to copy from (0 = left):\n");
fflush(stdin);
ret=scanf("%d",&xoff);
printf("Enter the Y offset to copy from (0 = top):\n");
ret=scanf("%d",&yoff);

newmap.physmap = (unsigned short *)M_get(newmap.w * newmap.h,sizeof(short));
newmap.roof = (unsigned	char *)M_get(newmap.w *	newmap.h,sizeof(char));
newmap.light = (unsigned char *)M_get(newmap.w * newmap.h,sizeof(char));

printf("Enter the file to save to (without extension):\n");
ret=scanf("%s",filename3);
strcpy(filename4,filename3);
strcat(filename4,".map");

map_W = curmap.w;
if(map_W > newmap.w)
    map_W = newmap.w;
map_H = curmap.h;
if(map_H > newmap.h)
    map_H = newmap.h;

printf("Processing Map\n");

for(cx=0;cx<newmap.w;cx++)
    for(cy=0;cy<newmap.h;cy++)
        {
//        newmap.physmap[(newmap.w * cy)+cx] = 0; // Be safe
        nx=cx+xoff;
        ny=cy+yoff;
        if(nx >= 0 && nx < curmap.w)
                if(ny >= 0 && ny < curmap.h)
                        newmap.physmap[(newmap.w * cy)+cx] = curmap.physmap[(curmap.w * ny)+nx];
        }

printf("Saving...\n");

fp=iopen_write(filename4);
if(!fp)
       {
       printf("Can't write to file '%s'\n",filename4);
       exit(1);
       }
       
iwrite((unsigned char *)COOKIE_MAPFILE,4,fp);
iwrite((unsigned char *)BINCOOKIE_R00,4,fp);
write_WORLD(&newmap,fp);
iwrite((unsigned char *)newmap.physmap,newmap.w*newmap.h*sizeof(short),fp);
iclose(fp);

printf("Needs rewrite\n");
exit(1);

printf("Loading Rooftops..\n");
strcpy(filename2,filename1);
strcat(filename2,".mz2");
fp=iopen(filename2);
if(!fp)
       {
       printf("Cannot open file '%s'\n",filename2);
       exit(1);
       }

iread((unsigned char *)sigtemp,8,fp);
if(strncmp(sigtemp,COOKIE_Z2,4))
	{
	if(strncmp(sigtemp,COOKIE_Z2,4))
		{
		printf("Unknown file format in map file '%s'\n",filename2);
		exit(1);
		}
	iseek(fp,40,SEEK_SET); // Old version, skip over it
	}

curmap.roof = (unsigned char *)M_get(curmap.w * curmap.h,sizeof(char));
iread((unsigned char *)curmap.roof,curmap.w*curmap.h*sizeof(unsigned char),fp);
iclose(fp);

strcpy(filename4,filename3);
strcat(filename4,".mz2");

printf("Processing Rooftops\n");
/*
for(cx=0;cx<map_W;cx++)
    for(cy=0;cy<map_H;cy++)
        newmap.roof[(newmap.w * cy)+cx] = curmap.roof[(curmap.w * (cy+yoff))+cx+xoff];
*/
for(cx=0;cx<newmap.w;cx++)
    for(cy=0;cy<newmap.h;cy++)
        {
        nx=cx+xoff;
        ny=cy+yoff;
        if(nx >= 0 && nx < curmap.w)
                if(ny >= 0 && ny < curmap.h)
                        newmap.roof[(newmap.w * cy)+cx] = curmap.roof[(curmap.w * ny)+nx];
        }

printf("Saving...\n");

fp=iopen_write(filename4);
if(!fp)
       {
       printf("Can't write to file '%s'\n",filename4);
       exit(1);
       }

iwrite((unsigned char *)COOKIE_Z2,4,fp);
iwrite((unsigned char *)BINCOOKIE_R00,4,fp);
iwrite((unsigned char *)newmap.roof,newmap.w*newmap.h*sizeof(char),fp);
iclose(fp);

printf("Copying Objects..\n");
strcpy(filename2,filename1);
strcat(filename2,".mz1");
strcpy(filename4,filename3);
strcat(filename4,".mz1");


fpi=fopen(filename2,"r");
if(!fpi)
       {
       printf("Can't open file '%s'\n",filename2);
       exit(1);
       }

fpo=fopen(filename4,"w");
if(!fpo)
       {
       printf("Can't open file '%s'\n",filename4);
       exit(1);
       }

do
        {
        retptr=fgets(buf,1024,fpi);
        word=strfirst(buf);
        strstrip(word);
        copy=1; // Assume we want to copy the line exactly

        // Object AT x y
        if(!istricmp(word,"at"))
                {
                word=strgetword(buf,3);
                strstrip(word);
                cx = atoi(word);

                word=strgetword(buf,4);
                strstrip(word);
                cy = atoi(word);

                cx -= xoff;
                cy -= yoff;

                if(cx<0)
                        cx=65535;
                if(cy<0)
                        cy=65535;
                
                copy=0;
                fprintf(fpo,"\t\tat %d %d\n",cx,cy);
                }

        // decorative NNN at x y
        if(!istricmp(word,"decorative"))
                {
                word=strgetword(buf,3);
                strstrip(word);

                strcpy(sigtemp,word);

                word=strgetword(buf,5);
                strstrip(word);
                cx = atoi(word);

                word=strgetword(buf,6);
                strstrip(word);
                cy = atoi(word);

                cx -= xoff;
                cy -= yoff;

                if(cx<0)
                        cx=65535;
                if(cy<0)
                        cy=65535;
                
                copy=0;
                fprintf(fpo,"\tdecorative %s at %d %d\n",sigtemp,cx,cy);
                }

        if(copy)
                fputs(buf,fpo);
        } while(!feof(fpi));

fclose(fpi);
fclose(fpo);

printf("Loading Lightmaps..\n");
strcpy(filename2,filename1);
strcat(filename2,".mz3");
fp=iopen(filename2);
if(!fp)
       {
       printf("Cannot open file '%s'\n",filename2);
       exit(1);
       }

iread((unsigned char *)sigtemp,8,fp);
if(strncmp(sigtemp,COOKIE_Z3,4))
	{
	if(strncmp(sigtemp,OLDCOOKIE_MZ3,4))
		{
		printf("Unknown file format in map file '%s'\n",filename2);
		exit(1);
		}
	iseek(fp,40,SEEK_SET); // Old version, skip over it
	}


curmap.light = (unsigned char *)M_get(curmap.w * curmap.h,sizeof(char));
iread((unsigned char *)curmap.light,curmap.w*curmap.h*sizeof(unsigned char),fp);
iclose(fp);

strcpy(filename4,filename3);
strcat(filename4,".mz3");

printf("Processing Lightmaps\n");

for(cx=0;cx<map_W;cx++)
    for(cy=0;cy<map_H;cy++)
        newmap.light[(newmap.w * cy)+cx] = curmap.light[(curmap.w * (cy+yoff))+cx+xoff];

printf("Saving...\n");

fp=iopen_write(filename4);
if(!fp)
       {
       printf("Can't write to file '%s'\n",filename4);
       exit(1);
       }

iwrite((unsigned char *)COOKIE_Z3,4,fp);
iwrite((unsigned char *)BINCOOKIE_R01,4,fp);
iwrite((unsigned char *)newmap.light,newmap.w*newmap.h*sizeof(char),fp);
iclose(fp);

printf("All done!\n");

return 1;
}

//END_OF_MAIN();


void ithe_userexitfunction()
{
}
