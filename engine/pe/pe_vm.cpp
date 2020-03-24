/*
 *      PEscript compiler Virtual Machine
 *
 *      A lot of this code will need checking/fixing on 64-bit architectures
 *      to prevent 64-bit pointers being truncated by 32-bit pointer math
 *      (EDIT: Well, it works on 64-bit linux and win32, win64 may need work)
 */

// Controls
#define EMERGENCY_QUIT // Allow SHIFT-BREAK to quit the VM for postmortems
//#define SAFER        // More vigorous pointer checks (but slower?)
//#define PENDING_DELETE_CHECK	// This will generate many false positives, especially with the Ark of Testimony

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ithelib.h"
#include "../graphics/iregraph.hpp"
#include "../console.hpp"
#include "../mouse.hpp"
#include "../oscli.hpp"
#include "../object.hpp"
#include "../loadsave.hpp"
#include "../gamedata.hpp"
#include "../vismap.hpp"
#include "opcodes.h"
#include "pe_api.hpp"

#ifdef __BEOS__
#define _WIN32
#endif


// Game manipulation functions:

#include "../core.hpp"
#include "../library.hpp"
#include "../gamedata.hpp"
#include "../init.hpp"
#include "../media.hpp"
#include "../object.hpp"
#include "../linklist.hpp"
#include "../sound.h"
#include "../nuspeech.hpp"
#include "../project.hpp"

extern void SetDarkness(int d);
extern char *BestName(OBJECT *o);
extern OBJECT *GameGetObject(int x,int y);
extern TILE *GetTile(int x,int y);
extern void MoveToTop(OBJECT *o);
extern void MoveToFloor(OBJECT *o);
extern void InsertAfter(OBJECT *dest, OBJECT *o);
extern void spillcontents(OBJECT *bag,int x,int y);
extern void movecontents(OBJECT *src, OBJECT *dest);
extern void gen_largemap(void);
extern void RedrawMap(void);
extern int isSolid(int x,int y);
extern void CheckHurt(OBJECT *list);
extern int GetBulk(OBJECT *obj);
extern int getYN(char *q);
extern int get_num(int no);
extern void ResyncEverything(void);
extern int Restart(void);
extern int get_key_debounced();
extern int get_key_debounced_quiet();
extern int get_key_ascii();
extern int get_key_ascii_quiet();
extern int AddToParty(OBJECT *new_member);
extern void object_sound(char *a, OBJECT *b);
extern int FindPath(OBJECT *start, OBJECT *end, int diagonal);
extern int SumObjects(OBJECT *c,char *o,int q);
extern void AddQuantity(OBJECT *c,char *o,int q);
extern int TakeQuantity(OBJECT *c,char *o,int q);
extern int MoveQuantity(OBJECT *s,OBJECT *d,char *o,int q);
extern void ResumeSchedule(OBJECT *o);
extern void CheckTime();
extern OBJECT *find_nearest(OBJECT *o, char *type);
extern void find_nearby(OBJECT *o, char *type, OBJECT **list, int listsize);
extern OBJECT *find_nearby_flag(OBJECT *o, VMINT flag, OBJECT **list, int listsize);
extern void set_light(int x, int y, int x2, int y2, int light);
extern void ForceUpdateTag(int tag);
extern OBJECT *GetFirstObject(OBJECT *cont, char *name);
extern OBJECT *GetFirstObjectFuzzy(OBJECT *cont, char *name);
extern void CentreMap(OBJECT *centre);
extern void get_mouse_release();
extern int LoadGame();
extern void SaveGame();
extern void SoundSettings();
extern int ZoomMap(int zoom, int roof);
extern int DisallowMap();
extern void Cheat_gimme();
extern void Cheat_setflag();


extern char *pe_get_function(unsigned int opcode);

extern char user_input[128];
extern char DebugVM;
extern VMINT do_lightning;
extern VMINT do_earthquake;
extern VMINT irekey;
extern VMINT pevm_err;
extern int ire_pushkey;
extern VisMap vismap;
// unused function
//static void CheckRange(PEVM *vm, void *ptr, int ident);


// Defines

#define VMOP(x) vmop[PEVM_##x] = PV_##x;
#define ADD_IP(ptr,add) (VMTYPE *)((char *)ptr + add)
#define OPMASK 31 // Max 32 operators (at present)
#define VM_STACKLIMIT 1024
#define VMEM 128

// Variables

static int cur_x=0;		// Text cursor X,Y
static int cur_y=0,cur_margin=0;
static int printlogging=0;
static int abortkernel=0;
static char vm_up=0,SI_Search=0;
static VMTYPE *vmem;   // This is the address space where the VM runs
static unsigned int vmem_ptr; // This is the allocation point
static unsigned int vmem_limit; // Size of the VM space in words

char debcurfunc[1024];
char debmsg[1024];
char *pevm_context="Undefined";

// Functions

static void PV_Invalid();
static void PV_Return();
static void PV_Printstr();
static void PV_Printint();
static void PV_PrintXint();
static void PV_PrintCR();
static void PV_ClearScreen();
static void PV_ClearLine();
static void PV_Callfunc();
static void PV_CallfuncS();
static void PV_Let_iei();
static void PV_Let_ieipi();
static void PV_Let_pes();
static void PV_Let_seU();
static void PV_ClearArrayI();
static void PV_ClearArrayS();
static void PV_ClearArrayO();
static void PV_Add();
static void PV_Goto();
static void PV_If_iei();
static void PV_If_ses();
static void PV_If_oics();
static void PV_If_oeo();
static void PV_If_oEo();
static void PV_If_o();
static void PV_If_no();
static void PV_If_i();
static void PV_If_ni();
static void PV_If_p();
static void PV_If_np();
static void PV_For();
static void PV_Searchin();
static void PV_Listafter();
static void PV_SearchWorld();
static void PV_StopSearch();
static void PV_PlayMusic();
static void PV_StopMusic();
static void PV_PlaySound();
static void PV_IsPlaying();
static void PV_ObjSound();
static void PV_Create();
static void PV_CreateS();
static void PV_Destroy();
static void PV_SetDarkness();
static void PV_SetFlag();
static void PV_GetFlag();
static void PV_ResetFlag();
static void PV_GetEngineFlag();
static void PV_If_on();
static void PV_If_non();
static void PV_If_tn();
static void PV_If_ntn();
static void PV_If_onLocal();
static void PV_If_nonLocal();
static void PV_MoveObject();
static void PV_PushObject();
static void PV_XferObject();
static void PV_ShowObject();
static void PV_PrintXYi();
static void PV_PrintXYx();
static void PV_PrintXYs();
static void PV_PrintXYcr();
static void PV_GotoXY();
static void PV_TextColour();
static void PV_TextColourDefault();
static void PV_GetTextColour();
static void PV_TextFont();
static void PV_GetTextFont();
static void PV_SetSequenceN();
static void PV_SetSequenceS();
static void PV_Lightning();
static void PV_Earthquake();
static void PV_Bestname();
static void PV_GetObject();
static void PV_GetSolidObject();
static void PV_GetFirstObject();
static void PV_GetTile();
static void PV_GetObjectBelow();
static void PV_GetBridge();
static void PV_ChangeObject();
static void PV_ChangeObjectS();
static void PV_ReplaceObject();
static void PV_ReplaceObjectS();
static void PV_SetDir();
static void PV_ForceDir();
static void PV_RedrawText();
static void PV_RedrawMap();
static void PV_MoveToPocket();
static void PV_XferToPocket();
static void PV_MoveFromPocket();
static void PV_ForceFromPocket();
static void PV_Spill();
static void PV_SpillXY();
static void PV_MoveContents();
static void PV_Empty();
static void PV_FadeOut();
static void PV_FadeIn();
static void PV_MoveToTop();
static void PV_MoveToFloor();
static void PV_InsertAfter();
static void PV_GetLOS();
static void PV_GetDistance();
static void PV_CheckHurt();
static void PV_IfSolid();
static void PV_IfVisible();
static void PV_SetUFlag();
static void PV_GetUFlag();
static void PV_If_Uflag();
static void PV_If_nUflag();
static void PV_SetLocal();
static void PV_GetLocal();
static void PV_WeighObject();
static void PV_GetBulk();
static void PV_InputInt();
static void PV_IfInPocket();
static void PV_IfNInPocket();
static void PV_ReSyncEverything();
static void PV_TalkTo1();
static void PV_TalkTo2();
static void PV_Random();
static void PV_GetYN();
static void PV_GetYNN();
static void PV_Restart();
static void PV_ScrTileN();
static void PV_ScrTileS();
static void PV_ResetTileN();
static void PV_ResetTileS();
static void PV_While1();
static void PV_While2();
static void PV_Break();
static void PV_GetKey();
static void PV_GetKey_quiet();
static void PV_GetKeyAscii();
static void PV_GetKeyAscii_quiet();
static void PV_FlushKeys();
static void PV_DoAct();
static void PV_DoActS();
static void PV_DoActTo();
static void PV_DoActSTo();
static void PV_NextAct();
static void PV_NextActS();
static void PV_NextActTo();
static void PV_NextActSTo();
static void PV_InsAct();
static void PV_InsActS();
static void PV_InsActTo();
static void PV_InsActSTo();
static void PV_StopAct();
static void PV_ResumeAct();
static void PV_GetFuncP();
static void PV_GetContainer();
static void PV_SetLeader();
static void PV_SetMember();
static void PV_AddMember();
static void PV_DelMember();
static void PV_MoveTowards8();
static void PV_MoveTowards4();
static void PV_WaitFor();
static void PV_WaitForAnimation();
static void PV_If_oonscreen();
static void PV_If_not_oonscreen();
static void PV_TakeQty();
static void PV_GiveQty();
static void PV_MoveQty();
static void PV_CountQty();
static void PV_CopySchedule();
static void PV_CopySpeech();
static void PV_AllOnscreen();
static void PV_AllAround();
static void PV_CheckTime();
static void PV_FindNear();
static void PV_FindNearby();
static void PV_FindNearbyFlag();
static void PV_FindTag();
static void PV_FastTag();
static void PV_MakeTagList();
static void PV_FindNext();
static void PV_SetLight();
static void PV_SetLight_single();
static void PV_LastOp();
static void PV_MovePartyToObj();
static void PV_MovePartyFromObj();
static void PV_MoveTag();
static void PV_UpdateTag();
static void PV_GetDataSSS();
static void PV_GetDataSSI();
static void PV_GetDataSIS();
static void PV_GetDataSII();
static void PV_GetDataISS();
static void PV_GetDataISI();
static void PV_GetDataIIS();
static void PV_GetDataIII();
static void PV_GetDataKeysII();
static void PV_GetDataKeysIS();
static void PV_GetDataKeysSI();
static void PV_GetDataKeysSS();
static void PV_GetDecor();
static void PV_DelDecor();
static void PV_SearchContainer();
static void PV_ChangeMap1();
static void PV_ChangeMap2();
static void PV_GetMapNo();
static void PV_PicScroll();
static void PV_SetPanel();
static void PV_FindPathMarker();
static void PV_ReSolid();
static void PV_SetPName();
static void PV_DelProp();
static void PV_QUpdate();

static void PV_AddRange();
static void PV_GridRange();
static void PV_AddButton();
static void PV_PushButton();
static void PV_RightClick();
static void PV_FlushMouse();
static void PV_RangePointer();
static void PV_TextButton();
static void PV_SaveScreen();
static void PV_RestoreScreen();
static void PV_fxGetXY();
static void PV_fxColour();
static void PV_fxAlpha();
static void PV_fxAlphaMode();
static void PV_fxRandom();
static void PV_fxLine();
static void PV_fxPoint();
static void PV_fxRect();
static void PV_fxPoly();
static void PV_fxOrbit();
static void PV_fxSprite();
static void PV_fxCorona();
static void PV_fxBlackCorona();
static void PV_fxAnimSprite();
static void PV_addstr();
static void PV_addstrn();
static void PV_setstr();
static void PV_strlen();
static void PV_strlen2();
static void PV_strsetpos();
static void PV_strgetpos();
static void PV_strgetpos2();
static void PV_strgetval();
static void PV_dofx();
static void PV_dofxS();
static void PV_dofxOver();
static void PV_dofxOverS();
static void PV_stopfx();

static void PV_CreateConsole();
static void PV_DestroyConsole();
static void PV_ClearConsole();
static void PV_ConsoleColour();
static void PV_ConsoleAddLine();
static void PV_ConsoleAddLineWrap();
static void PV_DrawConsole();
static void PV_ScrollConsole();
static void PV_FindString();
static void PV_FindTitle();
static void PV_AddJournal();
static void PV_AddJournal2();
static void PV_EndJournal();
static void PV_DrawJournal();
static void PV_DrawJournalDay();
static void PV_DrawJournalTasks();
static void PV_GetJournalDays();
static void PV_GetJournalDay();
static void PV_SetConsoleLine();
static void PV_SetConsolePercent();
static void PV_SetConsoleSelect();
static void PV_SetConsoleSelectS();
static void PV_SetConsoleSelectU();
static void PV_GetConsoleLine();
static void PV_GetConsolePercent();
static void PV_GetConsoleSelect();
static void PV_GetConsoleSelectU();
static void PV_GetConsoleLines();
static void PV_GetConsoleRows();

static void PV_CreatePicklist();
static void PV_DestroyPicklist();
static void PV_PicklistColour();
static void PV_PicklistPoll();
static void PV_PicklistAddLine();
static void PV_PicklistAddSave();
static void PV_PicklistItem();
static void PV_PicklistId();
static void PV_PicklistText();
static void PV_SetPicklistPercent();
static void PV_GetPicklistPercent();

static void PV_getvolume();
static void PV_setvolume();
static void PV_LastSaveSlot();
static void PV_SaveGame();
static void PV_LoadGame();
static void PV_GetDate();

static void PV_IfKey();
static void PV_IfKeyPressed();
static void PV_Printaddr();
static void PV_PrintLog();
static void PV_Dump();
static void PV_NOP();
static void PV_Dofus();

static void VMstacktrace();

static int OP_Invalid(int a, int b);
static int OP_Add(int a, int b);
static int OP_Sub(int a, int b);
static int OP_Mul(int a, int b);
static int OP_Div(int a, int b);
static int OP_Equal(int a, int b);
static int OP_NotEqual(int a, int b);
static int OP_Less(int a, int b);
static int OP_LessEq(int a, int b);
static int OP_Great(int a, int b);
static int OP_GreatEq(int a, int b);
static int OP_Modulus(int a, int b);
static int OP_And(int a, int b);
static int OP_Or(int a, int b);
static int OP_Xor(int a, int b);
static int OP_Nand(int a, int b);
static int OP_Shl(int a, int b);
static int OP_Shr(int a, int b);

static VMINT *GET_INT();
static VMINT *SAFE_INT();
static void GET_ARRAY_INFO(void **array, VMINT *size, VMINT *idx);
static void **CHECK_PTR();


static void SI_RecursiveLoop(OBJECT *obj, int funccall);

void InitVM();
void CallVM(char *function);
void CallVMnum(int pos);
void DumpVM(int force);
static void DumpVMtoconsole();
static void VMbug();
static VMTYPE *VMget(int bytes);
static void VMfree(VMTYPE *ptr);


static PEVM *AddVM(PEM *p);
static void DelVM();
static void microkernel();
static void Bang(char *errmsg);
static void VMstop(char *msg, char *deref);
static PEVM vmstack[VM_STACKLIMIT];
static int vmstack_ptr=0;

static PEVM *curvm;
static VMTYPE *lastip=NULL;

static void (*vmop[OPCODEMAX])(void);
static int (*Operator[32])(int x,int y);
static char *oplist[32];

// Code

#ifdef SAFER
	#define CHECK_VM(); if(!curvm) return NULL;
#else
	#define CHECK_VM(); ;
#endif

inline unsigned char GET_BYTE()
{
CHECK_VM();
return (*curvm->ip++).u8;
}

inline unsigned char UNGET_BYTE()
{
CHECK_VM();
return (*curvm->ip--).u8;
}

#define CHECK_POINTER_CORE(x,f,l) \
if(!x)\
	{\
	Bug("ERROR:  Null pointer in %s at %d\n",f,l);\
	VMstop("Null pointer!\n",NULL);\
	if(curvm) Bug("(ip = 0x%p)\n",(curvm->ip-curvm->code)*sizeof(VMTYPE));\
	return;\
	}

#define CHECK_POINTER(x); CHECK_POINTER_CORE(x,__FILE__,__LINE__);


inline VMINT GET_DWORD()
{
CHECK_VM();
curvm->ip++; // skip addressing word
return (*curvm->ip++).i32;
}

inline VMINT GET_RAW_DWORD()
{
CHECK_VM();
return (*curvm->ip++).i32;
}

inline VMINT *GET_DWORD_PTR()
{
CHECK_VM();
return (VMINT *)curvm->ip++; // Just get the address here, don't dereference it
}


inline void *GET_PTR()
{
VMTYPE *x;
CHECK_VM();
x = curvm->ip++;
return (void *)x->ptr;
}

inline char *GET_STR(VMINT len)
{
VMTYPE *x;
CHECK_VM();
x = curvm->ip;
curvm->ip = (VMTYPE *)((char *)curvm->ip + len);
return (char *)x;
}

inline VMINT *GET_MEMBER_PTR()
{
volatile char **obj;
volatile VMINT *ptr,**pointer=NULL;
volatile void *var2;
unsigned char refs,ctr;
char debugchain[2048];
char msg[128];

obj = (volatile char **)GET_PTR();
ptr = (VMINT *)*obj;

if(!ptr)
	{
	VMstop("DANGER!  Encountered NULL data structure in GetMemberPtr!\n",NULL);
	return NULL;
	}

if(DebugVM)
	sprintf(debugchain,"%p->",obj);

refs = GET_BYTE();

for(ctr=0;ctr<refs;ctr++)
	{
	var2 = (void *)GET_PTR();
	ptr = (VMINT *)((char *)ptr + (VMPTR)var2);
	pointer = (volatile VMINT **)ptr;
	ptr = (VMINT *)*ptr;
	if(DebugVM)
		{
		sprintf(msg,"%p->",pointer);
		strcat(debugchain,msg);
		}

	if(!ptr)
		{
		VMstop("DANGER!  NULL data lookup in GetMemberPtr!\n",debugchain);
		return NULL;
		}
	}
return (VMINT *)pointer;
}

inline VMINT *GET_ARRAY()
{
VMINT **array;
VMINT *idx;
VMINT size;

array = (VMINT **)GET_PTR();
size = GET_DWORD();
idx = (VMINT *)GET_INT();

if(!idx)
	{
	Bug("ERROR:  Array pointer corrupt\n");
	Bug("        Dumping VM to bootlog.txt...\n");
	DumpVM(1);
	Bug("        Current VM cannot continue.\n");
	VMbug();
	Bug("Terminating crashed VM...\n");
	DelVM();
	return NULL;
	}

// Make sure it doesn't exceed array bounds

if(*idx<1 || *idx > size)
	{
	Bug("ERROR:  Array access out of bounds\n");
	Bug("        Array stretches 1-%d, tried to access %d\n",size,*idx);
	Bug("        Dumping VM to bootlog.txt...\n");
	DumpVM(1);
	Bug("        Current VM cannot continue.\n");
	VMbug();
	Bug("Terminating crashed VM...\n");
	DelVM();
	return NULL;
	}

array += (*idx)-1; // Now points to appropriate element

M_chk(array);

return (VMINT *)array;
}


inline VMINT *CHECK_ARRAY()
{
VMINT **array;
VMINT *idx;
VMINT size;

array = (VMINT **)GET_PTR();
size = GET_DWORD();
idx = (VMINT *)GET_INT();

if(!idx)
	return NULL;

// Make sure it doesn't exceed array bounds

if(*idx<1 || *idx > size)
	return NULL; // Bad

if(!array)
	return NULL; // Even badder

array += (*idx)-1; // Point it where it ought to be

return (VMINT *)array;
}


static void GET_ARRAY_INFO(void **array, VMINT *length, VMINT *index)
{
VMINT **arr;
VMINT *idx;
int size;
VMTYPE *swap;
unsigned char flags;

swap = curvm->ip; // Store IP before processing

flags = GET_BYTE();
if(!(flags&ACC_ARRAY))
	{
	Bug("Array_info: Array is not an array!  Flag '%x' isn't %x\n",flags,ACC_ARRAY);
	Bug("         Dumping VM to bootlog.txt...\n");
	DumpVM(1);
	Bug("         Current VM cannot continue.\n");
	VMbug();
		Bug("Terminating crashed VM...\n");
	DelVM();
	return;
	}

arr = (VMINT **)GET_PTR();
size = GET_DWORD();
idx = (VMINT *)GET_INT();

// Store info
*array=(void *)arr;
*length=size;
*index=*idx;

curvm->ip = swap; // Restore pointer
}


inline VMINT *GET_MEMBER()
{
OBJECT **obj;
VMINT *ptr,**pointer=NULL;
void *var2;
unsigned char refs,ctr;
char debugchain[2048];
char msg[128];

obj = (OBJECT **)GET_PTR();
ptr = (VMINT *)*obj;
if(!ptr)
	{
	VMstop("DANGER!  NULL structure in GetMember\n",NULL);
	return NULL;
	}

if(DebugVM)
	sprintf(debugchain,"%p->",obj);

refs = GET_BYTE();
for(ctr=0;ctr<refs;ctr++)
	{
	var2 = (void *)GET_PTR();
	ptr = (VMINT *)((char *)ptr + (VMPTR)var2);
	pointer = (VMINT **)ptr;
	if(ptr)
		ptr = (VMINT *)*ptr;
	if(DebugVM)
		{
		sprintf(msg,"%p->",pointer);
		strcat(debugchain,msg);
		}

	if(!ptr) // Ignore the last one
	if(ctr<refs-1)
		{
		VMstop("DANGER!  NULL data lookup in GetMember!\n",debugchain);
		return NULL;
		}
	}
return (VMINT *)pointer;
}

