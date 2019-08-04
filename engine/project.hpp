/*
 *      Project - Sprite projection code
 */

// Defines

// Variables

extern void (*animate[8])(OBJECT *temp);
extern VMINT show_invisible;

// Functions

extern void Init_Projector(int x, int y);
extern void Project_Map(int x, int y);
extern void Project_Sprites(int x, int y);
extern void Project_Roof(int x, int y);
extern void MicroMap(int x, int y, int divisor, int roof);
extern void Hide_Unseen_Map();
extern void Hide_Unseen_Project();
extern void render_lighting(int x, int y);
extern int isBlanked(int x, int y);
//void Project_Darkness();
extern void SetDarkness(int d);
extern int GetDarkness();
extern void Project_lightbeam(int x, int y, int x2, int y2, int w);
extern void Project_darkbeam(int x, int y, int x2, int y2, int w);
extern void anim_random(OBJECT *temp);

extern int fastrandom();
