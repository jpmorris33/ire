#include <stdio.h>
#include "igui.hpp"
#include "../ithelib.h"
#include "menusys.h"

extern void Toolbar();
extern int focus,AB_Id;
extern void (*EdUpdateFunc)(void);

void AB_GoFocal()
{
if(focus==10)
	return;
focus=10;
Toolbar();
IG_SetFocus(AB_Id);

IG_Text(64,64,"IRE Editor 1.8",ITG_RED);

IG_Text(96,96,"Portions by Raphael Quinet and Dewi Morgan",ITG_BLACK);

//IG_Text(24,256,"\"Damn.. Napster hasn't connected.  Now we have to pinch it's little head.\"",ITG_BLACK);
IG_Text(24,256,"\"What would you call it?  Dobermans?  Dobermen?  Dobermensch?\"",ITG_BLACK);

EdUpdateFunc=NULL;

//setcolor(ITG_BLACK);
}

