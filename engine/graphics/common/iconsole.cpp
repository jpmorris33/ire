//
//      New text console engine
//

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../../types.h"
#include "../../ithelib.h"
#include "../iregraph.hpp"
#include "../iconsole.hpp"

extern IREFONT *DefaultFont;
extern IREBITMAP *swapscreen;

IRECONSOLE::IRECONSOLE()
{
backing=NULL;
font=DefaultFont;
col.Set(255,255,255);
highlight.Set(255,255,255);
inverse.Set(0,0,0);
W=H=X=Y=Cols=Rows=lines=0;
linehead=curline=topline=NULL;
Select=NULL;
running=0;
}


IRECONSOLE::~IRECONSOLE()
{
running=0;
//printf("Destruct console\n");
Clear();
if(backing)
	delete backing;
backing=NULL;
}


int IRECONSOLE::Init(int x, int y, int cols, int rows, int maxlines)
{
if(running)
	return 0;

if(x<0 || y<0 || cols < 1 || rows < 1)
	return 0;

FW=FH=8; // Assume 8 pixel font for now
X=x;
Y=y;
Cols=cols;
Rows=rows;
W=cols*FW;
H=rows*FH;
backing = MakeIREBITMAP(W,H);		// Make the restore buffer
if(!backing)
	return 0;

backing->Get(swapscreen,X,Y);

LineMax=maxlines;
lines=0;
conpos=0;

running=1; // We're now running enough for Add

Add();

linelen = 0;
if(curline)
	{
	linelen=sizeof(curline->text);
	if(Cols >= linelen)
		Cols=linelen-1;
	}
return 1;
}


void IRECONSOLE::Clear()
{
int wasrunning=running;
running=0;

//printf("Clearing\n");

CONSOLELINE *ptr=linehead;
CONSOLELINE *next=NULL;
do	{
	if(ptr)
		{
		next=ptr->next;
		free(ptr);
		}
	ptr=next;
	} while(ptr);

linehead=curline=NULL;
lines=0;
Select=NULL;
running=wasrunning;
}


void IRECONSOLE::Print(const char *msg)
{
int curlen,msglen;
if(!running || !msg)
	return;

if(!curline)
	if(!Add())
		return;
if(!curline)
	ithe_panic("IRECONSOLE::Print - logic fault",NULL);


if(!msg[0])     // If it's blank, don't bother
	{
	Newline();
	Update();
	return;
	}

curlen=conpos;
msglen=strlen(msg);

//printf("msglen = %d\n",msglen);

if(curlen + msglen >= linelen)
	msglen = (linelen-1)-curlen;

//printf("msglen now %d, linelen being %d\n",msglen,linelen);

if(msglen > 0)
	strncat(curline->text,msg,msglen);
curline->text[linelen-1]=0;

// Stop a little '^' from appearing at the end of the line..

strdeck(curline->text,'\n');
strdeck(curline->text,'\r');
conpos=strlen(curline->text);

curline->colour.Set(col.packed);

if(strchr(msg,'\n') != NULL)    // If there is a CR, act upon it
	{
	Newline();
	Update();
	conpos=0;
	}
}


void IRECONSOLE::Printf(const char *msg, ...)
{
char linebuf[MAX_LINE_LEN];
char *ptr,*ptr2;

va_list ap;

if(!running)
	return;

va_start(ap, msg);

#ifdef _WIN32
_vsnprintf(linebuf,MAX_LINE_LEN,msg,ap);          // Temp is now the message
#else
	#ifdef __DJGPP__
		vsprintf(linebuf,msg,ap);
	#else
		vsnprintf(linebuf,MAX_LINE_LEN,msg,ap);
	#endif
#endif

// Replace '~' with "
do	{
	ptr=strchr(linebuf,'~');
	if(ptr)
		*ptr='\"';
	} while(ptr);

// If it has any pipe characters (line break) split it into chunks,
// otherwise just print it

if(!strchr(linebuf,'|'))
	{
	PrintfCore(linebuf);
	va_end(ap);
	return;
	}

// Okay, there are some pipes, split it up

ptr=linebuf;

do
	{
	ptr2=strchr(ptr,'|');
	if(ptr2)
		{
		*ptr2=0;
		PrintfCore(ptr);
		Newline();
		ptr=ptr2+1;
		}
	else
		PrintfCore(ptr);
	} while(ptr2);

va_end(ap);
}


void IRECONSOLE::AddLine(const char *msg)
{
if(!running || !msg)
	return;

if(!curline)
	if(!Add())
		return;
if(!curline)
	ithe_panic("IRECONSOLE::Print - logic fault",NULL);


if(!msg[0])     // If it's blank, don't bother
	{
	Newline();
	return;
	}

SAFE_STRCPY(curline->text,msg);
curline->text[Cols]=0; // Truncate to console width
curline->colour.Set(col.packed);

Newline();
conpos=0;
}


