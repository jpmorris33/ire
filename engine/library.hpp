//
//      VRM_IN.HPP - Internal version of vrm.hpp
//

#ifndef IRE_NATIVE_EXECUTION_MODULE_HEADER_FILE
#define IRE_NATIVE_EXECUTION_MODULE_HEADER_FILE

#include "core.hpp"

typedef char *STRING;


#ifndef NULL
  #define NULL 0L
#endif


/////// Virtual Runnable Module subroutines ////////

#define AND &&
#define OR ||
#define NOT !

extern void print(char *msg, ...);
extern void printF(char *msg, ...);
extern void vrmclear();
extern void printxy(int x,int y,char *msg, ...);
extern void TextColor(int r,int g, int b);

extern OBJECT *create_object(STRING name, int x, int y);
extern void delete_pending(void);
extern void remove_object(OBJECT *obj);
extern void change_object(OBJECT *obj, STRING name);
extern void replace_object(OBJECT *obj, STRING name);
extern void set_object_direction(OBJECT *obj, int dir);
extern void set_object_sequence(OBJECT *obj, STRING name);
extern void set_object_behaviour(OBJECT *obj, STRING name);
extern void add_goal(OBJECT *o, int priority, char *vrm, OBJECT *target);
extern void end_goal(OBJECT *o);
extern void wipe_goals(OBJECT *o);
extern void kill_goals(OBJECT *o,int p);
#define set_object_behavior set_object_behaviour
extern void spill_contents(OBJECT *bag);
extern void spill_contents_at(OBJECT *bag,int x,int y);
extern int get_flag(OBJECT *target, VMUINT flag);
extern int get_tileflag(TILE *target, VMUINT flag);
extern void set_flag(OBJECT *target, VMUINT flag, int state);
extern void default_flag(OBJECT *target, VMUINT flag);
extern void add_quantity(OBJECT *container, char *objecttype, int quantity);
extern int take_quantity(OBJECT *container, char *objecttype, int quantity);
extern int move_quantity(OBJECT *src, OBJECT *dest, char *objecttype, int quantity);
extern int count_objects(OBJECT *container, char *objecttype);
extern OBJECT *find_object_with_tag(int a, char *name);
extern OBJECT *find_container(OBJECT *ex);
extern int get_pflag(OBJECT *target, VMUINT flag);
extern void set_pflag(OBJECT *target, VMUINT flag, int state);
extern int get_user_flag(char *flag);
extern void set_user_flag(char *flag, int state);
extern int get_local(OBJECT *pl,OBJECT *ch,char *flag);
extern void set_local(OBJECT *pl,OBJECT *ch,char *flag, int state);
extern void show_object(OBJECT *z,int x,int y);
extern void call_vrm(char *name);
extern void call_vrm_number(int name);
extern char *get_vrm_name(int num);
extern void play_song(char *song);
extern void play_sound(char *sound);
extern void object_sound(char *sound, OBJECT *o);
extern void start_song();
extern void stop_song();
extern int object_is_called(OBJECT *obj,char *str);
extern int rnd(int max);
extern void waitfor(unsigned millisec);
extern void waitredraw(unsigned ms);
extern int get_number(int no);

extern TILE *get_tile(int x,int y);
extern OBJECT *get_object(int x,int y);
extern OBJECT *get_solid_object(int x,int y);
extern OBJECT *get_first_object(int x,int y);
extern OBJECT *get_object_below(OBJECT *o);
extern OBJECT *search_container(OBJECT *pocket, char *name);
extern int count_active_objects();
extern void find_objects_with_tag(VMINT a, char *name, VMINT *num, OBJECT **list);

extern void move_to_pocket(OBJECT *src,OBJECT *dest);
extern void transfer_to_pocket(OBJECT *src,OBJECT *dest);
extern int force_from_pocket(OBJECT *obj,OBJECT *container,int x,int y);
extern int move_from_pocket(OBJECT *obj,OBJECT *container,int x,int y);
extern void move_to_top(OBJECT *obj);
extern void move_to_floor(OBJECT *obj);
extern int weigh_object(OBJECT *obj);
extern int get_bulk(OBJECT *obj);
extern int is_tile_solid(int x,int y);
extern int is_tile_water(int x,int y);
extern int is_solid(int x,int y);
extern int in_pocket(OBJECT *o);
extern int move_object(OBJECT *src,int x,int y);
extern void transfer_object(OBJECT *src,int x,int y);
extern int line_of_sight(int xa, int ya, int xb, int yb, int *tx, int *ty, OBJECT *ignore);
extern int move_forward(OBJECT *a);
extern int move_backward(OBJECT *a);
extern int turn_l(OBJECT *a);
extern int turn_r(OBJECT *a);
extern void wait_for_animation(OBJECT *a);
extern int move_towards(OBJECT *object, OBJECT *end);
extern int move_towards_4(OBJECT *object, OBJECT *end);
extern int move_towards_8(OBJECT *object, OBJECT *end);
extern int move_thick(OBJECT *object, OBJECT *end);
extern void move_stack(OBJECT *object, int x, int y);

