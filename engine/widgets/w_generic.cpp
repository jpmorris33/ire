//
//	In-game GUI widgets
//

#include <string.h>
#include "../ithelib.h"
#include "../graphics/iregraph.hpp"
#include "../graphics/iwidgets.hpp"
#include "../core.hpp"
#include "../loadsave.hpp"
#include "../gamedata.hpp"

extern IREFONT *DefaultFont;
extern IREBITMAP *swapscreen;
extern void SaveScreen();
extern void RestoreScreen();
extern IRECOLOUR *ire_black;

char *LoadSaveList_getter(int index, int *list_size);


#define GLS_BASEW 368
#define GLS_BASEH 166
#define GLS_BASEX ((640-GLS_BASEW)/2)
#define GLS_BASEY ((480-GLS_BASEH)/2)


int GetLoadSaveSlot(int saving)
{
int ctr,blank,mb;
char *savename;
char savegame[MAX_SAVES][40];
int savenum[MAX_SAVES];
char **list;
IREPICKLIST *picklist;
int listtotal,key;
IRECOLOUR main;

picklist = new IREPICKLIST();
if(!picklist)
	return -1;

memset(savegame,0,sizeof(savegame));
memset(savenum,0,sizeof(savenum));

// Scan existing savegames
listtotal=0;

// Record which ones exist
for(ctr=1;ctr<MAX_SAVES;ctr++)
	{
	savename=read_sgheader(ctr,&blank);
	if(savename)
		{
		SAFE_STRCPY(savegame[listtotal],savename);
		savenum[listtotal]=ctr;
		listtotal++;
		}
	}

// Now add an extra item to the list
if(saving)
	{
	SAFE_STRCPY(savegame[listtotal],"Empty Slot");
	if(listtotal>0)
		savenum[listtotal]=savenum[listtotal-1]+1; // New slot number
	else
		savenum[listtotal]=1;; // New slot number

	if(savenum[listtotal]>=MAX_SAVES)
		listtotal--; // No more slots
	}
else
	{
	SAFE_STRCPY(savegame[listtotal],"Restart Game");
	savenum[listtotal]=-666; // Special
	}
listtotal++;

list=(char **)M_get(listtotal,sizeof(char *)); // Make list to give to dialog

// POP-U-LATE!
for(ctr=0;ctr<listtotal;ctr++)
	list[ctr]=&savegame[ctr][0];

main.Set(200,200,200);
SaveScreen();
swapscreen->FillRect(GLS_BASEX-2,GLS_BASEY-2,GLS_BASEW+4,GLS_BASEH+4,ire_black);
swapscreen->FillRect(GLS_BASEX,GLS_BASEY,GLS_BASEW,GLS_BASEH,&main);

picklist->SetColour(ire_black);
picklist->Init(GLS_BASEX+16,GLS_BASEX+28,256,123,(const char **)list,listtotal);
M_free(list);

// Set list default to end
picklist->SetPos(listtotal); // -1?

swapscreen->HideMouse();
swapscreen->Render();
swapscreen->ShowMouse();

key=-1;
do
	{
	if(IRE_KeyPressed())
		{
		key=IRE_NextKey(NULL);
		picklist->Process(key);
		swapscreen->HideMouse();
		swapscreen->Render();
		swapscreen->ShowMouse();
		}
	else
		{
		IRE_WaitFor(1);  // Yield
		swapscreen->HideMouse();
		swapscreen->Render();
		swapscreen->ShowMouse();
		if(IRE_GetMouse(NULL,NULL,NULL,&mb))
			if(mb&IREMOUSE_LEFT)
				picklist->Process(-1);
		}
	} while(key != IREKEY_ESC && key != IREKEY_ENTER);

RestoreScreen();
IRE_ShowMouse(); // Mouse to screen

if(key == IREKEY_ESC)
	{
	delete picklist;
	return -1;
	}
ctr=picklist->GetItem();
delete picklist;

if(ctr<0 || ctr >= listtotal)
	return -1;

return savenum[ctr]; // Return slot number
}


//
//	Generic text input
//

#define GTB_BASEW 320
#define GTB_BASEH 64
#define GTB_BASEX ((640-GTB_BASEW)/2)
#define GTB_BASEY ((480-GTB_BASEH)/2)

int GetTextBox(char *text, const char *prompt, int maxlen)
{
int ret,key,asc;
IRECOLOUR main;
IRETEXTEDIT *edit;

edit = new IRETEXTEDIT();
if(!edit)
	return 0;

main.Set(200,200,200);
SaveScreen();
swapscreen->FillRect(GTB_BASEX-2,GTB_BASEY-2,GTB_BASEW+4,GTB_BASEH+4,ire_black);
swapscreen->FillRect(GTB_BASEX,GTB_BASEY,GTB_BASEW,GTB_BASEH,&main);

DefaultFont->Draw(swapscreen,GTB_BASEX+128,GTB_BASEY+16,ire_black,NULL,prompt);
edit->SetColour(ire_black);
if(!edit->InitText(GTB_BASEX+32,GTB_BASEY+32,256,16,maxlen))
	return 0;

swapscreen->Render();

key=-1;
do
	{
	if(IRE_KeyPressed())
		{
		key=IRE_NextKey(&asc);
		edit->Process(key,asc);
		swapscreen->Render();
		}
	else
		IRE_WaitFor(1);  // Yield
	} while(key != IREKEY_ESC && key != IREKEY_ENTER);

RestoreScreen();

if(key == IREKEY_ESC)
	{
	delete edit;
	return 0;
	}

strcpy(text,edit->GetText());
delete edit;

return 1;
}

