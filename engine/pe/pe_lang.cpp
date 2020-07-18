/*
 *      PEscript compiler Language Definition
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core.hpp"
#include "../gamedata.hpp"
#include "../ithelib.h"
#include "../mouse.hpp"
#include "../init.hpp"
#include "../linklist.hpp"
#include "../library.hpp"
#include "../graphics/irekeys.hpp"
#include "opcodes.h"
#include "pe_api.hpp"

#define IF_LABEL_NOP_HACK // Hack for gotos inside IF statements
#define ADD_CONST(x) add_symbol_val(#x,'n',x)
#define ADD_IREKEY(x) add_symbol_val(#x,'n',IRE##x)

#define MAX_ORS 255	// If you have 255 OR statements in a row, something's off

// Write the globals to the log as they are created, for debugging
//#define LOG_ADDING_GLOBALS

extern OBJECT *player;
extern OBJECT *current_object;
extern OBJECT *victim;
extern OBJECT *syspocket;
extern OBJECT *moveobject_blockage;
extern char *compilename;
extern VMINT map_W,map_H;	       // Default map size
int PE_FastBuild=0;
extern VMINT dark_mix;              // Additional darkness (caves etc)
extern VMINT show_invisible;


extern STRUCTURE objspec[];
extern STRUCTURE tilespec[];
extern STRUCTURE statspec[];
extern STRUCTURE funcspec[];
extern STRUCTURE userspec[];
extern STRUCTURE wieldspec[];
extern STRUCTURE labelspec[];

static void PE_generic(char **a);	//static void PE_fakefunc(char **a);
static void PE_checkaccess(char **);
static void PE_newfunc(char **a);	static void PEP_newfunc(char **a);
static void PE_endfunc(char **a);	static void PEP_endfunc(char **a);
static void PEP_StartLocal(char **a);static void PEP_EndLocal(char **a);
static void PEP_StartTransient(char **a);static void PEP_EndTransient(char **a);
static void PEP_Transient(char **a);
static void PE_classAct(char **a);
static void PE_classPrv(char **a);
static void PE_declare(char **a);	static void PEP_const(char **a);
static void PEP_integer(char **a);	static void PEP_object(char **a);
static void PEP_string(char **a);	static void PEP_tile(char **a);
static void PE_label(char **a);		static void PEP_label(char **a);
static void PEP_intarray(char **a);	static void PEP_objarray(char **a);
static void PEP_strarray(char **a);static void PEP_userstring(char **a);
void PEP_array(char **line, char type);
static void PE_goto(char **a);
static void PE_else(char **line);	static void PEP_else(char **line);
static void PE_endif(char **line);	static void PEP_endif(char **line);
static void PE_if(char **line);		static void PEP_if(char **line);
static void PE_and(char **line);		static void PEP_and(char **line);
static void PE_or(char **line);		static void PEP_or(char **line);
static void PE_if_oics(char **line);
static void PE_if_Oics(char **line);
static void PE_or_oics(char **line);
static void PE_or_Oics(char **line);
static void PE_and_oics(char **line);
static void PE_and_Oics(char **line);
static void PE_for(char **line);		static void PEP_for(char **line);
static void PE_next(char **line);	static void PEP_next(char **line);
static void PE_oChar(char **line);
static void PE_OChar(char **line);
static void PE_cChar(char **line);
static void PE_oSeq(char **line);
static void PE_OSeq(char **line);
static void PE_oSeq2(char **line);
static void PE_OSeq2(char **line);
static void PE_ifcore(char *label);
static void PE_orhelper(char *label, char **line);
static void PE_dword(char **line);
static void PE_while1(char **line);	static void PEP_while(char **line);
static void PE_while2(char **line);
static void PE_do(char **line);		static void PEP_do(char **line);
static void PE_continue(char **line);
static void PE_break(char **line);
static void PE_assert(char **line);
static void PE_setarray(char **line);

static OBJECT obj_template;
static TILE tile_template;
static STATS stats_template;
static FUNCS funcs_template;
static USEDATA usedata_template;
static WIELD wield_template;
static CHAR_LABELS label_template;

VMINT pevm_err=0;
static int funcid=0; // Unique id for functions

static 	char *IRE_VERSION=IKV;

//static WIELD wield_template;

//static SCHEDULE schedule_template;
//static SEQ_POOL form_template;
//static GOAL goal_template;


// Parameter codes:
//
// 0 = 'NULL'
// s = string (any valid string)
// P = string pointer
// n = number
// f = PE function name (string->int)
// i = integer (variable)
// o = object (variable)
// t = tile (variable)
// T = Table (string->int)
// l = label (a keyword the user has created)
// ? = any (matches anything)
// ( = array init
// p = math symbols -,+,=,*,/ etc
// x = complex expression
// > = redirection (In a STRUCT definition)
// E = Not Equal To
//
// I = structure member (integer)
// O = structure member (object)
// S = structure member (string)
//
// a = Array of Integer
// b = Array of String
// c = Array of Object
// U = userstring
//


OPCODE vmspec[] =
                {

                // Mnemonic, Opcode constant (from opcodes.h), Parameter code,
                // Compile function, Pre-Parse function, no of operands

				// READ AND OBEY
				// *OVERLOADED KEYWORDS *MUST* BE CONSECUTIVE IN THE LIST!
				// *'IF' STATEMENTS HAVE ONE EXTRA HIDDEN OPERAND

                    {"ret",PEVM_Return,"",PE_generic,NULL,0},
                    {"return",PEVM_Return,"",PE_generic,NULL,0},
                    {"finish",PEVM_Return,"",PE_generic,NULL,0},
                    {"print",PEVM_Printstr,"s",PE_generic,NULL,1},
                    {"print",PEVM_Printstr,"S",PE_generic,NULL,1},
                    {"print",PEVM_Printstr,"P",PE_generic,NULL,1},
                    {"print",PEVM_Printstr,"b",PE_generic,NULL,1},
                    {"print",PEVM_Printint,"n",PE_generic,NULL,1},
                    {"print",PEVM_Printint,"i",PE_generic,NULL,1},
                    {"print",PEVM_Printint,"I",PE_generic,NULL,1},
                    {"print",PEVM_Printint,"a",PE_generic,NULL,1},
                    {"print",PEVM_PrintCR,"",PE_generic,NULL,0},
                    {"printx",PEVM_PrintXint,"i",PE_generic,NULL,1},
                    {"printx",PEVM_PrintXint,"I",PE_generic,NULL,1},
                    {"printx",PEVM_PrintXint,"a",PE_generic,NULL,1},
                    {"printx",PEVM_PrintXint,"o",PE_generic,NULL,1},
                    {"printx",PEVM_PrintXint,"O",PE_generic,NULL,1},
                    {"printx",PEVM_PrintXint,"t",PE_generic,NULL,1},
                    {"printx",PEVM_PrintXint,"s",PE_generic,NULL,1},
                    {"printx",PEVM_PrintXint,"S",PE_generic,NULL,1},
                    {"printstr",PEVM_Printstr,"s",PE_generic,NULL,1},
                    {"printstr",PEVM_Printstr,"S",PE_generic,NULL,1},
                    {"printstr",PEVM_Printstr,"P",PE_generic,NULL,1},
                    {"printstr",PEVM_Printstr,"b",PE_generic,NULL,1},
                    {"printcr",PEVM_PrintCR,"",PE_generic,NULL,0},
                    {"newline",PEVM_PrintCR,"",PE_generic,NULL,0},
                    {"cls",PEVM_ClearScreen,"",PE_generic,NULL,0},
                    {"clear",PEVM_ClearScreen,"",PE_generic,NULL,0},
                    {"clearline",PEVM_ClearLine,"",PE_generic,NULL,0},
                    {"call",PEVM_Callfunc,"f",PE_generic,NULL,1},
                    {"call",PEVM_Callfunc,"a",PE_generic,NULL,1},
                    {"call",PEVM_Callfunc,"i",PE_generic,NULL,1},
                    {"call",PEVM_Callfunc,"I",PE_generic,NULL,1},
                    {"call",PEVM_CallfuncS,"s",PE_generic,NULL,1},
                    {"call",PEVM_CallfuncS,"S",PE_generic,NULL,1},
                    {"call",PEVM_CallfuncS,"c",PE_generic,NULL,1},
                    {"call",PEVM_CallfuncS,"P",PE_generic,NULL,1},
                    {"function",0,"l",PE_newfunc,PEP_newfunc,0},
                    {"end",0,"",PE_endfunc,PEP_endfunc,0},
                    {"local",0,"",PE_declare,PEP_StartLocal},
                    {"endlocal",0,"",PE_declare,PEP_EndLocal},
                    {"transient",0,"l",PE_declare,PEP_Transient},
                    {"start_transient",0,"",PE_declare,PEP_StartTransient},
                    {"end_transient",0,"",PE_declare,PEP_EndTransient},
                    {"starttransient",0,"",PE_declare,PEP_StartTransient},
                    {"endtransient",0,"",PE_declare,PEP_EndTransient},
                    {"activity",0,"",PE_classAct,NULL,0},
                    {"private",0,"",PE_classPrv,NULL,0},
                    {"const",0,"l=n",PE_declare,PEP_const,0},
                    {"constant",0,"l=n",PE_declare,PEP_const,0},
                    {"integer",0,"l",PE_declare,PEP_integer,0},
                    {"integer",0,"l=n",PE_declare,PEP_integer,0},
                    {"integer_array",0,"a",PE_declare,PEP_intarray,0},
                    {"string",0,"l",PE_declare,PEP_string,0},
                    {"string_array",0,"b",PE_declare,PEP_strarray,0},
                    {"userstring",0,"ln",PE_declare,PEP_userstring,0},
                    {"int",0,"l",PE_declare,PEP_integer,0},
                    {"int",0,"l=n",PE_declare,PEP_integer,0},
                    {"int_array",0,"a",PE_declare,PEP_intarray,0},
                    {"object",0,"o",PE_declare,PEP_object,0},
                    {"object_array",0,"c",PE_declare,PEP_objarray,0},
                    {"tile",0,"t",PE_declare,PEP_tile,0},
                    {"let",PEVM_Let_iei,"o=0",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"O=0",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"i=n",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"I=n",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"i=i",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"i=I",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"I=i",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"I=I",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"a=n",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"a=i",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"a=I",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"a=a",PE_checkaccess,NULL,2},
//                    {"let",PEVM_Let_iei,"i=x",PE_checkaccess,NULL},
                    {"let",PEVM_Let_iei,"o=o",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"o=O",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"o=c",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"O=o",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"O=O",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"O=c",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"c=0",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"c=o",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"c=O",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"c=c",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"P=P",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"P=S",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"P=b",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"S=P",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_pes,"b=s",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"b=S",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"b=P",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"b=b",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_pes,"P=s",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"t=0",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_iei,"t=t",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_seU,"S=U",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_seU,"P=U",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_seU,"b=U",PE_checkaccess,NULL,2},
                    {"let",PEVM_setstr,"U=s",PE_checkaccess,NULL,2},
                    {"let",PEVM_setstr,"U=S",PE_checkaccess,NULL,2},
                    {"let",PEVM_setstr,"U=p",PE_checkaccess,NULL,2},
                    {"let",PEVM_Let_ieipi,"i=npn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=ipn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=Ipn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=apn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=npn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=ipn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=Ipn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=apn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=npn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=ipn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=Ipn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=apn",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=npi",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=ipi",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=Ipi",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=api",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=npi",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=ipi",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=Ipi",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=api",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=npi",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=ipi",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=Ipi",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=api",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=npI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=ipI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=IpI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=apI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=npI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=ipI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=IpI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=apI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=npI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=ipI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=IpI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=apI",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=npa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=ipa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=Ipa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"i=apa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=npa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=ipa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=Ipa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"I=apa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=npa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=ipa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=Ipa",PE_checkaccess,NULL,4},
                    {"let",PEVM_Let_ieipi,"a=apa",PE_checkaccess,NULL,4},
                    {"add",PEVM_Add,"ipn",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"ipi",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"ipI",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"ipa",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"Ipn",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"Ipi",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"IpI",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"Ipa",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"apn",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"api",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"apI",PE_checkaccess,NULL,3},
                    {"add",PEVM_Add,"apa",PE_checkaccess,NULL,3},
                    {"p_let",PEVM_Let_iei,"i=o",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"I=o",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"i=O",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"I=O",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"i=c",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"I=c",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"o=i",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"o=I",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"O=i",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"O=I",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"c=i",PE_checkaccess,NULL,2},
                    {"p_let",PEVM_Let_iei,"c=I",PE_checkaccess,NULL,2},
                    {"clear_array",PEVM_ClearArrayI,"a",PE_checkaccess,NULL,1},
                    {"clear_array",PEVM_ClearArrayS,"b",PE_checkaccess,NULL,1},
                    {"clear_array",PEVM_ClearArrayO,"c",PE_checkaccess,NULL,1},
                    {"cleararray",PEVM_ClearArrayI,"a",PE_checkaccess,NULL,1},
                    {"cleararray",PEVM_ClearArrayS,"b",PE_checkaccess,NULL,1},
                    {"cleararray",PEVM_ClearArrayO,"c",PE_checkaccess,NULL,1},
                    {"copy_array",PEVM_CopyArrayI,"a=a",PE_checkaccess,NULL,2},
                    {"copy_array",PEVM_CopyArrayS,"b=b",PE_checkaccess,NULL,2},
                    {"copy_array",PEVM_CopyArrayO,"c=c",PE_checkaccess,NULL,2},
                    {"copyarray",PEVM_CopyArrayI,"a=a",PE_checkaccess,NULL,2},
                    {"copyarray",PEVM_CopyArrayS,"b=a",PE_checkaccess,NULL,2},
                    {"copyarray",PEVM_CopyArrayO,"c=a",PE_checkaccess,NULL,2},
                    {"set_array",PEVM_SetArrayI,"a=(",PE_setarray,NULL,1},
                    {"set_array",PEVM_SetArrayS,"b=(",PE_setarray,NULL,1},
//                    {"set_array",PEVM_SetArrayO,"c=(",PE_setarray,NULL,1},
                    {"setarray",PEVM_SetArrayI,"a=(",PE_setarray,NULL,1},
                    {"setarray",PEVM_SetArrayS,"b=(",PE_setarray,NULL,1},
//                    {"setarray",PEVM_SetArrayO,"c=(",PE_checkaccess,NULL,1},
                    {"label",0,"l",PE_label,PEP_label,0},
                    {"goto",PEVM_Goto,"l",PE_goto,NULL,1},
                    {"if",PEVM_If_i,"i",PE_if,PEP_if,2},
                    {"if",PEVM_If_ni,"i",PE_if,PEP_if,2},
                    {"if",PEVM_If_i,"I",PE_if,PEP_if,2},
                    {"if",PEVM_If_ni,"I",PE_if,PEP_if,2},
                    {"if",PEVM_If_i,"a",PE_if,PEP_if,2},
                    {"if",PEVM_If_ni,"a",PE_if,PEP_if,2},

                    {"if",PEVM_If_iei,"npn",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"npi",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"npI",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"npa",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"ipn",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"ipi",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"ipI",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"ipa",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"Ipn",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"Ipi",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"IpI",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"Ipa",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"apn",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"api",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"apI",PE_if,PEP_if,4},
                    {"if",PEVM_If_iei,"apa",PE_if,PEP_if,4},
                    {"if",PEVM_If_oeo,"oeo",PE_if,PEP_if,3},
                    {"if",PEVM_If_oeo,"oeO",PE_if,PEP_if,3},
                    {"if",PEVM_If_oeo,"oec",PE_if,PEP_if,3},
                    {"if",PEVM_If_oeo,"Oeo",PE_if,PEP_if,3},
                    {"if",PEVM_If_oeo,"OeO",PE_if,PEP_if,3},
                    {"if",PEVM_If_oeo,"Oec",PE_if,PEP_if,3},
                    {"if",PEVM_If_oeo,"ceo",PE_if,PEP_if,3},
                    {"if",PEVM_If_oeo,"ceO",PE_if,PEP_if,3},
                    {"if",PEVM_If_oeo,"cec",PE_if,PEP_if,3},
                    {"if",PEVM_If_oEo,"oEo",PE_if,PEP_if,3},
                    {"if",PEVM_If_oEo,"oEO",PE_if,PEP_if,3},
                    {"if",PEVM_If_oEo,"oEc",PE_if,PEP_if,3},
                    {"if",PEVM_If_oEo,"OEo",PE_if,PEP_if,3},
                    {"if",PEVM_If_oEo,"OEO",PE_if,PEP_if,3},
                    {"if",PEVM_If_oEo,"OEc",PE_if,PEP_if,3},
                    {"if",PEVM_If_oEo,"cEo",PE_if,PEP_if,3},
                    {"if",PEVM_If_oEo,"cEO",PE_if,PEP_if,3},
                    {"if",PEVM_If_oEo,"cEc",PE_if,PEP_if,3},
                    {"if",PEVM_If_oics,"o??s",PE_if_oics,PEP_if,3},
                    {"if",PEVM_If_oics,"O??s",PE_if_Oics,PEP_if,3},
                    {"if",PEVM_If_oics,"c??s",PE_if_Oics,PEP_if,3},
                    {"if",PEVM_If_oics,"o??S",PE_if_oics,PEP_if,3},
                    {"if",PEVM_If_oics,"O??S",PE_if_Oics,PEP_if,3},
                    {"if",PEVM_If_oics,"c??S",PE_if_Oics,PEP_if,3},
                    {"if",PEVM_If_oics,"o??P",PE_if_oics,PEP_if,3},
                    {"if",PEVM_If_oics,"O??P",PE_if_Oics,PEP_if,3},
                    {"if",PEVM_If_oics,"c??P",PE_if_Oics,PEP_if,3},
                    {"if",PEVM_If_oics,"o??b",PE_if_oics,PEP_if,3},
                    {"if",PEVM_If_oics,"O??b",PE_if_Oics,PEP_if,3},
                    {"if",PEVM_If_oics,"c??b",PE_if_Oics,PEP_if,3},
                    {"if",PEVM_If_ses,"sps",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"spS",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"spP",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"spb",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"Sps",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"SpS",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"SpP",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"Spb",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"Pps",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"PpS",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"PpP",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"Ppb",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"bps",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"bpS",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"bpP",PE_if,PEP_if,4},
                    {"if",PEVM_If_ses,"bpb",PE_if,PEP_if,4},
                    {"and",PEVM_If_i,"i",PE_and,PEP_and,2},
                    {"and",PEVM_If_ni,"i",PE_and,PEP_and,2},
                    {"and",PEVM_If_i,"I",PE_and,PEP_and,2},
                    {"and",PEVM_If_ni,"I",PE_and,PEP_and,2},
                    {"and",PEVM_If_i,"a",PE_and,PEP_and,2},
                    {"and",PEVM_If_ni,"a",PE_and,PEP_and,2},
                    {"and",PEVM_If_iei,"npn",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"npi",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"npI",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"npa",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"ipn",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"ipi",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"ipI",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"ipa",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"Ipn",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"Ipi",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"IpI",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"Ipa",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"apn",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"api",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"apI",PE_and,PEP_and,4},
                    {"and",PEVM_If_iei,"apa",PE_and,PEP_and,4},
                    {"and",PEVM_If_oeo,"oeo",PE_and,PEP_and,3},
                    {"and",PEVM_If_oeo,"oeO",PE_and,PEP_and,3},
                    {"and",PEVM_If_oeo,"oec",PE_and,PEP_and,3},
                    {"and",PEVM_If_oeo,"Oeo",PE_and,PEP_and,3},
                    {"and",PEVM_If_oeo,"OeO",PE_and,PEP_and,3},
                    {"and",PEVM_If_oeo,"Oec",PE_and,PEP_and,3},
                    {"and",PEVM_If_oeo,"ceo",PE_and,PEP_and,3},
                    {"and",PEVM_If_oeo,"ceO",PE_and,PEP_and,3},
                    {"and",PEVM_If_oeo,"cec",PE_and,PEP_and,3},
                    {"and",PEVM_If_oEo,"oEo",PE_and,PEP_and,3},
                    {"and",PEVM_If_oEo,"oEO",PE_and,PEP_and,3},
                    {"and",PEVM_If_oEo,"oEc",PE_and,PEP_and,3},
                    {"and",PEVM_If_oEo,"OEo",PE_and,PEP_and,3},
                    {"and",PEVM_If_oEo,"OEO",PE_and,PEP_and,3},
                    {"and",PEVM_If_oEo,"OEc",PE_and,PEP_and,3},
                    {"and",PEVM_If_oEo,"cEo",PE_and,PEP_and,3},
                    {"and",PEVM_If_oEo,"cEO",PE_and,PEP_and,3},
                    {"and",PEVM_If_oEo,"cEc",PE_and,PEP_and,3},
                    {"and",PEVM_If_ses,"sps",PE_and,PEP_and,4},
                    {"and",PEVM_If_ses,"spS",PE_and,PEP_and,4},
                    {"and",PEVM_If_ses,"spP",PE_and,PEP_and,4},
                    {"and",PEVM_If_ses,"Sps",PE_and,PEP_and,4},
                    {"and",PEVM_If_ses,"SpS",PE_and,PEP_and,4},
                    {"and",PEVM_If_ses,"SpP",PE_and,PEP_and,4},
                    {"and",PEVM_If_ses,"Pps",PE_and,PEP_and,4},
                    {"and",PEVM_If_ses,"PpS",PE_and,PEP_and,4},
                    {"and",PEVM_If_ses,"PpP",PE_and,PEP_and,4},
                    {"and",PEVM_If_oics,"o??s",PE_and_oics,PEP_and,3},
                    {"and",PEVM_If_oics,"O??s",PE_and_Oics,PEP_and,3},
                    {"and",PEVM_If_oics,"c??s",PE_and_Oics,PEP_and,3},
                    {"or",PEVM_If_i,"i",PE_or,PEP_or,2},
                    {"or",PEVM_If_ni,"i",PE_or,PEP_or,2},
                    {"or",PEVM_If_i,"I",PE_or,PEP_or,2},
                    {"or",PEVM_If_ni,"I",PE_or,PEP_or,2},
                    {"or",PEVM_If_i,"a",PE_or,PEP_or,2},
                    {"or",PEVM_If_ni,"a",PE_or,PEP_or,2},

                    {"or",PEVM_If_iei,"npn",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"npi",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"npI",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"npa",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"ipn",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"ipi",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"ipI",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"ipa",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"Ipn",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"Ipi",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"IpI",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"Ipa",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"apn",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"api",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"apI",PE_or,PEP_or,4},
                    {"or",PEVM_If_iei,"apa",PE_or,PEP_or,4},
                    {"or",PEVM_If_oeo,"oeo",PE_or,PEP_or,3},
                    {"or",PEVM_If_oeo,"oeO",PE_or,PEP_or,3},
                    {"or",PEVM_If_oeo,"oec",PE_or,PEP_or,3},
                    {"or",PEVM_If_oeo,"Oeo",PE_or,PEP_or,3},
                    {"or",PEVM_If_oeo,"OeO",PE_or,PEP_or,3},
                    {"or",PEVM_If_oeo,"Oec",PE_or,PEP_or,3},
                    {"or",PEVM_If_oeo,"ceo",PE_or,PEP_or,3},
                    {"or",PEVM_If_oeo,"ceO",PE_or,PEP_or,3},
                    {"or",PEVM_If_oeo,"cec",PE_or,PEP_or,3},
                    {"or",PEVM_If_oEo,"oEo",PE_or,PEP_or,3},
                    {"or",PEVM_If_oEo,"oEO",PE_or,PEP_or,3},
                    {"or",PEVM_If_oEo,"oEc",PE_or,PEP_or,3},
                    {"or",PEVM_If_oEo,"OEo",PE_or,PEP_or,3},
                    {"or",PEVM_If_oEo,"OEO",PE_or,PEP_or,3},
                    {"or",PEVM_If_oEo,"OEc",PE_or,PEP_or,3},
                    {"or",PEVM_If_oEo,"cEo",PE_or,PEP_or,3},
                    {"or",PEVM_If_oEo,"cEO",PE_or,PEP_or,3},
                    {"or",PEVM_If_oEo,"cEc",PE_or,PEP_or,3},
                    {"or",PEVM_If_ses,"sps",PE_or,PEP_or,4},
                    {"or",PEVM_If_ses,"spS",PE_or,PEP_or,4},
                    {"or",PEVM_If_ses,"spP",PE_or,PEP_or,4},
                    {"or",PEVM_If_ses,"Sps",PE_or,PEP_or,4},
                    {"or",PEVM_If_ses,"SpS",PE_or,PEP_or,4},
                    {"or",PEVM_If_ses,"SpP",PE_or,PEP_or,4},
                    {"or",PEVM_If_ses,"Pps",PE_or,PEP_or,4},
                    {"or",PEVM_If_ses,"PpS",PE_or,PEP_or,4},
                    {"or",PEVM_If_ses,"PpP",PE_or,PEP_or,4},
                    {"or",PEVM_If_oics,"o??s",PE_or_oics,PEP_or,3},
                    {"or",PEVM_If_oics,"O??s",PE_or_Oics,PEP_or,3},
                    {"or",PEVM_If_oics,"c??s",PE_or_Oics,PEP_or,3},
                    {"if_exist",PEVM_If_o,"o",PE_if,PEP_if,2},
                    {"if_exist",PEVM_If_o,"O",PE_if,PEP_if,2},
                    {"if_exist",PEVM_If_p,"t",PE_if,PEP_if,2},
                    {"if_exist",PEVM_If_o,"c",PE_if,PEP_if,2},
                    {"if_exist",PEVM_If_p,"P",PE_if,PEP_if,2},
                    {"if_exists",PEVM_If_o,"o",PE_if,PEP_if,2},
                    {"if_exists",PEVM_If_o,"O",PE_if,PEP_if,2},
                    {"if_exists",PEVM_If_p,"t",PE_if,PEP_if,2},
                    {"if_exists",PEVM_If_o,"c",PE_if,PEP_if,2},
                    {"if_exists",PEVM_If_p,"P",PE_if,PEP_if,2},
                    {"if_not_exist",PEVM_If_no,"o",PE_if,PEP_if,2},
                    {"if_not_exist",PEVM_If_no,"O",PE_if,PEP_if,2},
                    {"if_not_exist",PEVM_If_np,"t",PE_if,PEP_if,2},
                    {"if_not_exist",PEVM_If_no,"c",PE_if,PEP_if,2},
                    {"if_not_exist",PEVM_If_np,"P",PE_if,PEP_if,2},
                    {"if_not_exists",PEVM_If_no,"o",PE_if,PEP_if,2},
                    {"if_not_exists",PEVM_If_no,"O",PE_if,PEP_if,2},
                    {"if_not_exists",PEVM_If_np,"t",PE_if,PEP_if,2},
                    {"if_not_exists",PEVM_If_no,"c",PE_if,PEP_if,2},
                    {"if_not_exists",PEVM_If_np,"P",PE_if,PEP_if,2},
                    {"else",0,"",PE_else,PEP_else,0},
                    {"endif",0,"",PE_endif,PEP_endif,0},
                    {"for",PEVM_For,"i=n?n",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=n?i",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=n?I",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=n?a",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=i?n",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=i?i",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=i?I",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=i?a",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=I?n",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=I?i",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=I?I",PE_for,PEP_for,5},
                    {"for",PEVM_For,"i=I?a",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=n?n",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=n?i",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=n?I",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=n?a",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=i?n",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=i?i",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=i?I",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=i?a",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=I?n",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=I?i",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=I?I",PE_for,PEP_for,5},
                    {"for",PEVM_For,"I=I?a",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=n?n",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=n?i",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=n?I",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=n?a",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=i?n",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=i?i",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=i?I",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=i?a",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=I?n",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=I?i",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=I?I",PE_for,PEP_for,5},
                    {"for",PEVM_For,"a=I?a",PE_for,PEP_for,5},
                    {"next",0,"",PE_next,PEP_next,0},
                    {"next",0,"?",PE_next,PEP_next,0},
                    {"do",0,"",PE_do,PEP_do,0},
                    {"while",PEVM_While2,"npn",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"ipn",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"ipi",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"ipI",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"Ipn",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"Ipi",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"IpI",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"opo",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"opO",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"opc",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"op0",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"Opo",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"OpO",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"Opc",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"Op0",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"cpo",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"cpO",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"cpc",PE_while2,PEP_while,4},
                    {"while",PEVM_While2,"cp0",PE_while2,PEP_while,4},
                    {"while",PEVM_While1,"n",PE_while1,PEP_while,2},
                    {"while",PEVM_While1,"i",PE_while1,PEP_while,2},
                    {"while",PEVM_While1,"I",PE_while1,PEP_while,2},
                    {"while",PEVM_While1,"a",PE_while1,PEP_while,2},
                    {"while",PEVM_While1,"o",PE_while1,PEP_while,2},
                    {"while",PEVM_While1,"O",PE_while1,PEP_while,2},
                    {"while",PEVM_While1,"c",PE_while1,PEP_while,2},
                    {"continue",PEVM_Goto,"",PE_continue,NULL,1},
                    {"break",PEVM_Break,"",PE_break,NULL,1},
                    {"search_inside",PEVM_Searchin,"o?f",PE_generic,NULL,2},
                    {"search_inside",PEVM_Searchin,"O?f",PE_generic,NULL,2},
                    {"search_inside",PEVM_Searchin,"c?f",PE_generic,NULL,2},
                    {"list_after",PEVM_Listafter,"o?f",PE_generic,NULL,2},
                    {"list_after",PEVM_Listafter,"O?f",PE_generic,NULL,2},
                    {"list_after",PEVM_Listafter,"c?f",PE_generic,NULL,2},
                    {"search_world",PEVM_SearchWorld,"f",PE_generic,NULL,1},
                    {"search_all",PEVM_SearchWorld,"f",PE_generic,NULL,1},
                    {"create",PEVM_Create,"o=s",PE_oChar,NULL,2},
                    {"create",PEVM_Create,"O=s",PE_OChar,NULL,2},
                    {"create",PEVM_Create,"c=s",PE_cChar,NULL,2},
                    {"create",PEVM_CreateS,"o=S",PE_generic,NULL,2},
                    {"create",PEVM_CreateS,"O=S",PE_generic,NULL,2},
                    {"create",PEVM_CreateS,"c=S",PE_generic,NULL,2},
                    {"create",PEVM_CreateS,"o=P",PE_generic,NULL,2},
                    {"create",PEVM_CreateS,"O=P",PE_generic,NULL,2},
                    {"create",PEVM_CreateS,"c=P",PE_generic,NULL,2},
                    {"destroy",PEVM_Destroy,"o",PE_generic,NULL,1},
                    {"destroy",PEVM_Destroy,"O",PE_generic,NULL,1},
                    {"destroy",PEVM_Destroy,"c",PE_generic,NULL,1},
                    {"set_darkness",PEVM_SetDarkness,"n",PE_generic,NULL,1},
                    {"set_darkness",PEVM_SetDarkness,"i",PE_generic,NULL,1},
                    {"set_darkness",PEVM_SetDarkness,"I",PE_generic,NULL,1},
                    {"play_music",PEVM_PlayMusic,"s",PE_generic,NULL,1},
                    {"stop_music",PEVM_StopMusic,"",PE_generic,NULL,0},
                    {"is_playing",PEVM_IsPlaying,"i",PE_generic,NULL,1},
                    {"is_playing",PEVM_IsPlaying,"I",PE_generic,NULL,1},
                    {"play_sound",PEVM_PlaySound,"s",PE_generic,NULL,1},
                    {"object_sound",PEVM_ObjSound,"so",PE_generic,NULL,2},
                    {"object_sound",PEVM_ObjSound,"sO",PE_generic,NULL,2},
                    {"object_sound",PEVM_ObjSound,"sc",PE_generic,NULL,2},
                    {"object_sound",PEVM_ObjSound,"So",PE_generic,NULL,2},
                    {"object_sound",PEVM_ObjSound,"SO",PE_generic,NULL,2},
                    {"object_sound",PEVM_ObjSound,"Sc",PE_generic,NULL,2},
                    {"object_sound",PEVM_ObjSound,"Po",PE_generic,NULL,2},
                    {"object_sound",PEVM_ObjSound,"PO",PE_generic,NULL,2},
                    {"object_sound",PEVM_ObjSound,"Pc",PE_generic,NULL,2},
                    {"set_flag",PEVM_SetFlag,"on=n",PE_generic,NULL,3},
                    {"set_flag",PEVM_SetFlag,"on=i",PE_generic,NULL,3},
                    {"set_flag",PEVM_SetFlag,"on=I",PE_generic,NULL,3},
                    {"set_flag",PEVM_SetFlag,"On=n",PE_generic,NULL,3},
                    {"set_flag",PEVM_SetFlag,"On=i",PE_generic,NULL,3},
                    {"set_flag",PEVM_SetFlag,"On=I",PE_generic,NULL,3},
                    {"set_flag",PEVM_SetFlag,"cn=n",PE_generic,NULL,3},
                    {"set_flag",PEVM_SetFlag,"cn=i",PE_generic,NULL,3},
                    {"set_flag",PEVM_SetFlag,"cn=I",PE_generic,NULL,3},
                    {"get_flag",PEVM_GetFlag,"i=on",PE_checkaccess,NULL,3},
                    {"get_flag",PEVM_GetFlag,"I=on",PE_checkaccess,NULL,3},
                    {"get_flag",PEVM_GetFlag,"i=On",PE_checkaccess,NULL,3},
                    {"get_flag",PEVM_GetFlag,"I=On",PE_checkaccess,NULL,3},
                    {"get_flag",PEVM_GetFlag,"i=cn",PE_checkaccess,NULL,3},
                    {"get_flag",PEVM_GetFlag,"I=cn",PE_checkaccess,NULL,3},
                    {"reset_flag",PEVM_ResetFlag,"on",PE_generic,NULL,2},
                    {"reset_flag",PEVM_ResetFlag,"On",PE_generic,NULL,2},
                    {"reset_flag",PEVM_ResetFlag,"cn",PE_generic,NULL,2},
                    {"if_flag",PEVM_If_on,"on",PE_if,PEP_if,3},
                    {"if_flag",PEVM_If_on,"On",PE_if,PEP_if,3},
                    {"if_flag",PEVM_If_on,"cn",PE_if,PEP_if,3},
                    {"if_flag",PEVM_If_tn,"tn",PE_if,PEP_if,3},
                    {"if_not_flag",PEVM_If_non,"on",PE_if,PEP_if,3},
                    {"if_not_flag",PEVM_If_non,"On",PE_if,PEP_if,3},
                    {"if_not_flag",PEVM_If_non,"cn",PE_if,PEP_if,3},
                    {"if_not_flag",PEVM_If_ntn,"tn",PE_if,PEP_if,3},
                    {"get_engineflag",PEVM_GetEngineFlag,"i=on",PE_checkaccess,NULL,3},
                    {"move_object",PEVM_MoveObject,"o?nn",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"o?ni",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"o?nI",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"o?in",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"o?In",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"o?ii",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"o?iI",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"o?Ii",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"o?II",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"O?nn",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"O?ni",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"O?nI",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"O?in",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"O?In",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"O?ii",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"O?iI",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"O?Ii",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"O?II",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"c?nn",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"c?ni",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"c?nI",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"c?in",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"c?In",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"c?ii",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"c?iI",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"c?Ii",PE_generic,NULL,3},
                    {"move_object",PEVM_MoveObject,"c?II",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"o?nn",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"o?ni",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"o?nI",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"o?in",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"o?In",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"o?ii",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"o?iI",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"o?Ii",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"o?II",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"O?nn",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"O?ni",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"O?nI",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"O?in",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"O?In",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"O?ii",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"O?iI",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"O?Ii",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"O?II",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"c?nn",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"c?ni",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"c?nI",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"c?in",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"c?In",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"c?ii",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"c?iI",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"c?Ii",PE_generic,NULL,3},
                    {"push_object",PEVM_PushObject,"c?II",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"o?nn",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"o?ni",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"o?nI",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"o?in",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"o?In",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"o?ii",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"o?iI",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"o?Ii",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"o?II",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"O?nn",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"O?ni",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"O?nI",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"O?in",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"O?In",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"O?ii",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"O?iI",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"O?Ii",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"O?II",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"c?nn",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"c?ni",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"c?nI",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"c?in",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"c?In",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"c?ii",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"c?iI",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"c?Ii",PE_generic,NULL,3},
                    {"transfer_object",PEVM_XferObject,"c?II",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"o?nn",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"o?ni",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"o?nI",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"o?in",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"o?In",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"o?ii",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"o?iI",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"o?Ii",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"o?II",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"O?nn",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"O?ni",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"O?nI",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"O?in",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"O?In",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"O?ii",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"O?iI",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"O?Ii",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"O?II",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"c?nn",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"c?ni",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"c?nI",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"c?in",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"c?In",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"c?ii",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"c?iI",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"c?Ii",PE_generic,NULL,3},
                    {"draw_object",PEVM_ShowObject,"c?II",PE_generic,NULL,3},
                    {"gotoxy",PEVM_GotoXY,"nn",PE_generic,NULL,2},
                    {"gotoxy",PEVM_GotoXY,"ni",PE_generic,NULL,2},
                    {"gotoxy",PEVM_GotoXY,"nI",PE_generic,NULL,2},
                    {"gotoxy",PEVM_GotoXY,"in",PE_generic,NULL,2},
                    {"gotoxy",PEVM_GotoXY,"In",PE_generic,NULL,2},
                    {"gotoxy",PEVM_GotoXY,"ii",PE_generic,NULL,2},
                    {"gotoxy",PEVM_GotoXY,"iI",PE_generic,NULL,2},
                    {"gotoxy",PEVM_GotoXY,"Ii",PE_generic,NULL,2},
                    {"gotoxy",PEVM_GotoXY,"II",PE_generic,NULL,2},
                    {"printxy",PEVM_PrintXYs,"s",PE_generic,NULL,1},
                    {"printxy",PEVM_PrintXYs,"S",PE_generic,NULL,1},
                    {"printxy",PEVM_PrintXYs,"P",PE_generic,NULL,1},
                    {"printxy",PEVM_PrintXYs,"b",PE_generic,NULL,1},
                    {"printxy",PEVM_PrintXYi,"n",PE_generic,NULL,1},
                    {"printxy",PEVM_PrintXYi,"i",PE_generic,NULL,1},
                    {"printxy",PEVM_PrintXYi,"I",PE_generic,NULL,1},
                    {"printxy",PEVM_PrintXYi,"a",PE_generic,NULL,1},
                    {"printxy",PEVM_PrintXYcr,"",PE_generic,NULL,0},
                    {"printxycr",PEVM_PrintXYcr,"",PE_generic,NULL,0},
                    {"printxyx",PEVM_PrintXYx,"n",PE_generic,NULL,1},
                    {"printxyx",PEVM_PrintXYx,"i",PE_generic,NULL,1},
                    {"printxyx",PEVM_PrintXYx,"I",PE_generic,NULL,1},
                    {"textcolour",PEVM_TextColour,"nnn",PE_generic,NULL,3},
                    {"textcolour",PEVM_TextColour,"iii",PE_generic,NULL,3},
                    {"textcolour",PEVM_TextColourDefault,"",PE_generic,NULL,0},
                    {"textcolor",PEVM_TextColour,"nnn",PE_generic,NULL,3},
                    {"textcolor",PEVM_TextColour,"iii",PE_generic,NULL,3},
                    {"textcolor",PEVM_TextColourDefault,"",PE_generic,NULL,0},
                    {"text_colour",PEVM_TextColour,"nnn",PE_generic,NULL,3},
                    {"text_colour",PEVM_TextColour,"iii",PE_generic,NULL,3},
                    {"text_colour",PEVM_TextColourDefault,"",PE_generic,NULL,0},
                    {"text_color",PEVM_TextColour,"nnn",PE_generic,NULL,3},
                    {"text_color",PEVM_TextColour,"iii",PE_generic,NULL,3},
                    {"text_color",PEVM_TextColourDefault,"",PE_generic,NULL,0},
                    {"gettextcolour",PEVM_GetTextColour,"iii",PE_generic,NULL,3},
                    {"gettextcolor",PEVM_GetTextColour,"iii",PE_generic,NULL,3},
                    {"get_text_colour",PEVM_GetTextColour,"iii",PE_generic,NULL,3},
                    {"get_text_color",PEVM_GetTextColour,"iii",PE_generic,NULL,3},
                    {"textfont",PEVM_TextFont,"n",PE_generic,NULL,1},
                    {"textfont",PEVM_TextFont,"i",PE_generic,NULL,1},
                    {"text_font",PEVM_TextFont,"n",PE_generic,NULL,1},
                    {"text_font",PEVM_TextFont,"i",PE_generic,NULL,1},
                    {"gettextfont",PEVM_GetTextFont,"i",PE_generic,NULL,1},
                    {"get_text_font",PEVM_GetTextFont,"i",PE_generic,NULL,1},
                    {"set_sequence",PEVM_SetSequenceN,"os",PE_oSeq,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"oi",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"oI",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"oS",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"oP",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"Os",PE_OSeq,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"Oi",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"OI",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"OS",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"OP",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"cs",PE_OSeq,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"ci",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"cI",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"cS",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"cP",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"o=s",PE_oSeq2,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"o=i",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"o=I",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"o=S",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"o=P",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"O=s",PE_OSeq2,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"O=i",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"O=I",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"O=S",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"O=P",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"c=s",PE_OSeq2,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"c=i",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceN,"c=I",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"c=S",PE_generic,NULL,2},
                    {"set_sequence",PEVM_SetSequenceS,"c=P",PE_generic,NULL,2},
                    {"lightning",PEVM_Lightning,"n",PE_dword,NULL,1},
                    {"lightning",PEVM_Lightning,"i",PE_dword,NULL,1},
                    {"lightning",PEVM_Lightning,"I",PE_dword,NULL,1},
                    {"lightning",PEVM_Lightning,"a",PE_dword,NULL,1},
                    {"earthquake",PEVM_Earthquake,"n",PE_dword,NULL,1},
                    {"earthquake",PEVM_Earthquake,"i",PE_dword,NULL,1},
                    {"earthquake",PEVM_Earthquake,"I",PE_dword,NULL,1},
                    {"earthquake",PEVM_Earthquake,"a",PE_dword,NULL,1},
                    {"print_bestname",PEVM_Bestname,"o",PE_generic,NULL,1},
                    {"print_bestname",PEVM_Bestname,"O",PE_generic,NULL,1},
                    {"print_bestname",PEVM_Bestname,"c",PE_generic,NULL,1},
                    {"print_best_name",PEVM_Bestname,"o",PE_generic,NULL,1},
                    {"print_best_name",PEVM_Bestname,"O",PE_generic,NULL,1},
                    {"print_best_name",PEVM_Bestname,"c",PE_generic,NULL,1},
                    {"get_object",PEVM_GetObject,"o?nn",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"o?ni",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"o?in",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"o?ii",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"o?iI",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"o?Ii",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"o?II",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"o?nI",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"o?In",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"O?nn",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"O?ni",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"O?in",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"O?ii",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"O?iI",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"O?Ii",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"O?II",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"O?nI",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"O?In",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"c?nn",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"c?ni",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"c?in",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"c?ii",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"c?iI",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"c?Ii",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"c?II",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"c?nI",PE_checkaccess,NULL,3},
                    {"get_object",PEVM_GetObject,"c?In",PE_checkaccess,NULL,3},
                    {"get_tile",PEVM_GetTile,"t?nn",PE_generic,NULL,3},
                    {"get_tile",PEVM_GetTile,"t?ni",PE_generic,NULL,3},
                    {"get_tile",PEVM_GetTile,"t?in",PE_generic,NULL,3},
                    {"get_tile",PEVM_GetTile,"t?ii",PE_generic,NULL,3},
                    {"get_tile",PEVM_GetTile,"t?iI",PE_generic,NULL,3},
                    {"get_tile",PEVM_GetTile,"t?Ii",PE_generic,NULL,3},
                    {"get_tile",PEVM_GetTile,"t?II",PE_generic,NULL,3},
                    {"get_tile",PEVM_GetTile,"t?nI",PE_generic,NULL,3},
                    {"get_tile",PEVM_GetTile,"t?In",PE_generic,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"i?nn",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"i?ni",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"i?in",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"i?ii",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"i?iI",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"i?Ii",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"i?II",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"i?nI",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"i?In",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"I?nn",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"I?ni",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"I?in",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"I?ii",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"I?iI",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"I?Ii",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"I?II",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"I?nI",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"I?In",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"a?nn",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"a?ni",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"a?in",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"a?ii",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"a?iI",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"a?Ii",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"a?II",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"a?nI",PE_checkaccess,NULL,3},
                    {"get_cost",PEVM_GetTileCost,"a?In",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"o?nn",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"o?ni",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"o?in",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"o?ii",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"o?iI",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"o?Ii",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"o?II",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"o?nI",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"o?In",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"O?nn",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"O?ni",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"O?in",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"O?ii",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"O?iI",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"O?Ii",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"O?II",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"O?nI",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"O?In",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"c?nn",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"c?ni",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"c?in",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"c?ii",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"c?iI",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"c?Ii",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"c?II",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"c?nI",PE_checkaccess,NULL,3},
                    {"get_solid_object",PEVM_GetSolidObject,"c?In",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"o?nn",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"o?ni",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"o?in",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"o?ii",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"o?iI",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"o?Ii",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"o?II",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"o?nI",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"o?In",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"O?nn",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"O?ni",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"O?in",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"O?ii",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"O?iI",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"O?Ii",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"O?II",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"O?nI",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"O?In",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"c?nn",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"c?ni",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"c?in",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"c?ii",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"c?iI",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"c?Ii",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"c?II",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"c?nI",PE_checkaccess,NULL,3},
                    {"get_first_object",PEVM_GetFirstObject,"c?In",PE_checkaccess,NULL,3},
                    {"get_object_below",PEVM_GetObjectBelow,"o=o",PE_checkaccess,NULL,2},
                    {"get_object_below",PEVM_GetObjectBelow,"o=O",PE_checkaccess,NULL,2},
                    {"get_object_below",PEVM_GetObjectBelow,"o=c",PE_checkaccess,NULL,2},
                    {"get_object_below",PEVM_GetObjectBelow,"O=o",PE_checkaccess,NULL,2},
                    {"get_object_below",PEVM_GetObjectBelow,"O=O",PE_checkaccess,NULL,2},
                    {"get_object_below",PEVM_GetObjectBelow,"O=c",PE_checkaccess,NULL,2},
                    {"get_object_below",PEVM_GetObjectBelow,"c=o",PE_checkaccess,NULL,2},
                    {"get_object_below",PEVM_GetObjectBelow,"c=O",PE_checkaccess,NULL,2},
                    {"get_object_below",PEVM_GetObjectBelow,"c=c",PE_checkaccess,NULL,2},
                    {"get_bridge",PEVM_GetBridge,"o?nn",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"o?ni",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"o?in",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"o?ii",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"o?iI",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"o?Ii",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"o?II",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"o?nI",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"o?In",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"O?nn",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"O?ni",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"O?in",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"O?ii",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"O?iI",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"O?Ii",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"O?II",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"O?nI",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"O?In",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"c?nn",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"c?ni",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"c?in",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"c?ii",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"c?iI",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"c?Ii",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"c?II",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"c?nI",PE_checkaccess,NULL,3},
                    {"get_bridge",PEVM_GetBridge,"c?In",PE_checkaccess,NULL,3},

                    {"change",PEVM_ChangeObject,"o=s",PE_oChar,NULL,2},
                    {"change",PEVM_ChangeObject,"O=s",PE_OChar,NULL,2},
                    {"change",PEVM_ChangeObject,"c=s",PE_cChar,NULL,2},
                    {"change",PEVM_ChangeObjectS,"o=S",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObjectS,"O=S",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObjectS,"c=S",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObjectS,"o=P",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObjectS,"O=P",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObjectS,"c=P",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObject,"o=i",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObject,"O=i",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObject,"c=i",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObject,"o=I",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObject,"O=I",PE_generic,NULL,2},
                    {"change",PEVM_ChangeObject,"c=I",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObject,"o=s",PE_oChar,NULL,2},
                    {"replace",PEVM_ReplaceObject,"O=s",PE_OChar,NULL,2},
                    {"replace",PEVM_ReplaceObject,"c=s",PE_cChar,NULL,2},
                    {"replace",PEVM_ReplaceObjectS,"o=S",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObjectS,"O=S",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObjectS,"c=S",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObjectS,"o=P",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObjectS,"O=P",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObjectS,"c=P",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObject,"o=i",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObject,"O=i",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObject,"c=i",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObject,"o=I",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObject,"O=I",PE_generic,NULL,2},
                    {"replace",PEVM_ReplaceObject,"c=I",PE_generic,NULL,2},
                    {"set_direction",PEVM_SetDir,"o?n",PE_generic,NULL,2},
                    {"set_direction",PEVM_SetDir,"o?i",PE_generic,NULL,2},
                    {"set_direction",PEVM_SetDir,"o?I",PE_generic,NULL,2},
                    {"set_direction",PEVM_SetDir,"O?n",PE_generic,NULL,2},
                    {"set_direction",PEVM_SetDir,"O?i",PE_generic,NULL,2},
                    {"set_direction",PEVM_SetDir,"O?I",PE_generic,NULL,2},
                    {"set_direction",PEVM_SetDir,"c?n",PE_generic,NULL,2},
                    {"set_direction",PEVM_SetDir,"c?i",PE_generic,NULL,2},
                    {"set_direction",PEVM_SetDir,"c?I",PE_generic,NULL,2},
                    {"force_direction",PEVM_ForceDir,"o?n",PE_generic,NULL,2},
                    {"force_direction",PEVM_ForceDir,"o?i",PE_generic,NULL,2},
                    {"force_direction",PEVM_ForceDir,"o?I",PE_generic,NULL,2},
                    {"force_direction",PEVM_ForceDir,"O?n",PE_generic,NULL,2},
                    {"force_direction",PEVM_ForceDir,"O?i",PE_generic,NULL,2},
                    {"force_direction",PEVM_ForceDir,"O?I",PE_generic,NULL,2},
                    {"force_direction",PEVM_ForceDir,"c?n",PE_generic,NULL,2},
                    {"force_direction",PEVM_ForceDir,"c?i",PE_generic,NULL,2},
                    {"force_direction",PEVM_ForceDir,"c?I",PE_generic,NULL,2},
                    {"redraw_text",PEVM_RedrawText,"",PE_generic,NULL,0},
                    {"redraw_map",PEVM_RedrawMap,"",PE_generic,NULL,0},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?o?nn",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?o?ni",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?o?in",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?o?ii",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?o?iI",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?o?Ii",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?o?II",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?o?nI",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?o?In",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?O?nn",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?O?ni",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?O?in",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?O?ii",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?O?iI",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?O?Ii",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?O?II",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?O?nI",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"o?O?In",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?o?nn",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?o?ni",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?o?in",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?o?ii",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?o?iI",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?o?Ii",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?o?II",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?o?nI",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?o?In",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?O?nn",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?O?ni",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?O?in",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?O?ii",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?O?iI",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?O?Ii",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?O?II",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?O?nI",PE_generic,NULL,4},
                    {"move_from_pocket",PEVM_MoveFromPocket,"O?O?In",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?o?nn",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?o?ni",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?o?in",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?o?ii",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?o?iI",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?o?Ii",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?o?II",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?o?nI",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?o?In",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?O?nn",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?O?ni",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?O?in",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?O?ii",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?O?iI",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?O?Ii",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?O?II",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?O?nI",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"o?O?In",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?o?nn",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?o?ni",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?o?in",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?o?ii",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?o?iI",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?o?Ii",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?o?II",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?o?nI",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?o?In",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?O?nn",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?O?ni",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?O?in",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?O?ii",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?O?iI",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?O?Ii",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?O?II",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?O?nI",PE_generic,NULL,4},
                    {"force_from_pocket",PEVM_ForceFromPocket,"O?O?In",PE_generic,NULL,4},
                    {"move_to_pocket",PEVM_MoveToPocket,"o?o",PE_generic,NULL,2},
                    {"move_to_pocket",PEVM_MoveToPocket,"o?O",PE_generic,NULL,2},
                    {"move_to_pocket",PEVM_MoveToPocket,"O?o",PE_generic,NULL,2},
                    {"move_to_pocket",PEVM_MoveToPocket,"O?O",PE_generic,NULL,2},
                    {"transfer_to_pocket",PEVM_XferToPocket,"o?o",PE_generic,NULL,2},
                    {"transfer_to_pocket",PEVM_XferToPocket,"o?O",PE_generic,NULL,2},
                    {"transfer_to_pocket",PEVM_XferToPocket,"O?o",PE_generic,NULL,2},
                    {"transfer_to_pocket",PEVM_XferToPocket,"O?O",PE_generic,NULL,2},
                    {"spill",PEVM_Spill,"o",PE_generic,NULL,1},
                    {"spill",PEVM_Spill,"O",PE_generic,NULL,1},
                    {"spill",PEVM_SpillXY,"o?nn",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"o?ni",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"o?in",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"o?ii",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"o?iI",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"o?Ii",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"o?II",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"O?nn",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"O?ni",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"O?in",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"O?ii",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"O?iI",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"O?Ii",PE_generic,NULL,3},
                    {"spill",PEVM_SpillXY,"O?II",PE_generic,NULL,3},
                    {"move_contents",PEVM_MoveContents,"o?o",PE_generic,NULL,2},
                    {"move_contents",PEVM_MoveContents,"o?O",PE_generic,NULL,2},
                    {"move_contents",PEVM_MoveContents,"O?o",PE_generic,NULL,2},
                    {"move_contents",PEVM_MoveContents,"O?O",PE_generic,NULL,2},
                    {"empty",PEVM_Empty,"o",PE_generic,NULL,1},
                    {"empty",PEVM_Empty,"O",PE_generic,NULL,1},
                    {"fade_out",PEVM_FadeOut,"",PE_generic,NULL,0},
                    {"fade_in",PEVM_FadeIn,"",PE_generic,NULL,0},
                    {"move_to_top",PEVM_MoveToTop,"o",PE_generic,NULL,1},
                    {"move_to_top",PEVM_MoveToTop,"O",PE_generic,NULL,1},
                    {"move_to_bottom",PEVM_MoveToFloor,"o",PE_generic,NULL,1},
                    {"move_to_bottom",PEVM_MoveToFloor,"O",PE_generic,NULL,1},
                    {"move_to_floor",PEVM_MoveToFloor,"o",PE_generic,NULL,1},
                    {"move_to_floor",PEVM_MoveToFloor,"O",PE_generic,NULL,1},
                    {"insert_after",PEVM_InsertAfter,"o?o",PE_generic,NULL,2},
                    {"insert_after",PEVM_InsertAfter,"o?O",PE_generic,NULL,2},
                    {"insert_after",PEVM_InsertAfter,"O?o",PE_generic,NULL,2},
                    {"insert_after",PEVM_InsertAfter,"O?O",PE_generic,NULL,2},
                    {"check_hurt",PEVM_CheckHurt,"o",PE_generic,NULL,1},
                    {"check_hurt",PEVM_CheckHurt,"O",PE_generic,NULL,1},
                    {"check_hurt",PEVM_CheckHurt,"c",PE_generic,NULL,1},
                    {"get_line_of_sight",PEVM_GetLOS,"i=oo",PE_checkaccess,NULL,3},
                    {"get_line_of_sight",PEVM_GetLOS,"i=oO",PE_checkaccess,NULL,3},
                    {"get_line_of_sight",PEVM_GetLOS,"i=Oo",PE_checkaccess,NULL,3},
                    {"get_line_of_sight",PEVM_GetLOS,"i=OO",PE_checkaccess,NULL,3},
                    {"get_line_of_sight",PEVM_GetLOS,"I=oo",PE_checkaccess,NULL,3},
                    {"get_line_of_sight",PEVM_GetLOS,"I=oO",PE_checkaccess,NULL,3},
                    {"get_line_of_sight",PEVM_GetLOS,"I=Oo",PE_checkaccess,NULL,3},
                    {"get_line_of_sight",PEVM_GetLOS,"I=OO",PE_checkaccess,NULL,3},
                    {"get_distance",PEVM_GetDistance,"i=oo",PE_checkaccess,NULL,3},
                    {"get_distance",PEVM_GetDistance,"i=oO",PE_checkaccess,NULL,3},
                    {"get_distance",PEVM_GetDistance,"i=Oo",PE_checkaccess,NULL,3},
                    {"get_distance",PEVM_GetDistance,"i=OO",PE_checkaccess,NULL,3},
                    {"get_distance",PEVM_GetDistance,"I=oo",PE_checkaccess,NULL,3},
                    {"get_distance",PEVM_GetDistance,"I=oO",PE_checkaccess,NULL,3},
                    {"get_distance",PEVM_GetDistance,"I=Oo",PE_checkaccess,NULL,3},
                    {"get_distance",PEVM_GetDistance,"I=OO",PE_checkaccess,NULL,3},
                    {"if_solid",PEVM_IfSolid,"nn",PE_if,PEP_if,3},
                    {"if_solid",PEVM_IfSolid,"ni",PE_if,PEP_if,3},
                    {"if_solid",PEVM_IfSolid,"in",PE_if,PEP_if,3},
                    {"if_solid",PEVM_IfSolid,"nI",PE_if,PEP_if,3},
                    {"if_solid",PEVM_IfSolid,"In",PE_if,PEP_if,3},
                    {"if_solid",PEVM_IfSolid,"ii",PE_if,PEP_if,3},
                    {"if_solid",PEVM_IfSolid,"iI",PE_if,PEP_if,3},
                    {"if_solid",PEVM_IfSolid,"Ii",PE_if,PEP_if,3},
                    {"if_solid",PEVM_IfSolid,"II",PE_if,PEP_if,3},
                    {"if_visible",PEVM_IfVisible,"nn",PE_if,PEP_if,3},
                    {"if_visible",PEVM_IfVisible,"ni",PE_if,PEP_if,3},
                    {"if_visible",PEVM_IfVisible,"in",PE_if,PEP_if,3},
                    {"if_visible",PEVM_IfVisible,"nI",PE_if,PEP_if,3},
                    {"if_visible",PEVM_IfVisible,"In",PE_if,PEP_if,3},
                    {"if_visible",PEVM_IfVisible,"ii",PE_if,PEP_if,3},
                    {"if_visible",PEVM_IfVisible,"iI",PE_if,PEP_if,3},
                    {"if_visible",PEVM_IfVisible,"Ii",PE_if,PEP_if,3},
                    {"if_visible",PEVM_IfVisible,"II",PE_if,PEP_if,3},
                    {"set_user_flag",PEVM_SetUFlag,"s=n",PE_generic,NULL,2},
                    {"set_user_flag",PEVM_SetUFlag,"s=i",PE_generic,NULL,2},
                    {"set_user_flag",PEVM_SetUFlag,"s=I",PE_generic,NULL,2},
                    {"set_user_flag",PEVM_SetUFlag,"S=n",PE_generic,NULL,2},
                    {"set_user_flag",PEVM_SetUFlag,"S=i",PE_generic,NULL,2},
                    {"set_user_flag",PEVM_SetUFlag,"S=I",PE_generic,NULL,2},
                    {"set_user_flag",PEVM_SetUFlag,"P=n",PE_generic,NULL,2},
                    {"set_user_flag",PEVM_SetUFlag,"P=i",PE_generic,NULL,2},
                    {"set_user_flag",PEVM_SetUFlag,"P=I",PE_generic,NULL,2},
                    {"get_user_flag",PEVM_GetUFlag,"i=s",PE_checkaccess,NULL,2},
                    {"get_user_flag",PEVM_GetUFlag,"I=s",PE_checkaccess,NULL,2},
                    {"get_user_flag",PEVM_GetUFlag,"i=S",PE_checkaccess,NULL,2},
                    {"get_user_flag",PEVM_GetUFlag,"I=S",PE_checkaccess,NULL,2},
                    {"get_user_flag",PEVM_GetUFlag,"i=P",PE_checkaccess,NULL,2},
                    {"get_user_flag",PEVM_GetUFlag,"I=P",PE_checkaccess,NULL,2},
                    {"if_user_flag",PEVM_If_Uflag,"s",PE_if,PEP_if,2},
                    {"if_user_flag",PEVM_If_Uflag,"S",PE_if,PEP_if,2},
                    {"if_user_flag",PEVM_If_Uflag,"P",PE_if,PEP_if,2},
                    {"if_not_user_flag",PEVM_If_nUflag,"s",PE_if,PEP_if,2},
                    {"if_not_user_flag",PEVM_If_nUflag,"S",PE_if,PEP_if,2},
                    {"if_not_user_flag",PEVM_If_nUflag,"P",PE_if,PEP_if,2},
                    {"get_local",PEVM_GetLocal,"i=os",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"i=oS",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"i=oP",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"i=Os",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"i=OS",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"i=OP",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"I=os",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"I=oS",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"I=oP",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"I=Os",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"I=OS",PE_checkaccess,NULL,3},
                    {"get_local",PEVM_GetLocal,"I=OP",PE_checkaccess,NULL,3},
                    {"set_local",PEVM_SetLocal,"os=n",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"os=i",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"os=I",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"oS=n",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"oS=i",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"oS=I",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"oP=n",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"oP=i",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"oP=I",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"Os=n",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"Os=i",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"Os=I",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"OS=n",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"OS=i",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"OS=I",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"OP=n",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"OP=i",PE_generic,NULL,3},
                    {"set_local",PEVM_SetLocal,"OP=I",PE_generic,NULL,3},
                    {"if_local",PEVM_If_onLocal,"os",PE_if,PEP_if,3},
                    {"if_local",PEVM_If_onLocal,"Os",PE_if,PEP_if,3},
                    {"if_local",PEVM_If_onLocal,"cs",PE_if,PEP_if,3},
                    {"if_local",PEVM_If_onLocal,"oS",PE_if,PEP_if,3},
                    {"if_local",PEVM_If_onLocal,"OS",PE_if,PEP_if,3},
                    {"if_local",PEVM_If_onLocal,"cS",PE_if,PEP_if,3},
                    {"if_local",PEVM_If_onLocal,"oP",PE_if,PEP_if,3},
                    {"if_local",PEVM_If_onLocal,"OP",PE_if,PEP_if,3},
                    {"if_local",PEVM_If_onLocal,"cP",PE_if,PEP_if,3},
                    {"if_not_local",PEVM_If_nonLocal,"os",PE_if,PEP_if,3},
                    {"if_not_local",PEVM_If_nonLocal,"Os",PE_if,PEP_if,3},
                    {"if_not_local",PEVM_If_nonLocal,"cs",PE_if,PEP_if,3},
                    {"if_not_local",PEVM_If_nonLocal,"oS",PE_if,PEP_if,3},
                    {"if_not_local",PEVM_If_nonLocal,"OS",PE_if,PEP_if,3},
                    {"if_not_local",PEVM_If_nonLocal,"cS",PE_if,PEP_if,3},
                    {"if_not_local",PEVM_If_nonLocal,"oP",PE_if,PEP_if,3},
                    {"if_not_local",PEVM_If_nonLocal,"OP",PE_if,PEP_if,3},
                    {"if_not_local",PEVM_If_nonLocal,"cP",PE_if,PEP_if,3},
                    {"get_weight",PEVM_WeighObject,"i=o",PE_checkaccess,NULL,2},
                    {"get_weight",PEVM_WeighObject,"i=O",PE_checkaccess,NULL,2},
                    {"get_weight",PEVM_WeighObject,"I=o",PE_checkaccess,NULL,2},
                    {"get_weight",PEVM_WeighObject,"I=O",PE_checkaccess,NULL,2},
                    {"get_bulk",PEVM_GetBulk,"i=o",PE_checkaccess,NULL,2},
                    {"get_bulk",PEVM_GetBulk,"i=O",PE_checkaccess,NULL,2},
                    {"get_bulk",PEVM_GetBulk,"I=o",PE_checkaccess,NULL,2},
                    {"get_bulk",PEVM_GetBulk,"I=O",PE_checkaccess,NULL,2},
                    {"if_in_pocket",PEVM_IfInPocket,"o",PE_if,PEP_if,2},
                    {"if_in_pocket",PEVM_IfInPocket,"O",PE_if,PEP_if,2},
                    {"if_in_pocket",PEVM_IfInPocket,"c",PE_if,PEP_if,2},
                    {"if_not_in_pocket",PEVM_IfNInPocket,"o",PE_if,PEP_if,2},
                    {"if_not_in_pocket",PEVM_IfNInPocket,"O",PE_if,PEP_if,2},
                    {"if_not_in_pocket",PEVM_IfNInPocket,"c",PE_if,PEP_if,2},
                    {"resync_everything",PEVM_ReSyncEverything,"",PE_generic,NULL,0},
                    {"talk_to",PEVM_TalkTo1,"s",PE_generic,NULL,1},
                    {"talk_to",PEVM_TalkTo1,"S",PE_generic,NULL,1},
                    {"talk_to",PEVM_TalkTo1,"P",PE_generic,NULL,1},
                    {"talk_to",PEVM_TalkTo2,"ss",PE_generic,NULL,2},
                    {"talk_to",PEVM_TalkTo2,"sS",PE_generic,NULL,2},
                    {"talk_to",PEVM_TalkTo2,"sP",PE_generic,NULL,2},
                    {"talk_to",PEVM_TalkTo2,"Ss",PE_generic,NULL,2},
                    {"talk_to",PEVM_TalkTo2,"SS",PE_generic,NULL,2},
                    {"talk_to",PEVM_TalkTo2,"SP",PE_generic,NULL,2},
                    {"talk_to",PEVM_TalkTo2,"Ps",PE_generic,NULL,2},
                    {"talk_to",PEVM_TalkTo2,"PS",PE_generic,NULL,2},
                    {"talk_to",PEVM_TalkTo2,"PP",PE_generic,NULL,2},
                    {"random",PEVM_Random,"i?nn",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?ni",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?nI",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?na",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?in",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?ii",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?iI",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?ia",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?In",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?Ii",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?II",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?Ia",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?an",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?ai",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?aI",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"i?aa",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?nn",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?ni",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?nI",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?na",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?in",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?ii",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?iI",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?ia",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?In",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?Ii",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?II",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?Ia",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?an",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?ai",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?aI",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"I?aa",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?nn",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?ni",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?nI",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?na",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?in",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?ii",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?iI",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?ia",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?In",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?Ii",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?II",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?Ia",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?an",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?ai",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?aI",PE_checkaccess,NULL,3},
                    {"random",PEVM_Random,"a?aa",PE_checkaccess,NULL,3},
                    {"if_confirm",PEVM_GetYN,"s",PE_if,PEP_if,2},
                    {"if_confirm",PEVM_GetYN,"S",PE_if,PEP_if,2},
                    {"if_confirm",PEVM_GetYN,"P",PE_if,PEP_if,2},
                    {"if_getYN",PEVM_GetYN,"s",PE_if,PEP_if,2},
                    {"if_getYN",PEVM_GetYN,"S",PE_if,PEP_if,2},
                    {"if_getYN",PEVM_GetYN,"P",PE_if,PEP_if,2},
                    {"if_not_confirm",PEVM_GetYN,"s",PE_if,PEP_if,2},
                    {"if_not_confirm",PEVM_GetYNN,"S",PE_if,PEP_if,2},
                    {"if_not_confirm",PEVM_GetYNN,"P",PE_if,PEP_if,2},
                    {"if_not_getYN",PEVM_GetYN,"s",PE_if,PEP_if,2},
                    {"if_not_getYN",PEVM_GetYNN,"S",PE_if,PEP_if,2},
                    {"if_not_getYN",PEVM_GetYNN,"P",PE_if,PEP_if,2},
                    {"restart",PEVM_Restart,"",PE_generic,NULL,0},
                    {"restart_game",PEVM_Restart,"",PE_generic,NULL,0},
                    {"scroll_tile",PEVM_ScrTileN,"nnn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"nni",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"nnI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"nin",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"nIn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"nii",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"niI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"nIi",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"nII",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"inn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"ini",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"inI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"iin",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"iIn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"iii",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"iiI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"iIi",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"iII",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"Inn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"Ini",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"InI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"Iin",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"IIn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"Iii",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"IiI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"IIi",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileN,"III",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"snn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"sni",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"snI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"sin",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"sIn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"sii",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"siI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"sIi",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"sII",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"Snn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"Sni",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"SnI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"Sin",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"SIn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"Sii",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"SiI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"SIi",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"SII",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"Pnn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"Pni",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"PnI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"Pin",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"PIn",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"Pii",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"PiI",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"PIi",PE_generic,NULL,3},
                    {"scroll_tile",PEVM_ScrTileS,"PII",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"nnn",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"nni",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"nnI",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"nin",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"nIn",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"nii",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"niI",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"nIi",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"nII",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"inn",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"ini",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"inI",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"iin",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"iIn",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"iii",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"iiI",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"iIi",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"iII",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"Inn",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"Ini",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"InI",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"Iin",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"IIn",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"Iii",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"IiI",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"IIi",PE_generic,NULL,3},
                    {"scroll_tile_number",PEVM_ScrTileN,"III",PE_generic,NULL,3},
                    {"reset_tile",PEVM_ResetTileN,"n",PE_generic,NULL,1},
                    {"reset_tile",PEVM_ResetTileN,"i",PE_generic,NULL,1},
                    {"reset_tile",PEVM_ResetTileN,"I",PE_generic,NULL,1},
                    {"reset_tile",PEVM_ResetTileS,"s",PE_generic,NULL,1},
                    {"reset_tile",PEVM_ResetTileS,"S",PE_generic,NULL,1},
                    {"reset_tile",PEVM_ResetTileS,"P",PE_generic,NULL,1},
                    {"get_input",PEVM_GetKey,"",PE_generic,NULL,0},
                    {"get_key",PEVM_GetKey,"",PE_generic,NULL,0},
                    {"getkey",PEVM_GetKey,"",PE_generic,NULL,0},
                    {"get_input_quiet",PEVM_GetKey_quiet,"",PE_generic,NULL,0},
                    {"getkey_quiet",PEVM_GetKey_quiet,"",PE_generic,NULL,0},
                    {"get_key_quiet",PEVM_GetKey_quiet,"",PE_generic,NULL,0},

                    {"getkey_ascii",PEVM_GetKeyAscii,"i",PE_generic,NULL,0},
                    {"get_key_ascii",PEVM_GetKeyAscii,"i",PE_generic,NULL,0},
                    {"get_key_ascii_quiet",PEVM_GetKeyAscii_quiet,"i",PE_generic,NULL,0},
                    {"getkey_ascii_quiet",PEVM_GetKeyAscii_quiet,"i",PE_generic,NULL,0},
                    {"flushkeys",PEVM_FlushKeys,"",PE_generic,NULL,0},
                    {"flush_keys",PEVM_FlushKeys,"",PE_generic,NULL,0},


                    {"start_action",PEVM_DoAct,"o?f",PE_generic,NULL,2},
                    {"start_action",PEVM_DoAct,"O?f",PE_generic,NULL,2},
                    {"start_action",PEVM_DoAct,"c?f",PE_generic,NULL,2},
                    {"start_action",PEVM_DoAct,"o?i",PE_generic,NULL,2},
                    {"start_action",PEVM_DoAct,"o?I",PE_generic,NULL,2},
                    {"start_action",PEVM_DoAct,"O?i",PE_generic,NULL,2},
                    {"start_action",PEVM_DoAct,"O?I",PE_generic,NULL,2},
                    {"start_action",PEVM_DoAct,"c?i",PE_generic,NULL,2},
                    {"start_action",PEVM_DoAct,"c?I",PE_generic,NULL,2},
                    {"start_action",PEVM_DoActS,"o?s",PE_generic,NULL,2},
                    {"start_action",PEVM_DoActS,"o?S",PE_generic,NULL,2},
                    {"start_action",PEVM_DoActS,"o?P",PE_generic,NULL,2},
                    {"start_action",PEVM_DoActS,"O?s",PE_generic,NULL,2},
                    {"start_action",PEVM_DoActS,"O?S",PE_generic,NULL,2},
                    {"start_action",PEVM_DoActS,"O?P",PE_generic,NULL,2},
                    {"start_action",PEVM_DoActS,"c?s",PE_generic,NULL,2},
                    {"start_action",PEVM_DoActS,"c?S",PE_generic,NULL,2},
                    {"start_action",PEVM_DoActS,"c?P",PE_generic,NULL,2},
                    {"start_action",PEVM_DoActTo,"o?f?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActTo,"O?f?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActTo,"c?f?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActTo,"o?f?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActTo,"O?f?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActTo,"c?f?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"o?s?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"O?s?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"c?s?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"o?s?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"O?s?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"c?s?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"o?s?c",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"O?s?c",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"c?s?c",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"o?S?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"O?S?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"c?S?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"o?S?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"O?S?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"c?S?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"o?S?c",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"O?S?c",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"c?S?c",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"o?P?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"O?P?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"c?P?o",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"o?P?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"O?P?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"c?P?O",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"o?P?c",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"O?P?c",PE_generic,NULL,3},
                    {"start_action",PEVM_DoActSTo,"c?P?c",PE_generic,NULL,3},
                    {"do_action",PEVM_DoAct,"o?f",PE_generic,NULL,2},
                    {"do_action",PEVM_DoAct,"O?f",PE_generic,NULL,2},
                    {"do_action",PEVM_DoAct,"c?f",PE_generic,NULL,2},
                    {"do_action",PEVM_DoAct,"o?i",PE_generic,NULL,2},
                    {"do_action",PEVM_DoAct,"o?I",PE_generic,NULL,2},
                    {"do_action",PEVM_DoAct,"O?i",PE_generic,NULL,2},
                    {"do_action",PEVM_DoAct,"O?I",PE_generic,NULL,2},
                    {"do_action",PEVM_DoAct,"c?i",PE_generic,NULL,2},
                    {"do_action",PEVM_DoAct,"c?I",PE_generic,NULL,2},
                    {"do_action",PEVM_DoActS,"o?s",PE_generic,NULL,2},
                    {"do_action",PEVM_DoActS,"o?S",PE_generic,NULL,2},
                    {"do_action",PEVM_DoActS,"o?P",PE_generic,NULL,2},
                    {"do_action",PEVM_DoActS,"O?s",PE_generic,NULL,2},
                    {"do_action",PEVM_DoActS,"O?S",PE_generic,NULL,2},
                    {"do_action",PEVM_DoActS,"O?P",PE_generic,NULL,2},
                    {"do_action",PEVM_DoActS,"c?s",PE_generic,NULL,2},
                    {"do_action",PEVM_DoActS,"c?S",PE_generic,NULL,2},
                    {"do_action",PEVM_DoActS,"c?P",PE_generic,NULL,2},
                    {"do_action",PEVM_DoActTo,"o?f?o",PE_generic,NULL,3},
                    {"do_action",PEVM_DoActTo,"o?f?O",PE_generic,NULL,3},
                    {"do_action",PEVM_DoActTo,"o?f?c",PE_generic,NULL,3},
                    {"do_action",PEVM_DoActTo,"O?f?o",PE_generic,NULL,3},
                    {"do_action",PEVM_DoActTo,"O?f?O",PE_generic,NULL,3},
                    {"do_action",PEVM_DoActTo,"O?f?c",PE_generic,NULL,3},
                    {"do_action",PEVM_DoActTo,"c?f?o",PE_generic,NULL,3},
                    {"do_action",PEVM_DoActTo,"c?f?O",PE_generic,NULL,3},
                    {"do_action",PEVM_DoActTo,"c?f?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoAct,"o?f",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoAct,"O?f",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoAct,"c?f",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoAct,"o?i",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoAct,"o?I",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoAct,"O?i",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoAct,"O?I",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoAct,"c?i",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoAct,"c?I",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoActS,"o?s",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoActS,"o?S",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoActS,"O?s",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoActS,"O?S",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoActS,"c?s",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoActS,"c?S",PE_generic,NULL,2},
                    {"start_activity",PEVM_DoActTo,"o?f?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActTo,"o?f?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActTo,"o?f?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActTo,"O?f?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActTo,"O?f?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActTo,"O?f?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActTo,"c?f?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActTo,"c?f?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActTo,"c?f?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"o?s?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"o?s?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"o?s?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"O?s?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"O?s?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"O?s?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"c?s?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"c?s?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"c?s?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"o?S?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"o?S?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"o?S?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"O?S?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"O?S?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"O?S?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"c?S?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"c?S?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"c?S?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"o?P?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"o?P?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"o?P?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"O?P?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"O?P?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"O?P?c",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"c?P?o",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"c?P?O",PE_generic,NULL,3},
                    {"start_activity",PEVM_DoActSTo,"c?P?c",PE_generic,NULL,3},
                    {"do_activity",PEVM_DoAct,"o?f",PE_generic,NULL,2},
                    {"do_activity",PEVM_DoAct,"O?f",PE_generic,NULL,2},
                    {"do_activity",PEVM_DoAct,"c?f",PE_generic,NULL,2},
                    {"do_activity",PEVM_DoActTo,"o?f?o",PE_generic,NULL,3},
                    {"do_activity",PEVM_DoActTo,"o?f?O",PE_generic,NULL,3},
                    {"do_activity",PEVM_DoActTo,"o?f?c",PE_generic,NULL,3},
                    {"do_activity",PEVM_DoActTo,"O?f?o",PE_generic,NULL,3},
                    {"do_activity",PEVM_DoActTo,"O?f?O",PE_generic,NULL,3},
                    {"do_activity",PEVM_DoActTo,"O?f?c",PE_generic,NULL,3},
                    {"do_activity",PEVM_DoActTo,"c?f?o",PE_generic,NULL,3},
                    {"do_activity",PEVM_DoActTo,"c?f?O",PE_generic,NULL,3},
                    {"do_activity",PEVM_DoActTo,"c?f?c",PE_generic,NULL,3},
                    {"stop_action",PEVM_StopAct,"o",PE_generic,NULL,1},
                    {"stop_action",PEVM_StopAct,"O",PE_generic,NULL,1},
                    {"stop_action",PEVM_StopAct,"c",PE_generic,NULL,1},
                    {"stop_activity",PEVM_StopAct,"o",PE_generic,NULL,1},
                    {"stop_activity",PEVM_StopAct,"O",PE_generic,NULL,1},
                    {"stop_activity",PEVM_StopAct,"c",PE_generic,NULL,1},
                    {"resume_action",PEVM_ResumeAct,"o",PE_generic,NULL,1},
                    {"resume_action",PEVM_ResumeAct,"O",PE_generic,NULL,1},
                    {"resume_action",PEVM_ResumeAct,"c",PE_generic,NULL,1},
                    {"resume_activity",PEVM_ResumeAct,"o",PE_generic,NULL,1},
                    {"resume_activity",PEVM_ResumeAct,"O",PE_generic,NULL,1},
                    {"resume_activity",PEVM_ResumeAct,"c",PE_generic,NULL,1},
                    {"resume_schedule",PEVM_ResumeAct,"o",PE_generic,NULL,1},
                    {"resume_schedule",PEVM_ResumeAct,"O",PE_generic,NULL,1},
                    {"resume_schedule",PEVM_ResumeAct,"c",PE_generic,NULL,1},
                    {"last_action",PEVM_ResumeAct,"o",PE_generic,NULL,1},
                    {"last_action",PEVM_ResumeAct,"O",PE_generic,NULL,1},
                    {"last_action",PEVM_ResumeAct,"c",PE_generic,NULL,1},
                    {"last_activity",PEVM_ResumeAct,"o",PE_generic,NULL,1},
                    {"last_activity",PEVM_ResumeAct,"O",PE_generic,NULL,1},
                    {"last_activity",PEVM_ResumeAct,"c",PE_generic,NULL,1},

                    {"queue_action",PEVM_NextAct,"o?f",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextAct,"O?f",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextAct,"c?f",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextAct,"o?i",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextAct,"o?I",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextAct,"O?i",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextAct,"O?I",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextAct,"c?i",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextAct,"c?I",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextActS,"o?s",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextActS,"o?S",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextActS,"o?P",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextActS,"O?s",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextActS,"O?S",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextActS,"O?P",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextActS,"c?s",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextActS,"c?S",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextActS,"c?P",PE_generic,NULL,2},
                    {"queue_action",PEVM_NextActTo,"o?f?o",PE_generic,NULL,3},
                    {"queue_action",PEVM_NextActTo,"o?f?O",PE_generic,NULL,3},
                    {"queue_action",PEVM_NextActTo,"o?f?c",PE_generic,NULL,3},
                    {"queue_action",PEVM_NextActTo,"O?f?o",PE_generic,NULL,3},
                    {"queue_action",PEVM_NextActTo,"O?f?O",PE_generic,NULL,3},
                    {"queue_action",PEVM_NextActTo,"O?f?c",PE_generic,NULL,3},
                    {"queue_action",PEVM_NextActTo,"c?f?o",PE_generic,NULL,3},
                    {"queue_action",PEVM_NextActTo,"c?f?O",PE_generic,NULL,3},
                    {"queue_action",PEVM_NextActTo,"c?f?c",PE_generic,NULL,3},

                    {"insert_action",PEVM_InsAct,"o?f",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsAct,"O?f",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsAct,"c?f",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsAct,"o?i",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsAct,"o?I",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsAct,"O?i",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsAct,"O?I",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsAct,"c?i",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsAct,"c?I",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsActS,"o?s",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsActS,"o?S",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsActS,"o?P",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsActS,"O?s",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsActS,"O?S",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsActS,"O?P",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsActS,"c?s",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsActS,"c?S",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsActS,"c?P",PE_generic,NULL,2},
                    {"insert_action",PEVM_InsActTo,"o?f?o",PE_generic,NULL,3},
                    {"insert_action",PEVM_InsActTo,"o?f?O",PE_generic,NULL,3},
                    {"insert_action",PEVM_InsActTo,"o?f?c",PE_generic,NULL,3},
                    {"insert_action",PEVM_InsActTo,"O?f?o",PE_generic,NULL,3},
                    {"insert_action",PEVM_InsActTo,"O?f?O",PE_generic,NULL,3},
                    {"insert_action",PEVM_InsActTo,"O?f?c",PE_generic,NULL,3},
                    {"insert_action",PEVM_InsActTo,"c?f?o",PE_generic,NULL,3},
                    {"insert_action",PEVM_InsActTo,"c?f?O",PE_generic,NULL,3},
                    {"insert_action",PEVM_InsActTo,"c?f?c",PE_generic,NULL,3},

                    {"do_queue",PEVM_ResumeAct,"o",PE_generic,NULL,1},
                    {"do_queue",PEVM_ResumeAct,"O",PE_generic,NULL,1},
                    {"do_queue",PEVM_ResumeAct,"c",PE_generic,NULL,1},
                    {"start_queue",PEVM_ResumeAct,"o",PE_generic,NULL,1},
                    {"start_queue",PEVM_ResumeAct,"O",PE_generic,NULL,1},
                    {"start_queue",PEVM_ResumeAct,"c",PE_generic,NULL,1},
                    {"run_queue",PEVM_ResumeAct,"o",PE_generic,NULL,1},
                    {"run_queue",PEVM_ResumeAct,"O",PE_generic,NULL,1},
                    {"run_queue",PEVM_ResumeAct,"c",PE_generic,NULL,1},

                    {"find_container",PEVM_GetContainer,"o?o",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"o?O",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"o?c",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"O?o",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"O?O",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"O?c",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"c?o",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"c?O",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"c?c",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"o??o",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"o??O",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"o??c",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"O??o",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"O??O",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"O??c",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"c??o",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"c??O",PE_generic,NULL,2},
                    {"find_container",PEVM_GetContainer,"c??c",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"o?o",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"o?O",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"o?c",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"O?o",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"O?O",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"O?c",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"c?o",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"c?O",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"c?c",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"o??o",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"o??O",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"o??c",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"O??o",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"O??O",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"O??c",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"c??o",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"c??O",PE_generic,NULL,2},
                    {"find_pocket",PEVM_GetContainer,"c??c",PE_generic,NULL,2},
                    {"set_leader",PEVM_SetLeader,"off",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"ofi",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"ofI",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"oif",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"oIf",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"oii",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"oIi",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"oiI",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"oII",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"Off",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"Ofi",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"OfI",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"Oif",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"OIf",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"Oii",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"OIi",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"OiI",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"OII",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"cff",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"cfi",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"cfI",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"cif",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"cIf",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"cii",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"cIi",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"ciI",PE_generic,NULL,3},
                    {"set_leader",PEVM_SetLeader,"cII",PE_generic,NULL,3},
                    {"set_member",PEVM_SetMember,"of",PE_generic,NULL,2},
                    {"set_member",PEVM_SetMember,"oi",PE_generic,NULL,2},
                    {"set_member",PEVM_SetMember,"oI",PE_generic,NULL,2},
                    {"set_member",PEVM_SetMember,"Of",PE_generic,NULL,2},
                    {"set_member",PEVM_SetMember,"Oi",PE_generic,NULL,2},
                    {"set_member",PEVM_SetMember,"OI",PE_generic,NULL,2},
                    {"set_member",PEVM_SetMember,"cf",PE_generic,NULL,2},
                    {"set_member",PEVM_SetMember,"ci",PE_generic,NULL,2},
                    {"set_member",PEVM_SetMember,"cI",PE_generic,NULL,2},
                    {"set_player",PEVM_SetMember,"of",PE_generic,NULL,2},
                    {"set_player",PEVM_SetMember,"oi",PE_generic,NULL,2},
                    {"set_player",PEVM_SetMember,"oI",PE_generic,NULL,2},
                    {"set_player",PEVM_SetMember,"Of",PE_generic,NULL,2},
                    {"set_player",PEVM_SetMember,"Oi",PE_generic,NULL,2},
                    {"set_player",PEVM_SetMember,"OI",PE_generic,NULL,2},
                    {"set_player",PEVM_SetMember,"cf",PE_generic,NULL,2},
                    {"set_player",PEVM_SetMember,"ci",PE_generic,NULL,2},
                    {"set_player",PEVM_SetMember,"cI",PE_generic,NULL,2},
                    {"add_member",PEVM_AddMember,"o",PE_generic,NULL,1},
                    {"add_member",PEVM_AddMember,"O",PE_generic,NULL,1},
                    {"add_member",PEVM_AddMember,"c",PE_generic,NULL,1},
                    {"del_member",PEVM_DelMember,"o",PE_generic,NULL,1},
                    {"del_member",PEVM_DelMember,"O",PE_generic,NULL,1},
                    {"del_member",PEVM_DelMember,"c",PE_generic,NULL,1},
                    {"move_towards",PEVM_MoveTowards8,"oo",PE_generic,NULL,2},
                    {"move_towards",PEVM_MoveTowards8,"oO",PE_generic,NULL,2},
                    {"move_towards",PEVM_MoveTowards8,"oc",PE_generic,NULL,2},
                    {"move_towards",PEVM_MoveTowards8,"Oo",PE_generic,NULL,2},
                    {"move_towards",PEVM_MoveTowards8,"OO",PE_generic,NULL,2},
                    {"move_towards",PEVM_MoveTowards8,"Oc",PE_generic,NULL,2},
                    {"move_towards",PEVM_MoveTowards8,"co",PE_generic,NULL,2},
                    {"move_towards",PEVM_MoveTowards8,"cO",PE_generic,NULL,2},
                    {"move_towards",PEVM_MoveTowards8,"cc",PE_generic,NULL,2},
                    {"move_towards4",PEVM_MoveTowards4,"oo",PE_generic,NULL,2},
                    {"move_towards4",PEVM_MoveTowards4,"oO",PE_generic,NULL,2},
                    {"move_towards4",PEVM_MoveTowards4,"oc",PE_generic,NULL,2},
                    {"move_towards4",PEVM_MoveTowards4,"Oo",PE_generic,NULL,2},
                    {"move_towards4",PEVM_MoveTowards4,"OO",PE_generic,NULL,2},
                    {"move_towards4",PEVM_MoveTowards4,"Oc",PE_generic,NULL,2},
                    {"move_towards4",PEVM_MoveTowards4,"co",PE_generic,NULL,2},
                    {"move_towards4",PEVM_MoveTowards4,"cO",PE_generic,NULL,2},
                    {"move_towards4",PEVM_MoveTowards4,"cc",PE_generic,NULL,2},
                    {"wait",PEVM_WaitFor,"nn",PE_generic,NULL,2},
                    {"wait",PEVM_WaitFor,"in",PE_generic,NULL,2},
                    {"wait",PEVM_WaitFor,"In",PE_generic,NULL,2},
                    {"wait",PEVM_WaitFor,"an",PE_generic,NULL,2},
                    {"wait_for_animation",PEVM_WaitForAnimation,"o",PE_generic,NULL,1},
                    {"wait_for_animation",PEVM_WaitForAnimation,"O",PE_generic,NULL,1},
                    {"wait_for_animation",PEVM_WaitForAnimation,"c",PE_generic,NULL,1},
                    {"if_onscreen",PEVM_If_oonscreen,"o",PE_if,PEP_if,2},
                    {"if_onscreen",PEVM_If_oonscreen,"O",PE_if,PEP_if,2},
                    {"if_onscreen",PEVM_If_oonscreen,"c",PE_if,PEP_if,2},
                    {"if_not_onscreen",PEVM_If_not_oonscreen,"o",PE_if,PEP_if,2},
                    {"if_not_onscreen",PEVM_If_not_oonscreen,"O",PE_if,PEP_if,2},
                    {"if_not_onscreen",PEVM_If_not_oonscreen,"c",PE_if,PEP_if,2},
                    {"input",PEVM_InputInt,"i",PE_generic,NULL,1},
                    {"input",PEVM_InputInt,"I",PE_generic,NULL,1},
                    {"input",PEVM_InputInt,"a",PE_generic,NULL,1},
                    {"take",PEVM_TakeQty,"ns?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"is?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"Is?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"as?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"nS?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"iS?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"IS?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"aS?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"nP?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"iP?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"IP?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"aP?o",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"ns?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"is?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"Is?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"as?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"nS?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"iS?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"IS?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"aS?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"nP?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"iP?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"IP?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"aP?O",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"ns?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"is?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"Is?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"as?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"nS?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"iS?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"IS?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"aS?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"nP?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"iP?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"IP?c",PE_generic,NULL,3},
                    {"take",PEVM_TakeQty,"aP?c",PE_generic,NULL,3},

                    {"give",PEVM_GiveQty,"ns?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"is?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"Is?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"as?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"nS?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"iS?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"IS?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"aS?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"nP?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"iP?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"IP?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"aP?o",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"ns?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"is?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"Is?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"as?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"nS?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"iS?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"IS?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"aS?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"nP?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"iP?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"IP?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"aP?O",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"ns?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"is?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"Is?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"as?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"nS?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"iS?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"IS?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"aS?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"nP?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"iP?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"IP?c",PE_generic,NULL,3},
                    {"give",PEVM_GiveQty,"aP?c",PE_generic,NULL,3},

                    {"move",PEVM_MoveQty,"ns?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"is?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"Is?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"as?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nS?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iS?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IS?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aS?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nP?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iP?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IP?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aP?o?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"ns?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"is?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"Is?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"as?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nS?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iS?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IS?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aS?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nP?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iP?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IP?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aP?O?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"ns?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"is?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"Is?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"as?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nS?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iS?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IS?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aS?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nP?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iP?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IP?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aP?c?o",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"ns?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"is?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"Is?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"as?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nS?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iS?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IS?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aS?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nP?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iP?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IP?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aP?o?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"ns?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"is?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"Is?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"as?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nS?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iS?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IS?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aS?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nP?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iP?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IP?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aP?O?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"ns?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"is?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"Is?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"as?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nS?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iS?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IS?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aS?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nP?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iP?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IP?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aP?c?O",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"ns?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"is?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"Is?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"as?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nS?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iS?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IS?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aS?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nP?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iP?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IP?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aP?o?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"ns?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"is?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"Is?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"as?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nS?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iS?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IS?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aS?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nP?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iP?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IP?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aP?O?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"ns?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"is?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"Is?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"as?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nS?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iS?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IS?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aS?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"nP?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"iP?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"IP?c?c",PE_generic,NULL,4},
                    {"move",PEVM_MoveQty,"aP?c?c",PE_generic,NULL,4},

                    {"count",PEVM_CountQty,"i=s?o",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"I=s?o",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"a=s?o",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"i=S?o",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"I=S?o",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"a=S?o",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"i=P?o",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"I=P?o",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"a=P?o",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"i=s?O",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"I=s?O",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"a=s?O",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"i=S?O",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"I=S?O",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"a=S?O",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"i=P?O",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"I=P?O",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"a=P?O",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"i=s?c",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"I=s?c",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"a=s?c",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"i=S?c",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"I=S?c",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"a=S?c",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"i=P?c",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"I=P?c",PE_generic,NULL,3},
                    {"count",PEVM_CountQty,"a=P?c",PE_generic,NULL,3},
                    {"copy_schedule",PEVM_CopySchedule,"o?o",PE_generic,NULL,2},
                    {"copy_schedule",PEVM_CopySchedule,"o?O",PE_generic,NULL,2},
                    {"copy_schedule",PEVM_CopySchedule,"o?c",PE_generic,NULL,2},
                    {"copy_schedule",PEVM_CopySchedule,"O?o",PE_generic,NULL,2},
                    {"copy_schedule",PEVM_CopySchedule,"O?O",PE_generic,NULL,2},
                    {"copy_schedule",PEVM_CopySchedule,"O?c",PE_generic,NULL,2},
                    {"copy_schedule",PEVM_CopySchedule,"c?o",PE_generic,NULL,2},
                    {"copy_schedule",PEVM_CopySchedule,"c?O",PE_generic,NULL,2},
                    {"copy_schedule",PEVM_CopySchedule,"c?c",PE_generic,NULL,2},
                    {"copy_speech",PEVM_CopySpeech,"o?o",PE_generic,NULL,2},
                    {"copy_speech",PEVM_CopySpeech,"o?O",PE_generic,NULL,2},
                    {"copy_speech",PEVM_CopySpeech,"o?c",PE_generic,NULL,2},
                    {"copy_speech",PEVM_CopySpeech,"O?o",PE_generic,NULL,2},
                    {"copy_speech",PEVM_CopySpeech,"O?O",PE_generic,NULL,2},
                    {"copy_speech",PEVM_CopySpeech,"O?c",PE_generic,NULL,2},
                    {"copy_speech",PEVM_CopySpeech,"c?o",PE_generic,NULL,2},
                    {"copy_speech",PEVM_CopySpeech,"c?O",PE_generic,NULL,2},
                    {"copy_speech",PEVM_CopySpeech,"c?c",PE_generic,NULL,2},
                    {"for_all_onscreen",PEVM_AllOnscreen,"f",PE_generic,NULL,1},
                    {"all_around",PEVM_AllAround,"o?n?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"o?i?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"o?I?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"o?a?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"O?n?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"O?i?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"O?I?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"O?a?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"c?n?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"c?i?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"c?I?f",PE_generic,NULL,3},
                    {"all_around",PEVM_AllAround,"c?a?f",PE_generic,NULL,3},
                    {"check_time",PEVM_CheckTime,"",PE_generic,NULL,0},
                    {"find_nearest",PEVM_FindNear,"o=s?o",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"O=s?o",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"c=s?o",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"o=s?O",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"O=s?O",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"c=s?O",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"o=s?c",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"O=s?c",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"c=s?c",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"o=S?o",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"O=S?o",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"c=S?o",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"o=S?O",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"O=S?O",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"c=S?O",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"o=S?c",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"O=S?c",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"c=S?c",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"o=P?o",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"O=P?o",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"c=P?o",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"o=P?O",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"O=P?O",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"c=P?O",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"o=P?c",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"O=P?c",PE_checkaccess,NULL,3},
                    {"find_nearest",PEVM_FindNear,"c=P?c",PE_checkaccess,NULL,3},
                    {"find_nearby",PEVM_FindNearby,"c=s?o",PE_checkaccess,NULL,3},
                    {"find_nearby",PEVM_FindNearby,"c=s?O",PE_checkaccess,NULL,3},
                    {"find_nearby",PEVM_FindNearby,"c=s?c",PE_checkaccess,NULL,3},
                    {"find_nearby",PEVM_FindNearby,"c=S?o",PE_checkaccess,NULL,3},
                    {"find_nearby",PEVM_FindNearby,"c=S?O",PE_checkaccess,NULL,3},
                    {"find_nearby",PEVM_FindNearby,"c=S?c",PE_checkaccess,NULL,3},
                    {"find_nearby",PEVM_FindNearby,"c=P?o",PE_checkaccess,NULL,3},
                    {"find_nearby",PEVM_FindNearby,"c=P?O",PE_checkaccess,NULL,3},
                    {"find_nearby",PEVM_FindNearby,"c=P?c",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=n?o",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=n?O",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=n?c",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=i?o",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=i?O",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=i?c",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=I?o",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=I?O",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=I?c",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=a?o",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=a?O",PE_checkaccess,NULL,3},
                    {"find_nearby_flag",PEVM_FindNearbyFlag,"c=a?c",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=sn",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=si",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=sI",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=sa",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=Sn",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=Si",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=SI",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=Sa",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=Pn",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=Pi",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=PI",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"o=Pa",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=sn",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=si",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=sI",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=sa",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=Sn",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=Si",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=SI",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=Sa",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=Pn",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=Pi",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=PI",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"O=Pa",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=sn",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=si",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=sI",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=sa",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=Sn",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=Si",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=SI",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=Sa",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=Pn",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=Pi",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=PI",PE_checkaccess,NULL,3},
                    {"find_tag",PEVM_FindTag,"c=Pa",PE_checkaccess,NULL,3},

                    {"fast_tag",PEVM_FastTag,"o=n?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=n?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=n?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=i?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=i?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=i?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=I?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=I?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=I?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=a?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=a?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"o=a?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=n?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=n?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=n?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=i?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=i?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=i?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=I?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=I?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=I?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=a?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=a?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"O=a?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=n?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=n?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=n?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=i?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=i?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=i?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=I?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=I?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=I?c",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=a?o",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=a?O",PE_checkaccess,NULL,3},
                    {"fast_tag",PEVM_FastTag,"c=a?c",PE_checkaccess,NULL,3},

                    {"find_tags",PEVM_MakeTagList,"c=sn",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=si",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=sI",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=sa",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=Sn",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=Si",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=SI",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=Sa",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=Pn",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=Pi",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=PI",PE_checkaccess,NULL,3},
                    {"find_tags",PEVM_MakeTagList,"c=Pa",PE_checkaccess,NULL,3},

                    {"find_next",PEVM_FindNext,"o=s?o",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"o=s?O",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"o=s?c",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"o=S?o",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"o=S?O",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"o=S?c",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"o=P?o",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"o=P?O",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"o=P?c",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"O=s?o",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"O=s?O",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"O=s?c",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"O=S?o",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"O=S?O",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"O=S?c",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"O=P?o",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"O=P?O",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"O=P?c",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"c=s?o",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"c=s?O",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"c=s?c",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"c=S?o",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"c=S?O",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"c=S?c",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"c=P?o",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"c=P?O",PE_checkaccess,NULL,3},
                    {"find_next",PEVM_FindNext,"c=P?c",PE_checkaccess,NULL,3},

                    {"light_tag",PEVM_SetLight,"n=n",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"n=i",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"n=I",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"n=a",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"i=n",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"i=i",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"i=I",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"i=a",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"I=n",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"I=i",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"I=I",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"I=a",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"a=n",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"a=i",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"a=I",PE_generic,NULL,2},
                    {"light_tag",PEVM_SetLight,"a=a",PE_generic,NULL,2},

                    {"set_light",PEVM_SetLight_single,"o?o=n",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?o=i",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?o=I",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?o=a",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?O=n",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?O=i",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?O=I",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?O=a",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?c=n",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?c=i",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?c=I",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"o?c=a",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?o=n",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?o=i",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?o=I",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?o=a",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?O=n",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?O=i",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?O=I",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?O=a",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?c=n",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?c=i",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?c=I",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"O?c=a",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?o=n",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?o=i",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?o=I",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?o=a",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?O=n",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?O=i",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?O=I",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?O=a",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?c=n",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?c=i",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?c=I",PE_generic,NULL,3},
                    {"set_light",PEVM_SetLight_single,"c?c=a",PE_generic,NULL,3},

                    {"printaddr",PEVM_Printaddr,"i",PE_generic,NULL,1},
                    {"printaddr",PEVM_Printaddr,"I",PE_generic,NULL,1},
                    {"printaddr",PEVM_Printaddr,"o",PE_generic,NULL,1},
                    {"printaddr",PEVM_Printaddr,"O",PE_generic,NULL,1},
                    {"printaddr",PEVM_Printaddr,"c",PE_generic,NULL,1},
                    {"BUG",PEVM_PrintLog,"n",PE_generic,NULL,1},
                    {"dump_vm",PEVM_Dump,"",PE_generic,NULL,0},
                    {"move_party_to",PEVM_MovePartyToObj,"o",PE_generic,NULL,1},
                    {"move_party_to",PEVM_MovePartyToObj,"O",PE_generic,NULL,1},
                    {"move_party_to",PEVM_MovePartyToObj,"c",PE_generic,NULL,1},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?nn",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?ni",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?nI",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?na",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?in",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?ii",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?iI",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?ia",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?In",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?Ii",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?II",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?Ia",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?an",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?ai",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?aI",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"o?aa",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?nn",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?ni",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?nI",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?na",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?in",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?ii",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?iI",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?ia",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?In",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?Ii",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?II",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?Ia",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?an",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?ai",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?aI",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"O?aa",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?nn",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?ni",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?nI",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?na",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?in",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?ii",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?iI",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?ia",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?In",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?Ii",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?II",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?Ia",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?an",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?ai",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?aI",PE_generic,NULL,3},
                    {"move_party_from",PEVM_MovePartyFromObj,"c?aa",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?nn",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?ni",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?nI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?na",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?in",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?ii",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?iI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?ia",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?In",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?Ii",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?II",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?Ia",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?an",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?ai",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?aI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"n?aa",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?nn",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?ni",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?nI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?na",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?in",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?ii",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?iI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?ia",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?In",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?Ii",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?II",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?Ia",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?an",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?ai",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?aI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"i?aa",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?nn",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?ni",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?nI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?na",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?in",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?ii",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?iI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?ia",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?In",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?Ii",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?II",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?Ia",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?an",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?ai",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?aI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"I?aa",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?nn",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?ni",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?nI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?na",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?in",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?ii",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?iI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?ia",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?In",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?Ii",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?II",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?Ia",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?an",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?ai",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?aI",PE_generic,NULL,3},
                    {"move_tag",PEVM_MoveTag,"a?aa",PE_generic,NULL,3},
                    {"update_tag",PEVM_UpdateTag,"n",PE_generic,NULL,1},
                    {"update_tag",PEVM_UpdateTag,"i",PE_generic,NULL,1},
                    {"update_tag",PEVM_UpdateTag,"I",PE_generic,NULL,1},
                    {"get_data",PEVM_GetDataSSS,"P?s=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSS,"P?s=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSS,"P?s=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSS,"P?S=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSS,"P?S=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSS,"P?S=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSS,"P?P=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSS,"P?P=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSS,"P?P=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSI,"P?s=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSI,"P?s=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSI,"P?S=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSI,"P?S=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSI,"P?P=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSSI,"P?P=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSIS,"P?T=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSIS,"P?T=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSIS,"P?T=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSII,"P?T=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataSII,"P?T=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"i?s=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"i?s=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"i?s=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"i?S=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"i?S=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"i?S=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"i?P=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"i?P=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"i?P=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"i?s=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"i?s=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"i?S=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"i?S=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"i?P=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"i?P=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataIIS,"i?T=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataIIS,"i?T=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataIIS,"i?T=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataIII,"i?T=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataIII,"i?T=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"I?s=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"I?s=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"I?s=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"I?S=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"I?S=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"I?S=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"I?P=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISS,"I?P=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"I?s=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"I?s=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"I?S=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"I?S=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"I?P=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataISI,"I?P=I",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataIIS,"I?T=s",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataIIS,"I?T=S",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataIIS,"I?T=P",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataIII,"I?T=i",PE_checkaccess,NULL,3},
                    {"get_data",PEVM_GetDataIII,"I?T=I",PE_checkaccess,NULL,3},

                    {"get_datakeys",PEVM_GetDataKeysII,"a=T",PE_checkaccess,NULL,2},
                    {"get_datakeys",PEVM_GetDataKeysIS,"a=s",PE_checkaccess,NULL,2},
                    {"get_datakeys",PEVM_GetDataKeysIS,"a=S",PE_checkaccess,NULL,2},
                    {"get_datakeys",PEVM_GetDataKeysIS,"a=P",PE_checkaccess,NULL,2},
                    {"get_datakeys",PEVM_GetDataKeysSI,"b=T",PE_checkaccess,NULL,2},
                    {"get_datakeys",PEVM_GetDataKeysSS,"b=s",PE_checkaccess,NULL,2},
                    {"get_datakeys",PEVM_GetDataKeysSS,"b=S",PE_checkaccess,NULL,2},
                    {"get_datakeys",PEVM_GetDataKeysSS,"b=P",PE_checkaccess,NULL,2},
/*
                    {"get_datavalues",PEVM_GetDataValsII,"a=T",PE_checkaccess,NULL,2},
                    {"get_datavalues",PEVM_GetDataValsIS,"a=s",PE_checkaccess,NULL,2},
                    {"get_datavalues",PEVM_GetDataValsIS,"a=S",PE_checkaccess,NULL,2},
                    {"get_datavalues",PEVM_GetDataValsIS,"a=P",PE_checkaccess,NULL,2},
                    {"get_datavalues",PEVM_GetDataValsSI,"b=T",PE_checkaccess,NULL,2},
                    {"get_datavalues",PEVM_GetDataValsSS,"b=s",PE_checkaccess,NULL,2},
                    {"get_datavalues",PEVM_GetDataValsSS,"b=S",PE_checkaccess,NULL,2},
                    {"get_datavalues",PEVM_GetDataValsSS,"b=P",PE_checkaccess,NULL,2},
*/
                    {"get_decor",PEVM_GetDecor,"iio",PE_checkaccess,NULL,3},
                    {"get_decor",PEVM_GetDecor,"iIo",PE_checkaccess,NULL,3},
                    {"get_decor",PEVM_GetDecor,"Iio",PE_checkaccess,NULL,3},
                    {"get_decor",PEVM_GetDecor,"IIo",PE_checkaccess,NULL,3},
                    {"get_decor",PEVM_GetDecor,"iiO",PE_checkaccess,NULL,3},
                    {"get_decor",PEVM_GetDecor,"iIO",PE_checkaccess,NULL,3},
                    {"get_decor",PEVM_GetDecor,"IiO",PE_checkaccess,NULL,3},
                    {"get_decor",PEVM_GetDecor,"IIO",PE_checkaccess,NULL,3},

                    {"del_decor",PEVM_DelDecor,"nno",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"nio",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"nIo",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"ino",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"iio",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"iIo",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"Ino",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"Iio",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"IIo",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"nnO",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"niO",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"nIO",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"inO",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"iiO",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"iIO",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"InO",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"IiO",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"IIO",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"nnc",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"nic",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"nIc",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"inc",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"iic",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"iIc",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"Inc",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"Iic",PE_generic,NULL,3},
                    {"del_decor",PEVM_DelDecor,"IIc",PE_generic,NULL,3},
                    {"find",PEVM_SearchContainer,"o=s?o",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"o=s?O",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"o=s?c",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"o=S?o",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"o=S?O",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"o=S?c",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"o=P?o",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"o=P?O",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"o=P?c",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"O=s?o",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"O=s?O",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"O=s?c",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"O=S?o",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"O=S?O",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"O=S?c",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"O=P?o",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"O=P?O",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"O=P?c",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"c=s?o",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"c=s?O",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"c=s?c",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"c=S?o",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"c=S?O",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"c=S?c",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"c=P?o",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"c=P?O",PE_checkaccess,NULL,3},
                    {"find",PEVM_SearchContainer,"c=P?c",PE_checkaccess,NULL,3},

                    {"change_map",PEVM_ChangeMap1,"n?nn",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"n?ni",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"n?nI",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"n?in",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"n?ii",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"n?iI",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"n?In",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"n?Ii",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"n?II",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"i?nn",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"i?ni",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"i?nI",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"i?in",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"i?ii",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"i?iI",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"i?In",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"i?Ii",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"i?II",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"I?nn",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"I?ni",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"I?nI",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"I?in",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"I?ii",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"I?iI",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"I?In",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"I?Ii",PE_generic,NULL,3},
                    {"change_map",PEVM_ChangeMap1,"I?II",PE_generic,NULL,3},

                    {"change_map",PEVM_ChangeMap2,"n?n",PE_generic,NULL,2},
                    {"change_map",PEVM_ChangeMap2,"n?i",PE_generic,NULL,2},
                    {"change_map",PEVM_ChangeMap2,"n?I",PE_generic,NULL,2},
                    {"change_map",PEVM_ChangeMap2,"i?n",PE_generic,NULL,2},
                    {"change_map",PEVM_ChangeMap2,"i?i",PE_generic,NULL,2},
                    {"change_map",PEVM_ChangeMap2,"i?I",PE_generic,NULL,2},
                    {"change_map",PEVM_ChangeMap2,"I?n",PE_generic,NULL,2},
                    {"change_map",PEVM_ChangeMap2,"I?i",PE_generic,NULL,2},
                    {"change_map",PEVM_ChangeMap2,"I?I",PE_generic,NULL,2},
                    {"get_mapno",PEVM_GetMapNo,"i",PE_generic,NULL,1},
                    {"get_mapno",PEVM_GetMapNo,"I",PE_generic,NULL,1},

                    {"scroll_picture",PEVM_PicScroll,"snn",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"sni",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"snI",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"sna",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"sin",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"sii",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"siI",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"sia",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"sIn",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"sIi",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"sII",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"sIa",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"san",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"sai",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"saI",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"saa",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Snn",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Sni",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"SnI",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Sna",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Sin",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Sii",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"SiI",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Sia",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"SIn",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"SIi",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"SII",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"SIa",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"San",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Sai",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"SaI",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Saa",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Pnn",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Pni",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"PnI",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Pna",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Pin",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Pii",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"PiI",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Pia",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"PIn",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"PIi",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"PII",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"PIa",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Pan",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Pai",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"PaI",PE_generic,NULL,3},
                    {"scroll_picture",PEVM_PicScroll,"Paa",PE_generic,NULL,3},

                    {"set_panel",PEVM_SetPanel,"s",PE_generic,NULL,1},
                    {"set_panel",PEVM_SetPanel,"S",PE_generic,NULL,1},
                    {"set_panel",PEVM_SetPanel,"a",PE_generic,NULL,1},

                    {"pathmarker",PEVM_FindPathMarker,"o=s?o",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"o=s?O",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"o=s?c",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"o=S?o",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"o=S?O",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"o=S?c",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"o=P?o",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"o=P?O",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"o=P?c",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"O=s?o",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"O=s?O",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"O=s?c",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"O=S?o",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"O=S?O",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"O=S?c",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"O=P?o",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"O=P?O",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"O=P?c",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"c=s?o",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"c=s?O",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"c=s?c",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"c=S?o",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"c=S?O",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"c=S?c",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"c=P?o",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"c=P?O",PE_generic,NULL,3},
                    {"pathmarker",PEVM_FindPathMarker,"c=P?c",PE_generic,NULL,3},
                    {"centre",PEVM_ReSolid,"o",PE_generic,NULL,1},
                    {"centre",PEVM_ReSolid,"O",PE_generic,NULL,1},
                    {"centre",PEVM_ReSolid,"c",PE_generic,NULL,1},
                    {"centre",PEVM_ReSolid,"?o",PE_generic,NULL,1},
                    {"centre",PEVM_ReSolid,"?O",PE_generic,NULL,1},
                    {"centre",PEVM_ReSolid,"?c",PE_generic,NULL,1},
                    {"center",PEVM_ReSolid,"o",PE_generic,NULL,1},
                    {"center",PEVM_ReSolid,"O",PE_generic,NULL,1},
                    {"center",PEVM_ReSolid,"c",PE_generic,NULL,1},
                    {"center",PEVM_ReSolid,"?o",PE_generic,NULL,1},
                    {"center",PEVM_ReSolid,"?O",PE_generic,NULL,1},
                    {"center",PEVM_ReSolid,"?c",PE_generic,NULL,1},

                    {"set_personalname",PEVM_SetPName,"o=s",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"o=S",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"o=P",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"o=a",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"O=s",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"O=S",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"O=P",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"O=a",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"c=s",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"c=S",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"c=P",PE_generic,NULL,2},
                    {"set_personalname",PEVM_SetPName,"c=a",PE_generic,NULL,2},

                    {"erase_property",PEVM_DelProp,"o",PE_generic,NULL,1},
                    {"erase_property",PEVM_DelProp,"O",PE_generic,NULL,1},
                    {"erase_property",PEVM_DelProp,"c",PE_generic,NULL,1},

                    {"qupdate",PEVM_QUpdate,"o",PE_generic,NULL,1},
                    {"qupdate",PEVM_QUpdate,"O",PE_generic,NULL,1},
                    {"qupdate",PEVM_QUpdate,"c",PE_generic,NULL,1},
                    {"update_quantity",PEVM_QUpdate,"o",PE_generic,NULL,1},
                    {"update_quantity",PEVM_QUpdate,"O",PE_generic,NULL,1},
                    {"update_quantity",PEVM_QUpdate,"c",PE_generic,NULL,1},

                    {"mouse_range",PEVM_AddRange,"n=nnnn",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=nnni",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=nnin",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=nnii",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=ninn",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=nini",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=niin",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=niii",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=innn",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=inni",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=inin",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=inii",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=iinn",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=iini",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=iiin",PE_generic,NULL,5},
                    {"mouse_range",PEVM_AddRange,"n=iiii",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=nnss",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=nnsP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=nnPs",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=nnPP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=niss",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=nisP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=niPs",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=niPP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=inss",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=insP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=inPs",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=inPP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=iiss",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=iisP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=iiPs",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"n=iiPP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=nnss",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=nnsP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=nnPs",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=nnPP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=niss",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=nisP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=niPs",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=niPP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=inss",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=insP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=inPs",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=inPP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=iiss",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=iisP",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=iiPs",PE_generic,NULL,5},
                    {"button",PEVM_AddButton,"i=iiPP",PE_generic,NULL,5},
                    {"button_push",PEVM_PushButton,"n=n",PE_generic,NULL,2},
                    {"button_push",PEVM_PushButton,"n=i",PE_generic,NULL,2},
                    {"button_push",PEVM_PushButton,"i=n",PE_generic,NULL,2},
                    {"button_push",PEVM_PushButton,"i=i",PE_generic,NULL,2},
                    {"right_click",PEVM_RightClick,"n",PE_generic,NULL,1},
                    {"flush_mouse",PEVM_FlushMouse,"n",PE_generic,NULL,1},
                    {"mouse_grid",PEVM_GridRange,"n=nn",PE_generic,NULL,3},
                    {"mouse_grid",PEVM_GridRange,"n=n?n",PE_generic,NULL,3},
                    {"range_pointer",PEVM_RangePointer,"n=n",PE_generic,NULL,2},
                    {"textbutton",PEVM_TextButton,"n=nns",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"n=nnP",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"n=nis",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"n=niP",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"n=ins",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"n=inP",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"n=iis",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"n=iiP",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"i=nns",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"i=nnP",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"i=nis",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"i=niP",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"i=ins",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"i=inP",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"i=iis",PE_generic,NULL,4},
                    {"textbutton",PEVM_TextButton,"i=iiP",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"n=nns",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"n=nnP",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"n=nis",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"n=niP",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"n=ins",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"n=inP",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"n=iis",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"n=iiP",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"i=nns",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"i=nnP",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"i=nis",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"i=niP",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"i=ins",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"i=inP",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"i=iis",PE_generic,NULL,4},
                    {"text_button",PEVM_TextButton,"i=iiP",PE_generic,NULL,4},
                    {"save_screen",PEVM_SaveScreen,"",PE_generic,NULL,0},
                    {"store_screen",PEVM_SaveScreen,"",PE_generic,NULL,0},
                    {"restore_screen",PEVM_RestoreScreen,"",PE_generic,NULL,0},
                    {"load_screen",PEVM_RestoreScreen,"",PE_generic,NULL,0},
                    {"stopsearch",PEVM_StopSearch,"",PE_generic,NULL,0},
                    {"stop_search",PEVM_StopSearch,"",PE_generic,NULL,0},

                    {"createwindow",PEVM_CreateConsole,"i=nnnn",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=nnni",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=nnin",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=nnii",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=ninn",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=nini",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=niin",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=niii",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=innn",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=inni",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=inin",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=inii",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=iinn",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=iini",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=iiin",PE_generic,NULL,5},
                    {"createwindow",PEVM_CreateConsole,"i=iiii",PE_generic,NULL,5},
                    {"destroywindow",PEVM_DestroyConsole,"n",PE_generic,NULL,1},
                    {"destroywindow",PEVM_DestroyConsole,"i",PE_generic,NULL,1},
                    {"clearwindow",PEVM_ClearConsole,"n",PE_generic,NULL,1},
                    {"clearwindow",PEVM_ClearConsole,"i",PE_generic,NULL,1},
                    {"windowcolour",PEVM_ConsoleColour,"nn",PE_generic,NULL,2},
                    {"windowcolour",PEVM_ConsoleColour,"ni",PE_generic,NULL,2},
                    {"windowcolour",PEVM_ConsoleColour,"in",PE_generic,NULL,2},
                    {"windowcolour",PEVM_ConsoleColour,"ii",PE_generic,NULL,2},
                    {"addwindowline",PEVM_ConsoleAddLine,"ns",PE_generic,NULL,2},
                    {"addwindowline",PEVM_ConsoleAddLine,"nS",PE_generic,NULL,2},
                    {"addwindowline",PEVM_ConsoleAddLine,"nP",PE_generic,NULL,2},
                    {"addwindowline",PEVM_ConsoleAddLine,"nb",PE_generic,NULL,2},
                    {"addwindowline",PEVM_ConsoleAddLine,"is",PE_generic,NULL,2},
                    {"addwindowline",PEVM_ConsoleAddLine,"iS",PE_generic,NULL,2},
                    {"addwindowline",PEVM_ConsoleAddLine,"iP",PE_generic,NULL,2},
                    {"addwindowline",PEVM_ConsoleAddLine,"ib",PE_generic,NULL,2},
                    {"addwindowlinewrap",PEVM_ConsoleAddLineWrap,"ns",PE_generic,NULL,2},
                    {"addwindowlinewrap",PEVM_ConsoleAddLineWrap,"nS",PE_generic,NULL,2},
                    {"addwindowlinewrap",PEVM_ConsoleAddLineWrap,"nP",PE_generic,NULL,2},
                    {"addwindowlinewrap",PEVM_ConsoleAddLineWrap,"nb",PE_generic,NULL,2},
                    {"addwindowlinewrap",PEVM_ConsoleAddLineWrap,"is",PE_generic,NULL,2},
                    {"addwindowlinewrap",PEVM_ConsoleAddLineWrap,"iS",PE_generic,NULL,2},
                    {"addwindowlinewrap",PEVM_ConsoleAddLineWrap,"iP",PE_generic,NULL,2},
                    {"addwindowlinewrap",PEVM_ConsoleAddLineWrap,"ib",PE_generic,NULL,2},
                    {"drawwindow",PEVM_DrawConsole,"n",PE_generic,NULL,1},
                    {"drawwindow",PEVM_DrawConsole,"i",PE_generic,NULL,1},
                    {"scrollwindow",PEVM_ScrollConsole,"nn",PE_generic,NULL,2},
                    {"scrollwindow",PEVM_ScrollConsole,"ni",PE_generic,NULL,2},
                    {"scrollwindow",PEVM_ScrollConsole,"in",PE_generic,NULL,2},
                    {"scrollwindow",PEVM_ScrollConsole,"ii",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"S=s",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"S=S",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"S=P",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"S=b",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"P=s",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"P=S",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"P=P",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"P=b",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"b=s",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"b=S",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"b=P",PE_generic,NULL,2},
                    {"findstring",PEVM_FindString,"b=b",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"S=s",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"S=S",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"S=P",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"S=b",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"P=s",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"P=S",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"P=P",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"P=b",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"b=s",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"b=S",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"b=P",PE_generic,NULL,2},
                    {"findtitle",PEVM_FindTitle,"b=b",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal,"s",PE_generic,NULL,1},
                    {"journal",PEVM_AddJournal,"S",PE_generic,NULL,1},
                    {"journal",PEVM_AddJournal,"P",PE_generic,NULL,1},
                    {"journal",PEVM_AddJournal,"b",PE_generic,NULL,1},
                    {"journal",PEVM_AddJournal2,"ss",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"Ss",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"Ps",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"bs",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"sS",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"SS",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"PS",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"bS",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"sP",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"SP",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"PP",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"bP",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"sb",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"Sb",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"Pb",PE_generic,NULL,2},
                    {"journal",PEVM_AddJournal2,"bb",PE_generic,NULL,2},
                    {"journaldone",PEVM_EndJournal,"s",PE_generic,NULL,1},
                    {"journaldone",PEVM_EndJournal,"S",PE_generic,NULL,1},
                    {"journaldone",PEVM_EndJournal,"P",PE_generic,NULL,1},
                    {"journaldone",PEVM_EndJournal,"b",PE_generic,NULL,1},
                    {"journal_done",PEVM_EndJournal,"s",PE_generic,NULL,1},
                    {"journal_done",PEVM_EndJournal,"S",PE_generic,NULL,1},
                    {"journal_done",PEVM_EndJournal,"P",PE_generic,NULL,1},
                    {"journal_done",PEVM_EndJournal,"b",PE_generic,NULL,1},
                    {"finish_journal",PEVM_EndJournal,"s",PE_generic,NULL,1},
                    {"finish_journal",PEVM_EndJournal,"S",PE_generic,NULL,1},
                    {"finish_journal",PEVM_EndJournal,"P",PE_generic,NULL,1},
                    {"finish_journal",PEVM_EndJournal,"b",PE_generic,NULL,1},
                    {"drawjournal",PEVM_DrawJournal,"n",PE_generic,NULL,1},
                    {"drawjournal",PEVM_DrawJournal,"i",PE_generic,NULL,1},
                    {"drawjournalday",PEVM_DrawJournalDay,"nn",PE_generic,NULL,2},
                    {"drawjournalday",PEVM_DrawJournalDay,"ni",PE_generic,NULL,2},
                    {"drawjournalday",PEVM_DrawJournalDay,"nI",PE_generic,NULL,2},
                    {"drawjournalday",PEVM_DrawJournalDay,"na",PE_generic,NULL,2},
                    {"drawjournalday",PEVM_DrawJournalDay,"in",PE_generic,NULL,2},
                    {"drawjournalday",PEVM_DrawJournalDay,"ii",PE_generic,NULL,2},
                    {"drawjournalday",PEVM_DrawJournalDay,"iI",PE_generic,NULL,2},
                    {"drawjournalday",PEVM_DrawJournalDay,"ia",PE_generic,NULL,2},
                    {"drawjournaltasks",PEVM_DrawJournalTasks,"nn",PE_generic,NULL,2},
                    {"drawjournaltasks",PEVM_DrawJournalTasks,"ni",PE_generic,NULL,2},
                    {"drawjournaltasks",PEVM_DrawJournalTasks,"nI",PE_generic,NULL,2},
                    {"drawjournaltasks",PEVM_DrawJournalTasks,"na",PE_generic,NULL,2},
                    {"drawjournaltasks",PEVM_DrawJournalTasks,"in",PE_generic,NULL,2},
                    {"drawjournaltasks",PEVM_DrawJournalTasks,"ii",PE_generic,NULL,2},
                    {"drawjournaltasks",PEVM_DrawJournalTasks,"iI",PE_generic,NULL,2},
                    {"drawjournaltasks",PEVM_DrawJournalTasks,"ia",PE_generic,NULL,2},
                    {"getjournaldays",PEVM_GetJournalDays,"i",PE_generic,NULL,1},
                    {"getjournaldays",PEVM_GetJournalDays,"I",PE_generic,NULL,1},
                    {"getjournalday",PEVM_GetJournalDay,"i=n",PE_generic,NULL,2},
                    {"getjournalday",PEVM_GetJournalDay,"i=i",PE_generic,NULL,2},
                    {"getjournalday",PEVM_GetJournalDay,"i=I",PE_generic,NULL,2},
                    {"getjournalday",PEVM_GetJournalDay,"I=n",PE_generic,NULL,2},
                    {"getjournalday",PEVM_GetJournalDay,"I=i",PE_generic,NULL,2},
                    {"getjournalday",PEVM_GetJournalDay,"I=I",PE_generic,NULL,2},
                    {"setwindowline",PEVM_SetConsoleLine,"nn",PE_generic,NULL,2},
                    {"setwindowline",PEVM_SetConsoleLine,"ni",PE_generic,NULL,2},
                    {"setwindowline",PEVM_SetConsoleLine,"in",PE_generic,NULL,2},
                    {"setwindowline",PEVM_SetConsoleLine,"ii",PE_generic,NULL,2},
                    {"setwindowpercent",PEVM_SetConsolePercent,"nn",PE_generic,NULL,2},
                    {"setwindowpercent",PEVM_SetConsolePercent,"ni",PE_generic,NULL,2},
                    {"setwindowpercent",PEVM_SetConsolePercent,"in",PE_generic,NULL,2},
                    {"setwindowpercent",PEVM_SetConsolePercent,"ii",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelect,"nn",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelect,"ni",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelect,"in",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelect,"ii",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelectS,"ns",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelectS,"nS",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelectS,"nP",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelectS,"nb",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelectS,"is",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelectS,"iS",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelectS,"iP",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelectS,"ib",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelectU,"nU",PE_generic,NULL,2},
                    {"setwindowselect",PEVM_SetConsoleSelectU,"iU",PE_generic,NULL,2},
                    {"getwindowline",PEVM_GetConsoleLine,"i=n",PE_generic,NULL,2},
                    {"getwindowline",PEVM_GetConsoleLine,"i=i",PE_generic,NULL,2},
                    {"getwindowpercent",PEVM_GetConsolePercent,"i=n",PE_generic,NULL,2},
                    {"getwindowpercent",PEVM_GetConsolePercent,"i=i",PE_generic,NULL,2},
                    {"getwindowselect",PEVM_GetConsoleSelect,"i=n",PE_generic,NULL,2},
                    {"getwindowselect",PEVM_GetConsoleSelect,"i=i",PE_generic,NULL,2},
                    {"getwindowselect",PEVM_GetConsoleSelectU,"U=n",PE_generic,NULL,2},
                    {"getwindowselect",PEVM_GetConsoleSelectU,"U=i",PE_generic,NULL,2},
                    {"getwindowlines",PEVM_GetConsoleLines,"i=n",PE_generic,NULL,2},
                    {"getwindowlines",PEVM_GetConsoleLines,"i=i",PE_generic,NULL,2},
                    {"getwindowrows",PEVM_GetConsoleRows,"i=n",PE_generic,NULL,2},
                    {"getwindowrows",PEVM_GetConsoleRows,"i=i",PE_generic,NULL,2},

		    
                    {"createlist",PEVM_CreatePicklist,"i=nnnn",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=nnni",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=nnin",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=nnii",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=ninn",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=nini",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=niin",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=niii",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=innn",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=inni",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=inin",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=inii",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=iinn",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=iini",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=iiin",PE_generic,NULL,5},
                    {"createlist",PEVM_CreatePicklist,"i=iiii",PE_generic,NULL,5},
                    {"destroylist",PEVM_DestroyPicklist,"n",PE_generic,NULL,1},
                    {"destroylist",PEVM_DestroyPicklist,"i",PE_generic,NULL,1},
                    {"listcolour",PEVM_PicklistColour,"nn",PE_generic,NULL,2},
                    {"listcolour",PEVM_PicklistColour,"ni",PE_generic,NULL,2},
                    {"listcolour",PEVM_PicklistColour,"in",PE_generic,NULL,2},
                    {"listcolour",PEVM_PicklistColour,"ii",PE_generic,NULL,2},
                    {"listcolour",PEVM_PicklistColour,"ii",PE_generic,NULL,2},
                    {"listpoll",PEVM_PicklistPoll,"n",PE_generic,NULL,2},
                    {"listpoll",PEVM_PicklistPoll,"i",PE_generic,NULL,2},
                    {"polllist",PEVM_PicklistPoll,"n",PE_generic,NULL,2},
                    {"polllist",PEVM_PicklistPoll,"n",PE_generic,NULL,2},
                    {"addlistline",PEVM_PicklistAddLine,"nns",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"nnS",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"nnP",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"nis",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"niS",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"niP",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"ins",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"inS",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"inP",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"iis",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"iiS",PE_generic,NULL,3},
                    {"addlistline",PEVM_PicklistAddLine,"iiP",PE_generic,NULL,3},
                    {"addlistsave",PEVM_PicklistAddSave,"nn",PE_generic,NULL,2},
                    {"addlistsave",PEVM_PicklistAddSave,"ni",PE_generic,NULL,2},
                    {"addlistsave",PEVM_PicklistAddSave,"in",PE_generic,NULL,2},
                    {"addlistsave",PEVM_PicklistAddSave,"ii",PE_generic,NULL,2},
                    {"getlistitem",PEVM_PicklistItem,"i=n",PE_generic,NULL,2},
                    {"getlistitem",PEVM_PicklistItem,"i=i",PE_generic,NULL,2},
                    {"getlistid",PEVM_PicklistId,"i=n",PE_generic,NULL,2},
                    {"getlistid",PEVM_PicklistId,"i=i",PE_generic,NULL,2},
                    {"getlisttext",PEVM_PicklistText,"U=n",PE_generic,NULL,2},
                    {"getlisttext",PEVM_PicklistText,"U=i",PE_generic,NULL,2},
                    {"setlistpercent",PEVM_SetPicklistPercent,"nn",PE_generic,NULL,2},
                    {"setlistpercent",PEVM_SetPicklistPercent,"ni",PE_generic,NULL,2},
                    {"setlistpercent",PEVM_SetPicklistPercent,"in",PE_generic,NULL,2},
                    {"setlistpercent",PEVM_SetPicklistPercent,"ii",PE_generic,NULL,2},
                    {"getlistpercent",PEVM_GetPicklistPercent,"i=n",PE_generic,NULL,2},
                    {"getlistpercent",PEVM_GetPicklistPercent,"i=i",PE_generic,NULL,2},
		    
                    {"fx_getxy",PEVM_fxGetXY,"ii=o",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"ii=O",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"ii=c",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"iI=o",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"iI=O",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"iI=c",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"ia=o",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"ia=O",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"ia=c",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"Ii=o",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"Ii=O",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"Ii=c",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"II=o",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"II=O",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"II=c",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"Ia=o",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"Ia=O",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"Ia=c",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"ai=o",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"ai=O",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"ai=c",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"aI=o",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"aI=O",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"aI=c",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"aa=o",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"aa=O",PE_generic,NULL,3},
                    {"fx_getxy",PEVM_fxGetXY,"aa=c",PE_generic,NULL,3},

                    {"fx_colour",PEVM_fxColour,"n",PE_generic,NULL,1},
                    {"fx_colour",PEVM_fxColour,"i",PE_generic,NULL,1},
                    {"fx_colour",PEVM_fxColour,"I",PE_generic,NULL,1},
                    {"fx_color",PEVM_fxColour,"n",PE_generic,NULL,1},
                    {"fx_color",PEVM_fxColour,"i",PE_generic,NULL,1},
                    {"fx_color",PEVM_fxColour,"I",PE_generic,NULL,1},
                    {"fx_alpha",PEVM_fxAlpha,"n",PE_generic,NULL,1},
                    {"fx_alpha",PEVM_fxAlpha,"i",PE_generic,NULL,1},
                    {"fx_alpha",PEVM_fxAlpha,"I",PE_generic,NULL,1},
                    {"fx_alpha",PEVM_fxAlpha,"a",PE_generic,NULL,1},
                    {"fx_alphamode",PEVM_fxAlphaMode,"n",PE_generic,NULL,1},
                    {"fx_random",PEVM_fxRandom,"i=n",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"i=i",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"i=I",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"i=a",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"I=n",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"I=i",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"I=I",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"I=a",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"a=n",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"a=i",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"a=I",PE_generic,NULL,2},
                    {"fx_random",PEVM_fxRandom,"a=a",PE_generic,NULL,2},
                    {"fx_line",PEVM_fxLine,"",PE_generic,NULL,0},
                    {"fx_point",PEVM_fxPoint,"",PE_generic,NULL,0},
                    {"fx_rect",PEVM_fxRect,"",PE_generic,NULL,0},
                    {"fx_poly",PEVM_fxPoly,"a",PE_generic,NULL,1},
                    {"fx_orbit",PEVM_fxOrbit,"i",PE_generic,NULL,1},
                    {"fx_orbit",PEVM_fxOrbit,"I",PE_generic,NULL,1},
                    {"fx_orbit",PEVM_fxOrbit,"a",PE_generic,NULL,1},
                    {"fx_sprite",PEVM_fxSprite,"",PE_generic,NULL,0},
                    {"fx_corona",PEVM_fxCorona,"n",PE_generic,NULL,1},
                    {"fx_corona",PEVM_fxCorona,"i",PE_generic,NULL,1},
                    {"fx_corona",PEVM_fxCorona,"I",PE_generic,NULL,1},
                    {"fx_corona",PEVM_fxCorona,"a",PE_generic,NULL,1},
                    {"fx_blackcorona",PEVM_fxBlackCorona,"n",PE_generic,NULL,1},
                    {"fx_blackcorona",PEVM_fxBlackCorona,"i",PE_generic,NULL,1},
                    {"fx_blackcorona",PEVM_fxBlackCorona,"I",PE_generic,NULL,1},
                    {"fx_blackcorona",PEVM_fxBlackCorona,"a",PE_generic,NULL,1},
                    {"fx_animsprite",PEVM_fxAnimSprite,"",PE_generic,NULL,0},
                    {"start_fx",PEVM_dofx,"o?f",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofx,"O?f",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofx,"c?f",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofxS,"o?s",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofxS,"O?s",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofxS,"c?s",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofxS,"o?S",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofxS,"O?S",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofxS,"c?S",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofxS,"o?P",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofxS,"O?P",PE_generic,NULL,2},
                    {"start_fx",PEVM_dofxS,"c?P",PE_generic,NULL,2},
                    {"startfx",PEVM_dofx,"o?f",PE_generic,NULL,2},
                    {"startfx",PEVM_dofx,"O?f",PE_generic,NULL,2},
                    {"startfx",PEVM_dofx,"c?f",PE_generic,NULL,2},
                    {"startfx",PEVM_dofxS,"o?s",PE_generic,NULL,2},
                    {"startfx",PEVM_dofxS,"O?s",PE_generic,NULL,2},
                    {"startfx",PEVM_dofxS,"c?s",PE_generic,NULL,2},
                    {"startfx",PEVM_dofxS,"o?S",PE_generic,NULL,2},
                    {"startfx",PEVM_dofxS,"O?S",PE_generic,NULL,2},
                    {"startfx",PEVM_dofxS,"c?S",PE_generic,NULL,2},
                    {"startfx",PEVM_dofxS,"o?P",PE_generic,NULL,2},
                    {"startfx",PEVM_dofxS,"O?P",PE_generic,NULL,2},
                    {"startfx",PEVM_dofxS,"c?P",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOver,"o?f",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOver,"O?f",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOver,"c?f",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOverS,"o?s",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOverS,"O?s",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOverS,"c?s",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOverS,"o?S",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOverS,"O?S",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOverS,"c?S",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOverS,"o?P",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOverS,"O?P",PE_generic,NULL,2},
                    {"over_fx",PEVM_dofxOverS,"c?P",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOver,"o?f",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOver,"O?f",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOver,"c?f",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOverS,"o?s",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOverS,"O?s",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOverS,"c?s",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOverS,"o?S",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOverS,"O?S",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOverS,"c?S",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOverS,"o?P",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOverS,"O?P",PE_generic,NULL,2},
                    {"overfx",PEVM_dofxOverS,"c?P",PE_generic,NULL,2},
                    {"stop_fx",PEVM_stopfx,"o",PE_generic,NULL,1},
                    {"stop_fx",PEVM_stopfx,"O",PE_generic,NULL,1},
                    {"stop_fx",PEVM_stopfx,"c",PE_generic,NULL,1},
                    {"stopfx",PEVM_stopfx,"o",PE_generic,NULL,1},
                    {"stopfx",PEVM_stopfx,"O",PE_generic,NULL,1},
                    {"stopfx",PEVM_stopfx,"c",PE_generic,NULL,1},

                    {"set_string",PEVM_setstr,"U=s",PE_generic,NULL,2},
                    {"set_string",PEVM_setstr,"U=S",PE_generic,NULL,2},
                    {"set_string",PEVM_setstr,"U=P",PE_generic,NULL,2},
                    {"setstr",PEVM_setstr,"U=s",PE_generic,NULL,2},
                    {"setstr",PEVM_setstr,"U=S",PE_generic,NULL,2},
                    {"setstr",PEVM_setstr,"U=P",PE_generic,NULL,2},

                    {"add_string",PEVM_addstr,"U=s",PE_generic,NULL,2},
                    {"add_string",PEVM_addstr,"U=S",PE_generic,NULL,2},
                    {"add_string",PEVM_addstr,"U=P",PE_generic,NULL,2},
                    {"add_string",PEVM_addstrn,"U=n",PE_generic,NULL,2},
                    {"add_string",PEVM_addstrn,"U=i",PE_generic,NULL,2},
                    {"add_string",PEVM_addstrn,"U=I",PE_generic,NULL,2},
                    {"add_string",PEVM_addstrn,"U=a",PE_generic,NULL,2},
                    {"addstr",PEVM_addstr,"U=s",PE_generic,NULL,2},
                    {"addstr",PEVM_addstr,"U=S",PE_generic,NULL,2},
                    {"addstr",PEVM_addstr,"U=P",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen,"i=U",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen,"I=U",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen,"a=U",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"i=s",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"I=s",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"a=s",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"i=S",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"I=S",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"a=S",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"i=P",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"I=P",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"a=P",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"i=b",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"I=b",PE_generic,NULL,2},
                    {"strlen",PEVM_strlen2,"a=b",PE_generic,NULL,2},
                    {"strsetpos",PEVM_strsetpos,"Un=n",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Un=i",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Un=I",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Un=a",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Ui=n",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Ui=i",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Ui=I",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Ui=a",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"UI=n",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"UI=i",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"UI=I",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"UI=a",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Ua=n",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Ua=i",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Ua=I",PE_generic,NULL,3},
                    {"strsetpos",PEVM_strsetpos,"Ua=a",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"i=Un",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"i=Ui",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"i=UI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"i=Ua",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"I=Un",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"I=Ui",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"I=UI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"I=Ua",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"a=Un",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"a=Ui",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"a=UI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos,"a=Ua",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=sn",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=si",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=sI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=sa",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=sn",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=si",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=sI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=sa",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=sn",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=si",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=sI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=sa",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=Sn",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=Si",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=SI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=Sa",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=Sn",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=Si",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=SI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=Sa",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=Sn",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=Si",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=SI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=Sa",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=Pn",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=Pi",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=PI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"i=Pa",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=Pn",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=Pi",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=PI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"I=Pa",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=Pn",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=Pi",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=PI",PE_generic,NULL,3},
                    {"strgetpos",PEVM_strgetpos2,"a=Pa",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Un=n",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Un=i",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Un=I",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Un=a",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Ui=n",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Ui=i",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Ui=I",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Ui=a",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"UI=n",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"UI=i",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"UI=I",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"UI=a",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Ua=n",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Ua=i",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Ua=I",PE_generic,NULL,3},
                    {"str_setpos",PEVM_strsetpos,"Ua=a",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"i=Un",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"i=Ui",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"i=UI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"i=Ua",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"I=Un",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"I=Ui",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"I=UI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"I=Ua",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"a=Un",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"a=Ui",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"a=UI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos,"a=Ua",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=sn",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=si",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=sI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=sa",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=sn",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=si",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=sI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=sa",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=sn",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=si",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=sI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=sa",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=Sn",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=Si",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=SI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=Sa",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=Sn",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=Si",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=SI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=Sa",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=Sn",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=Si",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=SI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=Sa",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=Pn",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=Pi",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=PI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"i=Pa",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=Pn",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=Pi",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=PI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"I=Pa",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=Pn",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=Pi",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=PI",PE_generic,NULL,3},
                    {"str_getpos",PEVM_strgetpos2,"a=Pa",PE_generic,NULL,3},
                    {"getval",PEVM_strgetval,"i=s",PE_generic,NULL,2},
                    {"getval",PEVM_strgetval,"i=S",PE_generic,NULL,2},
                    {"getval",PEVM_strgetval,"i=P",PE_generic,NULL,2},
                    {"getval",PEVM_strgetval,"I=s",PE_generic,NULL,2},
                    {"getval",PEVM_strgetval,"I=S",PE_generic,NULL,2},
                    {"getval",PEVM_strgetval,"I=P",PE_generic,NULL,2},
                    {"getval",PEVM_strgetval,"a=s",PE_generic,NULL,2},
                    {"getval",PEVM_strgetval,"a=S",PE_generic,NULL,2},
                    {"getval",PEVM_strgetval,"a=P",PE_generic,NULL,2},

                    {"if_key",PEVM_IfKey,"n",PE_if,PEP_if,2},
                    {"if_key",PEVM_IfKey,"i",PE_if,PEP_if,2},
                    {"if_key",PEVM_IfKey,"I",PE_if,PEP_if,2},
                    {"if_keypressed",PEVM_IfKeyPressed,"",PE_if,PEP_if,1},
		    
/*
                    {"get_ascii",PEVM_getascii,"i=s",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"I=s",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"a=s",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"i=S",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"I=S",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"a=S",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"i=P",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"I=P",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"a=P",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"i=U",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"I=U",PE_generic,NULL,2},
                    {"get_ascii",PEVM_getascii,"a=U",PE_generic,NULL,2},
*/
                    {"getvolume",PEVM_getvolume,"i=n",PE_generic,NULL,2},
                    {"getvolume",PEVM_getvolume,"i=i",PE_generic,NULL,2},
                    {"getvolume",PEVM_getvolume,"i=I",PE_generic,NULL,2},
                    {"getvolume",PEVM_getvolume,"i=a",PE_generic,NULL,2},
                    {"getvolume",PEVM_getvolume,"a=n",PE_generic,NULL,2},
                    {"getvolume",PEVM_getvolume,"a=i",PE_generic,NULL,2},
                    {"getvolume",PEVM_getvolume,"a=I",PE_generic,NULL,2},
                    {"getvolume",PEVM_getvolume,"a=a",PE_generic,NULL,2},
                    {"setvolume",PEVM_setvolume,"n?n",PE_generic,NULL,2},
                    {"setvolume",PEVM_setvolume,"n?i",PE_generic,NULL,2},
                    {"setvolume",PEVM_setvolume,"n?I",PE_generic,NULL,2},
                    {"setvolume",PEVM_setvolume,"n?a",PE_generic,NULL,2},
                    {"setvolume",PEVM_setvolume,"i?n",PE_generic,NULL,2},
                    {"setvolume",PEVM_setvolume,"i?i",PE_generic,NULL,2},
                    {"setvolume",PEVM_setvolume,"i?I",PE_generic,NULL,2},
                    {"setvolume",PEVM_setvolume,"i?a",PE_generic,NULL,2},
                    {"lastsave",PEVM_LastSaveSlot,"i",PE_generic,NULL,1},
                    {"savegame",PEVM_SaveGame,"nU",PE_generic,NULL,2},
                    {"savegame",PEVM_SaveGame,"iU",PE_generic,NULL,2},
                    {"savegame",PEVM_SaveGame,"aU",PE_generic,NULL,2},
                    {"loadgame",PEVM_LoadGame,"n",PE_generic,NULL,1},
                    {"loadgame",PEVM_LoadGame,"i",PE_generic,NULL,1},
                    {"loadgame",PEVM_LoadGame,"a",PE_generic,NULL,1},
                    {"getdate",PEVM_GetDate,"U",PE_generic,NULL,1},
                    {"assert",PEVM_Assert,"npn",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"npi",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"npI",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"npa",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"ipn",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"ipi",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"ipI",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"ipa",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"Ipn",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"Ipi",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"IpI",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"Ipa",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"apn",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"api",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"apI",PE_assert,NULL,4},
                    {"assert",PEVM_Assert,"apa",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"sps",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"spS",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"spP",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"spU",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"spb",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"Sps",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"SpS",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"SpP",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"SpU",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"Spb",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"Pps",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"PpS",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"PpP",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"PpU",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"Ppb",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"Ups",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"UpS",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"Upp",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"UpU",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"Upb",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"bps",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"bpS",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"bpp",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"bpU",PE_assert,NULL,4},
                    {"assert",PEVM_AssertStr,"bpb",PE_assert,NULL,4},
                    {"status",PEVM_LastOp,"n",PE_generic,NULL,1},
                    {"get_funcname",PEVM_GetFuncP,"P=i",PE_generic,NULL,2},
                    {"get_funcname",PEVM_GetFuncP,"P=I",PE_generic,NULL,2},
                    {"no_operation",PEVM_NOP,"",PE_generic,NULL,0},
                    {"nop",PEVM_NOP,"",PE_generic,NULL,0},
                    {"fixup_locals",PEVM_Dofus,"",NULL,NULL,-1},
                    {NULL,0,NULL,NULL,0}
					// READ AND OBEY
					// * OVERLOADED KEYWORDS *MUST* BE CONSECUTIVE IN THE LIST!
					// * 'IF' STATEMENTS HAVE ONE EXTRA HIDDEN OPERAND
               };

