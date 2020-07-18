//
//      Compiler System
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
#define snprintf _snprintf	// 2011 and they still haven't caught up with 1999
#endif

#define ICODE_CACHE
#define FUDGE_CHARACTER -2	// This is for string munging
#define IFSTACK_SIZE 255	// I can't imagine code this complex
//#define LOG_FINDING		// Show all the variable finding for debugging

#include "opcodes.h"
#include "pe_api.hpp"
#include "../init.hpp"
#include "../ithelib.h"
#include "../console.hpp"
#include "../media.hpp"
#include "../sound.h"
#include "../textfile.h"

extern void CheckRefsPtr(void **o);

static char buffer[2048];
static ICODE *icode=NULL;
static ICODE *iccache=NULL;
static char compiler_initlevel=0;
static char thinktype=0;
static char numcheckfatal=1;
STRUCTURE *pe_datatype; // band-aid for supporting multiple structs
int pe_numfuncs=0,pe_lineid=0;
int icode_len=0,srcline=-1;
OPCODE *vmp;
char *curfunc=NULL; // Current function we're looking at (or NULL)
char *pe_parm=NULL;
char *pe_localfile=NULL;

static int pe_gptrs=0;		// List of globals for Garbage Collection
static int pe_gints=0;		// List of integers to preserve
static GLOBALINT *pe_globalint;
static GLOBALPTR *pe_globalptr;
static bool pe_preprocessor=false;

OBJCODE *pe_output;

// variables global to the VM system
int pe_globals=0;
void **pe_global;

extern KEYWORD **klist[256];
extern int klistlen[256];

// Possible first characters for a keyword (optimisation)
static char _keylist[]="abcdefghijklmnopqrstuvwxyz_";

struct OPCODE_cache
	{
	char *name;
	int firstline;
	int lastline;
	OPCODE *ptr;
	};


int start_icode();
int finish_icode();
void output_icode();
static void overlapped_strcpy(char *dest, const char *src);	// To keep valgrind quiet


static void diagnose_keyword(char *name, char type);
ICODE *add_byte(unsigned char byte);
void add_opcode(unsigned int opcode);
ICODE *add_dword(VMINT word);
ICODE *add_number(VMINT word);
void add_string(const char *string);
extern int get_keyword(const char *name, char type, const char *func);
extern KEYWORD *find_keyword(const char *name, char type, const char *func);
extern KEYWORD *find_keyword_global(const char *name, char type, const char *func);
extern KEYWORD *find_keyword_local(const char *name, char type, const char *func);
extern KEYWORD *find_datatype_global(const char *name, const char *func);
extern KEYWORD *find_datatype_local(const char *name, const char *func);
extern KEYWORD *find_variable_global(const char *name, const char *func);
extern KEYWORD *find_variable_local(const char *name, const char *func);

static int next_entry_same(int entry);
static int check_parm(char **line, unsigned int opcode);
static void find_fatalerror(char **line, int entry);
static char *diagnose_type(char t);
static void Purify();
extern int wipe_keywords();
static int get_firstentry(char *ptr);
static int get_firstentry_raw(char *ptr);
static int get_entry(char **line);
static ICODE *new_node();
static int fixup_locals();
static int count_locals();
static void fixup_jumps();
static int code_size();
static void find_orphans();
static int label_pos(KEYWORD *label);
static void show_labels();
static int ifstack[IFSTACK_SIZE];
static KEYWORD *labelstack[IFSTACK_SIZE];
static int if_sp=0;
static int label_sp=0;
static int pe_comment=0;
static char *VM_NextSame=NULL;
static int VM_Items=0;
static OPCODE_cache *VM_Fast=NULL;
static int VM_FastItems=0;

extern int pe_getnumber(char *r);        // Get a number (or constant)
int pe_isnumber(const char *r);                // Is the string numeric?
char *pe_checkstruct(const char *input);		// If it's not a valid struct say why
void pe_buildstruct(const char *input, char type);
void PeDump(long lineno,const char *error,const char *help);
extern int count_quotes(const char *line);
extern int srcline;
 bool pe_marktransient;

static struct TF_S script;
char *compilename="(sweet f-a)"; // You know what that stands for

extern void pe_predefined_symbols();
static void pe_resolve_globals();
static int CompareFastSort(const void *a, const void *b);
static int CompareFastSearch(const void *a, const void *b);


//
//
// User-level API
//
//

// Start the compiler

void compiler_init()
{
int ctr,ptr;

if(compiler_initlevel)
	return;
compiler_initlevel=1;

// Build some fast lookuptables
for(VM_Items=0;vmspec[VM_Items].mnemonic;VM_Items++);
VM_NextSame = (char *)calloc(VM_Items,sizeof(char));
if(!VM_NextSame)
	{
	compiler_initlevel=0;
	return;
	}

// Work out which mnemonics are consecutive
for(ctr=1;ctr<VM_Items;ctr++)
	if(vmspec[ctr].mnemonic && vmspec[ctr-1].mnemonic)
		if(!istricmp(vmspec[ctr-1].mnemonic,vmspec[ctr].mnemonic))
			VM_NextSame[ctr-1]=1;

// Now build a list sorted by keyword
VM_Fast = (OPCODE_cache *)calloc(VM_Items,sizeof(OPCODE_cache));
if(!VM_Fast)
	{
	free(VM_NextSame);
	compiler_initlevel=0;
	return;
	}

// Build the list
ptr=0;
VM_FastItems=0;
do
	{
	VM_Fast[VM_FastItems].name=vmspec[ptr].mnemonic;
	VM_Fast[VM_FastItems].ptr=&vmspec[ptr];
	// Find the beginning
	VM_Fast[VM_FastItems].firstline=get_firstentry_raw(vmspec[ptr].mnemonic);
	// Find the end
	for(ctr=VM_Fast[VM_FastItems].firstline;next_entry_same(ctr);ctr++);
	VM_Fast[VM_FastItems].lastline=ctr;
	ptr=ctr+1;
	
	VM_FastItems++;
	} while(ptr<VM_Items);

// And sort it
qsort(VM_Fast,VM_FastItems,sizeof(OPCODE_cache),CompareFastSort);

ilog_quiet("Last Opcode = %d (%x)\n",PEVM_LastOp,PEVM_LastOp);

pe_datatype = NULL; // Against Demons

// Add these reserved symbols to the compiler

add_keyword("<>",'E',NULL);
add_keyword("!=",'E',NULL);
add_keyword("=",'e',NULL);
add_keyword("==",'e',NULL);
add_keyword("=",'=',NULL);

pe_predefined_symbols();
}


int CompareFastSort(const void *a, const void *b)
{
OPCODE_cache *ax=(OPCODE_cache*)a,*bx=(OPCODE_cache*)b;
return istricmp(ax->name,bx->name);
}

int CompareFastSearch(const void *key, const void *member)
{
OPCODE_cache *ptr=(OPCODE_cache*)member;
return istricmp((char *)key,ptr->name);
}

// open a file for compilation

void compiler_openfile(char *fname)
{
if(!fname)
	ithe_panic("compiler_openfile passed NULL filename",NULL);

TF_init(&script);
compilename=fname;
TF_load(&script,fname);
Purify();
TF_splitwords(&script,'#'); // Break into words for faster processing later
}

// pre-parser (get variable, function keywords, labels..)

int compiler_parse()
{
int lines,lineno,entry,words;
char **line;

//char ptr[2048];

pe_numfuncs=0;
pe_lineid=0;
pe_localfile=NULL;
pe_marktransient=false;
pe_preprocessor=true;

lines = script.lines;
for(lineno=0;lineno<lines;lineno++)
	{
	pe_lineid++;
	srcline=lineno;
	words=script.linewords[lineno]; // Get no of words on this line
	line=script.lineword[lineno]; // pointer to line

	if(words == 0)
		continue; // skip blank line

	// Look for comment start
	if(strstr(line[0],"/*"))
		{
		pe_comment=1;
		continue;
		}

	if(pe_comment)
		{
		// Hack to deactivate comments
		if(strstr(line[0],"*/"))
			pe_comment=0;
		continue;
		}

/*
    quotes = count_quotes(line);
    if(quotes & 1)
        PeDump(lineno,"Missing \" in string",line);
*/

	entry = get_firstentry(line[0]); // No need for full syntax matching
	if(entry == -1)
		PeDump(lineno,"Unknown command",line[0]);

	if(vmspec[entry].parse)
		vmspec[entry].parse(line);
	}

if(pe_localfile != NULL)
	PeDump(lineno,"Missing endlocal statement in file",NULL);
if(pe_marktransient)
	PeDump(lineno,"Missing endtransient statement in file",NULL);

pe_preprocessor=false;

return pe_numfuncs;
}

//      Compile a single file