// Traverse the chain and make sure it's accessible.
// Although we want to abort on an error, we MUST traverse the whole thing
// otherwise the VM's EIP will be pointing at at the wrong address and the
// VM will crash.

inline VMINT **CHECK_MEMBER_PTR()
{
OBJECT **obj;

// Mark all the pointers volatile to avoid gcc optimiser bug(?)
volatile VMINT *ptr=NULL;
volatile VMINT **pointer=NULL;
volatile void *var2;
volatile unsigned char refs,ctr;
int good;

good=1;
obj = (OBJECT **)GET_PTR();
if(!obj)
	good=0;
if(good)
	{
	ptr = (VMINT *)*obj;
	if(!ptr)
		good=0;
	}

refs = GET_BYTE();
for(ctr=0;ctr<refs;ctr++)
	{
	var2 = (void *)GET_PTR();
	if(good && ptr)
		{
		ptr = (VMINT *)((char *)ptr + (VMPTR)var2);
		pointer = (volatile VMINT **)ptr;
		ptr = (VMINT *)*ptr;
		}
	if(!ptr)
		good=0;
	}
if(good)
	return (VMINT **)pointer;
return NULL;
}

inline VMINT *CHECK_MEMBER()
{
VMINT **l=CHECK_MEMBER_PTR();
if(l)
	return *l;
return NULL;
}



inline VMINT *SKIP_MEMBER()
{
OBJECT **obj;
void *var2;
unsigned char refs,ctr;
obj = (OBJECT **)GET_PTR();
refs = GET_BYTE();
for(ctr=0;ctr<refs;ctr++)
	var2 = (void *)GET_PTR();
return (VMINT *)obj;
}

inline VMINT *GET_ARRAY_MEMBER_PTR()
{
volatile void *var2;
unsigned char refs,ctr;
volatile VMINT *ptr,**pointer=NULL;
volatile VMINT **array;
volatile VMINT *idx;
int size;
char msg[128];
char debugchain[2048];

array = (volatile VMINT **)GET_PTR();
//ilog_quiet("Marray = %p\n",array);
size = GET_DWORD();
//ilog_quiet("Msize = %d\n",size);
idx = (VMINT *)GET_INT();
//ilog_quiet("Midx = %p\n",idx);

refs = GET_BYTE();
//ilog_quiet("Refs = %d\n",refs);

if(!idx || !array)
	{
	VMstop("ERROR:  Array pointer corrupt\n",NULL);
	return NULL;
	}

// Make sure it doesn't exceed array bounds

if(*idx<1 || *idx > size)
	{
	VMstop("ERROR:  Array access out of bounds\n",NULL);
	Bug("Details: Array is 1-%d, tried to access %d\n",size,*idx);
	return NULL;
	}

array += (*idx)-1; // Now points to appropriate element (it had better, anyway)

// Okay, now do the member lookup
ptr = *array;

//refs = GET_BYTE();
for(ctr=0;ctr<refs;ctr++)
	{
	var2 = (void *)GET_PTR();
	ptr = (VMINT *)((char *)ptr + (VMPTR)var2);
	pointer = (volatile VMINT **)ptr;
	ptr = (VMINT *)*ptr;
	if(DebugVM)
		{
		sprintf(msg,"%p->",pointer);
		strcat(debugchain,msg);
		}

	if(!ptr) // Ignore the last one
		{
		VMstop("DANGER!  NULL data lookup in GetArrayMemberPtr\n",debugchain);
		return NULL;
		}
	}

return (VMINT *)pointer;
}

inline VMINT *GET_ARRAY_MEMBER()
{

// Mark all the pointers volatile to avoid gcc optimiser bug(?)
volatile void *var2;
unsigned char refs,ctr;
volatile VMINT *ptr,**pointer=NULL;
volatile VMINT **array;
volatile VMINT *idx;
int size;
char msg[128];
char debugchain[2048];

array = (volatile VMINT **)GET_PTR();
//ilog_quiet("array = %p\n",array);
size = GET_DWORD();
//ilog_quiet("size = %d\n",size);
idx = (VMINT *)GET_INT();
//ilog_quiet("idx = %p\n",idx);

refs = GET_BYTE();
//ilog_quiet("Refs = %d\n",refs);

if(!idx || !array)
	{
	VMstop("ERROR:  Array pointer corrupt\n",NULL);
	return NULL;
	}

// Make sure it doesn't exceed array bounds

if(*idx<1 || *idx > size)
	{
	VMstop("ERROR:  Array access out of bounds\n",NULL);
	Bug("Details: Array is 1-%d elements long, tried to access %d\n",size,*idx);
	return NULL;
	}

array += (*idx)-1; // Now points to appropriate element (it had better, anyway)

// Okay, now do the member lookup
ptr = *array;

//refs = GET_BYTE();
for(ctr=0;ctr<refs;ctr++)
	{
	var2 = (void *)GET_PTR();
/*
	if(!var2)
		{
		VMstop("DANGER!  NULL data lookup in GetArrayMember\n",debugchain);
		return NULL;
		}
*/

	ptr = (VMINT *)((char *)ptr + (VMPTR)var2);
	if(!ptr)
		{
		VMstop("DANGER!  NULL data lookup in GetArrayMember\n",debugchain);
		return NULL;
		}

	pointer = (volatile VMINT **)ptr;
	ptr = (VMINT *)*ptr;
	if(DebugVM)
		{
		sprintf(msg,"%p->",pointer);
		strcat(debugchain,msg);
		}

	if(!ptr) // Ignore the last one
		if(ctr<refs-1)
			{
			VMstop("DANGER!  NULL data lookup(2) in GetArrayMember\n",debugchain);
			return NULL;
			}
	}

return (VMINT *)pointer;
}

inline VMINT *SKIP_ARRAY_MEMBER()
{
void *var2;
unsigned char refs,ctr;
VMINT **array;
VMINT *idx;
int size=0;

array = (VMINT **)GET_PTR();
size = GET_DWORD();
idx = (VMINT *)GET_INT();

if(!idx)
	return NULL;

//ilog_quiet("(%d/%d) ",*idx,size);

refs = GET_BYTE();
//ilog_quiet("[Refs = %d]\n",refs);
for(ctr=0;ctr<refs;ctr++)
	{
	var2 = (void *)GET_PTR();
//	ilog_quiet("<%p> ",var2);
	}

// Make sure it doesn't exceed array bounds

if(*idx<1 || *idx > size)
	return NULL; // Bad

if(!array)
	return NULL;

//(*array) += *idx; // Now points to appropriate element (it had better, anyway)
//return (int *)array;
return NULL;
}



OBJECT **GET_OBJECT()
{
unsigned char flags;
flags = GET_BYTE();
if(flags&ACC_INDIRECT)
	return (OBJECT **)GET_PTR();
if(flags&ACC_MEMBER)
	return (OBJECT **)GET_MEMBER_PTR();
if(flags&ACC_ARRAY)
	return (OBJECT **)GET_ARRAY();
Bug("Fatal: Unknown data method '%x' in GET_OBJECT()\n",flags);
return NULL;
}

OBJECT **CHECK_OBJ(int warn)
{
OBJECT **obj = (OBJECT **)CHECK_PTR();
if(!obj || *obj == NULL)
	return NULL;

#ifdef PENDING_DELETE_CHECK
OBJECT *optr = *obj;
if(warn)
	{
	if(!(optr->flags & IS_ON))
		{
		Bug("Warning: Object %p '%s' is pending delete in CHECK_OBJ()\n",optr,optr->name);
		Bug("Flags = 0x%08x\n",optr->flags);
	//	return obj;
		return NULL;
		}
	}
#endif

//Bug("Severe: Object %p DOES NOT EXIST)\n",*obj);
return obj;
}

void **CHECK_PTR()
{
unsigned char flags;

flags = GET_BYTE();
VMTYPE *old=curvm->ip;

if(flags&ACC_INDIRECT)
	return (void **)GET_PTR();
if(flags&ACC_MEMBER)
	{
	if(CHECK_MEMBER_PTR())
		{
		// Wind back and do it properly
		curvm->ip=old;
		return (void **)GET_MEMBER_PTR();
		}
	return NULL;
	}
if(flags&ACC_ARRAY)
	{
	if(CHECK_ARRAY())
		{
		// Wind back and do it properly
		curvm->ip=old;
		return (void **)GET_ARRAY();
		}
	return NULL;
	}

Bug("Fatal: Unknown data method '%x' in CHECK_PTR()\n",flags);
return NULL;
}


VMINT *GET_INT()
{
unsigned char flags;
flags = GET_BYTE();
if(!flags)
	return GET_DWORD_PTR();
if(flags == ACC_INDIRECT)
	return (VMINT *)GET_PTR();
if(flags == ACC_MEMBER)
	return GET_MEMBER();
if(flags == ACC_ARRAY)
	return (VMINT *)GET_ARRAY();
if(flags == (ACC_ARRAY| ACC_MEMBER))
	return (VMINT *)GET_ARRAY_MEMBER();
Bug("Fatal: Unknown data method '%x' in GET_INT()\n",flags);
return NULL;
}

VMINT *SAFE_INT()
{
unsigned char flags;
flags = GET_BYTE();
if(!flags)
	return GET_DWORD_PTR();
if(flags == ACC_INDIRECT)
	return (VMINT *)GET_PTR();
if(flags == ACC_MEMBER)
	return SKIP_MEMBER();
if(flags == ACC_ARRAY)
	return (VMINT *)CHECK_ARRAY();
if(flags == (ACC_ARRAY | ACC_MEMBER))
	return (VMINT *)SKIP_ARRAY_MEMBER();
Bug("Fatal: Unknown data method '%x' in GET_INT()\n",flags);
return NULL;
}

char *GET_STRING()
{
unsigned char len;
unsigned char flags;
char **str;

flags = GET_BYTE();
if(!flags || flags & ACC_IMMSTR)
	{
	len = GET_DWORD();
	return GET_STR(len);
	}
if(flags == ACC_INDIRECT)
	{
	str = (char **)GET_PTR();
	if(!str) return NULL;
	return *str;
	}
if(flags == ACC_ARRAY) {
	str =(char**)GET_ARRAY();
	if(!str) return NULL;
	return *str;
	}
if(flags == ACC_MEMBER)
	{
	str = (char **)GET_MEMBER_PTR();
	if(!str) return NULL;
	return *str;
	}
if(flags == (ACC_ARRAY|ACC_MEMBER))
	{
	str = (char **)GET_ARRAY_MEMBER_PTR();
	if(!str) return NULL;
	return *str;
	}
Bug("Fatal: Unknown data method '%x' in GET_STRING()\n",flags);
return NULL;
}


/*
 *      VM functions follow
 */


void InitVM()
{
int ctr,bytes;

// Create the VM address space

// If the user overrode the default of 128k..
if(vcpu_memory < 1)
	vcpu_memory = VMEM;

// Allocate the address space
bytes = vcpu_memory*1024;
vmem = (VMTYPE *)M_get(1,bytes);
vmem_ptr = 0;  // High-water mark to zero
vmem_limit = bytes/sizeof(VMTYPE); // get limit in words

// Set up opcode jump table

for(ctr=0;ctr<OPCODEMAX;ctr++)
	vmop[ctr]=PV_Invalid;

// VMOP macro splices strings in preprocessor to make VM maintenance easier
// For example VMOP(Return);
// compiles as: vmop[PEVM_Return] = PV_Return;

VMOP(Return);
VMOP(Printint);
VMOP(Printstr);
VMOP(PrintXint);
VMOP(PrintCR);
VMOP(ClearScreen);
VMOP(ClearLine);
VMOP(Callfunc);
VMOP(CallfuncS);
VMOP(Let_iei);
VMOP(Let_ieipi);
VMOP(Let_pes);
VMOP(Let_seU);
VMOP(ClearArrayI);
VMOP(ClearArrayS);
VMOP(ClearArrayO);
VMOP(Add);
VMOP(Goto);
VMOP(If_iei);
VMOP(If_ses);
VMOP(If_oics);
VMOP(If_oeo);
VMOP(If_oEo);
VMOP(If_o);
VMOP(If_no);
VMOP(If_i);
VMOP(If_ni);
VMOP(If_p);
VMOP(If_np);
VMOP(For);
VMOP(Searchin);
VMOP(Listafter);
VMOP(SearchWorld);
VMOP(StopSearch);
VMOP(PlayMusic);
VMOP(StopMusic);
VMOP(IsPlaying);
VMOP(PlaySound);
VMOP(ObjSound);
VMOP(Create);
VMOP(CreateS);
VMOP(Destroy);
VMOP(SetFlag);
VMOP(GetFlag);
VMOP(ResetFlag);
VMOP(GetEngineFlag);
VMOP(If_on);
VMOP(If_non);
VMOP(If_tn);
VMOP(If_ntn);
VMOP(MoveObject);
VMOP(PushObject);
VMOP(XferObject);
VMOP(ShowObject);
VMOP(SetDarkness);
VMOP(GotoXY);
VMOP(PrintXYs);
VMOP(PrintXYi);
VMOP(PrintXYx);
VMOP(PrintXYcr);
VMOP(TextColour);
VMOP(TextColourDefault);
VMOP(GetTextColour);
VMOP(TextFont);
VMOP(GetTextFont);
VMOP(SetSequenceS);
VMOP(SetSequenceN);
VMOP(Lightning);
VMOP(Earthquake);
VMOP(Bestname);
VMOP(GetObject);
VMOP(GetSolidObject);
VMOP(GetFirstObject);
VMOP(GetTile);
VMOP(GetObjectBelow);
VMOP(GetBridge);
VMOP(ChangeObject);
VMOP(ChangeObjectS);
VMOP(ReplaceObject);
VMOP(ReplaceObjectS);
VMOP(SetDir);
VMOP(ForceDir);
VMOP(RedrawMap);
VMOP(RedrawText);
VMOP(MoveToPocket);
VMOP(XferToPocket);
VMOP(MoveFromPocket);
VMOP(ForceFromPocket);
VMOP(Spill);
VMOP(SpillXY);
VMOP(MoveContents);
VMOP(Empty);
VMOP(FadeOut);
VMOP(FadeIn);
VMOP(MoveToTop);
VMOP(MoveToFloor);
VMOP(InsertAfter);
VMOP(GetLOS);
VMOP(GetDistance);
VMOP(CheckHurt);
VMOP(IfSolid);
VMOP(IfVisible);
VMOP(SetUFlag);
VMOP(GetUFlag);
VMOP(If_Uflag);
VMOP(If_nUflag);
VMOP(SetLocal);
VMOP(GetLocal);
VMOP(If_onLocal);
VMOP(If_nonLocal);
VMOP(WeighObject);
VMOP(GetBulk);
VMOP(InputInt);
VMOP(IfInPocket);
VMOP(IfNInPocket);
VMOP(ReSyncEverything);
VMOP(TalkTo1);
VMOP(TalkTo2);
VMOP(Random);
VMOP(GetYN);
VMOP(GetYNN);
VMOP(Restart);
VMOP(ScrTileN);
VMOP(ScrTileS);
VMOP(ResetTileN);
VMOP(ResetTileS);
VMOP(While1);
VMOP(While2);
VMOP(Break);
VMOP(GetKey);
VMOP(GetKey_quiet);
VMOP(GetKeyAscii);
VMOP(GetKeyAscii_quiet);
VMOP(FlushKeys);
VMOP(DoAct);
VMOP(DoActS);
VMOP(DoActTo);
VMOP(DoActSTo);
VMOP(NextAct);
VMOP(NextActS);
VMOP(NextActTo);
VMOP(NextActSTo);
VMOP(InsAct);
VMOP(InsActS);
VMOP(InsActTo);
VMOP(InsActSTo);
VMOP(StopAct);
VMOP(ResumeAct);
VMOP(GetFuncP);
VMOP(GetContainer);
VMOP(SetLeader);
VMOP(SetMember);
VMOP(AddMember);
VMOP(DelMember);
VMOP(MoveTowards8);
VMOP(MoveTowards4);
VMOP(WaitFor);
VMOP(WaitForAnimation);
VMOP(If_oonscreen);
VMOP(If_not_oonscreen);
VMOP(TakeQty);
VMOP(GiveQty);
VMOP(MoveQty);
VMOP(CountQty);
VMOP(CopySchedule);
VMOP(CopySpeech);
VMOP(AllOnscreen);
VMOP(AllAround);
VMOP(CheckTime);
VMOP(FindNear);
VMOP(FindNearby);
VMOP(FindNearbyFlag);
VMOP(FindTag);
VMOP(FastTag);
VMOP(MakeTagList);
VMOP(FindNext);
VMOP(SetLight);
VMOP(SetLight_single);
VMOP(Printaddr);
VMOP(PrintLog);
VMOP(MovePartyToObj);
VMOP(MovePartyFromObj);
VMOP(MoveTag);
VMOP(UpdateTag);
VMOP(GetDataSSS);
VMOP(GetDataSSI);
VMOP(GetDataSIS);
VMOP(GetDataSII);
VMOP(GetDataISS);
VMOP(GetDataISI);
VMOP(GetDataIIS);
VMOP(GetDataIII);
VMOP(GetDataKeysII);
VMOP(GetDataKeysIS);
VMOP(GetDataKeysSI);
VMOP(GetDataKeysSS);
VMOP(GetDecor);
VMOP(DelDecor);
VMOP(SearchContainer);
VMOP(ChangeMap1);
VMOP(ChangeMap2);
VMOP(GetMapNo);
VMOP(PicScroll);
VMOP(SetPanel);
VMOP(FindPathMarker);
VMOP(ReSolid);
VMOP(SetPName);
VMOP(DelProp);
VMOP(QUpdate);

VMOP(AddRange);
VMOP(GridRange);
VMOP(AddButton);
VMOP(PushButton);
VMOP(RightClick);
VMOP(FlushMouse);
VMOP(RangePointer);
VMOP(TextButton);
VMOP(SaveScreen);
VMOP(RestoreScreen);
VMOP(fxGetXY);
VMOP(fxColour);
VMOP(fxAlpha);
VMOP(fxAlphaMode);
VMOP(fxRandom);
VMOP(fxLine);
VMOP(fxPoint);
VMOP(fxRect);
VMOP(fxPoly);
VMOP(fxOrbit);
VMOP(fxSprite);
VMOP(fxCorona);
VMOP(fxBlackCorona);
VMOP(fxAnimSprite);
VMOP(addstr);
VMOP(addstrn);
VMOP(setstr);
VMOP(strlen);
VMOP(strlen2);
VMOP(strsetpos);
VMOP(strgetpos);
VMOP(strgetpos2);
VMOP(strgetval);
VMOP(dofx);
VMOP(dofxS);
VMOP(dofxOver);
VMOP(dofxOverS);
VMOP(stopfx);
VMOP(IfKey);
VMOP(IfKeyPressed);

VMOP(CreateConsole);
VMOP(DestroyConsole);
VMOP(ClearConsole);
VMOP(ConsoleColour);
VMOP(ConsoleAddLine);
VMOP(ConsoleAddLineWrap);
VMOP(DrawConsole);
VMOP(ScrollConsole);
VMOP(FindString);
VMOP(FindTitle);
VMOP(AddJournal);
VMOP(AddJournal2);
VMOP(EndJournal);
VMOP(DrawJournal);
VMOP(DrawJournalDay);
VMOP(DrawJournalTasks);
VMOP(GetJournalDays);
VMOP(GetJournalDay);
VMOP(SetConsoleLine);
VMOP(SetConsolePercent);
VMOP(SetConsoleSelect);
VMOP(SetConsoleSelectS);
VMOP(SetConsoleSelectU);
VMOP(GetConsoleLine);
VMOP(GetConsolePercent);
VMOP(GetConsoleSelect);
VMOP(GetConsoleSelectU);
VMOP(GetConsoleLines);
VMOP(GetConsoleRows);

VMOP(CreatePicklist);
VMOP(DestroyPicklist);
VMOP(PicklistColour);
VMOP(PicklistPoll);
VMOP(PicklistAddLine);
VMOP(PicklistAddSave);
VMOP(PicklistItem);
VMOP(PicklistId);
VMOP(PicklistText);
VMOP(SetPicklistPercent);
VMOP(GetPicklistPercent);

VMOP(getvolume);
VMOP(setvolume);
VMOP(LastSaveSlot);
VMOP(SaveGame);
VMOP(LoadGame);
VMOP(GetDate);
// These are used internally by the compiler

VMOP(LastOp);
VMOP(Dump);
VMOP(NOP);
VMOP(Dofus);

// Now set up Operator jumptable

for(ctr=0;ctr<32;ctr++)
	Operator[ctr]=OP_Invalid;

Operator[1]=OP_Add;
Operator[2]=OP_Sub;
Operator[3]=OP_Div;
Operator[4]=OP_Mul;
Operator[5]=OP_Equal;
Operator[6]=OP_NotEqual;
Operator[7]=OP_Less;
Operator[8]=OP_LessEq;
Operator[9]=OP_Great;
Operator[10]=OP_GreatEq;
Operator[11]=OP_Modulus;
Operator[12]=OP_And;
Operator[13]=OP_Or;
Operator[14]=OP_Xor;
Operator[15]=OP_Nand;
Operator[16]=OP_Shl;
Operator[17]=OP_Shr;

oplist[0]="<!>";
oplist[1]="+";
oplist[2]="-";
oplist[3]="/";
oplist[4]="*";
oplist[5]="=";
oplist[6]="!=";
oplist[7]="<";
oplist[8]="<=";
oplist[9]=">";
oplist[10]=">=";
oplist[11]="MOD";
oplist[12]="AND";
oplist[13]="OR";
oplist[14]="XOR";
oplist[15]="NAND";
oplist[16]="SHL";
oplist[17]="SHR";

vm_up=1;
ilog_quiet("Script VM initialised\n");
}

/*
 *  A simple (but fast) stack-based memory allocator
 */

