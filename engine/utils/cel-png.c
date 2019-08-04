/* $Id: cel-png.c,v 1.2 2004/09/11 21:21:03 mlefebvr Exp mlefebvr $
 *
 * Tool for the IRE engine.
 *
 * Convert a ".cel" image file to a paletted png one, and conversely.
 * Most of the knowledge and code about the ".cel" decoder comes
 * from the CEL image loader source file.
 *
 * Link against libpng library: "-lpng"
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <png.h>	/* PNG interface */



/* ------------------------------------------
 * Convert a CEL file to a [paletted] PNG one
 * ------------------------------------------
 */
int cel2png(char *cel_file	/*IN*/, 
            char *png_file	/*OUT*/,
            int set_trans_bg	/* bool IN; set it to 1 to make the first
                             	 * color of the cel palette appear transparent
                             	 * in the png image */)
{
FILE *cel_fp; 	/* cel file */
FILE *png_fp;	/* png file */

unsigned char *buf = NULL;
unsigned char *ptr;
png_uint_32 w, h;

/*
 * png variables
 */
png_structp png_ptr = NULL;
png_infop info_ptr = NULL;

png_bytep *row_pointers = NULL; /* array of pointers to scanlines */

png_color *palette; 	/* palette of the png file */
int num_palette;	/* number of colors in the png file palette */ 


/*
 * Read the whole file
 */
{
struct stat sbuf;
unsigned long filesize;

if (stat(cel_file, &sbuf) != 0) {
	fprintf(stderr, "Couldn't stat file %s: %s\n",
	        cel_file, strerror(errno));
	return 2;
}

filesize = (unsigned long) sbuf.st_size;

cel_fp = fopen(cel_file, "rb");
if (cel_fp == NULL) {
	fprintf(stderr, "Couldn't open %s: %s\n", cel_file, strerror(errno));
	return 2;
}

buf = (unsigned char *) malloc (filesize);
if (buf == NULL) {
	fprintf(stderr, "malloc CEL buffer: %s\n", strerror(errno));
	fclose(cel_fp);
	return 2;
}

if (fread(buf, 1, filesize, cel_fp) < filesize) {
	fprintf(stderr, "Couldn't read %s\n", cel_file);
	fclose(cel_fp);
	if (buf != NULL) { free(buf); }
	return 2;
}

fclose(cel_fp);
}


/*
 * Check file signature
 */
if ((buf[0] != 0x19) || (buf[1] != 0x91)) {
	fprintf(stderr, "CEL header invalid for image %s\n",
	        cel_file);
	return 2;
}

/* Read image width and height, 2 bytes each,
 * stored Less Significant Byte first */
w = buf[2] + (buf[3] << 8);
h = buf[4] + (buf[5] << 8);



/*
 * Create the libpng structures
 */
png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                  NULL /* user_error_ptr */,
                                  NULL /* user_error_fn */,
                                  NULL /* user_warning_fn */);
if (png_ptr == NULL) {
	fprintf (stderr, "png_create_write_struct() error\n");
	return 2;
}

info_ptr = png_create_info_struct(png_ptr);
if (info_ptr == NULL) {
	png_destroy_write_struct(&png_ptr, NULL);
	fprintf (stderr, "png_create_info_struct() error\n");
	return 2;
}

if (setjmp(png_jmpbuf(png_ptr)) != 0) {
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fprintf (stderr, "png_jmpbuf() error\n");
	return 2;
}

png_fp = fopen(png_file, "wb");
if (png_fp == NULL) {
	fprintf(stderr, "Couldn't open %s: %s\n", png_file, strerror(errno));
	return 2;
}

png_init_io(png_ptr, png_fp);


/*
 * Get palette from the CEL file (256 entries)
 */
num_palette = 256;
palette = (png_color *) malloc(num_palette * sizeof(png_color));
if (palette == NULL) {
	fprintf(stderr, "Couldn't allocate memory for the PNG palette: %s\n",
	        strerror(errno));
	return 2;
}

ptr = &buf[32];

{
int i;
for(i = 0; i < num_palette; i++) {
	/* Color values in .cel files range [0..63]
	 * in a png file, they range [0..255]
	 * We "enhance" the .cel colors by multiplying them by 4,
	 * or they will look dark in a png image. */
	palette[i].red   = (png_byte) (*ptr++ * 4);
	palette[i].green = (png_byte) (*ptr++ * 4);
	palette[i].blue  = (png_byte) (*ptr++ * 4);
}
}


/*
 * Get pixels
 */
ptr = &buf[800];

/* Allocate row pointers (freed automatically if "png_malloc()" used) */
row_pointers = png_malloc(png_ptr, h * sizeof(png_bytep));
if (row_pointers == NULL) {
	fprintf(stderr, "Couldn't allocate memory for the PNG row pointers\n");
	exit(2);
}

