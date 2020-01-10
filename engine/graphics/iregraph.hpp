//
//	IRE graphics abstraction layer
//
//	22/05/2011, JPM
//

#ifndef _IRE_IREGRAPH
#define _IRE_IREGRAPH

#include "irekeys.hpp"

class IRECOLOUR
	{
	public:
		IRECOLOUR();
		IRECOLOUR(int r, int g, int b);
		IRECOLOUR(unsigned int packed);
		void Set(int r, int g, int b);
		void Set(unsigned int packed);
		unsigned int packed;
		unsigned char r,g,b;
	};


class IREBITMAP
	{
	public:
		IREBITMAP(int w, int h);
		IREBITMAP(int w, int h, int bpp);
		virtual ~IREBITMAP();
		virtual void Clear(IRECOLOUR *col);
		virtual void Draw(IREBITMAP *dest, int x, int y)=0;
		virtual void DrawAlpha(IREBITMAP *dest, int x, int y, int level);
		virtual void DrawShadow(IREBITMAP *dest, int x, int y, int level);
		virtual void DrawSolid(IREBITMAP *dest, int x, int y);
		virtual void DrawStretch(IREBITMAP *dest, int x, int y, int w, int h);
		virtual void DrawStretch(IREBITMAP *dest, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);
		virtual void FillRect(int x, int y, int w, int h, IRECOLOUR *col);
		virtual void FillRect(int x, int y, int w, int h, int r, int g, int b);
		virtual void Line(int x, int y, int x2, int y2, IRECOLOUR *col);
		virtual void Line(int x, int y, int x2, int y2, int r, int g, int b);
		virtual void ThickLine(int x, int y, int x2, int y2, int radius, IRECOLOUR *col);
		virtual void ThickLine(int x, int y, int x2, int y2, int radius, int r, int g, int b);
		virtual void Polygon(int count, int *vertices, IRECOLOUR *col);
		virtual void Polygon(int count, int *vertices, int r, int g, int b);
		virtual void Circle(int x, int y, int radius, IRECOLOUR *col);
		virtual void Circle(int x, int y, int radius, int r, int g, int b);
		virtual void SetBrushmode(int mode, int alpha);
		virtual void Get(IREBITMAP *src, int x, int y);
		virtual int GetPixel(int x, int y);
		virtual int GetPixel(int x, int y, IRECOLOUR *col);
		virtual void PutPixel(int x, int y, IRECOLOUR *col);
		virtual void PutPixel(int x, int y, int r, int g, int b);
		virtual void HLine(int x, int y, int w, IRECOLOUR *col);
		virtual void HLine(int x, int y, int w, int r,int g,int b);
		virtual void Render();	// Render to physical screen
		virtual void Darken(unsigned int level);
		virtual void Merge(IREBITMAP *src);
		virtual void ShowMouse();
		virtual void HideMouse();

		virtual unsigned char *GetFramebuffer();
		virtual void ReleaseFramebuffer(unsigned char *fbptr);
		virtual int SaveBMP(const char *filename);
		virtual int GetW();
		virtual int GetH();
		virtual int GetDepth();
	};
	
// High-performance subset of bitmap

class IRESPRITE
	{
	public:
		IRESPRITE(IREBITMAP *src);
//		IRESPRITE(const char *filename);
		IRESPRITE(const void *data, int len);
		virtual ~IRESPRITE();
		virtual void Draw(IREBITMAP *dest, int x, int y)=0;
		virtual void DrawSolid(IREBITMAP *dest, int x, int y);
		virtual void DrawAddition(IREBITMAP *dest, int x, int y, int level);
		virtual void DrawAlpha(IREBITMAP *dest, int x, int y, int level);
		virtual void DrawDissolve(IREBITMAP *dest, int x, int y, int level);
		virtual void DrawShadow(IREBITMAP *dest, int x, int y, int level);
		virtual void DrawInverted(IREBITMAP *dest, int x, int y);
		virtual int GetW();
		virtual int GetH();
//		virtual int Save(const char *filename);
		virtual void *SaveBuffer(int *len);
		virtual void FreeSaveBuffer(void *buffer);
		virtual unsigned int GetAverageCol();
		virtual void SetAverageCol(unsigned int acw);
	};

	
