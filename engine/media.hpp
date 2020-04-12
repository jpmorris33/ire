/*
 *      Media System: mostly Input, Output and some display functions
 */
#include "graphics/iregraph.hpp"
#include "graphics/irekeys.hpp"

extern void init_media(int game);
extern void init_enginemedia();
extern void term_media();
//extern IREBITMAP *load_cel(char *filename, RGB *pal);
extern int sizeof_sprite(IREBITMAP *bmp);
extern int UpdateAnim();
extern unsigned int GetIREClock();
extern unsigned int GetAnimClock();
extern void ResetIREClock();
extern int GetKey();
extern int GetMouseZ(int z);
extern int WaitForKey();
extern unsigned char WaitForAscii();
extern void FlushKeys();
extern void Plot(int max);
extern void Screenshot(IREBITMAP *savescreen);
extern int GetStringInput(char *ptr,int len);

extern IREBITMAP *swapscreen;
//extern BITMAP *screen;
extern char *lastvrmcall;
extern IRECOLOUR *ire_transparent,*ire_black;
extern int ire_bpp,ire_bytespp;
extern unsigned char ire_transparent_r;
extern unsigned char ire_transparent_g;
extern unsigned char ire_transparent_b;
//extern IREBITMAP *MakeScreen(int w, int h);
extern void SaveScreen();
extern void RestoreScreen();

extern void LoadBacking(IREBITMAP *bmp, char *fname);