VMTYPE *VMget(int bytes)
{
unsigned int words,vp;
VMTYPE *pvm,*start;

// Now find out if it is divisible by a word and pad it if necessary
words = bytes/sizeof(VMTYPE);
if(bytes & (sizeof(VMTYPE)-1))
	words++;

// Add room for a control word
words++;

// Get start of block
start = &vmem[vmem_ptr];

// Make sure we're not out of memory
vp = vmem_ptr+words;
if(vp>vmem_limit)
	return NULL;
vmem_ptr=vp; // Start of next block

// Get the word at this point and go back to store block size
pvm = &vmem[vp];
pvm--;
(*pvm).u32 = words;

return start;
}

// Deallocator

void VMfree(VMTYPE *p)
{
VMTYPE *pvm;
unsigned int pos;

pvm=&vmem[vmem_ptr];
pvm--; // Should be control word

pos = (*pvm).u32;
pvm++; // Back across control word
pvm -= pos; // back this number of words

if(pvm != p)
	ithe_panic("VM address space corrupt deleting function",curvm->name);

vmem_ptr -= pos; // Just wind back the allocation pointer
}



char *CurrentVM()
{
return debcurfunc;
}

void CallVM(char *function)
{
PEVM *p;
int pos;

// Don't do this while we still have numbers instead of pointers
if(MZ1_LoadingGame)
	return;

if(!vm_up)
	return;
//	ithe_panic("Tried to use PEVM before InitVM()","Aiee!");

if(!function)
	{
	Bug("Tried to call NULL function\n");
	VMbug();
	return;
	}

strcpy(debcurfunc,function);

pos = getnum4PE(function);
if(pos == -1)
	{
	Bug("CallVM: PE script '%s' not found\n",function);
	return;
	}

if(show_vrm_calls)
	ilog_quiet("Call '%s'  (context %s)\n",debcurfunc,pevm_context);

p=AddVM(&PElist[pos]);
if(!p)
	return; // Stack error?
curvm = p;

DumpVM(0);

microkernel();
}

void VMbug()
{
if(!curvm)
	{
	Bug("Serious..  not in a function.  Error in game loop?\n");
	return;
	}
    
Bug("Error in function %s, in file %s (context %s)\n",curvm->name,curvm->file,pevm_context);

ilog_quiet("\n");
VMstacktrace();
}


void VMstop(char *msg, char *debugchain)
{
Bug(msg);

if(DebugVM)
	{
	Bug("         Dumping VM to bootlog.txt...\n");
	if(debugchain)
		{
		Bug("         Dereference chain:\n");
		Bug("         %sNULL\n",debugchain);
		}
	}
else
	Bug("         If repeatable, run IRE with -DebugVM for more info.\n");

DumpVM(1);

Bug("         Current VM cannot continue.\n");
VMbug();
Bug("Terminating crashed VM...\n");
DelVM();
}

void VMstacktrace()
{
int ctr;

if(!curvm)
	return;

ilog_quiet("VM Stack trace:\n");
for(ctr=vmstack_ptr;ctr>0;ctr--)
	ilog_quiet("  %d: %s (%s)\n",ctr,vmstack[ctr].name,vmstack[ctr].file);
}

void CallVMnum(int pos)
{
PEVM *p;

// Don't do this while we still have numbers instead of pointers
if(MZ1_LoadingGame)
	return;

if(!vm_up)
	ithe_panic("Tried to use PEVM before InitVM()","Aiee!");

if(pos < 0 || pos > PEtot)
	{
	Bug("CALL: Invalid function %d (0x%x)\n",pos,pos);
	VMbug();
	return;
	}

strcpy(debcurfunc,PElist[pos].name);
if(DebugVM||show_vrm_calls)
	ilog_quiet("Call '%s' (context %s)\n",debcurfunc,pevm_context);

p=AddVM(&PElist[pos]);
if(!p)
	return; // Stack error?
curvm = p;

DumpVM(0);

microkernel();
}


void microkernel()
{
unsigned int opcode;

if(!curvm)
	ithe_panic("VM started on an empty stomach",NULL);

do  {
	lastip = curvm->ip;    // Preserve IP (for error messages)
	opcode = (*curvm->ip++).u32; // Fetch
#ifdef EMERGENCY_QUIT
//	poll_keyboard();
	if(IRE_TestKey(IREKEY_PAUSE))
		if(IRE_TestShift(IRESHIFT_SHIFT))
			Bang("User Break");
#endif
	if(DebugVM)
		ilog_quiet("MK: %04x [%02x] %s\n",(lastip - curvm->code)*sizeof(VMTYPE),opcode,&pe_get_function(opcode)[1]);

	// Execute the instruction
	vmop[opcode&OPCODEMASK]();

	if(abortkernel)
		{
		abortkernel=0;
		return;
		}
	} while(curvm && curvm->ip);
}


void Bang(char *err)
{
char msg[128];

// We're going to die anyway, so disable break
ilog_break=0;

if(!curvm)
	{
	Bug("All virtual machines shut down.\n");
	return;
	}

VMbug();

ilog_quiet(">>> VIRTUAL MACHINE CRASHED...\n");
ilog_quiet(">>> DISASSEMBLY FOLLOWS...\n");
ilog_quiet(">>> IP = 0x%p\n",(lastip-curvm->code)*sizeof(VMTYPE));
ilog_quiet("==========================\n");
//DebugVM=1;
DumpVM(1);
ilog_quiet("==========================\n");

sprintf(msg,"In function %s in file %s, check bootlog.txt",curvm->name,curvm->file);
ithe_panic(err,msg);
}

// AddVM - create a new VM at the top of the stack

PEVM *AddVM(PEM *f)
{
PEVM *p;
// Absorb spurious abort codes
abortkernel=0;

if(vmstack_ptr<(VM_STACKLIMIT-1))
	{
	p=&vmstack[++vmstack_ptr];
	p->size = f->size;
	p->codelen = f->codelen;
//	p->code = (VMTYPE *)M_get(1,p->size);
	p->code = (VMTYPE *)VMget(p->size);
	if(!p->code)
		{
		Bang("Out of VCPU memory"); // time to die
		return NULL;
		}
	memcpy(p->code,f->code,p->size);
	p->name = f->name;
	p->file = f->file;
	p->ip = p->code;
	return p;
	}

// Oh shit.
Bang("Stack overflow.. too much recursion"); // time to die
return NULL;
}

// DelVM - pop the top Virtual Machine off the stack

void DelVM()
{
// Kill the running VM
abortkernel=1;

if(vmstack_ptr>0)
	{
	// Free existing VM
//	M_free(curvm->code);
	VMfree(curvm->code); // Free the VM
	// Get new current VM context and go away
	curvm=&vmstack[--vmstack_ptr];
	return;
	}

Bug("VM: Stack underflow.. no VM to delete");
}

void DumpVM(int force)
{
int ctr,len;
unsigned int proglen;
int data;
char *func;
unsigned int opcode,op;
VMTYPE *oldip;

if(!DebugVM && !force)
	return;

if(!curvm)
	{
	ilog_quiet("DumpVM: Not running.\n");
	return;
	}

if(!curvm->code)
	return;

ilog_quiet("(%s)\n",curvm->name);
ilog_quiet("(%lx-%lx)\n",(VMUINT)curvm->code,(VMUINT)curvm->code+(VMUINT)curvm->size);

oldip = curvm->ip;
curvm->ip = curvm->code;
proglen = (VMUINT)curvm->codelen + (VMUINT)curvm->ip;

for(;(VMUINT)curvm->ip<proglen;)
	{
	if(curvm->ip == lastip)
		ilog_quiet("-> ");
	else
		ilog_quiet("   ");
	// Calculate address
	ilog_quiet("%04x ",(curvm->ip-curvm->code)*sizeof(VMTYPE));
	opcode = (*curvm->ip++).u32; // Fetch
	func=pe_get_function(opcode);
	ilog_quiet(" [%02x] %s ",opcode,&func[1]); // Skip first byte (no of operands)
	len = func[0];
	for(ctr=0;ctr<len;ctr++)
		{
		M_chk(curvm->ip);
		op=GET_BYTE();
		UNGET_BYTE();
		switch(op)
			{
			case ACC_IMMEDIATE:
			data=GET_DWORD();
			// Hack to get a function name if applicable
			if(opcode== PEVM_Callfunc && data>0 && data<PEtot)
				ilog_quiet("'%s' ",PElist[data].name);
			else
				ilog_quiet("%d ",data);
			break;

			case ACC_INDIRECT:
			ilog_quiet("[%x] ",GET_DWORD());
			break;

			case ACC_MEMBER:
			ilog_quiet("[%x] ",SAFE_INT());
			break;

			case ACC_ARRAY:
			ilog_quiet("[%x] ",SAFE_INT());
			break;

			case ACC_ARRAY | ACC_MEMBER:
			ilog_quiet("[%x] ",SAFE_INT());
			break;

			case ACC_IMMSTR:
			ilog_quiet("\"%s\" ",GET_STRING());
			break;

			case ACC_JUMP:
			ilog_quiet(" jmp->%x ",GET_DWORD());
			break;

			case ACC_OPERATOR|1:
			GET_BYTE();
			break;

			default:
			if(op & ACC_OPERATOR)
				{
				ilog_quiet("%s ",oplist[op&OPMASK]);
				GET_BYTE();
				}
			else
				ilog_quiet("??%x?? ",op);
			};
		}
	if(len == -1)
		{
		ilog_quiet("%d ",GET_PTR());
		ilog_quiet("[%p] ",GET_PTR());
		}
	ilog_quiet("\n");
	}

curvm->ip = oldip;

}


void DumpVMtoconsole()
{
int ctr,len;
unsigned int proglen;
int data;
char *func;
unsigned int opcode,op;
VMTYPE *oldip;

if(!curvm)
	{
	printf("DumpVM: Not running.\n");
	return;
	}

if(!curvm->code)
	return;

printf("(%s)\n",curvm->name);
printf("(%lx-%lx)\n",(VMUINT)curvm->code,(VMUINT)curvm->code+(VMUINT)curvm->size);

oldip = curvm->ip;
curvm->ip = curvm->code;
proglen = (VMUINT)curvm->codelen + (VMUINT)curvm->ip;

for(;(VMUINT)curvm->ip<proglen;)
	{
	if(curvm->ip == lastip)
		printf("-> ");
	else
		printf("   ");
	// Calculate address
	printf("%04x ",(curvm->ip-curvm->code)*sizeof(VMTYPE));
	opcode = (*curvm->ip++).u32; // Fetch
	func=pe_get_function(opcode);
	printf(" [%02x] %s ",opcode,&func[1]); // Skip first byte (no of operands)
	len = func[0];
	for(ctr=0;ctr<len;ctr++)
		{
		M_chk(curvm->ip);
		op=GET_BYTE();
		UNGET_BYTE();
		switch(op)
			{
			case ACC_IMMEDIATE:
			data=GET_DWORD();
			// Hack to get a function name if applicable
			if(opcode== PEVM_Callfunc && data>0 && data<PEtot)
				printf("'%s' ",PElist[data].name);
			else
				printf("%d ",data);
			break;

			case ACC_INDIRECT:
			printf("[%x] ",GET_DWORD());
			break;

			case ACC_MEMBER:
			printf("[%x] ",SAFE_INT());
			break;

			case ACC_ARRAY:
			printf("[%x] ",SAFE_INT());
			break;

			case ACC_ARRAY | ACC_MEMBER:
			printf("[%x] ",SAFE_INT());
			break;

			case ACC_IMMSTR:
			printf("\"%s\" ",GET_STRING());
			break;

			case ACC_JUMP:
			printf(" jmp->%x ",GET_DWORD());
			break;

			case ACC_OPERATOR|1:
			GET_BYTE();
			break;

			default:
			if(op & ACC_OPERATOR)
				{
				printf("%s ",oplist[op&OPMASK]);
				GET_BYTE();
				}
			else
				printf("??%x?? ",op);
			};
		}
	if(len == -1)
		{
		printf("%d ",GET_PTR());
		printf("[%p] ",GET_PTR());
		}
	printf("\n");
	}

curvm->ip = oldip;

}


// Helper functions for VM opcodes

void SI_RecursiveLoop(OBJECT *obj, int funccall)
{
OBJECT *temp;

for(temp = obj;temp;temp=temp->next)
	if(temp->flags & IS_ON)
		{
		// Search cancelled?
		if(!SI_Search)
			return;
		current_object = temp;
		CallVMnum(funccall);
		if(temp->pocket.objptr)
			SI_RecursiveLoop(temp->pocket.objptr,funccall);
		}
}

/*
 *	Now the opcodes:
 */

void PV_Invalid()
{
char msg[128];
sprintf(msg,"Invalid VM opcode 0x%02lx",((*lastip).u32)&0xff);
Bang(msg);
}

void PV_Return()
{

DelVM();
if(DebugVM)
	DumpVM(0);
/*
if(curvm)
	ilog_quiet("VM now : %s\n",curvm->name);
else
	ilog_quiet("VM now empty\n");
*/
}

void PV_Printstr()
{
char *str;
str = GET_STRING();
//CHECK_POINTER(str);
if(!str)
	str="<null>";
irecon_printf(str); // Print it
if(DebugVM || printlogging)
	ilog_quiet("%s",str);
}

void PV_Printint()
{
VMINT *num;

num = GET_INT();
CHECK_POINTER(num);

irecon_printf("%ld",*num);
if(DebugVM || printlogging)
	ilog_quiet("%ld",*num);
}


void PV_PrintXint()
{
VMINT *num;
num = GET_INT();
CHECK_POINTER(num);

irecon_printf("%08lx",*num);       // Print it
if(DebugVM || printlogging)
	ilog_quiet("%08lx",*num);
}

void PV_PrintCR()
{
irecon_newline();
if(DebugVM || printlogging)
	ilog_quiet("\n");
}

void PV_ClearScreen()
{
irecon_cls();
}

void PV_ClearLine()
{
irecon_clearline();
}

void PV_Callfunc()
{
VMINT *func;

func = GET_INT();
CHECK_POINTER(func);
//if(DebugVM)
//	ilog_quiet("calling %d/%d\n",*func,PEtot);
CallVMnum(*func);
}

void PV_CallfuncS()
{
char *func;

func = GET_STRING();
CHECK_POINTER(func);
//if(DebugVM)
//	ilog_quiet("calling %s\n",func);
CallVM(func);
}

// LET <int> equal <number>

void PV_Let_iei()
{
VMINT *a,*b;

a = GET_INT();
CHECK_POINTER(a);
b = GET_INT();
CHECK_POINTER(b);

*a = *b;
}

// LET <int> equal <int> [operator] <int>

void PV_Let_ieipi()
{
VMINT *a,*b,*c;
unsigned char op;

a = GET_INT();
CHECK_POINTER(a);
b = GET_INT();
CHECK_POINTER(b);
op = GET_BYTE();
c = GET_INT();
CHECK_POINTER(c);

*a = Operator[op&OPMASK](*b,*c);
}

// LET <stringPtr> equal "string literal"

void PV_Let_pes()
{
VMINT **a,function;
char *b;
VMTYPE *p1,*p2;
unsigned int offset;

a = (VMINT **)GET_INT();
CHECK_POINTER(a);

b = GET_STRING();
CHECK_POINTER(b);

// Now we have the address of the string in the VCPU copy of the function
// Find the address in the original binary code

// First get the ID number of the currently running function
function = getnum4PE(curvm->name);
// I feel my eyes go really wide.
if(function == -1)
	ithe_panic("current function does not exist in let_pes",curvm->name);

// Now, work out the offset of the string into the bytecode
p1=(VMTYPE *)b;
p2=(VMTYPE *)curvm->code;
offset = (unsigned int)(p1 - p2); // The offset is in words not bytes

// Add this offset to the master copy and we should have the string
p1 = ADD_IP(PElist[function].code,offset*sizeof(VMTYPE*));

// Shove the address of the string into the variable
*a = (VMINT *)p1;
}


// LET <stringPtr> equal <UserString>

void PV_Let_seU()
{
char **a;
USERSTRING **b;

a = (char **)GET_INT();
CHECK_POINTER(a);

b = (USERSTRING **)GET_INT();
CHECK_POINTER(b);

// Point to the UserString
*a = (*b)->ptr;
}


// ADD <int> [operator] <int>

void PV_Add()
{
VMINT *a,*b;
unsigned char op;

a = GET_INT();
CHECK_POINTER(a);
op = GET_BYTE();
b = GET_INT();
CHECK_POINTER(b);

*a = Operator[op&OPMASK](*a,*b);
}

// Clear Array i[]

void PV_ClearArrayI()
{
VMINT *array;
VMINT size,idx,ctr;
GET_ARRAY_INFO((void **)&array, &size, &idx);
// Skip array member
GET_INT();

for(ctr=0;ctr<size;ctr++) {
	array[ctr]=0;
}
}

// Clear Array s[]

void PV_ClearArrayS()
{
char **array;
VMINT size,idx,ctr;
GET_ARRAY_INFO((void **)&array, &size, &idx);
// Skip array member
GET_STRING();

for(ctr=0;ctr<size;ctr++) {
	array[ctr]=NOTHING;
}
}

// Clear Array o[]

void PV_ClearArrayO()
{
OBJECT **array;
VMINT size,idx,ctr;
GET_ARRAY_INFO((void **)&array, &size, &idx);
// Skip array member
GET_OBJECT();

for(ctr=0;ctr<size;ctr++) {
	array[ctr]=NULL;
}
}


// Goto