void IRECONSOLE::Newline()
{
if(!running)
	return;
Add();
conpos=0;
}


void IRECONSOLE::ClearLine()
{
if(!running || !curline)
	return;
curline->text[0]=0;
conpos=0;
}


void IRECONSOLE::SetCol(int r, int g, int b)
{
if(!running || !curline)
	return;

//printf("SetCol to %d,%d,%d\n",r,g,b);
curline->colour.Set(r,g,b);
col.Set(r,g,b);
}


void IRECONSOLE::SetCol(unsigned int packed)
{
if(!running || !curline)
	return;

/*
if(packed == 0)
	{
	printf("packed = %x\n",packed);
	CRASH();
	}
printf("SetCol to %x\n",packed);
*/
curline->colour.Set(packed);
col.Set(packed);
}


void IRECONSOLE::SetCol(IRECOLOUR newcol)
{
SetCol(newcol.packed);
}


void IRECONSOLE::SetHighlight(int r, int g, int b)
{
if(!running)
	return;

highlight.Set(r,g,b);
inverse.Set(255-r,255-g,255-b);
}


void IRECONSOLE::SetHighlight(unsigned int packed)
{
if(!running)
	return;

highlight.Set(packed);
inverse.Set(packed ^ 0xffffff);
}


void IRECONSOLE::SetHighlight(IRECOLOUR newcol)
{
if(!running)
	return;

highlight.Set(newcol.packed);
inverse.Set(newcol.packed ^ 0xffffff);
}


unsigned int IRECONSOLE::GetPackedCol()
{
if(!running || !curline)
	return 0;
return curline->colour.packed;
}


unsigned int IRECONSOLE::GetPackedHighlight()
{
if(!running)
	return 0;
return highlight.packed;
}


void IRECONSOLE::Update()
{
FastUpdate();
swapscreen->Render();
}


void IRECONSOLE::FastUpdate()
{
CONSOLELINE *pos;
IRECOLOUR *colptr;
int backingw;
  
if(!running || !linehead || !curline)
	return;
backing->Draw(swapscreen,X,Y);
backingw=backing->GetW();

pos=topline;
if(!pos)
	pos=linehead;

for(int ctr=0;ctr<Rows;ctr++)
	{
	if(!pos)
		break; // Out of lines
//	printf("draw line %d/%d at %d, %d with font %p and colour %x : %s\n",ctr,Rows,X,Y+(ctr*FH),pos->font,pos->colour.packed,pos->text);
	if(pos->font)
		{
		colptr=&pos->colour;
		if(Select == pos)
			{
			swapscreen->FillRect(X,Y+(ctr*FH),backingw-1,FH,&highlight);
			colptr=&inverse;
			}
		pos->font->Draw(swapscreen,X, Y+(ctr*FH),colptr, NULL,pos->text);
		}
	pos=pos->next;
	}
}


int IRECONSOLE::GetPercent()
{
if(!running || !linehead || lines < 1)
	return 0;

return (GetPos() * 100)/lines;
}


int IRECONSOLE::SetPercent(int percent)
{
if(!running || !linehead || lines<1)
	return 0;

if(percent < 1)
	percent=1;
if(percent > 100)
	percent=100;

return SetPos((lines*100)/percent);
}


int IRECONSOLE::GetPos()
{
CONSOLELINE *p=linehead;
if(!running || !linehead)
	return 0;

if(topline == NULL)
	return 0;

for(int ctr=0;p;ctr++)	{
	if(p == topline)
		return ctr;
	if(p)
		p=p->next;
	}
  
return 0;
}


int IRECONSOLE::SetPos(int lineno)
{
if(!running || !linehead)
	return 0;

if(lineno + Rows > lines)
	lineno=lines-Rows;
if(lineno < 0)
	lineno=0;

topline=linehead;
for(int ctr=0;ctr<lineno;ctr++)
	{
	if(topline->next == NULL)
		return -1;  // Oops, none left
	topline=topline->next;
	}
	
Update();
return 1;
}


int IRECONSOLE::ScrollRel(int direction)
{
if(!running || !linehead)
	return 0;

if(!topline)
	topline=linehead;

if(direction == 0)
	return 1; // As you wish
	
if(direction < 0)
	{
	if(topline->prev)
		{
		topline=topline->prev;
		Update();
		return 1;
		}
	return 0; // No more
	}

if(direction > 0)
	{
	if(topline->next)
		{
		CONSOLELINE *p=topline->next;
		for(int ctr=0;ctr<Rows;ctr++) {
			p=p->next;
			if(!p)
				return 0;	// Can't scroll that far
			}
		topline=topline->next;
		Update();
		return 1;
		}
	return 0; // No more
	}

return 0; // Wait, what?
}


