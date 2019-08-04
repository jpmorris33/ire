//
//	Allegro 4.x sprite implementation
//

#include <stdio.h>
#include "ibitmap.hpp"

#define HEADER_ALLEGRO 0x344c4741 // AGL4

class A4SPR : public IRESPRITE
	{
	public:
		A4SPR(IREBITMAP *src);
//		A4SPR(const char *filename);
		A4SPR(const void *data, int len);
		~A4SPR();
		void Draw(IREBITMAP *dest, int x, int y);
		void DrawAddition(IREBITMAP *dest, int x, int y, int level);
		void DrawAlpha(IREBITMAP *dest, int x, int y, int level);
		void DrawDissolve(IREBITMAP *dest, int x, int y, int level);
		void DrawShadow(IREBITMAP *dest, int x, int y, int level);
		void DrawSolid(IREBITMAP *dest, int x, int y);
		void DrawInverted(IREBITMAP *dest, int x, int y);
//                int Save(const char *filename);
		void *SaveBuffer(int *len);
		void FreeSaveBuffer(void *buffer);
                unsigned int GetAverageCol();
                void SetAverageCol(unsigned int acw);
		int GetW();
		int GetH();
		
	private:
		RLE_SPRITE *img;
		unsigned int avcol;
	};

	
//
//  Bootstrap
//

IRESPRITE *MakeIRESPRITE(IREBITMAP *bmp)
{
A4SPR *spr = new A4SPR(bmp);
return (IRESPRITE *)spr;
}

IRESPRITE *MakeIRESPRITE(const void *data, int len)
{
unsigned int header=0;
if(!data || len<4)
	return NULL;
memcpy(&header,data,4);
if(header != HEADER_ALLEGRO)
	return NULL;

A4SPR *spr = new A4SPR(data,len);
return (IRESPRITE *)spr;
}


// Read a 32-bit word

static  unsigned int getwptr(unsigned char **ptr)	{
	unsigned char *bptr=*ptr;
	unsigned char byte;
	unsigned int out=0;
	
	byte=(*bptr++)&0xff;
	out = byte;
	byte=(*bptr++)&0xff;
	out |= byte<<8;
	byte=(*bptr++)&0xff;
	out |= byte<<16;
	byte=(*bptr++)&0xff;
	out |= byte<<24;
	*ptr=bptr;
	
	return out;
}

// Write a 32-bit word

static  void putwptr(unsigned char **ptr, unsigned int word)	{
	unsigned char *bptr=*ptr;
	*bptr++ = (word&0xff);
	*bptr++ = ((word>>8)&0xff);
	*bptr++ = ((word>>16)&0xff);
	*bptr++ = ((word>>24)&0xff);

	*ptr=bptr;
}


	
//
//	Constructor
//
	
A4SPR::A4SPR(IREBITMAP *src) : IRESPRITE(src)
{
if(!src)
	return;

A4BMP *srcptr=(A4BMP *)src;
img = get_rle_sprite(srcptr->img);
avcol=0;
}

/*
//
//	Constructor to read in from a file
//

A4SPR::A4SPR(const char *fname) : IRESPRITE(fname)
{
if(!fname)
	return;

int sz = 0;
FILE *fp;

fp = fopen(fname,"rb");
if(!fp)
	return;

sz = getw(fp);
if(sz < 1)
	{
	fclose(fp);
	return;
	}

img = (RLE_SPRITE *)calloc(1,sizeof(RLE_SPRITE) + sz);
if(!img)
	{
	fclose(fp);
	return;
	}

img->color_depth = getw(fp);
img->w = getw(fp);
img->h = getw(fp);
img->size = sz;

sz=fread(img->dat, 1, sz, fp);

// Get average pixel colour, if a pointer is given
avcol=getw(fp);

fclose(fp);
}
*/

// Constructor to build from cache data

