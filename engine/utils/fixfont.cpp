#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IRE_FNT1 0x31544e46
#define IRE_FNT2 0x32544e46


int main(int argc, char *argv[])
{
FILE *fpo,*fpi;
char filename[256];
int ctr,i;
unsigned char buffer[65536];
unsigned char Pal[768];
int val,w,h;
int GotPal=0;
unsigned int version=0;

if(argc < 3 || (argv[1][0] == '-'))
	{
	printf("Usage: fixfont <input.fnt> <output.fnt>\n");
	printf("       This program will make Magic Pink into the transparent colour\n");
	exit(1);
	}

if(!strcmp(argv[1],argv[2]))
	{
	printf("Error: Input and output files must be different\n");
	exit(1);
	}

fpi=fopen(argv[1],"rb");
if(!fpi)
	{
	printf("Error reading input file '%s'\n",argv[1]);
	exit(1);
	}

fread(&version,1,4,fpi);
if(version == IRE_FNT1)
	{
	printf("Error: This program is only for colour font files\n");
	exit(1);
	}

if(version!=IRE_FNT2)
	{
	printf("Error: input file '%s' has incorrect header\n",argv[1]);
	fclose(fpi);
	exit(1);
	}

fseek(fpi,-768L,SEEK_END);
fread(Pal,1,768,fpi);
fseek(fpi,4L,SEEK_SET);


// Palette swapping
int pinkindex=-1;
for(ctr=0;ctr<256;ctr++)
	{
	i=ctr*3;
//	printf("Entry %d = %d,%d,%d\n",ctr,Pal[i],Pal[i+1],Pal[i+2]);
	if(Pal[i] > 250 && Pal[i+1] == 0 && Pal[i+2] > 250)
		{
		pinkindex=i;
		break;  // If there's several pinks, you're screwed
		}
	}

if(pinkindex == -1)
	{
	printf("Error: Couldn't find Magic Pink in palette, cannot continue.\n");
	exit(1);
	}

printf("Pink is index %d\n",pinkindex);

// Swap the palette entries over
i=pinkindex*3;
val=Pal[i]; Pal[i] = Pal[0]; Pal[0]=val;
val=Pal[i+1]; Pal[i+1] = Pal[1]; Pal[1]=val;
val=Pal[i+2]; Pal[i+2] = Pal[2]; Pal[2]=val;

// Okay time to create the output file
fpo=fopen(argv[2],"wb");
if(!fpo)
	{
	printf("Error creating output file '%s'\n",argv[2]);
	exit(1);
	}

// Write header
val=IRE_FNT2;
fwrite(&val,1,4,fpo);

// Read in all 256 elements and export them
for(ctr=0;ctr<256;ctr++)
	{
	fread(&w,1,2,fpi);
	fread(&h,1,2,fpi);
	printf("element %d, %dx%d\n",ctr,w,h);

	fwrite(&w,1,2,fpo);
	fwrite(&h,1,2,fpo);

	if(w == 0 || h == 0)
		continue; // Blank, don't create it

	val=w*h;
	if(val > 65535)
		{
		printf("Error: %s is %d bytes long, should not exceed 64k!\n",filename,val);
		exit(1);
		}
	fread(buffer,1,val,fpi);

	for(i=0;i<val;i++)
		{
		if(buffer[i] == pinkindex)
			buffer[i]=0;
		else
			if(buffer[i]==0)
				buffer[i]=pinkindex;
		}

	fwrite(buffer,1,val,fpo);
	}

fclose(fpi);
fwrite(Pal,1,768,fpo);
fclose(fpo);

printf("Finished, written file '%s'\n",argv[2]);
}
