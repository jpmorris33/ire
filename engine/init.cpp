//
//      IRE init systems
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// MR: added
#ifndef _WIN32
#include <unistd.h>
#endif

// defines

#define IRE_SYSTEM_MODULE
#define ALREADY_KNOW_ITG
#define UNDEFINED -1
#define QRAND_MAX 32768

// includes

#include "ithelib.h"
#include "media.hpp"
#include "console.hpp"
#include "init.hpp"
#include "loadfile.hpp"
#include "gamedata.hpp"
#include "resource.hpp"
#include "oscli.hpp"
#include "database.hpp"
//#include "vrm_in.hpp"


// Global variables

//SQ **SQlist;    long SQtot;     // Sequences in the SQlist, no. of sequences
//CH *CHlist;     long CHtot;     // Array of characters, no. of characters
//SP *SPlist;     long SPtot;     // Array of sprites, number of sprites

PEM *PElist;                    // PissEasy Script table
S_POOL *s_pool;                 // Pool of sprites loaded into memory
SEQ_POOL *seq_pool;             // Pool of animation sequences

typedef struct search_st
	{
	char *name;
	int num;
	} search_st;

static search_st *sprites;
static search_st *sequences;
static search_st *scripts;
static search_st *characters;
static search_st *tables;

unsigned char digit[256][8];
char stemp[128];

/*
unsigned short font_bground=0,font_fground=0x7fff;
char *rootname="main\0";
*/
extern char mapname[];
//extern char big_font;

// Local variables

static int qrt[QRAND_MAX];

// functions

short FindProc(char *name);
extern void register_exporttable();
extern int sizeof_sprite(IREBITMAP *bmp);
extern IRELIGHTMAP *load_lightmap(char *filename);
static int CMP_sort(const void *a,const void *b);
static int CMP_search(const void *a,const void *b);
static int CMP_search_DTI(const void *a,const void *b);
static int CMP_search_DTS(const void *a,const void *b);
static int GetAvg(IREBITMAP *bmp);
static IRESPRITE *LoadCachedImage(char *fname, VMINT *avcol);

// code

void CheckCacheTable()
{
int len,error;
char *data;
data = GetSQLSingle("SELECT Backend FROM CacheVersion;",&len,&error);
if(!data)
	return; // Not yet set up, by the looks

if(stricmp(data,IRE_Backend()))	{
	ire_recache=1;	// Oh dear, wrong backend.  Rebuilt it all
	ilog_printf("Warning: Cache data is for %s, not %s.  Rebuilding\n",data,IRE_Backend());
	printf("Warning: Cache data is for %s, not %s.  Rebuilding\n",data,IRE_Backend());
	}
FreeSQLString(data);
}

void CreateCacheTable()
{
int len,error;
char query[1024];
	
if(ire_recache)	{
	// Wipe the cache
	snprintf(query,1023,"DROP TABLE \"%s\";",imgcachedir);
	query[1023]=0;
	RunSQL(query);
	RunSQL("DELETE FROM CacheVersion;");
	RunSQL("VACUUM;");
	}
	
snprintf(query,1023,"CREATE TABLE \"%s\" (filename text PRIMARY KEY, image blob, timestamp text, avgcol text);",imgcachedir);
query[1023]=0;
RunSQL(query);

RunSQL("CREATE TABLE CacheVersion (Backend text);");

char *data;
data = GetSQLSingle("SELECT Backend FROM CacheVersion;",&len,&error);
if(data)
	FreeSQLString(data);
else	{
	snprintf(query,1023,"INSERT INTO CacheVersion (Backend) VALUES (\"%s\");",IRE_Backend());
	query[1023]=0;
	RunSQL(query);
	}

}

/*
 *      LoadImage: high-level image loader
 */

