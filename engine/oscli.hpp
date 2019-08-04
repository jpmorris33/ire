/*
 *  OSCLI routines.  Process arguments and INI files
 *
 *  NB, in case you don't know/remember,  OSCLI stands for
 *  Operating System Command Line Interpreter
 */

#include "types.h"

// Functions
extern void OSCLI(int ArgC,char **ArgV);
extern int INI_file(char *ini);
extern int TryINI(char *filename);

// Variables
extern char projectname[];
extern char newbootlog[];
extern char oldbootlog[];
extern char loadingpic[];
extern char in_editor;
extern int VideoMode;
extern char test_console;
extern char no_panic;
extern char debug_nosound;
extern char SuperPanic;
extern char conlog;
extern char ire_NOASM;
extern bool ire_checkcache;
extern bool ire_recache;
extern char skip_unk;
extern char ire_showfuncs;
extern char ire_U6partymode;
extern char show_vrm_calls;
extern char use_light;
extern char vis_blanking;
extern char reset_stats;
extern char reset_funcs;
extern char probeInvalidObject;
extern char enable_censorware;
extern char use_hw_video;
extern char fixwalls;
extern char allow_breakout;
extern char ShowRanges;
extern char debug_act;
extern char SkipInit;
extern int blankercol;
extern VMINT speechfont;
extern int logx,logy,loglen;
extern int conx,cony,conlen,conwid;
extern int ig_min,ig_hour,ig_day,ig_month,ig_year,ig_dow;
extern int turns_min,min_hour,hour_day,day_month,month_year,day_week;
extern int destroyhp,imapx,imapy;
extern int EggLockout,EggDistance;
extern VMINT default_mapnumber;
extern int StartingTag;
extern int vcpu_memory;
extern char bookview[];
extern char bookview2[];
extern char editarea[];
extern char projectdir[];
extern unsigned int FRAMERATE;
extern unsigned int MouseWait;
extern VMINT map_W,map_H;	       // Default map size
extern int journalstyle;

extern unsigned char dlink_r,dlink_g,dlink_b;	// Default link colour
extern unsigned char dtext_r,dtext_g,dtext_b;	// Default conv. colour

