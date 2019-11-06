//
//      Compiler API for implementing a language definition, and using
//      the compiler inside a program
//

#include "../types.h"

struct OPCODE
{
char *mnemonic;
unsigned int opcode;
char *parm;
void (*compile)(char **line);
void (*parse)(char **line);
char operands;
};

struct ICODE
{
int len;
VMTYPE *data;
int srcline;   // line of code referencing this
VMINT variable;  // Variables are treated specially by the code generator
int arraysize;
int jump;      // Is it a jump?
struct KEYWORD *label; // Associated jump label (if any), either src or dest
ICODE *next;
};

struct KEYWORD
{
char name[32];
void *value;
int arraysize;
int id;
int refcount;    // Number of times this variable is used
char type;
const char *local;     // Name of function it's local to
char *localfile; // File it's local to (if any)
int index;	// Base Index within array (used for fast searching)
int keywordid;
bool preconfigured; // Constants can be overridden on the commandline
bool transient;	    // If it's a global it's not saved in the savegame
KEYWORD *next;
};

struct STRUCTURE
{
char *name;			// member name
char type;			// data type
char *access;		// access rights (R/W/none,special)
void *position;		// Address of the member in the Template object
					// The first entry in the structure has the address of
					// the base structure but is othewise blank
void *newspec;
};

struct OBJCODE
	{
	char *name;
	VMTYPE *code;
	int size;
	int codelen;
	char hidden;
	char Class;
	};

// Structure for loading, saving and generally referencing global variables
	
struct GLOBALINT
	{
	char name[48];
	VMINT *ptr;
	VMINT defaultintval;
	bool transient;
	};

struct GLOBALPTR
	{
	char name[48];
	void *ptr;
	bool transient;
	};
	

extern OPCODE vmspec[];
extern STRUCTURE *pe_datatypes[];
extern char pe_vartypes[];
extern char *pe_parm;
extern char *pe_localfile;
extern bool pe_marktransient;
extern OBJCODE *pe_output;
extern int PE_FastBuild;

// Low-level API, mainly for language definition

extern ICODE *add_byte(unsigned char byte);
extern void add_opcode(unsigned int opcode);
extern ICODE *add_dword(VMINT word);
extern ICODE *add_number(VMINT word);
extern void add_string(const char *string);
extern void add_operator(const char *string);
extern void add_variable(const char *name);
extern void add_intmember(const char *name);
extern void add_objmember(const char *name);
extern void add_strmember(const char *name);
extern void add_truestrmember(const char *name);
extern void add_array(const char *name, char t);
extern void add_jump(const char *name);
void mk_lineno(char *str,int line, char *end); // Hash for virtual labels
extern int pe_getnumber(const char *r);        // Get a number (or constant)
extern int pe_getvariable(const char *r);      // Get a number (or constant)
extern int pe_isnumber(const char *r);         // Is the string numeric?
extern int start_icode();
extern int finish_icode();
extern void output_icode();
extern KEYWORD *add_keyword(const char *name, char type, const char *func);
extern KEYWORD *add_symbol_val(const char *name, char type, VMINT value);
extern KEYWORD *add_symbol_ptr(const char *name, char type, void *value);
extern int get_keyword(const char *name, char type, const char *func);
extern KEYWORD *find_keyword(const char *name, char type, const char *func);
extern KEYWORD *find_keyword_global(const char *name, char type, const char *func);
extern KEYWORD **get_klist(char list, int *len);
extern void compiler_sortkeywords();
extern void PeDump(long line, const char *a, const char *b);
extern void extract_string(char *dest, char *src);
extern int read_ifstack();
extern int explore_ifstack(int offset);
extern void push_ifstack(int id);
extern void pop_ifstack();
extern STRUCTURE *pe_getstruct(const char *input);

extern OPCODE *vmp;
extern KEYWORD *labelpending;
extern int srcline,icode_len,pe_numfuncs,pe_lineid;
extern char *curfunc;
extern char *pevm_context;
extern char **pe_outputfuncnames;
extern void **pe_global;
extern int pe_globals;

// Top-level API

extern void compiler_init();
extern void compiler_openfile(char *n);
extern int compiler_parse();
extern void compiler_build(OBJCODE *pe);
extern void compiler_closefile();
extern void compiler_term();

// VM Api

extern void CallVM(char *function);
extern void CallVMnum(int pos);
extern void InitVM();
extern char *CurrentVM();

// Globals API

extern GLOBALPTR *FindGlobalPtr(const char *name);
extern GLOBALINT *FindGlobalInt(const char *name);
extern GLOBALPTR *GetGlobalPtrlist(int *count);
extern GLOBALINT *GetGlobalIntlist(int *count);


// VM data access modes

#define ACC_IMMEDIATE 0
#define ACC_INDIRECT 1
#define ACC_MEMBER 2
#define ACC_ARRAY 4
#define ACC_IMMSTR 8
#define ACC_JUMP 64
#define ACC_OPERATOR 128