IRESPRITE *LoadCachedImage(char *fname, VMINT *avcol)
{
char filename[1024];
char cachename[1024];
char query[2048];
IREBITMAP *img;
time_t ftime,itime;
struct stat sb;
int avg,err=0,len;
IRESPRITE *dest;
SQLROWSET *record=NULL;
void *imgbuffer;
const void *cimgbuffer;
const char *cptr;
	
makefilename(fname,filename);
snprintf(query,2047,"SELECT image, timestamp, avgcol FROM \"%s\" WHERE filename = \"%s\";",imgcachedir,fname);
query[2047]=0;
	
record = GetSQL(query,&err);
if(record)
	if(record->items < 1)	{
		delete record;
		record=NULL;
	}
	
if(!record)	{
	// It's not in the cache, try and insert it
	img = iload_bitmap(filename);
	if(!img)
		ithe_panic("Could not load image: (file not found)",fname);
	ftime=igettime(filename);
	
	dest = MakeIRESPRITE(img);
	if(!dest)
		ithe_panic("Could not load image: unsupported format?",filename);
	avg=GetAvg(img);
	delete img;
	
	dest->SetAverageCol(avg);
	if(avcol)
		*avcol=avg;
	// Write cache entry
	///ave_rle_sprite(dest,cachename,avg);
	imgbuffer=dest->SaveBuffer(&len);
	//printf("Savebuffer = %p, len = %d\n",imgbuffer,len);
	
	snprintf(query,2047,"INSERT INTO \"%s\" (filename, image, timestamp, avgcol) VALUES (\"%s\",?,%ld,%d);",imgcachedir,fname,(long)ftime,avg);
	InsertSQLBinary(query,(const char *)imgbuffer,len);
	dest->FreeSaveBuffer(imgbuffer);
	return dest;
}

// Okay, it is in the database

/*
printf("record items = %d\n",record->items);
for(int ctr=0;ctr<record->items;ctr++)	{
	printf("item %d = %p\n",ctr,record->GetRecord(ctr,NULL,NULL));
	}
*/	

cimgbuffer = record->GetRecord(0,&len,NULL);
if(!cimgbuffer)	{
	delete record;
	sprintf(query,"DROP TABLE \"%s\";",imgcachedir);
	RunSQL(query);
	TermSQL();
	ithe_panic("Image cache corrupted, please restart the game",NULL);
}
	
cptr = (const char *)record->GetRecord(1,NULL,NULL);
itime=0;
if(cptr)
	itime = atoi(cptr);
cptr = (const char *)record->GetRecord(2,NULL,NULL);
avg=0;
if(cptr)
	avg = atoi(cptr);

if(ire_checkcache)	{
	ftime=igettime(filename);
	//printf("Cache: file = %ld, cache = %ld, diff = %lf\n",ftime,itime,difftime(itime,ftime));
	if(difftime(itime,ftime) != 0.0)	{
		delete record;
		// The cache is stale, delete this entry, and rerun to force a rebuild
		printf("Cache: delete stale entry '%s'\n",fname);
		snprintf(query,2047,"DELETE FROM \"%s\" WHERE filename=\"%s\";",imgcachedir,fname);
		len=RunSQL(query);
		if(len != SQLITE_OK)
			printf("delete (%s) failed with %d\n",query,len);
		return LoadCachedImage(fname,avcol); // Recreate the cache entry
		}
}

dest = MakeIRESPRITE(cimgbuffer,len);
if(!dest)
    ithe_panic("Cached image damaged or wrong format: try running with -recache option",filename);
if(avcol)
	*avcol=avg;

// Close the database rowset
delete record;

return dest;
}



/*
 *      Init_Sprite() - Load in a single sprite, called by SCRIPT.CPP
 */

long Init_Sprite(int number)
{
VMINT avcol=0;
// Call the generic image loader to get the image

SPlist[number].image=LoadCachedImage(SPlist[number].fname, &avcol);
SPlist[number].thumbcol = new IRECOLOUR(avcol);

// Now store the width and height of the sprite in the pool

SPlist[number].w=SPlist[number].image->GetW();
SPlist[number].h=SPlist[number].image->GetH();

// This is the width times the height, for fast access later

SPlist[number].wxh=SPlist[number].w*SPlist[number].h;

Plot(0);

/*
Return image size in bytes
(this depends on the implementation of the sprite structure, so if it
 breaks later on if the API changes, just return width*height, or 1..)
*/
return SPlist[number].wxh*4;
}


/*
 *      Init_RoofTile() - Load in a single sprite, called by SCRIPT.CC
 */

long Init_RoofTile(int number)
{

// Call the generic image loader to get the image
RTlist[number].image=LoadCachedImage(RTlist[number].fname, NULL);

// Now store the width and height of the sprite in the pool
RTlist[number].w=RTlist[number].image->GetW();
RTlist[number].h=RTlist[number].image->GetH();

// This is the width times the height, for fast access later
RTlist[number].wxh=RTlist[number].w*RTlist[number].h;

// Write a dot for this image.  This Plot was set up in script.cc
Plot(0);

/*
Return image size in bytes
(this depends on the implementation of the sprite structure, so if it
 breaks later on if the API changes, just return width*height, or 1..)
*/
return RTlist[number].wxh;
}


/*
 *      Init_LightMap() - Load in a single sprite, called by SCRIPT.CC
 */

