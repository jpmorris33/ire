/*
 *      Cookies.h
 *
 *	Simple, 4-byte headers for the new file formats
 *      These replace the weird ones from before, apologies to my teenage self.
 */


#define COOKIE_MapName "_________________\n_________________\n_________________\n"


// After the 4-byte signature, there is another 4 bytes, e.g. "r00_"
// with an EOF character as the last byte to stop type or less from reading the binary.
// For text files, this may be a CR instead so only check the first 3 bytes

#define COOKIE_PWAD "PWAD"	// This has no version code

#define COOKIE_Z1 "iMZ1"
#define COOKIE_Z2 "iMZ2"
#define COOKIE_Z3 "iMZ3"
#define COOKIE_MAPFILE "iMAP"
#define COOKIE_SAVEFILE "iSAV"
#define COOKIE_LSDFILE "iLSD"

// Revision codes

#define BINCOOKIE_R00 "r00\x1a"
#define BINCOOKIE_R01 "r01\x1a"
#define BINCOOKIE_R02 "r02\x1a"

#define TEXTCOOKIE_R00 "r00\n"
#define TEXTCOOKIE_R01 "r01\n"
#define TEXTCOOKIE_R02 "r02\n"


// Legacy codes for detecting old versions.  You just don't want to know.
#define OLDCOOKIE_MZ1 "On W"
#define OLDCOOKIE_MZ2 "Two "
#define OLDCOOKIE_MZ3 "Weep"
#define OLDCOOKIE_MAP "On 2"
#define OLDCOOKIE_MS  "Fri "
#define OLDCOOKIE_SAV "On 4"
#define OLDCOOKIE_LSD "On 4"

