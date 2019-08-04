//
//	IRE pixel blending engines
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include "../iregraph.hpp"
#include "blenders.hpp"

#ifdef _WIN32
typedef unsigned short u_int16_t;
typedef unsigned long u_int32_t;
#endif

//void (*BlenderOp)(unsigned char *dest, unsigned char *src);
BLENDERFUNC BlenderOp=NULL;

static int BlendAlpha=0;
static int IRE_ChromaKey;
static void BlendSolid15(unsigned char *dest, unsigned char *src);
static void BlendSolid16(unsigned char *dest, unsigned char *src);
static void BlendSolid24(unsigned char *dest, unsigned char *src);
static void BlendSolid32(unsigned char *dest, unsigned char *src);
static void BlendTrans15(unsigned char *dest, unsigned char *src);
static void BlendTrans16(unsigned char *dest, unsigned char *src);
static void BlendTrans24(unsigned char *dest, unsigned char *src);
static void BlendTrans32(unsigned char *dest, unsigned char *src);
static void BlendAdd15(unsigned char *dest, unsigned char *src);
static void BlendAdd16(unsigned char *dest, unsigned char *src);
static void BlendAdd24(unsigned char *dest, unsigned char *src);
static void BlendAdd32(unsigned char *dest, unsigned char *src);
static void BlendSub15(unsigned char *dest, unsigned char *src);
static void BlendSub16(unsigned char *dest, unsigned char *src);
static void BlendSub24(unsigned char *dest, unsigned char *src);
static void BlendSub32(unsigned char *dest, unsigned char *src);
static void BlendInvert15(unsigned char *dest, unsigned char *src);
static void BlendInvert16(unsigned char *dest, unsigned char *src);
static void BlendInvert24(unsigned char *dest, unsigned char *src);
static void BlendInvert32(unsigned char *dest, unsigned char *src);
static void BlendDissolve15(unsigned char *dest, unsigned char *src);
static void BlendDissolve16(unsigned char *dest, unsigned char *src);
static void BlendDissolve24(unsigned char *dest, unsigned char *src);
static void BlendDissolve32(unsigned char *dest, unsigned char *src);

static int BlendGetRandom();

static u_int32_t *_16to32;	// Lookup table to convert to 32bpp
static u_int16_t *_32to16;	// Lookup table to convert to 16bpp

static void Init15tables();
static void Init16tables();

#define _16TO32TABLE 0x10000
#define _32TO16TABLE 0x400000	// We use some tricks to shrink the lookup table size dramatically

int BlenderInit(IRECOLOUR *chromakey, int bpp)
{
if(chromakey)
	IRE_ChromaKey = chromakey->packed;
BlenderOp=NULL;

printf("Init blenders for %d bpp\n",bpp);

if(bpp > 16)
	return 1; // Okay, we're done.  Don't bother with the lookup tables

// Okay, build some lookup tables to quickly convert between 16 and 32bpp
_16to32 = (u_int32_t *)calloc(_16TO32TABLE,sizeof(u_int32_t));	// This is straightforward, 64k 32-bit words
if(!_16to32)
	return 0;

_32to16 = (u_int16_t *)calloc(_32TO16TABLE,sizeof(u_int16_t));	// This uses some sneaky shortcuts to save memory
if(!_32to16)
	return 0;

if(bpp == 15)
	Init15tables();
if(bpp == 16)
	Init16tables();
return 1;
}

void BlenderTerm()
{
if(_16to32)
	free(_16to32);
_16to32=NULL;
if(_32to16)
	free(_32to16);
_32to16=NULL;
}


int BlenderGetAlpha()	{
return BlendAlpha;
}

void BlenderSetAlpha(int alpha)	{
if(alpha>0 && alpha <256)
	BlendAlpha=alpha;
else
	BlendAlpha=0;
}


void BlenderSetMode(int mode, int bpp)	{
BlenderOp = BlenderGetOp(mode,bpp);
}