long Init_LightMap(int number)
{
char filename[1024];

if(!loadfile(LTlist[number].fname,filename))
	ithe_panic("Could not load lightmap image: (file not found)",LTlist[number].fname);

LTlist[number].image = load_lightmap(filename);

if(!LTlist[number].image)
	ithe_panic("Could not load image: unsupported format?",filename);

if(LTlist[number].image->GetW() != 32 || LTlist[number].image->GetH() != 32)
	{
	ilog_quiet("%s: dimensions are %dx%d\n",filename,LTlist[number].image->GetW(),LTlist[number].image->GetH());
	ithe_panic("Lightmap file must be 32x32",filename);
	}

Plot(0);

return 1;
}

/*
 *    find_spr()  -  find the address of a sprite by its name
 */

S_POOL *find_spr(const char *name)
{
int cx;
for(cx=0;cx<SPtot;cx++)
	if(!istricmp(SPlist[cx].name,name))
		return(&SPlist[cx]);
return NULL;
}


/*
 *    Submit a function, register it with the game if it is a System Function
 */

void Init_PE(int pos)
{
if(!istricmp(PElist[pos].name,"status"))
	Sysfunc_status = pos;
if(!istricmp(PElist[pos].name,"erase"))
	Sysfunc_erase = pos;
if(!istricmp(PElist[pos].name,"splash"))
	Sysfunc_splash = pos;
if(!istricmp(PElist[pos].name,"scheduler"))
	Sysfunc_scheduler = pos;
if(!istricmp(PElist[pos].name,"update_life"))
	Sysfunc_updatelife = pos;
if(!istricmp(PElist[pos].name,"trackstop"))
	Sysfunc_trackstop = pos;
if(!istricmp(PElist[pos].name,"wakeup"))
	Sysfunc_wakeup = pos;
if(!istricmp(PElist[pos].name,"update_robot"))
	Sysfunc_updaterobot = pos;
}


/*
 *    Init_Funcs()  -  Assign each function in the characters to a VRM
 *						Calls InitFuncsFor(object)
 */

void Init_Funcs()
{
int ctr;

// Critical VRMs

if(Sysfunc_trackstop == UNDEFINED)
	ithe_panic("Could not find system function 'trackstop' in the VRM list!",NULL);

if(Sysfunc_updatelife == UNDEFINED)
	ithe_panic("Could not find system function 'update_life'!",NULL);

if(Sysfunc_erase == UNDEFINED)
	ithe_panic("Could not find system function 'erase'!",NULL);

if(Sysfunc_wakeup == UNDEFINED)
	ithe_panic("Could not find system function 'wakeup'!",NULL);

// Non-critical VRMs

if(Sysfunc_scheduler == UNDEFINED)
	ilog_quiet("Did not find system function 'scheduler', continuing anyway\n");
if(Sysfunc_status == UNDEFINED)
	ilog_quiet("Did not find system function 'status', continuing anyway\n");
if(Sysfunc_follower == UNDEFINED)
	ilog_quiet("Did not find system function 'follower', continuing anyway\n");
if(Sysfunc_splash == UNDEFINED)
	ilog_quiet("Did not find system function 'splash', continuing anyway\n");
if(Sysfunc_updaterobot == UNDEFINED)
	ilog_quiet("Did not find system function 'update_robot', continuing anyway\n");

for(ctr=0;ctr<CHtot;ctr++)
	InitFuncsFor(&CHlist[ctr],0);
}

/*
 *    InitFuncsFor()  -  Assign each function in a single character to a VRM
 *						 Called by Init_Funcs and somewhere in loadsave.cpp
 */