// Game API structure definitions
// With overloaded members, e.g. object.enemy, where the member is an Object,
// an Integer and a redirection to another structure definition, the
// Redirection MUST be LAST, i.e. after the Object and Integer entries..
// The Type of the first entry must be set if the structure can exist as
// a separate variable, e.g. a Tile or an Object.  It is used by the
// structure member lookup engine.

STRUCTURE objspec[] =
					{
					{"object",		'o',"",&obj_template,NULL},
					{"uid",			's',"R",&obj_template.uidptr,NULL},	// Absolutely not writable
					{"name",		's',"R",&obj_template.name,NULL},
					{"flags",		'i',"R",&obj_template.flags,NULL},
					{"w",			'i',"R",&obj_template.w,NULL},
					{"h",			'i',"R",&obj_template.h,NULL},
					{"mw",			'i',"R",&obj_template.mw,NULL},
					{"mh",			'i',"R",&obj_template.mh,NULL},
					{"x",			'i',"RW",&obj_template.x,NULL},	// We do kind of need x and y for the 'focus' spell
					{"y",			'i',"RW",&obj_template.y,NULL},
					{"z",			'i',"R",&obj_template.z,NULL},
					{"cost",		'i',"R",&obj_template.cost,NULL},
					{"personalname",'s',"R",&obj_template.personalname,NULL},
					{"schedule",	' ',"",&obj_template.schedule,NULL},
					{"form",		' ',"",&obj_template.form,NULL},
					{"sptr",		'i',"RW",&obj_template.sptr,NULL},
					{"sdir",		'i',"R",&obj_template.sdir,NULL},
					{"maxstats",	'>',"",&obj_template.maxstats,&statspec},
					{"stats",		'>',"",&obj_template.stats,&statspec},
					{"funcs",		'>',"",&obj_template.funcs,&funcspec},
					{"curdir",		'i',"RW",&obj_template.curdir,NULL},
					{"desc",		's',"R",&obj_template.desc,NULL},
					{"shortdesc",	's',"R",&obj_template.shortdesc,NULL},
					{"target",		'o',"RW",&obj_template.target,NULL},
					{"target",		'i',"RW",&obj_template.target,NULL},
					{"target",		'>',"R",&obj_template.target,&objspec},
					{"tag",			'i',"RW",&obj_template.tag,NULL},
					{"activity",	'i',"RW",&obj_template.activity,NULL},
					{"user",		'i',"RW",&obj_template.user,NULL},
					{"user",		'>',"R",&obj_template.user,&userspec},
					{"light",		'i',"RW",&obj_template.light,NULL},
					{"enemy",		'o',"RW",&obj_template.enemy,NULL},
					{"enemy",		'i',"RW",&obj_template.enemy,NULL},
					{"enemy",		'>',"R",&obj_template.enemy,&objspec},
					{"wield",		'i',"RW",&obj_template.wield,&wieldspec},
					{"wield",		'>',"R",&obj_template.wield,&wieldspec},
					{"pocket",		'o',"R",&obj_template.pocket,NULL},
					{"pocket",		'i',"R",&obj_template.pocket,NULL},
					{"pocket",		'>',"R",&obj_template.pocket,&objspec},
					{"parent",		'o',"R",&obj_template.parent,NULL},
					{"parent",		'i',"R",&obj_template.parent,NULL},
					{"parent",		'>',"R",&obj_template.parent,&objspec},
					{"next",		'o',"R",&obj_template.next,NULL},
					{"next",		'i',"R",&obj_template.next,NULL},
					{"next",		'>',"R",&obj_template.next,&objspec},
					{"label",		'i',"RW",&obj_template.labels,&labelspec},
					{"label",		'>',"R",&obj_template.labels,&labelspec},
					{NULL,			0,NULL,NULL,NULL},
					};

