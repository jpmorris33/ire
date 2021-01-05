//
//	Stubs for the editor for game functions we don't need
//

#include <stdio.h>
#include <string.h>
#include "core.hpp"
#include "console.hpp"
#include "ithelib.h"

// VRM code handle

char *solid_map;
VMINT dark_mix=0;
VMINT light_spell=0;
VMINT show_invisible=1;	// If it is used, we want 1 'cause we're in the editor

void VRM_RegisterExvar(char *name, void *ptr)
{
}

void register_exporttable()
{
}

void Call_VRM(int a)
{
return;
}

void CallVMnum(int a)
{
return;
}

void CallVM(char *a)
{
return;
}

char *CurrentVM()
{
return "";
}

void call_vrm(char *a)
{
return;
}

void ResumeSchedule(OBJECT *o)
{
return;
}

void waitfor(unsigned int x)
{
return;
}


int getYN(char *msg)
{
return 1;
}

void SetDarkness(int x)
{
}

int GetDarkness()
{
return 0;
}

void make_journaldate(char *date)
{
if(date)
	*date=0;
}


int fastrandom()
{
return rand();
}

void CheckSpecialKeys(int k)	{
/*
if(IRE_TestKey(IREKEY_PAUSE))
	if(IRE_TestShift(IRESHIFT_CTRL))
		ithe_panic("User Break","User requested emergency quit");
*/
}
