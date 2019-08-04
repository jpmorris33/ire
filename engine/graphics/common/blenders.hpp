#ifndef __IRE_BLENDERS
#define __IRE_BLENDERS

typedef void (*BLENDERFUNC)(unsigned char *dest, unsigned char *src);

extern int BlenderInit(IRECOLOUR *chromakey, int bpp);
extern void BlenderTerm();
extern void BlenderSetAlpha(int alpha);
extern int BlenderGetAlpha();
extern void BlenderSetMode(int mode, int bpp);
extern BLENDERFUNC BlenderGetOp(int mode, int bpp);

extern BLENDERFUNC BlenderOp;

#endif