BLENDERFUNC BlenderGetOp(int mode, int bpp)	{
switch(mode)
	{
	case ALPHA_SOLID:
		if(bpp==15)
			return BlendSolid15;
		if(bpp==16)
			return BlendSolid16;
		if(bpp==24)
			return BlendSolid24;
		if(bpp==32)
			return BlendSolid32;
		break;
		
	case ALPHA_TRANS:
		if(bpp==15)
			return BlendTrans15;
		if(bpp==16)
			return BlendTrans16;
		if(bpp==24)
			return BlendTrans24;
		if(bpp==32)
			return BlendTrans32;
		break;

	case ALPHA_ADD:
		if(bpp==15)
			return BlendAdd15;
		if(bpp==16)
			return BlendAdd16;
		if(bpp==24)
			return BlendAdd24;
		if(bpp==32)
			return BlendAdd32;
		break;

	case ALPHA_SUBTRACT:
		if(bpp==15)
			return BlendSub15;
		if(bpp==16)
			return BlendSub16;
		if(bpp==24)
			return BlendSub24;
		if(bpp==32)
			return BlendSub32;
		break;
		
	case ALPHA_INVERT:
		if(bpp==15)
			return BlendInvert15;
		if(bpp==16)
			return BlendInvert16;
		if(bpp==24)
			return BlendInvert24;
		if(bpp==32)
			return BlendInvert32;
		break;

	case ALPHA_DISSOLVE:
		if(bpp==15)
			return BlendDissolve15;
		if(bpp==16)
			return BlendDissolve16;
		if(bpp==24)
			return BlendDissolve24;
		if(bpp==32)
			return BlendDissolve32;
		break;
	};
	
return NULL;
}


/*
 * Lookup table generators
 */

void Init15tables()	{
int ctr;
u_int32_t r32,g32,b32;
u_int16_t r16,g16,b16;

// 0RRR RRGG GGGB BBBB 	-> 00000000 RRRRR000 GGGGG000 BBBBB000

// This is a straightforward 16bpp -> 32bpp lookup table
for(ctr=0;ctr<_16TO32TABLE;ctr++)	{
	r32 = (ctr & 0x7c00) << 9;
	g32 = (ctr & 0x03e0) << 6;
	b32 = (ctr & 0x001f) << 3;
	_16to32[ctr] = r32|g32|b32;
	}
	
// This one is more tricky.  We only care about the lower 24 bits anyway, but that still gives us a 32MB
// table (16.7M * 2 bytes), which on ARM devices like the AC100 or Pandora is uncomfortably large, and it's
// these devices which are likely to want to run in 16bpp anyway.
// However, since we're converting from 8-bit channels to 5-bit channels, the lower 3 bits of the blue channel
// don't matter.  That means that prior to the lookup we can shift the RGB triplet right three bits, and thereby
// reduce the search space to 21 bits, or a 4MB lookup table (2,097,152 * 16 bits)

for(ctr=0;ctr<_32TO16TABLE;ctr++)	{
	// This is a lopsided RGB entry
	// 00000000 000RRRRR RRRGGGGG GGGBBBBB -> 0RRR RRGG GGGB BBBB 
	r16 = (ctr & 0x001f0000) >> 6;
	g16 = (ctr & 0x00001f00) >> 3;
	b16 = (ctr & 0x0000001f);	// Already been shifted prior to the lookup
	_32to16[ctr] = r16|g16|b16;
	}

// Self-test
int v;
for(ctr=0;ctr<32768;ctr++)	{
	v = _16to32[ctr];
	if(_32to16[v>>3] != ctr)	{
		IRE_Shutdown();
		printf("Conversion ERROR: 16 = %x, 32 = %x, reconverted = %x\n",ctr,v,_32to16[v>>3]);
		exit(1);
		}
	}
}


