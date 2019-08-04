/*
 *      Console:   Graphical text output routines
 */

#ifndef __IRCONSOLE
#define __IRCONSOLE

#include "graphics/iregraph.hpp"

#ifdef __cplusplus
extern "C" {
#endif

extern void irecon_init(int x, int y, int w, int h);
extern void irecon_term();
extern void irecon_update();
extern void irecon_cls();
extern int irecon_font(int fnum);
extern int irecon_getfont();
extern int irecon_loadfont(int no);
extern void irecon_registerfont(const char *ffile, int no);
extern void irecon_loadfonts();
extern void irecon_mouse(int mouse);
extern void irecon_clearline();
extern void irecon_newline();
extern void irecon_colour(int r, int g, int b);
extern void irecon_colourfont();
extern void irecon_getcolour(int *r, int *g, int *b);
extern void irecon_savecol();
extern void irecon_loadcol();
extern void irecon_print(const char *msg);
extern void irecon_printf(const char *msg,...);
extern void irecon_printxy(int x, int y, const char *msg);
extern int  irecon_getinput(char *buffer, int maxlen);
extern void Show();
extern void ShowSimple();
extern void Bug(const char *msg,...);
extern void KillGFX();
extern int ilog_break;

#ifdef __cplusplus
}
#endif

extern IREFONT *curfont;

#endif

