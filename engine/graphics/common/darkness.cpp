/*
 *	Darken a bitmap
 */

#include "../../ithelib.h"
#include "../iregraph.hpp"

// Defines

// Variables

extern char ire_NOASM;
static unsigned char *light_cmap;
static unsigned char *dark_cmap;
static unsigned char light_cmapdata[65536];
static unsigned char dark_cmapdata[65536];

static unsigned short *coltab16;
static unsigned short *lightning16;
static int GotMMX=0;

void (*darken)(IREBITMAP *image, IRELIGHTMAP *tab);
void (*darken_bitmap)(IREBITMAP *image, unsigned int level);
void (*merge_blit)(unsigned char *dest, unsigned char *src, int h);
void (*lightning)(IREBITMAP *image);

void darken_darksprite(int x,int y, int scrw, int scrh, unsigned char *out, unsigned char *in);
void darken_lightsprite(int x,int y, int scrw, int scrh, unsigned char *out, unsigned char *in);

// Functions

void darken_init(int bpp);

static void darken_16_c(IREBITMAP *image, IRELIGHTMAP *tab);
static void darken_16s_c(IREBITMAP *image, unsigned int level);
static void darken_blit16_c(unsigned char *dest, unsigned char *src,int pixels);
static void darken_blit32_c(unsigned char *dest, unsigned char *src,int pixels);
#ifndef NO_ASM
static void darken_16_asm(IREBITMAP *image, IRELIGHTMAP *tab);
static void darken_32_asm(IREBITMAP *image, IRELIGHTMAP *tab);
static void darken_16s_asm(IREBITMAP *image, unsigned int level);
static void darken_32s_asm(IREBITMAP *image, unsigned int level);
#endif

static void lightning_generic_c(IREBITMAP *image);
static void lightning_16(IREBITMAP *image);
static void darken_16_init(void);
static void darken_32_c(IREBITMAP *image, IRELIGHTMAP *tab);
static void darken_generic_c(IREBITMAP *image, IRELIGHTMAP *tab);

static void darkens_generic_c(IREBITMAP *image, unsigned int level);
static void darken_32s_c(IREBITMAP *image, unsigned int level);
static void darken_blit_generic(unsigned char *dest, unsigned char *src,int pixels);
static void darken_sprite_c(unsigned char *dest, unsigned char *src, unsigned char *cmap, int widthoffset);
static void darken_sprite_c(unsigned char *dest, unsigned char *src, unsigned char *cmap, int widthoffset);


extern "C" void darken_asm_16(unsigned int *spr, unsigned char *darkmap, int pixels, unsigned short *lut);
extern "C" void darken_asm_16s(unsigned int *spr, unsigned int level, int pixels, unsigned short *lut);
extern "C" void darken_asm_32(unsigned int *spr, unsigned char *darkmap, int pixels);
extern "C" void darken_asm_32s(unsigned int *spr, unsigned int level, int pixels);

extern "C" void darken_asm_blit16(unsigned char *dest, unsigned char *src, int pixels);
extern "C" void darken_asm_blit32(unsigned char *dest, unsigned char *src, int pixels);
extern "C" void darken_asm_darksprite(unsigned char *dest, unsigned char *src, unsigned char *cmap, int widthoffset);
extern "C" void darken_asm_lightsprite(unsigned char *dest, unsigned char *src, unsigned char *cmap, int widthoffset);
extern "C" int ire_checkMMX();

// Code

