/*
 *      IT-HE Generic Support Library, (C) 2000 IT-HE Software
 */

#ifndef __ITHE_SUPPORT_LIB__
#define __ITHE_SUPPORT_LIB__

#include <stdio.h>            // Just in case
#include <time.h>
#include <stdlib.h>

// Enable full memory debugging with Fortify (very slow!)
//#define FORTIFY

// Are we a Unix-like system?
#if defined (__linux__) || defined (__BEOS__) || defined (__APPLE__)
#define __SOMEUNIX__
#endif

#ifndef _WIN32
#include <signal.h>
#include <stdbool.h> 
#endif


// Defines that may need changing

#define MAX_LINE_LEN 4096	// Maximum line length for string processing
//#define LOG_MEM		// Help find leaks

// Are we on Windows?  If so we need to help it
#ifdef _WIN32
	typedef __int64 qword;
	typedef unsigned __int64 uqword;

	#ifndef ALLEGRO_H
		typedef __int32 int32_t;
	#endif

	#define F_OK 0
	#define X_OK 1
	#define W_OK 2
	#define R_OK 4

	#ifndef __cplusplus
		typedef int bool;
		#define false 0
		#define true 1
	#endif

	#define BYTE_ORDER 1
	#define LITTLE_ENDIAN 1
	#define BIG_ENDIAN 2

#else
	typedef long long qword;
	typedef unsigned long long uqword;
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifndef NO_FORTIFY
#include "fortify.h"          // Fortify is a memory-leak detector
#endif

//
//      Log and console
//

extern char *ithe_get_version();

// Debugging log functions

extern void ilog_start(const char *logname,const char *prevlog);
extern void ilog_stop(void);
extern void (*ilog_printf)(const char *msg, ...); // dynamic logger (call this mainly)
extern void ilog_quiet(const char *msg, ...);  // Quiet logger
extern void ilog_text(const char *msg, ...);  // Text logger
extern char ilog_superpanic;				// Set to 1 for trace on exit

// Support routines

extern void ithe_panic(const char *m,const char *n);     // Crash out with an error
extern void ithe_safepanic(const char *m,const char *n); // Panic for C++ d'tors
extern void ithe_superpanic();               // Breakpoint
extern void ithe_complain(const char *m);          // Complain

#ifndef _WIN32
	#define CRASH(); raise(SIGTRAP);
#else
	#define CRASH(); DebugBreak();
#endif

// IT-HE Virtual Filesystem

typedef struct
	{
	int length;
	int origin;
	char *truename;
	FILE *fp;
	} IFILE;

#define igetb(i) igetc(i)
#define iputb(x,i) iputc(x,i)

#define igetsh_i(i) igetsh_native(i)
#define igetsh_m(i) igetsh_reverse(i)
#define igetl_i(i) igetl_native(i)
#define igetl_m(i) igetl_reverse(i)
#define igetll_i(i) igetll_native(i)
#define igetll_m(i) igetll_reverse(i)
#define iputsh_i(x,i) iputsh_native(x,i)
#define iputsh_m(x,i) iputsh_reverse(x,i)
#define iputl_i(x,i) iputl_native(x,i)
#define iputl_m(x,i) iputl_reverse(x,i)
#define iputll_i(x,i) iputll_native(x,i)
#define iputll_m(x,i) iputll_reverse(x,i)
#define iputlu_i(x,i) iputlu_native(x,i)
#define iputlu_m(x,i) iputlu_reverse(x,i)

extern char *ifile_prefix;
extern unsigned char igetc(IFILE *ifp);
extern unsigned short igetsh_native(IFILE *ifp);
extern unsigned short igetsh_reverse(IFILE *ifp);
extern long igetl_native(IFILE *ifp);
extern long igetl_reverse(IFILE *ifp);
extern unsigned long igetlu_native(IFILE *ifp);
extern unsigned long igetlu_reverse(IFILE *ifp);
extern uqword igetll_native(IFILE *ifp);
extern uqword igetll_reverse(IFILE *ifp);
extern int iread(unsigned char *buf, int l, IFILE *ifp);
extern void iputc(unsigned char c, IFILE *ifp);
extern void iputsh_native(unsigned short s, IFILE *ifp);
extern void iputsh_reverse(unsigned short s, IFILE *ifp);
extern void iputl_native(long l, IFILE *ifp);
extern void iputl_reverse(long l, IFILE *ifp);
extern void iputlu_native(unsigned long l, IFILE *ifp);
extern void iputlu_reverse(unsigned long l, IFILE *ifp);
extern void iputll_native(uqword q, IFILE *ifp);
extern void iputll_reverse(uqword q, IFILE *ifp);
extern int iwrite(unsigned char *buf, int l, IFILE *ifp);
extern unsigned int itell(IFILE *ifp);
extern unsigned int ifilelength(IFILE *ifp);
extern time_t igettime(const char *filename);