void compiler_build(OBJCODE *pe)
{
int lines,lineno,entry,words;
char **line;

pe_output = pe;
pe_lineid=0;
pe_preprocessor=false;

pe_resolve_globals();	// only needs to be called once, but makes API cleaner

lines = script.lines;
for(lineno=0;lineno<lines;lineno++)
	{
	pe_lineid++;
	srcline=lineno;
	words=script.linewords[lineno]; // Get no of words on this line
	line=script.lineword[lineno]; // pointer to line

	if(words == 0)
		continue; // skip blank line

	if(strstr(line[0],"/*"))
		{
		pe_comment=1;
		continue;
		}
	if(pe_comment)
		{
		// Hack to deactivate comments
		if(strstr(line[0],"*/"))
			pe_comment=0;
		continue;
		}

	// Find the possible matches for this command and check them in turn
	if(PE_FastBuild)
		entry = get_firstentry(line[0]); // No need for full syntax matching
	else
		entry = get_entry(line);	// Full compilation
	if(entry == -1)
		PeDump(lineno,"Uh!",line[0]);

	// Safety check before we make the call

	pe_parm = vmspec[entry].parm;
	if(vmspec[entry].compile)
		vmspec[entry].compile(line);
	else
		PeDump(lineno,"Compiler error, command has no function",line[0]);
	}
}


// close a file that has been compiled or pre-parsed

void compiler_closefile()
{
TF_term(&script);
compilename="sweet f-a";
}

// Stop the compiler

void compiler_term()
{
if(!compiler_initlevel)
	return;
	
wipe_keywords();

free(VM_NextSame);
VM_NextSame=NULL;

free(VM_Fast);
VM_Fast=NULL;

compiler_initlevel=0;
}


char *pe_get_function(unsigned int opcode)
{
int ctr;
char *msg;

for(ctr=0;vmspec[ctr].mnemonic;ctr++)
	if(vmspec[ctr].opcode == opcode)
		{
		msg = strLocalBuf();
		msg[0]=vmspec[ctr].operands;
		strcpy(&msg[1],vmspec[ctr].mnemonic);
		return msg;
		}

return "\0Invalid Opcode";
}


//
//
// Language developer API
//
//

// Prepare for code generation

int start_icode()
{
if(icode)       // Haven't finished!
    return 0;

#ifdef LOG_FINDING
ilog_quiet("Start function %s\n",curfunc);
#endif

return 1;
}

// Add a byte to the icode

ICODE *add_byte(unsigned char byte)
{
ICODE *i;
VMTYPE data;
// Format conversion
data.ptr=NULL;
data.u8=byte;
i = new_node();

//i->len = sizeof(unsigned char);
i->len=sizeof(data);
i->data = (VMTYPE *)M_get(1,i->len);
memcpy(i->data,&data,i->len);
i->variable=0;
i->jumplabel=NULL;
i->label=NULL;
i->labels=0;

return i;
}

// Add a dword to the icode

ICODE *add_dword_raw(VMINT word)
{
ICODE *i;
VMTYPE data;
// Format conversion
data.ptr=NULL;
data.i32=word;

i = new_node();

i->len = sizeof(data);
i->data = (VMTYPE *)M_get(1,i->len);
memcpy(i->data,&data,i->len);
i->variable=0;
i->jumplabel=NULL;
i->label=NULL;
i->labels=0;
return i;
}

// Add a dword to the icode

ICODE *add_dword(VMINT word)
{
ICODE *i;
VMTYPE data;
// Format conversion
data.ptr=NULL;
data.i32=word;

add_byte(ACC_IMMEDIATE); // Immediate address mode

i = new_node();

i->len = sizeof(data);
i->data = (VMTYPE *)M_get(1,i->len);
memcpy(i->data,&data,i->len);
i->variable=0;
i->jumplabel=NULL;
i->label=NULL;
i->labels=0;
return i;
}

// Add an opcode to the icode

void add_opcode(unsigned int opcode)
{
int labels;
ICODE *i;
VMTYPE data;
// Format conversion
data.ptr=NULL;
data.u32=opcode;

i = new_node();

i->len = sizeof(data);
i->data = (VMTYPE *)M_get(1,i->len);
memcpy(i->data,&data,i->len);
i->variable=0;
i->jumplabel=NULL;
i->label=NULL;
i->labels=0;

labels = sizeof_labelstack();
if(labels > 0) {
	i->label = (KEYWORD **)M_get(sizeof(KEYWORD *),labels);
	for(int ctr=0;ctr<labels;ctr++) {
		i->label[ctr] = read_label();
		pop_label();
//		ilog_quiet("%s: Writing label %s\n",curfunc,i->label[ctr]);
	}
	i->labels=labels;
}
}

// Add a raw integer to the icode

ICODE *add_number(VMINT word)
{
ICODE *i;
VMTYPE data;
// Format conversion
data.ptr=NULL;
data.i32=word;

add_byte(ACC_IMMEDIATE); // Immediate address mode

i = new_node();

i->len = sizeof(data);
i->data = (VMTYPE *)M_get(1,i->len);
memcpy(i->data,&data,i->len);
i->variable=0;
i->jumplabel=NULL;
i->label=NULL;
i->labels=0;

return i;
}


// Add a string to the icode

void add_string(const char *string)
{
ICODE *i;
int vstrlen; // virtual string length
int divstring;

// Get string length
vstrlen=strlen(string)+1;

// Now find out if it is divisible by a word and pad it if necessary
divstring = vstrlen/sizeof(VMTYPE);
if(vstrlen & (sizeof(VMTYPE)-1))
	{
	divstring++;
	vstrlen=divstring*sizeof(VMTYPE);
	}


add_byte(ACC_IMMSTR); // Immediate addressing mode
add_number(vstrlen); // Write padded string length
i = new_node();
i->len = vstrlen;
i->data = (VMTYPE *)M_get(1,i->len);
memcpy(i->data,string,strlen(string)+1);  // Copy unpadded string
i->variable=0;
i->jumplabel=NULL;
i->label=NULL;
i->labels=0;
}

// Reserve a variable

void add_variable(const char *name)
{
ICODE *i;
KEYWORD *k_local,*k_global;
VMTYPE data;
data.ptr=NULL;

add_byte(ACC_INDIRECT); // Indirect addressing mode

i = new_node();
i->jumplabel=NULL;
i->label=NULL;
i->labels=0;

// Find the keyword for this variable, so we can extract it's ID number,
// and store that in the i-code for fixup later on

k_local = find_variable_local(name,curfunc);
k_global = find_variable_global(name,curfunc);
//k_global = find_variable_global(name,NULL); // NULL will cause Bad Things

// This probably means that the symbol exists but in another scope
if(!k_local && !k_global)
	PeDump(srcline,"AV: Undefined symbol",name);

// Make adjustments for local/global type

if(k_local)
	{
	i->variable = k_local->id+1;
	data.ptr = k_local->value;
	k_local->refcount++;
	}
else
	{
	data.ptr = k_global->value;
	i->variable=0; // This is 'not a variable'
	}


// Ok, store the data

i->len = sizeof(data);
i->data = (VMTYPE *)M_get(1,i->len);
memcpy(i->data,&data,i->len);
}

// Reserve a member variable

void add_member(const char *name, char type)
{
ICODE *i,*acc;
KEYWORD *k_local,*k_global;
char temp[1024];
char *ptr,isarray;
VMTYPE data,d;
d.ptr=NULL;
data.ptr=NULL; // No data yet

acc = add_byte(ACC_MEMBER); // Member addressing mode
isarray=0;

// Find the keyword for this variable, so we can extract it's ID number,
// and store that in the i-code for fixup later on

k_local = NULL;

/*
 * Find the base object
 */

strcpy(temp,name);

ptr = strchr(temp,'.');
if(!ptr)
	ithe_panic("Parser fault in add_member",name);
*ptr=0;

// Is it an array?

ptr = strchr(temp,'[');
if(ptr)
	{
	*ptr=0;
	isarray=1;
	}

k_local = find_datatype_local(temp,curfunc);
k_global = find_datatype_global(temp,curfunc);

// Panic if nothing found

if(!k_local && !k_global)
	PeDump(srcline,"AM: Undefined symbol",temp);

// Otherwise, make some adjustments and store it

if(isarray)
	{
	// Set some flags up
	d.u8 = ACC_MEMBER | ACC_ARRAY;
	*acc->data=d; // Poke them in
	add_array(name,1);
	pe_buildstruct(name,type);
	return;
	}


i = new_node();
i->jumplabel=NULL;
i->label=NULL;
i->labels=0;

if(k_local)
	{
	i->variable = k_local->id+1;
	data.ptr = k_local->value;
	k_local->refcount++;
	}
else
	{
	data.ptr = k_global->value;
	i->variable=0; // This is 'not a variable'
	}

// Ok, store the data

i->len = sizeof(data);
i->data = (VMTYPE *)M_get(1,i->len);
memcpy(i->data,&data,i->len);

pe_buildstruct(name,type);
}


// Reserve a member variable

void add_intmember(const char *name)
{
add_member(name,'i');
}

// Reserve a member variable

void add_objmember(const char *name)
{
add_member(name,'o');
}

// Reserve a member variable

void add_strmember(const char *name)
{
add_member(name,'s');
}

// Reserve an array element access

