/*
 * Tool for the IRE engine.
 *
 * Convert a ".cel" image file to a paletted png one, and conversely.
 *
 * Link against libpng library: "-lpng"
 */


#ifndef CEL_PNG_H
#define CEL_PNG_H

#include <stdio.h>

/*
 * Convert a CEL file to a PNG one
 */
int cel2png(char *cel_file	/* IN,	CEL filename */,
            char *png_file	/* OUT,	PNG filename */,
            int set_trans_bg	/* IN,	bool. Set it to 1 to make the
                              	 * first color of the CEL palette appear
                              	 * transparent in the PNG image
                              	 */
);



/*
 * Convert a PNG file to a CEL one
 */
int png2cel(char *png_file /*IN*/, char *cel_file /*OUT*/);

#endif