void darken_init(int bpp)
{
int x,y,z;
// Choose default methods for BPP-independent code
lightning=lightning_generic_c;

#ifndef NO_ASM
GotMMX = ire_checkMMX();
if(!GotMMX)
	{
	ilog_printf("SIMD instructions DISABLED\n");
	}
else
	{
	ilog_printf("SIMD instructions enabled\n");
	}
#endif

// Choose a method for the shading

switch(bpp)
	{
	case 8:
	ithe_panic("8bpp is no longer supported!","");
	break;

	case 15:
	case 16:
	darken_16_init();
	darken = darken_16_c;
	darken_bitmap = darken_16s_c;
	merge_blit = darken_blit16_c;

#ifndef NO_ASM
	if(!ire_NOASM)
		{
		darken = darken_16_asm;
		darken_bitmap = darken_16s_asm;
		merge_blit = darken_asm_blit16;
		}
#endif
	lightning=lightning_16;
	break;

	case 24:
	darken = darken_generic_c;
	darken_bitmap = darkens_generic_c;
	merge_blit = darken_blit_generic;
	ilog_printf("WARNING: 24bpp not fully supported\n");
	break;

	case 32:
	darken = darken_32_c;
	darken_bitmap = darken_32s_c;
	merge_blit = darken_blit32_c;
#ifndef NO_ASM
	if(!ire_NOASM)
		{
		if(GotMMX)
			{
			darken = darken_32_asm;
			darken_bitmap = darken_32s_asm;
			}
		else
			{
			darken = darken_32_c;
			darken_bitmap = darken_32s_c;
			}
		merge_blit = darken_asm_blit32;
		}
#endif
	break;

	default:
	darken = darken_generic_c;
	darken_bitmap = darkens_generic_c;
	merge_blit = darken_blit_generic;
	ilog_printf("WARNING: unrecognised colour depth (%d bpp)\n",bpp);
	break;
	}

// Build colourmap for light sources
light_cmap = &light_cmapdata[0];
for(y=0;y<=255;y++)
	for(x=0;x<=255;x++)
		{
		z=y-x;
		if(z<0)
			z=0;
		light_cmap[(x<<8)+y]=(unsigned char)z;
		}

// Build colourmap for dark sources
dark_cmap = &dark_cmapdata[0];
for(y=0;y<=255;y++)
	for(x=0;x<=255;x++)
		{
		z=y+x;
		if(z>255)
			z=255;
		dark_cmap[(x<<8)+y]=(unsigned char)z;
		}

}


/*
 *  Build the lookup table for 15 and 16 bits per pixel
 */

void darken_16_init()
{
int ctr,ctr2,r,g,b;
IRECOLOUR col(0,0,0);

// Allocate lookup table

coltab16 = (unsigned short *)M_get(32,65536*sizeof(unsigned short));
for(ctr=0;ctr<32;ctr++)
	{
	for(ctr2=0;ctr2<65536;ctr2++)
		{
		col.Set(ctr2);
		r=col.r-(ctr<<3);
		if(r<0)
			r=0;
		g=col.g-(ctr<<3);
		if(g<0)
			g=0;
		b=col.b-(ctr<<3);
		if(b<0)
			b=0;
		col.Set(r,g,b);
		// 0 is used for Transparent in the rooftop shading code, so decide
		// what needs to be done.
		if(col.packed == 0)
			{
			if(ctr2 == 0)
				col.packed = 0; // Make it transparent
			else
				col.packed = 1; // very close to black
			}
		coltab16[(ctr*65536)+ctr2]= col.packed;
		}
	}

// Allocate lightning table
lightning16 = (unsigned short *)M_get(1,65536*sizeof(unsigned short));
for(ctr=0;ctr<65536;ctr++)
	{
	col.Set(ctr);
	r=col.r<<1;
	g=col.g<<1;
	b=col.b<<2;
	if(r>255)
		r=255;
	if(g>255)
		g=255;
	if(b>255)
		b=255;
	col.Set(r,g,b);
	lightning16[ctr]=col.packed;
	}
}


/*
 * Main darken functions
 */

void darken_16_c(IREBITMAP *image, IRELIGHTMAP *tab)
{
unsigned int x,y;
unsigned short *iptr=(unsigned short *)image->GetFramebuffer();
unsigned char *lptr=(unsigned char *)tab->GetFramebuffer();

y=tab->GetW()*tab->GetH();
for(x=0;x<y;x++)
	{
	*iptr = (coltab16[(((*lptr++)&0xf8)<<13)+*iptr]);
	iptr++;
	}
}