void add_array(const char *name, char member)
{
ICODE *i;
KEYWORD *k_local,*k_global,*var;
VMTYPE data;
char temp[1024];
char *ptr;
char *idx;
data.ptr=NULL;

if(!member)
	add_byte(ACC_ARRAY); // Array addressing mode if needed

i = new_node();
i->jumplabel=NULL;
i->label=NULL;
i->labels=0;

// Find the keyword for this variable, so we can extract it's ID number,
// and store that in the i-code for fixup later on

k_local = NULL;

// Find the base object

strcpy(temp,name);
ptr = strchr(temp,'[');
if(!ptr)
	ithe_panic("Parser fault in add_array: missing '['",name);
*ptr=0;

// Find the index

ptr++; // This must be the index of the array

idx = ptr;
ptr = strrchr(idx,']');
if(!ptr)
	ithe_panic("Parser fault in add_array: missing ']'",name);
*ptr=0;


k_local = find_keyword_local(temp,'?',curfunc);
k_global = find_keyword_global(temp,'?',curfunc);

// Panic if nothing found

if(!k_local && !k_global)
//	PeDump(srcline,"Can't find this array:",temp);
    PeDump(srcline,"Internal error 001",temp);

// Otherwise, make some adjustments and store it

if(k_local)
	{
	i->variable = k_local->id+1;
	data.ptr = k_local->value;
//	ilog_quiet("%s: localval = %p, id = %d, data.ptr=%p   idx offset = %d\n",k_local->name,k_local->value,k_local->id, data.ptr,atoi(idx)-1);
	k_local->refcount++;
	i->arraysize= k_local->arraysize;
	}
else
	{
	data.ptr = k_global->value;
//	ilog_quiet("Array base %x\n",data);
	i->variable=0; // This is 'not a variable'
	i->arraysize= k_global->arraysize;
	}

// Ok, store the base object

i->len = sizeof(data);
i->data = (VMTYPE *)M_get(1,i->len);
memcpy(i->data,&data,i->len);

/*
if(k_local)
	ilog_quiet("Writing %x for local array %s\n",&data,k_local->name);
else
	ilog_quiet("Writing %x for global array %s\n",&data,k_global->name);
*/

add_dword(i->arraysize); // Write the size of the array into the i-code

// Now we need to build the index data

// Is it a number?
if(pe_isnumber(idx))
	{
	add_number(atoi(idx));
	return;
	}

// Okay, assume it's a variable of some kind

k_local = find_keyword_local(idx,'?',curfunc);
k_global = find_keyword_global(idx,'?',curfunc);

// Panic if nothing found

if(!k_local && !k_global)
    PeDump(srcline,"Internal error 002",name);

// Otherwise, make some adjustments and store it

if(k_local)
	var = k_local;
else
	var = k_global;

switch(var->type)
	{
	case 'i':
	add_variable(var->name);
	break;

	case 'I':
	add_member(var->name,'i');
	break;

	case 'o':
	add_variable(var->name);
	break;

	case 'O':
	add_member(var->name,'o');
	break;

	case 'P':
	add_variable(var->name);
	break;

//	case 's':					// Shouldn't happen..?
//	add_variable(var->name);
//	break;

	case 'S':
	add_member(var->name,'s');
	break;

	// Oh wonderful
	case 'a':
	add_array(var->name,0);
	break;

	default:
	ilog_quiet("Unsupported datatype '%c'\n",var->type);
	PeDump(srcline,"Unsupported format: check bootlog and email me",var->name);
	break;
	};

//	ilog_quiet("added %s of type '%c'\n",var->name,var->type);

}


// Debug array if errors found

char *pe_checkarray(const char *name)
{
KEYWORD *k_local,*k_global,*var;
char temp[1024];
char *ptr;
char *idx;
char *msg;

msg = strLocalBuf();

k_local = NULL;

// Find the base object

strcpy(temp,name);
ptr = strchr(temp,'[');
if(!ptr)
	{
	strcpy(msg,"Array is missing start bracket '['");
	return msg;
	}
*ptr=0;

// Find the index

ptr++; // This must be the index of the array

idx = ptr;
ptr = strrchr(idx,']');
if(!ptr)
	{
	strcpy(msg,"Array is missing end bracket ']'");
	return msg;
	}
*ptr=0;

k_local = find_datatype_local(temp,curfunc);
k_global = find_datatype_global(temp,curfunc);

// Panic if nothing found

if(!k_local && !k_global)
	{
	sprintf(msg,"Can't find base variable '%s' in array in function '%s'",temp,curfunc);
	return msg;
	}

// Is it a number?
if(!pe_isnumber(idx))
	{
	sprintf(msg,"index '%s' in array is not a number",idx);
	return msg;
	}

// Okay, assume it's a variable of some kind

k_local = find_datatype_local(idx,curfunc);
k_global = find_datatype_global(idx,curfunc);

// Panic if nothing found

if(!k_local && !k_global)
	{
	sprintf(msg,"Can't understand array index '[%s]' in array",idx);
	return msg;
	}

if(k_local)
	var = k_local;
else
	var = k_global;

switch(var->type)
	{
	case 'i':
	case 'I':
	case 'o':
	case 'O':
	case 'P':
	case 'S':
	case 'a':
	; // Do Nothing
	break;

	default:
	sprintf(msg,"Array index is unsupported type '%s'\n",diagnose_type(var->type));
	return msg;
	break;
	};

return NULL;
}


// Check the availability of the specified type for given keyword

int pe_checkarray_type(const char *input, char type)
{
char temp[1024];
char term[1024];

char *ptr;
KEYWORD *k;

if(pe_preprocessor) {
	return 1;
}

strcpy(temp,input);
if(!strchr(temp,'['))
	ithe_panic("Parser fault at <1> in pe_checkarray_type",input);

ptr=strchr(temp,'[');
*ptr=0; // Remove the separator

// If it's a variable, try to find it
k = find_datatype_local(temp,curfunc);
if(!k) {
	k = find_datatype_global(temp,curfunc);
}
if(!k) {
	k = find_datatype_global(temp,NULL);
}
if(!k) {
//	printf(">> variable '%s' not found!\n", temp);
	return 0;
}

if(k->type == type) {
	return 1;
}

//printf("!!! Wanted '%c' but got '%c'\n", type,k->type);

return 0;
}



// Reserve a jump (has to be patched later)

void add_jump(const char *name)
{
ICODE *i;
KEYWORD *kptr;
VMTYPE data;

add_byte(ACC_JUMP);

i = new_node();
i->variable=0;

// Find the keyword for the appropriate label, so we can extract it's ID
// and store that in the i-code for fixup later on

kptr = find_keyword(name,'l',curfunc);
if(!kptr)
    PeDump(srcline,"Label not known",name);

data.ptr=NULL; // No data yet
i->jumplabel = kptr; // This is the label for future fixups

i->len = sizeof(data);
i->data = (VMTYPE *)M_get(1,i->len);
memcpy(i->data,&data,i->len);
}

// Get the opcode for a math operator

void add_operator(const char *name)
{
KEYWORD *kptr;
unsigned char byte;

// Find the keyword for the appropriate symbol, so we can extract it's ID
kptr = find_keyword(name,'p',curfunc);
if(!kptr)
    PeDump(srcline,"operator not known",name);

byte = (*(unsigned char*)(&kptr->value)) | ACC_OPERATOR; // Add the high bit to mark it as an operator
add_byte(byte);
}


// extract the string from the quotes casing it

void extract_string(char *buf, char *line)
{
char *p;

p = strchr(line,'\"');
if(!p)
    PeDump(srcline,"Missing first \" in string",line);

strcpy(buf,p+1);
p = strchr(buf,'\"');
if(!p)
    PeDump(srcline,"Missing ending \" in string",line);
*p=0;
}

// pe_getnumber - Get the value of a number or a constant

int pe_getnumber(const char *number)
{
KEYWORD *k;
int hex;

// Look for a constant
k = find_keyword(number,'n',curfunc);
if(k)
	return((VMINT)k->value);

// Is it hexadecimal?
if(number[0] == '0')
	if(number[1] == 'x')
		{
		sscanf(&number[2],"%x",&hex);
		return hex;
		}

// Is it an ascii character?
if(number[0] == '\'')
	if(number[1])
		if(number[2] == '\'')
			return number[1];

// Try decimal number
return(atoi(number));
}

// pe_isnumber - is the string a number?

int pe_isnumber(const char *thing)
{
const char *p;
char ok;
KEYWORD *k;
// List of valid characters for hexadecimal
char validhex[]="0123456789abcdefABCDEF";

if(!thing)
	return 0; // No it bloody well isn't!

thinktype=0;

// Is is a constant?
k = find_keyword(thing,'n',curfunc);
if(k)
	{
	thinktype='n';
	return(1);
	}
k = find_keyword_global(thing,'n',NULL);
if(k)
	PeDump(srcline,"Local constant used outside scope",thing);

// Is it a hexadecimal number?
if(thing[0] == '0')
	if(thing[1] == 'x')
		{
		ok=1;
		// Try each character of the word and see if it is valid
		for(p=&thing[2];*p;p++)
			if(!strchr(validhex,*p))
				ok=0;
		if(ok)
			{
			thinktype='n';
			return 1;
			}
		}

// Is it an ascii character?
if(thing[0] == '\'')
	if(thing[1])
		if(thing[2] == '\'')
			{
			thinktype='n';
			return 1;
			}

// Is it a valid number?
ok=1;
for(p=thing;*p;p++)
	if((*p<'0'||*p>'9')&&*p!='-'&&*p!='+')
		ok=0;

if(ok)
	{
	thinktype='n';
	return 1;
	}

// Is it a keyword of any form or kind at all?

k = find_keyword_global(thing,'?',NULL);
if(k)
	{
	thinktype=k->type;
	return 0;
	}

// Is it a string?

if(count_quotes(thing) == 2)
	{
	thinktype='s';
	return 0;
	}

// Is it a structure access?

if(strchr(thing,'.'))
	{
	thinktype='.';
	return 0;
	}

// Is it an array access?

if(strchr(thing,'['))
	{
	thinktype=']';
	return 0;
	}

// Is it a table?

if(getnum4table_slow(thing) != -1)
	{
	thinktype='T';
	return 0;
	}

// It must be a typo then

if(numcheckfatal)
	PeDump(srcline,"IN: Undefined symbol",thing);
return 0;
}

