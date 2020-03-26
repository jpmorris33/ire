/*
 *      Debugging and log routines
 */

#define ITHE_VERSION "V1.71 (built " __DATE__ " at " __TIME__ ")"
#define ITHE_VERSIOND "***DEBUG***" ITHE_VERSION

#ifdef _WIN32
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <conio.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "ithelib.h"

//      Variables

char ilog_nopanic=0;
char ilog_logoff=0;

static char *bootlog=NULL;

// Functions

void (*ilog_printf)(const char *msg, ...);

void ilog_start(const char *logname, const char *prevlog);
void ilog_stop(void);
void ilog_text(const char *msg, ...);
void ilog_quiet(const char *msg, ...);
void ithe_panic(const char *m,const char *n);
void ithe_safepanic(const char *m,const char *n);
void ithe_superpanic(void);


extern void ithe_userexitfunction(void);

// Code

char *ithe_get_version()
{
#ifdef ITHE_DEBUG
	return ITHE_VERSIOND;
#else
	return ITHE_VERSION;
#endif
}

/*
 *      Start the logging system
 */

void ilog_start(const char *logname, const char *prevlog)
{
char newlog[1024];
char oldlog[1024];

if(bootlog)
   ithe_panic("Bootlog already started",NULL);

strcpy(newlog,logname);
strcpy(oldlog,prevlog);

ilog_printf=ilog_text;          // Set default logger, backup last log
remove(oldlog);
rename(newlog,oldlog);

// Create the bootlog
bootlog=(char *)calloc(1,strlen(newlog)+1);
strcpy(bootlog,newlog);
}

/*
 *      Stop the logging system
 */

void ilog_stop(void)
{
if(bootlog)
	free(bootlog);

bootlog=NULL;
}

/*
 *      Log to file and using printf
 */

void ilog_text(const char *msg, ...)
{
FILE *blt;
va_list ap;

if(ilog_logoff)
    return;

if(!bootlog)
    ithe_panic("ilog_text() - Missing call to ilog_start",NULL);

va_start(ap, msg);
vprintf(msg,ap);
va_end(ap);

blt=fopen(bootlog,"a");
if(!blt)
    ithe_panic("Could not append to log file",bootlog);
va_start(ap, msg);
vfprintf(blt,msg,ap);
va_end(ap);
fclose(blt);

}

/*
 *      Log to file but don't print it onscreen
 */

void ilog_quiet(const char *msg, ...)
{
FILE *blt;
va_list ap;

if(ilog_logoff)
    return;

if(!bootlog)
	return;
//    ithe_panic("ilog_quiet() - Missing call to ilog_start",NULL);

va_start(ap, msg);

blt=fopen(bootlog,"a");
if(!blt)
    ithe_panic("Could not append to log file",bootlog);
vfprintf(blt,msg,ap);
fclose(blt);

va_end(ap);
}

/*
 * quit and report the problem
 */

void ithe_panic(const char *m,const char *n)
{
static int pip=0;
#ifdef _WIN32
char msg[1024];
#endif

if(ilog_nopanic) // Quit without ceremony
    exit(1);

if(pip)
	{
	printf("Panic Panic\n");
	printf("%s\n",m);
	if(n)
		printf("%s\n",n);
	exit(1);
	}
pip = 1;        // Safety valve, in case panic panics


ithe_userexitfunction();

#ifdef _WIN32
	strcpy(msg,m);
	if(n)
		{
		strcat(msg,"\n");
		strcat(msg,n);
		}

	MessageBoxA(NULL,msg,"IRE Panic:",MB_ICONSTOP);
#else
	#ifdef __DJGPP__
		printf("(!)\n");
		printf("ÚÄPANIC:ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n");
		printf("³                                                                             ³\r³ %s\n",m);
		if(n)
			printf("³                                                                             ³\r³ %s\n",n);
		printf("ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n\n\n");
	#else
		printf("(!)\n");
		printf("* PANIC: **********************************************************************\n");
		printf("*                                                                             *\r* %s\n",m);
		if(n)
			printf("*                                                                             *\r* %s\n",n);
		printf("*******************************************************************************\n\n\n");
	#endif
#endif

ilog_quiet("PANIC:\n");
ilog_quiet("\n");
ilog_quiet(m);

ilog_quiet("\n");
if(n) ilog_quiet(n);

ilog_quiet("\n");

if(getenv("SUPERPANIC"))
    ithe_superpanic();

exit(1);
}

/*
 *   'Safe' panic - for C++ destructors
 */

void ithe_safepanic(const char *m,const char *n)
{
#ifdef _WIN32
char msg[1024];
#endif

if(ilog_nopanic)
    exit(1);

#ifdef _WIN32
	strcpy(msg,m);
	if(n)
		{
		strcat(msg,"\n");
		strcat(msg,n);
		}

	MessageBoxA(NULL,msg,"IRE panic:",MB_ICONSTOP);
#else
	#ifdef __DJGPP__
		printf("ÚÄSAFEPANIC:ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n");
		printf("³                                                                             ³\r³ %s\n",m);
		if(n)
			printf("³                                                                             ³\r³ %s\n",n);
		printf("ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n\n\n");
	#else
		printf("* SAFEPANIC: ******************************************************************\n");
		printf("*                                                                             *\r* %s\n",m);
		if(n)
			printf("*                                                                             *\r* %s\n",n);
		printf("*******************************************************************************\n\n\n");
	#endif
#endif

exit(1);
}

/*
 * Crash the system to get a core/stack trace, so we can examine what happened
 */

void ithe_superpanic(void)
{
fprintf(stderr,"Causing an exception...\n");
#ifdef _WIN32
DebugBreak();
#else
raise(SIGTRAP);
#endif
}

/*
 *   Complain: whinge at the user
 */

void ithe_complain(const char *m)
{
int tmp,tmp2;

//	MessageBox(NULL,m,"Warning:",MB_ICONEXCLAMATION);

	printf("%s\n",m);
	printf("Press enter to continue.\n");

#if defined(__DJGPP__) || defined(_WIN32)
	tmp2=tmp=getch();
#else
	tmp2=read(0,&tmp,1); // blocking keypress (read byte from stdin)
#endif
}