void darken_16_asm(IREBITMAP *image, IRELIGHTMAP *tab)
{
#ifndef NO_ASM
darken_asm_16((unsigned int *)image->GetFramebuffer(),(unsigned char *)tab->GetFramebuffer(),tab->GetW()*tab->GetH(),coltab16);
#endif
}

void darken_32_c(IREBITMAP *image, IRELIGHTMAP *tab)
{
unsigned int x,y;
unsigned char *iptr=(unsigned char *)image->GetFramebuffer();
unsigned char *lptr=(unsigned char *)tab->GetFramebuffer();

y=tab->GetW()*tab->GetH();
for(x=0;x<y;x++)
	{
	*iptr =  (light_cmap[(*lptr<<8)+*iptr]);	// R
	iptr++;
	*iptr =  (light_cmap[(*lptr<<8)+*iptr]);	// G
	iptr++;
	*iptr =  (light_cmap[(*lptr<<8)+*iptr]);	// B
	iptr++;
	*iptr =  (light_cmap[(*lptr<<8)+*iptr]);	// A
	iptr++;
	lptr++;
	}
}

void darken_32_asm(IREBITMAP *image, IRELIGHTMAP *tab)
{
#ifndef NO_ASM
darken_asm_32((unsigned int *)image->GetFramebuffer(),(unsigned char *)tab->GetFramebuffer(),tab->GetW()*tab->GetH());
#endif
}

void darken_generic_c(IREBITMAP *image, IRELIGHTMAP *tab)
{
int x,y,level,r,g,b,w,h;
IRECOLOUR col(0,0,0);

if(!tab || !image)
	return;
w=tab->GetW();
h=tab->GetH();

for(y=0;y<h;y++)
	for(x=0;x<w;x++)
		{
		image->GetPixel(x,y,&col);
		level = tab->GetPixel(x,y);
		r = col.r;
		g = col.g;
		b = col.b;

		r-= level;
		if(r<0)
			r=0;
		g-= level;
		if(g<0)
			g=0;
		b-= level;
		if(b<0)
			b=0;
		image->PutPixel(x,y,r,g,b);
		}
}


/*
 *	Lightning
 */

void lightning_generic_c(IREBITMAP *image)
{
int x,y,r,g,b,w,h;
IRECOLOUR col(0,0,0);

w=image->GetW();
h=image->GetH();
for(y=0;y<h;y++)
	for(x=0;x<w;x++)
		{
		image->GetPixel(x,y,&col);
		r = col.r<<1;
		g = col.g<<1;
		b = col.b<<2;
		if(r>255)
			r=255;
		if(g>255)
			g=255;
		if(b>255)
			b=255;
		image->PutPixel(x,y,r,g,b);
		}
}


void lightning_16(IREBITMAP *image)
{
unsigned int x,y;
unsigned short *iptr=(unsigned short *)image->GetFramebuffer();

y=image->GetW()*image->GetH();
for(x=0;x<y;x++)
	{
	*iptr = (lightning16[*iptr]);
	iptr++;
	}
}

/*
 * 	Compositing functions
 */

void darken_blit16_c(unsigned char *dest, unsigned char *src, int len)
{
int ctr;
unsigned short *d=(unsigned short *)dest;
unsigned short *s=(unsigned short *)src;

for(ctr=0;ctr<len;ctr++)
	{
	if(*s)
		*d=*s;
	s++;
	d++;
	}
}

void darken_blit32_c(unsigned char *dest, unsigned char *src, int len)
{
int ctr;
unsigned int *d=(unsigned int *)dest;
unsigned int *s=(unsigned int *)src;

for(ctr=0;ctr<len;ctr++)
	{
	if(*s)
		*d=*s;
	s++;
	d++;
	}
}

void darken_blit_generic(unsigned char *dest, unsigned char *src, int len)
{
// Do Nothing
}


/*
 *	Darken solid
 */


