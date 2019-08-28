
/*
 *      NuSpeech - A script-based conversation engine
 *                 Beware: the parser is a bit ugly
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ithelib.h"
#include "core.hpp"
#include "textfile.h"
#include "gamedata.hpp"
#include "console.hpp"
#include "loadfile.hpp"
#include "media.hpp"
#include "init.hpp"
#include "oscli.hpp"
#include "sound.h" // need to poll the sound system
#include "mouse.hpp"
#include "library.hpp"
#include "linklist.hpp"

#define MAXFLAGS 8192

char tFlag[MAXFLAGS];
char *tFlagIdentifier[MAXFLAGS];

void Wipe_tFlags();
int Get_tFlag(const char *name);
void Set_tFlag(const char *name,int a);
void NPC_set_lFlag(OBJECT *o,const char *page, const char *reader, int state);
int NPC_get_lFlag(OBJECT *o,const char *page, const char *reader);
char *NPC_MakeName(OBJECT *o);

static int numFlags=0;
static struct TF_S conversation;       // TextFile class instance
static char img[256];
static char curpage[256];
static char autolink[256];
static char esclink[256];
static char backing[256];
static char leftlink[256];
static char rightlink[256];
static char exitvrm[64];
static char links[10][5][256];   // Ten is the number of numeric keys..
static char linktemp[256];
static int linkptr,y,link_x;
static unsigned char scan_only=0,link_now=0,link_r,link_g,link_b,def_r,def_g,def_b;
static const char *curfile;
static int curline;
static IREBITMAP *image,*picbuf;
static IREBITMAP *leftarrow,*rightarrow,*ticksprite;
static char picpath[256];
static OBJECT *npc;
static char *objname;
static int userfont,PageDelay,NumPages=0;
static char **NpcSession;


int NPC_Converse(const char *file,const char *startpage);
void NPC_BeenRead(OBJECT *o,const char *page, const char *reader);
int NPC_WasRead(OBJECT *o,const char *page, const char *reader);
void NPC_ReadWipe(OBJECT *o, int wflags);

static int  NPC_ParseCode(char *line, const char *reader);
static int  NPC_GetPage(const char *line, int clr, const char *reader);
static int  NPC_GetEnd(int start, const char *reader);
static int  NPC_GetPageLine(const char *page);
static void NPC_ResetLinks();
static void NPC_AddLink(const char *msg,const char *dest);
static void NPC_AltLinkM(const char *msg);
static void NPC_AltLinkD(const char *dest);
static void toke(char *string);
static int toker(char *string);
static OBJECT *ChoosePartyMember();
static void InitConvSprites();

static int NPC_CountPages();
static int NPC_InSession(const char *page);
static void NPC_AddSession(const char *page);
static void NPC_NewSession();
static void NPC_EndSession();
static void NPC_EraseLink(int link);
static void NPC_PurgeLinks();

extern int TakeQuantity(OBJECT *container,char *objectname,int qty);
extern void AddQuantity(OBJECT *container,char *objectname,int qty);
extern int MoveQuantity(OBJECT *src,OBJECT *dest,char *objectname,int qty);
extern void CallVM(char *name);
extern int IsOnscreen(char *pname);
extern int SumObjects(OBJECT *cont, char *name, int total);
extern void CheckSpecialKeys(int k);

//
//  This is the structure of the links[] array.
//  There are ten links.  Each link has five 256-byte arrays attached to it.
//
//  links[n][0] : 'Message', i.e. what is displayed onscreen
//  links[n][1] : 'Dest' the page that the link takes you to
//  links[n][2] : 'AltMessage', alternative message if link already visited
//  links[n][3] : 'AltDest', alternative destination if link already visited
//  links[n][4] : Data.  [0] is protection flag for the [always] command
//



/*
 *      NPC_Converse(file,startpage) - User function to start a conversation.
 *                                     File is the path to the conversation
 *                                     file using standard IRE conventions.
 *                                     startpage is a page title, or NULL
 *                                     to use 'start' as the starting page,
 */

static int start,end,linectr,fh,toph;

int NPC_Converse(const char *file,const char *startpage)
{
int k,lines,talking,ctr,jumpline,ready,ret;
char msg[5];
char buffer[1024];
char temp[1024];
char *linkpage;
OBJREGS oldvars;
char *playername;

InitConvSprites();

//IRE_HideMouse(); // Draw mouse to physical screen not swap
swapscreen->ShowMouse();

ret = 1; // Assume success
SaveRanges(); // Store old mouse ranges

userfont=speechfont;
fh = irecon_font(userfont); // Load new font, get height
toph = 8*(fh/8);

// Set the NPC we're talking to, especially if it's a link to another object

if(current_object)
	{
	if(GetNPCFlag(current_object,IS_SYMLINK) && current_object->stats->owner.objptr)
		npc=current_object->stats->owner.objptr;
	else
		npc=current_object;
	objname=current_object->personalname;	// Get name of object we're talking to
	}
else
	{
	npc=NULL;
	objname="NoName";
	}

playername = NPC_MakeName(player);

// Set default colour for links to be default (Pale blue)

link_r = dlink_r;
link_g = dlink_g;
link_b = dlink_b;
def_r = dtext_r;
def_g = dtext_g;
def_b = dtext_b;

link_x = 0;        // Default to X=0, left edge of the screen

// Find the path to load pictures from
// First, get the current path

strcpy(buffer,file);
strslash(buffer);

if(!strrchr(buffer,'/'))
	strcpy(picpath,"pics");
else
	{
	*strrchr(buffer,'/')=0;
	strcpy(picpath,buffer);
	strcat(picpath,"/pics");
	}
strcpy(buffer,"\0");
strcpy(linktemp,"\0");

irecon_savecol(); // Preserve console colour

TF_init(&conversation);

// Ok, now open the conversation

TF_load(&conversation,file);
curfile=file;

// Open the session log
NPC_NewSession();

IRE_HideMouse();
SaveScreen();
swapscreen->Clear(ire_black);

strcpy(backing,"\0");
strcpy(exitvrm,"\0");
strcpy(esclink,"\0");
talking=1;           // Just Keep Talking

start = -1;

// Look for a given starting page
if(startpage)
	start=NPC_GetPage(startpage,1,playername);

// If we couldn't find it or there wasn't one, use regular startpage
if(start == -1)
	{
	// Find default starting page
	start=NPC_GetPage("start",1,playername);
	// If we know the NPC's name, see if there's a StartName page
	if(npc)
		if(GetNPCFlag(npc,KNOW_NAME))
			{
			ctr=NPC_GetPage("startname",0,playername);
			if(ctr>=0)
				start=ctr;
			ctr=NPC_GetPage("start_name",0,playername);
			if(ctr>=0)
				start=ctr;
			}
	}

// Still no start page?  Shut down
if(start==-1)
	{
	ret=0;
	goto uncouth_shutdown;
	}

lines=50;
k=-1;
link_now=0;
linkptr=-1;

if(lines>conversation.lines)
	lines=conversation.lines;

end=NPC_GetEnd(start,playername);

FlushKeys();
do  {
	ClearMouseRanges();
//	AddMouseRange(IREKEY_ESC,0,0,640,400); // Right click on text quits
//	RightClick(IREKEY_ESC);

	y=toph;
	strcpy(img,"\0");
	strcpy(temp,"\0");
	strcpy(curpage,"\0");
	strcpy(autolink,"\0");
	strcpy(leftlink,"\0");
	strcpy(rightlink,"\0");
	NPC_ResetLinks();
	PageDelay=0;

	// Parse the page, displaying it, running scripts and storing links

	for(linectr=start;linectr<end;linectr++)
		{
		curline=linectr;
		strcpy(buffer,conversation.line[linectr]);
		toke(buffer);
		if(buffer[0] != '[')
			{
			irecon_printxy(0,y+=fh,buffer);
			}
		else
			{
			NPC_ParseCode(buffer,playername);
			// If we have an instantaneous page change, do it
			if(link_now)
				{
				jumpline=NPC_GetPage(autolink,1,playername);
				if(jumpline>=0)
					{
					linectr=jumpline;
					start=jumpline;
					end=NPC_GetEnd(start,playername);
					y=toph;
					strcpy(curpage,autolink);
					// Wipe state variables
					strcpy(img,"\0");
					strcpy(temp,"\0");
					//strcpy(curpage,"\0");
					strcpy(autolink,"\0");
					strcpy(leftlink,"\0");
					strcpy(rightlink,"\0");
					NPC_ResetLinks();
					}
				if(jumpline == -1)
					Bug("Failed switch to page '%s' in page %s at '%s:%d'\n",autolink,curpage,curfile,curline);

				if(jumpline == -2)
					{
					ret=0;
					goto uncouth_shutdown;   // Bad, bad Doug!
					}
				link_now=0;
				}
			}
		}

	// Remove any redundant page links

	NPC_PurgeLinks();

	// Now display the page links

	irecon_colour(link_r,link_g,link_b);   // Colour for the menu

	y+=fh;
	for(ctr=0;ctr<linkptr;ctr++)
		{
		if(ctr<9)
			itoa(ctr+1,msg,10);
		else
			strcpy(msg,"0");
		irecon_printxy(link_x,y,msg);
		if(links[ctr][3][0]!=0)     // Is there an alternate title?
			{
			if(NPC_WasRead(npc,links[ctr][0],playername)) // Should we use it?
				irecon_printxy(link_x+(fh*2),y,links[ctr][3]);   // Yes
			else
				irecon_printxy(link_x+(fh*2),y,links[ctr][1]);   // No
			}
		else
			irecon_printxy(link_x+(fh*2),y,links[ctr][1]);       // No there wasn't
		AddMouseRange(IREKEY_1+ctr,link_x,y,(fh*2)+(fh*strlen(links[ctr][1])),fh);
		SetRangePointer(IREKEY_1+ctr,1);
		y+=fh;
		}

	if(PageDelay)
		{
		ShowSimple();
		IRE_WaitFor(PageDelay*1000);
		FlushKeys();
		}

	irecon_colour(def_r,def_g,def_b);

	// If we have an autolink, display a little icon
	if(autolink[0])
		{
		if(!istricmp(autolink,"exit"))
			{
			// Exit, draw a tick
			if(ticksprite)
				{
				ticksprite->Draw(swapscreen,580,440);
				AddMouseRange(IREKEY_SPACE,580,440,ticksprite->GetW(),ticksprite->GetH());
				SetRangePointer(IREKEY_SPACE,1);
				}
			}
		else
			{
			// Not the exit, use the right arrow
			if(rightarrow)
				{
				rightarrow->Draw(swapscreen,580,440);
				AddMouseRange(IREKEY_RIGHT,580,440,rightarrow->GetW(),rightarrow->GetH());
				SetRangePointer(IREKEY_RIGHT,1);
				}
			}
		}

	// If we have left or right links, display a little icon
	if(leftlink[0] && leftarrow)
		{
		leftarrow->Draw(swapscreen,8,440);
		AddMouseRange(IREKEY_LEFT,8,440,leftarrow->GetW(),leftarrow->GetH());
		SetRangePointer(IREKEY_LEFT,1);
		}

	// If we have a right link, display a little icon
	if(rightlink[0])
		{
		if(!istricmp(rightlink,"exit"))
			{
			// Exit, draw a tick
			if(ticksprite)
				{
				ticksprite->Draw(swapscreen,580,440);
				AddMouseRange(IREKEY_ESC,580,440,ticksprite->GetW(),ticksprite->GetH());
				SetRangePointer(IREKEY_ESC,1);
				rightlink[0]=0; // Deactivate the Right key. Use ESC..
				                // prevents user 'falling' out of the book
				}
			}
		else
			{
			// Not the exit, use the right arrow
			if(rightarrow)
				{
				rightarrow->Draw(swapscreen,580,440);
				AddMouseRange(IREKEY_RIGHT,580,440,rightarrow->GetW(),rightarrow->GetH());
				SetRangePointer(IREKEY_RIGHT,1);
				}
			}
		}

	// If no links, print an error and finish

	if(!autolink[0] && !linkptr && !leftlink[0] && !rightlink[0])
		{
		irecon_colour(255,0,0);
		irecon_font(0);
		irecon_printxy(16,y,"No links on this page.  Press a key.");
		irecon_font(userfont);
		irecon_colour(def_r,def_g,def_b);
		Bug("No links for page '%s' in '%s:%d'\n",curpage,curfile,curline);
		strcpy(autolink,"exit"); // This will make it exit immediately
		ret=0;
		}

	ShowSimple();
	swapscreen->ShowMouse();

	do
		{
		ready=0;
		FlushKeys();
		IRE_ShowMouse();
		k=WaitForKey();
		swapscreen->ShowMouse();

		CheckSpecialKeys(k);	// F10, CTRL-BREAK

		if(k==IREKEY_MOUSE) // Mouse click?  Get the 'virtual key'
			{
			k=MouseID;
			WaitForMouseRelease();
			}

		if(k==IREKEY_ESC)
			{
			ilog_quiet("esclink='%s'\n",esclink);
			if(esclink[0] == 0)             // Special command for ESC?
				{
				talking=0;                  // No, just exit
				ready=1;
				}
			else
				if(istricmp(esclink,"disabled"))     // If not disabled,
					strcpy(autolink,esclink);       // go to the page
			}

		if(leftlink[0] != 0)
			if(k==IREKEY_LEFT)
				{
				strcpy(autolink,leftlink);
				ready=1;
				}

		if(rightlink[0] != 0)
			if(k==IREKEY_RIGHT)
				{
				strcpy(autolink,rightlink);
				ready=1;
				}

		if(autolink[0] != 0)        // If there's an automatic page change
			{
			ready=1;
			ctr=NPC_GetPage(autolink,1,playername);
			if(ctr>=0)
				{
				start=ctr;
				end=NPC_GetEnd(start,playername);
				}
			if(ctr==-2)             // -2 means end-of-conversation
				talking=0;
			}
		else
			{
			ctr = k - IREKEY_1;
			if(k == IREKEY_0)
				ctr=9;
			if(ctr>=0 && ctr<linkptr)
				{
				ready=1;
				linkpage = links[ctr][0];            // Use the default page

				if(links[ctr][2][0] != 0)            // Is there an alternate?
					if(NPC_WasRead(npc,links[ctr][0],playername)) // Go there?
						linkpage=links[ctr][2];      // Yes we do

				NPC_BeenRead(npc,links[ctr][0],playername);     // Mark as read
				NPC_AddSession(links[ctr][0]);              // Seen it

				start=NPC_GetPage(linkpage,1,playername);
				if(start==-1)   // Error in code
					{
					Bug("Conversation: broken link.  Could not find page '%s' in file '%s'\n",linkpage,curfile);
					ret=0;
					goto uncouth_shutdown;
					}
				if(start==-2)   // End of conversation
					talking = 0;
				end=NPC_GetEnd(start,playername);
				}
			}
		} while(!ready);

	} while(talking);

uncouth_shutdown:

M_free(playername);

FlushKeys();

TF_term(&conversation);
RestoreScreen();
swapscreen->ShowMouse();

irecon_loadcol(); // Restore console colour
irecon_font(0); // Restore font

if(exitvrm[0] != '\0')
	{
	VM_SaveRegs(&oldvars);
	ilog_quiet("Calling %s at exit:\n",exitvrm);
	CallVM(exitvrm);
	VM_RestoreRegs(&oldvars);
	}

RestoreRanges(); // Restore original mouse ranges
NPC_EndSession(); // Free session log
return ret;
}

