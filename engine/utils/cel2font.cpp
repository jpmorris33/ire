#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IRE_FNT1 0x31544e46
#define IRE_FNT2 0x32544e46

unsigned char Pal[768];
int PalCtr=0;

int FixColours(unsigned char *buffer, int len, unsigned char *celpal);
int SearchPal(unsigned char *pal, unsigned char r, unsigned char g, unsigned char b);

int main(int argc, char *argv[])
{
FILE *fpo,*fpi;
char filename[256];
int ctr;
unsigned char buffer[65536];
int val,w,h,mw,mh;
int colour=0;
unsigned char CelPal[768];

mw=mh=0;
memset(Pal,0,768);
memset(CelPal,0,768);
PalCtr=0;


if(argc < 2 || (argv[1][0] == '-'))
	{
	printf("Usage: cel2font <output.fnt> [-colour]\n");
	exit(1);
	}

// Colour or monochrome?
if(argc > 2)
	if(strcmp(argv[2],"-colour") == 0 || strcmp(argv[2],"-color") == 0)
		colour=1;

// Do a pre-scan
val=0;
for(ctr=0;ctr<256;ctr++)
	{
	sprintf(filename,"%02x.cel",ctr);
	fpi=fopen(filename,"rb");
	if(fpi)
		{
		fclose(fpi);
		val=1;
		}
	}
if(!val)
	{
	printf("Error: Searched from 00.cel to ff.cel - didn't find any files\n");
	exit(1);
	}


fpo=fopen(argv[1],"wb");
if(!fpo)
	{
	printf("Error writing to output file '%s'\n",argv[1]);
	exit(1);
	}

// Write header
if(colour)
	val=IRE_FNT2;
else
	val=IRE_FNT1;
fwrite(&val,1,4,fpo);

// Now write an element for each character from 0-255, which may be 0-length
for(ctr=0;ctr<256;ctr++)
	{
	sprintf(filename,"%02x.cel",ctr);
	fpi=fopen(filename,"rb");
	if(!fpi)
		{
		printf("Didn't find %s, skipping that character\n",filename);
		val=0;
		fwrite(&val,1,4,fpo);	// W&H
		continue;
		}
	val=0;
	fread(&val,1,2,fpi);
	if(val != 0x9119)
		{
		fclose(fpi);
		printf("Warning: %s is not a valid Animator 1 CEL file, skipping that character\n",filename);
		val=0;
		fwrite(&val,1,4,fpo);	// W&H
		continue;
		}

	w=0;
	fread(&w,1,2,fpi);
	fwrite(&w,1,2,fpo);	// Width

	h=0;
	fread(&h,1,2,fpi);
	fwrite(&h,1,2,fpo);	// Height

	if(w>mw)
		mw=w;
	if(h>mh)
		mh=h;

	val=w*h;
	if(val > 65535)
		{
		printf("Error: %s is %d bytes long, exceeds 64k!\n",filename,val);
		exit(1);
		}

	fseek(fpi,32L,SEEK_SET);	// Skip to the raw data

	fread(CelPal,1,768,fpi);
	fread(buffer,1,val,fpi);

	if(colour)
		if(!FixColours(buffer,val,CelPal))
			{
			printf("Error: too many distinct colours (got to '%s')\n",filename);
			exit(1);
			}

	fwrite(buffer,1,val,fpo);
	fclose(fpi);

	val=ctr;
	if(val<32 || val >=127)
		val=32;
	printf("Added %s, (%c)\n",filename,val);
	}

// Palette conversion
for(ctr=0;ctr<768;ctr++)
	Pal[ctr]=Pal[ctr]<<2;

if(colour)
	fwrite(Pal,1,768,fpo);

fclose(fpo);

printf("Wrote %s font file %s\n",colour?"colour":"monochrome",argv[2]);
printf("Largest character was %dx%d pixels\n",mw,mh);
if(colour)
	printf("Used %d colours\n",PalCtr/3);
}

///
// Remap the colours to the global palette
//

int FixColours(unsigned char *buffer, int len, unsigned char *celpal)
{
int ctr,rctr,i=0;
int mpi;
unsigned char outbuffer[65536];

for(ctr=0;ctr<256;ctr++)
	{
	mpi = SearchPal(Pal,celpal[i],celpal[i+1],celpal[i+2]);
	if(mpi == -1)
		{
		// Not in the main palette, try to add it
		if(PalCtr >= 768)
			return 0; // Out of colours
		mpi=PalCtr/3;
		Pal[PalCtr++]=celpal[i];
		Pal[PalCtr++]=celpal[i+1];
		Pal[PalCtr++]=celpal[i+2];
		}

	// Now remap the colour into the copy
	for(rctr=0;rctr<len;rctr++)
		if(buffer[rctr] == ctr)
			outbuffer[rctr]=mpi; // Patch in just this colour
	i+=3;
	}
// send the copy buffer back into the original, once all colours have been
// mapped to the new palette
memcpy(buffer,outbuffer,len);
return 1;
}


int SearchPal(unsigned char *pal, unsigned char r, unsigned char g, unsigned char b)
{
int ctr,i=0;
for(ctr=0;ctr<256;ctr++)
	{
	if(pal[i] == r && pal[i+1] == g && pal[i+2] == b)
		return ctr;
	i+=3;
	}

return -1;
}
