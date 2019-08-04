#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IRE_FNT1 0x31544e46
#define IRE_FNT2 0x32544e46

int Getpixel(int x, int y);
int FindNextRow(int start, int *height);
int FindNextCharacter(int x, int y, int *width);


int W,H;
int nullcolour=0;
unsigned char buffer[131072];

int main(int argc, char *argv[])
{
FILE *fpo,*fpi;
char filename[256];
int ctr;
unsigned char Pal[768];
int val,w,h,mw,mh,x,y;
int GotPal=0;
int colour=0;

mw=mh=0;
memset(Pal,0,768);

if(argc < 3 || (argv[1][0] == '-'))
	{
	printf("Usage: grabfont <input.cel> <output.fnt> [-colour]\n");
	printf("       Attempts to extract a font from a single large .CEL grid\n");
	exit(1);
	}

// Colour or monochrome?
if(argc > 3)
	if(strcmp(argv[3],"-colour") == 0 || strcmp(argv[3],"-color") == 0)
		colour=1;

fpi=fopen(argv[1],"rb");
if(!fpi)
	{
	printf("Error opening input file '%s'\n",argv[1]);
	exit(1);
	}
val=0;
fread(&val,1,2,fpi);
if(val != 0x9119)
	{
	printf("Error: File is not a valid Animator 1 CEL file\n");
	exit(1);
	}
w=0;
fread(&w,1,2,fpi);

h=0;
fread(&h,1,2,fpi);

// Global size
W=w;
H=h;

val=w*h;
if(val > 131071)
	{
	printf("Error: %s is %d bytes long, exceeds 128k!\n",filename,val);
	exit(1);
	}

fseek(fpi,32L,SEEK_SET);	// Skip to the raw data
fread(Pal,1,768,fpi);
fread(buffer,1,val,fpi);
fclose(fpi);

// Read in the CEL file, now try and create an output file
fpo=fopen(argv[2],"wb");
if(!fpo)
	{
	printf("Error: Could not create file '%s'\n",argv[2]);
	exit(1);
	}

// Write header
if(colour)
	val=IRE_FNT2;
else
	val=IRE_FNT1;
fwrite(&val,1,4,fpo);

// Write blank characters up to SPACE
val=0; // W&H = 0 for a null entry
for(ctr=0;ctr<32;ctr++)
	fwrite(&val,1,4,fpo);	// W&H

nullcolour=buffer[0]; // This is the invalid colour

ctr=32; // start from SPACE
y=0;
do
	{
	if(ctr>255)
		break; // Too many characters
	  
	y=FindNextRow(y,&h);
	if(y == -1 || h < 1)
		break;
	
//	printf("Row start at %d for %d pixels\n",y,h);
	
	x=0;
	do
		{
		if(ctr>255)
			break; // Too many characters
			
		x=FindNextCharacter(x,y,&w);
		if(x == -1 || w < 1)
			break;

//		printf("Col start at %d for %d pixels\n",x,w);
		
		// Okay, we have a character
		if(w>mw)
			mw=w;
		if(h>mh)
			mh=h;
		
		fwrite(&w,1,2,fpo);	// Width
		fwrite(&h,1,2,fpo);	// Height

		for(int vy=0;vy<h;vy++)
			for(int vx=0;vx<w;vx++)
				 fputc(Getpixel(vx+x,vy+y),fpo);
			
		printf("Added %d (%c)\n",ctr,ctr);
			
		ctr++; // Another character written
		x+=w;
		} while(x < W);

	y+=h;
	} while(y < H);

// Okay, now write the remaining entries, starting from whatever ctr is at the moment
val=0; // W&H = 0 for a null entry
for(;ctr<256;ctr++)
	fwrite(&val,1,4,fpo);	// W&H
	
// Palette conversion
for(ctr=0;ctr<768;ctr++)
	Pal[ctr]=Pal[ctr]<<2;

if(colour)
	fwrite(Pal,1,768,fpo);

fclose(fpo);

printf("Wrote %s font file %s\n",colour?"colour":"monochrome",argv[1]);
printf("Largest character was %dx%d pixels\n",mw,mh);
}


int Getpixel(int x, int y)
{
if(x<0 || x>=W)
	return -1;
if(y<0 || y>=H)
	return -1;
return buffer[(y*W)+x];
}

//
//  Find a row of characters
//

int FindNextRow(int start, int *height)
{
int x,y;
int startrow=-1;
int endrow=-1;
int found;

*height=0;

found=0;
for(y=start;y<H;y++)
	{
	for(x=0;x<W;x++)
		if(Getpixel(x,y) != nullcolour)
			{
			found=1;
			break;
			}
	if(found)
		{
		startrow=y;
		break;
		}
	}
			
if(startrow == -1)
	return -1;

// Find end of row
for(y=startrow;y<H;y++)
	{
	found=0;
	for(x=0;x<W;x++)
		if(Getpixel(x,y) != nullcolour)
			found=1;
	if(!found)
		{
		endrow=y;
		break;
		}
	}

if(endrow == -1)
	return -1;

*height=endrow-startrow;
return startrow;
}



int FindNextCharacter(int x, int y, int *width)
{
int ctr;
int st=-1;
int nd=-1;

*width=0;

for(ctr=x;ctr<W;ctr++)
	if(Getpixel(ctr,y) != nullcolour)
		{
		st=ctr;
		break;
		}

if(st == -1)
	return -1;

for(ctr=st;ctr<W;ctr++)
	if(Getpixel(ctr,y) == nullcolour)
		{
		nd=ctr;
		break;
		}
		
if(nd == -1)
	return -1;

*width=nd-st;
return st;
}