//
//	Allegro 4.x bitmap implementation
//

#include <stdio.h>
#include "ibitmap.hpp"

extern int _IRE_BPP;
extern int _IRE_MouseActivated;

extern "C" void fatline(BITMAP *bmp, int x, int y, int col);
static int fatline_radius=1;


//
//  Bootstrap
//

IREBITMAP *MakeIREBITMAP(int w, int h)
{
A4BMP *spr = new A4BMP(w,h);
return (IREBITMAP *)spr;
}

IREBITMAP *MakeIREBITMAP(int w, int h, int bpp)
{
A4BMP *spr = new A4BMP(w,h,bpp);
return (IREBITMAP *)spr;
}

void IRE_SetPixelFormat(int bpp)
{
// SDL needs this, we don't
}


//
//	Constructor
//
	
A4BMP::A4BMP(int w, int h) : IREBITMAP(w,h)
{
width=w;
height=h;
bytesperpixel=_IRE_BPP/8;
img = create_bitmap(w,h);
if(img)
	clear_to_color(img,0);
}

//
//	Multi-bpp constructor
//

A4BMP::A4BMP(int w, int h, int bpp) : IREBITMAP(w,h,bpp)
{
width=w;
height=h;
bytesperpixel=bpp/8;

select_palette(default_palette);

img = create_bitmap_ex(bpp,w,h);
if(img)
	clear_to_color(img,0);
else
	{
	printf("Create failed for %dx%d, %d\n",w,h,bpp);
	abort();
	}
unselect_palette();

}

//
//  Destructor
//

A4BMP::~A4BMP()
{
if(img)
	destroy_bitmap(img);
img=NULL;
}

//
//  Clear to a given colour
//

void A4BMP::Clear(IRECOLOUR *col)
{
if(!img || !col)
	return;
clear_to_color(img,col->packed);
}

//
//  Regular transparent drawing
//

void A4BMP::Draw(IREBITMAP *dest, int x, int y)
{
if(!img || !dest)
	return;
draw_sprite(((A4BMP *)dest)->img,img,x,y);
}

//
//  Additive drawing
//

void A4BMP::DrawAddition(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest)
	return;
A4BMP *destptr=(A4BMP *)dest;

set_add_blender(0,0,0,level);
draw_trans_sprite(destptr->img,img,x,y);
}

//
//  Alpha-blended drawing
//

void A4BMP::DrawAlpha(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest)
	return;
A4BMP *destptr=(A4BMP *)dest;

set_trans_blender(0,0,0,level);
draw_trans_sprite(destptr->img,img,x,y);
}

//
//  Dissolved drawing
//

void A4BMP::DrawDissolve(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest)
	return;
A4BMP *destptr=(A4BMP *)dest;

set_dissolve_blender(0,0,0,level);
draw_trans_sprite(destptr->img,img,x,y);
}

//
//  Shadow drawing
//

void A4BMP::DrawShadow(IREBITMAP *dest, int x, int y, int level)
{
if(!img || !dest)
	return;
A4BMP *destptr=(A4BMP *)dest;

set_burn_blender(0,0,0,level);
draw_trans_sprite(destptr->img,img,x,y);
}

//
//  Non-transparent drawing
//

void A4BMP::DrawSolid(IREBITMAP *dest, int x, int y)
{
if(!img || !dest)
	return;

A4BMP *destptr=(A4BMP *)dest;
blit(img,destptr->img,0,0,x,y,width,height);
}

void A4BMP::DrawStretch(IREBITMAP *dest, int x, int y, int w, int h)
{
if(!img || !dest)
	return;

A4BMP *destptr=(A4BMP *)dest;
stretch_blit(img,destptr->img,0,0,width,height,x,y,w,h);
}


//
//  Fillrect
//

void A4BMP::FillRect(int x, int y, int w, int h, IRECOLOUR *col)
{
if(!img || !col)
	return;
rectfill(img,x,y,x+w,y+h,col->packed);
}

void A4BMP::FillRect(int x, int y, int w, int h, int r, int g, int b)
{
if(!img)
	return;
rectfill(img,x,y,x+w,y+h,makecol(r,g,b));
}

//
//  Circle
//

void A4BMP::Circle(int x, int y, int radius, IRECOLOUR *col)
{
if(!img || !col)
	return;
circlefill(img,x,y,radius,col->packed);
}

void A4BMP::Circle(int x, int y, int radius, int r, int g, int b)
{
if(!img)
	return;
circlefill(img,x,y,radius,makecol(r,g,b));
}

//
//  Line
//

void A4BMP::Line(int x, int y, int x2, int y2, IRECOLOUR *col)
{
if(!img || !col)
	return;
line(img,x,y,x2,y2,col->packed);
}

void A4BMP::Line(int x, int y, int x2, int y2, int r, int g, int b)
{
if(!img)
	return;
line(img,x,y,x2,y2,makecol(r,g,b));
}

//
//  Thick line
//

void A4BMP::ThickLine(int x, int y, int x2, int y2, int radius, IRECOLOUR *col)
{
if(!img || !col)
	return;
fatline_radius=radius;
do_line(img,x,y,x2,y2,col->packed,fatline);
}