/*
 *      NPC_ParseCode(line) - Parse the given line and return an opcode.
 *                            This is the core of the conversation engine.
 */


int NPC_ParseCode(char *line, const char *reader)
{
char line1[1024];
char line2[1024];
char filename[1024];
char temp[256];
char tmp[32];
char *pos;
OBJECT *obj;
OBJREGS oldvars;
int rgb,r,g,b;

if(!line)
    return 0;

strcpy(line1,line);

strdeck(line1,'[');
strdeck(line1,'\"');
strdeck(line1,']');
strdeck(line1,0xa);
strdeck(line1,0xd);
strstrip(line1);

strcpy(line2,strfirst(line1));

if(!istricmp(line2,"page") || !istricmp(line2,"page="))
    {
    strcpy(curpage,strrest(line1));
    strstrip(curpage);
    return 2;
    }

if(!istricmp(line2,"endpage"))
    return 3;

if(!istricmp(line2,"link_colour=") || !istricmp(line2,"link_color=")
|| !istricmp(line2,"linkcolour=") || !istricmp(line2,"linkcolor=")
|| !istricmp(line2,"links=") || !istricmp(line2,"links="))
    {
    pos=strchr(line,'#');
    if(!pos)
        {
        Bug("Colour is not specified properly in %s:%d\n",curfile,curline);
        return 30;
        }
    pos++;
    strcpy(line2,pos);  // Isolate just the hex number
    line2[7]=0;
    sscanf(line2,"%x",&rgb);   // Got it in binary.
    r=(rgb>>16)&0xff;   // get Red
    g=(rgb>>8)&0xff;    // get green
    b=rgb&0xff;         // get blue

    pos=strchr(line,']');
    if(!pos)
        {
        Bug("Colour is not specified properly in %s:%d\n",curfile,curline);
        return 30;
        }

    // Use this colour for the menu links

    link_r = r;
    link_g = g;
    link_b = b;

    return 30;
    }

if(!istricmp(line2,"link_offset=") || !istricmp(line2,"linkoffset=")
|| !istricmp(line2,"link_x=") || !istricmp(line2,"linkx="))
    {
    pos=strchr(line,'=');
    if(!pos)
        {
        Bug("Missing = in %s:%d\n",curfile,curline);
        return 31;
        }
    pos++;
    strcpy(line2,pos);  // Isolate just the hex number
    strdeck(line2,']');
    strdeck(line2,'\"');
    sscanf(line2,"%d",&rgb);   // Got it in binary.

    // Use this offset for the menu links

    link_x = rgb;

    return 31;
    }

if(!istricmp(line2,"font") || !istricmp(line2,"font="))
	{
	strcpy(img,strrest(line1));
	strstrip(img);

	// IMG is now the name of the font to load
	userfont = atoi(img);
	fh = irecon_font(userfont); // Load new font, get height
	toph = 8*(fh/8);

	return 13;
	}

// If we don't have a link yet, look for a default
if(!esclink[0])
	if(!istricmp(line2,"escpage") || !istricmp(line2,"escpage=")
	|| !istricmp(line2,"esc") || !istricmp(line2,"esc="))
	    {
	    strcpy(esclink,strrest(line1));
	    strstrip(esclink);
	    return 4;
	    }

if(scan_only)
    return 0;

if(!istricmp(line2,"backing") || !istricmp(line2,"backing="))
	{
	strcpy(img,strrest(line1));
	strstrip(img);

	// Try same directory as
	strcpy(temp,picpath);
	strcat(temp,"/");
	strcat(temp,img);
	if(loadfile(temp,filename))
		{
		picbuf = iload_bitmap(filename);
		swapscreen->Clear(ire_black);
		picbuf->DrawSolid(swapscreen,0,0);
		delete picbuf;
		}
	else
		{
		strcpy(temp,"backings/");
		strcat(temp,img);
		if(loadfile(temp,filename))
			{
			picbuf = iload_bitmap(filename);
			swapscreen->Clear(ire_black);
//        masked_blit(picbuf,swapscreen,0,0,0,0,picbuf->w,picbuf->h);
			picbuf->DrawSolid(swapscreen,0,0);
			delete picbuf;
			}
		else
			{
			strcpy(temp,picpath);
			strcat(temp,"/");
			strcat(temp,img);
			irecon_colour(255,0,0);
			irecon_printxy(8,8,temp); // Print duff filename
			irecon_colour(def_r,def_g,def_b);
			Bug("Could not load image file '%s' in %s\n",temp,curfile);
			}
		}

	return 13;
	}


if(!istricmp(line2,"image") || !istricmp(line2,"image="))
    {
    strcpy(img,strrest(line1));
    strstrip(img);

    // IMG is now the name of the sprite to be used

    strcpy(temp,picpath);
    // Try pics16 first, if in 16bpp mode
    if(ire_bpp < 24)
	{
	strcat(temp,"16");
	strcat(temp,"/");
	strcat(temp,img);
	if(!loadfile(temp,filename))
	    {
	    // Revert to default
	    strcpy(temp,picpath);
	    strcat(temp,"/");
	    strcat(temp,img);
	    }
	}
    else
        {
	strcat(temp,"/");
	strcat(temp,img);
        }
    
    if(loadfile(temp,filename))
        {
        image=iload_bitmap(filename);
	image->Draw(swapscreen,(640-image->GetW())/2,0);
        r = image->GetH();
//        if(r>toph)                   // If it's bigger than 128 pixels
            y=r;                 // then make room for it.
        delete image;
        }
    else
        {
        irecon_colour(255,0,0);
        irecon_printxy(8,8,temp); // Print duff filename
        irecon_colour(def_r,def_g,def_b);
        Bug("Could not load image file '%s' in %s\n",temp,curfile);
        }

    return 1;
    }

if(!istricmp(line2,"nextpage") || !istricmp(line2,"nextpage="))
	{
	if(autolink[0]!=0)  // Already an automatic link.  Use by preference.
		{
		return 4;
		}
	strcpy(autolink,strrest(line1));
	strstrip(autolink);
	return 4;
	}

if(!istricmp(line2,"left") || !istricmp(line2,"left="))
	{
	strcpy(leftlink,strrest(line1));
	strstrip(autolink);
	return 4;
	}

if(!istricmp(line2,"right") || !istricmp(line2,"right="))
	{
	strcpy(rightlink,strrest(line1));
	strstrip(autolink);
	return 4;
	}

if(!istricmp(line2,"goto") || !istricmp(line2,"goto="))
    {
    strcpy(line2,strrest(line1));
    strstrip(line2);
    if(NPC_GetPageLine(line2) == -1)
        {
        Bug("Page not found for [goto=\"%s\"] in %s:%d\n",line2,curfile,curline);
//        strcpy(autolink,"exit");
        }
    else
        {
	//printf("On '%s', going to '%s'\n",curpage,line2);
	if(!istricmp(line2,curpage))
		{
		Bug("Goto current page '%s' in %s:%d\n",curpage,curfile,curline);
		return 4;
		}

        strcpy(autolink,line2);
        link_now=1;
        }
    return 4;
    }

if(!istricmp(line2,"append") || !istricmp(line2,"append="))
	{
	strcpy(line2,strrest(line1));
	strstrip(line2);
	if(NPC_GetPageLine(line2) == -1)
		{
		Bug("Page not found for [append=\"%s\"] in %s:%d\n",line2,curfile,curline);
//        strcpy(autolink,"exit");
		}
	else
		{
		start=NPC_GetPage(line2,0,reader);
		end=NPC_GetEnd(start,reader);
		curline=linectr=start;
		}
	return 4;
	}

if(!istricmp(line2,"random_page") || !istricmp(line2,"random_page="))
	{
	strcpy(line2,strrest(line1));
	strstrip(line2);
	hardfirst(line2);
	// get first number
	r=atoi(strrest(strrest(line1)));
	// get last number
	g=atoi(strrest(strrest(strrest(line1))));
	if(g)
		{
		// find the number
		b=rand()%g;
		if(b<10)
			strcat(line2,"0");
		itoa(b,tmp,10);
		strcat(line2,tmp);
		}
	else
		{
		Bug("Random Page: invalid range in %s:%d\n",curfile,curline);
		return 4;
		}

    if(NPC_GetPageLine(line2) == -1)
        {
        Bug("Random Page \"%s\" not found in %s:%d\n",line2,curfile,curline);
//        strcpy(autolink,"exit");
        }
    else
        {
        strcpy(autolink,line2);
        link_now=1;
        }
    return 4;
    }

if(!istricmp(line2,"link") || !istricmp(line2,"link="))
    {
    strcpy(linktemp,strrest(line1));
    strstrip(linktemp);
    return 5;
    }

if(!istricmp(line2,"linkto") || !istricmp(line2,"linkto="))
    {
    NPC_AddLink(strrest(line1),linktemp);
    strcpy(linktemp,"\0");
    return 6;
    }

if(!istricmp(line2,"alt_link") || !istricmp(line2,"alt_link=")
|| !istricmp(line2,"altlink") || !istricmp(line2,"altlink="))
    {
    NPC_AltLinkM(strrest(line1));
    return 5;
    }

if(!istricmp(line2,"alt_linkto") || !istricmp(line2,"alt_linkto=")
|| !istricmp(line2,"altlinkto") || !istricmp(line2,"altlinkto="))
    {
    NPC_AltLinkD(strrest(line1));
    return 6;
    }

if(!istricmp(line2,"once"))
    {
    if(linkptr == 0)
        {
        Bug("[once] before [link_to] in %s:%d, should come afterwards\n",curfile,curline);
        return 6;
        }
    if(NPC_WasRead(npc,links[linkptr-1][0],reader)) // If it's been read
        {                                              // remove it
        linkptr--;
        strcpy(links[linkptr][0],"\0");
        strcpy(links[linkptr][1],"\0");
        strcpy(links[linkptr][2],"\0");
        strcpy(links[linkptr][3],"\0");
        }
    return 6;
    }

if(!istricmp(line2,"always"))
	{
	if(linkptr == 0)
		{
		Bug("[always] before [link_to] in %s:%d, should come afterwards\n",curfile,curline);
		return 6;
		}
	// Set the protect flag
	links[linkptr-1][4][0]=1;
	return 6;
	}


// Delete record of which pages we've seen
if(!istricmp(line2,"resetlinks") || !istricmp(line2,"reset_links"))
	{
	NPC_ReadWipe(npc,0);
	return 4;
	}

if(!istricmp(line2,"if"))
    {
    if(!Get_tFlag(strfirst(strrest(line1))))
        return 8;
    pos=strchr(line,']');
    if(!pos)
        {
        Bug("Missing ] in %s:%d\n",curfile,curline);
        return 8;
        }
    pos++;

    if(pos[0]=='[')
        return(NPC_ParseCode(pos,reader));
    else
        irecon_printxy(0,y+=fh,pos);
    return 8;
    }

if(!istricmp(line2,"if_not"))
    {
    if(Get_tFlag(strfirst(strrest(line1))))
        return 8;
    pos=strchr(line,']');
    if(!pos)
        {
        Bug("Missing ] in %s:%d\n",curfile,curline);
        return 8;
        }
    pos++;

    if(pos[0]=='[')
        return(NPC_ParseCode(pos,reader));
    else
        irecon_printxy(0,y+=fh,pos);
    return 8;
    }

if(!istricmp(line2,"if_local"))
	{
	if(!NPC_get_lFlag(npc,strfirst(strrest(line1)),reader))
		return 8;

	pos=strchr(line,']');
	if(!pos)
		{
		Bug("Missing ] in %s:%d\n",curfile,curline);
		return 8;
		}
	pos++;

	if(pos[0]=='[')
		return(NPC_ParseCode(pos,reader));
	else
		irecon_printxy(0,y+=fh,pos);
	return 8;
	}

if(!istricmp(line2,"if_not_local"))
    {
    if(NPC_get_lFlag(npc,strfirst(strrest(line1)),reader))
        return 8;
    pos=strchr(line,']');
    if(!pos)
        {
        Bug("Missing ] in %s:%d\n",curfile,curline);
        return 8;
        }
    pos++;

    if(pos[0]=='[')
        return(NPC_ParseCode(pos,reader));
    else
        irecon_printxy(0,y+=fh,pos);
    return 8;
    }

if(!istricmp(line2,"if_seen"))
	{
	if(!NPC_WasRead(npc,strfirst(strrest(line1)),reader))
		return 8;
	pos=strchr(line,']');
	if(!pos)
		{
		Bug("Missing ] in %s:%d\n",curfile,curline);
		return 8;
		}
	pos++;

	if(pos[0]=='[')
		return(NPC_ParseCode(pos,reader));
	else
		irecon_printxy(0,y+=fh,pos);
	return 8;
	}

if(!istricmp(line2,"if_notseen") || !istricmp(line2,"if_not_seen"))
	{
	if(NPC_WasRead(npc,strfirst(strrest(line1)),reader))
		return 8;
	pos=strchr(line,']');
	if(!pos)
		{
		Bug("Missing ] in %s:%d\n",curfile,curline);
		return 8;
		}
	pos++;

	if(pos[0]=='[')
		return(NPC_ParseCode(pos,reader));
	else
		irecon_printxy(0,y+=fh,pos);
	return 8;
	}



if(!istricmp(line2,"call") || !istricmp(line2,"call=")
|| !istricmp(line2,"callvrm") || !istricmp(line2,"callvrm=")
|| !istricmp(line2,"call_vrm") || !istricmp(line2,"call_vrm="))
	{
	strcpy(temp,strrest(line1));
	strstrip(temp);
	if(temp[0] != '\0')
		{
		VM_SaveRegs(&oldvars);
		ilog_quiet("Calling %s\n",temp);
		CallVM(temp);
		VM_RestoreRegs(&oldvars);
		}
	strcpy(temp,"\0");
	return 9;
	}

if(!istricmp(line2,"set"))
    {
    Set_tFlag(strfirst(strrest(line1)),1);
    return 10;
    }

if(!istricmp(line2,"clr") || !istricmp(line2,"clear"))
    {
    Set_tFlag(strfirst(strrest(line1)),0);
    return 10;
    }

if(!istricmp(line2,"set_local"))
    {
    NPC_set_lFlag(npc,strfirst(strrest(line1)),reader,1);
    return 10;
    }

if(!istricmp(line2,"clr_local") || !istricmp(line2,"clear_local"))
    {
    NPC_set_lFlag(npc,strfirst(strrest(line1)),reader,0);
    return 10;
    }

if(!istricmp(line2,"colour=") || !istricmp(line2,"color="))
    {
    pos=strchr(line,'#');
    if(!pos)
        {
        Bug("Colour is not specified properly in %s:%d\n",curfile,curline);
        return 11;
        }
    pos++;
    strcpy(line2,pos);  // Isolate just the hex number
    line2[7]=0;
    sscanf(line2,"%x",&rgb);   // Got it in binary.
    r=(rgb>>16)&0xff;   // get Red
    g=(rgb>>8)&0xff;    // get green
    b=rgb&0xff;         // get blue

    pos=strchr(line,']');
    if(!pos)
        {
        Bug("Colour is not specified properly in %s:%d\n",curfile,curline);
        return 11;
        }
    pos++;

def_r = r;
def_g = g;
def_b = b;
irecon_colour(def_r,def_g,def_b);

// Decide whether to print the rest of the line or not

	strcpy(line2,pos);
	if(strlen(line2)>1)
		{
		toke(line2);
		irecon_printxy(0,y+=fh,line2);
		}

    return 11;
    }

if(!istricmp(line2,"remove") || !istricmp(line2,"destroy"))
    {
    strcpy(temp,strgetword(line,2));
    r = atoi(temp);
    strcpy(temp,strgetword(line,3));
    strdeck(temp,']');
    strdeck(temp,'\"');
    strstrip(temp);
    Set_tFlag("true",0);
    Set_tFlag("false",1);
	if(!bookview[0])
		if(TakeQuantity(player,temp,r))
		{
		Set_tFlag("true",1);
		Set_tFlag("false",0);
		}
    return 12;
    }

if(!istricmp(line2,"create"))
    {
    strcpy(temp,strgetword(line,2));
    r = atoi(temp);
    strcpy(temp,strgetword(line,3));
    strdeck(temp,']');
    strdeck(temp,'\"');
    strstrip(temp);
	if(!bookview[0])
		AddQuantity(player,temp,r);
    return 12;
    }

if(!istricmp(line2,"set_personal_flag")|| !istricmp(line2,"set_pflag"))
    {
    NPC_set_lFlag(npc,strgetword(line1,2),reader,1);
    return 14;
    }

if(!istricmp(line2,"clear_personal_flag") || !istricmp(line2,"clear_pflag")
|| !istricmp(line2,"clr_personal_flag")|| !istricmp(line2,"clr_pflag"))
    {
    NPC_set_lFlag(npc,strgetword(line1,2),reader,0);
    return 14;
    }

if(!istricmp(line2,"if_pflag") || !istricmp(line2,"ifpflag")
|| !istricmp(line2,"if_personal_flag") || !istricmp(line2,"ifpersonalflag"))
    {
    if(!NPC_get_lFlag(npc,strgetword(line1,2),reader))
        return 15;                                      // If not true, abort
    pos=strchr(line,']');
    if(!pos)
        {
        Bug("Missing ] in %s:%d\n",curfile,curline);
        return 15;
        }
    pos++;

    if(pos[0]=='[')
        return(NPC_ParseCode(pos,reader));
    else
        irecon_printxy(0,y+=fh,pos);
    return 15;
    }

if(!istricmp(line2,"if_npflag") || !istricmp(line2,"ifnpflag")
|| !istricmp(line2,"if_not_pflag") || !istricmp(line2,"ifnotpflag")
|| !istricmp(line2,"if_not_personal_flag") || !istricmp(line2,"ifnotpersonalflag"))
    {
    if(NPC_get_lFlag(npc,strgetword(line1,2),reader))
        return 15;                                    // If true, abort
    pos=strchr(line,']');
    if(!pos)
        {
        Bug("Missing ] in %s:%d\n",curfile,curline);
        return 15;
        }
    pos++;

    if(pos[0]=='[')
        return(NPC_ParseCode(pos,reader));
    else
        irecon_printxy(0,y+=fh,pos);
    return 15;
    }

if(!istricmp(line2,"is_in_party") || !istricmp(line2,"is_in_party?")
|| !istricmp(line2,"in_party") || !istricmp(line2,"in_party?"))
    {
    Set_tFlag("true",0);
    Set_tFlag("false",1);
    for(r=0;r<MAX_MEMBERS;r++)
        if(party[r] == npc)
            {
            Set_tFlag("true",1);
            Set_tFlag("false",0);
            }
    return 16;
    }

if(!istricmp(line2,"on_exit_call") || !istricmp(line2,"on_exit_call=")
|| !istricmp(line2,"at_exit_call") || !istricmp(line2,"at_exit_call=")
|| !istricmp(line2,"call_at_exit") || !istricmp(line2,"call_at_exit=")
|| !istricmp(line2,"call_on_exit") || !istricmp(line2,"call_on_exit=")
|| !istricmp(line2,"at_exit_callvrm") || !istricmp(line2,"at_exit_callvrm=")
|| !istricmp(line2,"at_exit_call_vrm") || !istricmp(line2,"at_exit_call_vrm=")
|| !istricmp(line2,"on_exit_callvrm") || !istricmp(line2,"on_exit_callvrm=")
|| !istricmp(line2,"on_exit_call_vrm") || !istricmp(line2,"on_exit_call_vrm="))
    {
    strcpy(exitvrm,strrest(line1));
    strstrip(exitvrm);
    return 17;
    }

if(!istricmp(line2,"am_carrying") || !istricmp(line2,"is_carrying")
|| !istricmp(line2,"is_in_pocket"))
    {
    Set_tFlag("true",0);
    Set_tFlag("false",1);
    strcpy(temp,strgetword(line,2));
    strdeck(temp,']');
    strdeck(temp,'\"');
    strstrip(temp);
	for(obj=player->pocket.objptr;obj;obj=obj->next)
		{
		if(obj->flags & IS_ON)
			if(!istricmp(obj->name,temp))
				{
				Set_tFlag("true",1);
				Set_tFlag("false",0);
				return 18;
				}
		}
	return 18;
	}

if(!istricmp(line2,"is_npc_carrying") || !istricmp(line2,"are_they_carrying"))
    {
    Set_tFlag("true",0);
    Set_tFlag("false",1);
    strcpy(temp,strgetword(line,2));
    strdeck(temp,']');
    strdeck(temp,'\"');
    strstrip(temp);
	for(obj=current_object->pocket.objptr;obj;obj=obj->next)
		{
		if(obj->flags & IS_ON)
			if(!istricmp(obj->name,temp))
				{
				Set_tFlag("true",1);
				Set_tFlag("false",0);
				return 18;
				}
		}
	return 18;
	}

if(!istricmp(line2,"are_there"))
    {
    strcpy(temp,strgetword(line,2));
    r = atoi(temp);
    strcpy(temp,strgetword(line,3));
    strdeck(temp,']');
    strdeck(temp,'\"');
    strstrip(temp);
    Set_tFlag("true",0);
    Set_tFlag("false",1);
	if(!bookview[0])
		if(SumObjects(player,temp,0)>=r)
			{
			Set_tFlag("true",1);
			Set_tFlag("false",0);
			}
    return 18;
    }

// Take from Player and give to the guy you're talking to

if(!istricmp(line2,"take"))
    {
    strcpy(temp,strgetword(line,2));
    r = atoi(temp);
    strcpy(temp,strgetword(line,3));
    strdeck(temp,']');
    strdeck(temp,'\"');
    strstrip(temp);
    Set_tFlag("true",0);
    Set_tFlag("false",1);
	if(!bookview[0])
		if(MoveQuantity(player,npc,temp,r))
			{
			Set_tFlag("true",1);
			Set_tFlag("false",0);
			}
    return 19;
    }

// Give to player, take from guy you're talking to

if(!istricmp(line2,"give"))
    {
    strcpy(temp,strgetword(line,2));
    r = atoi(temp);
    strcpy(temp,strgetword(line,3));
    strdeck(temp,']');
    strdeck(temp,'\"');
    strstrip(temp);
    Set_tFlag("true",0);
    Set_tFlag("false",1);
	if(!bookview[0])
		if(MoveQuantity(npc,player,temp,r))
			{
			Set_tFlag("true",1);
			Set_tFlag("false",0);
			}
    return 20;
    }

// Set behaviour

/*
if(!istricmp(line2,"setbehave") || !istricmp(line2,"setbehave=")
|| !istricmp(line2,"setbehaviour") || !istricmp(line2,"setbehaviour=")
|| !istricmp(line2,"behaviour") || !istricmp(line2,"behaviour=")
|| !istricmp(line2,"setbehavior") || !istricmp(line2,"setbehavior=")
|| !istricmp(line2,"behavior") || !istricmp(line2,"behavior="))
    {
    strcpy(temp,rest(line1));
    strstrip(temp);
    r = getnum4VRM(temp);
    if(r)
        npc->behave = getnum4VRM(r);
    strcpy(temp,"\0");
    return 21;
    }
*/
// Karma

if(!istricmp(line2,"add_karma") || !istricmp(line2,"add_karma=")
|| !istricmp(line2,"addkarma") || !istricmp(line2,"addkarma="))
    {
    strcpy(temp,strrest(line1));
    strstrip(temp);
    r = atoi(temp);
    if(r)
        player->stats->karma += r;
    strcpy(temp,"\0");
    return 22;
    }

if(!istricmp(line2,"sub_karma") || !istricmp(line2,"sub_karma=")
|| !istricmp(line2,"subkarma") || !istricmp(line2,"subkarma="))
    {
    strcpy(temp,strrest(line1));
    strstrip(temp);
    r = atoi(temp);
    if(r)
        player->stats->karma -= r;
    strcpy(temp,"\0");
    return 22;
    }

if(!istricmp(line2,"set_karma") || !istricmp(line2,"set_karma=")
|| !istricmp(line2,"setkarma") || !istricmp(line2,"setkarma="))
    {
    strcpy(temp,strrest(line1));
    strstrip(temp);
    r = atoi(temp);
    if(r)
        player->stats->karma = r;
    strcpy(temp,"\0");
    return 22;
    }

if(!istricmp(line2,"is_onscreen") || !istricmp(line2,"is_on_screen")
|| !istricmp(line2,"isonscreen"))
    {
    strcpy(temp,strrest(line1));
    strstrip(temp);
    Set_tFlag("true",0);
    Set_tFlag("false",1);
    if(IsOnscreen(temp))
        {
        Set_tFlag("true",1);
        Set_tFlag("false",0);
        }
    return 23;
    }

if(!istricmp(line2,"choosemember") || !istricmp(line2,"choose_member")
|| !istricmp(line2,"chooseparty") || !istricmp(line2,"choose_party"))
    {
    strcpy(temp,strrest(line1));
    strstrip(temp);
    Set_tFlag("true",0);
    Set_tFlag("false",1);
    victim = ChoosePartyMember();
    if(victim)
        {
        strcpy(autolink,temp);
        link_now=1;
        Set_tFlag("true",1);
        Set_tFlag("false",0);
        }
    return 24;
    }

if(!istricmp(line2,"iftime") || !istricmp(line2,"if_time")
|| !istricmp(line2,"iftime=") || !istricmp(line2,"if_time=")
|| !istricmp(line2,"iftimeis") || !istricmp(line2,"if_time_is"))
    {
    strcpy(temp,strrest(line1));
    strstrip(temp);
    r = sscanf(temp,"%d-%d",&g,&b);
    if(r<2 || g>b)
        {
        Bug("Error in if_time command in %s:%d\n",curfile,curline);
        return 25;
        }
    else
        {
        r = (game_hour * 100) + game_minute;
        if( r >= g && r <= b)
            {
            pos=strchr(line,']');
            if(!pos)
                {
                Bug("Missing ] in %s:%d\n",curfile,curline);
                return 25;
                }
            pos++;

            if(pos[0]=='[')
                return(NPC_ParseCode(pos,reader));
            else
                irecon_printxy(0,y+=fh,pos);
            }
        }
    return 25;
    }

if(!istricmp(line2,"ifntime") || !istricmp(line2,"if_not_time")
|| !istricmp(line2,"ifntime=") || !istricmp(line2,"if_not_time=")
|| !istricmp(line2,"ifntimeis") || !istricmp(line2,"if_not_time_is"))
    {
    strcpy(temp,strrest(line1));
    strstrip(temp);
ilog_quiet("ifntime: '%s'\n",temp);
    r = sscanf(temp,"%d-%d",&g,&b);
    if(r<2 || g>b)
        {
        Bug("Error in if_not_time command in %s:%d\n",curfile,curline);
        return 25;
        }
    else
        {
        r = (game_hour * 100) + game_minute;
ilog_quiet("r = %d, g = %d, b = %d: \n",r,g,b);
        if(!(r >= g && r <= b))
            {
ilog_quiet("phase 2\n");
            pos=strchr(line,']');
            if(!pos)
                {
                Bug("Missing ] in %s:%d\n",curfile,curline);
                return 25;
                }
            pos++;

ilog_quiet("pos='%s'\n",pos);
            if(pos[0]=='[')
		{
ilog_quiet("executeit='%s'\n",pos);
                return(NPC_ParseCode(pos,reader));
		}
            else
		{
ilog_quiet("printit='%s'\n",pos);
                irecon_printxy(0,y+=fh,pos);
		}
            }
ilog_quiet("stop\n");
        }
    return 25;
    }

if(!istricmp(line2,"knowname") || !istricmp(line2,"know_name")
|| !istricmp(line2,"learnname") || !istricmp(line2,"learn_name"))
    {
    SetNPCFlag(npc, KNOW_NAME);  // Learn their name
    return 26;
    }

if(!istricmp(line2,"ifknowname") || !istricmp(line2,"if_know_name")
|| !istricmp(line2,"if_knowname"))
    {
    if(GetNPCFlag(npc,KNOW_NAME))
        {
        pos=strchr(line,']');
        if(!pos)
            {
            Bug("Missing ] in %s:%d\n",curfile,curline);
            return 27;
            }
        pos++;

        if(pos[0]=='[')
            return(NPC_ParseCode(pos,reader));
        else
            irecon_printxy(0,y+=fh,pos);
        }
    return 27;
    }

if(!istricmp(line2,"ifnknowname") || !istricmp(line2,"if_not_know_name")
|| !istricmp(line2,"if_not_knowname") || !istricmp(line2,"if_nknowname"))
    {
    if(!GetNPCFlag(npc, KNOW_NAME))
        {
        pos=strchr(line,']');
        if(!pos)
            {
            Bug("Missing ] in %s:%d\n",curfile,curline);
            return 27;
            }
        pos++;

        if(pos[0]=='[')
            return(NPC_ParseCode(pos,reader));
        else
            irecon_printxy(0,y+=fh,pos);
        }
    return 27;
    }

if(!istricmp(line2,"ifplayermale") || !istricmp(line2,"if_player_male")
|| !istricmp(line2,"if_male"))
    {
    if(!GetNPCFlag(player,IS_FEMALE))
        {
        pos=strchr(line,']');
        if(!pos)
            {
            Bug("Missing ] in %s:%d\n",curfile,curline);
            return 28;
            }
        pos++;

        if(pos[0]=='[')
            return(NPC_ParseCode(pos,reader));
        else
            irecon_printxy(0,y+=fh,pos);
        }
    return 28;
    }

if(!istricmp(line2,"ifplayerfemale") || !istricmp(line2,"if_player_female")
|| !istricmp(line2,"if_female"))
    {
    if(GetNPCFlag(player,IS_FEMALE))
        {
        pos=strchr(line,']');
        if(!pos)
            {
            Bug("Missing ] in %s:%d\n",curfile,curline);
            return 28;
            }
        pos++;

        if(pos[0]=='[')
            return(NPC_ParseCode(pos,reader));
        else
            irecon_printxy(0,y+=fh,pos);
        }
    return 28;
    }

if(!istricmp(line2,"ifplayerhero") || !istricmp(line2,"if_player_hero")
|| !istricmp(line2,"if_hero"))
    {
    if(GetNPCFlag(player,IS_HERO))
        {
        pos=strchr(line,']');
        if(!pos)
            {
            Bug("Missing ] in %s:%d\n",curfile,curline);
            return 29;
            }
        pos++;

        if(pos[0]=='[')
            return(NPC_ParseCode(pos,reader));
        else
            irecon_printxy(0,y+=fh,pos);
        }
    return 29;
    }

if(!istricmp(line2,"ifnplayerhero") || !istricmp(line2,"if_not_player_hero")
|| !istricmp(line2,"if_not_hero"))
    {
    if(!GetNPCFlag(player,IS_HERO))
        {
        pos=strchr(line,']');
        if(!pos)
            {
            Bug("Missing ] in %s:%d\n",curfile,curline);
            return 29;
            }
        pos++;

        if(pos[0]=='[')
            return(NPC_ParseCode(pos,reader));
        else
            irecon_printxy(0,y+=fh,pos);
        }
    return 29;
    }

if(!istricmp(line2,"escpage") || !istricmp(line2,"escpage=")
|| !istricmp(line2,"esc") || !istricmp(line2,"esc="))
    {
    strcpy(esclink,strrest(line1));
    strstrip(esclink);
    return 4;
    }

if(!istricmp(line2,"delay"))
	{
	strcpy(temp,strgetword(line,2));
    strdeck(temp,']');
	strstrip(temp);
	ilog_quiet("delay '%s'\n",temp);
	r = atoi(temp);
	if(r>0)
		PageDelay=r;
	return 30;
	}

if(!istricmp(line2,"journal") || !istricmp(line2,"set_journal")
|| !istricmp(line2,"setjournal"))
    {
    strcpy(temp,strrest(line1));
    strstrip(temp);
    char *strdata = getstring4name(temp);
    if(!strdata)
	{
        Bug("Error in journal in %s:%d\n",curfile,curline);
        Bug("Did not find string entry '%s'\n",temp);
        return 31;
	}
    
    int journalid = getnum4string(strdata);
    
    char date[64];
    make_journaldate(date);
    
    J_Add(journalid,days_passed+1,date);
    
    return 31;
    }

if(!istricmp(line2,"journal_done")) {
    strcpy(temp,strrest(line1));
    strstrip(temp);
    JOURNALENTRY *journal = J_Find(temp);
    if(!journal) {
	Bug("Did not find journal '%s'\n", temp);
        return 32;
    }
    journal->status = 100;
    
    return 32;
}

if(!istricmp(line2,"if_race") || !istricmp(line2,"if_race=")) {
    strcpy(temp,strgetword(line1,2));
    strstrip(temp);
//	printf("Check player race '%s'%s'\n", temp, player->labels->race);
    if(!istricmp_fuzzy(player->labels->race, temp)) {
        pos=strchr(line,']');
        if(!pos) {
            Bug("Missing ] in %s:%d\n",curfile,curline);
            return 33;
        }
        pos++;

        if(pos[0]=='[')
            return(NPC_ParseCode(pos,reader));
        else
            irecon_printxy(0,y+=fh,pos);
    }
    return 33;
}


ilog_quiet("Parse error in file '%s':\n>>%s\n",curfile,line);
return 0;
}

