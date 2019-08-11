//
//	IRE initialisation systems
//

#include "core.hpp"

// Defines

#define FORCE_SHAPE 1
#define FORCE_FRAME 2

//#define CHECK_OBJECT_STRICT	// Enable check_object debugging, very slow


// functions

extern int OB_Check(OBJECT *o);
extern OBJECT *OB_Alloc();
extern OBJECT *OB_Decor(char *name);
extern OBJECT *OB_Decor_num(int num);
extern void OB_Kill(OBJECT *a);
extern void OB_Free(OBJECT *a);
extern void OB_Funcs(OBJECT *a);
extern void LL_Add(OBJECT **base, OBJECT *obj);
extern void LL_Add2(OBJECT **base, OBJECT *obj);
extern void LL_Remove(OBJECT **base, OBJECT *obj);
extern int LL_Query(OBJECT **base,OBJECT *object);
extern void LL_Kill(OBJECT *obj);
extern void ML_Add(OBJLIST **m,OBJECT *a);
extern void ML_Del(OBJLIST **m,OBJECT *a);
extern void AL_Add(OBJLIST **m,OBJECT *a);
extern void AL_Del(OBJLIST **m,OBJECT *a);
extern int ML_Query(OBJLIST **m,char *name);
extern int ML_InList(OBJLIST **m,OBJECT *o);

extern void AL_Add(OBJLIST **a,OBJECT *o);
extern int LLcache_register(void *ptr);
extern int LLcache_ALC(char *fname, int line);
#define LLCHECK() LLcache_ALC(__FILE__,__LINE__)

extern int OB_Init(OBJECT *a,char *name);
extern OBJECT *OB_Copy(OBJECT *a);
extern int OB_SetDir(OBJECT *objsel,int dir, int force);
extern void OB_SetSeq(OBJECT *objsel,char *seq);

extern OBJECT *OB_Previous(OBJECT *a);
extern void FreePockets(OBJECT *obj);

extern void ShrinkDecor(OBJECT *o);
extern void ExpandDecor(OBJECT *o);

extern int OB_TurnDir(OBJECT *objsel,int dir);

extern USERSTRING *STR_New(int length);
extern void STR_Add(USERSTRING *u, const char *str);
extern void STR_Set(USERSTRING *u, const char *str);
extern void STR_Free(USERSTRING *u);

extern JOURNALENTRY *J_Add(int id, int day, const char *date);
extern void J_Del(JOURNALENTRY *j);
extern void J_Free();
extern JOURNALENTRY *J_Find(const char *name);


#ifdef CHECK_OBJECT_STRICT
	#define CHECK_OBJECT(ptr) if(!OB_Check(ptr)) {Bug("Screwup in %s at line %d\n",__FILE__,__LINE__);ithe_panic("Screwup detected, check logfile",NULL);}
#else
	#define CHECK_OBJECT(ptr) ;
#endif

// Variables


extern int NoDeps;
extern char AL_dirty;         // Has the ActiveList changed?

