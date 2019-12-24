/*
 *  Stand-alone editor for defining Large Object sizes
 *  (Actually part of the world editor, invoked with ed -area <object>)
 */

#include <stdio.h>
#include "../ithelib.h"
#include "igui.hpp"
#include "../console.hpp"
#include "../core.hpp"
#include "../gamedata.hpp"
#include "../media.hpp"
#include "../init.hpp"
#include "menusys.h"

// defines

#undef VIEWX	// Need to override this
#undef VIEWY

#define VIEWX 64
#define VIEWY 112

// variables

static int Polarity=0,Direction=0;
static int Edit_H_Id,Edit_V_Id,sol_x_Id,sol_y_Id,sol_w_Id,sol_h_Id,act_x_Id,act_y_Id,act_w_Id,act_h_Id;
static int curchr=0;

static char sol_x_str[] ="0  ";
static char sol_y_str[] ="0  ";
static char sol_w_str[] ="0  ";
static char sol_h_str[] ="0  ";

static char act_x_str[] ="0  ";
static char act_y_str[] ="0  ";
static char act_w_str[] ="0  ";
static char act_h_str[] ="0  ";

static char Out1_str[32];
static char Out2_str[32];
static char Out3_str[32];
static char Out4_str[32];

// functions

static void EditSize();
static void Size_Update();
static void SetPolarityV();
static void SetPolarityH();
static void SetSolidX();
static void SetSolidY();
static void SetSolidW();
static void SetSolidH();
static void SetActiveX();
static void SetActiveY();
static void SetActiveW();
static void SetActiveH();
static void Flip();
static void Finish();
static void PickChar();

extern void GetOBFromList( int x0, int y0, char *prompt, int listsize, char **list, char *name);
//extern void OB_Thumb(int x,int y,char *name);

// code

/*
 *  Entry point (from editor startup)
 */

void EditArea(char *obj)
{
curchr=getnum4char(obj);
if(curchr == -1)
	ithe_panic("Cannot find object called",obj);

// Start the GUI engine
irecon_term();
ilog_break=0; // Disable break now we're ready

IG_Init(128);                   // Set up the iGUI for max 128 buttons
focus=0;

// Backing
SetColor(ITG_BLACK);
IG_KillAll();
IG_Panel(0,0,640,480);


swapscreen->HideMouse();	// MOUSE HACK
EditSize();
swapscreen->ShowMouse();	// MOUSE HACK

do
	{
	IG_Dispatch();
	IRE_WaitFor(1); // Save CPU
	} while(running);       // Loop until quit

IG_Term();                      // Free the iGUI resources
term_media();
exit(1);
}





void EditSize()
{
focus=0;
Polarity=0; // V
Direction=CHAR_L;
IG_KillAll();

DrawScreenBox3D(32,32,608,400);

SetColor(ITG_WHITE);
Edit_H_Id = IG_Tab(48,48,"Horizontal",SetPolarityH,NULL,NULL);
Edit_V_Id = IG_Tab(152,48,"Vertical",SetPolarityV,NULL,NULL);
IG_TextButton(240,48,"Flip direction",Flip,NULL,NULL);

IG_TextButton(368,352,"New Object",PickChar,NULL,NULL);
IG_TextButton(528,352,"Quit",Finish,NULL,NULL);

IG_BlackPanel(VIEWX-2,VIEWY-2,260,260);

SetColor(ITG_WHITE);
DrawScreenText(384,96,"SOLID X offset:");
DrawScreenText(384,128,"SOLID Y offset:");
DrawScreenText(384,160,"SOLID Width:");
DrawScreenText(384,192,"SOLID Height:");

sol_x_Id = IG_InputButton(524,88,sol_x_str,SetSolidX,NULL,NULL);
sol_y_Id = IG_InputButton(524,120,sol_y_str,SetSolidY,NULL,NULL);
sol_w_Id = IG_InputButton(524,152,sol_w_str,SetSolidW,NULL,NULL);
sol_h_Id = IG_InputButton(524,184,sol_h_str,SetSolidH,NULL,NULL);

SetColor(ITG_DARKRED);
DrawScreenText(384,224,"ACTIVE X offset:");
DrawScreenText(384,256,"ACTIVE Y offset:");
DrawScreenText(384,288,"ACTIVE Width:");
DrawScreenText(384,320,"ACTIVE Height:");

act_x_Id = IG_InputButton(524,216,act_x_str,SetActiveX,NULL,NULL);
act_y_Id = IG_InputButton(524,248,act_y_str,SetActiveY,NULL,NULL);
act_w_Id = IG_InputButton(524,280,act_w_str,SetActiveW,NULL,NULL);
act_h_Id = IG_InputButton(524,312,act_h_str,SetActiveH,NULL,NULL);

IG_AddKey(IREKEY_SPACE,Flip);
IG_AddKey(IREKEY_ESC,Finish);

Size_Update();
}