/*
 *      NPC_GetPageLine(title) - Find the string '[page="title"]' and return
 *                               the line number where it's found without any
 *                               additional processing
 */

int NPC_GetPageLine(const char *page)
{
char line1[1024];
char line2[1024];
int ctr;

if(!page)
    return -1;

if(!istricmp(page,"exit"))       // exit is a virtual page
    return -2;

for(ctr=0;ctr<conversation.lines;ctr++)
    {
    strcpy(line1,conversation.line[ctr]);

    strdeck(line1,'[');
    strdeck(line1,'\"');
    strdeck(line1,']');
    strdeck(line1,0xa);
    strdeck(line1,0xd);
    strstrip(line1);

    strcpy(line2,strfirst(line1));

    if(!istricmp(line2,"page") || !istricmp(line2,"page="))
        {
        strcpy(line2,strrest(line1));
        strstrip(line2);
        if(!istricmp(line2,page))
            return ctr;
        }
    }
return -1;
}

/*
 *      NPC_GetPage(title,clr) - Find the string '[page="title"]' and return
 *                               the line number where it's found
 */

int NPC_GetPage(const char *name, int clr, const char *reader)
{
int ctr;
char prev_scanonly;

if(clr)
	{
	swapscreen->Clear(ire_black);
	}

if(!istricmp(name,"exit"))
    return -2;
prev_scanonly=scan_only;
scan_only=1;
for(ctr=0;ctr<conversation.lines;ctr++)
    if(NPC_ParseCode(conversation.line[ctr],reader) == 2)
        {
        if(!istricmp(curpage,name))
            {
            scan_only=prev_scanonly;
            return ctr;
            }
        }
scan_only=prev_scanonly;
return -1;
}