void InitFuncsFor(OBJECT *o, int rebuild)
{
char *str;
int tmp,objtype=0;

if(rebuild)
	objtype=getnum4char(o->name);

    // Find the given USE function in the VRM list and put it in the Ucache
    str = o->funcs->use;
    if(str[0])
        {
        tmp = getnum4PE(str);
        if(tmp == UNDEFINED)
            {
            Bug("use: Did not find function '%s'.  Ignoring\n",str);
            o->funcs->ucache = -1;
			if(rebuild)
				{
				o->funcs->ucache = CHlist[objtype].funcs->ucache;
				strcpy(o->funcs->use,CHlist[objtype].funcs->use);
				}
            }
        else
            o->funcs->ucache = tmp;
        }
    else
		{
		o->funcs->ucache = -1;
		}

    // Find a STAND function in the VRM list and put it in the Scache
    // Switch off the trigger bit if none found

	str = o->funcs->stand;
	if(str[0])
		{
		tmp = getnum4PE(str);
		if(tmp == UNDEFINED)
			{
			Bug("stand: Did not find function '%s'.  Ignoring\n",str);
			o->funcs->scache = -1;
			if(rebuild)
				{
				o->funcs->scache = CHlist[objtype].funcs->scache;
				strcpy(o->funcs->stand,CHlist[objtype].funcs->stand);
				}
			else
				o->flags &= ~IS_TRIGGER;
			}
		else
			o->funcs->scache = tmp;
		}
	else
		o->flags &= ~IS_TRIGGER;

    // Find the given LOOK function in the VRM list and put it in the Lcache

    str = o->funcs->look;
    if(str[0])
        {
        tmp = getnum4PE(str);
        if(tmp==UNDEFINED)
            {
            Bug("look: Did not find function '%s'.  Ignoring\n",str);
            o->funcs->lcache = -1;
			if(rebuild)
				{
				o->funcs->lcache = CHlist[objtype].funcs->lcache;
				strcpy(o->funcs->look,CHlist[objtype].funcs->look);
				}
            }
        else
            o->funcs->lcache = tmp;
        }
    else
        o->funcs->lcache = -1;

    // Find the given KILLED function in the VRM list and put it in the Kcache
    // If none, the engine will just vanish the dead thing

    str = o->funcs->kill;
    if(str[0])
        {
        tmp = getnum4PE(str);
        if(tmp == UNDEFINED)
            {
            Bug("kill: Did not find function '%s'.  Ignoring\n",str);
            o->funcs->kcache = -1;
			if(rebuild)
				{
				o->funcs->kcache = CHlist[objtype].funcs->kcache;
				strcpy(o->funcs->kill,CHlist[objtype].funcs->kill);
				}
            }
        else
            o->funcs->kcache = tmp;
        }
    else
        o->funcs->kcache = -1;

    // Find the given HURT function in the VRM list and put it in the Hcache
    str = o->funcs->hurt;
    if(str[0])
        {
        tmp = getnum4PE(str);
        if(tmp == UNDEFINED)
            {
            Bug("hurt: Did not find function '%s'.  Ignoring\n",str);
            o->funcs->hcache = -1;
			if(rebuild)
				{
				o->funcs->hcache = CHlist[objtype].funcs->hcache;
				strcpy(o->funcs->hurt,CHlist[objtype].funcs->hurt);
				}
            }
        else
            o->funcs->hcache = tmp;
        }
    else
        o->funcs->hcache = -1;

    // Find the given WIELD function in the VRM list and put it in the Wcache
    str = o->funcs->wield;
    if(str[0])
        {
        tmp = getnum4PE(str);
        if(tmp == UNDEFINED)
		{
		Bug("wield: Did not find function '%s'.  Ignoring\n",str);
		o->funcs->wcache = -1;
		if(rebuild)
			{
			o->funcs->wcache = CHlist[objtype].funcs->wcache;
			strcpy(o->funcs->wield,CHlist[objtype].funcs->wield);
			}
		}
        else
		o->funcs->wcache = tmp;
        }
    else
        o->funcs->wcache = -1;

    // Find the given INIT function in the VRM list and put it in the Icache
    str = o->funcs->init;
    if(str[0])
        {
        tmp = getnum4PE(str);
        if(tmp == UNDEFINED)
            {
            Bug("init: Did not find function '%s'.  Ignoring\n",str);
            o->funcs->icache = -1;
			if(rebuild)
				{
				o->funcs->icache = CHlist[objtype].funcs->icache;
				strcpy(o->funcs->init,CHlist[objtype].funcs->init);
				}
            }
        else
            o->funcs->icache = tmp;
        }
    else
        o->funcs->icache = -1;

    // Find an ATTACK function in the VRM list and put it in the Acache

	str = o->funcs->attack;
	if(str[0])
		{
		tmp = getnum4PE(str);
		if(tmp == UNDEFINED)
			{
			Bug("attack: Did not find function '%s'.  Ignoring\n",str);
			o->funcs->acache = -1;
			if(rebuild)
				{
				o->funcs->acache = CHlist[objtype].funcs->acache;
				strcpy(o->funcs->attack,CHlist[objtype].funcs->attack);
				}
			}
		else
			o->funcs->acache = tmp;
		}
	else
		o->funcs->acache = -1;

    // Find the given HORROR function in the VRM list and put it in the HRcache
    str = o->funcs->horror;
    if(str[0])
        {
        tmp = getnum4PE(str);
        if(tmp == UNDEFINED)
            {
            Bug("horror: Did not find function '%s'.  Ignoring\n",str);
            o->funcs->hrcache = -1;
			if(rebuild)
				{
				o->funcs->hrcache = CHlist[objtype].funcs->hrcache;
				strcpy(o->funcs->horror,CHlist[objtype].funcs->horror);
				}
            }
        else
            o->funcs->hrcache = tmp;
        }
    else
        o->funcs->hrcache = -1;

    // Find the given Quantity Change function in the VRM list and put it in the HRcache

    str = o->funcs->quantity;
    if(str[0])
        {
        tmp = getnum4PE(str);
        if(tmp==UNDEFINED)
            {
            Bug("look: Did not find function '%s'.  Ignoring\n",str);
            o->funcs->qcache = -1;
			if(rebuild)
				{
				o->funcs->qcache = CHlist[objtype].funcs->qcache;
				strcpy(o->funcs->quantity,CHlist[objtype].funcs->quantity);
				}
            }
        else
            o->funcs->qcache = tmp;
        }
    else
        o->funcs->qcache = -1;

    // Find the given IfGet function in the VRM list and put it in the HRcache

    str = o->funcs->get;
    if(str[0])
        {
        tmp = getnum4PE(str);
        if(tmp==UNDEFINED)
            {
            Bug("look: Did not find function '%s'.  Ignoring\n",str);
            o->funcs->gcache = -1;
		if(rebuild)
			{
			o->funcs->gcache = CHlist[objtype].funcs->gcache;
			strcpy(o->funcs->get,CHlist[objtype].funcs->get);
			}
            }
        else
            o->funcs->gcache = tmp;
        }
    else
        o->funcs->gcache = -1;


}