extern void iseek(IFILE *ifp, int offset, int whence);
extern int iexist(const char *filename);
extern IFILE *iopen(const char *filename);
extern IFILE *iopen_write(const char *filename);

extern void iclose(IFILE *ifp);
extern int ieof(IFILE *ifp);
extern void ihome(char *path);
extern char **igetdir(const char *dir, int *items, int showdirs);
extern void ifreedir(char **list, int items);

#define IGETDIR_SHOWDIRS	1
#define IGETDIR_DOTS		2

extern unsigned char *iload_file(char *filename, int *outlen);

// Memory subsystem

extern void M_init(void);
extern void M_term(void);
#ifndef FORTIFY
extern void *M_get(unsigned int qty,long amt);
extern void *M_resize(void *old,long amt);
extern void M_free(void *ptr);
extern void *M_get_log(unsigned int qty,long amt,int l,char *f);
extern void M_free_log(void *ptr,int l,char *f);

#ifdef LOG_MEM
#ifndef ILIB_MMOD
#define M_get(a,b) M_get_log(a,b,__LINE__,__FILE__)
#define M_free(a) M_free_log(a,__LINE__,__FILE__)
#endif
#endif

#endif
extern int M_chk(void *ptr);
extern void M_error2(char *msg,unsigned long a);

// String manipulation library

extern const char *strrest(const char *input);         // Get pointer to tail of string
extern char *strlast(const char *input);         // Get pointer to end of tail
extern char *strfirst(const char *input);        // Get COPY of head of string
extern char *hardfirst(char *line);        // Mutilate original string
extern char *strgetword(const char *line, int p);// Get the word at this position
extern char *itoa(int v, char *s, int r);  // Make a string from an integer
extern void strstrip(char *tbuf);          // Strip a string
extern char isxspace(unsigned char c);     // Is this character whitespace?
extern int strcount(const char *a);              // Count number of words
extern int strgetnumber(const char *a);          // String-to-number
extern int strisnumber(const char *a);           // Is the string a number?
extern void strslash(char *a);             // Convert slashes to what we want
extern void strwinslash(char *a);          // Convert slashes to Windows format

extern char *strLocalBuf(void);        		// Get a local buffer
extern int istricmp(const char *a,const char *b);		// Fast stricmp
extern int istricmp_fuzzy(const char *a,const char *b);		// Fuzzy stricmp
extern void strdeck(char *a, char b);		// Kill all instances of one char
extern char NOTHING[];                      // blank, non-NULL string
extern char *stradd(char *old, const char *extra);

// DOS functions (For Linux and BeOS, Windows already has them)

#ifdef __SOMEUNIX__

#ifndef ALLEGRO_H // Allegro emulates these too
	extern void strlwr(char *a);
	extern void strupr(char *a);
	// If no allegro, plug in my istricmp function
	#define stricmp(a,b) istricmp(a,b)
#endif

extern long filelength(int fhandle);

#else
#include <io.h>
#include <string.h>
#endif

#define SAFE_STRCPY(a,b) {strncpy((a),(b),sizeof((a))-1);a[sizeof((a))-1]=0;}


// Beos is not well

#ifdef __BEOS__
extern int getw(FILE *fp);
extern void putw(int w, FILE *fp);
#endif

#ifdef _WIN32
#define snprintf _snprintf	// Windows is shit
#endif

// JUG file stuff

//0x747.500 jug version no
#define JUG_BASE 0x7074	
#define JUG_VERSION 0x0500
#define JUG_MAXPATH 224

typedef struct {
char filename[JUG_MAXPATH+1]; // 224 on disk, 225 in memory for null terminator
short year;
char month,day;
char hour,min;
char spare[22];
int32_t len;

// Not part of disk format
long _headerStart;
long _dataStart;
FILE *_fp;
} JUGFILE_ENTRY;


extern void JUG5clean(JUGFILE_ENTRY *fileentry);
extern bool JUG5writeHeader(const char *jugfile);
extern FILE *JUG5startFileSave(const char *jugfile, JUGFILE_ENTRY *fileentry);
extern void JUG5close(JUGFILE_ENTRY *fileentry);

extern bool JUG5readHeader(const char *jugfile, JUGFILE_ENTRY *fileentry);
extern FILE *JUG5getFileInfo(JUGFILE_ENTRY *fileentry);
extern void JUG5skipFileData(JUGFILE_ENTRY *fileentry);
extern void JUG5empty(JUGFILE_ENTRY *jug);


extern bool jug_mountfiles(char **jugfiles, int listlen);
extern FILE *jug_openfile(const char *filename, int32_t *length);
extern bool jug_fileexists(const char *filename);
extern void jug_closefile(FILE *fp);



// End of library functions

#ifdef __cplusplus
}
#endif

#endif // __ITHE_SUPPORT_LIB__