// check for valid structure member

char *pe_checkstruct(const char *input)
{
char temp[1024];
char base[1024];
char term[1024];
char *msg,*ptr;
int ctr,ok;
STRUCTURE *mystruct;
KEYWORD *k;

msg = strLocalBuf();

strcpy(temp,input);
if(!strchr(temp,'.'))
	{
	strcpy(msg,"Missing '.' or silly input");
	return msg;
	}

ptr=strchr(temp,'.');
*ptr=0; // Remove the separator
strcpy(base,temp);
overlapped_strcpy(temp,&ptr[1]);

// If it's an array, get the base object

ptr = strchr(temp,'[');
if(ptr)
	*ptr=0;
ptr = strchr(base,'[');
if(ptr)
	*ptr=0;

// Make sure the variable which is being used as a struct is one

k = find_datatype_local(base,curfunc);
if(!k)
	{
	k = find_datatype_global(base,NULL);
	if(!k)
		{
		ilog_quiet("(%s).(%s) of (%s)\n",base,temp,input);
        sprintf(msg,"%s is being used as a structure",base);
		return msg;
		}
	}

mystruct=pe_datatype;

do	{
	strcpy(term,temp);
	ptr=strchr(term,'.');
	if(ptr)
		*ptr=0;

	ok=0;
	for(ctr=1;mystruct[ctr].name;ctr++)
		{
		if(!istricmp(mystruct[ctr].name,term))
			{
			if(mystruct[ctr].type == '>')
				{
				if(strchr(temp,'.'))	// Check we're not the terminus
					{
					ok=1;
					mystruct=(STRUCTURE *)mystruct[ctr].newspec;
					break;
					}
				else	// Agh!  The user is accessing a struct as an integer
					return NULL; // They might be doing a NULL pointer check
				}
			return NULL;
			}
		}

	if(!ok)
		{
		sprintf(msg,"Unrecognised member '%s' of type '%s'",term,mystruct[0].name);
		return msg;
		}

	ptr=strchr(temp,'.');
	if(ptr)
		*ptr=0; // Remove the separator
	overlapped_strcpy(temp,&ptr[1]);
	} while(ptr);

PeDump(srcline,"Internal error 003",input);

return NULL;
}

// Check the availability of the specified type for given keyword

int pe_checkstruct_type(const char *input, char type)
{
char temp[1024];
char term[1024];

char *ptr;
int ctr,ok;
STRUCTURE *mystruct;

strcpy(temp,input);
if(!strchr(temp,'.'))
	ithe_panic("Parser fault at <1> in pe_checkstruct_type",input);

ptr=strchr(temp,'.');
*ptr=0; // Remove the separator
overlapped_strcpy(temp,&ptr[1]);

mystruct=pe_datatype;

do	{
	strcpy(term,temp);
	ptr=strchr(term,'.');
	if(ptr)
		*ptr=0;
	ok=0;
	for(ctr=1;mystruct[ctr].name;ctr++) {
		if(!istricmp(mystruct[ctr].name,term)) {
			if(mystruct[ctr].type == type) {
				return 1; // Found exact match
			}
			if(mystruct[ctr].type == '>') {
				if(strchr(temp,'.')) {	// Check we're not the terminus
					ok=1;
					mystruct=(STRUCTURE *)mystruct[ctr].newspec;
					break;
				}
			}
		}
	}

	if(!ok)
		return 0;

	ptr=strchr(temp,'.');
	if(ptr)
		*ptr=0; // Remove the separator
	overlapped_strcpy(temp,&ptr[1]);
} while(ptr);

return 0;
}

// Walk the structure and return the terminus, e.g. so we can check access rights

STRUCTURE *pe_getstruct(const char *input) {
char temp[1024];
char term[1024];
char base[1024];
char *ptr;
int ctr,ok;
STRUCTURE *mystruct;
KEYWORD *k;

strcpy(temp,input);
if(!strchr(temp,'.')) {
	return NULL;
}

ptr=strchr(temp,'.');
*ptr=0; // Remove the separator
strcpy(base,temp);
overlapped_strcpy(temp,&ptr[1]);

// If it's an array, get the base object

ptr = strchr(temp,'[');
if(ptr)
	*ptr=0;
ptr = strchr(base,'[');
if(ptr)
	*ptr=0;

// Make sure the variable which is being used as a struct is one

k = find_datatype_local(base,curfunc);
if(!k) {
	k = find_datatype_global(base,NULL);
	if(!k) {
		return NULL;
	}
}

mystruct=pe_datatype;

do	{
	strcpy(term,temp);
	ptr=strchr(term,'.');
	if(ptr)
		*ptr=0;
	ok=0;
	for(ctr=1;mystruct[ctr].name;ctr++) {
		if(!istricmp(mystruct[ctr].name,term)) {
			if(mystruct[ctr].type == '>') {
				if(strchr(temp,'.')) {	// Check we're not the terminus
					ok=1;
					mystruct=(STRUCTURE *)mystruct[ctr].newspec;
					break;
				}
			} else {
				return &mystruct[ctr];
			}
		}
	}

	if(!ok)
		return NULL;

	ptr=strchr(temp,'.');
	if(ptr)
		*ptr=0; // Remove the separator
	overlapped_strcpy(temp,&ptr[1]);
} while(ptr);

return NULL;
}



// Build the structure dereference chain

void pe_buildstruct(const char *input, char type)
{
char temp[1024];
char term[1024];
unsigned char refs;
ICODE *no;
STRUCTURE *mystruct;
VMTYPE d;
d.ptr=NULL;

char *ptr;
int ctr,ok;


//ilog_quiet("[%s] '%c'\n",input,type);

mystruct = pe_datatype; // Hack for supporting multiple struct formats

strcpy(temp,input);
if(!strchr(temp,'.'))
	ithe_panic("Parser fault at <1> in pe_buildstruct",input);

ptr=strchr(temp,'.');
*ptr=0; // Remove the separator
overlapped_strcpy(temp,&ptr[1]);

no = add_byte(0); // Reserve space for the number of references

refs=0;

do	{
	strcpy(term,temp);
	ptr=strchr(term,'.');
	if(ptr)
		*ptr=0;
//	ilog_quiet("(%s)\n",term);
	ok=0;
	for(ctr=1;mystruct[ctr].name;ctr++)
		{
		if(!istricmp(mystruct[ctr].name,term))
			{
//			printf("cmp: '%c'%c'\n",mystruct[ctr].type,type);
			if(mystruct[ctr].type == type)
				{
				if(!strchr(temp,'.'))	// Check we are the terminus
					{
					// Calculate offset and store it
					add_dword_raw((char *)mystruct[ctr].position - (char *)mystruct[0].position);
					if(refs>254)
						ithe_panic("Structure nested too deep",input);
					d=*no->data; // Get refs
					d.u8=++refs; // update refs
					*no->data=d;	// Poke it back in
					return;
					}
				}
			if(mystruct[ctr].type == '>')
				if(strchr(temp,'.'))	// Check we're not the terminus
					{
					ok=1;
					// Calculate offset and store it
					add_dword_raw((char *)mystruct[ctr].position - (char *)mystruct[0].position);
					refs++;
					if(refs>254)
						ithe_panic("Structure nested too deep",input);
					d=*no->data; // Get refs
					d.u8=refs; // update refs
					*no->data=d;	// Poke it back in
//					*(char *)no->data=refs;	// Update refs counter
					// Enter the new sub-structure
					mystruct=(STRUCTURE *)mystruct[ctr].newspec;
					break;
					}
			}
		}

	if(!ok)
		{
		ilog_quiet("input = '%s'\n",input);
		ilog_quiet("temp='%s' term='%s'\n",temp,term);
//		ithe_panic("Parser fault at <2> in pe_buildstruct",input);
		PeDump(srcline,"Member not known for this structure",term);
		}

	ptr=strchr(temp,'.');
	if(ptr)
		{
		*ptr=0; // Remove the separator
		overlapped_strcpy(temp,&ptr[1]);
		}
	} while(ptr);

ithe_panic("Parser fault at <3> in pe_buildstruct",input);

return;
}


// output the generated code to pe_outputcode array

