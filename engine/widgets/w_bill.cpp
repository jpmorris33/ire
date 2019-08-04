//
//	In-game GUI widgets (TDGUI version)
//

#define USE_TDGUI

#ifdef USE_ALLEGRO4
#ifdef USE_TDGUI

#include "../ithelib.h"
#include "../graphics/iregraph.hpp"
#include "../core.hpp"
#include "../loadsave.hpp"
#include "../gamedata.hpp"
#include <allegro.h>
extern "C" {
#include "../tdgui.h"
}

char *LoadSaveList_getter(int index, int *list_size);


/*
 *   Get savegame slot number from an Allegro dialog
 */

// Declare the stuff we'll need

static DIALOG SaveList_dialog[] =
	{
		/* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
		{ d_billwin_proc,        0,    8,    368,  166,  0,    0,    0,    0,       0,    0,    (void *)"" },
		{ d_ctext_proc,          0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    (void *)"" },
		{ d_billbutton_proc,     288,  113,  64,  16,   0,    0,    0,    D_EXIT,  0,    0,    (void *)"OK" },
		{ d_billbutton_proc,     288,  135,  64,  16,   0,    0,    27,   D_EXIT,  0,    0,    (void *)"Cancel" },
		{ d_billlist_proc,       16,   28,   256,  123,  0,    0,    0,    D_EXIT,  0,    0,    (void *)LoadSaveList_getter},
		{ NULL }
	};

static char **SL_UserList;
static int SL_ListLen=0;


// Helper function

char *LoadSaveList_getter(int index, int *list_size)
{
if(index < 0)
	{
	if(list_size)
		*list_size = SL_ListLen;
	return NULL;
	}

return SL_UserList[index];
}

// Do it

int GetLoadSaveSlot(int saving)
{
int ctr,blank;
char *savename;
char savegame[MAX_SAVES][40];
int savenum[MAX_SAVES];
char **list;

int listtotal,ret,ret2;

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

// Set list default to end
SaveList_dialog[4].d1=listtotal-1;// Control 4 is the list box

// Set up some globals
SL_ListLen=listtotal;
SL_UserList=list;

// Do the dialog
centre_dialog(SaveList_dialog);
ret = moveable_do_dialog(SaveList_dialog, 4);
ret2 = SaveList_dialog[4].d1;

M_free(list);

if(ret==3) // Cancel
	return -1; // No savegame for you

if(ret2>=listtotal)
	return -1; // Problem

return savenum[ret2]; // Return slot number
}

//
//	Generic text input
//

int GetTextBox(char *text, const char *prompt)
{
int ret;

DIALOG TextEdit_dialog[] =
	{
		/* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)   (dp) */
		{ d_billwin_proc,        0,    8,    320,  64,   0,    0,   0,    0,       0,    0,    (void *)""},
		{ d_ctext_proc,          128,  16,   0,    0,    0,    0,   0,    0,       0,    0,    (void *)""},
		{ d_billedit_proc,       32,   32,   256,  16,   0,    0,   0,    D_EXIT,  40,   0,    (void *)""},
		{ d_billbutton_proc,     0,    0,    0,    0,    0,    0,   27,   D_EXIT,  0,    0,    (void *)""},
		{ NULL }
	};

TextEdit_dialog[1].dp=(void *)prompt;
TextEdit_dialog[1].bg=get_bill_color(bill_face);
TextEdit_dialog[2].dp=(void *)text;

// Do the dialog
centre_dialog(TextEdit_dialog);
ret = moveable_do_dialog(TextEdit_dialog, 2); // Set 2 in focus

if(ret == 3)
	return 0;
return 1;
}



int GetFileSelect(char *prompt, char *newfile, char *x)
{
return !billfile_select(prompt,newfile,x);
}


// user can choose a datafile
int data_choose(char *path,char *path_file,int size)
{
	char *home;
	int i,len;
	int k=0;

	memset(path,0,size); // Blank it
	
//	home=getenv("HOME");
//	if(!home)
		home=".";
	strcpy(path_file,home);
	strcat(path_file,"/");
//	if(!file_select_ex("load game (gamedata.ini)",path_file,"ini",size,0,0))
	if(!billfile_select("load game (gamedata.ini)",path_file,"ini"))
		return 0;
	else
		{
		len = strlen(path_file);
		strslash(path_file); // Ensure slashes are correct
		// Find last slash
		for (i=0;i<len;i++)
			{
			if (path_file[i]=='/')
				{
				k=i;
				}
			}	
		i=0;
		// Copy everything up to the last slash
		while(i<=k)
			{
			path[i]=path_file[i];
			i++;
			}
		return 1;
		}
}	


#endif
#endif
