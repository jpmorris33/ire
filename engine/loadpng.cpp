/* $Id: loadpng.c,v 1.1 2004/04/26 20:24:47 mlefebvr Exp $ */

#include <stdio.h>
#include <string.h>
#include <png.h>

#include "graphics/iregraph.hpp"
#include "ithelib.h"
#include "media.hpp"	/* IRE transparent colors */

#ifdef _WIN32
#define snprintf _snprintf		// God I hate Microsoft
#endif


IREBITMAP *iload_png(char *filename)
{
IFILE *img_fp;
#ifdef _WIN32
	#define signature_size 8
#else
	const int signature_size = 8; /* Nb of magic bytes in a png signature */
#endif

unsigned char magic[signature_size];

char *img_pixels = NULL; /* Pixels stream from the PNG image */


/*
 * png variables
 */
png_structp png_ptr = NULL;
png_infop info_ptr = NULL;

int color_type=0;      	/* Type of the png image */
int bit_depth=0;  	/* bits per _channel_ (red, green, blue, alpha) */ 
png_byte channels=0;	/* nb of channels. channels * bit_depth = bpp */

png_uint_32 width=0, height=0;

png_colorp img_palette; /* palette of the png file, if any */
int num_palette=0;	/* number of colors in the png file palette, if any */ 
png_bytep trans;	/* array of transparencies attached to the palette */
int num_trans=0;  	/* number of transparency entries */
png_color_16p trans_values; /* RGB samples for the transparent color (>8bpp) */

/* array of pointers to scanlines, needed to load the image
 * matches the lines of img_pixels */
png_bytep *row_pointers = NULL;
png_uint_32 rowbytes=0; 	/* size of a row in bytes */


/*
 * variables
 */
IREBITMAP *bmp = NULL; 	/* bitmap */
//PALETTE bmp_palette;   	/* Palette associated to the bitmap */
unsigned char bmp_palette[768];


/*
 * Open png file and check signature
 */
img_fp = iopen(filename); /* handles errors */

memset (magic, '\0', sizeof magic);
iread(magic, signature_size, img_fp);

/* Check whether the signature matches a png file */
if (png_sig_cmp(magic, 0, signature_size) != 0) {
	iclose(img_fp);
	ithe_panic("PNG: Not a png file", filename);
}


/*
 * Create the main png variables
 */
png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                 (png_voidp) NULL,
                                 (png_error_ptr) NULL,
                                 (png_error_ptr) NULL);

if (png_ptr == NULL) { /* error */
	iclose(img_fp);
	ithe_panic("PNG: png_create_read_struct() error", filename);
}

info_ptr = png_create_info_struct(png_ptr);
if (info_ptr == NULL) { /* error */
	iclose(img_fp);
	png_destroy_read_struct(&png_ptr, NULL, NULL);
	ithe_panic("PNG: png_create_info_struct() error", filename);
}

/* libpng requires this */
if (setjmp(png_jmpbuf(png_ptr)) != 0) { /* error */
	iclose(img_fp);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	ithe_panic("PNG: setjmp() failed", filename);
}

/* Set the png input stream */
png_init_io(png_ptr, img_fp->fp); /*Caveat to the IRE fs funcs.?*/

/* Tell libpng we have already read the signature of the png file */
png_set_sig_bytes(png_ptr, signature_size);

/* Get info header from the png image */
png_read_info(png_ptr, info_ptr);


/*
 * Transform image
 */
bit_depth = png_get_bit_depth(png_ptr, info_ptr);

/* Strip 16 bits per channel to 8 bits */
if (bit_depth == 16) {
	png_set_strip_16(png_ptr);
}

if (bit_depth < 8) {
	png_set_packing(png_ptr); /* At least 1 byte per pixel */
}


/* Get the image type:
 * grayscale, grayscale + alpha, paletted, RGB, RGBA */
color_type = png_get_color_type(png_ptr, info_ptr);


/* I don't want to deal with grayscale images */
if ((color_type == PNG_COLOR_TYPE_GRAY)
   || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) {
	png_set_gray_to_rgb(png_ptr);
}


/* Update the "info" structure of the png image */
png_read_update_info(png_ptr, info_ptr);

/* Get updated informations about the image */ 
png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
             NULL, NULL, NULL);


/*
 * Simple transparency (i.e without Alpha channel)
 */
if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
	png_get_tRNS(png_ptr, info_ptr,
	             &trans /* array of transparent entries */,
	             &num_trans /* nb of transparent entries */,
	             &trans_values);
} else {
	/* Sanity */
	trans = NULL;
	num_trans = 0;
	trans_values = NULL;
}

/* Create the bitmap */
bmp = MakeIREBITMAP(width, height);
if (bmp == NULL) {
	ithe_panic("PNG: failed to create bitmap", filename);
}


/* Get the number of channels in the image
 * _libpng_ says:
 * (valid values are 1 (GRAY, PALETTE), 2 (GRAY_ALPHA),
 * 3 (RGB), 4 (RGB_ALPHA or RGB + filler byte))
 */