void darken_16s_c(IREBITMAP *image, unsigned int level)
{
unsigned int x,y;
unsigned short *iptr=(unsigned short *)image->GetFramebuffer();
level &=0xf8;
y=image->GetW()*image->GetH();
for(x=0;x<y;x++)
	{
	*iptr = (coltab16[(level<<13)+*iptr]);
	iptr++;
	}
}

void darken_16s_asm(IREBITMAP *image, unsigned int level)
{
#ifndef NO_ASM
darken_asm_16s((unsigned int *)image->GetFramebuffer(),level&0xff,image->GetW()*image->GetH(),coltab16);
#endif
}

void darken_32s_c(IREBITMAP *image, unsigned int level)
{
unsigned int x,y;
unsigned char *iptr=(unsigned char *)image->GetFramebuffer();
unsigned int *l = (unsigned int *)iptr;
level &= 0xff;
unsigned char *levelptr = light_cmap + (level<<8);

y=image->GetW()*image->GetH();
for(x=0;x<y;x++)
	{
	// If it was zero before, it's supposed to be translucent for the colour
	// separation used by the roof overlay projector
	if(*l == 0)
		{
		iptr+=4;
		}
	else
		{
		// Otherwise darken it and set the alpha to make sure it's non-zero
		*iptr =  levelptr[*iptr];	// R
		iptr++;
		*iptr =  levelptr[*iptr];	// G
//		*iptr =  (light_cmap.data[level][*iptr]);	// G
		iptr++;
		*iptr =  levelptr[*iptr];	// B
		iptr++;
		*iptr =  0x01;							// A
		iptr++;
		}
	l++;
	}
}

void darken_32s_asm(IREBITMAP *image, unsigned int level)
{
#ifndef NO_ASM
darken_asm_32s((unsigned int *)image->GetFramebuffer(),level,image->GetW()*image->GetH());
#endif
}


void darkens_generic_c(IREBITMAP *image, unsigned int level)
{
int x,y,r,g,b;
IRECOLOUR col(0,0,0);
int w=image->GetW();
int h=image->GetH();

for(y=0;y<h;y++)
	for(x=0;x<w;x++)
		{
		image->GetPixel(x,y,&col);
		r = col.r;
		g = col.g;
		b = col.b;

		r-= level;
		if(r<0)
			r=0;
		g-= level;
		if(g<0)
			g=0;
		b-= level;
		if(b<0)
			b=0;
		image->PutPixel(x,y,r,g,b);
		}
}


/*
 *	Sprite darkening and illuminating
 */


// Put a sprite using the colourmap

void darken_sprite_c(unsigned char *out, unsigned char *in, unsigned char *cmap, int woff)
{
int ctr,ctr2;

for(ctr=0;ctr<32;ctr++)
    {
    for(ctr2=0;ctr2<32;ctr2++)
	{
        *out = cmap[((*in++)<<8) + *out];
	out++;
	}
    out+=woff;
    }
}


void darken_darksprite(int x,int y, int scrw, int scrh, unsigned char *out, unsigned char *in)
{
int woff;

if(x<0 || x+31>scrw)
	return;
if(y<0 || y+31>scrh)
	return;

out = out+(y*scrw)+x;
woff = scrw-32;

#ifdef NO_ASM
	darken_sprite_c(out,in,dark_cmap,woff);
#else
if(!GotMMX)
	darken_sprite_c(out,in,dark_cmap,woff);
else
	darken_asm_darksprite(out,in,dark_cmap,woff);	// If we don't have MMX or something, we'll need the colourmap
#endif
}


void darken_lightsprite(int x,int y, int scrw, int scrh, unsigned char *out, unsigned char *in)
{
int woff;

if(x<0 || x+31>scrw)
	return;
if(y<0 || y+31>scrh)
	return;

out = out+(y*scrw)+x;
woff = scrw-32;


#ifdef NO_ASM
darken_sprite_c(out,in,light_cmap,woff);
#else
if(!GotMMX)
	darken_sprite_c(out,in,light_cmap,woff);
else
	darken_asm_lightsprite(out,in,light_cmap,woff);	// If we don't have MMX or something, we'll need the colourmap
#endif
}

