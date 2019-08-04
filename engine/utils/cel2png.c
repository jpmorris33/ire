/* $Id: cel2png.c,v 1.4 2004/09/11 16:16:44 mlefebvr Exp $
 *
 * Tool for the IRE engine.
 *
 * Convert a CEL image file to a PNG one.
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

/*
 * Use GNU long options if compiling with gcc
 */
#ifdef __GNUC__ 
#include <getopt.h>
#endif

#include <errno.h>
#include <string.h>

#include "cel-png.h"	/* the CEL <-> PNG conversion functions */


static char progname[] = "cel2png";

static int usage(const char *progname);



int main(int argc, char **argv)
{
int rc = 0; /* return code */

int transp_bg = 1; /* Bool, 1 if background asked to be transparent */

char *cel_file;
char *png_file;
char *filename_tmp = NULL;


/*
 * Variables to deal with program arguments
 */

#ifdef __GNUC__
/* GNU long options */
struct option long_option[] = {
	/*
	 * Make the background transparent or solid
	 */
	{ "solid-bg", no_argument, NULL, 's' },

	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0}
};
#endif

int c; /* return value of getopt_long() or getopt() */

int remaining_args;

/*
 * Process options
 */
#ifdef __GNUC__
/* Use GNU long options */
while ((c = getopt_long(argc, argv, "hs", long_option, NULL)) != -1) {
#else
/* Use unistd options */
while ((c = getopt(argc, argv, "hs") != -1) {
#endif
	switch (c) {
	case 0: /* Recognized long options */
		break;

	case 's': /* Background option */
		transp_bg = 0; /* Keep solid */
		break;

	case 'h':
		exit(usage(progname));

	default:
		exit(1);
	}
}

/*
 * Process args
 */
remaining_args = argc - optind;
if ((remaining_args == 0) || (remaining_args > 2)) /* No or too many args */ {
	usage(progname);
	exit(1); /* Bad args */
} else if (remaining_args == 1) {
	char *ptr;

	cel_file = argv[optind];

	/*
	 * Build the png file name
	 * (png_file = cel_file) =~ s/.cel$/.png/i;
	 */
	filename_tmp = strdup(cel_file);
	if (filename_tmp == NULL) {
		perror("strdup");
		exit(2);
	}
	
	#ifdef _GNU_SOURCE
	ptr = strcasestr(filename_tmp, ".cel"); /* case independent */

	#else
	ptr = strstr    (filename_tmp, ".cel");

	#endif
	
	if (ptr == NULL) {
		fprintf(stderr, "%s \"%s\": CEL files must end by \".cel\"\n",
		        progname, filename_tmp);
		free(filename_tmp);
		exit(1);
	}
	strcpy(ptr, ".png");

	png_file = filename_tmp;

} else /* CEL file and PNG file specified */ {
	cel_file = argv[optind];
	png_file = argv[optind + 1];
}

/* Convert cel file to png */
rc = cel2png(cel_file, png_file, transp_bg);

if (filename_tmp != NULL) { free(filename_tmp); }

exit(rc);
}

static int usage(const char *progname)
{
printf("Usage: %s [options] celfile.cel [pngfile.png]\n\n", progname);
puts("Convert an IRE cel file to png format");
puts("If pngfile.png is not specified, it is named after celfile.cel\n");
puts("Options:\n");

#ifdef __GNUC__
puts("--help"); /* Long option */
#endif
puts("-h          Print this help");

#ifdef __GNUC__
puts("--solid-bg"); /* Long option */
#endif
puts("-s          Keep the background (first color index in the");
puts("            cel file) solid; otherwise it is made transparent\n");

return 0;
}