channels = png_get_channels(png_ptr, info_ptr);


/*
 * Allocate a memory segment to store the image
 */
rowbytes = png_get_rowbytes(png_ptr, info_ptr); /* nb of bytes per row */

img_pixels = (char *) M_get(1, height * rowbytes);

/* Allocate an array of row pointers, as required by libpng */
row_pointers = (png_bytep *) M_get(height, sizeof (png_bytep));

/* Match it to the data segment previously allocated */
{
unsigned int row;
for (row = 0; row < height; row++) {
	row_pointers[row] = (png_bytep) (img_pixels + (row * rowbytes));
}
}

/* Load the whole (transformed) png image in memory */
png_read_image(png_ptr, row_pointers);

png_read_end(png_ptr, NULL); /* required, but useless */
iclose(img_fp);


/*
 * Fill in the bitmap
 */
if (channels == 1) /* Paletted image */ {
	/*
	 * Pixels are byte-length indexes to the palette.
	 */
	int i,i3;
	unsigned int x, y;
	unsigned char pixel;

	/*
	 * Get the palette
	 */
	png_get_PLTE(png_ptr, info_ptr, &img_palette, &num_palette);

	i3=0;
	/* Transparent indexes first */
	for (i = 0; i < num_trans; i++) {
		bmp_palette[i3] = ire_transparent_r;
		bmp_palette[i3+1] = ire_transparent_g;
		bmp_palette[i3+2] = ire_transparent_b;
		i3+=3;
	}
	/* go on with other indexes */
	for (; i < num_palette; i++) {
		bmp_palette[i3] = (unsigned char) img_palette[i].red;
		bmp_palette[i3+1] = (unsigned char) img_palette[i].green;
		bmp_palette[i3+2] = (unsigned char) img_palette[i].blue;
		i3+=3;
	}


	/*
	 * Fill in the pixels grid of the bitmap
	 */
	for (y = 0; y < height; y ++) {
		for (x = 0; x < width; x ++) {
			pixel = (unsigned char) row_pointers[y][x];
			i3=pixel+pixel+pixel;
			bmp->PutPixel(x, y, bmp_palette[i3],bmp_palette[i3+1],bmp_palette[i3+2]); 
		}
	}
} else if (channels >= 3) /* RGB or RGBA image */ {
	/*
	 * Pixels are a stream of RGB or RGBA samples, each byte-length.
	 */
	unsigned int x,y;
	unsigned char *pixel;

	/* RGB offsets relatively to a pixel starting position */
	const int red_offset   = 0,
	          green_offset = 1,
	          blue_offset  = 2,
	          alpha_offset = 3;

	/* RGB samples of a pixel color */
	int red_sample, green_sample, blue_sample;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			/*
			 * The pixel info is at:
			 *   img_pixels + (y * rowbytes) + (x * channels)
			 * also written:
			 *   row_pointers[y] + (x * channels)
			 */
			pixel = (unsigned char *) 
			        (row_pointers[y] + (x * channels));
	
			/*
			 * Translate transparent pixels to IRE transparency
			 * if there is an alpha channel or a transparent color
			 * in the image.
			 */

			/* Transparency through alpha channel
			 * The alpha channel in a PNG image means a level
			 * of opacity, ranging from 0 (transparency) to 255.
			 *
			 * From _this_ program point of view,
			 * a pixel can be either transparent or opaque,
			 * translucency is not supported.
			 * We arbitrarily use the value "0" as the only 
			 * meaning of transparency; any other value will
			 * be taken as _opaque_.
			 */ 
			if ((channels == 4)
			   && (*(pixel + alpha_offset) == 0)) {
				bmp->PutPixel(x,y,ire_transparent);
				continue; /* next pixel */
			}

			red_sample   = *(pixel + red_offset),
			green_sample = *(pixel + green_offset),
			blue_sample  = *(pixel + blue_offset);

			/* Single transparent color 
			 * Note:
			 * Maybe it should be an "else if" relative to the
			 * alpha channel test, I don't know if both an alpha 
			 * channel _and_ a single transparent color are 
			 * possible in a png image
			 */
			if ((trans_values != NULL) 
			   && (trans_values->red == red_sample)
			   && (trans_values->green == green_sample)
			   && (trans_values->blue == blue_sample)) {
				bmp->PutPixel(x,y,ire_transparent);
				continue; /* next pixel */
			}
			
			/*
			 * Set the pixel in the bitmap
			 */
			bmp->PutPixel(x,y,red_sample,green_sample,blue_sample);
		}
	}
} else /* a freaky png image */ {
	char msg[80];
#ifndef __DJGPP__
	snprintf(msg, sizeof msg,
	         "PNG: strange number of channels: %d in file", channels); 
#else
	sprintf(msg,"PNG: strange number of channels: %d in file", channels);
#endif

	/* Free memory */
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	M_free(row_pointers);
	M_free(img_pixels);

	ithe_panic(msg, filename);
} /* if (channels...) */


/* Free memory */
png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
M_free(row_pointers);
M_free(img_pixels);

return bmp;
}