/* Make the row pointers match the CEL pixel stream */
{
int row;
for (row = 0; row < h; row++) {
	row_pointers[row] = (png_bytep) (ptr + row * w);
}
}

png_set_rows(png_ptr, info_ptr, row_pointers);


/*
 * Set png info struct
 */
png_set_IHDR(png_ptr, info_ptr,
             w, h, 8, PNG_COLOR_TYPE_PALETTE,
             PNG_INTERLACE_NONE,
             PNG_COMPRESSION_TYPE_DEFAULT,
             PNG_FILTER_TYPE_DEFAULT);

png_set_PLTE(png_ptr, info_ptr, palette, num_palette);

/* Add a transparency chunk if asked to make the background color
 * (= first color of the cel palette) transparent */
if (set_trans_bg) {
	png_byte transp_index = 0;
		/* index of the transparent color in the palette
		 * = the first one */

	png_set_tRNS(png_ptr, info_ptr, &transp_index, 1,
	             NULL /*unused in paletted images*/);
}


png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

png_write_end(png_ptr, info_ptr);
fclose(png_fp);

png_destroy_write_struct(&png_ptr, &info_ptr);

free(buf);
free(palette);

return 0;
}



/* ----------------------------------------
 * Convert a paletted PNG file to a CEL one 
 * ----------------------------------------
 */
int png2cel(char *png_file /*IN*/, char *cel_file /*OUT*/)
{

FILE *png_fp;
FILE *cel_fp;

/*
 * libpng variables
 */
png_structp png_ptr = NULL;
png_infop info_ptr = NULL;

int color_type;      	/* Type of the png image */
int bit_depth;  	/* bits per channel */ 
png_byte channels;	/* nb of channels. channels * bit_depth = bpp */

png_uint_32 width, height;

png_colorp palette = NULL;	/* palette of the png file */
int num_palette;	/* number of colors in the png file palette */ 

/* array of pointers to scanlines, needed to load the image
 * matches the lines of img_pixels */
png_bytep *row_pointers = NULL;
png_uint_32 rowbytes;	/* nb of bytes per row */


/* Open PNG and CEL files */
png_fp = fopen(png_file, "rb");
if (png_fp == NULL) {
	fprintf(stderr, "Couldn't open %s: %s\n", png_file, strerror(errno));
	return 2;
}

cel_fp = fopen(cel_file, "wb");
if (cel_fp == NULL) {
	fprintf(stderr, "Couldn't open %s: %s\n", cel_file, strerror(errno));
	return 2;
}


{ /* Check the PNG signature */
unsigned char signature[8];
memset (signature, '\0', sizeof signature);

if (fread(&signature, sizeof signature, 1, png_fp) < 1) {
	fprintf(stderr, "Couldn't read the PNG signature of %s\n", png_file);
	fclose(png_fp);
	fclose(cel_fp);
	return 2;
}

if (png_sig_cmp(signature, 0, 8) != 0) {
	fprintf(stderr, "Bad PNG signature in %s!\n", png_file);
	fclose(png_fp);
	fclose(cel_fp);
	return 2;
}
} /* Check the PNG signature */


/*
 * Create the main png structures
 */
png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                 (png_voidp) NULL,
                                 (png_error_ptr) NULL,
                                 (png_error_ptr) NULL);

if (png_ptr == NULL) { /* error */
	fprintf(stderr,
	        "png2cel(): png_create_read_struct() error on %s!\n",
	        png_file);
	fclose(png_fp);
	fclose(cel_fp);
	return 2;
}

info_ptr = png_create_info_struct(png_ptr);

if (info_ptr == NULL) { /* error */
	fprintf(stderr, 
	        "png2cel(): png_create_info_struct() error on %s!\n",
	        png_file);
	fclose(png_fp);
	fclose(cel_fp);
	png_destroy_read_struct(&png_ptr, NULL, NULL);
	return 2;
}

/* libpng requires this */
if (setjmp(png_jmpbuf(png_ptr)) != 0) { /* error */
	fprintf(stderr, "setjmp() failed on %s!\n", png_file);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(png_fp);
	fclose(cel_fp);
	return 2;
}

/* Set the png input stream */
png_init_io(png_ptr, png_fp);

/* Tell libpng we have already read the signature of the png file */
png_set_sig_bytes(png_ptr, 8);


/* Get info header from the png image */
png_read_info(png_ptr, info_ptr);


/* Transform image */
bit_depth = png_get_bit_depth(png_ptr, info_ptr);