STRUCTURE intarrayhack[] =
					{
					{"integer",		'a',"",NULL,NULL},
					{NULL,			0,NULL,NULL,NULL},
					};

STRUCTURE strarrayhack[] =
					{
					{"string",		'b',"",NULL,NULL},
					{NULL,			0,NULL,NULL,NULL},
					};

STRUCTURE objarrayhack[] =
					{
					{"object",		'c',"",&obj_template,NULL},
					{"name",		's',"R",&obj_template.name,NULL},
					{"flags",		'i',"R",&obj_template.flags,NULL},
					{"w",			'i',"R",&obj_template.w,NULL},
					{"h",			'i',"R",&obj_template.h,NULL},
					{"mw",			'i',"R",&obj_template.mw,NULL},
					{"mh",			'i',"R",&obj_template.mh,NULL},
					{"x",			'i',"R",&obj_template.x,NULL},
					{"y",			'i',"R",&obj_template.y,NULL},
					{"z",			'i',"R",&obj_template.z,NULL},
					{"personalname",'s',"R",&obj_template.personalname,NULL},
					{"schedule",	' ',"",&obj_template.schedule,NULL},
					{"form",		' ',"",&obj_template.form,NULL},
					{"sptr",		'i',"RW",&obj_template.sptr,NULL},
					{"sdir",		'i',"R",&obj_template.sdir,NULL},
					{"maxstats",	'>',"",&obj_template.maxstats,&statspec},
					{"stats",		'>',"",&obj_template.stats,&statspec},
					{"funcs",		'>',"",&obj_template.funcs,&funcspec},
					{"curdir",		'i',"RW",&obj_template.curdir,NULL},
					{"desc",		's',"R",&obj_template.desc,NULL},
					{"shortdesc",	's',"R",&obj_template.shortdesc,NULL},
					{"tag",			'i',"RW",&obj_template.tag,NULL},
					{"user",		'i',"RW",&obj_template.user,NULL},
					{"user",		'>',"R",&obj_template.user,&userspec},
					{"hotx",		'i',"R",&obj_template.hotx,NULL},
					{"hoty",		'i',"R",&obj_template.hoty,NULL},
					{"light",		'i',"RW",&obj_template.light,NULL},
					{"enemy",		'o',"RW",&obj_template.enemy,NULL},
					{"enemy",		'i',"RW",&obj_template.enemy,NULL},
					{"enemy",		'>',"R",&obj_template.enemy,&objspec},
					{"wield",		'i',"RW",&obj_template.wield,&wieldspec},
					{"wield",		'>',"R",&obj_template.wield,&wieldspec},
					{"pocket",		'o',"RW",&obj_template.pocket,NULL},
					{"pocket",		'i',"RW",&obj_template.pocket,NULL},
					{"pocket",		'>',"R",&obj_template.pocket,&objspec},
					{"parent",		'o',"RW",&obj_template.parent,NULL},
					{"parent",		'i',"RW",&obj_template.parent,NULL},
					{"parent",		'>',"R",&obj_template.parent,&objspec},
					{"next",		'o',"RW",&obj_template.next,NULL},
					{"next",		'i',"RW",&obj_template.next,NULL},
					{"next",		'>',"R",&obj_template.next,&objspec},
					{"label",		'i',"RW",&obj_template.labels,&labelspec},
					{"label",		'>',"R",&obj_template.labels,&labelspec},
					{NULL,			0,NULL,NULL,NULL},
					};