int IRECONSOLE::GetSelect()
{
CONSOLELINE *p=linehead;
if(!running || !linehead || !Select)
	return -1;

for(int ctr=0;p;ctr++)	{
	if(p == Select)
		return ctr;
	if(p)
		p=p->next;
	}
  
return -1;
}


int IRECONSOLE::GetRows()
{
if(!running)
	return 0;
return Rows;
}


int IRECONSOLE::GetLines()
{
if(!running)
	return 0;
return lines;
}


int IRECONSOLE::SelectLine(int lineno)
{
if(!running || !linehead)
	return 0;

if(lineno < 0)
	{
	Select=NULL;
	Update();
	return 1; // Deselect OK
	}

if(lineno > lines)
	lineno=lines;
/*
if(lineno + Rows > lines)
	lineno=lines-Rows;
*/

Select=linehead;
for(int ctr=0;ctr<lineno;ctr++)
	{
	if(Select->next == NULL)
		return -1;  // Oops, none left
	Select=Select->next;
	}
	
Update();
return 1;
}


int IRECONSOLE::SelectText(const char *string)
{
if(!string || !running)
	return 0;
  
for(CONSOLELINE *p=linehead;p;p=p->next)
	if(p->text)
		if(!istricmp_fuzzy(p->text,string))
			return 1;
return 0;
}


const char *IRECONSOLE::GetSelectText()
{
if(!running || !Select)
	return NULL;
return Select->text;
}



//
//	Private bits
//


int IRECONSOLE::Add()
{
CONSOLELINE *p;
if(!running)
	return 0;

// Do we not have an anchor?
if(!linehead) {
	linehead = (CONSOLELINE *)calloc(1,sizeof(CONSOLELINE));
	if(!linehead)
		return 0;
	curline = linehead;
	curline->colour.Set(col.packed);
	curline->font = font;
	lines=1;
	return 1;
	}

//  Do we have a line limit?  If so, recycle existing line
if(LineMax > 0)
	if(lines >= LineMax) {
	  
		CONSOLELINE *oldtop;
		oldtop = linehead;
		
		if(oldtop == curline)
			{
			// There is just one line, reuse it
			curline->colour.Set(col.packed);
			curline->font = font;
			curline->text[0]=0;
			return 1;
			}

		// Scroll the top pointer along one
		linehead = linehead->next;
		linehead->prev = NULL; // Nothing higher at the top
		
		// Isolate the old top line
		oldtop->next=oldtop->prev=NULL;
		
		// Now add the old top at the end of the list
		curline->next = oldtop;
		oldtop->prev = curline;
		
		// Now move the current line down
		curline = curline->next;

		// Now use it
		curline->colour.Set(col.packed);
		curline->font = font;
		curline->text[0]=0;
		return 1;
		}

// Okay, create a new line
p=curline;
p->next = (CONSOLELINE *)calloc(1,sizeof(CONSOLELINE));
if(!p->next)
	return 0;
curline = curline->next;
curline->prev=p;
curline->colour.Set(col.packed);
curline->font = font;
lines++;

return 1;
}


/*
 *  printf  -  Printf on the console and split the string if it won't fit.
 *             I've never done word splits before, so I'm making this up
 *             as I go along.. I presume there is a 'proper' way to do it?
 */

void IRECONSOLE::PrintfCore(const char *linebuf)
{
char line[MAX_LINE_LEN];
char oldchar;
int len,pos;
char *ptr;

if(!curline)
	if(!Add())
		return;

len=strlen(linebuf);

pos = conpos;
//ilog_quiet("%d,%d\n[%s]\n",pos,len,linebuf);

// Check for simple case

if(pos+len<=Cols)
	{
//	ilog_quiet("SC{%s}\n",linebuf);
	Print(linebuf);
	return;
	}

SAFE_STRCPY(line,linebuf);

do	{
	// Punch a hole.
	oldchar=line[Cols-pos];
	line[Cols-pos]=0;
	ptr=strrchr(line,' ');
	if(ptr)
		{
		*ptr=0; // Okay, punch the hole here instead, fix old hole
		line[Cols-pos]=oldchar;
//		ilog_quiet("PFOH{%s}\n",line);
		Print(line);
		Newline();
		pos=0;
		}
	else
		{
		// If there's something in front, start a new line instead
		if(pos>0)
			{
//			ilog_quiet("STNL{%s}\n",line);
			Newline();
			Printf(line);
			return;
			}
		else // otherwise do emergency break
			{
//			ilog_quiet("EMB{%s}\n",line);
			ptr=&line[Cols-pos];
			Print(line);
			Newline();
			pos=0;
			}
		}
	ptr++;

	strcpy(line,ptr);
	len=strlen(line);
	} while(len > Cols);

//ilog_quiet("REMAINDER{%s}\n",line);
Print(line);
}