/*
 *      NPC_GetEnd(starting link) - Find the end of the current page
 */

int NPC_GetEnd(int start, const char *reader)
{
int ctr,lines;
lines=conversation.lines;
if(start<0)
    return -1;
scan_only=1;
for(ctr=start;ctr<lines;ctr++)
    if(NPC_ParseCode(conversation.line[ctr],reader) == 3)
        {
        scan_only=0;
        return ctr;
        }
scan_only=0;
return lines;
}

/*
 *      NPC_CountPages() - Count the number of page entries
 */

int NPC_CountPages()
{
char line1[1024];
char line2[1024];
int ctr,num;

num=0;
for(ctr=0;ctr<conversation.lines;ctr++)
	{
	if(!conversation.line[ctr])
		continue;
	strcpy(line1,conversation.line[ctr]);
	strdeck(line1,'[');
	strdeck(line1,'\"');
	strdeck(line1,']');
	strdeck(line1,0xa);
	strdeck(line1,0xd);
	strstrip(line1);

	strcpy(line2,strfirst(line1));

	if(!istricmp(line2,"page") || !istricmp(line2,"page="))
		num++;
	}

return num;
}

/*
 *      NPC_ResetLinks() - Erase the links table
 */

void NPC_ResetLinks()
{
int ctr;

// Set links count to zero
linkptr=0;

// And delete any protection flags
for(ctr=0;ctr<10;ctr++)
	{
	strcpy(links[ctr][0],"<no description>");
	links[ctr][4][0]=0;
	}
return;
}

