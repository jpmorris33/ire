//
//      Interaction between the objects and the map
//

#include "core.hpp"


void trigger_objects(OBJECT *s);
int WeighObject(OBJECT *list);
int add_to_party(OBJECT *new_member);
void seek(OBJECT *hunter,OBJECT *target);
int ChooseLeader(int x);
void SubFromParty(OBJECT *member);

// LinkedlistWorld functions

void MoveToPocket(OBJECT *object, OBJECT *container);
void TransferToPocket(OBJECT *object, OBJECT *container);
int MoveFromPocket(OBJECT *object, OBJECT *container,int x,int y);
int ForceFromPocket(OBJECT *object, OBJECT *container,int x,int y);
OBJECT *GetAverageObject();
OBJECT *GetSolidObject(int x,int y);
OBJECT *GetRawSolidObject(int x,int y, OBJECT *except);
OBJECT *GetTopObject(int x,int y);
OBJECT *GetBestPlace(int x,int y);
OBJECT *GetUserSpec(int x,int y,int inc,int exc,int layer);

// MatrixWorld functions

extern int MoveToMap(int x, int y, OBJECT *object);
extern int DropObject(int x,int y, OBJECT *object);
extern void TakeObject(int x,int y, OBJECT *container);
extern int MoveObject(OBJECT *object,int x,int y,int pushed);
extern void TransferObject(OBJECT *object,int x,int y);
extern void TransferDecor(int x1, int y1, int x2, int y2);
extern OBJECT *GetObject(int x,int y);
extern OBJECT *GetObjectBase(int x,int y);
extern OBJECT *GameGetObject(int x,int y);
extern OBJECT *GetRawObjectBase(int x,int y);
extern OBJECT *GetBridge(int x,int y);
extern void ForceDropObject(int x,int y, OBJECT *object);
extern void DestroyObject(OBJECT *obj);
extern void DelDecor(int x,int y, OBJECT *o);
extern int GetDecor(int *x,int *y, OBJECT *o);
extern void gen_largemap();
extern int OB_CheckPos(OBJECT *o);

extern int InPocket(OBJECT *obj);
extern void CreateContents(OBJECT *obj);
extern void EraseContents(OBJECT *cont);
extern void MoveToTop(OBJECT *object);
extern void MoveToFloor(OBJECT *object);

extern void ActivityNum(OBJECT *o,int activity, OBJECT *target);
extern void ActivityName(OBJECT *o,char *activity, OBJECT *target);
extern void Inactive(OBJECT *o);
extern void ResumeActivity(OBJECT *o);

extern OBJECT *FindTag(int tag,char *name);
extern void CheckDepend(OBJECT *ptr);
extern void MassCheckDepend();
extern void DeletePending();
extern void DeletePendingProgress(void (*func)(int count, int max));
extern void UnWield(OBJECT *container, OBJECT *dest);


void DeleteObject(OBJECT *o);
extern char pending_delete;

extern void SubAction_Push(OBJECT *o,int activity,OBJECT *target);
extern void SubAction_Pop(OBJECT *o,int *activity,OBJECT **target);
extern void SubAction_Wipe(OBJECT *o);


extern void SetLocation(OBJECT *obj, char *loc);
extern void GetLocationList(char ***list,int *num);

extern OBJECT **GetWieldPtr(OBJECT *container, int handle);
extern OBJECT *GetWielded(OBJECT *container, int handle);
extern int SetWielded(OBJECT *container, int handle, OBJECT *item);
extern OBJECT **FindWieldPoint(OBJECT *object, int *handle);

//extern char CheckDepend_on;