STRUCTURE tilespec[] =
					{
					{"tile",		't',"",&tile_template,NULL},
					{"name",		's',"R",&tile_template.name,NULL},
					{"flags",		'i',"R",&tile_template.flags,NULL},
					{"form",		' ',"",&tile_template.form,NULL},
					{"seqname",		' ',"",&tile_template.seqname,NULL},
					{"sptr",		'i',"R",&tile_template.sptr,NULL},
					{"sdir",		'i',"R",&tile_template.sdir,NULL},
					{"tick",		'i',"R",&tile_template.tick,NULL},
					{"sdx",			'i',"R",&tile_template.sdx,NULL},
					{"sdy",			'i',"R",&tile_template.sdy,NULL},
					{"sx",			'i',"R",&tile_template.sx,NULL},
					{"sy",			'i',"R",&tile_template.sy,NULL},
//					{"funcs",		'>',"",&tile_template.funcs,&funcspec},
					{"desc",		's',"R",&tile_template.desc,NULL},
					{NULL,			0,NULL,NULL,NULL},
					};

STRUCTURE statspec[] =
					{
					{"stats",		' ',"",&stats_template,NULL},
					{"hp",			'i',"RW",&stats_template.hp,NULL},
					{"dex",			'i',"RW",&stats_template.dex,NULL},
					{"dexterity",	'i',"RW",&stats_template.dex,NULL},
					{"str",			'i',"RW",&stats_template.str,NULL},
					{"strength",	'i',"RW",&stats_template.str,NULL},
					{"intel",		'i',"RW",&stats_template.intel,NULL},
					{"intelligence",'i',"RW",&stats_template.intel,NULL},
					{"weight",		'i',"RW",&stats_template.weight,NULL},
					{"quantity",	'i',"RW",&stats_template.quantity,NULL},
					{"npcflags",	'i',"R", &stats_template.npcflags,NULL},
					{"damage",		'i',"RW",&stats_template.damage,NULL},
					{"armour",		'i',"RW",&stats_template.armour,NULL},
					{"radius",		'i',"RW",&stats_template.radius,NULL},
					{"owner",		'o',"RW",&stats_template.owner,NULL},
					{"owner",		'>',"",&stats_template.owner,&objspec},
					{"karma",		'i',"RW",&stats_template.karma,NULL},
					{"bulk",		'i',"RW",&stats_template.bulk,NULL},
					{"range",		'i',"RW",&stats_template.range,NULL},
//					{"speed",		'i',"RW",&stats_template.speed,NULL},
					{"level",		'i',"RW",&stats_template.level,NULL},
					{"align",		'i',"RW",&stats_template.alignment,NULL},
					{"alignment",	'i',"RW",&stats_template.alignment,NULL},
					{NULL,			0,NULL,NULL,NULL},
					};