void Size_Update()
{
SEQ_POOL *form;

// Recalc WxH levels for this object
Init_Areas(&CHlist[curchr]);

if(Polarity == 0)
	{
	IG_SetFocus(Edit_H_Id);
	IG_ResetFocus(Edit_V_Id);
	}
else
	{
	IG_ResetFocus(Edit_H_Id);
	IG_SetFocus(Edit_V_Id);
	}

form = &SQlist[CHlist[curchr].dir[Direction]];

fbox2(0,0,256,256,ITG_BLUE,gamewin);
if(Polarity == 0)
	{
	// Horizontal
	form->seq[0]->image->Draw(gamewin,0,0);
	if(form->overlay)
		form->overlay->image->Draw(gamewin,0,0);
    ClipBox(CHlist[curchr].hblock[BLK_X]*32,CHlist[curchr].hblock[BLK_Y]*32,(CHlist[curchr].hblock[BLK_W]*32),(CHlist[curchr].hblock[BLK_H]*32),ITG_WHITE,gamewin);
    ClipBox((CHlist[curchr].harea[BLK_X]*32)+1,CHlist[curchr].harea[BLK_Y]*32+1,(CHlist[curchr].harea[BLK_W]*32)-1,(CHlist[curchr].harea[BLK_H]*32)-1,ITG_RED,gamewin);

    sprintf(sol_x_str,"%-3d",CHlist[curchr].hblock[BLK_X]);
    sprintf(sol_y_str,"%-3d",CHlist[curchr].hblock[BLK_Y]);
    sprintf(sol_w_str,"%-3d",CHlist[curchr].hblock[BLK_W]);
    sprintf(sol_h_str,"%-3d",CHlist[curchr].hblock[BLK_H]);
    sprintf(act_x_str,"%-3d",CHlist[curchr].harea[BLK_X]);
    sprintf(act_y_str,"%-3d",CHlist[curchr].harea[BLK_Y]);
    sprintf(act_w_str,"%-3d",CHlist[curchr].harea[BLK_W]);
    sprintf(act_h_str,"%-3d",CHlist[curchr].harea[BLK_H]);
    }
else
    {
    // Vertical
    form->seq[0]->image->Draw(gamewin,0,0);
	if(form->overlay)
		form->overlay->image->Draw(gamewin,0,0);
    ClipBox(CHlist[curchr].vblock[BLK_X]*32,CHlist[curchr].vblock[BLK_Y]*32,(CHlist[curchr].vblock[BLK_W]*32),(CHlist[curchr].vblock[BLK_H]*32),ITG_WHITE,gamewin);
    ClipBox((CHlist[curchr].varea[BLK_X]*32)+1,(CHlist[curchr].varea[BLK_Y]*32)+1,(CHlist[curchr].varea[BLK_W]*32)-1,(CHlist[curchr].varea[BLK_H]*32)-1,ITG_RED,gamewin);

    sprintf(sol_x_str,"%-3d",CHlist[curchr].vblock[BLK_X]);
    sprintf(sol_y_str,"%-3d",CHlist[curchr].vblock[BLK_Y]);
    sprintf(sol_w_str,"%-3d",CHlist[curchr].vblock[BLK_W]);
    sprintf(sol_h_str,"%-3d",CHlist[curchr].vblock[BLK_H]);
    sprintf(act_x_str,"%-3d",CHlist[curchr].varea[BLK_X]);
    sprintf(act_y_str,"%-3d",CHlist[curchr].varea[BLK_Y]);
    sprintf(act_w_str,"%-3d",CHlist[curchr].varea[BLK_W]);
    sprintf(act_h_str,"%-3d",CHlist[curchr].varea[BLK_H]);
    }

IG_UpdateText(sol_x_Id,sol_x_str);
IG_UpdateText(sol_y_Id,sol_y_str);
IG_UpdateText(sol_w_Id,sol_w_str);
IG_UpdateText(sol_h_Id,sol_h_str);

IG_UpdateText(act_x_Id,act_x_str);
IG_UpdateText(act_y_Id,act_y_str);
IG_UpdateText(act_w_Id,act_w_str);
IG_UpdateText(act_h_Id,act_h_str);

// Draw it

gamewin->Draw(swapscreen,VIEWX,VIEWY);

// Write out the goodies

sprintf(Out1_str,"setsolid      V %d %d %d %d",CHlist[curchr].vblock[BLK_X],CHlist[curchr].vblock[BLK_Y],CHlist[curchr].vblock[BLK_W],CHlist[curchr].vblock[BLK_H]);
sprintf(Out2_str,"setsolid      H %d %d %d %d",CHlist[curchr].hblock[BLK_X],CHlist[curchr].hblock[BLK_Y],CHlist[curchr].hblock[BLK_W],CHlist[curchr].hblock[BLK_H]);
sprintf(Out3_str,"setactivearea V %d %d %d %d",CHlist[curchr].varea[BLK_X],CHlist[curchr].varea[BLK_Y],CHlist[curchr].varea[BLK_W],CHlist[curchr].varea[BLK_H]);
sprintf(Out4_str,"setactivearea H %d %d %d %d",CHlist[curchr].harea[BLK_X],CHlist[curchr].harea[BLK_Y],CHlist[curchr].harea[BLK_W],CHlist[curchr].harea[BLK_H]);

//fbox2(32,416,256,32,makecol(200,200,200),gamewin);
IG_BlackPanel(36,416,256,44);
DrawScreenText(40,424,Out1_str);
DrawScreenText(40,432,Out2_str);
DrawScreenText(40,440,Out3_str);
DrawScreenText(40,448,Out4_str);

}

