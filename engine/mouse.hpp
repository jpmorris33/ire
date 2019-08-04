// Mouse support

#define MAX_MBUTTONS 64
#define RANGE_GRID 1
#define RANGE_BUTTON 2
#define RANGE_BUTTONIN 4
#define RANGE_LBUTTON 8
#define RANGE_RBUTTON 16

typedef struct MOUSERANGE
	{
	int left,top,right,bottom;
	int id,pointer,flags;
	int gw,gh;
	IRESPRITE *normal,*in;
	} MOUSERANGE;
extern int MouseOver,MouseRanges;
extern long MouseID,MouseGridX,MouseGridY,MouseMapX,MouseMapY;

extern MOUSERANGE MouseRange[MAX_MBUTTONS];

extern void AddMouseRange(int ID, int x, int y, int w, int h);
extern void ClearMouseRanges();
extern void SetRangeGrid(int ID, int w, int h);
extern void SetRangePointer(int ID, int pointerno);
extern void SetRangeButton(int ID, char *normal, char *in);
extern void CheckMouseRanges();
extern void DrawButtons();
extern void DrawRanges();
extern void PushButton(int ID, int state);
extern void RightClick(int ID);
extern void SaveRanges();
extern void RestoreRanges();
extern void WaitForMouseRelease();