STRUCTURE funcspec[] =
					{
					{"funcs",		' ',"",&funcs_template,NULL},
					{"use",			's',"R",&funcs_template.suse,NULL},
					{"ucache",		'i',"RW",&funcs_template.ucache,NULL},
					{"talk",		's',"R",&funcs_template.stalk,NULL},
					{"tcache",		'i',"RW",&funcs_template.tcache,NULL},
					{"kill",		's',"R",&funcs_template.skill,NULL},
					{"kcache",		'i',"RW",&funcs_template.kcache,NULL},
					{"look",		's',"R",&funcs_template.slook,NULL},
					{"lcache",		'i',"RW",&funcs_template.lcache,NULL},
					{"stand",		's',"R",&funcs_template.sstand,NULL},
					{"scache",		'i',"RW",&funcs_template.scache,NULL},
					{"hurt",		's',"R",&funcs_template.shurt,NULL},
					{"hcache",		'i',"RW",&funcs_template.hcache,NULL},
					{"init",		's',"R",&funcs_template.sinit,NULL},
					{"icache",		'i',"RW",&funcs_template.icache,NULL},
					{"wield",		's',"R",&funcs_template.swield,NULL},
					{"wcache",		'i',"RW",&funcs_template.wcache,NULL},
					{"resurrect",	's',"R",&funcs_template.sresurrect,NULL},
					{"attack",		's',"R",&funcs_template.sattack,NULL},
					{"acache",		'i',"RW",&funcs_template.acache,NULL},
					{"quantity",		's',"R",&funcs_template.squantity,NULL},
					{"qcache",		'i',"RW",&funcs_template.qcache,NULL},
					{"get",			's',"R",&funcs_template.sget,NULL},
					{"gcache",		'i',"RW",&funcs_template.gcache,NULL},
					{"user1",		's',"R",&funcs_template.suser1,NULL},
					{"user2",		's',"R",&funcs_template.suser2,NULL},
					{NULL,			0,NULL,NULL,NULL},
					};