void Init16tables()	{
int ctr;
u_int32_t r32,g32,b32;
u_int16_t r16,g16,b16;

// RRRR RGGG GGGB BBBB 	-> 00000000 RRRRR000 GGGGGG00 BBBBB000

// This is a straightforward 16bpp -> 32bpp lookup table
for(ctr=0;ctr<_16TO32TABLE;ctr++)	{
	r32 = (ctr & 0xf800) << 8;
	g32 = (ctr & 0x07e0) << 5;
	b32 = (ctr & 0x001f) << 3;
	_16to32[ctr] = r32|g32|b32;
	}
	
// This one is more tricky.  We only care about the lower 24 bits anyway, but that still gives us a 32MB
// table (16.7M * 2 bytes), which on ARM devices like the AC100 or Pandora is uncomfortably large, and it's
// these devices which are likely to want to run in 16bpp anyway.
// However, since we're converting from 8-bit channels to 5-bit channels (6 for green), the lower 3 bits of
// the blue channel don't matter.  That means that prior to the lookup we can shift the RGB triplet right
// three bits, and thereby reduce the search space to 21 bits, or a 4MB lookup table (2,097,152 * 16 bits)

for(ctr=0;ctr<_32TO16TABLE;ctr++)	{
	// This is a lopsided RGB entry
	// 00000000 000RRRRR RRRGGGGG GGGBBBBB -> RRRR RGGG GGGB BBBB 
	r16 = (ctr & 0x001f0000) >> 5;
	g16 = (ctr & 0x00001f80) >> 2;
	b16 = (ctr & 0x0000001f);	// Already been shifted prior to the lookup
	_32to16[ctr] = r16|g16|b16;
	}
	
// Self-test
int v;
for(ctr=0;ctr<65536;ctr++)	{
	v = _16to32[ctr];
	if(_32to16[v>>3] != ctr)	{
		IRE_Shutdown();
		printf("Conversion ERROR: 16 = %x, 32 = %x, reconverted = %x\n",ctr,v,_32to16[v>>3]);
		exit(1);
		}
	}
}



/*
 * Blender operations
 */

#define INITINT16(newptr,srcptr) u_int16_t* newptr = (u_int16_t *)srcptr;
#define INITINT32(newptr,srcptr) u_int32_t* newptr = (u_int32_t *)srcptr;

// Solid

void BlendSolid15(unsigned char *dest, unsigned char *src)	{
INITINT16(d16,dest);
INITINT16(s16,src);
*d16=*s16;
}

void BlendSolid16(unsigned char *dest, unsigned char *src)	{
INITINT16(d16,dest);
INITINT16(s16,src);
*d16=*s16;
}

void BlendSolid24(unsigned char *dest, unsigned char *src)	{
*dest++=*src++;
*dest++=*src++;
*dest++=*src++;
}

void BlendSolid32(unsigned char *dest, unsigned char *src)	{
INITINT32(d32,dest);
INITINT32(s32,src);
*d32=*s32;
}

// Transparent

void BlendTrans15(unsigned char *dest, unsigned char *src)	{
INITINT16(d16,dest);
INITINT16(s16,src);
if(*s16 == IRE_ChromaKey)
	return;
u_int32_t s32 = _16to32[*s16];
u_int32_t d32 = _16to32[*d16];
BlendTrans32((unsigned char *)&d32,(unsigned char *)&s32);	// Please inline me...
*d16=_32to16[d32>>3];
}

void BlendTrans16(unsigned char *dest, unsigned char *src)	{
INITINT16(d16,dest);
INITINT16(s16,src);
if(*s16 == IRE_ChromaKey)
	return;
u_int32_t s32 = _16to32[*s16];
u_int32_t d32 = _16to32[*d16];
BlendTrans32((unsigned char *)&d32,(unsigned char *)&s32);	// Please inline me...
*d16=_32to16[d32>>3];
}

