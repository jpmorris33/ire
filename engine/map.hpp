//
//      Map manipulation functions
//

extern int isSolid(int x,int y);
extern int isDecor(int x,int y);
extern int BlocksLight(int x,int y);
extern int AlwaysSolid(int x,int y);
extern void CentreMap(OBJECT *focus);
extern TILE *GetTile(int x,int y);
extern OBJECT *GetSolidMap(int x,int y);
extern int GetTileCost(int x,int y,OBJECT *npc);
extern int IsTileSolid(int x,int y);
extern int IsTileWater(int x,int y);
extern int LargeIntersect(OBJECT *temp, int x, int y);