void SetPolarityH()
{
Polarity=0;
Direction=CHAR_L;
Size_Update();
}

void SetPolarityV()
{
Polarity=1;
Direction=CHAR_D;
Size_Update();
}

void SetSolidX()
{
int i;
if(Polarity == 0)
    {
    i = CHlist[curchr].hblock[BLK_X];
    CHlist[curchr].hblock[BLK_X] = InputIntegerValue(-1,-1,0,255,i);
    }
else
    {
    i = CHlist[curchr].vblock[BLK_X];
    CHlist[curchr].vblock[BLK_X]= InputIntegerValue(-1,-1,0,255,i);
    }
Size_Update();
}

void SetSolidY()
{
int i;
if(Polarity == 0)
    {
    i = CHlist[curchr].hblock[BLK_Y];
    CHlist[curchr].hblock[BLK_Y] = InputIntegerValue(-1,-1,0,255,i);
    }
else
    {
    i = CHlist[curchr].vblock[BLK_Y];
    CHlist[curchr].vblock[BLK_Y] = InputIntegerValue(-1,-1,0,255,i);
    }
Size_Update();
}

void SetSolidW()
{
int i;
if(Polarity == 0)
    {
    i = CHlist[curchr].hblock[BLK_W];
    CHlist[curchr].hblock[BLK_W]= InputIntegerValue(-1,-1,0,255,i);
    }
else
    {
    i = CHlist[curchr].vblock[BLK_W];
    CHlist[curchr].vblock[BLK_W]= InputIntegerValue(-1,-1,0,255,i);
    }
Size_Update();
}