if (bit_depth == 16) {
	png_set_strip_16(png_ptr); /* Strip 16 bits per channel to 8 bits */
} else if (bit_depth < 8) {
	png_set_packing(png_ptr); /* At least 1 byte per pixel */
}


/* Update the "info" structure of the png image */
png_read_update_info(png_ptr, info_ptr);

/* Get updated informations about the image */ 
png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
             NULL, NULL, NULL);


/*
 * Filter image:
 * Keep going only if it can be converted to CEL
 */

/*
 * Palette filter: We need a palette.
 *
 * Get the number of channels in the image
 * libpng says valid values are 1 (GRAY, PALETTE), 2 (GRAY_ALPHA),
 * 3 (RGB), 4 (RGB_ALPHA or RGB + filler byte)
 */
channels = png_get_channels(png_ptr, info_ptr);
if (channels != 1) {
	fprintf(stderr,
	        "%s rejected: unpaletted PNG file!\n",
	        png_file);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(png_fp);
	fclose(cel_fp);
	return 3;
}

/* Get the palette */
png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);	
	/* Should allocate the palette memory
	 * and free it automatically on "png_destroy_..." calls */

/*
 * Image size filter
 *
 * CEL uses two 16 bits ints to store the image size,
 * PNG uses two 32 bits ints
 */
if (((width >> 16) > 0) || ((height >> 16) > 0)) {
	fprintf(stderr,
	        "%s rejected: size %lix%li too big!\n",
	        png_file, width, height);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(png_fp);
	fclose(cel_fp);
	return 3;
}



/*
 * Create the structure to store the PNG image to read
 */

/* Allocate row pointers (freed automatically if "png_malloc()" used) */
row_pointers = png_malloc(png_ptr, height * sizeof(png_bytep));
if (row_pointers == NULL) {
	fprintf(stderr, "malloc PNG row pointers: %s\n!",
	        strerror(errno));
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(png_fp);
	fclose(cel_fp);
	return 2;
}
 
{ /* Allocate rows */
int row;
rowbytes = png_get_rowbytes(png_ptr, info_ptr);

for (row = 0; row < height; row++) {
	row_pointers[row] = png_malloc(png_ptr, rowbytes);
	if (row_pointers[row] == NULL) {
		fprintf(stderr, "malloc PNG row[%d]: %s\n!",
		        row, strerror(errno));
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(png_fp);
		fclose(cel_fp);
		return 2;
	}
}
}

/*
 * Read the PNG image
 */
png_read_image(png_ptr, row_pointers);

png_read_end(png_ptr, NULL); /* required, but useless */

fclose(png_fp);




/*
 * Write the CEL header
 */
{
unsigned char cel_header[32];

memset(cel_header, '\0', sizeof cel_header);

/* CEL  Signature */
cel_header[0] = 0x19;
cel_header[1] = 0x91;

/* Image size, Less Significant Byte First */
cel_header[2] = (unsigned char) width; 
cel_header[3] = (unsigned char) (width >> 8);

cel_header[4] = (unsigned char) height;
cel_header[5] = (unsigned char) (height >> 8);


if (fwrite((void *) cel_header, sizeof cel_header, 1, cel_fp) < 1) {
	fprintf(stderr, "Couldn't write CEL header to %s\n!",
	        cel_file);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(cel_fp);
	return 2;
}
}


/*
 * Write the CEL palette
 *
 * CEL palette consists of 256 sequences of 3 bytes (red, green, blue) records.
 * The PNG palette can have less than 256 entries.
 * CEL values range [0..63], PNG ones range [0..255]
 */
{
unsigned char cel_palette[256 * 3]; /* 256 indexes, 3 bytes each */
unsigned char *color_sample_ptr = cel_palette;
int i;

memset(cel_palette, '\0', sizeof cel_palette);

for (i = 0; i < num_palette; i++) {
	*color_sample_ptr++ = (unsigned char) (palette[i].red   / 4);
	*color_sample_ptr++ = (unsigned char) (palette[i].green / 4);
	*color_sample_ptr++ = (unsigned char) (palette[i].blue  / 4);
}

if (fwrite((void *) cel_palette, sizeof cel_palette, 1, cel_fp) < 1) {
	fprintf(stderr, "Couldn't write CEL palette to %s\n!",
	        cel_file);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(cel_fp);
	return 2;
}
}


/*
 * Write the CEL pixels
 */
{
int row;
for (row = 0; row < height; row++) {
	if (fwrite((void *) row_pointers[row], rowbytes, 1, cel_fp) < 1) {
		fprintf(stderr, "Couldn't write row #%d to %s\n!",
	                row, cel_file);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(cel_fp);
	        return 2;
	}
}
}

png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	/* Should trigger the destruction of the PNG palette,
	 * row pointers and rows */
	 
fclose(cel_fp);

return 0;
}