/*
 *      NPC_AddLink(title,address) - Add a link to the table
 */

void NPC_AddLink(const char *msg, const char *dest)
{
if(linkptr>=10)
    {
    Bug("Too many links in page '%s' in %s:%d\n",curpage,curfile,curline);
    return;
    }
strcpy(links[linkptr][0],msg);
strcpy(links[linkptr][1],dest);
strstrip(links[linkptr][0]);
strstrip(links[linkptr][1]);
strcpy(links[linkptr][2],"\0");  // Initialise for later
strcpy(links[linkptr][3],"\0");
links[linkptr][4][0]=0; // No protection flag
linkptr++;
}

/*
 *      NPC_AltLinkM(title) - Add alternate link title to the table
 */

void NPC_AltLinkM(const char *msg)
{
if(linkptr<1)
    {
    Bug("AltLink before LinkTo in page '%s' in %s:%d\n",curpage,curfile,curline);
    return;
    }
strcpy(links[linkptr-1][3],msg);
strstrip(links[linkptr-1][3]);
}

/*
 *      NPC_AltLinkD(address) - Add alternate Dest to the table
 */

void NPC_AltLinkD(const char *dest)
{
if(linkptr<1)
    {
    Bug("AltLinkTo before LinkTo in page '%s' in %s:%d\n",curpage,curfile,curline);
    return;
    }
strcpy(links[linkptr-1][2],dest);
strstrip(links[linkptr-1][2]);
}