/*
 *		calculate the size of a large object based on ideal values
 */

void Init_Areas(OBJECT *objsel)
{
objsel->flags &= ~IS_LARGE;  // Assume it's only 1 square

// First the vertical

objsel->w = SQlist[objsel->dir[CHAR_D]].seq[0]->w;
objsel->h = SQlist[objsel->dir[CHAR_D]].seq[0]->h;

// Set up the size in tiles
objsel->mw = objsel->w>>5;
if(objsel->w & 0x1f || !objsel->mw)
	objsel->mw++;

objsel->mh = objsel->h>>5;
if(objsel->h & 0x1f || !objsel->mh)
	objsel->mh++;

if(objsel->mh>1 || objsel->mw>1)        // If it's bigger than 1 square..
	objsel->flags |= IS_LARGE;

// Set up verticals

if(objsel->varea[BLK_W] == 0)
	{
	objsel->varea[BLK_W]=objsel->mw;
	objsel->varea[BLK_X]=0;
	}

if(objsel->varea[BLK_H] == 0)
	{
	objsel->varea[BLK_H]=objsel->mh;
	objsel->varea[BLK_Y]=0;
	}

if(objsel->vblock[BLK_W] == 0)
	{
	objsel->vblock[BLK_W]=objsel->mw;
	objsel->vblock[BLK_X]=0;
	}

if(objsel->vblock[BLK_H] == 0)
	{
	objsel->vblock[BLK_H]=objsel->mh;
	objsel->vblock[BLK_Y]=0;
	}

// Then the horizontal

objsel->w = SQlist[objsel->dir[CHAR_R]].seq[0]->w;
objsel->h = SQlist[objsel->dir[CHAR_R]].seq[0]->h;

// Set up the size in tiles
objsel->mw = objsel->w>>5;
if(objsel->w & 0x1f || !objsel->mw)
     objsel->mw++;

objsel->mh = objsel->h>>5;
if(objsel->h & 0x1f || !objsel->mh)
     objsel->mh++;

// Just to make sure
if(objsel->mh>1 || objsel->mw>1)        // If it's bigger than 1 square..
	objsel->flags |= IS_LARGE;

// Set up horizontals

if(objsel->harea[BLK_W] == 0)
	{
	objsel->harea[BLK_W]=objsel->mw;
	objsel->harea[BLK_X]=0;
	}

if(objsel->harea[BLK_H] == 0)
	{
	objsel->harea[BLK_H]=objsel->mh;
	objsel->harea[BLK_Y]=0;
	}

if(objsel->hblock[BLK_W] == 0)
	{
	objsel->hblock[BLK_W]=objsel->mw;
	objsel->hblock[BLK_X]=0;
	}

if(objsel->hblock[BLK_H] == 0)
	{
	objsel->hblock[BLK_H]=objsel->mh;
	objsel->hblock[BLK_Y]=0;
	}
}

