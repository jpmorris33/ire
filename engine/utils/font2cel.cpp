#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IRE_FNT1 0x31544e46
#define IRE_FNT2 0x32544e46


int main(int argc, char *argv[])
{
FILE *fpo,*fpi;
char filename[256];
int ctr;
char buffer[65536];
unsigned char Pal[768];
int val,w,h;
int GotPal=0;
unsigned int version=0;

// Simple default palette in case there isn't one
memset(Pal,255,768);
Pal[0]=0;
Pal[1]=0;
Pal[2]=0;
Pal[3]=0;
Pal[4]=0;
Pal[5]=255;

if(argc < 2 || (argv[1][0] == '-'))
	{
	printf("Usage: font2cel <input.fnt>\n");
	printf("       Note that this program will spit out many .CEL files in the current dir!\n");
	exit(1);
	}

fpi=fopen(argv[1],"rb");
if(!fpi)
	{
	printf("Error reading input file '%s'\n",argv[1]);
	exit(1);
	}

fread(&version,1,4,fpi);
if(version == IRE_FNT2)
	GotPal=1;

if(version!=IRE_FNT1 && version != IRE_FNT2)
	{
	printf("Error: input file '%s' has incorrect header\n",argv[1]);
	fclose(fpi);
	exit(1);
	}

// If we have a palette in the file, get it
if(GotPal)
	{
	fseek(fpi,-768L,SEEK_END);
	fread(Pal,1,768,fpi);
	fseek(fpi,4L,SEEK_SET);
	}

// Palette conversion
for(ctr=0;ctr<768;ctr++)
	Pal[ctr]=Pal[ctr]>>2;

// Read in all 256 elements and export them
for(ctr=0;ctr<256;ctr++)
	{
	w=h=0;
	fread(&w,1,2,fpi);
	fread(&h,1,2,fpi);

	if(w == 0 || h == 0)
		continue; // Blank, don't create it

	sprintf(filename,"%02x.cel",ctr);
	fpo=fopen(filename,"wb");
	if(!fpo)
		{
		printf("Error writing file '%s', skipping it\n",filename);
		continue;
		}
	val=0x9119;
	fwrite(&val,1,2,fpo);
	fwrite(&w,1,2,fpo);
	fwrite(&h,1,2,fpo);
	val=0;
	fwrite(&val,1,2,fpo);	// X,Y offset
	fwrite(&val,1,2,fpo);
	fputc(8,fpo); // 8bpp
	fputc(0,fpo); // compression
	val=w*h;
	fwrite(&val,1,4,fpo);	// image data length
	val=0;
	// 16 zeroes
	fwrite(&val,1,4,fpo);
	fwrite(&val,1,4,fpo);
	fwrite(&val,1,4,fpo);
	fwrite(&val,1,4,fpo);

	fwrite(Pal,1,768,fpo);

	val=w*h;
	if(val > 65535)
		{
		printf("Error: %s is %d bytes long, should not exceed 64k!\n",filename,val);
		exit(1);
		}
	fread(buffer,1,val,fpi);
	fwrite(buffer,1,val,fpo);
	fclose(fpo);

	val=ctr;
	if(ctr<32 || ctr >=127)
		ctr=32;
	printf("Wrote file %s, (%c)\n",filename,val);
	}

fclose(fpi);

printf("Finished export\n");
}
