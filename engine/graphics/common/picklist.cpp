//
//	Pick list
//

#include <string.h>
#include "../../ithelib.h"
#include "../iregraph.hpp"
#include "../iconsole.hpp"
#include "../iwidgets.hpp"

IREPICKLIST::IREPICKLIST()
{
win = NULL;
running=0;
}


IREPICKLIST::~IREPICKLIST()
{
if(!running)
	return;

if(win)
	delete win;
}


int IREPICKLIST::Init(int x, int y, int w, int h, const char **list, int listlen)
{
if(running)
	return 0;

win = new IRECONSOLE;
if(!win)
	return 0;
win->Init(x,y,w/8,h/8,-1);
X=x;
Y=y;
W=w;
H=h;
win->Clear();
for(int ctr=0;ctr<listlen;ctr++)
	win->AddLine(list[ctr]);
highlightposition=0;
win->SelectLine(highlightposition);

running=1;
return 1;
}

void IREPICKLIST::Process(int key)
{
int mx,my,mz,mb,mpos;
if(!running)
	return;

int rows = win->GetRows();
int pos = win->GetPos();
int page = rows/2;
int lines = win->GetLines()-2;	// Always adds 1 for the next, unfinished line
int lastpos = (win->GetLines()-rows)-1;
if(lastpos<0)
	lastpos=0;

mpos=mx=my=mz=mb=0;
if(IRE_GetMouse(&mx,&my,&mz,&mb))
	{
	if(mb & IREMOUSE_LEFT)
		if(mx>=X && mx < X+W)
			if(my>=Y && my < Y+H)
				{
				mpos = (my - Y)/8;
				if(mpos <= lines)
					win->SelectLine(pos+mpos);
				}
	}

// Mouse wheel
if(mz<0)
	key=IREKEY_UP;
if(mz>0)
	key=IREKEY_DOWN;

// Normal keys
if(key == IREKEY_UP)
	highlightposition--;

if(key == IREKEY_DOWN)
	highlightposition++;

if(key == IREKEY_PGUP)	{
	highlightposition-=page;
	if(highlightposition < 0)	{
		highlightposition=0;
		pos-=page;
		}
}

if(key == IREKEY_PGDN)	{
	highlightposition+=page;
	if(highlightposition > (rows-1))	{
		highlightposition=rows-1;
		pos+=page;
		}
}

if(key == IREKEY_HOME)	{
	// Move to start
	highlightposition=0;
	pos=0;
}

if(key == IREKEY_END)	{
	// Move to end
	highlightposition=rows;
	pos=lines;
}

if(highlightposition < 0)	{
	highlightposition=0;
	pos--;
}

if(highlightposition > (rows-1))	{
	highlightposition=rows-1;
	pos++;
}

if(pos < 0)
	pos=0;
if(pos > lastpos)
	pos=lastpos;

//printf("highlight = %d, lines = %d, rows = %d, pos = %d, lastpos = %d\n",highlightposition,lines,rows,pos,lastpos);

win->SelectLine(highlightposition+pos);
win->SetPos(pos);

win->Update();
}

void IREPICKLIST::Update()
{
if(running && win)
	win->Update();
}

int IREPICKLIST::GetItem()
{
if(!running)
	return -1;
/*
int t1,t2;
t1=win->GetSelect();
t2=win->GetPos();
printf("select = %d, pos = %d\n",t1,t2);
*/
return win->GetSelect();
}


int IREPICKLIST::GetItem(char *out, int len)
{
const char *select;

if(!running || !out)
	return 0;
select=win->GetSelectText();
if(!select)
	return 0;

strncpy(out,select,len);
out[len-1]=0;
return 1;
}

int IREPICKLIST::SetItem(const char *string)
{
if(!running)
	return 0;
return win->SelectText(string);
}

int IREPICKLIST::GetPos()
{
if(!running)
	return -1;
return win->GetPos();
}

int IREPICKLIST::SetPos(int line)
{
if(!running)
	return 0;
return win->SetPos(line);
}

int IREPICKLIST::GetPercent()
{
if(!running)
	return -1;
if(highlightposition < 0)
	return 0;

int lines = win->GetLines()-2;	// Always adds 1 for the next, unfinished line
if(lines < 1)
	return 0;
return (highlightposition* 100)/lines;
}

int IREPICKLIST::SetPercent(int percent)
{
if(!running)
	return 0;

int rows = win->GetRows();
int lines = win->GetLines()-2;	// Always adds 1 for the next, unfinished line

//printf("list: SetPercent = %d\n",percent);

if(percent < 1)
	{
	percent=0;
	highlightposition=0;
	win->SetPos(0);
	win->SelectLine(0);
	return 1;
	}
if(percent >= 100)
	{
	percent=100;
	highlightposition=rows-2;
	if(highlightposition > lines)
		highlightposition=lines;
//	printf("SetPercent, clip highlightposition = %d\n",highlightposition);

	win->SetPos(lines);
	win->SelectLine(lines);
	return 1;
	}


int ret = win->SetPercent(percent);
int r = (lines*100)/percent;
return ret;
}


int IREPICKLIST::SetColour(IRECOLOUR *col)
{
if(!running || !col)
	return 0;
win->SetCol(col->packed);
return 1;
}

const char *IREPICKLIST::GetSelectText()
{
if(!running || !win)
	return 0;
return win->GetSelectText();
}

void IREPICKLIST::AddLine(const char *a)
{
if(!running || !win)
	return;
win->AddLine(a);
}