/*
 *      toke(string) - search for and expand any system tokens in the string.
 *                     Modifies the given string so be sure it's not floating.
 *                     Calls toker.
 */

void toke(char *string)
{
for(;toker(string););	// Call it until there's nothing left to do.
}

/*
 *      toker(string) - Does the work.
 */

int toker(char *string)
{
char *a,*b;
DT_ITEM *replacement;
char temp[1024];
char token[128];
char tempx[32];
char got_quote=0,notail=0,ch;
char punctuation[]="  \0\0\0\0";
int k,ctr,bad;

// You may need to add to this list of offensive words

char *censor[][2]=
	{
		{"Fuck","F***",},
		{"fuck","f***",},
		{"FUCK","F***",},
		{"Shit","S***",},
		{"shit","s***",},
		{"SHIT","S***",},
		{"Crap","C***",},
		{"crap","c***",},
		{"CRAP","C***",},
		{"Bugger","B*****",},
		{"bugger","b*****",},
		{"BUGGER","B*****",},
		{"Bastard","B******",},
		{"bastard","b******",},
		{"BASTARD","B******",},
//		{"GW Bush","GW B***",},
		{NULL,NULL}
	};

// Censor 'bad words' ;-)

if(enable_censorware)
	for(ctr=0;censor[ctr][0];ctr++)
		{
		a=strstr(string,censor[ctr][0]);
		if(a)
			{
			bad=1;
			if(a != string) // Make sure we're not at start before backing up
				{
				// Get character before the word starts
				// If it's inside a word, we don't want to censor, e.g. scrap
				// if it's whitespace the word is bad and must die
				ch = *(a-1);
				switch(ch)
					{
					case '\"':
					case '\'':
						bad=1;
						break;

					default:
						if(!isxspace(ch))
							bad=0;
					}
				}
			if(bad) // censor it
				{
				// Patch the replacement in
				k=strlen(censor[ctr][0]);
				memcpy(a,censor[ctr][1],k);
				}
			}
		}

a=strchr(string,'$');
if(!a)
	{
	a=strchr(string,'{'); // matches with '}'
	if(!a)
		return 0;      // No special tokens, don't need to change it
	}
*a=0; // smash the token symbol away

notail=0;

strcpy(temp,string);    // All up to the token
strcpy(token,(a+1));    // Copy the token to buffer
// In case the token used '{' to start with, remove any terminators
strdeck(token,'}');

// Do we have an end quote at the end of the token?

b=strrchr(token,'\"');
if(b)
	{
	*b=0;	// Kill it, my cyborgs
	got_quote=1;
	}

// Make the token just one word
b=(char *)strrest(token);
if(b != NOTHING)
	*b=0;

strstrip(token);

//ilog_quiet("token = '%s'\n",token);

punctuation[0]=' ';
k=strlen(token);

if(strrest(a+1) == NOTHING)
	notail=1;

if(k>1)
	{
	// Several cases where we deal with punctuation.

	// Case 1.  "name." the name, followed by a terminator, e.g. !?.
	if(token[k-1] == '?' || token[k-1] == '!' || token[k-1] == '.')      //if(token[k-1]<'a' && token[k-1]<'A')
		{
		punctuation[0]=token[k-1];
		if(got_quote && notail)
			{
			punctuation[1]='\"';
			punctuation[2]=0; // Two spaces after a terminator.
			}
		else
			{
			punctuation[1]=' ';
			punctuation[2]=' ';
			punctuation[3]=0; // Two spaces after a terminator.
			}
		token[k-1]=0;
		}
	else // Case 2.  "name," the name, followed by a single punctuation mark
	if(token[k-1]<'A' && (token[k-1]<'0' || token[k-1]>'9')) // exclude nums
		{
		punctuation[0]=token[k-1];
		token[k-1]=0;
		}
	else // Case 3.  "name's" the name, followed by apostrophe and 's'
	if(token[k-2]=='\'' && token[k-1]=='s')
		{
		punctuation[0]='\'';
		punctuation[1]='s';
		punctuation[2]=' ';
		token[k-2]=0;
		}
	else // Case 4.  "name" No punctuation at all.
		punctuation[1]=0;
	}
	else
		punctuation[1]=0; // default

if(got_quote) // Add quote to the end of the line as necessary
	{
	k=strlen(punctuation)-1;
	if(k>0)
		{
		if(punctuation[k]==' ') // If there's a space, remove it
			punctuation[k]=0;
		if(punctuation[k]=='\"') // If there's already one, remove it
			punctuation[k]=0;
		}
	if(notail)
		strcat(punctuation,"\"");
	}

if(!strcmp(token,"PLAYER") || !strcmp(token,"PLAYERNAME"))
    {
//	ilog_quiet("PLAYA\n");
    strcat(temp,player->personalname);
    strcat(temp,punctuation);
    strcat(temp,strrest((a+1)));
    strcpy(string,temp);
    return 1;
    }

if(!strcmp(token,"CHARNAME") || !strcmp(token,"NAME"))
    {
	strcat(temp,npc->personalname);
    strcat(temp,punctuation);
    strcat(temp,strrest((a+1)));
    strcpy(string,temp);
    return 1;
    }

if(!strcmp(token,"OBJNAME"))
    {
    strcat(temp,objname);
    strcat(temp,punctuation);
    strcat(temp,strrest((a+1)));
    strcpy(string,temp);
    return 1;
    }

//ilog_quiet("token:'%s'\n",token);
if(!strcmp(token,"USERNUM1"))
	{
	sprintf(tempx,"%ld",pe_usernum1);
//	ilog_quiet("un: = '%s'\n",tempx);
	strcat(temp,tempx);
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
//	ilog_quiet("str = '%s'\n",string);
	return 1;
	}

if(!strcmp(token,"USERNUM2"))
	{
	sprintf(tempx,"%ld",pe_usernum2);
	strcat(temp,tempx);
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
	return 1;
	}

if(!strcmp(token,"USERNUM3"))
	{
	sprintf(tempx,"%ld",pe_usernum3);
	strcat(temp,tempx);
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
	return 1;
	}

if(!strcmp(token,"USERNUM4"))
	{
	sprintf(tempx,"%ld",pe_usernum4);
	strcat(temp,tempx);
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
	return 1;
	}

if(!strcmp(token,"USERNUM5"))
	{
	sprintf(tempx,"%ld",pe_usernum5);
	strcat(temp,tempx);
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
	return 1;
	}

if(!strcmp(token,"USERSTR1"))
	{
	if(pe_userstr1) {
		strcat(temp,pe_userstr1);
	}
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
	return 1;
	}

if(!strcmp(token,"USERSTR2"))
	{
	if(pe_userstr2) {
		strcat(temp, pe_userstr2);
	}
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
	return 1;
	}

if(!strcmp(token,"USERSTR3"))
	{
	if(pe_userstr3) {
		strcat(temp, pe_userstr3);
	}
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
	return 1;
	}

if(!strcmp(token,"USERSTR4"))
	{
	if(pe_userstr4) {
		strcat(temp, pe_userstr4);
	}
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
	return 1;
	}

if(!strcmp(token,"USERSTR5"))
	{
	if(pe_userstr5) {
		strcat(temp, pe_userstr5);
	}
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
	return 1;
	}

// Look for gender-sensitive tokens in the data tables

replacement=NULL;
if(GetNPCFlag(player,IS_FEMALE))
	replacement=GetTableCase("FemaleWords",token);
else
	replacement=GetTableCase("MaleWords",token);

// If we got one, change it
if(replacement)
	{
	strcat(temp,replacement->is);
	strcat(temp,punctuation);
	strcat(temp,strrest((a+1)));
	strcpy(string,temp);
	return 1;
	}

return 0;
}