STRUCTURE userspec[] =
					{
					{"usedata",		' ',"",&usedata_template,NULL},
					{"user0",		'i',"RW",&usedata_template.user[0],NULL},
					{"user1",		'i',"RW",&usedata_template.user[1],NULL},
					{"user2",		'i',"RW",&usedata_template.user[2],NULL},
					{"user3",		'i',"RW",&usedata_template.user[3],NULL},
					{"user4",		'i',"RW",&usedata_template.user[4],NULL},
					{"user5",		'i',"RW",&usedata_template.user[5],NULL},
					{"user6",		'i',"RW",&usedata_template.user[6],NULL},
					{"user7",		'i',"RW",&usedata_template.user[7],NULL},
					{"user8",		'i',"RW",&usedata_template.user[8],NULL},
					{"user9",		'i',"RW",&usedata_template.user[9],NULL},
					{"user10",	'i',"RW",&usedata_template.user[10],NULL},
					{"user11",	'i',"RW",&usedata_template.user[11],NULL},
					{"user12",	'i',"RW",&usedata_template.user[12],NULL},
					{"user13",	'i',"RW",&usedata_template.user[13],NULL},
					{"user14",	'i',"RW",&usedata_template.user[14],NULL},
					{"user15",	'i',"RW",&usedata_template.user[15],NULL},
					{"user16",	'i',"RW",&usedata_template.user[16],NULL},
					{"user17",	'i',"RW",&usedata_template.user[17],NULL},
					{"user18",	'i',"RW",&usedata_template.user[18],NULL},
					{"user19",	'i',"RW",&usedata_template.user[19],NULL},
					{"poison",		'i',"RW",&usedata_template.poison,NULL},
					{"unconscious",	'i',"RW",&usedata_template.unconscious,NULL},
					{"potion0",		'i',"RW",&usedata_template.potion[0],NULL},
					{"potion1",		'i',"RW",&usedata_template.potion[1],NULL},
					{"potion2",		'i',"RW",&usedata_template.potion[2],NULL},
					{"potion3",		'i',"RW",&usedata_template.potion[3],NULL},
					{"potion4",		'i',"RW",&usedata_template.potion[4],NULL},
					{"potion5",		'i',"RW",&usedata_template.potion[5],NULL},
					{"potion6",		'i',"RW",&usedata_template.potion[6],NULL},
					{"potion7",		'i',"RW",&usedata_template.potion[7],NULL},
					{"potion8",		'i',"RW",&usedata_template.potion[8],NULL},
					{"potion9",		'i',"RW",&usedata_template.potion[9],NULL},
					{"dx",			'i',"RW",&usedata_template.dx,NULL},
					{"dy",			'i',"RW",&usedata_template.dy,NULL},
					{"vigilante",	'i',"RW",&usedata_template.vigilante,NULL},
					{"counter",		'i',"RW",&usedata_template.counter,NULL},
					{"experience",	'i',"RW",&usedata_template.experience,NULL},
					{"magic",		'i',"RW",&usedata_template.magic,NULL},
					{"oldhp",		'i',"RW",&usedata_template.oldhp,NULL},
					{"pathgoal",	'o',"RW",&usedata_template.pathgoal,NULL},
					{"pathgoal",	'i',"RW",&usedata_template.pathgoal,NULL},
					{"pathgoal",	'>',"",&usedata_template.pathgoal,&objspec},
					{"timeout",	'i',"RW",&usedata_template.counter,NULL},
					{"fx_func",	'i',"R",&usedata_template.fx_func,NULL},
					{"originmap",	'i',"R",&usedata_template.originmap,NULL},
					{NULL,			0,NULL,NULL,NULL},
					};

STRUCTURE wieldspec[] =
					{
					{"wield",		' ',"",&wield_template,NULL},
					{"head",		'o',"RW",&wield_template.head,NULL},
					{"head",		'>',"R",&wield_template.head,&objspec},
					{"neck",		'o',"RW",&wield_template.neck,NULL},
					{"neck",		'>',"R",&wield_template.neck,&objspec},
					{"body",		'o',"RW",&wield_template.body,NULL},
					{"body",		'>',"R",&wield_template.body,&objspec},
					{"legs",		'o',"RW",&wield_template.legs,NULL},
					{"legs",		'>',"R",&wield_template.legs,&objspec},
					{"feet",		'o',"RW",&wield_template.feet,NULL},
					{"feet",		'>',"R",&wield_template.feet,&objspec},
					{"arms",		'o',"RW",&wield_template.arms,NULL},
					{"arms",		'>',"R",&wield_template.arms,&objspec},
					{"l_hand",		'o',"RW",&wield_template.l_hand,NULL},
					{"l_hand",		'>',"R",&wield_template.l_hand,&objspec},
					{"r_hand",		'o',"RW",&wield_template.r_hand,NULL},
					{"r_hand",		'>',"R",&wield_template.r_hand,&objspec},
					{"l_finger",	'o',"RW",&wield_template.l_finger,NULL},
					{"l_finger",	'>',"R",&wield_template.l_finger,&objspec},
					{"r_finger",	'o',"RW",&wield_template.r_finger,NULL},
					{"r_finger",	'>',"R",&wield_template.r_finger,&objspec},
					{"spare1",		'o',"RW",&wield_template.spare1,NULL},
					{"spare1",		'>',"R",&wield_template.spare1,&objspec},
					{"spare2",		'o',"RW",&wield_template.spare2,NULL},
					{"spare2",		'>',"R",&wield_template.spare2,&objspec},
					{"spare3",		'o',"RW",&wield_template.spare3,NULL},
					{"spare3",		'>',"R",&wield_template.spare3,&objspec},
					{"spare4",		'o',"RW",&wield_template.spare4,NULL},
					{"spare4",		'>',"R",&wield_template.spare4,&objspec},
					{NULL,			0,NULL,NULL,NULL},
					};