//extern  int add_to_party(OBJECT *new_member);
extern void remove_from_party(OBJECT *member);
extern int choose_member(OBJECT *member, char *player);
extern int choose_leader(OBJECT *leader, char *player, char *others);
extern void move_party_from_object(OBJECT *o, int x, int y);
extern void move_party_to_object(OBJECT *o);

extern void set_darkness(int level);
extern void redraw();
extern void restart();
extern void get_input();
extern int talk_to(char *speechfile,char *startpage);
extern void check_hurt(OBJECT *list);
extern void redraw_map();
extern int get_yn(char *question);
extern void ___lightning(int ticks);
extern void scroll_tile(char *name, int x, int y);
extern void scroll_tile_number(int no, int x, int y);
extern void scroll_tile_reset(int no);
extern void vrmfade_in();
extern void vrmfade_out();
extern void check_time();
extern void check_time(VMINT *timeptr);
extern void check_date(VMINT *timeptr);
extern void unpack_time(VMINT packed, VMINT *year, VMINT *month, VMINT *day, VMINT *hour);
extern VMINT pack_time(VMINT year, VMINT month, VMINT day, VMINT hour);
extern void unpack_date(VMINT packed, VMINT *year, VMINT *month, VMINT *day);
extern VMINT pack_date(VMINT year, VMINT month, VMINT day);

extern OBJECT *find_nearest(OBJECT *o, char *type);
extern OBJECT *find_pathmarker(OBJECT *o, char *name);
extern OBJECT *find_neartag(OBJECT *o, char *type, int tag);

extern char *best_name(OBJECT *o);
extern int character_onscreen(char *pname);
extern int object_onscreen(OBJECT *a);
extern void resume_schedule(OBJECT *o);
extern void resync_everything();
extern void check_object(OBJECT *o, char *label);
extern void set_light(int x, int y, int x2, int y2, int light);
extern void CalcOrbit(VMINT *angle, VMINT radius, VMINT drift, VMINT speed, VMINT *x, VMINT *y, VMINT ox, VMINT oy);
extern void InitOrbit();
//extern void ProjectCorona(int x,int y,int w,int h, int intensity, int falloff);
extern void ProjectCorona(VMINT x,VMINT y,VMINT radius, VMINT intensity, VMINT falloff, VMINT tint);
extern void ProjectBlackCorona(VMINT x,VMINT y,VMINT radius, VMINT intensity, VMINT falloff, VMINT tint);

extern void InitConsoles();
extern void TermConsoles();
extern int CreateConsole(int x, int y, int wpix, int hpix);
extern void DeleteConsole(int console);
extern void SetConsoleColour(int console, unsigned int packed);
extern void AddConsoleLine(int console, const char *line);
extern void AddConsoleLineWrap(int console, const char *line);
extern void ClearConsole(int console);

extern void DrawConsole(int console);
extern void ScrollConsole(int console, int dir);
extern void SetConsoleLine(int console, int line);
extern void SetConsolePercent(int console, int percent);
extern void SetConsoleSelect(int console, int line);
extern void SetConsoleSelect(int console, const char *line);
extern int GetConsoleLine(int console);
extern int GetConsolePercent(int console);
extern int GetConsoleSelect(int console);
extern const char *GetConsoleSelectS(int console);
extern int GetConsoleLines(int console);
extern int GetConsoleRows(int console);
extern void ConsoleJournal(int console);
extern void ConsoleJournalDay(int console, int day);
extern void ConsoleJournalTasks(int console, int mode);

extern void make_journaldate(char *date);

extern void InitPicklists();
extern void TermPicklists();
extern int CreatePicklist(int x, int y, int wpix, int hpix);
extern void DeletePicklist(int console);
extern void SetPicklistColour(int console, unsigned int packed);
extern void PollPicklist(int console);
extern void AddPicklistLine(int console, int id, const char *line);
extern void AddPicklistSave(int console, int savenum);
extern int GetPicklistItem(int console);
extern int GetPicklistId(int console);
extern const char *GetPicklistText(int console);
extern void SetPicklistLine(int console, int line);
extern void SetPicklistPercent(int console, int percent);
extern int GetPicklistLine(int console);
extern int GetPicklistPercent(int console);


extern int FindLastSave();


#define hurt_object(x,y) {x->stats->hp-=y; check_hurt(x);}

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

#define CHAR_U 0
#define CHAR_D 1
#define CHAR_L 2
#define CHAR_R 3

#define MAX_MEMBERS 128

// Tint bitmasks
#define TINT_RED 1
#define TINT_GREEN 2
#define TINT_BLUE 4
#define TINT_YELLOW 3
#define TINT_PURPLE 5
#define TINT_MAGENTA 5
#define TINT_CYAN 6
#define TINT_LIGHTBLUE 6
#define TINT_WHITE 7

#define JOURNAL_ALL 0
#define JOURNAL_TODO_ONLY 1
#define JOURNAL_TODO_HEADER_ONLY 2
#define JOURNAL_DONE_ONLY 3
#define JOURNAL_DONE_HEADER_ONLY 4

#endif