/*
void NPC_DisplayFile(char *file)
{
int k,i,lines;
conversation.init(file);

i=0;
lines=50;
k=-1;

if(lines>conversation.lines)
    lines=conversation.lines;

SYS_KEY_FLUSH();
do  {
    itg_memset(swapscreen,0,itg_blitsize);
    for(int ctr=0;ctr<lines;ctr++)
        C_printxy(0,ctr<<3,conversation.line[ctr+i]);
    itg_update(swapscreen);
    k=SYS_KEY_WAIT();
    if(k == KEY_UP)
         {
         i--;
         if(i<0)
             i=0;
         }
    if(k == KEY_DOWN)
         {
         i++;
         if(i>conversation.lines-lines)
             i=conversation.lines-lines;
         }
    } while(k!=KEY_ESC);
SYS_KEY_FLUSH();

conversation.term();
}
*/

/*
 *    Reset all user-named game flags
 */

void NPC_Init()
{
int ctr;
for(ctr=0;ctr<MAXFLAGS;ctr++)
	{
	tFlag[ctr]=0;
	tFlagIdentifier[ctr]=NULL;
	}
numFlags=0;
}

/*
 *    Destroy all user-named game flags
 */

void Wipe_tFlags()
{
int ctr;
for(ctr=0;ctr<MAXFLAGS;ctr++)
	{
	tFlag[ctr]=0;
	if(tFlagIdentifier[ctr])
		{
		M_free(tFlagIdentifier[ctr]);
		tFlagIdentifier[ctr]=NULL;
		}
	}
numFlags=0;
}

/*
 *    Get state of a user-named game flag
 */

int Get_tFlag(const char *name)
{
int ctr;

for(ctr=0;ctr<=numFlags;ctr++)
	if(tFlagIdentifier[ctr])
		if(!istricmp(tFlagIdentifier[ctr],name))
			{
//		ilog_printf("match flag %s,%s does exist\n",tFlagIdentifier[ctr],name);
			return(tFlag[ctr]);
			}
return 0;
}

/*
 *    Change or create a user-named game flag
 */

void Set_tFlag(const char *name,int a)
{
int ctr,len;

for(ctr=0;ctr<MAXFLAGS;ctr++)
	if(tFlagIdentifier[ctr])
		{
		if(!istricmp(tFlagIdentifier[ctr],name))
			{
			tFlag[ctr]=a;
//            ilog_quiet("match flag %s,%s set to %d\n",tFlagIdentifier[ctr],name,a);
			return;
			}
		}
	else
		{
//            ilog_quiet("add flag %s set to %d\n",name,a);
		len = strlen(name);
		tFlagIdentifier[ctr]=(char *)M_get(1,len+1);
		strcpy(tFlagIdentifier[ctr],name);
		tFlag[ctr]=a;
		numFlags=ctr+1;
		return;
		}

ithe_panic("Set_tFlag: Too many flags defined: attempted to make flag called:",name);
}


/*
 *    Show a list of (max 9) party members and wait for user input
 */

OBJECT *ChoosePartyMember()
{
int ctr,members,k;
char msg[5];

//SYS_KEY_FLUSH();
FlushKeys();
members=0;
for(ctr=0;ctr<MAX_MEMBERS;ctr++)
    if(party[ctr])
        members++;

irecon_colour(0,0,160);

//y+=32;
for(ctr=0;ctr<members;ctr++)
    {
    itoa(ctr+1,msg,10);
    irecon_printxy(0,y,msg);
    if(party[ctr]->personalname)
        irecon_printxy((fh*2),y,party[ctr]->personalname);
    else
        irecon_printxy((fh*2),y,party[ctr]->name);
    y+=fh;
    }

irecon_printxy(0,y,"0");
irecon_printxy((fh*2),y,"No-one");
y+=fh;

irecon_colour(def_r,def_g,def_b);

Show();

do  {
    FlushKeys();
    k=WaitForKey();
    ctr=0;
    if((k>= IREKEY_1 && k<= IREKEY_9) || k == IREKEY_0)
        ctr=1;
    } while(!ctr);

if(k == IREKEY_0)
    return NULL;

return (party[k-IREKEY_1]);
}

/*
 *      Record that a link has been traversed for this character
 */

void NPC_BeenRead(OBJECT *o,const char *page, const char *reader)
{
NPC_RECORDING *temp;

if(!o)     // Press F1 for Help has no object attached to it
	return;
if(!reader)
	return; // If no 'player' involved

if(!o->user)
	ithe_panic("NPC_BeenRead: Passed invalid object (no usedata)",o->name);

	ilog_quiet("%s talks to %s\n",o->name,reader);

// Empty list

if(o->user->npctalk == NULL)
	{
	temp = (NPC_RECORDING *)M_get(1,sizeof(NPC_RECORDING));
	o->user->npctalk = temp;
	temp->next = NULL;
	temp->page = (char *)M_get(1,strlen(page)+1);
	strcpy(temp->page,page);
	temp->readername = (char *)M_get(1,strlen(reader)+1);
	strcpy(temp->readername,reader);
	return;
	}

// Non-empty list, skip if already present

for(temp=o->user->npctalk;temp->next;temp=temp->next)
	if(!istricmp(temp->page,page) && temp->readername && !istricmp(temp->readername,reader))
		return;

// Add new entry to end of list

temp->next = (NPC_RECORDING *)M_get(1,sizeof(NPC_RECORDING));
temp->next->next = NULL;
temp->next->page = (char *)M_get(1,strlen(page)+1);
strcpy(temp->next->page,page);
temp->next->readername = (char *)M_get(1,strlen(reader)+1);
strcpy(temp->next->readername,reader);
}


/*
 *  Allocate the session log
 */

void NPC_NewSession()
{
// Count the number of pages
NumPages = NPC_CountPages();

// Anything to allocate?
if(!NumPages)
	{
	NpcSession=NULL;
	return;
	}

// Get it
NpcSession = (char **)M_get(NumPages,sizeof(char *));
}


/*
 *  Free the session log
 */

void NPC_EndSession()
{
int ctr;

if(NpcSession)
	{
	for(ctr=0;ctr<NumPages;ctr++)
		if(NpcSession[ctr])
			M_free(NpcSession[ctr]);
	M_free(NpcSession);
	NpcSession=NULL;
	}
}


/*
 *      Record that a page has been read this session
 */

void NPC_AddSession(const char *page)
{
int ctr;

if(!page)
	return; // Ooops

// Look for an existing entry, or the next free slot

for(ctr=0;ctr<NumPages;ctr++)
	if(NpcSession[ctr])
		{
		if(!istricmp(NpcSession[ctr],page))
			return; // Already there
		}
	else
		{
		// Record it
		NpcSession[ctr] = (char *)M_get(1,strlen(page)+1);
		strcpy(NpcSession[ctr],page);
//		ilog_quiet("session: add '%s'\n",page);
		return;
		}

return;
}

/*
 *      Has the page has been read this session?
 */

int NPC_InSession(const char *page)
{
int ctr;

if(!page)
	return 0; // Ooops

// Look for an existing entry, or the next free slot.
// There won't ever be fragmentation, so an empty slot marks the end

for(ctr=0;ctr<NumPages;ctr++)
	if(NpcSession[ctr])
		{
		if(!istricmp(NpcSession[ctr],page))
			return 1; // Read
		}
	else
		return 0; // Not there

return 0; // Not there
}

/*
 *      Erase any links that we've already seen (unless protected)
 */