/*
 *	recalculate current size of a large object from current direction
 */

void CalcSize(OBJECT *objsel)
{
if(!(objsel->flags & IS_LARGE)) // Don't bother for small object
	return;

objsel->w = SQlist[objsel->dir[objsel->curdir]].seq[0]->w;
objsel->h = SQlist[objsel->dir[objsel->curdir]].seq[0]->h;

// Set up the size in tiles
objsel->mw = objsel->w>>5;
if(objsel->w & 0x1f || !objsel->mw)
	objsel->mw++;

objsel->mh = objsel->h>>5;
if(objsel->h & 0x1f || !objsel->mh)
	objsel->mh++;
}


/*
 *      Getnum4char - Find the index of the character in the CHlist array
 */

int getnum4char_slow(const char *name)
{
int ctr;
if(!name)
    return -1;
for(ctr=0;ctr<CHtot;ctr++)
	{
	// We use strfirst here in case it's an Alias... those still have
	// junk after the name which we can't get rid of just yet
	if(!istricmp(name,strfirst(CHlist[ctr].name)))
		return ctr;
	}
return -1;
}


/*
 *      Getnum4PE - Find the index of the PE script in the PElist array
 */

int getnum4PE_slow(const char *name)
{
int ctr;

if(editarea[0])  // Are we the Area Editor?
	return 0;

if(PEtot <1)
	ithe_panic("getnum4PE called before any scripts defined",NULL);

// Fast search first

for(ctr=0;ctr<PEtot;ctr++)
	if(!PElist[ctr].hidden)
		if(!istricmp(name,PElist[ctr].name))
			return ctr;

// Not found.  Is it a local function?

for(ctr=0;ctr<PEtot;ctr++)
	if(!istricmp(name,PElist[ctr].name))
		if(PElist[ctr].hidden)
			{
			Bug("PE script '%s' is local.. can't call it directly\n",name);
			return -1;
			}

// It simply doesn't exist..

Bug("PE script '%s' not found\n",name);
return -1;
}


/*
 *      Getnum4sprite - Find the index of the sequence in the SPlist array
 */

int getnum4sprite_slow(const char *name)
{
int ctr;
for(ctr=0;ctr<SPtot;ctr++)
	if(!istricmp(name,SPlist[ctr].name))
		return ctr;
return -1;
}

/*
 *      Getnum4sequence - Find the index of the sequence in the SQlist array
 */

int getnum4sequence_slow(const char *name)
{
int ctr;

for(ctr=0;ctr<SQtot;ctr++)
	{
	if(!istricmp(name,SQlist[ctr].name))
		return ctr;
	}

return -1;

}

/*
 *      Getnum4table - Find the index of the data table in the DTlist array
 */

int getnum4table_slow(const char *name)
{
int ctr;

for(ctr=0;ctr<DTtot;ctr++)
	if(!istricmp(name,DTlist[ctr].name))
		return ctr;

return -1;
}

/*
 *      Getname4string - Work out the name of a string, if any
 */

char *getname4string(const char *data)
{
int ctr;

for(ctr=0;ctr<DTtot;ctr++)
	if(STlist[ctr].data == data)
		return STlist[ctr].name;

return NULL;
}

int getnum4string(const char *data)
{
int ctr;

for(ctr=0;ctr<DTtot;ctr++)
	if(STlist[ctr].data == data)
		return ctr;

return -1;
}



static int CMP_sort(const void *a,const void *b)
{
return istricmp(((search_st *)a)->name,((search_st *)b)->name);
}

static int CMP_search(const void *a,const void *b)
{
return istricmp((char *)a,((search_st *)b)->name);
}

static int CMP_search_DTI(const void *a,const void *b)
{
return (int)((VMINT)a-((DT_ITEM*)b)->ki);
}

static int CMP_search_DTS(const void *a,const void *b)
{
return istricmp((char *)a,((DT_ITEM*)b)->ks);
}

static int CMP_search_ST(const void *a,const void *b)
{
return istricmp((char *)a,((ST_ITEM*)b)->name);
}

//
//	Build ordered lists for quick searching
//