void PV_Goto()
{
int jumpoffset;

jumpoffset = GET_DWORD();
curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// IF <int> = <number> then continue, else jump to offset

void PV_If_iei()
{
int jumpoffset;
VMINT *a,*b;
unsigned char op;

//DumpVM(1);

a = (VMINT *)GET_INT();
CHECK_POINTER(a);
op=GET_BYTE();
//ilog_quiet("Operator %d\n",op&OPMASK);
b = (VMINT *)GET_INT();
CHECK_POINTER(b);
jumpoffset = GET_DWORD();

if(!Operator[op&OPMASK](*a,*b))
	{
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
	}
}

// IF <string> = <string> then continue, else jump to offset

void PV_If_ses()
{
int jumpoffset;
char *a,*b;
unsigned char op;

a = GET_STRING();
CHECK_POINTER(a);
op=GET_BYTE();
b = GET_STRING();
CHECK_POINTER(b);
jumpoffset = GET_DWORD();

// Do the comparison, then compare to 0 via given op.  If zero, jump
if(!Operator[op&OPMASK](istricmp_fuzzy(a,b),0))
	{
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
	}
}

// IF <object> Is Called <string> then continue, else jump to offset
// The string is stored first, not second

void PV_If_oics()
{
int jumpoffset;
char *str;
OBJECT **obj;

//obj = GET_OBJECT();
obj = CHECK_OBJ(1);	// Allow a NULL object
//CHECK_POINTER(obj);
str = GET_STRING();
CHECK_POINTER(str);
jumpoffset = GET_DWORD();

// If there was an accident, assume it's not true and continue

if(!obj || *obj == NULL)
	{
/*
	Bug("If object is called '%s': Object has not been initialised!\n",str);
	DumpVM(1);
	VMbug();
*/
	// It doesn't exist, we'll count it as false and do the 'else' jump
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
	return;
	}

// Ok, do the condition

if(istricmp_fuzzy((*obj)->name,str))
	{
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
	}
}

// IF <object> = <object> then continue, else jump to offset

void PV_If_oeo()
{
int jumpoffset;
OBJECT **a,**b,*c1,*c2;

a = CHECK_OBJ(1);
b = CHECK_OBJ(1);
jumpoffset = GET_DWORD();

// Just because it's NULL doesn't mean it can't be equal

if(a)
	c1=*a;
else
	c1=(OBJECT *)a;

if(b)
	c2=*b;
else
	c2=(OBJECT *)b;

// Ok, try the condition

if(!(c1 == c2))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// IF <object> != <object> then continue, else jump to offset

void PV_If_oEo()
{
int jumpoffset;
OBJECT **a,**b,*c1,*c2;

a = CHECK_OBJ(1);
b = CHECK_OBJ(1);
jumpoffset = GET_DWORD();

// Just because it's NULL doesn't mean it can't be equal

if(a)
	c1=*a;
else
	c1=(OBJECT *)a;

if(b)
	c2=*b;
else
	c2=(OBJECT *)b;

// Ok, try the condition

if(!(c1 != c2))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// IF <object> then continue, else jump to offset

void PV_If_o()
{
int jumpoffset;
OBJECT **obj;
int ok;

obj = CHECK_OBJ(0);
jumpoffset = GET_DWORD();

//printf("obj = %p\n",obj);

ok=0;
if(obj)
	if(*obj)
		ok=1;

if(!ok)
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

void PV_If_no()
{
int jumpoffset;
OBJECT **obj;
int ok;

obj = CHECK_OBJ(0);
jumpoffset = GET_DWORD();

ok=0;
if(obj)
	if(*obj)
		ok=1;

if(ok)
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}


// IF <integer> then continue, else jump to offset

void PV_If_i()
{
int jumpoffset;
VMINT *a;
VMINT ok;

a = (VMINT *)GET_INT();
CHECK_POINTER(a);
jumpoffset = GET_DWORD();

ok=0;
if(a && *a)
	ok=1;

if(!ok)
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

void PV_If_ni()
{
int jumpoffset;
VMINT *a;
int ok;

a = (VMINT *)GET_INT();
CHECK_POINTER(a);
jumpoffset = GET_DWORD();

ok=0;
if(a && *a)
	ok=1;

if(ok)
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// IF <generic pointer> then continue, else jump to offset

void PV_If_p()
{
int jumpoffset;
void **a;

a = CHECK_PTR();
jumpoffset = GET_DWORD();

// Do the comparison, then compare to 0 via given op.  If zero, jump
if(!a || *a == NULL)
	{
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
	}
}

// IF !<generic pointer> then continue, else jump to offset

void PV_If_np()
{
int jumpoffset;
void **a;

a = CHECK_PTR();
jumpoffset = GET_DWORD();

// Do the comparison, then compare to 0 via given op.  If zero, jump
if(a && *a != NULL)
	{
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
	}
}



// FOR <integer> = <number> TO <number> (continue, unless false, then jump exit)

void PV_For()
{
int jumpoffset;
VMINT *control,*start,*end,delta,oob;
VMTYPE *firsttime;

control = GET_INT();
CHECK_POINTER(control);
start = GET_INT();
CHECK_POINTER(start);
end = GET_INT();
CHECK_POINTER(end);

firsttime = curvm->ip++;      // This is the 'first time' flag
jumpoffset = GET_DWORD();

if(*start<*end)
    delta=1;
else
    delta=-1;

// Kludge to make it go the whole range

if((*firsttime).u32 & 1)
    {
    *control-=delta;
    (*firsttime).u32 = ACC_OPERATOR;
    }

*control+=delta;

// Check for out of bounds

oob=0;
if(delta > 0)
   {
   if(*control > *end)
       oob=1;
   }
else
   {
   if(*control < *end)
       oob=1;
   }

// If we're out of bounds, stop

if(oob)
    {
    curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
    (*firsttime).u32=1|ACC_OPERATOR;           // Reset the first-time flag
    }
}

// Search inside an object's pockets calling a function as we go

void PV_Searchin()
{
OBJECT **obj;
OBJREGS objvars;
int func;

// First get the object we're going to search inside of

obj = GET_OBJECT();
CHECK_POINTER(obj);
func = GET_DWORD();

if(!*obj)
	{
	Bug("Search_inside Obj calling <func>: Obj not initialised!\n");
	DumpVM(1);
	VMbug();
	return;
	}

VM_SaveRegs(&objvars);
SI_Search=1;
SI_RecursiveLoop((*obj)->pocket.objptr,func);
SI_Search=0;
VM_RestoreRegs(&objvars);
}

// List all objects after this one calling a function as we go

void PV_Listafter()
{
OBJECT **obj;
OBJECT *ptr;
OBJREGS objvars;
int func;

// First get the object we're going to search

obj = GET_OBJECT();
CHECK_POINTER(obj);
func = GET_DWORD();

if(!*obj)
	{
	Bug("List_After Obj calling <func>: Obj not initialised!\n");
	DumpVM(1);
	VMbug();
	return;
	}

VM_SaveRegs(&objvars);
SI_Search=1;
for(ptr=*obj;ptr;ptr=ptr->next)
	{
	// Search cancelled?
	if(!SI_Search)
		break;
	current_object=ptr;
	CallVMnum(func);
	}
SI_Search=0;
VM_RestoreRegs(&objvars);
}

// Search all the world calling a function as we go

void PV_SearchWorld()
{
OBJLIST *t;
OBJREGS objvars;
int func;

func = GET_DWORD();

VM_SaveRegs(&objvars);
SI_Search=1;
for(t=MasterList;t;t=t->next)
	{
	// Search cancelled?
	if(!SI_Search)
		break;
	if(t->ptr)
		if(t->ptr->flags & IS_ON)
			{
			current_object = t->ptr;
			CallVMnum(func);
			}
	}
VM_RestoreRegs(&objvars);
}

// Stop a search if one is in progress

void PV_StopSearch()
{
SI_Search=0;
}


void PV_Create()
{
OBJECT **obj,*temp;
VMINT *var;

obj = GET_OBJECT();
CHECK_POINTER(obj);
var = GET_INT();
CHECK_POINTER(var);

// Is it decorative?
temp = OB_Decor_num(*var);
if(!temp)
	{
	temp = OB_Alloc();
	temp->curdir = temp->dir[0];
	OB_Init(temp,CHlist[*var].name);
	OB_SetDir(temp,CHAR_D,FORCE_SHAPE);
	TransferToPocket(temp,syspocket);
	CreateContents(temp);

	*obj = temp;
	}
else
	*obj = NULL;
}

void PV_CreateS()
{
OBJECT **obj,*temp;
char *str;
int type;

obj = GET_OBJECT();
CHECK_POINTER(obj);
str = GET_STRING();
CHECK_POINTER(str);

type = getnum4char(str);
if(type == -1)
	{
	Bug("Create obj = %s : Could not find character '%s'\n",str,str);
	DumpVM(1);
	VMbug();
	*obj = NULL;
	return;
	}

// Is it decorative?
temp = OB_Decor(str);
if(!temp)
	{
	temp = OB_Alloc();
	temp->curdir = temp->dir[0];
	OB_Init(temp,CHlist[type].name);
	OB_SetDir(temp,CHAR_D,FORCE_SHAPE);
	TransferToPocket(temp,syspocket);
	CreateContents(temp);
	*obj = temp;
	}
else
	*obj = NULL;
}

void PV_Destroy()
{
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);

// DumpVMtoconsole();

if(*obj == player)
	{
	Bug("Attempted to destroy the player!\n");
	VMbug();
	return;
	}

if(*obj)
	{
	if((*obj)->flags & IS_DECOR)	// Mustn't remove a decor, that's bad
		return;

//	ilog_printf("pevm: destroy %s\n",(*obj)->name);
//	ilog_printf("pevm: destroy %p (%s)\n",(*obj),(*obj)->name);
	DeleteObject(*obj);
	}

*obj = NULL;
}


void PV_PlayMusic()
{
S_PlayMusic(GET_STRING());
}

void PV_StopMusic()
{
S_StopMusic();
}

void PV_IsPlaying()
{
VMINT *a;

a = GET_INT();
CHECK_POINTER(a);

// Get result of music player
*a=S_IsPlaying();
}

void PV_PlaySound()
{
S_PlaySample(GET_STRING(),sf_volume);
}

void PV_ObjSound()
{
OBJECT **obj;
char *str;

str = GET_STRING();
CHECK_POINTER(str);
obj = GET_OBJECT();
CHECK_POINTER(obj);

object_sound(str, *obj);
//S_PlaySample(GET_STRING(),sf_volume);
}

void PV_SetDarkness()
{
VMINT *var,dl;
var = GET_INT();
CHECK_POINTER(var);
dl = *var;
if(dl<0)
	dl=0;
if(dl>255)
	dl=255;

SetDarkness(dl);
}

// set_flag <object> <flag> = <i>

void PV_SetFlag()
{
VMINT *var,*flag;
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
flag = GET_INT();
CHECK_POINTER(flag);
var = GET_INT();
CHECK_POINTER(var);

if(!obj || *obj == NULL)
	return;
if((*obj)->flags & IS_DECOR)	// Mustn't change a decor, that's bad
	return;
set_flag(*obj,*flag,(*var) != 0);
}

// get_flag <i> = <object> <flag>

void PV_GetFlag()
{
VMINT *var,*flag;
OBJECT **obj;

var = GET_INT();
CHECK_POINTER(var);
obj = GET_OBJECT();
CHECK_POINTER(obj);
flag = GET_INT();
CHECK_POINTER(flag);

*var = get_flag(*obj,*flag);
}

// reset_flag <object> <flag>

void PV_ResetFlag()
{
VMINT *flag;
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
flag = GET_INT();
CHECK_POINTER(flag);

if((*obj)->flags & IS_DECOR)	// Mustn't change a decor, that's bad
	return;

default_flag(*obj,*flag);
}

// get_flag <i> = <object> <flag>

void PV_GetEngineFlag()
{
VMINT *var,*flag;
OBJECT **obj;

var = GET_INT();
CHECK_POINTER(var);
obj = GET_OBJECT();
CHECK_POINTER(obj);
flag = GET_INT();
CHECK_POINTER(flag);

*var = ((*obj)->engineflags & (*flag)) ? 1 : 0;
}


// IF <object> <flag> then continue, else jump to offset

void PV_If_on()
{
int jumpoffset;
VMINT *flag;
OBJECT **obj;

obj = CHECK_OBJ(1);	// Allow a NULL object

flag = GET_INT();
CHECK_POINTER(flag);
jumpoffset = GET_DWORD();

// If we don't have a valid object, return false
if(!obj)
	{
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
	return;
	}

if(!get_flag(*obj,*flag))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// IF NOT <object> <flag> then continue, else jump to offset

void PV_If_non()
{
int jumpoffset;
VMINT *flag;
OBJECT **obj;

obj = CHECK_OBJ(1);	// Allow a NULL object

flag = GET_INT();
CHECK_POINTER(flag);
jumpoffset = GET_DWORD();

// If it's null, assume it's false (do nothing)
if(!obj)
	return;

if(get_flag(*obj,*flag))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// IF <tile> <flag> then continue, else jump to offset

void PV_If_tn()
{
int jumpoffset;
VMINT *flag;
TILE **tile;

tile = (TILE **)GET_OBJECT();
CHECK_POINTER(tile);
flag = GET_INT();
CHECK_POINTER(flag);
jumpoffset = GET_DWORD();

if(!get_tileflag(*tile,*flag))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// IF NOT <tile> <flag> then continue, else jump to offset

void PV_If_ntn()
{
int jumpoffset;
VMINT *flag;
TILE **tile;

tile = (TILE **)GET_OBJECT();
CHECK_POINTER(tile);
flag = GET_INT();
CHECK_POINTER(flag);
jumpoffset = GET_DWORD();

if(get_tileflag(*tile,*flag))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// IF <object> <localflag> then continue, else jump to offset

void PV_If_onLocal()
{
int jumpoffset;
OBJECT **obj;
char *str;

obj = CHECK_OBJ(1);	// Allow a NULL object

str = GET_STRING();
CHECK_POINTER(str);

jumpoffset = GET_DWORD();

// If we don't have a valid object, return false
if(!obj)
	{
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
	return;
	}

char *pname = NPC_MakeName(player);
if(!NPC_get_lFlag(*obj,str,pname))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
M_free(pname);
}

// IF NOT <object> <flag> then continue, else jump to offset

void PV_If_nonLocal()
{
int jumpoffset;
OBJECT **obj;
char *str;

obj = CHECK_OBJ(1);	// Allow a NULL object

str = GET_STRING();
CHECK_POINTER(str);

jumpoffset = GET_DWORD();

// If we don't have a valid object, do nothing
if(!obj)
	return;

char *pname = NPC_MakeName(player);
if(NPC_get_lFlag(*obj,str,pname))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
M_free(pname);
}


// Move <object> <x> <y> and set pe_result_ok accordingly

void PV_MoveObject()
{
VMINT *x,*y;
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

// Do it and record the result

if(MoveObject(*obj,*x,*y,0))
	pevm_err=0;
else
	pevm_err=-1;
}


// Move <object> <x> <y> (slightly more forcibly) and set pe_result_ok accordingly

void PV_PushObject()
{
VMINT *x,*y;
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

// Do it and record the result

if(MoveObject(*obj,*x,*y,1))
	pevm_err=0;
else
	pevm_err=-1;
}

// Forcibly move <object> <x> <y>

void PV_XferObject()
{
VMINT *x,*y;
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

transfer_object(*obj,*x,*y);
}

// Display <object> onscreen at <x> <y>

void PV_ShowObject()
{
VMINT *x,*y;
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

if(!*obj)
	return;
//draw_rle_sprite(swapscreen,(*obj)->form->seq[0]->image,*x,*y);
(*obj)->form->seq[0]->image->Draw(swapscreen,*x + (*obj)->form->xoff,*y + (*obj)->form->yoff) ;
}

void PV_PrintXYs()
{
char *str;
str = GET_STRING();
CHECK_POINTER(str);

irecon_printxy(cur_x,cur_y,str); // Print it
if(curfont)
	cur_x += curfont->Length(str);
}

void PV_PrintXYi()
{
VMINT *num;
char str[128];
int inum;

num = GET_INT();
CHECK_POINTER(num);

//printf("PrintXYI '%ld' (%d)\n",*num,*num);

inum = *num;
sprintf(str,"%d",inum);
irecon_printxy(cur_x,cur_y,str); // Print it
if(curfont)
	cur_x += curfont->Length(str);
}

void PV_PrintXYx()
{
VMINT *num;
char str[128];

num = GET_INT();
CHECK_POINTER(num);

sprintf(str,"%08lx",*num);
irecon_printxy(cur_x,cur_y,str); // Print it
if(curfont)
	cur_x += curfont->Length(str);
}

void PV_PrintXYcr()
{
cur_x = cur_margin;
if(curfont)
	cur_y += curfont->Height();
}

void PV_GotoXY()
{
VMINT *x,*y;

x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

cur_x = *x;
cur_margin = cur_x;
cur_y = *y;
}

void PV_TextColour()
{
VMINT *r,*g,*b;

r = GET_INT();
CHECK_POINTER(r);
g = GET_INT();
CHECK_POINTER(g);
b = GET_INT();
CHECK_POINTER(b);

irecon_colour(*r,*g,*b);
}

void PV_TextColourDefault()
{
irecon_colour(dtext_r, dtext_g, dtext_b);
}

void PV_GetTextColour()
{
VMINT *r,*g,*b;

r = GET_INT();
CHECK_POINTER(r);
g = GET_INT();
CHECK_POINTER(g);
b = GET_INT();
CHECK_POINTER(b);

int ir,ig,ib;
irecon_getcolour(&ir,&ig,&ib);
*r=ir;
*g=ig;
*b=ib;
}

void PV_TextFont()
{
VMINT *f;

f = GET_INT();
CHECK_POINTER(f);

irecon_font(*f);
}

void PV_GetTextFont()
{
VMINT *f;

f = GET_INT();
CHECK_POINTER(f);

*f=irecon_getfont();
}


void PV_SetSequenceS()
{
char *str;
OBJECT **obj;
obj = GET_OBJECT();
CHECK_POINTER(obj);
str = GET_STRING();
CHECK_POINTER(str);

OB_SetSeq(*obj,str);
}

void PV_SetSequenceN()
{
OBJECT **obj;
VMINT *n;
obj = GET_OBJECT();
CHECK_POINTER(obj);
n = GET_INT();
CHECK_POINTER(n);

OB_SetSeq(*obj,SQlist[*n].name);
}

void PV_Lightning()
{
VMINT *n;

n=GET_INT();
CHECK_POINTER(n);

do_lightning=*n;
}

void PV_Earthquake()
{
VMINT *n;

n=GET_INT();
CHECK_POINTER(n);

do_earthquake=*n;
}

void PV_Bestname()
{
OBJECT **obj;
obj = GET_OBJECT();
CHECK_POINTER(obj);

irecon_printf(BestName(*obj)); // Print it
if(DebugVM || printlogging)
	ilog_quiet(BestName(*obj));
}

void PV_GetObject()
{
OBJECT **obj;
VMINT *x,*y;
obj = (OBJECT **)GET_INT(); // Hack: get pointer without final NULL check
CHECK_POINTER(obj);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

*obj = GameGetObject(*x,*y);
}

void PV_GetSolidObject()
{
OBJECT **obj;
VMINT *x,*y;
obj = (OBJECT **)GET_INT(); // Hack: get pointer without final NULL check
CHECK_POINTER(obj);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

*obj = GetSolidObject(*x,*y);
}

void PV_GetFirstObject()
{
OBJECT **obj;
VMINT *x,*y;
obj = (OBJECT **)GET_INT(); // Hack: get pointer without final NULL check
CHECK_POINTER(obj);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

*obj = GetObjectBase(*x,*y);
}

void PV_GetTile()
{
TILE **tile;
VMINT *x,*y;
tile = (TILE **)GET_INT(); // Hack: get pointer without final NULL check
CHECK_POINTER(tile);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

*tile = GetTile(*x,*y);
}

void PV_GetObjectBelow()
{
OBJECT **obj1,**obj2;
obj1 = (OBJECT **)GET_INT(); // Hack: get pointer without final NULL check
CHECK_POINTER(obj1);
obj2 = GET_OBJECT();
CHECK_POINTER(obj2);

*obj1 = get_object_below(*obj2);
}

void PV_GetBridge()
{
OBJECT **obj;
VMINT *x,*y;
obj = (OBJECT **)GET_INT(); // Hack: get pointer without final NULL check
CHECK_POINTER(obj);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

*obj = GetBridge(*x,*y);
}

void PV_ChangeObject()
{
OBJECT **obj;
VMINT *var;

STATS ostats;
char resname[32];
char pname[32];
char speech[128];
char user1[128];
char user2[128];
int tcache;
VMUINT partie;     // Must preserve this for dead party members

obj = GET_OBJECT();
CHECK_POINTER(obj);
CHECK_POINTER(*obj);

var = GET_INT();
CHECK_POINTER(var);

if(!*obj)
	{
	if(*var)
		Bug("ChangeObject <null> %s\n",CHlist[*var].name);
	else
		Bug("ChangeObject <null> <null>\n");
	return;
	}

//partie = (*obj)->flags & IS_PARTY; // This is in stats now
memcpy(&ostats,(*obj)->stats,sizeof(STATS));
strcpy(resname,(*obj)->funcs->resurrect);
strcpy(speech,(*obj)->funcs->talk);
strcpy(user1,(*obj)->funcs->user1);
strcpy(user2,(*obj)->funcs->user2);
tcache = (*obj)->funcs->tcache;
strcpy(pname,(*obj)->personalname);
OB_Init(*obj,CHlist[*var].name);
memcpy((*obj)->stats,&ostats,sizeof(STATS));
strcpy((*obj)->funcs->resurrect,resname);
strcpy((*obj)->personalname,pname);
strcpy((*obj)->funcs->talk,speech);
strcpy((*obj)->funcs->user1,user1);
strcpy((*obj)->funcs->user2,user2);
//(*obj)->flags &= ~IS_PARTY;
//(*obj)->flags |= partie;

(*obj)->funcs->tcache = tcache;
AL_Add(&ActiveList,*obj); // Make active (if it can be)
}


void PV_ChangeObjectS()
{
OBJECT **obj;
char *str;
int type;

STATS ostats;
char resname[32];
char pname[32];
char speech[128];
int tcache;
VMUINT partie;     // Must preserve this for dead party members

obj = GET_OBJECT();
CHECK_POINTER(obj);
CHECK_POINTER(*obj);
str = GET_STRING();
CHECK_POINTER(str);

type = getnum4char(str);
if(type == -1)
	{
	Bug("Change obj %s : Could not find character '%s'\n",str,str);
	DumpVM(1);
	VMbug();
	return;
	}

//partie = (*obj)->flags & IS_PARTY;
memcpy(&ostats,(*obj)->stats,sizeof(STATS));
strcpy(resname,(*obj)->funcs->resurrect);
strcpy(speech,(*obj)->funcs->talk);
tcache = (*obj)->funcs->tcache;
strcpy(pname,(*obj)->personalname);
OB_Init(*obj,CHlist[type].name);
memcpy((*obj)->stats,&ostats,sizeof(STATS));
strcpy((*obj)->funcs->resurrect,resname);
strcpy((*obj)->personalname,pname);
strcpy((*obj)->funcs->talk,speech);
//(*obj)->flags &= ~IS_PARTY;
//(*obj)->flags |= partie;
(*obj)->funcs->tcache = tcache;
AL_Add(&ActiveList,*obj); // Make active (if it can be)
}

void PV_ReplaceObject()
{
OBJECT **obj;
VMINT *var,v;

obj = GET_OBJECT();
CHECK_POINTER(obj);
CHECK_POINTER(*obj);
var = GET_INT();
CHECK_POINTER(var);

v=*var;
if(v<0 || v>=CHtot)
if(v == -1)
	{
	Bug("Replace obj %d : Could not find character type %d\n",v,v);
	DumpVM(1);
	VMbug();
	return;
	}

replace_object(*obj,CHlist[v].name);

/*
k=fullrestore;
fullrestore=0;
OB_Init(*obj,CHlist[*var].name);
fullrestore=k;
gen_largemap();     // Recalc solid objects cache
AL_Add(&ActiveList,*obj); // Make active (if it can be)
*/
}

void PV_ReplaceObjectS()
{
OBJECT **obj;
int type;
char *str;

obj = GET_OBJECT();
CHECK_POINTER(obj);
CHECK_POINTER(*obj);
str = GET_STRING();
CHECK_POINTER(str);

type = getnum4char(str);
if(type == -1)
	{
	Bug("Replace obj %s : Could not find character '%s'\n",str,str);
	DumpVM(1);
	VMbug();
	return;
	}

replace_object(*obj,CHlist[type].name);

/*
k=fullrestore;
fullrestore=0;
OB_Init(*obj,CHlist[type].name);
fullrestore=k;
gen_largemap();     // Recalc solid objects cache
AL_Add(&ActiveList,*obj); // Make active (if it can be)
*/
}

void PV_SetDir()
{
OBJECT **obj;
VMINT *n;
obj = GET_OBJECT();
CHECK_POINTER(obj);
n = GET_INT();
CHECK_POINTER(n);
pevm_err=OB_SetDir(*obj,*n,FORCE_FRAME);
}

void PV_ForceDir()
{
OBJECT **obj;
VMINT *n;
obj = GET_OBJECT();
CHECK_POINTER(obj);
n = GET_INT();
CHECK_POINTER(n);
pevm_err=OB_SetDir(*obj,*n,FORCE_SHAPE);
}

void PV_RedrawText()
{
irecon_update();            // Redraw all text output
Show();                     // And blit the screen again
IRE_WaitFor(0);	// Yield to OS
}

void PV_RedrawMap()
{
RedrawMap();
Show();                     // And blit the screen again
IRE_WaitFor(0);	// Yield to OS
}

// Forcibly move <object> <to> <pocket>

void PV_MoveToPocket()
{
OBJECT **pocket,**obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
pocket = GET_OBJECT();
CHECK_POINTER(pocket);

move_to_pocket(*obj,*pocket);
}

// Forcibly move <object> <to> <pocket>

void PV_XferToPocket()
{
OBJECT **pocket,**obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
pocket = GET_OBJECT();
CHECK_POINTER(pocket);

transfer_to_pocket(*obj,*pocket);
}

// move <object> <from> <pocket> to <x> <y>

void PV_MoveFromPocket()
{
OBJECT **pocket,**obj;
VMINT *x,*y;

obj = GET_OBJECT();
CHECK_POINTER(obj);
pocket = GET_OBJECT();
CHECK_POINTER(pocket);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

if(MoveFromPocket(*obj,*pocket,*x,*y))
	pevm_err=0;
else
	pevm_err=-1;
}

// Forcibly move <object> <from> <pocket> to <x> <y>

void PV_ForceFromPocket()
{
OBJECT **pocket,**obj;
VMINT *x,*y;

obj = GET_OBJECT();
CHECK_POINTER(obj);
pocket = GET_OBJECT();
CHECK_POINTER(pocket);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

ForceFromPocket(*obj,*pocket,*x,*y);
}

// Spill contents of <object> at current location

void PV_Spill()
{
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);

spillcontents(*obj,(*obj)->x,(*obj)->y);
}

// Spill contents of <object> at <X> <Y>

void PV_SpillXY()
{
OBJECT **obj;
VMINT *x,*y;

obj = GET_OBJECT();
CHECK_POINTER(obj);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

spillcontents(*obj,*x,*y);
}

// move contents of <object> to <object>

void PV_MoveContents()
{
OBJECT **src;
OBJECT **dest;

src = GET_OBJECT();
CHECK_POINTER(src);

dest = GET_OBJECT();
CHECK_POINTER(dest);

movecontents(*src,*dest);
}


// Erase contents of <object>

void PV_Empty()
{
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);

EraseContents(*obj);
}

// Fade Out

void PV_FadeOut()
{
vrmfade_out();
}

// Fade Out

void PV_FadeIn()
{
vrmfade_in();
}

// Move object to top of the heap

void PV_MoveToTop()
{
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);

MoveToTop(*obj);
}

// Move object to bottom of the heap

void PV_MoveToFloor()
{
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);

MoveToFloor(*obj);
}

// Move object to after another one

void PV_InsertAfter()
{
OBJECT **dest;
OBJECT **obj;

dest = GET_OBJECT();
CHECK_POINTER(dest);

obj = GET_OBJECT();
CHECK_POINTER(obj);

InsertAfter(*dest,*obj);
}


// Get line-of-sight between two objects, return number of steps

void PV_GetLOS() {
VMINT *x;
OBJECT **obj1,**obj2;

x = GET_INT();
CHECK_POINTER(x);
obj1 = GET_OBJECT();
CHECK_POINTER(obj1);
obj2 = GET_OBJECT();
CHECK_POINTER(obj2);

if(!*obj1) {
	Bug("get_line_of_sight: Object1 has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
}
if(!*obj2) {
	Bug("get_line_of_sight: Object2 has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
}

// Ok, do it

*x=line_of_sight((*obj1)->x,(*obj1)->y,(*obj2)->x,(*obj2)->y);
}

// Get crude distance between two objects

void PV_GetDistance() {
VMINT *dist;
OBJECT **obj1,**obj2;
int dx,dy;

dist = GET_INT();
CHECK_POINTER(dist);
obj1 = GET_OBJECT();
CHECK_POINTER(obj1);
obj2 = GET_OBJECT();
CHECK_POINTER(obj2);

if(!*obj1) {
	Bug("get_distance: Object1 has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
}
if(!*obj2) {
	Bug("get_distance: Object2 has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
}

// Ok, do it
dx=(*obj1)->x - (*obj2)->x;
dy=(*obj1)->y - (*obj2)->y;
if(dx<0) {
	dx=-dx;
}
if(dy<0) {
	dy=-dy;
}

if(dx > dy) {
	*dist=dx;
} else {
	*dist=dy;
}

}

// Check if an object has been hurt

void PV_CheckHurt()
{
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);

CheckHurt(*obj);
}

// IF a specified square is solid then continue, else jump to offset

void PV_IfSolid()
{
int jumpoffset;
VMINT *a,*b;

a = (VMINT *)GET_INT();
CHECK_POINTER(a);
b = (VMINT *)GET_INT();
CHECK_POINTER(b);
jumpoffset = GET_DWORD();

if(!isSolid(*a,*b))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// IF a specified square can be seen then continue, else jump to offset

void PV_IfVisible()
{
int jumpoffset;
VMINT *a,*b;

a = (VMINT *)GET_INT();
CHECK_POINTER(a);
b = (VMINT *)GET_INT();
CHECK_POINTER(b);
jumpoffset = GET_DWORD();

if(vismap.isBlanked(*a,*b))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump

}

// set_user_flag <string> = <i>

void PV_SetUFlag()
{
VMINT *var;
char *str;

str = GET_STRING();
CHECK_POINTER(str);
var = GET_INT();
CHECK_POINTER(var);

Set_tFlag(str,*var);
}

// get_user_flag <i> = <string>

void PV_GetUFlag()
{
VMINT *var;
char *str;

var = GET_INT();
CHECK_POINTER(var);
str = GET_STRING();
CHECK_POINTER(str);

*var = Get_tFlag(str);
}

void PV_If_Uflag() {
char *str;
int val, jumpoffset;
str = GET_STRING();
CHECK_POINTER(str);
jumpoffset = GET_DWORD();

if(!Get_tFlag(str)) {
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}
}

void PV_If_nUflag() {
char *str;
int val, jumpoffset;
str = GET_STRING();
CHECK_POINTER(str);
jumpoffset = GET_DWORD();

if(Get_tFlag(str)) {
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}
}


// set_local <person> <flag> = <state>

void PV_SetLocal()
{
VMINT *var;
char *str;
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
str = GET_STRING();
CHECK_POINTER(str);
var = GET_INT();
CHECK_POINTER(var);

char *pname = NPC_MakeName(player);
NPC_set_lFlag(*obj,str,pname,*var);
M_free(pname);
}

// get_local <var> = <person> <flag>

void PV_GetLocal()
{
VMINT *var;
char *str;
OBJECT **obj;

var = GET_INT();
CHECK_POINTER(var);
obj = GET_OBJECT();
CHECK_POINTER(obj);
str = GET_STRING();
CHECK_POINTER(str);

char *pname = NPC_MakeName(player);
*var = NPC_get_lFlag(*obj,str,pname);
M_free(pname);
}

// weigh_object <var> = <object>

void PV_WeighObject()
{
VMINT *var;
OBJECT **obj;

var = GET_INT();
CHECK_POINTER(var);
obj = GET_OBJECT();
CHECK_POINTER(obj);

*var = WeighObject(*obj);
}

// get_bulk <var> = <object>

void PV_GetBulk()
{
VMINT *var;
OBJECT **obj;

var = GET_INT();
CHECK_POINTER(var);
obj = GET_OBJECT();
CHECK_POINTER(obj);

*var = GetBulk(*obj);
}

// input <intvar>

void PV_InputInt()
{
VMINT *var;

var = GET_INT();
CHECK_POINTER(var);

if(!get_num(*var))
    *var = 0;
else
	*var = atoi(user_input);
}

// IF object is in a pocket then continue, else jump to offset

void PV_IfInPocket()
{
int jumpoffset;
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);

jumpoffset = GET_DWORD();

// If there was an accident, assume it's not true and continue

if(!*obj)
	{
	Bug("If_In_Pocket: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
	return; // Leave quickly
	}

// Ok, do the condition

if(!(*obj)->parent.objptr)
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// IF object is NOT in a pocket then continue, else jump to offset

void PV_IfNInPocket()
{
int jumpoffset;
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
jumpoffset = GET_DWORD();

// If there was an accident, assume it's not true and continue

if(!*obj)
	{
	Bug("If_In_Pocket: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
	return; // Leave quickly
	}

// Ok, do the condition

if((*obj)->parent.objptr)
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

void PV_ReSyncEverything()
{
ResyncEverything();
}

// Talk to (file, startpage)

void PV_TalkTo2()
{
char *str1,*str2;

str1 = GET_STRING();
CHECK_POINTER(str1);
str2 = GET_STRING();
CHECK_POINTER(str2);

talk_to(str1,str2); // Str1 is null-protected
}

// Talk to (file)

void PV_TalkTo1()
{
char *str1;

str1 = GET_STRING();
CHECK_POINTER(str1);
talk_to(str1,NULL);
}

// random <var> max <maxnum>

void PV_Random()
{
VMINT *var;
VMINT *min;
VMINT *max,tot;

var = GET_INT();
CHECK_POINTER(var);
min = GET_INT();
CHECK_POINTER(min);
max = GET_INT();
CHECK_POINTER(max);

if(*max < *min)
	{
	*var = 1; // Prevent division by zero
	Bug("Random: maximum (%d) is less than minimum (%d)!\n",*max,*min);
	DumpVM(1);
	VMbug();
	return;
	}

tot = ((*max)-(*min))+1;

if(tot<1)
	{
	*var = 1; // Prevent division by zero
	Bug("Random: maximum and minimum are less than 1\n");
	DumpVM(1);
	VMbug();
	}

if(*max > 0)
	*var = (rand()%tot)+(*min);
else
	{
	*var = 1; // Prevent division by zero
	Bug("Random: maximum is less than 1!\n");
	DumpVM(1);
	VMbug();
	}
}

void PV_GetYN()
{
int jumpoffset;
char *str;

str = GET_STRING();
CHECK_POINTER(str);
jumpoffset = GET_DWORD();

// Ok, do the condition

if(!getYN(str))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

void PV_GetYNN()
{
int jumpoffset;
char *str;

str = GET_STRING();
CHECK_POINTER(str);
jumpoffset = GET_DWORD();

// Ok, do the condition

if(getYN(str))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

void PV_Restart()
{
Restart();
DelVM(); // Restart() will invalidate any object pointers, so quit the script immediately
}

// scroll_tile <num> <x> <y>

void PV_ScrTileN()
{
VMINT *num,*x,*y;

num = GET_INT();
CHECK_POINTER(num);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

if(*num<0 || *num>=TItot)
	{
	Bug("No such tile number %d in %s\n",*num,debcurfunc);
	return;
	}
scroll_tile_number(*num,*x,*y);
}

// scroll_tile "name" <x> <y>

void PV_ScrTileS()
{
int ctr;
VMINT *x,*y;
char *name;

name = GET_STRING();
CHECK_POINTER(name);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

for(ctr=0;ctr<TItot;ctr++)
    if(!istricmp_fuzzy(TIlist[ctr].name,name))
		{
		scroll_tile_number(ctr,*x,*y);
		return;
		}

Bug("No such tile '%s' in %s\n",name,debcurfunc);
}

// reset_tile <num> 

void PV_ResetTileN()
{
VMINT *num;

num = GET_INT();
CHECK_POINTER(num);

scroll_tile_reset(*num);
}

// scroll_tile "name" <x> <y>

void PV_ResetTileS()
{
int ctr;
char *name;

name = GET_STRING();
CHECK_POINTER(name);

for(ctr=0;ctr<TItot;ctr++)
	if(!istricmp_fuzzy(TIlist[ctr].name,name))
		{
		scroll_tile_reset(ctr);
		return;
		}

Bug("No such tile '%s' in %s\n",name,debcurfunc);
}

// While <exp> is nonzero go around the loop else fall through

void PV_While1()
{
int jumpoffset;
VMINT *a;

a = (VMINT *)GET_INT();
CHECK_POINTER(a);

jumpoffset = GET_DWORD();

if(!*a)
	; // Do Nothing
else
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// While <x> = <y> go around the loop else fall through

void PV_While2()
{
int jumpoffset;
VMINT *a,*b;
unsigned char op;

a = (VMINT *)GET_INT();
CHECK_POINTER(a);
op=GET_BYTE();
b = (VMINT *)GET_INT();
CHECK_POINTER(b);

jumpoffset = GET_DWORD();

if(Operator[op&OPMASK](*a,*b))
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// Break from a While loop or For Loop.
// This is functionally equivalent to Goto, but has a separate opcode
// to make the disassembly clearer.

void PV_Break()
{
int jumpoffset;

jumpoffset = GET_DWORD();
curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump

}

// Wait for a key to be pressed

void PV_GetKey()
{
irekey=get_key_debounced();
}

// Wait for a key to be pressed

void PV_GetKey_quiet()
{
irekey=get_key_debounced_quiet();
}

// Wait for a key to be pressed

void PV_GetKeyAscii()
{
VMINT *k;
k = GET_INT();
CHECK_POINTER(k);
*k = (VMINT)get_key_ascii();
}

// Wait for a key to be pressed

void PV_GetKeyAscii_quiet()
{
VMINT *k;
k = GET_INT();
CHECK_POINTER(k);
*k = (VMINT)get_key_ascii_quiet();
}

void PV_FlushKeys()
{
FlushKeys();
}



// Set Object's current activity

void PV_DoAct()
{
OBJECT **obj;
VMINT *act;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_INT();
CHECK_POINTER(act);

if(!*obj)
	{
	Bug("Do_Activity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

//ilog_quiet("actnum %p %d NULL\n",*obj,*act);
//ilog_quiet("%s %s\n",(*obj)->name,PElist[*act].name);

if((*obj)->flags & IS_DECOR)
	return;

SubAction_Wipe(*obj); // Delete any sub-actions
ActivityNum(*obj,*act,NULL);
}

// Set Object's current activity by string

void PV_DoActS()
{
OBJECT **obj;
char *act;
int pe;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_STRING();
CHECK_POINTER(act);

if(!*obj)
	{
	Bug("Do_Activity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

//ilog_quiet("actstr %p %d NULL\n",*obj,getnum4PE(act));

pe = getnum4PE(act);
if(pe == -1)
	{
	Bug("Do_Activity: PE script '%s' not found in function %s\n",act,debcurfunc);
	return;
	}
SubAction_Wipe(*obj); // Delete any sub-actions
ActivityNum(*obj,pe,NULL);
}

// Set Object's current activity (with a target)

void PV_DoActTo()
{
OBJECT **obj,**target;
VMINT *act;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_INT();
CHECK_POINTER(act);
target = GET_OBJECT();
CHECK_POINTER(target);

if(!*obj)
	{
	Bug("Do_Activity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if(!*target)
	{
	Bug("Do_Activity: Target has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

//ilog_quiet("actnum %p %d %s\n",*obj,*act,(*target)->name);

SubAction_Wipe(*obj); // Delete any sub-actions
ActivityNum(*obj,*act,*target);
}

// Set Object's current activity by string (with a target)

void PV_DoActSTo()
{
OBJECT **obj,**target;
char *act;
int pe;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_STRING();
CHECK_POINTER(act);
target = GET_OBJECT();
CHECK_POINTER(target);

if(!*obj)
	{
	Bug("Do_Activity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if(!*target)
	{
	Bug("Do_Activity: Target has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

//ilog_quiet("actstr %p %d %s\n",*obj,getnum4PE(act),(*target)->name);

pe = getnum4PE(act);
if(pe == -1)
	{
	Bug("Do_Activity: PE script '%s' not found in function %s\n",act,debcurfunc);
	return;
	}
SubAction_Wipe(*obj); // Delete any sub-actions
ActivityNum(*obj,pe,*target);
}


// InsertAction - Set Object's current activity WITHOUT erasing sub-tasks

// by number

void PV_InsAct()
{
OBJECT **obj;
VMINT *act;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_INT();
CHECK_POINTER(act);

if(!*obj)
	{
	Bug("Ins_Activity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

ActivityNum(*obj,*act,NULL);
}

// by string

void PV_InsActS()
{
OBJECT **obj;
char *act;
int pe;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_STRING();
CHECK_POINTER(act);

if(!*obj)
	{
	Bug("Do_Activity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

pe = getnum4PE(act);
if(pe == -1)
	{
	Bug("Do_Activity: PE script '%s' not found in function %s\n",act,debcurfunc);
	return;
	}

ActivityNum(*obj,pe,NULL);
}

// by number with target

void PV_InsActTo()
{
OBJECT **obj,**target;
VMINT *act;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_INT();
CHECK_POINTER(act);
target = GET_OBJECT();
CHECK_POINTER(target);

if(!*obj)
	{
	Bug("Do_Activity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if(!*target)
	{
	Bug("Do_Activity: Target has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

ActivityNum(*obj,*act,*target);
}

// by string with target

void PV_InsActSTo()
{
OBJECT **obj,**target;
char *act;
int pe;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_STRING();
CHECK_POINTER(act);
target = GET_OBJECT();
CHECK_POINTER(target);

if(!*obj)
	{
	Bug("Do_Activity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if(!*target)
	{
	Bug("Do_Activity: Target has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

pe = getnum4PE(act);
if(pe == -1)
	{
	Bug("Do_Activity: PE script '%s' not found in function %s\n",act,debcurfunc);
	return;
	}

ActivityNum(*obj,pe,*target);
}



// Halt scheduled activity (and stand there stupidly)

void PV_StopAct()
{
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);

if(!*obj)
	{
	Bug("stop_activity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

//ilog_quiet("stop action for %s\n",(*obj)->name);

Inactive(*obj);
}


// Resume scheduled activity

void PV_ResumeAct()
{
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);

if(!*obj)
	{
	Bug("resume_activity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

//ilog_quiet("resume action for %s\n",(*obj)->name);

ResumeSchedule(*obj); // Anything within last hour
}

// Set Object's next sub-activity

void PV_NextAct()
{
OBJECT **obj;
VMINT *act;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_INT();
CHECK_POINTER(act);

if(!*obj)
	{
	Bug("NextActivity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

SubAction_Push(*obj,*act,NULL);
}

// Set next subactivity by string

void PV_NextActS()
{
OBJECT **obj;
char *act;
int pe;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_STRING();
CHECK_POINTER(act);

if(!*obj)
	{
	Bug("NextActivity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

//ilog_quiet("actstr %p %d NULL\n",*obj,getnum4PE(act));

pe = getnum4PE(act);
if(pe == -1)
	{
	Bug("NextActivity: PE script '%s' not found in function %s\n",act,debcurfunc);
	return;
	}
SubAction_Push(*obj,pe,NULL);
}

// Set Object's current activity (with a target)

void PV_NextActTo()
{
OBJECT **obj,**target;
VMINT *act;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_INT();
CHECK_POINTER(act);
target = GET_OBJECT();
CHECK_POINTER(target);

if(!*obj)
	{
	Bug("NextActivity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if(!*target)
	{
	Bug("NextActivity: Target has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

//ilog_quiet("actnum %p %d %s\n",*obj,*act,(*target)->name);

SubAction_Push(*obj,*act,*target);
}

// Set Object's current activity by string (with a target)

void PV_NextActSTo()
{
OBJECT **obj,**target;
char *act;
int pe;

obj = GET_OBJECT();
CHECK_POINTER(obj);
act = GET_STRING();
CHECK_POINTER(act);
target = GET_OBJECT();
CHECK_POINTER(target);

if(!*obj)
	{
	Bug("NextActivity: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if(!*target)
	{
	Bug("NextActivity: Target has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if((*obj)->flags & IS_DECOR)
	return;

//ilog_quiet("actstr %p %d %s\n",*obj,getnum4PE(act),(*target)->name);

pe = getnum4PE(act);
if(pe == -1)
	{
	Bug("NextActivity: PE script '%s' not found in function %s\n",act,debcurfunc);
	return;
	}
SubAction_Push(*obj,pe,*target);
}


// Debugging function: Get PEscript name from a number

void PV_GetFuncP()
{
char **str;
VMINT *a;

str = (char **)GET_OBJECT(); // Hack, we need the pointer not just the string
CHECK_POINTER(str);
a = GET_INT();
CHECK_POINTER(a);

if(*a < 1 || *a > PEtot)
	*str=NULL;
else
	*str=PElist[*a].name;
}

// Debugging function: Get root container for object

void PV_GetContainer()
{
OBJECT **obj1;
OBJECT **obj2;
OBJECT *ptr;

obj1 = GET_OBJECT();
CHECK_POINTER(obj1);
obj2 = GET_OBJECT();
CHECK_POINTER(obj2);

*obj1=NULL; // Set to Nothing

if(!*obj2)
	return; // Nothing to look for

// Go backwards until we have the root container

ptr = *obj2;
if(ptr->parent.objptr)
	while(ptr->parent.objptr) ptr=ptr->parent.objptr;
else
	ptr=NULL; // No container

*obj1 = ptr;
}

void PV_SetLeader()
{
OBJECT **o;
VMINT *p,*f;

o = GET_OBJECT(); // Hack, we need the pointer not just the string
CHECK_POINTER(o);
p = GET_INT();
CHECK_POINTER(p);
f = GET_INT();
CHECK_POINTER(f);

choose_leader(*o, PElist[*p].name, PElist[*f].name);
//int choose_leader(OBJECT *x, char *player_call, char *follower_call)
}

void PV_SetMember()
{
OBJECT **o;
VMINT *p;

o = GET_OBJECT(); // Hack, we need the pointer not just the string
CHECK_POINTER(o);
p = GET_INT();
CHECK_POINTER(p);

//int choose_member(OBJECT *x, char *player_call)
choose_member(*o, PElist[*p].name);
}

void PV_AddMember()
{
OBJECT **o;

o = GET_OBJECT(); // Hack, we need the pointer not just the string
CHECK_POINTER(o);

AddToParty(*o);
}

void PV_DelMember()
{
OBJECT **o;

o = GET_OBJECT(); // Hack, we need the pointer not just the string
CHECK_POINTER(o);

SubFromParty(*o);
ResumeSchedule(*o);

}

void PV_MoveTowards8()
{
OBJECT **o1,**o2;
char *s1,*s2;

pevm_err=-1; // Assume failure

o1 = GET_OBJECT();
CHECK_POINTER(o1);
o2 = GET_OBJECT();
CHECK_POINTER(o2);

if(*o1 == NULL || *o2 == NULL)
	{
	s1="Null";
	s2="Null";

	if(*o1)
		s1=BestName(*o1);
	if(*o2)
		s2=BestName(*o2);
	Bug("FindPath (%s,%s)\n",s1,s2);
	return;
	}

pevm_err=FindPath(*o1,*o2,1);
}

void PV_MoveTowards4()
{
OBJECT **o1,**o2;
char *s1,*s2;

pevm_err=-1; // Assume failure

o1 = GET_OBJECT();
CHECK_POINTER(o1);
o2 = GET_OBJECT();
CHECK_POINTER(o2);

if(*o1 == NULL || *o2 == NULL)
	{
	s1="Null";
	s2="Null";

	if(*o1)
		s1=BestName(*o1);
	if(*o2)
		s2=BestName(*o2);
	Bug("FindPath (%s,%s)\n",s1,s2);
	return;
	}

pevm_err=FindPath(*o1,*o2,0);
}

void PV_WaitForAnimation()
{
OBJECT **o;

o = GET_OBJECT();
CHECK_POINTER(o);

if((*o)->form->flags&6)	// Ignore if looped or stepped
	return;				// (Otherwise it will loop forever)

if((*o)->parent.objptr)		// Objects in pockets do not animate
	return;

for(;(*o)->sdir;RedrawMap());
}

void PV_WaitFor()
{
VMINT *ms,*flag;
ms = GET_INT();
CHECK_POINTER(ms);
flag = GET_INT();
CHECK_POINTER(flag);
if(*flag)
	waitredraw(*ms);
else
	waitfor(*ms);
}

// IF <object> is onscreen then continue, else jump to offset

void PV_If_oonscreen()
{
int jumpoffset;
OBJECT **obj,*a;
int ok;

obj = CHECK_OBJ(1);
jumpoffset = GET_DWORD();

ok=0;
if(obj)
	if(*obj)
		{
		a=*obj;
		if(a->x+(a->mw-1) >=mapx)
			if(a->x <= mapx2)
				if(a->y+(a->mh-1) >=mapy)
					if(a->y <= mapy2)
						ok=1;    // Yes
		}
    
if(!ok)
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

void PV_If_not_oonscreen()
{
int jumpoffset;
OBJECT **obj,*a;
int ok;

obj = CHECK_OBJ(1);
jumpoffset = GET_DWORD();

ok=0;
if(obj)
	if(*obj)
		{
		a=*obj;
		if(a->x+(a->mw-1) >=mapx)
			if(a->x <= mapx2)
				if(a->y+(a->mh-1) >=mapy)
					if(a->y <= mapy2)
						ok=1;    // Yes
		}
    
if(ok)
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

// Take quantity of objects from a container

void PV_TakeQty()
{
OBJECT **obj;
VMINT *amount;
char *str;

amount = GET_INT();
CHECK_POINTER(amount);
str = GET_STRING();
CHECK_POINTER(str);
obj = GET_OBJECT();
CHECK_POINTER(obj);

if(!*obj)
	{
	Bug("Take: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

pevm_err=!TakeQuantity(*obj,str,*amount);
}

// Move quantity of objects from one container to another

void PV_MoveQty()
{
OBJECT **src,**dest;
VMINT *amount;
char *str;

amount = GET_INT();
CHECK_POINTER(amount);
str = GET_STRING();
CHECK_POINTER(str);
src = GET_OBJECT();
CHECK_POINTER(src);
dest = GET_OBJECT();
CHECK_POINTER(dest);

if(!*src)
	{
	Bug("Move: source object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

if(!*dest)
	{
	Bug("Move: destination object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

pevm_err = !MoveQuantity(*src,*dest,str,*amount);
}

// Give quantity of objects to a container

void PV_GiveQty()
{
OBJECT **obj;
VMINT *amount;
char *str;

amount = GET_INT();
CHECK_POINTER(amount);
str = GET_STRING();
CHECK_POINTER(str);
obj = GET_OBJECT();
CHECK_POINTER(obj);

if(!*obj)
	{
	Bug("Give: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

//pevm_err=!AddQuantity(*obj,str,*amount);
AddQuantity(*obj,str,*amount);
}


// Count quantity of objects in a container

void PV_CountQty()
{
OBJECT **obj;
VMINT *amount;
char *str;

amount = GET_INT();
CHECK_POINTER(amount);
str = GET_STRING();
CHECK_POINTER(str);
obj = GET_OBJECT();
CHECK_POINTER(obj);

if(!*obj)
	{
	Bug("Count: Object has not been initialised!\n");
	DumpVM(1);
	VMbug();
	return; // Leave quickly
	}

*amount=SumObjects((*obj)->pocket.objptr,str,0);
}

void PV_CopySchedule()
{
OBJECT **o1,**o2;
char *s1,*s2;

o1 = GET_OBJECT();
CHECK_POINTER(o1);
o2 = GET_OBJECT();
CHECK_POINTER(o2);

if(*o1 == NULL || *o2 == NULL)
	{
	s1="Null";
	s2="Null";

	if(*o1)
		s1=BestName(*o1);
	if(*o2)
		s2=BestName(*o2);
	Bug("CopySchedule(%s -> %s)\n",s1,s2);
	return;
	}

// Make sure the source has a schedule

if(!(*o1)->schedule)
	return;

// Make sure

if(!(*o2)->schedule)
	(*o2)->schedule = (SCHEDULE *)M_get(24,sizeof(SCHEDULE));

memcpy((*o2)->schedule,(*o1)->schedule,sizeof(SCHEDULE)*24);

// Copy labels too (they're just pointers to static text)
(*o2)->labels->race = (*o1)->labels->race;
(*o2)->labels->rank = (*o1)->labels->rank;
(*o2)->labels->party = (*o1)->labels->party;
}

void PV_CopySpeech()
{
OBJECT **o1,**o2;
char *s1,*s2;

o1 = GET_OBJECT();
CHECK_POINTER(o1);
o2 = GET_OBJECT();
CHECK_POINTER(o2);

if(*o1 == NULL || *o2 == NULL)
	{
	s1="Null";
	s2="Null";

	if(*o1)
		s1=BestName(*o1);
	if(*o2)
		s2=BestName(*o2);
	Bug("CopySpeech(%s -> %s)\n",s1,s2);
	return;
	}

strcpy((*o2)->funcs->talk,(*o1)->funcs->talk);
(*o2)->funcs->tcache=(*o1)->funcs->tcache;
}

// Call a function for all objects onscreen

void PV_AllOnscreen()
{
OBJREGS objvars;
int func,x,y;

// First get the function to call

func = GET_DWORD();

VM_SaveRegs(&objvars);

SI_Search=1;
for(y=mapy;y<mapy2;y++)
	{
	// Search cancelled?
	if(!SI_Search)
		break;
		for(x=mapx;x<mapx2;x++)
			{
			// Search cancelled?
			if(!SI_Search)
				break;
			current_object = GameGetObject(x,y);
			if(current_object)
				if(current_object->stats->hp > 0)
					if(current_object->stats->intel > 0)
						CallVMnum(func);
			}
	}
SI_Search=0;

VM_RestoreRegs(&objvars);
}

// Call a function for all objects in a certain radius
// ALL_AROUND <obj> radius <int> does <func>

void PV_AllAround()
{
OBJECT **focus;
OBJREGS objvars;
int func,sx,sy,ex,ey,x,y;
VMINT *r;

// First the focus object,
focus = GET_OBJECT();
CHECK_POINTER(focus);

// Then the 'radius',
r = GET_INT();
CHECK_POINTER(r);

// Now get the function to call.
func = GET_DWORD();

// Calculate the start coordinates
sx = (*focus)->x - *r;
if(sx<0)
	sx=0;

sy = (*focus)->y - *r;
if(sy<0)
	sy=0;

// And the end coordinates
ex = (*focus)->x + *r;
if(ex>curmap->w)
	ex=curmap->w;

ey = (*focus)->y + *r;
if(ey>curmap->h)
	ey=curmap->h;

VM_SaveRegs(&objvars);

SI_Search=1;
for(y=sy;y<ey;y++)
	{
	// Search cancelled?
	if(!SI_Search)
		break;
		for(x=sx;x<ex;x++)
			{
			// Search cancelled?
			if(!SI_Search)
				break;
			current_object = GameGetObject(x,y);
			if(current_object)
				{
				if(current_object->stats->hp > 0)
					if(current_object->stats->intel > 0)
						CallVMnum(func);
				}
			}
	}
SI_Search=0;

VM_RestoreRegs(&objvars);
}


void PV_CheckTime()
{
CheckTime();
}

void PV_FindNear()
{
OBJECT **o1;
OBJECT **o2;
char *str;

o1 = GET_OBJECT();
CHECK_POINTER(o1);
str = GET_STRING();
CHECK_POINTER(str);
o2 = GET_OBJECT();
CHECK_POINTER(o2);

//ilog_quiet("find_nearest %s,%s\n",(*o2)->name,str);
*o1 = find_nearest(*o2,str);
}

// Find up to n nearby objects of given type (fuzzy search supported)

void PV_FindNearby()
{
OBJECT **o1;
OBJECT **o2;
char *str;
OBJECT *array;
VMINT size,idx;

GET_ARRAY_INFO((void **)&array,&size,&idx); // Get size of array at IP

o1 = GET_OBJECT();
CHECK_POINTER(o1);
str = GET_STRING();
CHECK_POINTER(str);
o2 = GET_OBJECT();
CHECK_POINTER(o2);

// Give it the start of the array, not o1, in case it has been poisoned
find_nearby(*o2,str,(OBJECT **)array,size);
}

// Find up to n nearby objects of given flag

void PV_FindNearbyFlag()
{
OBJECT **o1;
OBJECT **o2;
VMINT *flag;
OBJECT *array;
VMINT size,idx;

GET_ARRAY_INFO((void **)&array,&size,&idx); // Get size of array at IP

o1 = GET_OBJECT();
CHECK_POINTER(o1);
flag = GET_INT();
CHECK_POINTER(flag);
o2 = GET_OBJECT();
CHECK_POINTER(o2);

// Give it the start of the array, not o1, in case it has been poisoned
find_nearby_flag(*o2,*flag,(OBJECT **)array,size);
}


// Find a tag anywhere in the world

void PV_FindTag()
{
OBJLIST *t;
OBJECT **obj;
char *str;
VMINT *tag;

obj = GET_OBJECT();
CHECK_POINTER(obj);
str = GET_STRING();
CHECK_POINTER(str);
tag = GET_INT();
CHECK_POINTER(tag);

// If it has empty string, find any matching tag

//if(strlen(str) == 0)
if(!str[0])
	str=NULL;


// Look near focussed object to start with (for speed)
if(person)
	{
	*obj=find_neartag(person,str,*tag);
	if(*obj)
		return;
	// Didn't find anything, carry on and search entire list
	}

for(t=MasterList;t;t=t->next)
	if(t->ptr->tag == *tag)
		{
		if(str)
			{
			if(!istricmp_fuzzy(str,t->ptr->name))
				{
				*obj = t->ptr;
				return;
				}
			}
		else
			{
			*obj = t->ptr;
			return;
			}
		}

*obj = NULL;
}

// Find a tag nearby someone

void PV_FastTag()
{
OBJECT **obj,**nearby;
VMINT *tag;

obj = GET_OBJECT();
CHECK_POINTER(obj);
tag = GET_INT();
CHECK_POINTER(tag);
nearby = GET_OBJECT();
CHECK_POINTER(nearby);

*obj=find_neartag(*nearby,NULL,*tag);
}


// Build a list of matching tags

void PV_MakeTagList()
{
OBJLIST *t;
OBJECT **obj;
char *str;
VMINT *tag;

OBJECT *array;
VMINT size,idx,actr;

GET_ARRAY_INFO((void **)&array,&size,&idx); // Get size of array at IP
obj = GET_OBJECT();
CHECK_POINTER(obj);
str = GET_STRING();
CHECK_POINTER(str);
tag = GET_INT();
CHECK_POINTER(tag);


// If it has empty string, find any matching tag
if(strlen(str) == 0)
	str=NULL;

obj=(OBJECT **)array;
memset(obj,0,size*sizeof(OBJECT *)); // Blank the array
actr=0;

for(t=MasterList;t;t=t->next)
	if(t->ptr->tag == *tag)
		{
		if(str)
			{
			if(!istricmp_fuzzy(str,t->ptr->name))
				{
				if(actr<size)
					obj[actr++] = t->ptr;
				}
			}
		else
			{
			if(actr<size)
				obj[actr++] = t->ptr;
			}
		}

// *obj = NULL;
}

// Find the next occurence of an object in a list

void PV_FindNext()
{
OBJLIST *t;
OBJECT **obj;
const char *str;
OBJECT **list;
OBJECT *ptr;

obj = GET_OBJECT();
CHECK_POINTER(obj);
str = GET_STRING();
CHECK_POINTER(str);
list = GET_OBJECT();
CHECK_POINTER(list);

// Theoretically we could set *obj to NULL here as an optimsation,
// but that will cause merry hell if it's also the list, e.g. get_next X = "hatch*" after X

// Nothing to search in, so return nothing
if(!(*list)) {
	*obj = NULL;
	return;
}

// If it has empty string, it'll return the first object
if(!str[0]) {
	str="*";
}

for(ptr = *list;ptr;ptr=ptr->next) {
	if(!str || (!istricmp_fuzzy(str,ptr->name))) {
		*obj = ptr;
		return;
	}
}
*obj = NULL;
}

// Set light state for a region between two control objects.

#define MAX_LIGHTS 128

void PV_SetLight()
{
VMINT *tag,*state;
OBJLIST *t;
OBJECT *start[MAX_LIGHTS],*end[MAX_LIGHTS];
int sctr,ectr;

tag = GET_INT();
CHECK_POINTER(tag);
state = GET_INT();
CHECK_POINTER(state);

sctr=0;
ectr=0;

// Build a list of all light starts and ends for this tag

for(t=MasterList;t;t=t->next)
	if(t->ptr->tag == *tag)
		{
		if(sctr<MAX_LIGHTS)
			if(!istricmp("light_start",t->ptr->name))
				start[sctr++]=t->ptr;
		if(ectr<MAX_LIGHTS)
			if(!istricmp("light_end",t->ptr->name))
				end[ectr++]=t->ptr;
		}

// Make sctr the lower of the two
if(ectr<sctr)
	sctr=ectr;

for(ectr=0;ectr<sctr;ectr++)
	{
//	printf("ctr = %d start = %p, end = %p",ectr,start[ectr],end[ectr]);
	set_light(start[ectr]->x,start[ectr]->y,end[ectr]->x,end[ectr]->y,*state);
	}
}


void PV_SetLight_single()
{
VMINT *state;
OBJECT **start,**end;

start = GET_OBJECT();
CHECK_POINTER(start);
end = GET_OBJECT();
CHECK_POINTER(end);
state = GET_INT();
CHECK_POINTER(state);

set_light((*start)->x,(*start)->y,(*end)->x,(*end)->y,*state);
}


void PV_Printaddr()
{
VMINT *num;
num = GET_INT();
irecon_printf("%p",num);
if(DebugVM || printlogging)
	ilog_quiet("%p",num);
}

// Log console output?

void PV_PrintLog()
{
VMINT *p;

p = GET_INT();
CHECK_POINTER(p);

printlogging = *p;

// Special effects for debugging purposes:

switch(printlogging)
	{
	case 1:
	irecon_savecol();
	irecon_colour(200,0,0);
	Bug("Problems in %s:\n",debcurfunc);
	break;

	case 0:
	irecon_loadcol();
	irecon_printf("\n");
	break;
	}
}


void PV_MovePartyToObj()
{
int ctr;
OBJECT **o1;

o1 = GET_OBJECT();
CHECK_POINTER(o1);

for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	if(party[ctr])
		{
		TransferToPocket(party[ctr],*o1);
		// Update X/Y to match object
		party[ctr]->x = (*o1)->x;
		party[ctr]->y = (*o1)->y;
		}

}

void PV_MovePartyFromObj()
{
int ctr;
VMINT *x,*y;
OBJECT **o1;

o1 = GET_OBJECT();
CHECK_POINTER(o1);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	if(party[ctr])
		ForceFromPocket(party[ctr],*o1,*x,*y);

}


void PV_MoveTag()
{
OBJLIST *t;
VMINT *x,*y,*tag1,tag;

tag1 = GET_INT();
CHECK_POINTER(tag1);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

tag = -(*tag1); // Get negated tag number (e.g. 50 -> -50)

// Make sure they're not flagged as moved
for(t=MasterList;t;t=t->next) {
	t->ptr->engineflags &= ~ENGINE_DIDMOVESTACK;
}

// Move them if applicable
for(t=MasterList;t;t=t->next) {
	if(t->ptr->tag == tag) {
		if(!t->ptr->pocket.objptr) {
			if(!(t->ptr->engineflags & ENGINE_DIDMOVESTACK)) {
				move_stack(t->ptr,t->ptr->x+*x,t->ptr->y+*y);
			}
		}
	}
}
}


void PV_UpdateTag()
{
VMINT *tag;

tag = GET_INT();
CHECK_POINTER(tag);
ForceUpdateTag(*tag);
}

// GetDataSSS - GetData into S from table S with key S

void PV_GetDataSSS()
{
char **var,*table,*key;
DT_ITEM *p;

var = (char **)GET_OBJECT(); // Hack, we need the pointer not just the string
CHECK_POINTER(var);
table = GET_STRING();
CHECK_POINTER(table);
key = GET_STRING();
CHECK_POINTER(key);

pevm_err=0;
p = GetTableName_s(table,key);
if(!p)
	{
	pevm_err=-1;
	return;
	}

*var = p->is;
}

// GetDataSSI - GetData into S from table S with key I

void PV_GetDataSSI()
{
char **var,*table;
VMINT *key;
DT_ITEM *p;

var = (char **)GET_OBJECT(); // Hack, we need the pointer not just the string
CHECK_POINTER(var);
table = GET_STRING();
CHECK_POINTER(table);
key = GET_INT();
CHECK_POINTER(key);

pevm_err=0;
p = GetTableName_i(table,*key);
if(!p)
	{
	pevm_err=-1;
	return;
	}

*var = p->is;
}

// GetDataSIS - GetData into S from table I with key S

void PV_GetDataSIS()
{
char **var,*key;
VMINT *table;
DT_ITEM *p;

var = (char **)GET_OBJECT(); // Hack, we need the pointer not just the string
CHECK_POINTER(var);
table = GET_INT();
CHECK_POINTER(table);
key = GET_STRING();
CHECK_POINTER(key);

pevm_err=0;
p = GetTableNum_s(*table,key);
if(!p)
	{
	pevm_err=-1;
	return;
	}

*var = p->is;
}

// GetDataSII - GetData into S from table I with key I

void PV_GetDataSII()
{
char **var;
VMINT *key,*table;
DT_ITEM *p;

var = (char **)GET_OBJECT(); // Hack, we need the pointer not just the string
CHECK_POINTER(var);
table = GET_INT();
CHECK_POINTER(table);
key = GET_INT();
CHECK_POINTER(key);

pevm_err=0;
p = GetTableNum_i(*table,*key);
if(!p)
	{
	pevm_err=-1;
	return;
	}

*var = p->is;
}

// GetDataISS - GetData into I from table S with key S

void PV_GetDataISS()
{
VMINT *var;
char *table,*key;
DT_ITEM *p;

var = GET_INT();
CHECK_POINTER(var);
table = GET_STRING();
CHECK_POINTER(table);
key = GET_STRING();
CHECK_POINTER(key);

pevm_err=0;
p = GetTableName_s(table,key);
if(!p)
	{
	pevm_err=-1;
	return;
	}

*var = p->ii;
}

// GetDataISI - GetData into I from table S with key I

void PV_GetDataISI()
{
char *table;
VMINT *var,*key;
DT_ITEM *p;

var = GET_INT();
CHECK_POINTER(var);
table = GET_STRING();
CHECK_POINTER(table);
key = GET_INT();
CHECK_POINTER(key);

pevm_err=0;
p = GetTableName_i(table,*key);
if(!p)
	{
	pevm_err=-1;
	return;
	}

*var = p->ii;
}

// GetDataIIS - GetData into S from table I with key S

void PV_GetDataIIS()
{
char *key;
VMINT *var,*table;
DT_ITEM *p;

var = GET_INT();
CHECK_POINTER(var);
table = GET_INT();
CHECK_POINTER(table);
key = GET_STRING();
CHECK_POINTER(key);

pevm_err=0;
p = GetTableNum_s(*table,key);
if(!p)
	{
	pevm_err=-1;
	return;
	}

*var = p->ii;
}

// GetDataIII - GetData into I from table I with key I

void PV_GetDataIII()
{
VMINT *var,*key,*table;
DT_ITEM *p;

var = GET_INT();
CHECK_POINTER(var);
table = GET_INT();
CHECK_POINTER(table);
key = GET_INT();
CHECK_POINTER(key);

pevm_err=0;
p = GetTableNum_i(*table,*key);
if(!p)
	{
	pevm_err=-1;
	return;
	}

*var = p->ii;
}

// Get all (integer) keys into I[] for table I

void PV_GetDataKeysII()
{
VMINT *table;
OBJECT **o2;
OBJECT *array;
VMINT size,idx;

GET_ARRAY_INFO((void **)&array,&size,&idx); // Get size of array at IP

GET_INT();  // Skip array target (first param), use array instead

// Get table ID
table = GET_INT();
CHECK_POINTER(table);

GetTableNumKeys_i(*table, (VMINT *)array, size);
}


// Get all (integer) keys into I[] for table S

void PV_GetDataKeysIS()
{
char *table;
OBJECT **o2;
OBJECT *array;
VMINT size,idx;

GET_ARRAY_INFO((void **)&array,&size,&idx); // Get size of array at IP

GET_INT();  // Skip array target (first param), use array instead

table = GET_STRING();
CHECK_POINTER(table);

GetTableNameKeys_i(table, (VMINT *)array, size);
}

// Get all (string) keys into S[] for table I

void PV_GetDataKeysSI()
{
VMINT *table;
OBJECT **o2;
OBJECT *array;
VMINT size,idx;

GET_ARRAY_INFO((void **)&array,&size,&idx); // Get size of array at IP

GET_STRING(); // Skip string array

// Get table ID
table = GET_INT();
CHECK_POINTER(table);

GetTableNumKeys_s(*table, (char **)array, size);
}

// Get all (string) keys into S[] for table S

void PV_GetDataKeysSS()
{
char *table;
OBJECT **o2;
OBJECT *array;
VMINT size,idx;

GET_ARRAY_INFO((void **)&array,&size,&idx); // Get size of array at IP

GET_STRING(); // Skip string array

// Get table
table = GET_STRING();
CHECK_POINTER(table);

GetTableNameKeys_s(table, (char **)array, size);
}



// Delete a Decorative object

void PV_GetDecor()
{
VMINT *x,*y;
OBJECT **obj;

x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);
obj = GET_OBJECT();
CHECK_POINTER(obj);

int ix,iy;
if(GetDecor(&ix,&iy,*obj))
	{
	pevm_err=0;
	*x=ix;
	*y=iy;
	}
else
	pevm_err=-1;
}


// Delete a Decorative object

void PV_DelDecor()
{
VMINT *x,*y;
OBJECT **obj;

x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);
obj = GET_OBJECT();
CHECK_POINTER(obj);

DelDecor(*x,*y,*obj);
}

void PV_SearchContainer()
{
OBJECT **cont,**obj;
char *name;

obj = GET_OBJECT();
CHECK_POINTER(obj);
name = GET_STRING();
CHECK_POINTER(name);
cont = GET_OBJECT();
CHECK_POINTER(cont);

if(!*cont)
	{
	*obj=NULL;
	return;
	}

*obj = GetFirstObjectFuzzy((*cont)->pocket.objptr, name);
}

// Change map to fixed coordinates

void PV_ChangeMap1()
{
VMINT *x,*y,*z;

z = GET_INT();
CHECK_POINTER(z);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);

change_map = *z;
change_map_x = *x;
change_map_y = *y;
}

// Change map using a tag

void PV_ChangeMap2()
{
VMINT *z,*t;

z = GET_INT();
CHECK_POINTER(z);
t = GET_INT();
CHECK_POINTER(t);

change_map = *z;
change_map_tag = *t;
}

// Return map number

void PV_GetMapNo()
{
VMINT *z;

z = GET_INT();
CHECK_POINTER(z);

// We can't expose mapnumber as a writeable variable!
*z = (VMINT)mapnumber;
}

// Scroll a still picture

void PV_PicScroll()
{
char *name;
VMINT *dx,*dy;

name = GET_STRING();
CHECK_POINTER(name);

dx = GET_INT();
CHECK_POINTER(dx);

dy = GET_INT();
CHECK_POINTER(dy);

NPC_ScrollPic(name,*dx,*dy,tfx_picdelay1,tfx_picdelay2,tfx_picdelay3);
}

// Set the panel image

void PV_SetPanel()
{
char *name;

name = GET_STRING();
CHECK_POINTER(name);

LoadBacking(swapscreen,name);

// Set up console
irecon_term();
irecon_init(conx,cony,conwid,conlen);
Show();
IRE_WaitFor(0);	// Yield to OS
}

//

void PV_FindPathMarker()
{
OBJECT **o1;
OBJECT **o2;
char *str;

o1 = GET_OBJECT();
CHECK_POINTER(o1);
str = GET_STRING();
CHECK_POINTER(str);
o2 = GET_OBJECT();
CHECK_POINTER(o2);

*o1 = find_pathmarker(*o2,str);
}

// Centre the solid-map around an object

void PV_ReSolid()
{
OBJECT **obj;

obj = GET_OBJECT();
CHECK_POINTER(obj);
CentreMap(*obj);
gen_largemap();
}


// Set personal name

void PV_SetPName()
{
OBJECT **obj,*o;
char *name;

obj = GET_OBJECT();
CHECK_POINTER(obj);

name = GET_STRING();
CHECK_POINTER(name);

o=*obj;
if(o)
	{
	if(o->personalname)
		{
		// Don't want the nabes to do a buffer overflow
		strncpy(o->personalname,name,32);
		o->personalname[31]=0; // Trim any excess
		}
	}
}

// Delete all object's property

void PV_DelProp()
{
OBJECT **obj;
OBJLIST *t;
OBJREGS objvars;
int pe;

pe=getnum4PE("delprop_callback");  // If this exists, call it for each one

obj = GET_OBJECT();
CHECK_POINTER(obj);

VM_SaveRegs(&objvars);

for(t=MasterList;t;t=t->next)
	if(t->ptr)
		if(t->ptr->stats)
			if(t->ptr->stats->owner.objptr == *obj)
				{
				// Destroy it and everything within
				EraseContents(t->ptr->pocket.objptr);

				// If we have a special function, use that
				// Otherwise, nuke it
				if(pe>0)
					{
					person=t->ptr;
					current_object=person;
					CallVMnum(pe);
					}
				else
					DeleteObject(t->ptr);
				}

VM_RestoreRegs(&objvars);
}

void PV_QUpdate()
{
OBJECT **obj,*oldcur;

obj = GET_OBJECT();
CHECK_POINTER(obj);

if((*obj)->funcs->qcache >= 0)
	{
	oldcur = current_object;
	current_object = (*obj);
	CallVMnum((*obj)->funcs->qcache);
	current_object = oldcur;
	}
}

//
//  Graphics, Effects, Interface etc
//


// Create a mouse region

void PV_AddRange()
{
VMINT *id,*x,*y,*w,*h;

id = GET_INT();
CHECK_POINTER(id);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);
w = GET_INT();
CHECK_POINTER(w);
h = GET_INT();
CHECK_POINTER(h);

AddMouseRange(*id,*x,*y,*w,*h);
}


void PV_GridRange()
{
VMINT *id,*w,*h;

id = GET_INT();
CHECK_POINTER(id);
w = GET_INT();
CHECK_POINTER(w);
h = GET_INT();
CHECK_POINTER(h);

SetRangeGrid(*id,*w,*h);
}

// Create a mouse region

void PV_AddButton()
{
VMINT *id,*x,*y;
char *normal, *pressed;

id = GET_INT();
CHECK_POINTER(id);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);
normal = GET_STRING();
CHECK_POINTER(normal);
pressed = GET_STRING();
CHECK_POINTER(pressed);

AddMouseRange(*id,*x,*y,1,1);
SetRangeButton(*id,normal,pressed);
DrawButtons();
}

// Create a mouse region

void PV_PushButton()
{
VMINT *id,*x;

id = GET_INT();
CHECK_POINTER(id);
x = GET_INT();
CHECK_POINTER(x);

PushButton(*id,*x);
DrawButtons();
}

// Make the button right-clickable

void PV_RightClick()
{
VMINT *id;

id = GET_INT();
CHECK_POINTER(id);

RightClick(*id);
}

// Wait for mouse button to be released

void PV_FlushMouse()
{
VMINT *flag;
flag = GET_INT();
CHECK_POINTER(flag);
if(*flag)
	get_mouse_release();  // animating
else
	WaitForMouseRelease(); // blocking
}

void PV_RangePointer()
{
VMINT *id,*p;

id = GET_INT();
CHECK_POINTER(id);
p = GET_INT();
CHECK_POINTER(p);

SetRangePointer(*id,*p);
}

// Create a mouse region

void PV_TextButton()
{
VMINT *id,*x,*y;
char *text;
int w,h;

id = GET_INT();
CHECK_POINTER(id);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);
text = GET_STRING();
CHECK_POINTER(text);

irecon_printxy(*x,*y,text); // Print it
if(curfont)
	{
	w= curfont->Length(text);
	h= curfont->Height();
	}
else
	{
	w=8*strlen(text);
	h=8;
	}

AddMouseRange(*id,*x,*y,w,h);
}


void PV_SaveScreen()
{
SaveScreen();
SaveRanges();
ClearMouseRanges();
irecon_savecol();
}

void PV_RestoreScreen()
{
RestoreScreen();
RestoreRanges();
irecon_loadcol();
}


void PV_dofx()
{
VMINT *z;
OBJECT **o;

o = GET_OBJECT();
CHECK_POINTER(o);
z = GET_INT();
CHECK_POINTER(z);

if(*o)
	if((*o)->user)
		(*o)->user->fx_func=*z;
}

void PV_dofxS()
{
char *z;
OBJECT **o;
int zfunc;

o = GET_OBJECT();
CHECK_POINTER(o);
z = GET_STRING();
CHECK_POINTER(z);

zfunc = getnum4PE(z);
if(zfunc == -1)
	{
	Bug("startfx: Cannot find effect function '%s' in %s\n",z,debcurfunc);
	return;
	}

if(*o)
	if((*o)->user)
		(*o)->user->fx_func=zfunc;
}

void PV_dofxOver()
{
VMINT *z;
OBJECT **o;

o = GET_OBJECT();
CHECK_POINTER(o);
z = GET_INT();
CHECK_POINTER(z);

if(*o)
	if((*o)->user)
		(*o)->user->fx_func=-*z;
}

void PV_dofxOverS()
{
int zfunc;
char *z;
OBJECT **o;

o = GET_OBJECT();
CHECK_POINTER(o);
z = GET_STRING();
CHECK_POINTER(z);

zfunc = getnum4PE(z);
if(zfunc == -1)
	{
	Bug("overfx: Cannot find effect function '%s' in %s\n",z,debcurfunc);
	return;
	}

if(*o)
	if((*o)->user)
		(*o)->user->fx_func=-zfunc;
}



void PV_stopfx()
{
OBJECT **o;

o = GET_OBJECT();
CHECK_POINTER(o);

if(*o)
	if((*o)->user)
		(*o)->user->fx_func=0;
}

// Get X,Y pixel coords for an object

void PV_fxGetXY()
{
VMINT *x,*y;
OBJECT **o;

x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);
o = GET_OBJECT();
CHECK_POINTER(o);

if(!*o)
	return; // Safety check

*x=(((*o)->x-mapx)<<5)+(*o)->hotx; // Get grid position of viewscreen, multiply by tile size
*y=(((*o)->y-mapy)<<5)+(*o)->hoty; // Get grid position of viewscreen, multiply by tile size
}


// Set effects colour

void PV_fxColour()
{
VMINT *a;
int r,g,b;

a = GET_INT();
CHECK_POINTER(a);

#if BYTE_ORDER == LITTLE_ENDIAN
	r=((*a)>>16)&0xff;   // get Red
	g=((*a)>>8)&0xff;    // get Green
	b=(*a)&0xff;         // get Blue
#else
	r=(*a)&0xff;         // get Red
	g=((*a)>>8)&0xff;    // get Green
	b=((*a)>>16)&0xff;   // get Blue
#endif

tfx_Colour->Set(r,g,b);
}


// Set effects alpha level

void PV_fxAlpha()
{
VMINT *a;

a = GET_INT();
CHECK_POINTER(a);
tfx_Alpha=*a;
}

#define ALPHA tfx_Alpha

// Set effects alpha mode

void PV_fxAlphaMode()
{
VMINT *a;

a = GET_INT();
CHECK_POINTER(a);

gamewin->SetBrushmode(*a,tfx_Alpha);
}

// Optimised random number generator

void PV_fxRandom()
{
VMINT *var,*r;

var = GET_INT();
CHECK_POINTER(var);
r = GET_INT();
CHECK_POINTER(r);

if(*r > 0)
	*var = qrand()%*r;
}

// Line and Point primitives

void PV_fxLine()
{
if(tfx_Brushsize>1)
	gamewin->ThickLine(tfx_sx,tfx_sy,tfx_dx,tfx_dy,tfx_Brushsize,tfx_Colour);
else
	gamewin->Line(tfx_sx,tfx_sy,tfx_dx,tfx_dy,tfx_Colour);
}

void PV_fxPoint()
{
if(tfx_Brushsize>1)
	gamewin->Circle(tfx_sx,tfx_sy,tfx_Brushsize,tfx_Colour);
else
	gamewin->PutPixel(tfx_sx,tfx_sy,tfx_Colour);
}

void PV_fxRect()
{
gamewin->FillRect(tfx_sx,tfx_sy,tfx_dx-tfx_sx,tfx_dy-tfx_sy,tfx_Colour);
}

void PV_fxPoly()
{
VMINT *pointdata,idx,vertices,t;
static int newpoints[64];

GET_ARRAY_INFO((void **)&pointdata,&vertices,&idx); // Get size of array at IP
GET_INT();	// skip the array

vertices>>=1; // This many vertices (half number of points)

// Clip vertices to 32
if(vertices>32)
	vertices=32;

// Copy vertices and offset them
for(idx=0;idx<vertices;idx++)
	{
	t=idx<<1; // two ints per vertex
	newpoints[t]=pointdata[t]+tfx_dx;
	t++;
	newpoints[t]=pointdata[t]+tfx_dy;
	}

// 'Do it' (Gollum gets straight to the point)
gamewin->Polygon(vertices,newpoints,tfx_Colour);
//polygon((BITMAP *)IRE_GetBitmap(gamewin),vertices,newpoints,tfx_Colour->packed);
}

void PV_fxOrbit()
{
VMINT *angle;

angle = GET_INT();
CHECK_POINTER(angle);

CalcOrbit(angle, tfx_radius, tfx_drift, tfx_speed, &tfx_sx, &tfx_sy, tfx_dx, tfx_dy);
}

// Draw the current object's sprite in the current pen at sx,sy

void PV_fxSprite()
{
if(current_object)
	current_object->form->seq[current_object->sptr]->image->DrawAlpha(gamewin,tfx_sx,tfx_sy,tfx_Alpha);
}

// Draw a corona at sx,sy of size radius

void PV_fxCorona()
{
VMINT *tint;
tint = GET_INT();
CHECK_POINTER(tint);

ProjectCorona(tfx_sx,tfx_sy,tfx_radius,tfx_intensity,tfx_falloff,*tint);
}


// Draw a corona at sx,sy of size radius

void PV_fxBlackCorona()
{
VMINT *tint;
tint = GET_INT();
CHECK_POINTER(tint);

ProjectBlackCorona(tfx_sx,tfx_sy,tfx_radius,tfx_intensity,tfx_falloff,*tint);
}


// Display <object> onscreen at <x> <y>

void PV_fxAnimSprite()
{
if(current_object)
	{
	current_object->stats->tick++;
	if(current_object->stats->tick>=current_object->form->speed)
		{
		if(current_object->form->flags&SEQFLAG_RANDOM)
			anim_random(current_object);
		else
			animate[current_object->form->flags&SEQFLAG_ANIMCODE](current_object);  // animate once
		current_object->stats->tick=0;
		}
//	draw_trans_rle_sprite(gamewin,current_object->form->seq[current_object->sptr]->image,tfx_sx,tfx_sy);
	current_object->form->seq[current_object->sptr]->image->DrawAlpha(gamewin,tfx_sx,tfx_sy,tfx_Alpha);
	}
}


// Append to <userstring> the <new string>

void PV_addstr()
{
USERSTRING **a;
char *b;

a = (USERSTRING **)GET_INT();
CHECK_POINTER(a);

b = GET_STRING();
CHECK_POINTER(b);

STR_Add(*a,b);
}

// Append a number to the string

void PV_addstrn()
{
USERSTRING **a;
VMINT *b;
char text[256];


a = (USERSTRING **)GET_INT();
CHECK_POINTER(a);

b = GET_INT();
CHECK_POINTER(b);

int num = *b;
snprintf(text,255,"%d",num);
text[255]=0;

STR_Add(*a,text);
}

// Assign to <userstring> the <new string>

void PV_setstr()
{
USERSTRING **a;
char *b;

a = (USERSTRING **)GET_INT();
CHECK_POINTER(a);

b = GET_STRING();
CHECK_POINTER(b);

STR_Set(*a,b);
}

// strlen <len> = len <userstring>

void PV_strlen()
{
VMINT *a;
USERSTRING **b;

a = (VMINT *)GET_INT();
CHECK_POINTER(a);

b = (USERSTRING **)GET_INT();
CHECK_POINTER(b);

// Point to the UserString
*a = strlen((*b)->ptr);
}

// strlen <len> = len <userstring>

void PV_strlen2()
{
VMINT *a;
char *b;

a = (VMINT *)GET_INT();
CHECK_POINTER(a);

b = GET_STRING();
CHECK_POINTER(b);

*a = strlen(b);
}


// set <userstring> <position> = <integer>

void PV_strsetpos()
{
USERSTRING **str;
VMINT *pos,vpos;
VMINT *character;

str = (USERSTRING **)GET_INT();
CHECK_POINTER(str);

pos = (VMINT *)GET_INT();
CHECK_POINTER(pos);

character = (VMINT *)GET_INT();
CHECK_POINTER(character);

vpos = (*pos)-1; // Convert from 1-base to 0-base

if(vpos < 0 || vpos >= (*str)->len)
	return; // don't bother

// Do it
(*str)->ptr[vpos] = (char)*character;
}


// set <integer> = <userstring> <position>

void PV_strgetpos()
{
USERSTRING **str;
VMINT *pos,vpos;
VMINT *intvar;

intvar = (VMINT *)GET_INT();
CHECK_POINTER(intvar);

str = (USERSTRING **)GET_INT();
CHECK_POINTER(str);

pos = (VMINT *)GET_INT();
CHECK_POINTER(pos);

vpos = (*pos)-1; // Convert from 1-base to 0-base
if(vpos < 0 || vpos >= (*str)->len)
	{
	*intvar = 0;
	return; // out of range don't do it
	}

// Do it
*intvar = (int)((*str)->ptr[vpos]);
}

// set <integer> = <string> <position>

void PV_strgetpos2()
{
char *str;
VMINT *pos,vpos;
VMINT *intvar;

intvar = (VMINT *)GET_INT();
CHECK_POINTER(intvar);

str = GET_STRING();
CHECK_POINTER(str);

pos = (VMINT *)GET_INT();
CHECK_POINTER(pos);

vpos = (*pos)-1; // Convert from 1-base to 0-base
if(vpos < 0 || vpos >= strlen(str))
	{
	*intvar = 0;
	return; // out of range don't do it
	}

// Do it
*intvar = (int)str[vpos];
}


void PV_strgetval()
{
char *b;
VMINT *a;

a = (VMINT *)GET_INT();
CHECK_POINTER(a);

b = GET_STRING();
CHECK_POINTER(b);

*a = atoi(b);
}


void PV_IfKey()
{
int jumpoffset,k;
VMINT *a;
VMINT ok;

a = (VMINT *)GET_INT();
CHECK_POINTER(a);
jumpoffset = GET_DWORD();

ok=0;
if(a)
	{
	k = *a;
	if(IRE_TestKey(k))
		ok=1;	// It is pressed
	}

if(!ok)
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}

void PV_IfKeyPressed()
{
int jumpoffset;
jumpoffset = GET_DWORD();

if(!IRE_KeyPressed())
	curvm->ip = ADD_IP(curvm->code,jumpoffset);  // Mess with IP for the jump
}



// Create a text window

void PV_CreateConsole()
{
VMINT *id,*x,*y,*w,*h;

id = GET_INT();
CHECK_POINTER(id);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);
w = GET_INT();
CHECK_POINTER(w);
h = GET_INT();
CHECK_POINTER(h);

*id = CreateConsole(*x,*y,*w,*h);
}


// Delete a text window

void PV_DestroyConsole()
{
VMINT *id;

id = GET_INT();
CHECK_POINTER(id);

DeleteConsole(*id);
}

//Clear a text window

void PV_ClearConsole()
{
VMINT *id;

id = GET_INT();
CHECK_POINTER(id);

ClearConsole(*id);
}


// Set text colour

void PV_ConsoleColour()
{
VMINT *id,*colour;

id = GET_INT();
CHECK_POINTER(id);
colour = GET_INT();
CHECK_POINTER(colour);

SetConsoleColour(*id,*colour);
}

// Add a line

void PV_ConsoleAddLine()
{
VMINT *id;
char *str;

id = GET_INT();
CHECK_POINTER(id);

str = GET_STRING();
CHECK_POINTER(str);

AddConsoleLine(*id,str);
}


void PV_ConsoleAddLineWrap()
{
VMINT *id;
char *str;

id = GET_INT();
CHECK_POINTER(id);

str = GET_STRING();
CHECK_POINTER(str);

AddConsoleLineWrap(*id,str);
}


void PV_DrawConsole()
{
VMINT *id;
id = GET_INT();
CHECK_POINTER(id);

DrawConsole(*id);
}


void PV_ScrollConsole()
{
VMINT *id,*direction;
id = GET_INT();
CHECK_POINTER(id);

direction = GET_INT();
CHECK_POINTER(direction);

ScrollConsole(*id,*direction);
}

void PV_SetConsoleLine()
{
VMINT *id,*lineno;
id = GET_INT();
CHECK_POINTER(id);

lineno = GET_INT();
CHECK_POINTER(lineno);

SetConsoleLine(*id,*lineno);
}


void PV_SetConsolePercent()
{
VMINT *id,*pct;
id = GET_INT();
CHECK_POINTER(id);

pct = GET_INT();
CHECK_POINTER(pct);

SetConsolePercent(*id,*pct);
}

void PV_SetConsoleSelect()
{
VMINT *id,*line;
id = GET_INT();
CHECK_POINTER(id);

line = GET_INT();
CHECK_POINTER(line);

SetConsoleSelect(*id,*line);
}

void PV_SetConsoleSelectS()
{
VMINT *id;
char *line;
id = GET_INT();
CHECK_POINTER(id);

line = GET_STRING();
CHECK_POINTER(line);

SetConsoleSelect(*id,line);
}

void PV_SetConsoleSelectU()
{
VMINT *id;
USERSTRING **line;
id = GET_INT();
CHECK_POINTER(id);

line = (USERSTRING **)GET_INT();
CHECK_POINTER(line);

SetConsoleSelect(*id,(*line)->ptr);
}


void PV_GetConsoleLine()
{
VMINT *id,*line;

line = GET_INT();
CHECK_POINTER(line);

id = GET_INT();
CHECK_POINTER(id);

*line = GetConsoleLine(*id);
}


void PV_GetConsolePercent()
{
VMINT *id,*pct;

pct = GET_INT();
CHECK_POINTER(pct);

id = GET_INT();
CHECK_POINTER(id);

*pct = GetConsolePercent(*id);
}


void PV_GetConsoleSelect()
{
VMINT *id,*line;

line = GET_INT();
CHECK_POINTER(line);

id = GET_INT();
CHECK_POINTER(id);

*line = GetConsoleSelect(*id);
}


void PV_GetConsoleSelectU()
{
VMINT *id;
USERSTRING **line;
int len;
char *ptr;
const char *res;

line = (USERSTRING **)GET_INT();
CHECK_POINTER(line);

id = GET_INT();
CHECK_POINTER(id);

len=(*line)->len;
ptr=(*line)->ptr;

res=GetConsoleSelectS(*id);
if(res)
	STR_Set(*line,res);
else
	(*line)->ptr[0]=0;	// Erase it
}


void PV_GetConsoleLines()
{
VMINT *id,*lines;

lines = GET_INT();
CHECK_POINTER(lines);

id = GET_INT();
CHECK_POINTER(id);

*lines = GetConsoleLines(*id);
}


void PV_GetConsoleRows()
{
VMINT *id,*rows;

rows = GET_INT();
CHECK_POINTER(rows);

id = GET_INT();
CHECK_POINTER(id);

*rows = GetConsoleRows(*id);
}


void PV_FindString()
{
char **out;
char *name;
out = (char **)GET_INT();
CHECK_POINTER(out);

name = GET_STRING();
CHECK_POINTER(name);

if(name)
	*out = getstring4name(name);
else
	*out = NULL;
}


void PV_FindTitle()
{
char **out;
char *name;
out = (char **)GET_INT();
CHECK_POINTER(out);

name = GET_STRING();
CHECK_POINTER(name);

if(name)
	*out = getstringtitle4name(name);
else
	*out = NULL;
}


void PV_AddJournal()
{
char *name;
int id;
name = GET_STRING();
CHECK_POINTER(name);

char date[64];

make_journaldate(date);

// This may be a raw string ('s' or 'S') or a string pointer ('P')
// But we need to treat those differently
if(name) {
	// First try the string pointer
	id=getnum4string(name);
	if(id < 0) {
		// Then try a raw string
		id=getnum4stringname(name);
	}

	if(id < 0)
		Bug("ERROR:  Journal entry not found for string '%s'\n",name);
	else
		J_Add(id,days_passed+1,date);
	}
}


void PV_AddJournal2()
{
char *title,*data;
JOURNALENTRY *entry;

title = GET_STRING();
CHECK_POINTER(title);
data = GET_STRING();
CHECK_POINTER(data);

char date[64];
make_journaldate(date);

entry=J_Add(-1,days_passed+1,date);
if(entry)
	 {
	 entry->title = stradd(NULL,title);
	 entry->text = stradd(NULL,data);
	 }
}

void PV_EndJournal()
{
char *name;
int id;
name = GET_STRING();
CHECK_POINTER(name);

JOURNALENTRY *journal = J_Find(name);
if(!journal) {

	id=getnum4string(name);
	if(id < 0) {
		// Then try a raw string
		id=getnum4stringname(name);
	}
	if(id < 0) {
		pevm_err=1;
		return;
	}
	// It does exist, but hasn't been started
	pevm_err=0;
	return;
}
pevm_err=0;
journal->status = 100;
}


void PV_DrawJournal()
{
VMINT *id;

id = GET_INT();
CHECK_POINTER(id);

ConsoleJournal(*id);
}


void PV_DrawJournalDay()
{
VMINT *id,*day;

id = GET_INT();
CHECK_POINTER(id);
day = GET_INT();
CHECK_POINTER(day);

ConsoleJournalDay(*id,*day);
}

void PV_DrawJournalTasks()
{
VMINT *id,*mode;

id = GET_INT();
CHECK_POINTER(id);
mode = GET_INT();
CHECK_POINTER(mode);

ConsoleJournalTasks(*id,*mode);
}


void PV_GetJournalDays()
{
VMINT *days;
JOURNALENTRY *ptr;
int ctr=0;
int lastday=-2;

days = GET_INT();
CHECK_POINTER(days);

// Get number of unique days in the journal

for(ptr=Journal;ptr;ptr=ptr->next)
	if(ptr->day != lastday)
		{
		ctr++;
		lastday=ptr->day;
		}
*days=ctr;
}


void PV_GetJournalDay()
{
VMINT *dayno,*index;
JOURNALENTRY *ptr;
int ctr=0;
int lastday=-2;

dayno = GET_INT();
CHECK_POINTER(dayno);
index = GET_INT();
CHECK_POINTER(index);

*dayno=0;

// Walk the list and map the index to a day

for(ptr=Journal;ptr;ptr=ptr->next)
	if(ptr->day != lastday)
		{
		ctr++;
		if(ctr == *index)
			{
			*dayno=ptr->day;
			return;
			}
		lastday=ptr->day;
		}
}


// Create a text list

void PV_CreatePicklist()
{
VMINT *id,*x,*y,*w,*h;

id = GET_INT();
CHECK_POINTER(id);
x = GET_INT();
CHECK_POINTER(x);
y = GET_INT();
CHECK_POINTER(y);
w = GET_INT();
CHECK_POINTER(w);
h = GET_INT();
CHECK_POINTER(h);

*id = CreatePicklist(*x,*y,*w,*h);
}


// Delete a text list

void PV_DestroyPicklist()
{
VMINT *id;

id = GET_INT();
CHECK_POINTER(id);

DeletePicklist(*id);
}


// Set text colour

void PV_PicklistColour()
{
VMINT *id,*colour;

id = GET_INT();
CHECK_POINTER(id);
colour = GET_INT();
CHECK_POINTER(colour);

SetPicklistColour(*id,*colour);
}

// Set text colour

void PV_PicklistPoll()
{
VMINT *id;

id = GET_INT();
CHECK_POINTER(id);

PollPicklist(*id);
}


// Add a line

void PV_PicklistAddLine()
{
VMINT *id,*num;
char *str;

id = GET_INT();
CHECK_POINTER(id);

num = GET_INT();
CHECK_POINTER(num);

str = GET_STRING();
CHECK_POINTER(str);

AddPicklistLine(*id,*num,str);
}

// Add a line

void PV_PicklistAddSave()
{
VMINT *id,*num;

id = GET_INT();
CHECK_POINTER(id);

num = GET_INT();
CHECK_POINTER(num);

AddPicklistSave(*id,*num);
}


void PV_PicklistItem()
{
VMINT *id,*out;

out = GET_INT();
CHECK_POINTER(out);

id = GET_INT();
CHECK_POINTER(id);

*out = GetPicklistItem(*id);
}


void PV_PicklistId()
{
VMINT *id,*out;

out = GET_INT();
CHECK_POINTER(out);

id = GET_INT();
CHECK_POINTER(id);

*out = GetPicklistId(*id);
}


void PV_PicklistText()
{
VMINT *id;
USERSTRING **line;
int len;
char *ptr;
const char *res;

line = (USERSTRING **)GET_INT();
CHECK_POINTER(line);

id = GET_INT();
CHECK_POINTER(id);

len=(*line)->len;
ptr=(*line)->ptr;

res=GetPicklistText(*id);
if(res)
	STR_Set(*line,res);
else
	(*line)->ptr[0]=0;	// Erase it
}


void PV_SetPicklistPercent()
{
VMINT *id,*pct;
id = GET_INT();
CHECK_POINTER(id);

pct = GET_INT();
CHECK_POINTER(pct);

SetPicklistPercent(*id,*pct);
}


void PV_GetPicklistPercent()
{
VMINT *id,*pct;

pct = GET_INT();
CHECK_POINTER(pct);

id = GET_INT();
CHECK_POINTER(id);

*pct = GetPicklistPercent(*id);
}



void PV_getvolume()
{
VMINT *output;
VMINT *port;

int out=0;

output = GET_INT();
CHECK_POINTER(output);

port = GET_INT();
CHECK_POINTER(port);

switch(*port)
	{
	case 0:
		out=sf_volume;
		break;

	case 1:
		out=mu_volume;
		break;

	default:
		out=0;
		break;
	}
	
*output = out;
}

void PV_setvolume()
{
VMINT *port;
VMINT *level;
int val;

port = GET_INT();
CHECK_POINTER(port);

level = GET_INT();
CHECK_POINTER(level);

val=*level;
if(val<0)
	val=0;
if(val>100)
	val=100;

switch(*port)
	{
	case 0:
		S_SoundVolume(val);
		break;

	case 1:
		S_MusicVolume(val);
		break;
	}
}


void PV_LastSaveSlot()
{
VMINT *output;

output = GET_INT();
CHECK_POINTER(output);

*output = FindLastSave();
}


void PV_SaveGame()
{
VMINT *slot;
USERSTRING **line;

slot = GET_INT();
CHECK_POINTER(slot);

line = (USERSTRING **)GET_INT();
CHECK_POINTER(line);

if(*line)
	SaveGame(*slot,(*line)->ptr);
}


void PV_LoadGame()
{
VMINT *slot;

slot = GET_INT();
CHECK_POINTER(slot);

pevm_err=LoadGame(*slot);
if(pevm_err)
	ire_running=-1; // Success
else
	ire_running=1;
}


void PV_GetDate()
{
USERSTRING **out;
char date[64];
time_t t;
t=time(NULL);
SAFE_STRCPY(date,ctime(&t));

// ctime() likes to add a CR and stuff.. remove it
strdeck(date,0x0d);
strdeck(date,0x0a);
strstrip(date);

out = (USERSTRING **)GET_INT();
CHECK_POINTER(out);
if(out && *out)
	STR_Set(*out,date);
}


//
// LASTOP: new opcodes go above this line
//


// Print various status items (for debugging)

void PV_LastOp()
{
OBJLIST *t,*active;
OBJECT *o;
int ctr;
VMINT *num;
VMINT bytes;
num = GET_INT();

switch(*num)
	{
	case 0:
	ctr=0;
	for(t=ActiveList;t;t=t->next) ctr++;
	irecon_printf("%d Active objects\n",ctr);
	break;

	case 1:
	ilog_printf("current = %p\n",current_object);
	if(current_object)
		ilog_printf("current = '%s'\n",current_object->name);
	break;

	case 2:
	ctr=0;
	bytes=0;
	for(t=MasterList;t;t=t->next)
		{
		o=t->ptr;
		bytes += sizeof(OBJECT);
		if(o->stats)
			bytes += sizeof(STATS);
		if(o->funcs)
			bytes += sizeof(FUNCS);
		if(o->user)
			bytes += sizeof(USEDATA);
		if(o->labels)
			bytes += sizeof(CHAR_LABELS);
		ctr++;
		}
	irecon_printf("%d objects in masterlist (%d bytes approx.)\n",ctr,bytes);
	break;

	case 3:
	irecon_printf("Stack trace written to logfile\n");
	VMstacktrace();
	break;

	case 4:
//	irecon_printf("Causing Panic at file e\n");
	DebugVM=1;
	VMstacktrace();
	Bang("Script code requested panic (status 4)");
	break;

	case 5:
	if(current_object)
		{
		if(ML_InList(&ActiveList,current_object))
			irecon_printf("%s is active\n",current_object->name);
		else
			irecon_printf("%s is not active\n",current_object->name);
		}
	break;

	case 6:
	if(current_object)
		{
		for(ctr=0;ctr<ACT_STACK;ctr++)
		if(current_object->user->actlist[ctr]>0)
			{
			ilog_printf("%d: do %s",ctr,PElist[current_object->user->actlist[ctr]].name);
			if(current_object->user->acttarget[ctr])
				ilog_printf(" to %s",current_object->user->acttarget[ctr]->name);
			ilog_printf("\n");
			}
		}
	break;

	case 7:
		// Quit game
		ire_running=0;
	break;

	case 8:
		pevm_err = LoadGame();
		if(pevm_err)
			ire_running=-1; // Success
		else
			ire_running=1;
	break;

	case 9:
		SaveGame();
	break;

	case 10:
		// NA
//		SoundSettings();
	break;

	case 11:
		// Project shrunk map (wizard eye etc)
		pevm_err=ZoomMap(pe_usernum1,pe_usernum2);
	break;

	case 12:
		// Is the map allowed?
		pevm_err=DisallowMap();
	break;

	case 13:
		// Screenshot
		CentreMap(player);
		Show();
		Screenshot(swapscreen);
	break;
	
	case 14:
		ilog_printf("Dumping activelist\n");
		for(active=ActiveList;active;active=active->next)
			ilog_quiet("%s\n", active->ptr->name);
		ilog_printf("Done\n");
	break;

	case 15:
		show_vrm_calls=!show_vrm_calls;
		ilog_printf("VM listing now %s\n",show_vrm_calls?"ON":"OFF");
	break;

	case 16:
		DebugVM=!DebugVM;
		ilog_printf("VM Debugging now %s\n",DebugVM?"ON":"OFF");
	break;

	case 17:
		Cheat_gimme();
	break;

	case 18:
		Cheat_setflag();
	break;

	case 19:
		ire_pushkey = pevm_err;
		pevm_err=0;
	break;

	case 20:
		if(!current_object) {
			return;
		}
		ilog_printf(">>> Dump schedule uuids for %s (%s)\n",current_object->name, current_object->personalname);
		if(current_object->schedule) {
			for(int ctr2=0;ctr2<24;ctr2++) {
				if(current_object->schedule[ctr2].active) {
					ilog_printf("%d: hour = %d, activity = %s, target = %s\n",ctr2,current_object->schedule[ctr2].hour,current_object->schedule[ctr2].vrm,current_object->schedule[ctr2].uuid);
				}
			}
		}
		ilog_printf(">>> Dump schedule done\n");

	break;

	default:
	Bug("Unknown status request %d\n",*num);
	}

}


// DumpVM

void PV_Dump()
{
int dvm;
dvm = DebugVM;
DebugVM=1;
DumpVM(1);
DebugVM = dvm;
}

// No Operation

void PV_NOP()
{
}

/*
 *  For VM debugging of pointers
 *  To make sure what we're changing is fully inside the address-space
 *  This is probably not portable and certainly not for general use.
 *  If it prevents the engine compiling on some future arch or compiler
 *  just comment the thing out.
 */
/*
void CheckRange(PEVM *vm, void *ptr, int ident)
{
unsigned int a,b,c,d;

a=(unsigned int)vm->code;
b=(unsigned int)vm->size;
c=a+b;
d=(unsigned int)ptr;

if(d<a || d>c)
	{
	printf("%s: POINTER 0x%p OUT OF RANGE (0x%x-0x%x), ident %d\n",vm->name,ptr,a,c,ident);
//	CRASH();
	}
}
*/

/*
 * DO Fix-Up System, internal system for local variable management
 * Pronounced 'doofus'
 */

/*
 This is an opcode that is needed in all functions that have local
 variables.  It is added automatically by the compiler, which will
 also have built a table of offsets and allocated space for the
 local variables to use.
 DOFUS goes through the table, and transforms each reference into a
 pointer that the VM will use.  The pointer must point to a slot
 allocated for the variable by the compiler.  Since local variables
 can and will have multiple instances, the fixup must be done at
 runtime whenever the function is called.  That's what DOFUS does.

 The memory map for functions with local variables will look like
 this:

   CODE
    :
   DOFUS PATCH TABLE
    :
   LOCAL VARIABLE DATASTORE

 ..it would be tempting to reuse the patch table for the datastore
 since it won't be needed after DOFUS has run, but alas the variables
 are not patched in physical storage order so this doesn't work.
 It might be possible to force the compiler to do this, but that is
 left as an exercise for the reader.

 The patch table consists of two words per entry; the offset into
 the code, and the variable slot number to use.

 The final point of note is that locals can be initialised to default
 values.  These default values are actually in the code!  Each reference
 to a variable in the code will have the default value stuffed in it
 so must be picked up and put into the variable store, before we can
 put the pointer in it's place.

*/

void PV_Dofus()
{
VMUINT num,ctr,val,*value_ptr;
VMUINT varoff,slotno,dataoff;
VMTYPE *varptr,*slotaddr,*data;

//#define DOFUSDUMP // For debugging the local variable fixup system
//#define DOFUS_DEBUG

#ifdef DOFUS_DEBUG
ilog_quiet("in %s\n",debcurfunc);
#define DOFUS_LOG(x,y); ilog_quiet(x,y);
#define DOFUS_LOG2(x,y); ilog_quiet(x,y,(y)-(VMINT)(curvm->code));
#else
#define DOFUS_LOG(x,y); ;
#define DOFUS_LOG2(x,y); ;
#endif

#ifdef DOFUSDUMP
//ilog_quiet("%s\n",debcurfunc);
ilog_quiet("Doofus: dump before: ");
unsigned char *cc=(unsigned char *)curvm->code;
for(ctr=0;ctr<curvm->size;ctr++)
	{
	if(!(ctr % 16))
		ilog_quiet("\n%-3x  ",ctr/16);
	ilog_quiet("%02x ",cc[ctr]&255);
	}
ilog_quiet("\n");
#endif

num = GET_RAW_DWORD(); // Get number of patches
DOFUS_LOG("0x%x patches\n",num);

// Get the offset of the patch table into the VM's memory space
dataoff = GET_RAW_DWORD();
DOFUS_LOG("ptoffset = 0x%x\n",dataoff);

// Turn the patch table offset into an actual pointer
// by adding the address of the start of the function
data = ADD_IP(curvm->code,dataoff);
DOFUS_LOG2("patchptr = 0x%x [0x%x]\n",data);

// Variable slots are after the patch table, i.e. the patch table's address
// plus the no. of patches times the word size x2 (each patch has two entries)
slotaddr = data + (num*2); // Each slot takes two words
#ifdef DOFUS_DEBUG
ilog_quiet("(address space = 0x%x -> 0x%x)\n",curvm->code,(VMUINT)curvm->code + (VMUINT)curvm->size);
#endif
DOFUS_LOG2("slotaddress = 0x%x [0x%x]\n---\n",slotaddr->u32);

// Patch each of the entries

for(ctr=0;ctr<num;ctr++)
	{
	// Get the first word (variable offset)
	DOFUS_LOG("dataptr = %p\n",data);
	varoff = (*data++).u32;
	DOFUS_LOG("dataptr now %p\n",data);
	DOFUS_LOG("var offset = %ld\n",varoff);

//	printf("sl=%d, su32=%d, sptr=%d\n",sizeof(VMINT),sizeof((*data).u32),sizeof(void *));


	// Get the second word (variable slot number) and increment again
	slotno = (*data++).u32;
	DOFUS_LOG("slot number = %ld\n",slotno);

	// Make the code offset into a pointer
	varptr=ADD_IP(curvm->ip,varoff);
	DOFUS_LOG2("patch address = 0x%p [0x%lx]\n",varptr->l);

	// Get the value that was there, this is the variable's default value
	// We put that into the var.slot later.  This could be done at build time
	// which may speed things up slightly.
	val = (*varptr).u32;
	DOFUS_LOG("def value = %ld\n",val);

	// Replace the integer value that was there with the pointerized version
	// That the VM will use.
	(*varptr).ptr = slotaddr + slotno;
	DOFUS_LOG2("varslot ptr = 0x%p [0x%lx]\n",varptr->u32);

	// Fill the variable slot with the default value obtained above
//	ilog_quiet("varptr = %p\n",varptr->ptr);
	value_ptr = (VMUINT *)(*varptr).ptr; // Get address
	DOFUS_LOG("old slotvalue = %ld\n",*value_ptr);
	*value_ptr = val; // Write default value into the variable

#ifdef DOFUS_DEBUG
ilog_quiet("round again.. [%d/%d]\n\n",ctr+1,num);
#endif
	}
#ifdef DOFUSDUMP
ilog_quiet("Doofus: dump after\n");

//cc=(unsigned char *)curvm->code;
for(ctr=0;ctr<curvm->size;ctr++)
	{
	if(!(ctr % 16))
		ilog_quiet("\n%-3x  ",ctr/16);
	ilog_quiet("%02x ",cc[ctr]&255);
	}
ilog_quiet("\n");
ilog_quiet("DumpVM\n");

DumpVM(0);
ilog_quiet("Doofus: signing off\n");
#endif
}


//
//	Math Operators
//

int OP_Invalid(int a, int b)
{
Bug("Unknown operator! Either 'if %d ? %d' or let x = %d ? %d\n",a,b,a,b);
VMbug();
return 0;
}

int OP_Add(int a, int b)
{
return a+b;
}

int OP_Sub(int a, int b)
{
return a-b;
}

int OP_Div(int a, int b)
{
if(b==0)
	return 0; // No, this is not mathematically correct
return a/b;
}

int OP_Mul(int a, int b)
{
return a*b;
}

int OP_Equal(int a, int b)
{
return (a==b);
}

int OP_NotEqual(int a, int b)
{
//ilog_quiet("cmp %x != %x : result %d\n",a,b,(a!=b));
return (a!=b);
}

int OP_Less(int a, int b)
{
return (a<b);
}

int OP_LessEq(int a, int b)
{
return (a<=b);
}

int OP_Great(int a, int b)
{
return (a>b);
}

int OP_GreatEq(int a, int b)
{
return (a>=b);
}

int OP_Modulus(int a, int b)
{
if(b==0)
	return 0; // No, this is not mathematically correct
return a%b;
}

int OP_And(int a, int b)
{
return a&b;
}

int OP_Or(int a, int b)
{
return a|b;
}

int OP_Xor(int a, int b)
{
return a^b;
}

int OP_Nand(int a, int b)
{
return a&((VMUINT)b^0xffffffff);
}

int OP_Shl(int a, int b)
{
return (int)((VMUINT)a<<(VMUINT)b);
}

int OP_Shr(int a, int b)
{
return (int)((VMUINT)a>>(VMUINT)b);
}

/*
// Write to a string character

void PV_PokeStr()
{
char *name;
VMTYPE *strptr;
int *p;

// Get the address of the start of the string
name = GET_STRING();
CHECK_POINTER(name);

// Now go backwards from the start of the string to get the max. length
strptr = (VMTYPE *)name;
strptr--; // Back one word, this will be the string length
len=(*strptr).i32;

// Get the string offset to wurdle
p=GET_INT();
CHECK_POINTER(p);

b=GET_INT();

}
*/
