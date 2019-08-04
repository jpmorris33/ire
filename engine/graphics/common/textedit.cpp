//
//	Pick list
//

#include <string.h>
#include <stdlib.h>
#include "../../ithelib.h"
#include "../iregraph.hpp"
#include "../iconsole.hpp"
#include "../iwidgets.hpp"

extern IREFONT *DefaultFont;
extern IREBITMAP *swapscreen;

#define TEXTSTRING 1
#define TEXTNUMERIC 2

static void deletestrfrom(char *string, int cursor);
static void insertstrat(char *string, int cursor, int chr);

IRETEXTEDIT::IRETEXTEDIT()
{
backing = NULL;
string = NULL;
font=DefaultFont;
col.Set(255,255,255);
invcol.Set(0,0,0);
maxchars=0;
min=max=cx=0;
offset=0;
running=0;
}


IRETEXTEDIT::~IRETEXTEDIT()
{
if(!running)
	return;

if(backing)
	delete backing;

if(string)
	M_free(string);
string=NULL;
}


void IRETEXTEDIT::SetColour(IRECOLOUR *colour)
{
if(colour)
	{
	col.Set(colour->packed);
	invcol.Set(255-colour->r,255-colour->g,255-colour->b);
	}
}


void IRETEXTEDIT::SetFont(IREFONT *fnt)
{
if(fnt)
	font=fnt;
}


int IRETEXTEDIT::InitText(int x, int y, int w, int h, int maxch)
{
if(running)
	return 0;
if(maxch <= 0)
	return 0;

X=x;
Y=y;
W=w;
H=h;

backing = MakeIREBITMAP(W,H);		// Make the restore buffer
if(!backing)
	return 0;
backing->Get(swapscreen,X,Y);

cursor=0;
string = (char *)M_get(1,maxch+1);

running=TEXTSTRING;
return 1;
}


int IRETEXTEDIT::InitNumber(int x, int y, int w, int h, int maxnumber, int minnumber)
{
int ret;
if(running)
	return 0;

ret=InitText(x,y,w,h,16);
if(!ret)
	return 0;
running=TEXTNUMERIC;
max=maxnumber;
min=minnumber;

return ret;
}


void IRETEXTEDIT::Redraw()
{
int backingw,ctr,x,len,w,h,maxx;
char str[2];

if(!running || !font || !string)
	return;
backing->Draw(swapscreen,X,Y);
backingw=backing->GetW();

h=font->Height();

len=strlen(string);
x=X;
maxx=X+W;
for(ctr=offset;ctr<len;ctr++)
	{
	str[0]=string[ctr];
	w=font->Length(str);
	if(w+X > maxx)
		break;
	if(ctr == cursor)
		{
		cx=x;
		swapscreen->FillRect(x,Y,w,h,&col);
		font->Draw(swapscreen,x, Y,&invcol, NULL,string);
		}
	else
		font->Draw(swapscreen,x, Y,&col, NULL,string);
	x+=w;
	}

if(cursor == len)
	{
	cx=x;
	if(x+h < maxx)
		swapscreen->FillRect(x,Y,h,h,&col);
	}

return;
}


void IRETEXTEDIT::Process(int key, int ascii)
{
int len;
if(!running || !string)
	return;

len=strlen(string);

if(key == IREKEY_LEFT)
	{
	cursor--;
	if(cursor < 0)
		{
		cursor=0;
		if(offset > 0)
			offset--;
		}
	Redraw();
	return;
	}

if(key == IREKEY_RIGHT)
	{
	cursor++;
	if(cursor > len)
		cursor=len;
	else
		if(cx >= W && offset < len)
			offset++;
	Redraw();
	return;
	}

if(key == IREKEY_HOME)
	{
	cursor=0;
	offset=0;
	Redraw();
	return;
	}

if(key == IREKEY_END)
	{
	cursor=len;
	offset = len-5;
	if(offset < 0)
		offset=0;
	Redraw();
	return;
	}

if(key == IREKEY_DEL || key == IREKEY_DEL_PAD)
	{
	deletestrfrom(string,cursor);
	Redraw();
	return;
	}

if(key == IREKEY_BACKSPACE)
	{
	deletestrfrom(string,cursor-1);
	Redraw();
	return;
	}

if(len >= maxchars)
	return;
insertstrat(string,cursor,ascii);
cursor++;
if(cursor > len)
	cursor=len;
else
	if(cx >= W && offset < len)
		offset++;
Redraw();
}



int IRETEXTEDIT::GetNumber()
{
if(!running || !string)
	return 0;

return atoi(string);
}


const char *IRETEXTEDIT::GetText()
{
if(!running || !string)
	return NULL;

return string;
}



void deletestrfrom(char *string, int cursor)
{
int len;
char *ptr;
if(cursor < 0)
	return;
len=strlen(string);
if(cursor >= len)
	return;
for(ptr=&string[cursor];*ptr;ptr++)
	*ptr=*(ptr+1);
*ptr=0; // ensure NULL terminator
}


void insertstrat(char *string, int cursor, int chr)
{
int len,ctr;
for(ctr=len;ctr>cursor;ctr--)
	string[ctr+1]=string[ctr];
}
