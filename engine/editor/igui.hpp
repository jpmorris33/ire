/*
 *   ITHE GUI library, based on DEU's GUI constructs
 */

#include "menusys.h"

// defines


#define UP_KEY  72
#define DN_KEY  80
#define LF_KEY  75
#define RT_KEY  77
#define UNUSED NULL

#define VIEWX 32        // Window coords
#define VIEWY 72        // was 64

typedef void (*FPTR)();
enum {NORMAL,RADIOBUTTON,UNDEFINED,REGION,TOGGLE_OFF,TOGGLE,SUNK};
enum {ST_NONE,ST_OUT,ST_IN,ST_3,ST_STUCK};

struct BUTTON
    {
    int x,y,w,h,x2,y2;
    unsigned char flag;
    char type,state;
    int *toggle;                // Address of toggle state variable

    void (*left_ptr)();
    void (*right_ptr)();
    void (*middle_ptr)();
    int left_key;
    int right_key;
    int middle_key;
    char text[256];
    char intext[256];
    int textlen;
    };

struct KEYBIND
    {    
    int key;
    FPTR DoKey;
    };

// variables

extern IREBITMAP *swapscreen;
extern IREFONT *DefaultFont;
extern char running,dragflag;
extern int focus,x,y;

// Functions

int  IG_TextButton(int nx,int ny,char *ntext,FPTR l,FPTR m,FPTR r);
int  IG_InputButton(int nx,int ny,char *ntext,FPTR l,FPTR m,FPTR r);
int  IG_ToggleButton(int nx,int ny,char *ntext,FPTR l,FPTR m,FPTR r,int *tog);
void IG_UpdateText(int ID,char *text);
int  IG_Region(int nx,int ny,int w,int h,FPTR l,FPTR m,FPTR r);
void IG_Panel(int nx,int ny,int w,int h);
void IG_BlackPanel(int nx,int ny,int w,int h);
void IG_KillAll();
int  IG_Tab(int nx,int ny,char *ntext,FPTR l,FPTR m,FPTR r);
void IG_AddKey(int key,FPTR func);
void IG_SetFocus(int ID);
void IG_ResetFocus(int ID);
void IG_SetInText(int ID,char *intext);
void IG_Text(int x,int y,char *a,IRECOLOUR *col);
void IG_WaitForRelease();
void IG_SetRepeat(int ID);


void IG_Init(int max);
void IG_Term();
void IG_Dispatch();
void draw_button(int no);

int CMP(const void *a,const void *b);