// Mouse cursor

class IRECURSOR
	{
	public:
		IRECURSOR();
		IRECURSOR(IREBITMAP *src, int hotx, int hoty);
		virtual ~IRECURSOR();
		virtual void Set()=0;
	};
	
// Lightmap - basically an 8-bit monochrome bitmap
	
class IRELIGHTMAP
	{
	public:
		IRELIGHTMAP(int w, int h);
		virtual ~IRELIGHTMAP();
		virtual void Clear(unsigned char level)=0;
		virtual void DrawLight(IRELIGHTMAP *dest, int x, int y);
		virtual void DrawDark(IRELIGHTMAP *dest, int x, int y);
		virtual void DrawSolid(IRELIGHTMAP *dest, int x, int y);
		virtual void DrawCorona(int x, int y, int radius, int intensity, int falloff, bool darken);
		virtual void Get(IRELIGHTMAP *src, int x, int y);
		virtual unsigned char GetPixel(int x, int y);
		virtual void PutPixel(int x, int y, unsigned char level);
		virtual void Render(IREBITMAP *dest);
		virtual void RenderRaw(IREBITMAP *dest);

		virtual unsigned char *GetFramebuffer();
		virtual int GetW();
		virtual int GetH();
	};

	
// Font and renderer
	
class IREFONT
	{
	public:
		IREFONT();
		IREFONT(const char *filename);
		virtual ~IREFONT();
		virtual void Draw(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *string)=0;
		virtual void Printf(IREBITMAP *dest, int x, int y, IRECOLOUR *fg, IRECOLOUR *bg, const char *fmt, ...);
		virtual int Length(const char *str);
		virtual int Height();
		virtual int IsColoured();
	};

	

extern IREBITMAP *MakeIREBITMAP(int w, int h);
extern IREBITMAP *MakeIREBITMAP(int w, int h, int bpp);
extern IRESPRITE *MakeIRESPRITE(IREBITMAP *bmp);
extern IRESPRITE *MakeIRESPRITE(const void *data, int len);
extern IRECURSOR *MakeIRECURSOR();
extern IRECURSOR *MakeIRECURSOR(IREBITMAP *bmp, int hotx, int hoty);
extern IRELIGHTMAP *MakeIRELIGHTMAP(int w, int h);
extern IRELIGHTMAP *MakeIRELIGHTMAP(int w, int h, const unsigned char *data);
extern IREFONT *MakeBitfont();
extern IREFONT *MakeBitfont(const char *filename);
extern IREFONT *MakeBytefont();
extern IREFONT *MakeBytefont(const char *filename);


// Nasty hack for compatibility with Allegro while reworking the engine
//extern void *IRE_GetBitmap(IREBITMAP *bmp);

// Startup functions
extern const char *IRE_InitBackend();
extern const char *IRE_Backend();
extern void *IRE_SetTimer(void (*timer)(), int clockhz);
extern void IRE_StopTimer(void *handle, void (*timer)());
extern void IRE_Shutdown();
extern int IRE_Startup(int allow_break, int videomode);

// Other utilities
extern void IRE_StartGFX(int bpp);
extern void IRE_StopGFX();
extern void IRE_WaitFor(int ms);

extern void IRE_SetMouse(int status);
extern int IRE_GetMouse();
extern int IRE_GetMouse(int *b);
extern int IRE_GetMouse(int *x, int *y, int *z, int *b);
extern void IRE_ShowMouse();
extern void IRE_HideMouse();

#define IREMOUSE_LEFT		1
#define IREMOUSE_RIGHT		2
#define IREMOUSE_MIDDLE		3

// Blending modes
#include "common/blenders.hpp"

#define ALPHA_SOLID		0
#define ALPHA_TRANS		1
#define ALPHA_ADD		2
#define ALPHA_SUBTRACT		3
#define ALPHA_INVERT		4
#define ALPHA_DISSOLVE		5


#define IRE_FNT1 0x31544e46
#define IRE_FNT2 0x32544e46

extern IRECOLOUR *ire_transparent;

#endif