A4SPR::A4SPR(const void *datafile, int len) : IRESPRITE(datafile,len)
{
unsigned char *dptr=(unsigned char *)datafile; // Dodgy, but we aren't actually altering it
unsigned int sz=0;
img=NULL;

if(!datafile)
	return;

sz = getwptr(&dptr);
if(sz != HEADER_ALLEGRO)
	return;

sz = getwptr(&dptr);
img = (RLE_SPRITE *)calloc(1,sizeof(RLE_SPRITE)+sz); // Allegro dumps the image data into the end of the structure
if(!img)
	return;

img->color_depth = getwptr(&dptr);
img->w = getwptr(&dptr);
img->h = getwptr(&dptr);
img->size = sz;
memcpy(img->dat,dptr,sz);
dptr += sz;

// Get average pixel colour, if a pointer is given
avcol=getwptr(&dptr);
}


//
//  Destructor
//

A4SPR::~A4SPR()
{
if(img)
	destroy_rle_sprite(img);
img=NULL;
}

//
//  Regular transparent drawing
//

void A4SPR::Draw(IREBITMAP *dest, int x, int y)
{
if(!img || !dest)
	return;
draw_rle_sprite(((A4BMP *)dest)->img,img,x,y);
}

//
//  Additive blended drawing
//

void A4SPR::DrawAddition(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest)
	return;
A4BMP *destptr=(A4BMP *)dest;

set_add_blender(0,0,0,level);
draw_trans_rle_sprite(destptr->img,img,x,y);
}

//
//  Alpha-blended drawing
//

void A4SPR::DrawAlpha(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest)
	return;
A4BMP *destptr=(A4BMP *)dest;

set_trans_blender(0,0,0,level);
draw_trans_rle_sprite(destptr->img,img,x,y);
}

//
//  Dissolved drawing
//

void A4SPR::DrawDissolve(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest)
	return;
A4BMP *destptr=(A4BMP *)dest;

set_dissolve_blender(0,0,0,level);
draw_trans_rle_sprite(destptr->img,img,x,y);
}

//
//  Shadow drawing
//

void A4SPR::DrawShadow(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest)
	return;
A4BMP *destptr=(A4BMP *)dest;

set_burn_blender(0,0,0,level);
draw_trans_rle_sprite(destptr->img,img,x,y);
}

//
//  Non-transparent drawing
//

void A4SPR::DrawSolid(IREBITMAP *dest, int x, int y)
{
if(!img || !dest)
	return;

A4BMP *destptr=(A4BMP *)dest;
draw_rle_sprite(destptr->img,img,x,y);
}

//
//  Inversed drawing
//

void A4SPR::DrawInverted(IREBITMAP *dest, int x, int y)
{
if(!img || !dest)
	return;
A4BMP *destptr=(A4BMP *)dest;
set_invert_blender(0,0,0,255);
draw_trans_rle_sprite(destptr->img,img,x,y);
}




unsigned int A4SPR::GetAverageCol()
{
return avcol;
}

void A4SPR::SetAverageCol(unsigned int acw)
{
avcol=acw;
}

void *A4SPR::SaveBuffer(int *len)
{
unsigned char *outbuffer,*ptr;
unsigned int sz=0;
if(!len|| !img)
	return 0;

sz = 4+4+4+4+4+(img->size)+4; // Header, image size, depth, w, h, image data, avcol
*len=sz;
outbuffer = (unsigned char *)calloc(1,sz);
if(!outbuffer)
	return 0;
ptr=outbuffer;

putwptr(&ptr,HEADER_ALLEGRO);
putwptr(&ptr,img->size);
putwptr(&ptr,img->color_depth);
putwptr(&ptr,img->w);
putwptr(&ptr,img->h);
memcpy(ptr,img->dat,img->size);
ptr += img->size;
putwptr(&ptr,avcol);
return outbuffer;
}

void A4SPR::FreeSaveBuffer(void *buffer)
{
if(buffer)
	free(buffer);
}


int A4SPR::GetW()
{
if(!img)
	return 0;
return img->w;
}

int A4SPR::GetH()
{
if(!img)
	return 0;
return img->h;
}