void Init_Lookups()
{
int ctr;

ilog_quiet("Init lookups\n");

if(SPtot < 1)
	ithe_panic("No sprites","Init_Lookups");
if(SQtot < 1)
	ithe_panic("No sequences","Init_Lookups");
if(CHtot < 1)
	ithe_panic("No characters","Init_Lookups");
//if(PEtot < 1)
//	ithe_panic("No scripts","Init_Lookups");

// Sprites

sprites = (search_st *)M_get(SPtot+1,sizeof(search_st));
for(ctr=0;ctr<SPtot;ctr++)
	{
	sprites[ctr].name=SPlist[ctr].name;
	sprites[ctr].num=ctr;
	}
qsort(sprites,SPtot,sizeof(search_st),CMP_sort);


// Sequences

sequences = (search_st *)M_get(SQtot+1,sizeof(search_st));
for(ctr=0;ctr<SQtot;ctr++)
	{
	sequences[ctr].name=SQlist[ctr].name;
	sequences[ctr].num=ctr;
	}
qsort(sequences,SQtot,sizeof(search_st),CMP_sort);

// Characters

characters = (search_st *)M_get(CHtot+1,sizeof(search_st));
for(ctr=0;ctr<CHtot;ctr++)
	{
	characters[ctr].name=CHlist[ctr].name;
	characters[ctr].num=ctr;
	}
qsort(characters,CHtot,sizeof(search_st),CMP_sort);

// Scripts (not used for book viewer)

if(PEtot > 0)
	{
	scripts = (search_st *)M_get(PEtot+1,sizeof(search_st));
	for(ctr=0;ctr<PEtot;ctr++)
		{
		scripts[ctr].name=PElist[ctr].name;
		scripts[ctr].num=ctr;
		}
	qsort(scripts,PEtot,sizeof(search_st),CMP_sort);
	}
else
	scripts=NULL;

// Tables (optional)

if(DTtot < 1)
	tables=NULL;
else
	{
	tables = (search_st *)M_get(DTtot+1,sizeof(search_st));
	for(ctr=0;ctr<DTtot;ctr++)
		{
		tables[ctr].name=DTlist[ctr].name;
		tables[ctr].num=ctr;
		}
	qsort(tables,DTtot,sizeof(search_st),CMP_sort);
	}

}

//
//	Search for a string
//

#define FIND(key,list,num) (search_st *)bsearch(key,list,num,sizeof(search_st),CMP_search)

/*
 *      Getnum4char - Find the index of the character in the CHlist array
 */

int getnum4char(const char *name)
{
search_st *p;

p = FIND(name,characters,CHtot);
if(p)
	return p->num;
return -1;
}

/*
 *      Getnum4tile- Find the index of the tile in the TIlist array
 */

int getnum4tile(const char *name)
{
int ctr;

for(ctr=0;ctr<TItot;ctr++)
	if(!istricmp(name,TIlist[ctr].name))
		return ctr;
//Bug("Tile '%s' not found\n",name);
return -1;
}

/*
 *      Getnum4tilelink- Find the index of the tilelink in the TLlist array
 */

int getnum4tilelink(const char *name)
{
int ctr;

for(ctr=0;ctr<TLtot;ctr++)
	{
	if(!istricmp(name,TLlist[ctr].name))
		return ctr;
	}
//Bug("Tile '%s' not found\n",name);
return -1;
}


int getnum4rooftop(const char *name)
{
register int ctr;
S_POOL *ptr;
if(!name)
	return 0;

ptr=RTlist;
for(ctr=1;ctr<RTtot;ctr++)
	{
	ptr++; // Skip [0] as it's reserved
	if(*name == ptr->fname[0])	// Check first char
		if(!istricmp(name,ptr->fname))
			return ctr;
	}
	  
return 0;
}


int getnum4light(const char *name)
{
register int ctr;
L_POOL *ptr;
if(!name)
	return 0;

ptr=LTlist;
for(ctr=1;ctr<LTtot;ctr++)
	{
	ptr++; // Skip [0] as it's reserved
	if(*name == ptr->fname[0])	// Check first char
		if(!istricmp(name,ptr->fname))
			return ctr;
	}
	  
return 0;
}


/*
 *      Getnum4PE - Find the index of the PE script in the PElist array
 */

int getnum4PE(const char *name)
{
search_st *p;

if(editarea[0])  // Are we the Area Editor?
	return 0;

if(!scripts)
	return -1;

p = FIND(name,scripts,PEtot);
if(p)
	{
	if(PElist[p->num].hidden)
		{
		Bug("PE script '%s' is local.. can't call it directly\n",name);
		return -1;
		}
	return p->num;
	}
return -1;
}


/*
 *      Getnum4sprite - Find the index of the sequence in the SPlist array
 */

int getnum4sprite(const char *name)
{
search_st *p;

p = FIND(name,sprites,SPtot);
if(p)
	return p->num;
return -1;
}