void BlendTrans24(unsigned char *dest, unsigned char *src)	{
int target = dest[0] + ((dest[1]<<8)&0xff00) + ((dest[2]<<16)&0xff0000);
if(target == IRE_ChromaKey)
	return;
if(BlendAlpha==255)	{
	*dest++=*src++;
	*dest++=*src++;
	*dest++=*src++;
} else	{
	int temp;
	temp = ((*dest)-(*src))*BlendAlpha/256+(*src);
	src++;
	*dest++=temp;
	
	temp = ((*dest)-(*src))*BlendAlpha/256+(*src);
	src++;
	*dest++=temp;
	
	temp = ((*dest)-(*src))*BlendAlpha/256+(*src);
	src++;
	*dest++=temp;
}
}

void BlendTrans32(unsigned char *dest, unsigned char *src)	{
INITINT32(d32,dest);
INITINT32(s32,src);
if(*s32 == IRE_ChromaKey)
	return;
if(BlendAlpha==255)
	*d32=*s32;
else	{
	int temp;
	temp = ((*dest)-(*src))*BlendAlpha/256+(*src);
	src++;
	*dest++=temp;
	
	temp = ((*dest)-(*src))*BlendAlpha/256+(*src);
	src++;
	*dest++=temp;
	
	temp = ((*dest)-(*src))*BlendAlpha/256+(*src);
	src++;
	*dest++=temp;
	
	*dest++=0;
}
}


// Add

void BlendAdd15(unsigned char *dest, unsigned char *src)	{
INITINT16(d16,dest);
INITINT16(s16,src);

if(BlendAlpha < 255)	{
	u_int32_t s32 = _16to32[*s16];
	u_int32_t d32 = _16to32[*d16];
	BlendAdd32((unsigned char *)&d32,(unsigned char *)&s32);	// Please inline me...
	*d16=_32to16[d32>>3];
	return;
	}

// 0111 1100 0001 1111
u_int16_t tempsrcA = *s16 & 0x7c1f;	// Strip out the middle so we can parallelise the additions
u_int16_t tempdstA = *d16 & 0x7c1f;
// 0000 0011 1110 0000
u_int16_t tempsrcB = *s16 & 0x03e0;	// Now just the middle
u_int16_t tempdstB = *d16 & 0x03e0;

tempdstA += tempsrcA;
if(tempdstA > 0x7cff)	{
	// If the top half overran, saturate it, making sure not to squash the lower half
	tempdstA |= 0x7c00;
	tempdstA &= 0x7cff;
	}
if((tempdstA & 0xff)> 0x1f)	{
	// Make sure the bottom half saturates properly as well
	tempdstA |= 0x1f;
	tempdstA &= 0xff1f;
}
	
tempdstB += tempsrcB;
if(tempdstB < 0x03e0)
	tempdstB=0x03e0;

// Now merge them
*d16=tempdstA|tempdstB;
}

void BlendAdd16(unsigned char *dest, unsigned char *src)	{
INITINT16(d16,dest);
INITINT16(s16,src);

if(BlendAlpha < 255)	{
	u_int32_t s32 = _16to32[*s16];
	u_int32_t d32 = _16to32[*d16];
	BlendAdd32((unsigned char *)&d32,(unsigned char *)&s32);	// Please inline me...
	*d16=_32to16[d32>>3];
	return;
	}

// 1111 1000 0001 1111
u_int16_t tempsrcA = *s16 & 0xf81f;	// Strip out the middle so we can parallelise the additions
u_int16_t tempdstA = *d16 & 0xf81f;
// 0000 0111 1110 0000
u_int16_t tempsrcB = *s16 & 0x07e0;	// Now just the middle
u_int16_t tempdstB = *d16 & 0x07e0;

tempdstA += tempsrcA;
if(tempdstA > 0xffff)	{
	// If the top half overran, saturate it, making sure not to squash the lower half
	tempdstA |= 0xf800;
	tempdstA &= 0xf8ff;
	}
if((tempdstA & 0xff)> 0x1f)	{
	// Make sure the bottom half saturates properly as well
	tempdstA |= 0x1f;
	tempdstA &= 0xff1f;
}
	
tempdstB += tempsrcB;
if(tempdstB < 0x07e0)
	tempdstB=0x07e0;

// Now merge them
*d16=tempdstA|tempdstB;
}