void output_icode()
{
ICODE *i;
VMTYPE *p,*p1;
VMTYPE d;
int icode_data,patches,locals;
d.ptr=NULL;

// Calculate the length of the finished code

icode_len = code_size();

// Data starts after code finishes
icode_data=icode_len;

find_orphans(); // Abort if local variables are declared and never used

locals = count_locals(); // Get number of variable slots we'll need

patches = fixup_locals();
if(patches)
	{
	icode_len = code_size(); // Recalculate after data added
	// Reserve space for Dofus opcode, and 2 parameters
	icode_len+=(sizeof(VMTYPE)*3);
	icode_data+=(sizeof(VMTYPE)*3);
	icode_len+=(locals * sizeof(VMTYPE *)); // Allocate the slots
	}

fixup_jumps();

if(!icode_len)
	ithe_panic(curfunc,"No data!");

// Allocate space for the code+data
p = (VMTYPE *)M_get(1,icode_len);

pe_output->code = p;
pe_output->size = icode_len;
pe_output->codelen = icode_data;

// Insert the DOFUS linker into the output code if it is needed

if(patches) {
//	ilog_quiet("Building DOOFUS in %s: %d patches from %x onwards\n",compilename,patches,icode_data);
	// Insert Dofus opcode 
	d.u32=PEVM_Dofus;
	*p++=d;

	// Insert Dofus counter
	d.i32=patches;
	*p++=d;

	// Insert Dofus offset
	d.i32=icode_data;
	*p++=d;
}

p1 = p;	// For debugging if necessary

// Copy the icode to the pointer

for(i=icode;i;i=i->next) {
	memcpy(p,i->data,i->len);
	p=(VMTYPE *)((char *)p+i->len); // advance pointer in bytes
//	p+=i->len;
//	ilog_quiet("%04x: [%x] (%d bytes long)\n",(unsigned)p-(unsigned)p1,*(int *)i->data,i->len);
}

pe_output++;
}

// Delete the code after generation and output

int finish_icode()
{
ICODE *i,*in;

if(!icode)      // Haven't started!
	return 0;

i = icode;      // Traverse the list, erasing as you go

do {
	in = i->next;
	M_free(i->data);
	if(i->label) {
		M_free(i->label);
	}
	i->next = NULL;
	M_free(i);
	i = in;
} while(i); // When i=NULL we've finished

icode = NULL;
iccache = NULL;
return 1;
}

//
//
// Internal System (not for external use)
//
//


//  Create a new node in the icode list

ICODE *new_node()
{
ICODE *i;
ICODE **newnode;

// Find the place for the new node

if(!icode)
	{
	newnode = (ICODE **)&icode;
	iccache=NULL;
	}
else
	{
#ifdef ICODE_CACHE
	if(!iccache)
		{
#endif
		newnode = NULL;
		for(i=icode;i;i=i->next)
			if(i->next == NULL)
				{
				newnode = &i->next;
				iccache = (ICODE *)newnode;
				}
#ifdef ICODE_CACHE
		}
	else
		newnode=&iccache->next;
#endif
	}


if(!newnode)
	ithe_panic("I have lost the unobtainable",NULL);

// Make the new node in the linked list

*newnode = (ICODE *)M_get(1,sizeof(ICODE));
i=*newnode;
i->next = NULL;
iccache=(ICODE *)i;
return i;
}

// Get the first instance of a keyword in the language definition
// This is slow and is only used to build the lookuptable at the start

int get_firstentry_raw(char *ptr)
{
int ctr;
for(ctr=0;vmspec[ctr].mnemonic;ctr++)
    if(!istricmp(vmspec[ctr].mnemonic,ptr))
        {
        vmp = &vmspec[ctr];
        return ctr;
        }
return -1;
}

// Optimised version

int get_firstentry(char *ptr)
{
OPCODE_cache *o;
o=(OPCODE_cache *)bsearch(ptr,VM_Fast,VM_FastItems,sizeof(OPCODE_cache),CompareFastSearch);
if(!o)
	return -1;
	
return o->firstline;
}

// Find out if the next keyword is the same (with different parameters)
// This is slow and is only used to build the lookuptable at the start

int next_entry_same(int entry)
{
if(entry < 0 || entry >= VM_Items)
	return 0;

return VM_NextSame[entry];
/*
// Is the next definition the end marker?
if(vmspec[entry+1].mnemonic == NULL)
    return 0;
// Is the next definition for the same name?
if(istricmp(vmspec[entry].mnemonic,vmspec[entry+1].mnemonic))
    return 0;
return 1;
*/
}

// Get a keyword in the language definition and return the best fit

int get_entry(char **line)
{
int firstentry,lastentry,entry,len,ctr;
char *buf,buffer[1024];
OPCODE_cache *ptr;


// Find first matching entry, or die trying
/*
firstentry = get_firstentry(line[0]);
if(firstentry == -1)
	PeDump(srcline,"Unknown command",line[0]);

// Now look for the last matching entry
for(lastentry=firstentry;next_entry_same(lastentry);lastentry++);
*/

ptr=(OPCODE_cache *)bsearch(line[0],VM_Fast,VM_FastItems,sizeof(OPCODE_cache),CompareFastSearch);
if(!ptr)
	PeDump(srcline,"Unknown command",line[0]);

firstentry=ptr->firstline;
lastentry=ptr->lastline;
vmp=ptr->ptr;

// Now find the best fit

for(entry = firstentry;entry<=lastentry;entry++)
	{
//	ilog_quiet("CheckEntry '%s' trying(%s)\n",line[1],vmspec[entry].parm);
	if(check_parm(line,entry))
		{
		vmp = &vmspec[entry];
//		ilog_quiet("Matched\n");
		return entry;
		}
	}

// No matches, crash out

if(firstentry == lastentry)
	find_fatalerror(line,entry-1);

ilog_quiet("No match found for '%s' at line %d\n",line[0],srcline);
ilog_quiet("Possible candidates:\n");
for(entry = firstentry;entry<=lastentry;entry++)
	{
	len=strlen(vmspec[entry].parm);
	ilog_quiet("    %s ",vmspec[entry].mnemonic);
	for(ctr=0;ctr<len;ctr++)
		ilog_quiet("%s ",diagnose_type(vmspec[entry].parm[ctr]));
	ilog_quiet("\n");
	}

ilog_quiet("\nGuessing type for input:\n");
numcheckfatal=0;
ilog_quiet("%s ",line[1]);
for(ctr=0;ctr<15;ctr++)
	{
	buf=line[ctr+1];
	if(buf != NULL)
		{
		strcpy(buffer,buf);
		strstrip(buffer);
		pe_isnumber(buffer);
		if(thinktype != 0)
			ilog_quiet("%s ",diagnose_type(thinktype));
		else
			ilog_quiet("??? ");
		}
	}

// Do it again, and see if we can deduce reason for failure

for(ctr=0;ctr<15;ctr++)
	{
	buf=line[ctr+1];
	if(buf != NULL)
		{
		strcpy(buffer,buf);
		strstrip(buffer);
		if(!pe_isnumber(buffer))
			if(thinktype != 0)
				diagnose_keyword(buffer,thinktype);
		}
	}

PeDump(srcline,"No match","Check bootlog.txt or details");
return -1;
}

// Check the syntax of a line, return 0 if error or doesn't match definition

