/* $Id: png2cel.c,v 1.1 2004/09/11 16:17:55 mlefebvr Exp $
 *
 * Tool for the IRE engine.
 *
 * Convert a paletted PNG file to a CEL one.
 *
 * Restrictions on the PNG image:
 * - must be a paletted image
 * - height and width can't excess a two bytes-seized unsigned integer
 *   in order to store them in the CEL file header ;)
 * - Transparency [indexes] will be lost.
 * Check the source file "cel-png.c"
 *
 * Link against libpng library: "-lpng"
 */

#ifdef __GNUC__ 
#undef _GNU_SOURCE	// Shut the compiler up
#define _GNU_SOURCE	/* To allow the use of the strcasestr() function
                         * (case independent)
                         */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "cel-png.h"	/* CEL <-> PNG conversion functions */


static char progname[] = "png2cel";

static int usage(const char *progname);



int main(int argc, char **argv)
{
int rc = 0; /* return code */
int remaining_args;

char *cel_file;
char *png_file;
char *filename_tmp = NULL;

remaining_args = argc - 1;

if ((remaining_args == 0) || (remaining_args > 2)) /* No or too many args */ {
	usage(progname);
	exit(1); /* Bad args */
} else if (remaining_args == 1) /* Only the PNG file specified */ {
	char *ptr;

	png_file = argv[1];

	/*
	 * Build the cel file name
	 * (cel_file = png_file) =~ s/.png$/.cel/i;
	 */
	filename_tmp = strdup(png_file);
	if (filename_tmp == NULL) {
		perror("strdup");
		exit(2);
	}

	#ifdef _GNU_SOURCE
	ptr = strcasestr(filename_tmp, ".png"); /* case independent */

	#else
	ptr = strstr    (filename_tmp, ".png");

	#endif

	if (ptr == NULL) {
		fprintf(stderr, "%s \"%s\": PNG files must end by \".png\"\n",
		        progname, filename_tmp);
		free(filename_tmp);
		exit(1);
	}
	strcpy(ptr, ".cel");
	cel_file = filename_tmp;

} else /* CEL file and PNG file specified */ {
	png_file = argv[1];
	cel_file = argv[2];
}

rc = png2cel(png_file, cel_file);

if (filename_tmp != NULL) { free(filename_tmp); }

exit(rc);
}

static int usage(const char *progname)
{
printf("Usage: %s pngfile.png [celfile.cel]\n\n", progname);
puts("Convert a paletted PNG file to the CEL format used by IRE");
puts("If celfile.cel is not specified, it is named after pngfile.png\n");
puts("Restrictions on the PNG image:");
puts("- must be a paletted image");
puts("- Transparency [indexes] will be lost\n");

return 0;
}