void SetSolidH()
{
int i;
if(Polarity == 0)
    {
    i = CHlist[curchr].hblock[BLK_H];
    CHlist[curchr].hblock[BLK_H]= InputIntegerValue(-1,-1,0,255,i);
    }
else
    {
    i = CHlist[curchr].vblock[BLK_H];
    CHlist[curchr].vblock[BLK_H]= InputIntegerValue(-1,-1,0,255,i);
    }
Size_Update();
}

void SetActiveX()
{
int i;
if(Polarity == 0)
    {
    i = CHlist[curchr].harea[BLK_X];
    CHlist[curchr].harea[BLK_X]= InputIntegerValue(-1,-1,0,255,i);
    }
else
    {
    i = CHlist[curchr].varea[BLK_X];
    CHlist[curchr].varea[BLK_X]= InputIntegerValue(-1,-1,0,255,i);
    }
Size_Update();
}

void SetActiveY()
{
int i;
if(Polarity == 0)
    {
    i = CHlist[curchr].harea[BLK_Y];
    CHlist[curchr].harea[BLK_Y] = InputIntegerValue(-1,-1,0,255,i);
    }
else
    {
    i = CHlist[curchr].varea[BLK_Y];
    CHlist[curchr].varea[BLK_Y] = InputIntegerValue(-1,-1,0,255,i);
    }
Size_Update();
}

void SetActiveW()
{
int i;
if(Polarity == 0)
	{
	i = CHlist[curchr].harea[BLK_W];
	CHlist[curchr].harea[BLK_W] = InputIntegerValue(-1,-1,0,255,i);
	}
else
	{
	i = CHlist[curchr].varea[BLK_W];
	CHlist[curchr].varea[BLK_W] = InputIntegerValue(-1,-1,0,255,i);
	}
Size_Update();
}

void SetActiveH()
{
int i;
if(Polarity == 0)
	{
	i = CHlist[curchr].harea[BLK_H];
	CHlist[curchr].harea[BLK_H] = InputIntegerValue(-1,-1,0,255,i);
	}
else
	{
	i = CHlist[curchr].varea[BLK_H];
	CHlist[curchr].varea[BLK_H] = InputIntegerValue(-1,-1,0,255,i);
	}
Size_Update();
}

// Flip sprite polarity


void Flip()
{
switch(Direction)
	{
	case CHAR_U:
	Direction = CHAR_D;
	break;

	case CHAR_D:
	Direction = CHAR_U;
	break;

	case CHAR_L:
	Direction = CHAR_R;
	break;

	case CHAR_R:
	Direction = CHAR_L;
	break;
	}
Size_Update();
}

// Quit

void Finish()
{
running=0;
}



void PickChar()
{
char **list;
char Name[256];
int ctr;

// Allocate a list of possible characters
list=(char **)M_get(CHtot+1,sizeof(char *));

// Set it up, convert to uppercase for Wyber's ordered selection routine.

for(ctr=0;ctr<CHtot;ctr++)
	{
	list[ctr]=CHlist[ctr].name;
	strupr(list[ctr]);              // Affects the original too
	}

strcpy(Name,CHlist[curchr].name);      // Set up the default string
qsort(list,CHtot,sizeof(char*),CMP);  // Sort them for the dialog box

// This is the graphical dialog box, taken from DEU.

GetOBFromList( -1,-1, "Choose a character:", CHtot-1, list, Name);

// If the user didn't press ESC instead of choosing, modify the character

if(Name[0])     // It's ok, do it
	{
	ctr=getnum4char(Name);
	if(ctr>=0)
		curchr=ctr;
	}

free(list);                             // Dispose of the list
EditSize();  // Redraw all
}