STRUCTURE labelspec[] =
					{
					{"labels",		' ',"",&label_template,NULL},
					{"rank",		's',"R",&label_template.rank,NULL},
					{"race",		's',"R",&label_template.race,NULL},
					{"party",		's',"R",&label_template.party,NULL},
					{"location",		's',"R",&label_template.location,NULL},
					{"faction",		's',"R",&label_template.faction,NULL},
					{NULL,			0,NULL,NULL,NULL},
					};

// These are the structs that can exist as separate variables
// rather than just as members of an existing variable

STRUCTURE *pe_datatypes[] =
					{
					&objspec[0],
					&tilespec[0],
					&objarrayhack[0],
					&strarrayhack[0],
					&intarrayhack[0],
					NULL
					};

// Allowed variable types

char pe_vartypes[]="iotsPU\0";

static int pe_localvars=0;

void pe_predefined_symbols()
{
KEYWORD *ptr;

// These symbols are part of the game engine API

add_symbol_ptr("player",'o',&player);
add_symbol_ptr("me",'o',&person);
add_symbol_ptr("current",'o',&current_object);
add_symbol_ptr("curtile",'t',&current_tile);
add_symbol_ptr("victim",'o',&victim);
add_symbol_ptr("syspocket",'o',&syspocket);
add_symbol_ptr("blockage",'o',&moveobject_blockage);

ptr = add_symbol_ptr("party",'c',party);
ptr->arraysize=MAX_MEMBERS;

add_symbol_ptr("game_minute",'i',&game_minute);
add_symbol_ptr("game_hour",'i',&game_hour);
add_symbol_ptr("game_day",'i',&game_day);
add_symbol_ptr("game_month",'i',&game_month);
add_symbol_ptr("game_year",'i',&game_year);
add_symbol_ptr("window_left",'i',&mapx);
add_symbol_ptr("window_right",'i',&mapx2);
add_symbol_ptr("window_top",'i',&mapy);
add_symbol_ptr("window_bottom",'i',&mapy2);
add_symbol_ptr("mapw",'i',&map_W);
add_symbol_ptr("maph",'i',&map_H);
add_symbol_ptr("darkmix",'i',&dark_mix);

add_symbol_ptr("key",'i',&irekey);
add_symbol_ptr("new_x",'i',&new_x);
add_symbol_ptr("new_y",'i',&new_y);
add_symbol_ptr("show_roof",'i',&force_roof);
add_symbol_ptr("roof_visible",'i',&show_roof);
add_symbol_ptr("blanking_off",'i',&blanking_off);
add_symbol_ptr("combat_mode",'i',&combat_mode);
add_symbol_ptr("mouseclick",'i',&MouseID);
add_symbol_ptr("mousegrid_x",'i',&MouseGridX);
add_symbol_ptr("mousegrid_y",'i',&MouseGridY);
add_symbol_ptr("mousemap_x",'i',&MouseMapX);
add_symbol_ptr("mousemap_y",'i',&MouseMapY);

add_symbol_ptr("usernum1",'i',&pe_usernum1); // NUSPEECH can use these
add_symbol_ptr("usernum2",'i',&pe_usernum2);
add_symbol_ptr("usernum3",'i',&pe_usernum3);
add_symbol_ptr("usernum4",'i',&pe_usernum4);
add_symbol_ptr("usernum5",'i',&pe_usernum5);
add_symbol_ptr("userstr1",'s',&pe_userstr1); 
add_symbol_ptr("userstr2",'s',&pe_userstr2);
add_symbol_ptr("userstr3",'s',&pe_userstr3);
add_symbol_ptr("userstr4",'s',&pe_userstr4);
add_symbol_ptr("userstr5",'s',&pe_userstr5);

// These are the supported math operators

add_symbol_val("+",'p',1);
add_symbol_val("-",'p',2);
add_symbol_val("/",'p',3);
add_symbol_val("*",'p',4);
add_symbol_val("=",'p',5);
add_symbol_val("==",'p',5);
add_symbol_val("!=",'p',6);
add_symbol_val("<>",'p',6);
add_symbol_val("<",'p',7);
add_symbol_val("<=",'p',8);
add_symbol_val(">",'p',9);
add_symbol_val(">=",'p',10);
add_symbol_val("mod",'p',11);
add_symbol_val("&",'p',12);
add_symbol_val("&&",'p',12);
add_symbol_val("and",'p',12);
add_symbol_val("|",'p',13);
add_symbol_val("||",'p',13);
add_symbol_val("or",'p',13);
add_symbol_val("^",'p',14);
add_symbol_val("xor",'p',14);
add_symbol_val("nand",'p',15);
add_symbol_val("shl",'p',16);
add_symbol_val("<<",'p',16);
add_symbol_val("shr",'p',17);
add_symbol_val(">>",'p',17);

// constants (defined in opcodes.h)
// ADD_CONST is a macro to quote the symbol name.  Constants are type 'n'

ADD_CONST(UP);
ADD_CONST(DOWN);
ADD_CONST(LEFT);
ADD_CONST(RIGHT);
add_symbol_val("NORTH",'n',UP);
add_symbol_val("SOUTH",'n',DOWN);
add_symbol_val("WEST",'n',LEFT);
add_symbol_val("EAST",'n',RIGHT);

add_symbol_val("BLOCKING",'n',0);
add_symbol_val("NONBLOCKING",'n',1);
add_symbol_val("GAMEWINDOW",'n',0);

add_symbol_ptr("IRE_VERSION",'P',&IRE_VERSION);


ADD_CONST(CHAR_U);
ADD_CONST(CHAR_D);
ADD_CONST(CHAR_L);
ADD_CONST(CHAR_R);

// Pathfinder

ADD_CONST(PATH_FINISHED);
ADD_CONST(PATH_WAITING);
ADD_CONST(PATH_BLOCKED);

// Object Flags

ADD_CONST(IS_ON);
ADD_CONST(CAN_OPEN);
ADD_CONST(IS_WINDOW);
ADD_CONST(IS_SOLID);
ADD_CONST(IS_FRAGILE);
ADD_CONST(IS_TRIGGER);
ADD_CONST(IS_INVISIBLE);
ADD_CONST(IS_SEMIVISIBLE);
ADD_CONST(IS_FIXED);
ADD_CONST(IS_CONTAINER);
ADD_CONST(IS_TRANSLUCENT);
ADD_CONST(IS_SPIKEPROOF);
ADD_CONST(CAN_WIELD);
ADD_CONST(IS_REPEATSPIKE);
ADD_CONST(IS_BAGOFHOLDING);
ADD_CONST(DOES_BLOCKLIGHT);
ADD_CONST(IS_TABLETOP);
ADD_CONST(DID_INIT);
ADD_CONST(IS_PERSON);
ADD_CONST(IS_HORRIBLE);
ADD_CONST(IS_HORROR);
ADD_CONST(IS_SHOCKING);
ADD_CONST(IS_QUANTITY);
ADD_CONST(IS_BOAT);
ADD_CONST(IS_WATER);
ADD_CONST(IS_SHADOW);
ADD_CONST(IS_DECOR);
ADD_CONST(IS_SYSTEM);
//ADD_CONST(IS_NPCFLAG);	// RESERVED

ADD_CONST(IS_INVISSHADOW);	// COMBO FLAG

// NPC Flags

ADD_CONST(IS_FEMALE);
ADD_CONST(KNOW_NAME);
ADD_CONST(IS_HERO);
ADD_CONST(CANT_EAT);
ADD_CONST(CANT_DRINK);
ADD_CONST(IS_CRITICAL);
ADD_CONST(NOT_CLOSE_DOOR);
ADD_CONST(NOT_CLOSE_DOORS);
ADD_CONST(IS_SYMLINK);
ADD_CONST(IS_BIOLOGICAL);
ADD_CONST(IS_GUARD);
ADD_CONST(IS_SPAWNED);
ADD_CONST(NOT_OPEN_DOOR);
ADD_CONST(NOT_OPEN_DOORS);
ADD_CONST(IN_BED);
ADD_CONST(NO_SCHEDULE);
ADD_CONST(IS_OVERDUE);
ADD_CONST(IS_WIELDED);
ADD_CONST(IS_ROBOT);
ADD_CONST(IN_PARTY);

// Engine flags, you shouldn't need to do this

ADD_CONST(ENGINE_ISLARGE);

// Keys

ADD_IREKEY(KEY_A);
ADD_IREKEY(KEY_B);
ADD_IREKEY(KEY_C);
ADD_IREKEY(KEY_D);
ADD_IREKEY(KEY_E);
ADD_IREKEY(KEY_F);
ADD_IREKEY(KEY_G);
ADD_IREKEY(KEY_H);
ADD_IREKEY(KEY_I);
ADD_IREKEY(KEY_J);
ADD_IREKEY(KEY_K);
ADD_IREKEY(KEY_L);
ADD_IREKEY(KEY_M);
ADD_IREKEY(KEY_N);
ADD_IREKEY(KEY_O);
ADD_IREKEY(KEY_P);
ADD_IREKEY(KEY_Q);
ADD_IREKEY(KEY_R);
ADD_IREKEY(KEY_S);
ADD_IREKEY(KEY_T);
ADD_IREKEY(KEY_U);
ADD_IREKEY(KEY_V);
ADD_IREKEY(KEY_W);
ADD_IREKEY(KEY_X);
ADD_IREKEY(KEY_Y);
ADD_IREKEY(KEY_Z);
ADD_IREKEY(KEY_0);
ADD_IREKEY(KEY_1);
ADD_IREKEY(KEY_2);
ADD_IREKEY(KEY_3);
ADD_IREKEY(KEY_4);
ADD_IREKEY(KEY_5);
ADD_IREKEY(KEY_6);
ADD_IREKEY(KEY_7);
ADD_IREKEY(KEY_8);
ADD_IREKEY(KEY_9);
ADD_IREKEY(KEY_0_PAD);
ADD_IREKEY(KEY_1_PAD);
ADD_IREKEY(KEY_2_PAD);
ADD_IREKEY(KEY_3_PAD);
ADD_IREKEY(KEY_4_PAD);
ADD_IREKEY(KEY_5_PAD);
ADD_IREKEY(KEY_6_PAD);
ADD_IREKEY(KEY_7_PAD);
ADD_IREKEY(KEY_8_PAD);
ADD_IREKEY(KEY_9_PAD);
ADD_IREKEY(KEY_F1);
ADD_IREKEY(KEY_F2);
ADD_IREKEY(KEY_F3);
ADD_IREKEY(KEY_F4);
ADD_IREKEY(KEY_F5);
ADD_IREKEY(KEY_F6);
ADD_IREKEY(KEY_F7);
ADD_IREKEY(KEY_F8);
ADD_IREKEY(KEY_F9);
ADD_IREKEY(KEY_F10);
ADD_IREKEY(KEY_F11);
ADD_IREKEY(KEY_F12);
ADD_IREKEY(KEY_ESC);
ADD_IREKEY(KEY_TILDE);
ADD_IREKEY(KEY_MINUS);
ADD_IREKEY(KEY_EQUALS);
ADD_IREKEY(KEY_BACKSPACE);
ADD_IREKEY(KEY_TAB);
ADD_IREKEY(KEY_OPENBRACE);
ADD_IREKEY(KEY_CLOSEBRACE);
ADD_IREKEY(KEY_ENTER);
ADD_IREKEY(KEY_COLON);
ADD_IREKEY(KEY_QUOTE);
ADD_IREKEY(KEY_BACKSLASH);
ADD_IREKEY(KEY_BACKSLASH2);
ADD_IREKEY(KEY_COMMA);
ADD_IREKEY(KEY_STOP);
ADD_IREKEY(KEY_SLASH);
ADD_IREKEY(KEY_SPACE);
ADD_IREKEY(KEY_INSERT);
ADD_IREKEY(KEY_DEL);
ADD_IREKEY(KEY_HOME);
ADD_IREKEY(KEY_END);
ADD_IREKEY(KEY_PGUP);
ADD_IREKEY(KEY_PGDN);
ADD_IREKEY(KEY_LEFT);
ADD_IREKEY(KEY_RIGHT);
ADD_IREKEY(KEY_UP);
ADD_IREKEY(KEY_DOWN);
ADD_IREKEY(KEY_SLASH_PAD);
ADD_IREKEY(KEY_ASTERISK);
ADD_IREKEY(KEY_MINUS_PAD);
ADD_IREKEY(KEY_PLUS_PAD);
ADD_IREKEY(KEY_DEL_PAD);
ADD_IREKEY(KEY_ENTER_PAD);
ADD_IREKEY(KEY_PRTSCR);
ADD_IREKEY(KEY_PAUSE);
ADD_IREKEY(KEY_YEN);
ADD_IREKEY(KEY_MODIFIERS);
ADD_IREKEY(KEY_LSHIFT);
ADD_IREKEY(KEY_RSHIFT);
ADD_IREKEY(KEY_LCONTROL);
ADD_IREKEY(KEY_RCONTROL);
ADD_IREKEY(KEY_ALT);
ADD_IREKEY(KEY_ALTGR);
ADD_IREKEY(KEY_LWIN);
ADD_IREKEY(KEY_RWIN);
ADD_IREKEY(KEY_MENU);
ADD_IREKEY(KEY_SCRLOCK);
ADD_IREKEY(KEY_NUMLOCK);
ADD_IREKEY(KEY_CAPSLOCK);

add_symbol_val("SHIFT_F1",'n',IREKEY_F1|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F2",'n',IREKEY_F2|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F3",'n',IREKEY_F3|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F4",'n',IREKEY_F4|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F5",'n',IREKEY_F5|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F6",'n',IREKEY_F6|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F7",'n',IREKEY_F7|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F8",'n',IREKEY_F8|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F9",'n',IREKEY_F9|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F10",'n',IREKEY_F10|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F11",'n',IREKEY_F11|IREKEY_SHIFTMOD);
add_symbol_val("SHIFT_F12",'n',IREKEY_F12|IREKEY_SHIFTMOD);

add_symbol_val("KEY_SHIFT_F1",'n',IREKEY_F1|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F2",'n',IREKEY_F2|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F3",'n',IREKEY_F3|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F4",'n',IREKEY_F4|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F5",'n',IREKEY_F5|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F6",'n',IREKEY_F6|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F7",'n',IREKEY_F7|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F8",'n',IREKEY_F8|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F9",'n',IREKEY_F9|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F10",'n',IREKEY_F10|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F11",'n',IREKEY_F11|IREKEY_SHIFTMOD);
add_symbol_val("KEY_SHIFT_F12",'n',IREKEY_F12|IREKEY_SHIFTMOD);

add_symbol_val("CTRL_F1",'n',IREKEY_F1|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F2",'n',IREKEY_F2|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F3",'n',IREKEY_F3|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F4",'n',IREKEY_F4|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F5",'n',IREKEY_F5|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F6",'n',IREKEY_F6|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F7",'n',IREKEY_F7|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F8",'n',IREKEY_F8|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F9",'n',IREKEY_F9|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F10",'n',IREKEY_F10|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F11",'n',IREKEY_F11|IREKEY_CTRLMOD);
add_symbol_val("CTRL_F12",'n',IREKEY_F12|IREKEY_CTRLMOD);

add_symbol_val("KEY_CTRL_F1",'n',IREKEY_F1|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F2",'n',IREKEY_F2|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F3",'n',IREKEY_F3|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F4",'n',IREKEY_F4|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F5",'n',IREKEY_F5|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F6",'n',IREKEY_F6|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F7",'n',IREKEY_F7|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F8",'n',IREKEY_F8|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F9",'n',IREKEY_F9|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F10",'n',IREKEY_F10|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F11",'n',IREKEY_F11|IREKEY_CTRLMOD);
add_symbol_val("KEY_CTRL_F12",'n',IREKEY_F12|IREKEY_CTRLMOD);

add_symbol_val("KEY_MOUSE",'n',IREKEY_MOUSE);
add_symbol_val("KEY_MOUSEUP",'n',IREKEY_MOUSEUP);
add_symbol_val("KEY_MOUSEDOWN",'n',IREKEY_MOUSEDOWN);

ADD_CONST(MAX_MEMBERS);

// Alpha blending methods
ADD_CONST(ALPHA_SOLID);
ADD_CONST(ALPHA_TRANS);
ADD_CONST(ALPHA_ADD);
ADD_CONST(ALPHA_SUBTRACT);
ADD_CONST(ALPHA_DISSOLVE);
ADD_CONST(ALPHA_INVERT);

ADD_CONST(TINT_RED);
ADD_CONST(TINT_GREEN);
ADD_CONST(TINT_BLUE);
ADD_CONST(TINT_CYAN);
ADD_CONST(TINT_YELLOW);
ADD_CONST(TINT_MAGENTA);
ADD_CONST(TINT_PURPLE);
ADD_CONST(TINT_LIGHTBLUE);
ADD_CONST(TINT_WHITE);

ADD_CONST(JOURNAL_ALL);
ADD_CONST(JOURNAL_TODO_ONLY);
ADD_CONST(JOURNAL_TODO_HEADER_ONLY);
ADD_CONST(JOURNAL_DONE_ONLY);
ADD_CONST(JOURNAL_DONE_HEADER_ONLY);

add_symbol_ptr("fx_srcx",'i',&tfx_sx);
add_symbol_ptr("fx_srcy",'i',&tfx_sy);
add_symbol_ptr("fx_destx",'i',&tfx_dx);
add_symbol_ptr("fx_desty",'i',&tfx_dy);
add_symbol_ptr("fx_brush",'i',&tfx_Brushsize);

add_symbol_ptr("start_delay",'i',&tfx_picdelay1);
add_symbol_ptr("end_delay",'i',&tfx_picdelay2);
add_symbol_ptr("scroll_delay",'i',&tfx_picdelay3);

add_symbol_ptr("fx_radius",'i',&tfx_radius);
add_symbol_ptr("fx_drift",'i',&tfx_drift);
add_symbol_ptr("fx_speed",'i',&tfx_speed);
add_symbol_ptr("fx_intensity",'i',&tfx_intensity);
add_symbol_ptr("fx_falloff",'i',&tfx_falloff);

// Other symbols

add_symbol_val("null",'0',0);
add_symbol_val("null",'n',0);
add_symbol_ptr("err",'i',&pevm_err);
add_symbol_ptr("SoundFixed",'i',&SoundFixed);
add_symbol_ptr("ShowInvisibleObjects",'i',&show_invisible);


add_symbol_val("START",'n',1);
add_symbol_val("END",'n',0);
}


//
//  The first parameter is a destination, make sure it's writable
//
void PE_checkaccess(char **line) {
char buf[2048];
STRUCTURE *member = NULL;

// If we're not doing a full compile, forget it
if(PE_FastBuild) {
	return;
}

if(strlen(pe_parm) < 1) {
	PeDump(srcline,"Internal error","Shouldn't use PE_checkaccess for functions with no parameters");
}

switch(pe_parm[0]) {
	case '0':
	case 'n':
		PeDump(srcline, "Syntax error", "Can't assign a value to a constant!");
		break;

	case 'f':
		PeDump(srcline, "Internal error", "No, you can't assign something to a function");
		break;

	case 'i':
	case 'o':
	case 't':
	case 'P':
	case 'U':
		// Basic objects: not yet supported
		break;

	case 'a':
	case 'b':
	case 'c':
		// Not yet supported
		break;

	case 's':
		break;

	case 'I':
	case 'O':
	case 'S':
		member = pe_getstruct(line[1]);
		if(!member) {
			PeDump(srcline,"Internal error in structure access",line[1]);
		}
		if(!member->access) {
			PeDump(srcline,"Internal error", "Structure member has no access rights");
		}
		if(!strchr(member->access,'W')) {
			// We have a winner
			buf[2047]=0;
			snprintf(buf,2047,"Object %s is read-only!", line[1]);
			PeDump(srcline,"Syntax error", buf);
		}
		break;
}

// OK, checks passed, proceed as usual
PE_generic(line);
}


void PE_generic(char **line)
{
char msg[]="Internal error handling type '*'";
char buf[2048];
KEYWORD *funcptr;
int ctr,len,num;
char *v;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

add_opcode(vmp->opcode);

len = strlen(pe_parm);
for(ctr=0;ctr<len;ctr++)
	{
	switch(pe_parm[ctr])
		{
		case '0':
		add_number(0);
		break;

		case 'n':
		add_number(pe_getnumber(line[ctr+1]));
		break;

		case 'i':
		case 'o':
		case 't':
		case 'P':
		case 'U':
		add_variable(line[ctr+1]);
		break;

		case 'a':
		case 'b':
		case 'c':
		add_array(line[ctr+1],0);
		break;

		case 's':
		extract_string(buf,line[ctr+1]);
		add_string(buf);
		break;

		case 'f':
		funcptr = find_keyword(line[ctr+1],'f',NULL);
		if(!funcptr)
			PeDump(srcline,"Internal error","Initial parse checking failure");
		add_dword(funcptr->id); // This is the function number, compatible with PElist
		break;

		case 'T':
		v = line[ctr+1];

		num = getnum4table_slow(v);
		if(num <0)
			PeDump(srcline,"Unknown data table",v);
		add_number(num);
		break;

/*
		case 'x':
		add_expression(strgetword(line,ctr+1));
		break;
*/

		case 'I':
		add_intmember(line[ctr+1]);
		break;

		case 'O':
		add_objmember(line[ctr+1]);
		break;

		case 'S':
		add_strmember(line[ctr+1]);
		break;

		case 'p':
		add_operator(line[ctr+1]);
		break;

		case '=':
		case '?':
		case '(':
		case 'e':
		case 'E':
		break;

		default:
		*strchr(msg,'*')=pe_parm[ctr];
		PeDump(srcline,msg,line[0]);
		break;
		}
	}
}


void PE_newfunc(char **line)
{
KEYWORD *ptr;

if(curfunc)
    PeDump(srcline,"Missing END in previous function",NULL);

ptr = find_keyword(line[1],'f',NULL);
if(!ptr)
    PeDump(srcline,"Internal error, invalid function in PE_newfunc",line[0]);
curfunc = ptr->name;

pe_output->name = ptr->name;
pe_output->hidden=0;
if(ptr->localfile)
	pe_output->hidden=1;

if(!start_icode())
    PeDump(srcline,"Haven't Finished","PE_newfunc()");
}

void PEP_newfunc(char **line)
{
KEYWORD *ptr;

if(strlen(line[1]) > 31) {
    PeDump(srcline,"Function name too long",line[1]);
}

ptr = add_keyword(line[1],'f',NULL);
if(!ptr)
	PeDump(srcline,"Duplicate function",line[1]);
curfunc = ptr->name;
ptr->id = funcid++;
pe_numfuncs++;
pe_localvars=0;
}


void PE_endfunc(char **line)
{
if(!curfunc)
    PeDump(srcline,"END before function starts",NULL);

add_opcode(PEVM_Return);

output_icode();

curfunc=NULL;

if(!finish_icode())
    PeDump(srcline,"Haven't Started","PE_endfunc()");
}

void PEP_endfunc(char **line)
{
// Mark current function is ended
curfunc=NULL;
}

void PEP_StartLocal(char **line)
{
pe_localfile=compilename;
}

void PEP_EndLocal(char **line)
{
pe_localfile=NULL;
}

void PEP_Transient(char **line)
{
// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!line[1]) {
	PeDump(srcline,"no variable specified",NULL);
}

KEYWORD *k=find_keyword(line[1],'?',curfunc);
if(!k) {
	PeDump(srcline,"Unknown variable",line[1]);
}
if(curfunc) {
	PeDump(srcline,"Transient can only be used on globals", NULL);
}
k->transient = true;
}

void PEP_StartTransient(char **line)
{
pe_marktransient=true;
}

void PEP_EndTransient(char **line)
{
pe_marktransient=false;
}

// Mark this function as being an Activity (used as a label for the Editor)
void PE_classAct(char **line)
{
if(!curfunc)
	PeDump(srcline,"Activity statement outside a function",NULL);

pe_output->Class='A';
}

// Mark this function as being hidden (used as a label for the Editor)
void PE_classPrv(char **line)
{
if(!curfunc)
	PeDump(srcline,"Private statement outside a function",NULL);

pe_output->Class='P';
}


void PE_declare(char **line)
{
// Do nothing
}

void PEP_const(char **line)
{
char *ptr;
VMINT val;
KEYWORD *k;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!line[1])
	PeDump(srcline,"missing constant name in declaration",NULL);

ptr = line[3];
if(!pe_isnumber(ptr))
    PeDump(srcline,"Tried to define a constant to an invalid number",ptr);
val=pe_getnumber(ptr);

k = add_keyword(line[1],'n',curfunc);
if(!k) {
    k=find_keyword(line[1],'n',curfunc);
    if(k && k->preconfigured) {
        return; // It's been overridden on the commandline, ignore and keep the original value
    }
    PeDump(srcline,"Duplicate constant",line[1]);
}

k->value = (void *)val;
return;
}

void PEP_integer(char **line)
{
char *ptr;
VMINT val;
KEYWORD *k;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!line[1])
	PeDump(srcline,"missing variable name in declaration",NULL);

ptr = line[3];
if(ptr == NULL)
	val=0;
else
	{
	if(!pe_isnumber(ptr))
		PeDump(srcline,"Tried to declare integer variable as invalid number",ptr);
	val=pe_getnumber(ptr);
	}

ptr = line[1];
if(strchr(ptr,'['))
	PeDump(srcline,"Invalid character '[' in variable name",line[1]);
if(strchr(ptr,']'))
	PeDump(srcline,"Invalid character ']' in variable name",line[1]);

k = add_keyword(line[1],'i',curfunc);
if(!k)
	PeDump(srcline,"Duplicate variable",line[1]);


k->value = (void *)val;

// Make provision for it

if(curfunc == NULL)
	{
	k->id = pe_globals++; // Add to list of globals
#ifdef LOG_ADDING_GLOBALS
	ilog_quiet("Adding global %d: integer '%s' in %s\n",pe_globals-1,k->name,compilename);
#endif
	}
else
	k->id = pe_localvars++;  // Add to locals for current function
return;
}

void PEP_intarray(char **line)
{
if(!line[1])
	PeDump(srcline,"missing variable name in declaration",NULL);

PEP_array(line,'a');
}

void PEP_strarray(char **line)
{
if(!line[1])
	PeDump(srcline,"missing variable name in declaration",NULL);

PEP_array(line,'b');
}