/*
 *      Getnum4sequence - Find the index of the sequence in the SQlist array
 */

int getnum4sequence(const char *name)
{
search_st *p;

p = FIND(name,sequences,SQtot);
if(p)
	return p->num;
return -1;
}

/*
 *      Getnum4table - Find the index of the table in the DTlist array
 */

int getnum4table(const char *name)
{
search_st *p;

if(DTtot<1)
	return -1;

p = FIND(name,tables,DTtot);
if(p)
	return p->num;
return -1;
}

/*
 *      Getstring4name - 
 */

char *getstring4name(const char *name)
{
if(STtot<1)
	return NULL;
ST_ITEM *str = (ST_ITEM *)bsearch(name,STlist,STtot,sizeof(ST_ITEM),CMP_search_ST);
/*
ST_ITEM *str=NULL;
for(int ctr=0;ctr<STtot;ctr++)
	{
	printf("Look for '%s' vs '%s'\n",name,STlist[ctr].name);
	if(!stricmp(name,STlist[ctr].name))
		{
		str=&STlist[ctr];
		break;
		}
	}
*/
	
if(str)
	return str->data;
return NULL;
}

/*
 *      Getstringtitle4name - 
 */

char *getstringtitle4name(const char *name)
{
if(STtot<1)
	return NULL;
ST_ITEM *str = (ST_ITEM *)bsearch(name,STlist,STtot,sizeof(ST_ITEM),CMP_search_ST);
if(str)
	return str->title;
return NULL;
}





/*
 *      Return a pointer to a data item in a table specified by its index
 *      by searching the table for a string
 */

DT_ITEM *GetTableNum_s(int table,const char *name)
{
if(DTtot<1)
	return NULL;
return (DT_ITEM *)bsearch(name,DTlist[table].list,DTlist[table].entries,sizeof(DT_ITEM),CMP_search_DTS);
}

/*
 *      Return a pointer to a data item in a table specified by its name
 *      by searching the table for a string
 */

DT_ITEM *GetTableName_s(const char *tablename,const char *name)
{
int table;
table = getnum4table(tablename);
if(table < 0)
	{
	Bug("Get_Data: Cannot find table called '%s' in section: tables\n",tablename);
	return NULL;
	}
return (DT_ITEM *)bsearch(name,DTlist[table].list,DTlist[table].entries,sizeof(DT_ITEM),CMP_search_DTS);
}

/*
 *      Return a pointer to a data item in a table specified by its index
 *      by searching the table for a number
 */

DT_ITEM *GetTableNum_i(int table,long num)
{
if(DTtot<1)
	return NULL;
if(table < 0)
	return NULL;
return (DT_ITEM *)bsearch((void *)num,DTlist[table].list,DTlist[table].entries,sizeof(DT_ITEM),CMP_search_DTI);
}

/*
 *      Return a pointer to a data item in a table specified by its name
 *      by searching the table for a string
 */

DT_ITEM *GetTableName_i(const char *tablename, long num)
{
int table;
table = getnum4table(tablename);
if(table == -1)
	return NULL;
return (DT_ITEM *)bsearch((void *)num,DTlist[table].list,DTlist[table].entries,sizeof(DT_ITEM),CMP_search_DTI);
}

/*
 *      Return a pointer to a data item in a table specified by its name
 *      by searching the table for a string.  This is a case-sensitive search
 */

DT_ITEM *GetTableCase(const char *tablename,const char *name)
{
int table,ctr;
table = getnum4table(tablename);
if(table < 0)
	{
	Bug("Get_Data: Cannot find table called '%s' in section: tables\n",tablename);
	return NULL;
	}

// Make sure table datatypes are as expected
if(DTlist[table].keytype != 's' || DTlist[table].listtype != 's')
	return NULL;

for(ctr=0;ctr<DTlist[table].entries;ctr++)
	if(!strcmp(DTlist[table].list[ctr].ks,name))
		return &DTlist[table].list[ctr];
return NULL;
}


/*
 *    Get quick random number
 */

int qrand()
{
static int qptr=0;
qptr++;
if(qptr>QRAND_MAX)
	qptr=0;

return qrt[qptr];
}

/*
 *    Initialise quick random number generator
 */

void qrand_init()
{
int ctr;
for(ctr=0;ctr<QRAND_MAX;ctr++)
	qrt[ctr]=rand();
}

/*
 *   GetAvg - Get average colour for a tile
 */

int GetAvg(IREBITMAP *b)
{
return b->GetPixel(0,0);
//return getpixel(b,(b->w/2)+1,(b->h/2)+1);
}
