/*
 *      Nuspeech.hpp - Mark II conversation engine
 */

#define MAXFLAGS 8192

extern char tFlag[MAXFLAGS];
extern char *tFlagIdentifier[MAXFLAGS];
extern void Wipe_tFlags();
extern int Get_tFlag(const char *name);
extern void Set_tFlag(const char *name,int a);
extern int NPC_Converse(const char *file,const char *startpage);
extern void NPC_ScrollPic(const char *file, int dx, int dy, int wait1, int wait2, int scrolldelay);
extern void NPC_Init();
extern void NPC_BeenRead(OBJECT *o,const char *page, const char *reader);
extern int NPC_WasRead(OBJECT *o,const char *page, const char *reader);
extern void NPC_set_lFlag(OBJECT *o,const char *page, const char *reader, int state);
extern int NPC_get_lFlag(OBJECT *o,const char *page, const char *reader);
extern void NPC_ReadWipe(OBJECT *o, int wflags);
extern char *NPC_MakeName(OBJECT *o);

#ifdef __IRCORE
extern void setPflag(OBJECT *o,int n,int s);
extern int getPflag(OBJECT *o,int n);
#endif