void A4BMP::ThickLine(int x, int y, int x2, int y2, int radius, int r, int g, int b)
{
if(!img)
	return;
fatline_radius=radius;
do_line(img,x,y,x2,y2,makecol(r,g,b),fatline);
}


//
//  Polygon
//

void A4BMP::Polygon(int count, int *vertices, IRECOLOUR *col)
{
if(!img || !vertices || count < 2 || !col)
	return;
polygon(img,count,vertices,col->packed);  
}

void A4BMP::Polygon(int count, int *vertices, int r, int g, int b)
{
if(!img || !vertices || count < 2)
	return;
polygon(img,count,vertices,makecol(r,g,b));  
}


void A4BMP::SetBrushmode(int mode, int alpha)
{
BlenderMode=mode;
BlenderAlpha=alpha;

SetBlender();
}

void A4BMP::SetBlender()
{
switch(BlenderMode)
	{
	case ALPHA_TRANS:
	set_trans_blender(BlenderAlpha,BlenderAlpha,BlenderAlpha,BlenderAlpha);
	drawing_mode(DRAW_MODE_TRANS,NULL,0,0);
	break;

	case ALPHA_ADD:
	set_add_blender(BlenderAlpha,BlenderAlpha,BlenderAlpha,BlenderAlpha);
	drawing_mode(DRAW_MODE_TRANS,NULL,0,0);
	break;

	case ALPHA_SUBTRACT:
	set_difference_blender(BlenderAlpha,BlenderAlpha,BlenderAlpha,BlenderAlpha);
	drawing_mode(DRAW_MODE_TRANS,NULL,0,0);
	break;

	case ALPHA_INVERT:
	set_invert_blender(BlenderAlpha,BlenderAlpha,BlenderAlpha,BlenderAlpha);
	drawing_mode(DRAW_MODE_TRANS,NULL,0,0);
	break;

	case ALPHA_DISSOLVE:
	set_dissolve_blender(BlenderAlpha,BlenderAlpha,BlenderAlpha,BlenderAlpha);
	drawing_mode(DRAW_MODE_TRANS,NULL,0,0);
	break;

	default:
//	tfx_Alpha=255;
	set_trans_blender(255,255,255,255);
	drawing_mode(DRAW_MODE_SOLID,NULL,0,0);
	break;
	};
}


//
//  Acquire from an existing bitmap
//

void A4BMP::Get(IREBITMAP *src, int x, int y)
{
if(!img || !src)
	return;
A4BMP *srcptr=(A4BMP *)src;
blit(srcptr->img,img,x,y,0,0,width,height);
}

//
//  Get a pixel
//

int A4BMP::GetPixel(int x, int y)
{
if(!img)
	return -1;
return getpixel(img,x,y);
}

int A4BMP::GetPixel(int x, int y, IRECOLOUR *col)
{
if(!img || !col)
	return 0;
*col = getpixel(img,x,y);
return 1;
}

//
//  Set a pixel
//

void A4BMP::PutPixel(int x, int y, IRECOLOUR *col)
{
if(!img || !col)
	return;
putpixel(img,x,y,col->packed);
}

void A4BMP::PutPixel(int x, int y, int r, int g, int b)
{
if(!img)
	return;
putpixel(img,x,y,makecol(r,g,b));
}

void A4BMP::HLine(int x, int y, int w, IRECOLOUR *col)
{
if(!img || !col)
	return;
line(img,x,y,x+w,y,col->packed);
}

void A4BMP::HLine(int x, int y, int w, int r, int g, int b)
{
if(!img)
	return;
line(img,x,y,x+w,y,makecol(r,g,b));
}

//
//  Render to physical screen at 0,0 - this is intended for the swapscreen bitmap
//

void A4BMP::Render()
{
if(!img || !screen)
	return;
if(_IRE_MouseActivated)
	{
	show_mouse(img);
	}
blit(img,screen,0,0,0,0,width,height);
if(_IRE_MouseActivated)
	{
	show_mouse(screen);
	}
}

//
//
//

void A4BMP::ShowMouse()
{
if(img)
	show_mouse(img);
}

void A4BMP::HideMouse()
{
show_mouse(NULL);
}


//
//  Get framebuffer
//

unsigned char *A4BMP::GetFramebuffer()
{
if(!img)
	return NULL;
return img->line[0];
}


void A4BMP::ReleaseFramebuffer(unsigned char *ptr)
{
// SDL needs the framebuffer released afterwards
}



int A4BMP::SaveBMP(const char *filename)
{
RGB pal[768];
generate_332_palette(pal);
save_bitmap(filename,img,pal);
return 1;
}


int A4BMP::GetW()
{
return width;
}


int A4BMP::GetH()
{
return height;
}


int A4BMP::GetDepth()
{
if(!img)
	return 0;
return bitmap_color_depth(img);
}


//
//	Helper function
//

void fatline(BITMAP *bmp, int x, int y, int col)
{
circle(bmp,x,y,fatline_radius,col);
}