void NPC_PurgeLinks()
{
int ctr;

ctr=0;
while(ctr<linkptr)
	{
	if(!links[ctr][4][0])  						// Not protected
		if(NPC_InSession(links[ctr][0]))		// On the kill list?
			{
			NPC_EraseLink(ctr);					// Yes, erase it
			// Start again
			ctr=0;
			continue;
			}
	ctr++;
	}
}

/*
 *      Erase a link from the array and move the others along
 */

void NPC_EraseLink(int link)
{
int ctr;

for(ctr=link;ctr<10;ctr++)
	if(ctr+1 < 9)
		{
		// bump them along
		strcpy(links[ctr][0],links[ctr+1][0]);
		strcpy(links[ctr][1],links[ctr+1][1]);
		strcpy(links[ctr][2],links[ctr+1][2]);
		strcpy(links[ctr][3],links[ctr+1][3]);
		links[ctr][4][0]=links[ctr+1][4][0];

		links[ctr+1][0][0]=0; // Erase the copied ones
		links[ctr+1][1][0]=0;
		links[ctr+1][2][0]=0;
		links[ctr+1][3][0]=0;
		links[ctr+1][4][0]=0;
		}
linkptr--;
}


/*
void NPC_lFlag(OBJECT *o,const char *page, OBJECT *p)
{
NPC_RECORDING *temp;

if(!o)     // Press F1 for Help has no object attached to it
	return;
if(!p)
	return; // If no 'player' involved

if(!o->user)
	ithe_panic("NPC_UnRead: Passed invalid object (no usedata)",o->name);

// Empty list

if(o->user->npctalk == NULL)
	{
	temp = (NPC_RECORDING *)M_get(1,sizeof(NPC_RECORDING));
	o->user->npctalk = temp;
	temp->next = NULL;
	temp->page = (char *)M_get(1,strlen(page)+1);
	temp->player = p;
	strcpy(temp->page,page);
	return;
	}

// Non-empty list, skip if already present

for(temp=o->user->npctalk;temp->next;temp=temp->next)
	if(!istricmp(temp->page,page) && temp->readername && !istricmp(temp->readername,reader))
		return;

// Look for an empty slot and use that if possible

for(temp=o->user->npctalk;temp->next;temp=temp->next)
	if(temp->page[0]=='\0' && temp->player == NULL)
		{
		M_free(temp->page);
		temp->page = (char *)M_get(1,strlen(page)+1);
		temp->player = p;
		return;
		}

// Add new entry to end of list

temp->next = (NPC_RECORDING *)M_get(1,sizeof(NPC_RECORDING));
temp->next->next = NULL;
temp->next->page = (char *)M_get(1,strlen(page)+1);
temp->next->player = p;
strcpy(temp->next->page,page);
}
*/

/*
 *      Found out if a link has been traversed for this character
 */

int NPC_WasRead(OBJECT *o,const char *page, const char *reader)
{
NPC_RECORDING *temp;

if(!o)     // Press F1 for Help has no object attached to it
	return 0;
if(!reader)
	return 0; // If no 'player' involved

for(temp=o->user->npctalk;temp;temp=temp->next)
	if(!istricmp(temp->page,page) && temp->readername && !istricmp(temp->readername,reader))
		return(1);
return 0;
}

/*
 *      Add or delete an lFlag
 */

void NPC_set_lFlag(OBJECT *o,const char *page, const char *reader, int state)
{
NPC_RECORDING *temp;

if(!o)     // Press F1 for Help has no object attached to it
	{
//	ilog_quiet("set_lflag bailed on O\n");
	return;
	}
if(!reader)
	{
//	ilog_quiet("set_lflag bailed on P\n");
	return; // If no 'player' involved
	}

if(!o->user)
	ithe_panic("NPC_set_lFlag: Passed invalid object (no usedata)",o->name);

// Empty list

if(o->user->lFlags == NULL)
	{
	if(!state)
		{
		return; // Not there, don't bother to delete it
		}
	temp = (NPC_RECORDING *)M_get(1,sizeof(NPC_RECORDING));
	o->user->lFlags = temp;
	temp->next = NULL;

	temp->page = (char *)M_get(1,strlen(page)+1);
	strcpy(temp->page,page);

	temp->readername = (char *)M_get(1,strlen(reader)+1);
	strcpy(temp->readername,reader);
	return;
	}

// Non-empty list, skip if already present

for(temp=o->user->lFlags;temp;temp=temp->next)
	if(!istricmp(temp->page,page) && temp->readername && !istricmp(temp->readername,reader))
		{
		if(state)
			return;	// Already there, don't add
		// Mark unused
		temp->page[0]='\0';
		M_free(temp->readername);
		temp->readername=NULL;
		return;
		}

if(!state)
	return; // It wasn't there, don't remove it

// Look for an empty slot and use that if possible

for(temp=o->user->lFlags;temp;temp=temp->next)
	if(temp->page[0]=='\0' && temp->readername == NULL)
		{
		M_free(temp->page);
		temp->page = (char *)M_get(1,strlen(page)+1);
		strcpy(temp->page,page);

		temp->readername = (char *)M_get(1,strlen(reader)+1);
		strcpy(temp->readername,reader);
		return;
		}

// Ok, add new entry to end of list

for(temp=o->user->lFlags;temp->next;temp=temp->next);

temp->next = (NPC_RECORDING *)M_get(1,sizeof(NPC_RECORDING));
temp->next->next = NULL;
temp->next->page = (char *)M_get(1,strlen(page)+1);
strcpy(temp->next->page,page);

temp->next->readername = (char *)M_get(1,strlen(reader)+1);
strcpy(temp->next->readername,reader);
  

}

/*
 *      Found out if an lFlag has been set for this character
 */

int NPC_get_lFlag(OBJECT *o,const char *page, const char *reader)
{
NPC_RECORDING *temp;

if(!o)     // Press F1 for Help has no object attached to it
	return 0;
if(!reader)
	return 0; // If no 'player' involved

for(temp=o->user->lFlags;temp;temp=temp->next)
	{
	if(!istricmp(temp->page,page) && temp->readername && !istricmp(temp->readername,reader))
		{
		return(1);
		}
	}

return 0;
}

/*
 *      Delete all list entries
 */

void NPC_ReadWipe(OBJECT *o, int wflags)
{
NPC_RECORDING *temp,*old;

if(!o)     // Press F1 for Help has no object attached to it
    return;

// First the Npc talk

old=o->user->npctalk;
for(temp=old;temp;)
	if(temp)
		{
		temp=temp->next;
		old->next=NULL; // break the list in case of error
		if(old->page)
			M_free(old->page);
		if(old->readername)
			M_free(old->readername);
		M_free(old);
		old=temp;
		}
o->user->npctalk=NULL;

// Then the lFlags

if(wflags)
	{
	old=o->user->lFlags;
	for(temp=old;temp;)
		if(temp)
			{
			temp=temp->next;
			old->next=NULL; // break the list in case of error
			if(old->page)
				M_free(old->page);
			if(old->readername)
				M_free(old->readername);
			M_free(old);
			old=temp;
			}
	o->user->lFlags=NULL;
	}
}



/*
 *  Scroll a still picture (>640x480)
 */

void NPC_ScrollPic(const char *file, int dx, int dy, int wait1, int wait2, int scroll_delay)
{
char filename[1024];
int stop,w,h,cx,cy,mw,mh,delayctr;
IREBITMAP *big;

// Try to load in image
if(!loadfile(file,filename))
	{
	Bug("ScrollPicture: Image file '%s' not found",file);
	return;
	}
big=iload_bitmap(filename);
w=big->GetW();
h=big->GetH();
mw=w-640;
mh=h-480;

// Save the screen, and black it
SaveScreen();
swapscreen->Clear(ire_black);

// Figure out where to start from

if(dx>=0)
	cx=0; // Start from left side
else
	cx=mw; // Start from right side

if(dy>=0)
	cy=0; // Start from top
else
	cy=mh; // Start from bottom

// Wait for a bit before scrolling

swapscreen->Get(big,cx,cy);	// Copy part of the image into the swapscreen
Show();
waitfor(wait1*100);


// Do the deed

delayctr=scroll_delay;

stop=0;
while(!stop)
	{
	if(UpdateAnim())
		{
		if(delayctr<1)
			{
			delayctr=scroll_delay;
			// Add vectors
			cx+=dx;
			cy+=dy;
			// Clamp coordinates
			if(cx>mw)
				{
				cx=mw;
				stop=1;
				}
			if(cx<0)
				{
				cx=0;
				stop=1;
				}
			if(cy>mh)
				{
				cy=mh;
				stop=1;
				}
			if(cy<0)
				{
				cy=0;
				stop=1;
				}
			}
		else
			delayctr--;
		}
	// Show it
	swapscreen->Get(big,cx,cy);
	Show();

	irekey = GetKey();
	if(irekey == IREKEY_ESC)
		stop=1;
	}

waitfor(wait2*100);

delete big;

// Restore screen
RestoreScreen();

}

//
//	Create a unique NPC name
//

char *NPC_MakeName(OBJECT *o)
{
char *ret;
const char *type;
const char *name;
int len;

if(!o)
	return NULL;

type = o->name;
if(!type || !type[0])
	type="NULL";

name = o->personalname;
if(!name || !name[0])
	name="NULL";

len=strlen(type)+strlen(name)+2;	// 2 = NUL and '.'
ret = (char *)M_get(1,len);

strcpy(ret,type);
strcat(ret,".");
strcat(ret,name);

return ret;
}



void InitConvSprites()
{
char filename[1024];

if(!leftarrow)
	{
	if(loadfile("left.cel",filename))
		leftarrow = iload_bitmap(filename);
	}

if(!rightarrow)
	{
	if(loadfile("right.cel",filename))
		rightarrow = iload_bitmap(filename);
	}

if(!ticksprite)
	{
	if(loadfile("tick.cel",filename))
		ticksprite = iload_bitmap(filename);
	}

}