void BlendAdd24(unsigned char *dest, unsigned char *src)	{
int temp;

temp = (*dest)+(((*src++)*BlendAlpha)>>8);
if(temp>255)
	temp=255;
*dest++=temp;

temp = (*dest)+(((*src++)*BlendAlpha)>>8);
if(temp>255)
	temp=255;
*dest++=temp;

temp = (*dest)+(((*src++)*BlendAlpha)>>8);
if(temp>255)
	temp=255;
*dest++=temp;
}

void BlendAdd32(unsigned char *dest, unsigned char *src)	{
int temp;

temp = (*dest)+(((*src++)*BlendAlpha)>>8);
if(temp>255)
	temp=255;
*dest++=temp;

temp = (*dest)+(((*src++)*BlendAlpha)>>8);
if(temp>255)
	temp=255;
*dest++=temp;

temp = (*dest)+(((*src++)*BlendAlpha)>>8);
if(temp>255)
	temp=255;
*dest++=temp;

*dest=0; // Alpha
}

// Subtract

void BlendSub15(unsigned char *dest, unsigned char *src)	{
INITINT16(d16,dest);
INITINT16(s16,src);
u_int16_t dr,dg,db;
u_int16_t sr,sg,sb;

if(BlendAlpha < 255)	{
	u_int32_t s32 = _16to32[*s16];
	u_int32_t d32 = _16to32[*d16];
	BlendSub32((unsigned char *)&d32,(unsigned char *)&s32);	// Please inline me...
	*d16=_32to16[d32>>3];
	return;
	}

dr=dg=db=*d16;
dr&=0xf800;
dg&=0x07e0;
db&=0x001f;

sr=sg=sb=*s16;
sr&=0xf800;
sg&=0x07e0;
sb&=0x001f;

// 0RRR RRGG GGGB BBBB
dr-=sr;
if(dr<0x0400) dr=0;	// Minimum value for red
dg-=sg;
if(dg<0x0020) dg=0;	// Minimum value for green
db-=sb;
if(db&0x8000) db=0;	// And blue will wrap around

*d16=dr|dg|db;
}

void BlendSub16(unsigned char *dest, unsigned char *src)	{
INITINT16(d16,dest);
INITINT16(s16,src);
u_int16_t dr,dg,db;
u_int16_t sr,sg,sb;

if(BlendAlpha < 255)	{
	u_int32_t s32 = _16to32[*s16];
	u_int32_t d32 = _16to32[*d16];
	BlendSub32((unsigned char *)&d32,(unsigned char *)&s32);	// Please inline me...
	*d16=_32to16[d32>>3];
	return;
	}

dr=dg=db=*d16;
dr&=0x7c00;
dg&=0x03e0;
db&=0x001f;

sr=sg=sb=*s16;
sr&=0x7c00;
sg&=0x03e0;
sb&=0x001f;

// RRRR RGGG GGGB BBBB
dr-=sr;
if(dr<0x0800) dr=0;	// Minimum value for red
dg-=sg;
if(dg<0x0020) dg=0;	// Minimum value for green
db-=sb;
if(db&0x8000) db=0;	// And blue will wrap around

*d16=dr|dg|db;
}

void BlendSub24(unsigned char *dest, unsigned char *src)	{
int temp;

temp = (*dest)-(((*src++)*BlendAlpha)>>8);
if(temp<0)
	temp=0;
*dest++=temp;

temp = (*dest)-(((*src++)*BlendAlpha)>>8);
if(temp<0)
	temp=0;
*dest++=temp;

temp = (*dest)-(((*src++)*BlendAlpha)>>8);
if(temp<0)
	temp=0;
*dest++=temp;
}