int check_parm(char **line, unsigned int entry)
{
int no,len,ctr,ok;
char *errbuf;

len=strlen(vmspec[entry].parm);
for(ctr=0;line[ctr];ctr++);
no=ctr-1;

// Wrong number of parameters

if(len != no) {

	// Is it an array initialisation?
	if(strchr(vmspec[entry].parm, '(') && len < no) {
		// It's fine... really
		return 1;
	}

//	printf("<%d/%d for %s>\n",len,no,vmspec[entry].parm);
	return 0;
}

// Is there something we don't understand.. an invalid keyword?

for(ctr=0;ctr<len;ctr++)
    {
    strcpy(buffer,line[ctr+1]);
    strstrip(buffer);

    // side effect of this is to locate undefined symbol, but we mustn't if
    // the symbol is the wildcard '?'.
    if(vmspec[entry].parm[ctr] != '?')
        pe_isnumber(buffer);
    }


// Ok, check the parameters

for(ctr=0;ctr<len;ctr++)
    {
    strcpy(buffer,line[ctr+1]);
    strstrip(buffer);
//	ilog_quiet("trying %s as a %c\n",buffer,vmspec[entry].parm[ctr]);

    switch(vmspec[entry].parm[ctr])
        {
        case 'T':
		if(getnum4table_slow(buffer) == -1)
            return 0;
        break;

        case 'o':
		ok=0;
        if(get_keyword(buffer,'o',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'o',curfunc) != -1)
			ok=1;
		if(!ok)
			return 0;
        break;

        case 't':
		ok=0;
        if(get_keyword(buffer,'t',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'t',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            return 0;
            }
        break;

        case 'P':
		ok=0;
        if(get_keyword(buffer,'s',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'s',curfunc) != -1)
			ok=1;
        if(get_keyword(buffer,'P',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'P',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            return 0;
            }
        break;

        case 'U':
		ok=0;
        if(get_keyword(buffer,'U',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'U',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            return 0;
            }
        break;

        case 's':
        if(count_quotes(buffer) == 0)
            {
            return 0;
            }
        break;

        case 'n':
        if(!pe_isnumber(buffer))
            {
            return 0;
            }
        break;

        case 'f':
		ok=0;
        if(get_keyword(buffer,'f',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'f',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            return 0;
            }
        break;

        case 'i':
		ok=0;
        if(get_keyword(buffer,'i',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'i',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            return 0;
            }
        break;

        case 'l':
//        if(get_keyword(buffer,'?',curfunc)) // Duplication
//            return 0;
        break;

        case 'p':
		if(get_keyword(buffer,'p',NULL) == -1)
            {
            return 0;
            }
        break;

		case '=':
		case 'e':
		if(get_keyword(buffer,'e',NULL) == -1)
			{
			return 0;
			}
		break;

		case 'E':
		if(get_keyword(buffer,'E',NULL) == -1)
			{
			return 0;
			}
		break;

        case 'I':
        case 'O':
        case 'S':
		// It claims to be a member of a structure, so look for '.'
		if(!strchr(buffer,'.')) {
            		return 0;
            	}

		errbuf = pe_checkstruct(buffer);
		//  Quit if we get this kind of error
		if(errbuf) {
			PeDump(srcline,errbuf,NULL);
		}
		//  Make sure it's the right type
		if(!pe_checkstruct_type(buffer,tolower(vmspec[entry].parm[ctr]))) {
			pe_datatype=NULL; // prevent subtle errors
			return 0;
		}
		pe_datatype=NULL;
/*
		// OK, make sure the base structure is known
		strcpy(temp,buffer);
		ptr=strchr(buffer,'.');
		*ptr=0; // Kill the . and get the base pointer

        if(get_keyword(temp,'o',NULL) == -1)
            {
            return 0;
            }
*/
        break;

        case 'a':
        case 'b':
        case 'c':
		// It claims to be an array.  Look for the brackets
		if(!strchr(buffer,'[')) {
			return 0;
		}
		if(!strchr(buffer,']')) {
			return 0;
		}

//		errbuf = pe_checkarray(buffer);
//		if(errbuf) {
//			PeDump(srcline,errbuf,NULL);
//		}

		// If the opcode is 0 it's a declaration and we can't do the full check
		// (Technically RETURN is also opcode 0 but that won't need the check anyway)
		if(vmspec[entry].opcode) {
			//  Make sure it's the right type for overloading
			if(!pe_checkarray_type(buffer,vmspec[entry].parm[ctr])) {
				return 0;
			}
		}

        break;

        case '0':
        if(istricmp(buffer,"null") && istricmp(buffer,"nil"))
            {
            return 0;
            }
	break;

        case '?':
        break;

        case '(':

//		// It claims to be an array initialisation.  Look for the brackets
		if(!strchr(buffer,'(')) {
			return 0;
		}
		if(!strchr(buffer,')')) {
			return 0;
		}
        break;

        default:
        ilog_quiet("check_parm: Failed on parameter type '%c' from '%s'\n",vmspec[entry].parm[ctr],vmspec[entry].parm);
        PeDump(srcline,"Internal error, unknown parameter type for command",buffer);
        break;
        }
    }

// All OK!

return 1;
}

// Ok, we couldn't find a match or we had an error.  Explain it.

void diagnose_keyword(char *name, char type)
{
char *msg;
int ok;

// Display appropriate message

switch(type)
	{
	case 'T':
	if(getnum4table_slow(name) == -1)
		PeDump(srcline,"Could not find data table",name);
	break;

	case 'o':
	ok=0;
	if(get_keyword(name,'o',NULL) != -1)
		ok=1;
	if(get_keyword(name,'o',curfunc) != -1)
		ok=1;
	if(!ok)
		PeDump(srcline,"Could not find object",name);
	break;

	case 't':
	ok=0;
	if(get_keyword(name,'t',NULL) != -1)
		ok=1;
	if(get_keyword(name,'t',curfunc) != -1)
		ok=1;
	if(!ok)
		PeDump(srcline,"Could not find tile",name);
	break;

	case 'P':
	ok=0;
	if(get_keyword(name,'s',NULL) != -1)
		ok=1;
	if(get_keyword(name,'s',curfunc) != -1)
		ok=1;
	if(!ok)
		PeDump(srcline,"Could not find string variable",name);
	break;

	case 'U':
	ok=0;
	if(get_keyword(name,'U',NULL) != -1)
		ok=1;
	if(get_keyword(name,'U',curfunc) != -1)
		ok=1;
	if(!ok)
		PeDump(srcline,"Could not find user-string variable",name);
	break;

	case 's':
	if(count_quotes(name) == 0)
		PeDump(srcline,"Quotes don't match up",name);
	break;

	case 'n':
	if(!pe_isnumber(name))
		PeDump(srcline,"Not a number",name);
	break;

	case 'f':
	ok=0;
	if(get_keyword(name,'f',NULL) != -1)
		ok=1;
	if(get_keyword(name,'f',curfunc) != -1)
		ok=1;
	if(!ok)
		PeDump(srcline,"Could not find function",name);
	break;

	case 'i':
	ok=0;
	if(get_keyword(name,'i',NULL) != -1)
	ok=1;
	if(get_keyword(name,'i',curfunc) != -1)
		ok=1;
	if(!ok)
		PeDump(srcline,"Could not find integer variable",name);
	break;

	case 'l':
	ok=0;
	if(get_keyword(name,'l',NULL) != -1)
		ok=1;
	if(get_keyword(name,'l',curfunc) != -1)
		ok=1;
	if(!ok)
		PeDump(srcline,"Could not find label",name);
	break;

	case 'p':
	if(get_keyword(name,'p',NULL) == -1)
		PeDump(srcline,"Unknown operator [should be +,-,/,*,==,!=,<,> etc]",name);
	break;

	case '=':
	case 'e':
	if(get_keyword(name,'e',NULL) == -1)
		PeDump(srcline,"Inappropriate symbol [should be '=']",name);
	break;

	case 'E':
	if(get_keyword(name,'E',NULL) == -1)
		PeDump(srcline,"Inappropriate symbol [should be '!=' or '<>']",name);
	break;

	case 'I':
	case 'O':
	case 'S':
	msg=pe_checkstruct(name);
	pe_datatype=NULL; // prevent subtle errors
	if(msg)
		PeDump(srcline,msg,name);
//		PeDump(srcline,"Unknown error in structure access",name);
	break;

	case 'a':
	case 'b':
	case 'c':
	msg=pe_checkarray(name);
	if(msg)
		PeDump(srcline,msg,name);
	break;

	case '0':
	if(istricmp(buffer,"null") && istricmp(buffer,"nil"))
		PeDump(srcline,"Inappropriate symbol, should have been 'NULL'",name);
	break;

	case '?':
	ilog_quiet("diagnose_keyword: array init error with '%s'\n",name);
	break;

	case '(':
	//PeDump(srcline,"Undefined symbol [type 'any']",name);
	break;

	case '.':
	ilog_quiet("diagnose_keyword: Failed on parameter type '%c' from '%s'\n",type,name);
	msg=pe_checkstruct(buffer);
	if(msg)
		PeDump(srcline,msg,buffer);
	PeDump(srcline,"Invalid structure access",buffer);
	break;

	case ']':
	ilog_quiet("diagnose_keyword: array error with '%s'\n",name);
	msg=pe_checkarray(buffer);
	if(msg)
		PeDump(srcline,msg,buffer);
	PeDump(srcline,"Invalid array access",buffer);
	break;

	default:
	ilog_quiet("diagnose_keyword: Failed on parameter type '%c' for keyword '%s'\n",type,name);
	PeDump(srcline,"Internal error, unknown parameter type for command",buffer);
	break;
	}

return;
}

// Return a string describing each of the syntax codes from the lang. def.

char *diagnose_type(char type)
{
switch(type)
    {
    case 'T':
	return "<table>";
	break;

    case 's':
    return "<string>";
    break;

    case 'P':
    return "<string variable>";
    break;

    case 'U':
    return "<user-string variable>";
    break;

    case 'n':
    return "<number>";
    break;

    case 'f':
    return "<function>";
    break;

    case 'o':
    return "<object>";
    break;

    case 't':
    return "<tile>";
    break;

    case 'i':
    return "<variable>";
    break;

    case 'I':
    return "<object.integer>";
    break;

    case 'O':
    return "<object.object>";
    break;

    case 'S':
    return "<object.string>";
    break;

    case 'a':
    return "<integer_array[member]>";
    break;

    case 'b':
    return "<string_array[member]>";
    break;

    case 'c':
    return "<object_array[member]>";
    break;

    case 'l':
    return "<user label>";
    break;

    case 'p':
    return "<expression>";
    break;

    case 'e':
    return "==";
    break;

    case 'E':
    return "!=";
    break;

    case '0':
    return "NULL";
    break;

    case '=':
    return "=";
    break;

    case '.':
    return "<structure access>";
    break;

    case ']':
    return "<array access>";
    break;

    case '(':
    return "<array initialisation>";
    break;

    default:
    return "<unknown>";
    break;
    }

return "AIEEE!";
}

// Analyse a line and return help for the error

void find_fatalerror(char **line, int entry)
{
int no,len,ctr,ok;
char msg[128];
char *ptr;

len=strlen(vmspec[entry].parm);
for(ctr=0;line[ctr];ctr++);
no=ctr-1;

// If we don't have the right no of parameters, halt the compilation

if(len != no)
    {
    sprintf(msg,"Expected %d but found %d\n",len,no);
    PeDump(srcline,"Wrong number of parameters",msg);
    }

// If we can't match the parameters, halt the compilation

for(ctr=0;ctr<len;ctr++)
    {
    strcpy(buffer,line[ctr+1]);
    strstrip(buffer);

    switch(vmspec[entry].parm[ctr])
        {
		case 'T':
		if(getnum4table_slow(buffer) == -1)
            {
            PeDump(srcline,"Undefined table",buffer);
            }
		break;

        case 'o':
		ok=0;
        if(get_keyword(buffer,'o',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'o',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            PeDump(srcline,"Undefined object",buffer);
            }
        break;

        case 't':
		ok=0;
        if(get_keyword(buffer,'t',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'t',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            PeDump(srcline,"Undefined tile",buffer);
            }
        break;

        case 'P':
		ok=0;
        if(get_keyword(buffer,'s',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'s',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            PeDump(srcline,"Undefined string",buffer);
            }
        break;

        case 'U':
		ok=0;
        if(get_keyword(buffer,'U',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'U',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            PeDump(srcline,"Undefined user-string",buffer);
            }
        break;

        case 's':
        if(count_quotes(buffer) == 0)
             {
             PeDump(srcline,"Not a valid string",buffer);
             }
        break;

        case 'n':
        if(!pe_isnumber(buffer))
            {
            sprintf(msg,"Parameter %d should be a number\n",(len-ctr));
            PeDump(srcline,msg,buffer); // Oh no, we're through
            }
        break;

        case 'f':
		ok=0;
        if(get_keyword(buffer,'f',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'f',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            PeDump(srcline,"Undefined function",buffer);
            }
        break;

        case 'i':
		ok=0;
        if(get_keyword(buffer,'i',NULL) != -1)
			ok=1;
        if(get_keyword(buffer,'i',curfunc) != -1)
			ok=1;
		if(!ok)
            {
            PeDump(srcline,"Undefined integer variable",buffer);
            }
        break;

        case 'l':
//        if(get_keyword(buffer,'?',curfunc) == -1)
//            {
//            unfix_quotes(line);
//            PeDump(srcline,"Label name already used",buffer);
//            }
        break;

        case 'I':
        case 'O':
        case 'S':
		ptr=pe_checkstruct(buffer);
		if(ptr)
            {
            PeDump(srcline,ptr,buffer);
            }
		if(!pe_checkstruct_type(buffer,tolower(vmspec[entry].parm[ctr])))
			PeDump(srcline,"No match for this member",buffer);
		pe_datatype=NULL; // prevent subtle errors
        break;

        case 'a':
        case 'b':
        case 'c':
		ptr=pe_checkarray(buffer);
		if(ptr)
            {
            PeDump(srcline,ptr,buffer);
            }
//		if(!pe_checkstruct_type(buffer,tolower(vmspec[entry].parm[ctr])))
//			PeDump(srcline,"No match for this member",buffer);
        break;

        case 'p':
        if(get_keyword(buffer,'p',NULL) == -1)
            {
            PeDump(srcline,"Missing operator (+,-,/,*,<,>,==,!= etc) in expression",buffer);
            }
        break;

        case '=':
        if(istricmp(buffer,"="))
            {
            PeDump(srcline,"Missing = in expression",buffer);
            }
        break;

        case '0':
        if(istricmp(buffer,"null") && istricmp(buffer,"nil"))
            {
            PeDump(srcline,"Missing NULL in expression",buffer);
            }
        break;

        case '?':
        break;

        default:
        PeDump(srcline,"Internal error in find_fatal, unknown parameter type",buffer);
        break;
        }
    }

PeDump(srcline,"Transient fatal error detected",buffer);
}


// Resolve the global variables to pointers

void pe_resolve_globals()
{
KEYWORD *k;
static char didglobals=0;
int gptr,iptr,ctr,lc,kctr;

if(!pe_globals)
	return;
if(didglobals)
	return;
didglobals=1;

// Allocate space for the globals to live in.
//ilog_quiet("Allocating space for %d globals\n",pe_globals);
pe_global = (void **)M_get(pe_globals, sizeof(void *));

/*
 Find out how many are pointers that must be subject to Garbage Collection
 This is only for variables that can be dynamically allocated in the
 script, currently only Objects ('o') have this capability
 */

pe_gptrs=0;
pe_gints=0;
for(kctr=0;_keylist[kctr];kctr++)
	{
	lc=_keylist[kctr];
	if(!klist[lc])
		continue;
	for(k=klist[lc][0];k;k=k->next)
		{
		if(k->local == NULL)
			switch(k->type)
				{
				case 'i':
				if(k->id != -1) // Is it a System variable?  If so it doesn't count
					pe_gints++;
				break;

				case 'o':
				if(k->id != -1) // Is it a System variable?  If so it doesn't count
					pe_gptrs++;
				break;

/*
				case 'U':
				if(k->id != -1) // Is it a System variable?  If so it doesn't count
					pe_gstrs++;
				break;
*/

				case 'a':
				if(k->id != -1)
					{
//					ilog_quiet("Adding %d array members to the list\n",k->arraysize);
					pe_gints+=k->arraysize; // Add the whole array to the list
					}

				case 'c':
				if(k->id != -1)
					{
//					ilog_quiet("Adding %d array members to the list\n",k->arraysize);
					pe_gptrs+=k->arraysize; // Add the whole array to the list
					}
				break;

				default:
				break;
				}
		}
	}

//
// Now we have a count, allocate the list of globals for garbage collection (at least one)
//

pe_globalptr = (GLOBALPTR *)M_get(pe_gptrs+1,sizeof(GLOBALPTR));
pe_globalint = (GLOBALINT *)M_get(pe_gints+1,sizeof(GLOBALINT));


gptr=0;
iptr=0;

for(kctr=0;_keylist[kctr];kctr++)
	{
	lc=_keylist[kctr];
	if(!klist[lc])
		continue;
	for(k=klist[lc][0];k;k=k->next)
		{
		if(k->local == NULL)
			switch(k->type)
				{
				case 'i':
				case 'o':
				case 't':
				case 's':
				case 'U':
				case 'a':
				case 'b':
				case 'c':

				// Copy the current default value to the globals array
				// Then change the value so it points to the global instead

				if(k->id != -1) {	// -1 is for system variables, pre-resolved
					if(k->arraysize>0) {
						for(ctr=0;ctr<k->arraysize;ctr++) {
							pe_global[k->id+ctr] = NULL;
						}
						k->value = (void *)&pe_global[k->id]; // Point to [0]
					} else {
//						ilog_quiet("fixup %s = id:%d\n",k->name,k->id);
						pe_global[k->id] = (void *)k->value;
						k->value = (void *)&pe_global[k->id];
					}
				}

				// Register the pointer for Garbage Collection
				if(k->type == 'o' && k->id != -1)
					{
					SAFE_STRCPY(pe_globalptr[gptr].name,k->name);
					pe_globalptr[gptr].ptr = &pe_global[k->id];
					pe_globalptr[gptr].transient = k->transient;
					gptr++;
					}

				// Register the int for logging
				if(k->type == 'i' && k->id != -1)
					{
					SAFE_STRCPY(pe_globalint[iptr].name,k->name);
					pe_globalint[iptr].ptr = (VMINT *)&pe_global[k->id];
					// Store default value
					pe_globalint[iptr].defaultintval = *pe_globalint[iptr].ptr;
					pe_globalint[iptr].transient = k->transient;
//					ilog_quiet("setdefault int variable %s = %d\n",k->name,pe_globalint[iptr].defaultintval);
					iptr++;
					}

				// Register the array for GC
				if(k->type == 'c' && k->id != -1)
					{
//					ilog_quiet("Array at %x\n",&pe_global[k->id]);
					for(ctr=0;ctr<k->arraysize;ctr++)
						{
						snprintf(pe_globalptr[gptr].name,47,"%s[%d]",k->name,ctr);
						pe_globalptr[gptr].ptr = &pe_global[k->id+ctr];
						pe_globalptr[gptr].transient = k->transient;
						gptr++;
						}
					}

				// Register the array for logging
				if(k->type == 'a' && k->id != -1)
					{
//					ilog_quiet("Array at %x\n",&pe_global[k->id]);
					for(ctr=0;ctr<k->arraysize;ctr++)
						{
						snprintf(pe_globalint[iptr].name,47,"%s[%d]",k->name,ctr);
						pe_globalint[iptr].ptr = (VMINT *)&pe_global[k->id+ctr];
						pe_globalint[iptr].transient = k->transient;
						// Globals don't have default values, leave it as 0
						iptr++;
						}
					}

				break;

				default:
//				ilog_quiet("Not adding %s: type %c\n",k->name,k->type);
				break;
				};
		}
	}
}

void wipe_refs()
{
int ctr;
for(ctr=0;ctr<(int)pe_gptrs;ctr++)
	{
	//CheckRefsPtr((void **)pe_globalptrs[ctr]);
	CheckRefsPtr((void **)pe_globalptr[ctr].ptr);
	}
}

//
//  Get an object pointer
//

GLOBALPTR *FindGlobalPtr(const char *name)
{
int ctr;
if(!name)
	return NULL;

for(ctr=0;ctr<pe_gptrs;ctr++)
	if(!istricmp(pe_globalptr[ctr].name,name))
		return &pe_globalptr[ctr];

return NULL;  
}

GLOBALINT *FindGlobalInt(const char *name)
{
int ctr;
if(!name)
	return NULL;

for(ctr=0;ctr<pe_gints;ctr++)
	if(!istricmp(pe_globalint[ctr].name,name))
		return &pe_globalint[ctr];

return NULL;  
}

//
//  Reset the global variables to their original settings, i.e. restart the game
//

void reset_globals()
{
void **ptr;
VMINT *i;
int ctr;

for(ctr=0;ctr<pe_gptrs;ctr++)
	{
	ptr=(void **)pe_globalptr[ctr].ptr;
	if(ptr && *ptr)
		*ptr=NULL;
	}
	
for(ctr=0;ctr<pe_gints;ctr++)
	{
	i=pe_globalint[ctr].ptr;
	if(i)
		{
//		ilog_quiet("Reset int variable %s to %d\n",pe_globalint[ctr].name,pe_globalint[ctr].defaultintval);
		*i=pe_globalint[ctr].defaultintval;
		}
	}
	
}

//
//  Get a list of names for all known global ints, free the list, but not the contents
//

GLOBALINT *GetGlobalIntlist(int *count)
{
if(!count)
	return NULL;

*count = pe_gints;
return pe_globalint;
}

//
//  Get a list of names for all known global ints, free the list, but not the contents
//

GLOBALPTR *GetGlobalPtrlist(int *count)
{
if(!count)
	return NULL;

*count = pe_gptrs;
return pe_globalptr;
}








// Return number of local variables

int count_locals()
{
KEYWORD *k;
int locals=0,lc,ctr;

for(ctr=0;_keylist[ctr];ctr++)
	{
	lc=_keylist[ctr];
	if(!klist[lc])
		continue;
	for(k=klist[lc][0];k;k=k->next)
		{
		if(k->local == curfunc)
			switch(k->type)
				{
				case 'i':
				case 'o':
				case 't':
				case 's':
				locals++;
				break;

				case 'a':
				case 'b':
				case 'c':
				locals+=k->arraysize; // An array is lots of variables
				break;

				default:
				break;
				};
		}
	}
return locals;
}

// Fix up the local variables

int fixup_locals()
{
ICODE *i,*in;
int pos,locals;

locals = count_locals();
if(!locals)
    return 0;

i=icode;
pos = 0;

locals=0;

if(i) {
	do {
		in=i->next; // Store next in list (because it might change!)
		if(i->variable) {
			add_dword_raw(pos);            // Store code offset to patch
			add_dword_raw(i->variable-1);  // Followed by which slot to use
			locals++;
		}
		pos+=i->len; // Update position counter
		i=in;  // Next, please
	} while(i);
}
return locals;
}


// Fix up the jumps: gotos, if statements etc..

void fixup_jumps()
{
ICODE *i;
int lpos;
VMTYPE d;
d.ptr=NULL;

for(i=icode;i;i=i->next)
	if(i->jumplabel)
		{
		lpos = label_pos(i->jumplabel);
		if(lpos == -1)
			{
			ilog_quiet("%s: Cannot find label 0x%x\n",curfunc,i->jumplabel);
			ilog_quiet("It is called %s\n",i->jumplabel->name);
			show_labels();
			PeDump(srcline,"Cannot find label",i->jumplabel->name);
			}
		// Poke in new position
		d.i32=lpos;
		*i->data = d;
		}
return;
}

// FInd size of code and data

int code_size()
{
ICODE *i;
int pos;

pos = 0;
for(i=icode;i;i=i->next)
	pos+=i->len;

return pos;
}

// Find i-code offset for a jump label

int label_pos(KEYWORD *label)
{
ICODE *i;
int pos;

pos = 0;
if(count_locals())
	pos=(sizeof(VMTYPE)*3);  // Take the 'DOFUS' linker into account
//	pos=(sizeof(unsigned int)*3);  // Take the 'DOFUS' linker into account

for(i=icode;i;i=i->next) {
	if(i->labels) {
		for(int ctr=0;ctr<i->labels;ctr++) {
			if(i->label[ctr] == label) {
				return pos;
			}
		}
	}
	pos+=i->len;
}

return -1;
}

// Debug the label engine

void show_labels()
{
ICODE *i;

for(i=icode;i;i=i->next) {
	if(i->jumplabel) {
		ilog_quiet("> %s (J)\n",i->jumplabel->name);
	}
	if(i->labels > 0) {
		for(int ctr=0;ctr<i->labels;ctr++) {
			ilog_quiet("> %s (D)\n",i->label[ctr]->name);
		}
	}
}

return;
}


// Find orphaned local variables

void find_orphans()
{
char msg[128];
KEYWORD *k;
int lc,ctr;

for(ctr=0;_keylist[ctr];ctr++)
	{
	lc=_keylist[ctr];
	if(!klist[lc])
		continue;
	for(k=klist[lc][0];k;k=k->next)
		{
		if(k->local == curfunc)
			if(k->type != 'l' && k->type != 'n')
				if(!k->refcount)
					{
					sprintf(msg,"Local variable '%s' defined but never used",k->name);
					PeDump(srcline,msg,NULL);
					}
		}
	}
return;
}

//
//
// Generic support functions
//
//


// Return the number of quotes.  If odd, there's a problem

int count_quotes(const char *line)
{
int quotes;
const char *p;

quotes=0;
for(p=line;*p;p++)
	if(*p=='\"')
		quotes++;

return quotes;
}

/*
 *      PeDump - Crash out and show a fragment of the code
 */

void PeDump(long lineno,const char *error,const char *help)
{
char errbuf[1024];
char errstr[1024];
short ctr;
long l2,Lctr;

ilog_printf("\n\n");
irecon_colour(0,200,0); // Green
sprintf(errstr,"Error in %s at line %ld:\n",compilename,lineno+1);
ilog_printf(errstr);
ilog_printf("%s\n",error);

l2=lineno+5;
if(l2>script.lines)
	l2=script.lines;
irecon_colour(200,0,0); // Red
for(Lctr=lineno;Lctr<l2;Lctr++)
	if(script.line[Lctr]!=NOTHING&&script.line[Lctr]!=NULL)
		{
		strcpy(errbuf,"");
		for(ctr=0;ctr<script.linewords[Lctr];ctr++)
			{
			strcat(errbuf,script.lineword[Lctr][ctr]);
			strcat(errbuf," ");
			}

	sprintf(errstr,"%6ld: %s\n",Lctr+1,errbuf);
	ilog_printf(errstr);
	}
irecon_colour(0,200,0); // Green
if(help)
	{
	sprintf(errstr,"\n%s\n",help);
	ilog_printf(errstr);
	}

S_Term(); // Shut down sound system.. may take a while

ilog_printf("Press a key to exit\n");
WaitForAscii();
//KillGFX();
exit(1);
}

/*
 *      Purify - Remove comments from the script to ease compilation
 */

void Purify()
{
int ctr,len,pos;
char *ptr;

// Scan each line for comments

for(ctr=0;ctr<script.lines;ctr++)
	{
	ptr = strchr(script.line[ctr],'%');         // Look for a %
	if(ptr)
		{
		// Neutralise any '%s'-type string codes to prevent death or 0wnership
		switch(*(ptr+1))
			{
			case 'd':
			case 's':
			case 'i':
			case 'u':
			case 'l':
			case 'f':
			case 'x':
			*ptr='#';
			break;

			default:
			break;
			};
		}

	// Look for CR/LF codes and neutralise them
	// This probably isn't strictly necessary

	ptr = strchr(script.line[ctr],13);
	if(ptr)
		*ptr=' ';
	ptr = strchr(script.line[ctr],10);
	if(ptr)
		*ptr=' ';
	}

// Scan each line for whitespace and turn it into space

for(ctr=0;ctr<script.lines;ctr++)
	{
	len = strlen(script.line[ctr]);    // Got length of line
	for(pos=0;pos<len;pos++)
		if(isxspace(script.line[ctr][pos]))
			script.line[ctr][pos] = ' ';    // Whitespace becomes space
	strstrip(script.line[ctr]);
	}

return;
}

void mk_lineno(char *str,int line,char *end)
{
sprintf(str,"@@%d@@%s",line,end);
}

void push_ifstack(int id)
{
if(if_sp == IFSTACK_SIZE)
    PeDump(srcline,"Too many nested IF statements",NULL);

ifstack[++if_sp]=id;
}

void pop_ifstack()
{
if(if_sp == 0)
    PeDump(srcline,"Internal error: IF stack underflow",NULL);

ifstack[if_sp]=0;
if_sp--;
}

int read_ifstack()
{
return ifstack[if_sp];
}

int explore_ifstack(int offset)
{
if(if_sp-offset < 0)
	return -1;
return ifstack[if_sp-offset];
}


// This isn't a 'pure' stack, in that we don't need strict push/pop symmetry
// as we'll always empty the whole thing in one go.
// Hence we ignore some 'push' operations if they make no sense
void push_label(KEYWORD *label)
{
// Silently abort if it's nothing, or it's already there
if(!label) {
	return;  
}
for(int ctr=1;ctr<label_sp;ctr++) {
	if(labelstack[ctr] == label) {
		return;
	}
}

if(label_sp == IFSTACK_SIZE)
    PeDump(srcline,"Too many pending labels",NULL);

labelstack[++label_sp]=label;
}

void pop_label()
{
if(label_sp == 0)
    PeDump(srcline,"Internal error: label stack underflow",NULL);

labelstack[label_sp]=NULL;
label_sp--;
}

KEYWORD *read_label()
{
if(label_sp < 1) {
	return NULL;
}
return labelstack[label_sp];
}

int sizeof_labelstack() {
return label_sp;
}


void overlapped_strcpy(char *dest, const char *src)
{
while(*src)
	{
	*dest=*src;
	dest++;
	src++;
	}
*dest=0; // Null terminator
}