void PEP_objarray(char **line)
{
if(!line[1])
	PeDump(srcline,"missing variable name in declaration",NULL);

PEP_array(line,'c');
}


void PEP_object(char **line)
{
KEYWORD *k;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!line[1])
	PeDump(srcline,"missing variable name in declaration",NULL);

k = add_keyword(line[1],'o',curfunc);
if(!k)
	PeDump(srcline,"Duplicate variable",line[1]);

k->value = NULL; // Zero it

// Make provision for it

if(curfunc == NULL)
	{
	k->id = pe_globals++; // Add to list of globals
#ifdef LOG_ADDING_GLOBALS
	ilog_quiet("Adding global %d: object '%s' in %s\n",pe_globals-1,k->name,compilename);
#endif
	}
else
	k->id = pe_localvars++;  // Add to locals for current function
return;
}

void PEP_tile(char **line)
{
KEYWORD *k;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!line[1])
	PeDump(srcline,"missing variable name in declaration",NULL);

k = add_keyword(line[1],'t',curfunc);
if(!k)
	PeDump(srcline,"Duplicate variable",line[1]);

k->value = NULL; // Zero it

// Make provision for it

if(curfunc == NULL)
	{
	k->id = pe_globals++; // Add to list of globals
#ifdef LOG_ADDING_GLOBALS
	ilog_quiet("Adding global %d: tile '%s' in %s\n",pe_globals-1,k->name,compilename);
#endif
	}
else
	k->id = pe_localvars++;  // Add to locals for current function
return;
}


void PEP_label(char **line)
{
KEYWORD *ptr;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!curfunc)
    PeDump(srcline,"Labels can only be declared inside functions",line[1]);

if(!line[1])
	PeDump(srcline,"missing label name in declaration",NULL);

ptr = add_keyword(line[1],'l',curfunc);
if(!ptr)
    PeDump(srcline,"Duplicate label in current function",line[1]);
ptr->id = 0;
}

void PEP_string(char **line)
{
char *ptr;
KEYWORD *k;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!line[1])
	PeDump(srcline,"missing variable name in declaration",NULL);

ptr = line[2];
if(ptr != NULL)
	{
	PeDump(srcline,"String declaration doesn't work that way, define a 'userstring'",line[1]);
	}

k = add_keyword(line[1],'s',curfunc);
if(!k)
	PeDump(srcline,"Duplicate variable",line[1]);

k->value = NULL;

// Make provision for it

if(curfunc == NULL)
	{
	k->id = pe_globals++; // Add to list of globals
#ifdef LOG_ADDING_GLOBALS
	ilog_quiet("Adding global %d: string '%s' in %s\n",pe_globals-1,k->name,compilename);
#endif
	}
else
	k->id = pe_localvars++;  // Add to locals for current function
return;
}

void PEP_array(char **line, char type)
{
char *ptr,*ptr2;
int val;
KEYWORD *k;
char str[1024];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

ptr = line[1];
if(ptr == NULL)
	PeDump(srcline,"Something bad happened while declaring array",NULL);

strcpy(str,ptr);
ptr = strchr(str,'[');
if(!ptr)
	PeDump(srcline,"Array has no size specified.. should be 'array[100]' etc",line[1]);
*ptr=0;
ptr++;

ptr2 = strchr(ptr,']');
if(!ptr2)
	PeDump(srcline,"Couldn't find closing bracket in array declaration",line[1]);
*ptr2=0;

if(!pe_isnumber(ptr))
	PeDump(srcline,"Array size is not a valid number",ptr);
val=pe_getnumber(ptr);

if(val<1)
	PeDump(srcline,"Array is zero length, or less",ptr);

k = add_keyword(strgetword(str,1),type,curfunc);
if(!k)
	PeDump(srcline,"Duplicate variable",str);

k->value = NULL;
k->arraysize = val;

// Make provision for it

if(curfunc == NULL)
	{
#ifdef LOG_ADDING_GLOBALS
	ilog_quiet("Creating array %s in file %s\n",k->name,compilename);
	ilog_quiet("Creating %d objects (array) %d-%d\n",val,pe_globals,(pe_globals+val)-1);
#endif
	k->id = pe_globals; // Add to list of globals
	pe_globals+=val;
	}
else
	{
	k->id = pe_localvars;  // Add to locals for current function
	pe_localvars+=val;
	}
return;
}




void PEP_userstring(char **line)
{
char *ptr;
KEYWORD *k;
int len;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!line[1])
	PeDump(srcline,"missing variable name in declaration",NULL);

if(curfunc != NULL)
	PeDump(srcline,"User-strings can't be declared locally",line[1]);

ptr = line[2];
if(ptr == NULL)
	{
	PeDump(srcline,"You must specify the maximum length of the string.",line[1]);
	}
len=pe_getnumber(ptr);
if(len<1)
	PeDump(srcline,"You must a valid maximum length for the string.",line[2]);

k = add_keyword(line[1],'U',curfunc);
if(!k)
	PeDump(srcline,"Duplicate variable",line[1]);

k->value = STR_New(len); // Create the new string

// Make provision for it
k->id = pe_globals++; // Add to list of globals
#ifdef LOG_ADDING_GLOBALS
	ilog_quiet("Adding global %d: userstring '%s' in %s\n",pe_globals-1,k->name,compilename);
#endif

return;
}


void PE_label(char **line)
{
KEYWORD *ptr;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!line[1])
	PeDump(srcline,"missing label name in declaration",NULL);

/*
if(labelpending)
    {
    if(strchr(labelpending->name,'@'))
        {
#ifdef IF_LABEL_NOP_HACK
        add_opcode(PEVM_NOP);
#else
        PeDump(srcline,"You cannot have a label inside an IF statement",NULL);
#endif
        }
    else
        PeDump(srcline,"You cannot have two labels for the same line",NULL);
    }
*/

ptr = find_keyword(line[1],'l',curfunc);
if(!ptr)
    PeDump(srcline,"Internal error: Transient label detected",line[1]);
push_label(ptr);
}

void PE_goto(char **line)
{
// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;
add_opcode(vmp->opcode);
add_jump(line[1]);
}

void PEP_if(char **line)
{
KEYWORD *k;
char label[20];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!curfunc)
    PeDump(srcline,"IF statement outside a function",NULL);

mk_lineno(label,pe_lineid,"if");
push_ifstack(pe_lineid);

k = add_keyword(label,'l',curfunc);
if(!k)
    PeDump(srcline,"Internal error handling IF statement",NULL);
k->id = 0;
}

void PEP_and(char **line)
{
int if_id;
/*
KEYWORD *k;
char label[20];
*/

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!curfunc)
    PeDump(srcline,"AND statement outside a function",NULL);

if_id = read_ifstack();
if(!if_id)
    PeDump(srcline,"AND statement without IF",NULL);

/*
mk_lineno(label,pe_lineid,"if");
push_ifstack(pe_lineid);

k = add_keyword(label,'l',curfunc);
if(!k)
    PeDump(srcline,"Internal error handling IF statement",line);
k->id = 0;
*/
}

void PEP_or(char **line)
{
KEYWORD *k;
char label[20];
int if_id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!curfunc)
	PeDump(srcline,"OR statement outside a function",NULL);

if_id = read_ifstack();
if(!if_id)
	PeDump(srcline,"OR statement without IF",NULL);

mk_lineno(label,pe_lineid,"or");
//ilog_quiet("%s: Laying %s\n",curfunc,label);
k = add_keyword(label,'l',curfunc);
if(!k)
	PeDump(srcline,"Internal error handling OR statement",NULL);
k->id = 0;

mk_lineno(label,pe_lineid,"or_else");
//ilog_quiet("%s: Laying %s\n",curfunc,label);
k = add_keyword(label,'l',curfunc);
if(!k)
	PeDump(srcline,"Internal error handling OR statement",NULL);
k->id = 0;
}

void PEP_else(char **line)
{
KEYWORD *k;
char label[20];
int if_id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if_id = read_ifstack();

if(!if_id)
	PeDump(srcline,"ELSE statement without IF",NULL);

mk_lineno(label,if_id,"else");

k = add_keyword(label,'l',curfunc);
if(!k)
	PeDump(srcline,"Error handling ELSE statement: missing IF or ENDIF?",NULL);
k->id = 0;
}

void PEP_endif(char **line)
{
KEYWORD *k;
char label[20];
int if_id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if_id = read_ifstack();

if(!if_id)
	PeDump(srcline,"ENDIF statement without IF",NULL);

mk_lineno(label,if_id,"endif");

k = add_keyword(label,'l',curfunc);
if(!k)
	PeDump(srcline,"Error handling ENDIF statement: missing IF or previous ENDIF?",NULL);
k->id = 0;

pop_ifstack();
}

void PE_ifcore(char *label)
{
// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

push_ifstack(pe_lineid);

// First find the associated ELSE statement, or ENDIF if there is no ELSE

mk_lineno(label,pe_lineid+1,"or");
if(!find_keyword(label,'l',curfunc))
	{
	mk_lineno(label,pe_lineid,"else");
	if(!find_keyword(label,'l',curfunc))
		{
		mk_lineno(label,pe_lineid,"endif");
		if(!find_keyword(label,'l',curfunc))
			PeDump(srcline,"Missing ENDIF",label);
		}
	}

// Label is now the appropriate entry to jump to
}

void PE_andcore(char *label)
{
int id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

id = read_ifstack();

// First find the associated ELSE statement, or ENDIF if there is no ELSE

mk_lineno(label,id,"else");
if(!find_keyword(label,'l',curfunc))
	{
	mk_lineno(label,id,"endif");
	if(!find_keyword(label,'l',curfunc))
		PeDump(srcline,"Missing ENDIF",label);
	}

// Label is now the appropriate entry to jump to
}

// Find the appropriate destination for the OR statement
void PE_orcore(char *label)
{
// If we're not doing a full compile, forget it
if(PE_FastBuild) {
	return;
}

mk_lineno(label,pe_lineid+1,"or");
if(find_keyword(label,'l',curfunc)) {
	// If there's another OR in the next line, use that for the destination label
	return;
}

// Otherwise, use the else label as the target
PE_andcore(label);
}



// If we've got an OR statement following we need to do some trickery
void PE_orhelper(char *label, char **line)
{
KEYWORD *orptr,*tmp;

// If we're not doing a full compile, forget it
if(PE_FastBuild) {
	return;
}

orptr = NULL;
mk_lineno(label,pe_lineid+1,"or");
tmp = find_keyword(label,'l',curfunc);
if(tmp) {
	orptr = tmp;
}

if(orptr) {
	// Add an intermediate goto, for in case first one is OK and 2nd isn't
	mk_lineno(label,pe_lineid+1,"or_else");
	if(!find_keyword(label,'l',curfunc)) {
		PeDump(srcline,"Internal error handling OR statement (or else)",NULL);
	}
	add_opcode(PEVM_Goto);
	add_jump(label);
	// Generate the address for the second condition, for if the 1st is false
	push_label(orptr);
}
}

void PE_endif(char **line)
{
KEYWORD *ptr;
char label[20];
int if_id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

// If there's a jump in the way, add a NOP to prevent problems
//if(labelpending)
//	add_opcode(PEVM_NOP);

if_id = read_ifstack();
pop_ifstack();

mk_lineno(label,if_id,"endif");
ptr = find_keyword(label,'l',curfunc);

if(!ptr)
    PeDump(srcline,"Internal error: Transient IF statement",NULL);

push_label(ptr);
}

void PE_else(char **line)
{
KEYWORD *elseptr,*endifptr;
char label[20];
int if_id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if_id = read_ifstack();

mk_lineno(label,if_id,"else");
elseptr = find_keyword(label,'l',curfunc);
if(!elseptr)
    PeDump(srcline,"Internal error: Transient ELSE in PE_else()",NULL);

mk_lineno(label,if_id,"endif");
endifptr = find_keyword(label,'l',curfunc);
if(!endifptr)
    PeDump(srcline,"No matching ENDIF statement",NULL);

// Add the jump to the ENDIF at the end of the code branch

add_opcode(PEVM_Goto);
add_jump(endifptr->name);

// Fix up the ELSE label

push_label(elseptr);
}

void PE_if(char **line)
{
char label[20];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

PE_ifcore(label);
PE_generic(line);
add_jump(label);

PE_orhelper(label,line);
}


void PE_and(char **line)
{
char label[20];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

PE_andcore(label);
PE_generic(line);
add_jump(label);

PE_orhelper(label,line);
}

void PE_or(char **line)
{
char label[20];
KEYWORD *ptr;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

PE_orcore(label);
PE_generic(line);
add_jump(label);

// add dest label
mk_lineno(label,pe_lineid,"or");
ptr=find_keyword(label,'l',curfunc);
if(ptr) {
	push_label(ptr);
}

// Generate the destination for if the previous statement was false

mk_lineno(label,pe_lineid,"or_else");
//ilog_quiet("seeking %s\n",label);
ptr=find_keyword(label,'l',curfunc);
if(!ptr)
	PeDump(srcline,"Internal error handling OR statement (no or_else)",NULL);

push_label(ptr);

PE_orhelper(label,line);
}


// If Object Is Called <String>

void PE_if_oics(char **line)
{
char label[20];
char buf[2048];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

PE_ifcore(label);

add_opcode(vmp->opcode);
add_variable(line[1]);

extract_string(buf,line[4]);
add_string(buf);
add_jump(label);

PE_orhelper(label,line);

}

// If member-Object Is Called <String>

void PE_if_Oics(char **line)
{
char label[20];
char buf[2048];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

PE_ifcore(label);

add_opcode(vmp->opcode);
add_objmember(line[1]);

extract_string(buf,line[4]);
add_string(buf);
add_jump(label);

PE_orhelper(label,line);
}


void PE_and_Oics(char **line)
{
char label[20];
char buf[2048];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

PE_andcore(label);

add_opcode(vmp->opcode);
add_objmember(line[1]);

extract_string(buf,line[4]);
add_string(buf);
add_jump(label);

PE_orhelper(label,line);
}

void PE_and_oics(char **line)
{
char label[20];
char buf[2048];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

PE_andcore(label);

add_opcode(vmp->opcode);
add_variable(line[1]);

extract_string(buf,line[4]);
add_string(buf);
add_jump(label);

PE_orhelper(label,line);
}

void PE_or_Oics(char **line)
{
char label[20];
char buf[2048];
KEYWORD *ptr;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

PE_orcore(label);

add_opcode(vmp->opcode);
add_objmember(line[1]);

extract_string(buf,line[4]);
add_string(buf);
add_jump(label);

// add dest label
mk_lineno(label,pe_lineid,"or");
ptr=find_keyword(label,'l',curfunc);
if(ptr) {
	push_label(ptr);
}

mk_lineno(label,pe_lineid,"or_else");
ptr=find_keyword(label,'l',curfunc);
if(!ptr)
	PeDump(srcline,"Internal error handling OR statement (transience)",NULL);

push_label(ptr);

PE_orhelper(label,line);
}

void PE_or_oics(char **line)
{
char label[20];
char buf[2048];
KEYWORD *ptr;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

PE_orcore(label);

add_opcode(vmp->opcode);
add_variable(line[1]);

extract_string(buf,line[4]);
add_string(buf);
add_jump(label);

// add dest label
mk_lineno(label,pe_lineid,"or");
ptr=find_keyword(label,'l',curfunc);
if(ptr) {
	push_label(ptr);
}

mk_lineno(label,pe_lineid,"or_else");
ptr=find_keyword(label,'l',curfunc);
if(!ptr)
	PeDump(srcline,"Internal error handling OR statement (transience)",NULL);

push_label(ptr);

PE_orhelper(label,line);
}


void PEP_for(char **line)
{
KEYWORD *k;
char label[20];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if(!curfunc)
    PeDump(srcline,"FOR loop outside a function",NULL);

mk_lineno(label,pe_lineid,"for");
push_ifstack(pe_lineid);

k = add_keyword(label,'l',curfunc);
if(!k)
    PeDump(srcline,"Internal error handling FOR loop",NULL);
k->id = 0;
}

void PEP_next(char **line)
{
KEYWORD *k;
char label[20];
int if_id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if_id = read_ifstack();

if(!if_id)
    PeDump(srcline,"NEXT statement without FOR",NULL);

mk_lineno(label,if_id,"next");

k = add_keyword(label,'l',curfunc);
if(!k)
    PeDump(srcline,"Error handling NEXT statement: missing FOR?",NULL);
k->id = 0;

pop_ifstack();
}


void PE_forcore(char *label, char **line)
{
KEYWORD *ptr;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

// Initialiser code to set the variable to an appropriate starting value
// We need to detect if the starting value is a number or integer
// and build the code accordingly.
// This will incidentally, remove any pending label.

add_opcode(PEVM_Let_iei);

if(pe_isnumber(line[3]))
   {
   add_variable(line[1]);
   add_number(pe_getnumber(line[3]));
   }
else
   {
   add_variable(line[1]);
   add_variable(line[3]);
   }

// OK, that's that out of the way

push_ifstack(pe_lineid);

// Find our FOR statement's ID

mk_lineno(label,pe_lineid,"for");
ptr = find_keyword(label,'l',curfunc);
if(!ptr)
    PeDump(srcline,"Internal error: Transient FOR loop",NULL);

// When we actually write the FOR loop code, the label will point to it
push_label(ptr); // This will be NULL, courtesy of the initialiser code above

// Find the associated NEXT statement

mk_lineno(label,pe_lineid,"next");
if(!find_keyword(label,'l',curfunc))
    PeDump(srcline,"Missing NEXT statement",NULL);

// Label is now the appropriate entry to jump to
}

void PE_next(char **line)
{
KEYWORD *forptr,*nextptr;
char label[20];
int if_id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if_id = read_ifstack();
pop_ifstack();

mk_lineno(label,if_id,"for");
forptr = find_keyword(label,'l',curfunc);
if(!forptr)
    PeDump(srcline,"Internal error: Transient FOR statement",NULL);

mk_lineno(label,if_id,"next");
nextptr = find_keyword(label,'l',curfunc);
if(!nextptr)
    PeDump(srcline,"Internal error: Transient NEXT statement",NULL);

add_opcode(PEVM_Goto);
add_jump(forptr->name);

push_label(nextptr);
}

void PE_for(char **line)
{
char label[20];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

// First the initialiser
PE_forcore(label,line);

PE_generic(line);
add_byte(1|ACC_OPERATOR);                       // 'First Time' flag
add_jump(label);                                // (Exit jump)
}

void PEP_do(char **line)
{
KEYWORD *k;
char label[20];

if(!curfunc)
	PeDump(srcline,"DO/WHILE loop outside a function",NULL);

mk_lineno(label,pe_lineid,"do");
push_ifstack(pe_lineid);

k = add_keyword(label,'l',curfunc);
if(!k)
	PeDump(srcline,"Internal error handling DO loop",NULL);
k->id = 0;
}

void PE_do(char **line)
{
KEYWORD *doptr;
char label[20];

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

//if(labelpending)
//	add_opcode(PEVM_NOP);

push_ifstack(pe_lineid); // Store the line number for the while block

mk_lineno(label,pe_lineid,"do");
doptr = find_keyword(label,'l',curfunc);
if(!doptr)
	PeDump(srcline,"Internal error: Transient DO statement",NULL);

push_label(doptr);
}

void PEP_while(char **line)
{
KEYWORD *k;
char label[20];
int if_id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if_id = read_ifstack();

if(!if_id)
	PeDump(srcline,"WHILE statement without DO",NULL);

mk_lineno(label,if_id,"while");

k = add_keyword(label,'l',curfunc);
if(!k)
	PeDump(srcline,"Error handling WHILE statement: missing DO?",NULL);
k->id = 0;

pop_ifstack();
}

void PE_while1(char **line)
{
KEYWORD *doptr,*whileptr;
char label[20];
int if_id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if_id = read_ifstack();
pop_ifstack();

mk_lineno(label,if_id,"do");
doptr = find_keyword(label,'l',curfunc);
if(!doptr)
	PeDump(srcline,"Internal error: Transient DO statement",NULL);

mk_lineno(label,if_id,"while");
whileptr = find_keyword(label,'l',curfunc);
if(!whileptr)
	PeDump(srcline,"Internal error: Transient WHILE statement",NULL);

PE_generic(line);
add_jump(doptr->name);

push_label(whileptr);
}

void PE_while2(char **line)
{
KEYWORD *doptr,*whileptr;
char label[20];
int if_id;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

if_id = read_ifstack();
pop_ifstack();

mk_lineno(label,if_id,"do");
doptr = find_keyword(label,'l',curfunc);
if(!doptr)
	PeDump(srcline,"Internal error: Transient DO statement",NULL);

mk_lineno(label,if_id,"while");
whileptr = find_keyword(label,'l',curfunc);
if(!whileptr)
	PeDump(srcline,"Internal error: Transient WHILE statement",NULL);

PE_generic(line);
add_jump(doptr->name);

push_label(whileptr);
}

void PE_continue(char **line)
{
KEYWORD *doptr;
char label[20];
int if_id,lookback;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

lookback=0;
do  {
	// Read a line number off the ifstack (without changing it)
	if_id = explore_ifstack(lookback++);
	// If there aren't any more, stop
	if(if_id < 0)
		PeDump(srcline,"CONTINUE without DO/WHILE or FOR loop",NULL);

	// Otherwise, see if we can find a matching label for the jump
	mk_lineno(label,if_id,"do");
	doptr = find_keyword(label,'l',curfunc);
	if(!doptr)
			{
			mk_lineno(label,if_id,"for");
			doptr = find_keyword(label,'l',curfunc);
			}
	// Now, do we have an address to jump to?
	} while(!doptr); // No..

// Yes, use it
add_opcode(PEVM_Goto);
add_jump(doptr->name);
}

void PE_break(char **line)
{
KEYWORD *doptr;
char label[20];
int if_id,lookback;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

lookback=0;
do  {
	// Read a line number off the ifstack (without changing it)
	if_id = explore_ifstack(lookback++);
	// If there aren't any more, stop
	if(if_id < 0)
		PeDump(srcline,"CONTINUE without DO/WHILE or FOR loop",NULL);

	// Otherwise, see if we can find a matching label for the jump
	mk_lineno(label,if_id,"while");
	doptr = find_keyword(label,'l',curfunc);
	if(!doptr)
			{
			mk_lineno(label,if_id,"next");
			doptr = find_keyword(label,'l',curfunc);
			}
	// Now, do we have an address to jump to?
	} while(!doptr); // No..

// Yes, use it
add_opcode(PEVM_Break);
add_jump(doptr->name);
}


void PE_oChar(char **line)
{
char buf[1024];
int var;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

add_opcode(vmp->opcode);
add_variable(line[1]);

extract_string(buf,line[3]);
var = getnum4char_slow(buf);
if(var<0)
    PeDump(srcline,"Unknown character",buf);
add_number(var);
}

void PE_OChar(char **line)
{
char buf[1024];
int var;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

add_opcode(vmp->opcode);
add_objmember(line[1]);

extract_string(buf,line[3]);
var = getnum4char_slow(buf);
if(var<0)
    PeDump(srcline,"Unknown character",buf);
add_number(var);
}


void PE_cChar(char **line)
{
char buf[1024];
int var;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

add_opcode(vmp->opcode);
add_array(line[1],0);

extract_string(buf,line[3]);
var = getnum4char_slow(buf);
if(var<0)
    PeDump(srcline,"Unknown character",buf);
add_number(var);
}
/*
void PE_fakefunc(char **line)
{
char buf[1024];
KEYWORD *funcptr;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

add_opcode(vmp->opcode);

extract_string(buf,line[1]);
funcptr = find_keyword(buf,'f',NULL);
if(!funcptr)
	PeDump(srcline,"Not a function name",buf);
add_dword(funcptr->id); // This is the function number, compatible with PElist
}
*/

void PE_oSeq(char **line)
{
char buf[1024];
int var;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

add_opcode(vmp->opcode);
add_variable(line[1]);

extract_string(buf,line[2]);
var = getnum4sequence_slow(buf);
if(var<0)
    PeDump(srcline,"Unknown sequence",buf);
add_number(var);
}

void PE_OSeq(char **line)
{
char buf[1024];
int var;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

add_opcode(vmp->opcode);
add_objmember(line[1]);

extract_string(buf,line[2]);
var = getnum4sequence_slow(buf);
if(var<0)
    PeDump(srcline,"Unknown sequence",buf);
add_number(var);
}

void PE_oSeq2(char **line)
{
char buf[1024];
int var;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

add_opcode(vmp->opcode);
add_variable(line[1]);

extract_string(buf,line[3]);
var = getnum4sequence_slow(buf);
if(var<0)
    PeDump(srcline,"Unknown sequence",buf);
add_number(var);
}

void PE_OSeq2(char **line)
{
char buf[1024];
int var;

// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

add_opcode(vmp->opcode);
add_objmember(line[1]);

extract_string(buf,line[3]);
var = getnum4sequence_slow(buf);
if(var<0)
    PeDump(srcline,"Unknown sequence",buf);
add_number(var);
}

void PE_dword(char **line)
{
// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;

add_opcode(vmp->opcode);
add_dword(pe_getnumber(line[1]));
}


void PE_assert(char **line)
{
// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;
PE_generic(line);
add_dword(pe_lineid);
}

void PE_setarray(char **line)
{
int words,ctr, openp, closep, totalcount=0;
char *lineptr,*word,type;
// If we're not doing a full compile, forget it
if(PE_FastBuild)
	return;
PE_checkaccess(line);

type = pe_parm[0];

if(!line[2]) {
	return; // Nothing there
}

if(strcmp(line[2],"=")) {
	PeDump(srcline,"array initialisation didn't have an = as second parameter",line[2]);
}

openp=3;

if(!strchr(line[openp],'(')) {
	PeDump(srcline,"array initialisation list must start with (",line[openp]);
}

closep=0;
lineptr="";
for(ctr=3;line[ctr];ctr++) {
	if(!line[ctr]) {
		break;
	}
	lineptr=line[ctr];
	closep=ctr;
}

// Does the last term have a ')' at the end
if(!lineptr[0] || lineptr[strlen(lineptr)-1] != ')') {
	PeDump(srcline,"Couldn't find closing bracket in array initialisation list",lineptr);
} else {
	lineptr[strlen(lineptr)-1]=0;
}

words=(closep-openp)+1; 

// Check algorithm must match both versions (See below)
for(ctr=0;ctr<words;ctr++) {
	word = line[ctr+openp];
	if(word == NOTHING || word == NULL) {
		continue;
	}
	if(word[0] == '(') {
		word++;
	}

	if(!word[0]) {
		continue;
	}
	totalcount++;
}

// Write number of following items
add_dword(totalcount);


// Check algorithm must match both versions (See above)
for(ctr=0;ctr<words;ctr++) {
	word = line[ctr+openp];
	if(word == NOTHING || word == NULL) {
		continue;
	}
	if(word[0] == '(') {
		word++;
	}

	if(!word[0]) {
		continue;
	}
	switch(type) {
		case 'a':
			add_number(pe_getnumber(word));
			break;
		case 'b':
			add_string(word);
			break;
		case 'c':
			add_variable(word);
			break;
		default:
			char outmsg[2];
			outmsg[0]=type;
			outmsg[1]=0;
			PeDump(srcline,"Unsupported array type",outmsg);
			break;
	}
}
}