void BlendSub32(unsigned char *dest, unsigned char *src)	{
int temp;

temp = (*dest)-(((*src++)*BlendAlpha)>>8);
if(temp<0)
	temp=0;
*dest++=temp;

temp = (*dest)-(((*src++)*BlendAlpha)>>8);
if(temp<0)
	temp=0;
*dest++=temp;

temp = (*dest)-(((*src++)*BlendAlpha)>>8);
if(temp<0)
	temp=0;
*dest++=temp;

*dest=0; // Alpha
}


// Invert

void BlendInvert15(unsigned char *dest, unsigned char *src)	{
INITINT16(d16,dest);
INITINT16(s16,src);

if(BlendAlpha < 255)	{
	u_int32_t s32 = _16to32[*s16];
	u_int32_t d32 = _16to32[*d16];
	BlendInvert32((unsigned char *)&d32,(unsigned char *)&s32);	// Please inline me...
	*d16=_32to16[d32>>3];
	return;
	}

*d16=*s16 ^ 0x7fff;
}

void BlendInvert16(unsigned char *dest, unsigned char *src)	{
INITINT16(d16,dest);
INITINT16(s16,src);

if(BlendAlpha < 255)	{
	u_int32_t s32 = _16to32[*s16];
	u_int32_t d32 = _16to32[*d16];
	BlendInvert32((unsigned char *)&d32,(unsigned char *)&s32);	// Please inline me...
	*d16=_32to16[d32>>3];
	return;
	}
	
*d16=*s16 ^ 0xffff;
}

void BlendInvert24(unsigned char *dest, unsigned char *src)	{
if(BlendAlpha==255)	{
	*dest++=(*src++)^0xff;
	*dest++=(*src++)^0xff;
	*dest++=(*src++)^0xff;
} else	{
	int temp;
	temp = (((*src++)^0xff)*BlendAlpha)>>8;
	*dest++=temp;
	temp = (((*src++)^0xff)*BlendAlpha)>>8;
	*dest++=temp;
	temp = (((*src++)^0xff)*BlendAlpha)>>8;
	*dest++=temp;
}
}

void BlendInvert32(unsigned char *dest, unsigned char *src)	{
INITINT32(d32,dest);
INITINT32(s32,src);

if(BlendAlpha==255)
	*d32=*s32 ^ 0x00ffffff;
else	{
	int temp;
	temp = (((*src++)^0xff)*BlendAlpha)>>8;
	*dest++=temp;
	temp = (((*src++)^0xff)*BlendAlpha)>>8;
	*dest++=temp;
	temp = (((*src++)^0xff)*BlendAlpha)>>8;
	*dest++=temp;
	*dest=0;
}
}

// Dissolve

void BlendDissolve15(unsigned char *dest, unsigned char *src)	{
if(BlendGetRandom() > BlendAlpha)
	return;
INITINT16(d16,dest);
INITINT16(s16,src);
*d16=*s16;
}

void BlendDissolve16(unsigned char *dest, unsigned char *src)	{
if(BlendGetRandom() > BlendAlpha)
	return;
INITINT16(d16,dest);
INITINT16(s16,src);
*d16=*s16;
}

void BlendDissolve24(unsigned char *dest, unsigned char *src)	{
if(BlendGetRandom() > BlendAlpha)
	return;
*dest++=*src++;
*dest++=*src++;
*dest++=*src++;
}

void BlendDissolve32(unsigned char *dest, unsigned char *src)	{
if(BlendGetRandom() > BlendAlpha)
	return;
INITINT32(d32,dest);
INITINT32(s32,src);
*d32=*s32;
}


//
//  Optimise this later
//

int BlendGetRandom()
{
return rand()&0xff;
}
