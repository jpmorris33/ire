//
// Miscellaneous savegame State (legacy version without uuids!)
//



#include <stdio.h>
#include <string.h>

#include "ithelib.h"
#include "media.hpp"
#include "console.hpp"
#include "core.hpp"
#include "gamedata.hpp"
#include "loadsave.hpp"
#include "oscli.hpp"
#include "cookies.h"
#include "object.hpp"
#include "nuspeech.hpp"
#include "loadfile.hpp"
#include "linklist.hpp"
#include "resource.hpp"
#include "init.hpp"
#include "project.hpp"
#include "textfile.h"
#include "pe/pe_api.hpp"

extern OBJECT *find_id(unsigned int id);
extern void read_USEDATA(USEDATA *u,IFILE *f);


/*
 *      load_ms - Read miscellaneous savegame data
 */

void load_ms(char *filename)
{
IFILE *ifp;
char buf[256];
char name48[48];
int num,ctr,maxnum,slot,ok;
unsigned int ut,ctr2,ctr3,ctr4,newpos,saveid,dark;
OBJECT *o,*o2;
USEDATA usedata;
GLOBALINT *globalint;
GLOBALPTR *globalptr;
char *journalname;

int act;

// Open the file.

ifp=iopen(filename);

// Read the header
memset(buf,0,sizeof(buf));

newpos = itell(ifp);
iread((unsigned char *)buf,8,ifp);			 // Cookie
if(!strncmp(COOKIE_PWAD,buf,4))
	{
	// Good, wind back to start
	iseek(ifp,0,newpos);
	}
else
	{
	// Okay, it's not a straight PWAD, look for the original header instead
	iseek(ifp,0,newpos);
	iread((unsigned char *)buf,40,ifp);			 // Cookie
	if(strncmp(OLDCOOKIE_MS,buf,4))
		{
		// Wrong
		iclose(ifp);
		return;
		}
	}

if(MZ1_SavingGame)
	goto reload_game;	// Naughty Doug
	
if(!GetWadEntry(ifp,"PARTYBLK"))
	ithe_panic("Savegame corrupted","PARTYBLK not found");

saveid = igetl_i(ifp);
player=find_id(saveid);
#ifdef DEBUG_SAVE
ilog_quiet("player id = %d\n",saveid);
#endif
if(!player)
	ithe_panic("load_ms: Player AWOL in savegame",NULL);

// Erase existing party
slot=0;
for(ctr=0;ctr<MAX_MEMBERS-1;ctr++)
	{
	party[ctr]=0;
	partyname[ctr][0]='\0';
	}

// Load new party
for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	{
	saveid = igetl_i(ifp);
	if(saveid)
		{
		o = find_id(saveid);      // ID numbers change to objptrs
		
		ok=1;
		for(ctr2=0;ctr2<MAX_MEMBERS;ctr2++)
			if(party[ctr2] == o)
				{
				ok=0;
				break;
				}
			  
		if(ok)
			{
			party[slot]=o;
			SetNPCFlag(party[slot],IN_PARTY);
			slot++;
			}
		}
	}

if(!GetWadEntry(ifp,"FLAGSBLK"))
	ithe_panic("Savegame corrupted","FLAGSBLK not found");

// Load user flags

Wipe_tFlags();
ut=igetl_i(ifp);
for(ctr2=0;ctr2<ut;ctr2++)
	{
	num=igetb(ifp);
	iread((unsigned char *)buf,num,ifp);
//	ilog_quiet("flag: '%s'\n",buf);
	Set_tFlag(buf,1);
	}

// Load time/date

if(!GetWadEntry(ifp,"TIMEDATE"))
	ithe_panic("Savegame corrupted","TIMEDATE not found");

game_minute = igetl_i(ifp);
game_hour   = igetl_i(ifp);
game_day    = igetl_i(ifp);
game_month  = igetl_i(ifp);
game_year   = igetl_i(ifp);
days_passed = igetl_i(ifp);
day_of_week = igetl_i(ifp);
dark = igetl_i(ifp);
if(dark >= 0 && dark < 256)	// If it's an old savegame, this will be junk
	SetDarkness(dark);

ilog_quiet("game time = %02d:%02d\n",game_hour,game_minute);

// Old format!
if(GetWadEntry(ifp,"JOURNAL "))	{
	int id,day;
	// Load journal entries
	J_Free();

	ut=igetl_i(ifp);
	for(ctr2=0;ctr2<ut;ctr2++) {
		id=igetl_i(ifp);	// ID
		day=igetl_i(ifp);	// day
		num=igetl_i(ifp);	
		iread((unsigned char *)buf,num,ifp); // Date
		buf[32]=0;
		JOURNALENTRY *j=J_Add(id,day,buf);
		if(!j)
			ithe_panic("Savegame error","Out of memory allocating journal entry");
		if(id != -1) {
			// Read title, then text
			num=igetl_i(ifp);
			j->title = (char *)M_get(1,num+1);
			iread((unsigned char *)j->title,num,ifp);
			num=igetl_i(ifp);
			j->text = (char *)M_get(1,num+1);
			iread((unsigned char *)j->text,num,ifp);
		}
	}
}

// New format
if(GetWadEntry(ifp,"JOURNAL2"))	{
	int id,day;
	// Load journal entries
	J_Free();

	ut=igetl_i(ifp);
	for(ctr2=0;ctr2<ut;ctr2++) {
		id=igetl_i(ifp);

		num=igetl_i(ifp);
		journalname = (char *)M_get(1,num+1);
		iread((unsigned char *)journalname,num,ifp);
		if(id != -1) {
			id=getnum4stringname(journalname);
			if(id == -1) {
				ithe_panic("Savegame corrupted, journal entry not found",journalname);
			}
		}

		day=igetl_i(ifp);	// day
		num=igetl_i(ifp);
		iread((unsigned char *)buf,num,ifp); // Date
		buf[32]=0;
		JOURNALENTRY *j=J_Add(id,day,buf);
		if(!j) {
			ithe_panic("Savegame error","Out of memory allocating journal entry");
		}
		if(id == -1) {
			// Read title, then text
			num=igetl_i(ifp);
			j->title = (char *)M_get(1,num+1);
			iread((unsigned char *)j->title,num,ifp);
			num=igetl_i(ifp);
			j->text = (char *)M_get(1,num+1);
			iread((unsigned char *)j->text,num,ifp);
			num=igetl_i(ifp);
			j->tag = (char *)M_get(1,num+1);
			iread((unsigned char *)j->tag,num,ifp);

			j->name = journalname;
		} else {
			M_free(journalname);
		}
		j->status = igetl_i(ifp); // status code
	}
}


// If we're doing a map switch, reload from here:

reload_game:

// Load current goal data
if(!GetWadEntry(ifp,"GOALDATA"))
	ithe_panic("Savegame corrupted","GOALDATA not found");

num = igetl_i(ifp);
#ifdef DEBUG_SAVE
ilog_quiet("%d goals\n",num);
#endif
for(ctr=0;ctr<num;ctr++)
	{
	saveid = igetl_i(ifp);
	o = find_id(saveid);
	if(!o)
		{
		Bug("Goal: Cannot find object number %d\n",saveid);
//		ithe_panic("Oh well whatever","Never mind");
		}
	iread((unsigned char *)buf,32,ifp);
	saveid = igetl_i(ifp);
	o2=NULL;
	if(saveid != SAVEID_INVALID)
		{
		o2 = find_id(saveid);
		if(!o2)
			{
			Bug("Goal: Cannot find target object number %d\n",saveid);
//			ithe_panic("Oh well whatever","Never mind");
			}
		}
	if(o)
		{
		SubAction_Wipe(o); // Erase any sub-tasks
		ActivityName(o,buf,o2);
		}
	}

// Load usedata
if(!GetWadEntry(ifp,"USEDATA "))
	ithe_panic("Savegame corrupted","USEDATA not found");

num = igetl_i(ifp);
ctr2 = igetl_i(ifp);
#ifdef DEBUG_SAVE
ilog_printf("Reading %d usedata entries:\n",num);
#endif
for(ctr=0;ctr<num;ctr++)
	{
	saveid = igetl_i(ifp);
	o = find_id(saveid);
	if(!o)
		{
		ilog_quiet("Load_miscellaneous_state (usedata): can't load object %d\n",saveid);
//		ithe_panic("Load_miscellaneous_state (usedata): Can't find object","Savegame corrupted");
		}

	if(ctr2 == USEDATA_VERSION)
		read_USEDATA(&usedata,ifp);
	else
		{
		// Not good, but try to do it anyway:
		newpos = itell(ifp)+ctr2;
		read_USEDATA(&usedata,ifp);
		iseek(ifp,newpos,SEEK_SET);
		}

	if(o)
		{
		NPC_RECORDING *oldn,*oldl; 	// These should be stable enough to survive

		oldn=o->user->npctalk;
		oldl=o->user->lFlags;

		memcpy(o->user,&usedata,sizeof(USEDATA));
		// Now we must clear all the pointers to prevent death
		o->user->arcpocket.objptr=NULL;
		o->user->pathgoal.objptr=NULL;
	
		o->user->npctalk=oldn;
		o->user->lFlags=oldl;

		// Delete activity lists too
		memset(o->user->actlist,0,sizeof(o->user->actlist));
		memset(o->user->acttarget,0,sizeof(o->user->acttarget)); // Delete list
		}
	}

// Load secondary goal data (sub-tasks or subactions)

// 'SUBTASKS' is the old format, but is entirely dependent on the PElist remaining the same, which it won't.
// We can import old savegames by simply ignoring it

// Load secondary goal data (sub-tasks or subactions)
if(!GetWadEntry(ifp,"SUBTASK2")) {
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++) {
		saveid = igetl_i(ifp);
		o = find_id(saveid);
		if(!o) {
			Bug("SubTasks: Cannot find object number %d\n",saveid);
		}

		for(ctr2=0;ctr2<ACT_STACK;ctr2++) {
			iread((unsigned char *)buf,32,ifp);
			saveid = igetl_i(ifp);

			if(!strcmp(buf,"-")) {
				continue;
			}

			act=getnum4PE(buf);
			if(act != -1) {
				if(saveid == SAVEID_INVALID) {
					SubAction_Push(o,act,NULL);
//					ilog_quiet("restore queue: %s does %s\n",o->name,PElist[act].name);
				} else {
					o2 = find_id(saveid);
					if(!o2) {
						Bug("Subtask: Cannot find target object number %d\n",saveid);
					}
//					ilog_quiet("restore queue: %s does %s to %s\n",o->name,PElist[act].name,o2->name);
					SubAction_Push(o,act,o2);
				}
			} else {
				Bug("Subtask: did not find function '%s'\n", buf);
			}
		}
	}
}

// load record of what we've said to NPCs (OLD FORMAT!)
if(GetWadEntry(ifp,"NPCTALKS"))
	{
	num = igetl_i(ifp);
	#ifdef DEBUG_SAVE
	ilog_quiet("Reading %d npctalks\n",num);
	#endif
	for(ctr=0;ctr<num;ctr++)
		{
		saveid = igetl_i(ifp);
		o = find_id(saveid);
		if(!o)
			{
			ilog_quiet("looking for %d\n",saveid);
			ilog_quiet("WARNING: Did not find object in NPCtalk(1) while loading map \n");
			if(fullrestore)
				ithe_panic("Load_miscellaneous_state (NPCtalk): Can't find object","Savegame corrupted");
			}
		ut = igetl_i(ifp); // number of entries for this NPC
		for(ctr2=0;ctr2<ut;ctr2++)
			{
			ctr3=igetl_i(ifp);
			ctr4=igetb(ifp)&0xff; // Limit to 255 for safety
			iread((unsigned char *)buf,ctr4,ifp);
			o2 = find_id(ctr3);

	#ifdef DEBUG_SAVE
			ilog_quiet("npc %d spoke to %d\n",saveid,ctr3);
	#endif

			if(!o2)
				{
				ilog_quiet("looking for %d, player's id is %d\n",ctr3,player->save_id);
				ilog_quiet("WARNING: Did not find player in NPCtalk(2) while loading map \n");
				if(fullrestore)
					ithe_panic("Load_miscellaneous_state (NPCtalk): Can't find 'player'","Savegame corrupted");
				}
	#ifdef DEBUG_SAVE
		ilog_quiet("BeenRead: %x,%s,%x\n",o,buf,o2);
	#endif
			if(o && o2)
				{
				char *bufname = NPC_MakeName(o2);
				NPC_BeenRead(o,buf,bufname);
				M_free(bufname);
				}
			}
		}
	}

// load record of what we've said to NPCs
if(GetWadEntry(ifp,"NPCTALK2"))
	{
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		saveid = igetl_i(ifp);
		o = find_id(saveid);
		if(!o)
			{
			ilog_quiet("looking for %d\n",saveid);
			ilog_quiet("WARNING: Did not find object in NPCtalk(1) while loading map \n");
			if(fullrestore)
				ithe_panic("Load_miscellaneous_state (NPCtalk): Can't find object","Savegame corrupted");
			}
		ut = igetl_i(ifp); // number of entries for this NPC
		for(ctr2=0;ctr2<ut;ctr2++)
			{
			ctr4=igetl_i(ifp);
			char *bufname = (char *)M_get(1,ctr4+1);
			iread((unsigned char *)bufname,ctr4,ifp);
		
			ctr4=(unsigned char)igetb(ifp);
			iread((unsigned char *)buf,ctr4,ifp);
			if(o)
				NPC_BeenRead(o,buf,bufname);
			M_free(bufname);
			}
		}
	}

// load record of local NPC flags (Legacy format)
if(GetWadEntry(ifp,"NPCLFLAG"))
	{
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		saveid = igetl_i(ifp);
		o = find_id(saveid);
		if(!o)
			{
			ilog_quiet("can't find object %d (0x%x)\n",saveid,saveid);
			ilog_quiet("WARNING: Did not find object in lFlags(1) while loading map \n");
			if(fullrestore)
				ithe_panic("Load_miscellaneous_state (lFlags): Can't find object","Savegame corrupted");
			}
			
		ut = igetl_i(ifp); // number of entries for this NPC
		for(ctr2=0;ctr2<ut;ctr2++)
			{
			ctr3=igetl_i(ifp);
			ctr4=(unsigned char)igetb(ifp);
			iread((unsigned char *)buf,ctr4,ifp);
			o2 = find_id(ctr3);
			if(!o2)
				{
				ilog_quiet("can't find object %d (0x%x)\n",saveid,saveid);
				ilog_quiet("WARNING: Did not find object in lFlags(2) while loading map \n");
				if(fullrestore)
					ithe_panic("Load_miscellaneous_state (lFlags): Can't find 'player'","Savegame corrupted");
				}
			if(o && o2)
				{
				char *bufname = NPC_MakeName(o2);
				NPC_set_lFlag(o,buf,bufname,1);
				M_free(bufname);
				}
			}
		}
	}
	
// load record of local NPC flags
if(GetWadEntry(ifp,"NPCLFLG2"))
	{
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		saveid = igetl_i(ifp);
		o = find_id(saveid);
		if(!o)
			{
			ilog_quiet("can't find object %d (0x%x)\n",saveid,saveid);
			ilog_quiet("WARNING: Did not find object in lFlags(1) while loading map \n");
			if(fullrestore)
				ithe_panic("Load_miscellaneous_state (lFlags): Can't find object","Savegame corrupted");
			}
			
		ut = igetl_i(ifp); // number of entries for this NPC
		for(ctr2=0;ctr2<ut;ctr2++)
			{
			ctr4=igetl_i(ifp);
			char *bufname = (char *)M_get(1,ctr4+1);
			iread((unsigned char *)bufname,ctr4,ifp);
		
			ctr4=(unsigned char)igetb(ifp);
			iread((unsigned char *)buf,ctr4,ifp);
			if(o)
				NPC_set_lFlag(o,buf,bufname,1);
			M_free(bufname);
			}
		}
	}
	

// Load wielded objects
if(!GetWadEntry(ifp,"NPCWIELD"))
	ithe_panic("Savegame corrupted","NPCWIELD not found");
	
num = igetl_i(ifp);
ctr2 = igetl_i(ifp);
for(ctr=0;ctr<num;ctr++) {
	saveid = igetl_i(ifp);
	o = find_id(saveid);
	if(!o)
		ithe_panic("Load_miscellaneous_state (wieldtab): Can't find object","Savegame corrupted");

	if(fullrestore || !GetNPCFlag(o,IN_PARTY)) {
		memset(o->wield,0,sizeof(WIELD)); // Default to blank, unless a party member changing map
	}

	for(ctr3=0;ctr3<ctr2;ctr3++) {
		saveid = igetl_i(ifp);
		if(saveid != SAVEID_INVALID) {
			o2 = find_id(saveid);
			if(!o2) {
				ilog_quiet("Did not find id %ld\n",saveid);
				ilog_quiet("WARNING: Did not find object in wieldtab(2) while loading map \n");
				if(fullrestore)
					ithe_panic("Load_miscellaneous_state (wieldtab2): Can't find object","Savegame corrupted");
			}

			// Don't actually do it if the map is changing and it's a party member being done
			// Otherwise, the objects they're wielding will change from one map to the other
			if(fullrestore || !GetNPCFlag(o,IN_PARTY))	{
				if(o2) {
					SetWielded(o,ctr3,o2);
				}
			}
		}
	}
}

// Load Global Integers (old format for backwards compatibility only)
if(fullrestore)	// Not for changing map
	{
	if(GetWadEntry(ifp,"GLOBALVI"))
		{
		globalint = GetGlobalIntlist(&maxnum);
		if(!globalint)
			maxnum=0;

		num= igetl_i(ifp);
		for(ctr=0;ctr<num;ctr++)
			{
			ctr2 = igetl_i(ifp);
			// Protect against array overflow while still reading in the data from disk
			if(ctr<maxnum)
				if(globalint[ctr].ptr)
					{
					//ilog_quiet("Globalvi: Setting %s to %d\n",globalint[ctr].name,ctr2);
					*globalint[ctr].ptr = ctr2;
					}
			}
		}
	}

// Load Global Objects (old format!)
if(GetWadEntry(ifp,"GLOBALVO"))
	{
	globalptr = GetGlobalPtrlist(&maxnum);
	if(!globalptr)
		maxnum=0;

	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		ctr2 = igetl_i(ifp);
		o = find_id(ctr2);

		if(ctr<maxnum)
			if(globalptr[ctr].ptr)
				{
//				ilog_quiet("Globalvo: Setting %s to %p\n",globalptr[ctr].name,o);
				*(OBJECT **)globalptr[ctr].ptr = o;
				}
		}
	}

// Now try the new formats

// Load Global Integers
if(fullrestore)	// Not for changing map
	{
	if(GetWadEntry(ifp,"GLOBALI2"))
		{
		num = igetl_i(ifp);
		for(ctr=0;ctr<num;ctr++)
			{
			iread((unsigned char *)name48,48,ifp);
			ctr2 = igetl_i(ifp);
		
			globalint = FindGlobalInt(name48);
			if(globalint && globalint->ptr && (!globalint->transient))
				{
//				ilog_quiet("Globali2: Setting %s to %d\n",name48,ctr2);
				*globalint->ptr = ctr2;
				}
			}
		}
	}

// Load Global Objects
if(GetWadEntry(ifp,"GLOBALO2"))
	{
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		iread((unsigned char *)name48,48,ifp);
		ctr2 = igetl_i(ifp);
		o = find_id(ctr2);

		globalptr = FindGlobalPtr(name48);
		if(globalptr && globalptr->ptr && (!globalptr->transient))
			{
			ilog_quiet("Globalo2: Setting %s to %p\n",name48,o);
			*(OBJECT **)(globalptr->ptr) = o;
			}
		}
	}

// Load goal destinations
if(GetWadEntry(ifp,"PATHGOAL"))
	{
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		// Get object to set
		ctr2 = igetl_i(ifp);
		o = find_id(ctr2);

		// Get object in path goal (if any)
		ctr2 = igetl_i(ifp);
		o2 = find_id(ctr2);

		// Set it up
		if(o && o->user)
			o->user->pathgoal.objptr = o2;
		}
	}

// Load tile animation states
if(GetWadEntry(ifp,"TILEANIM"))
	{
	num = igetl_i(ifp);
	if(num > TItot)
		num=TItot;
	for(ctr=0;ctr<num;ctr++)
		{
		TIlist[ctr].sptr=igetl_i(ifp);
		TIlist[ctr].sdir=igetl_i(ifp);
		TIlist[ctr].tick=igetl_i(ifp);
		TIlist[ctr].sdx=igetl_i(ifp);
		TIlist[ctr].sdy=igetl_i(ifp);
		TIlist[ctr].sx=igetl_i(ifp);
		TIlist[ctr].sy=igetl_i(ifp);
		}
	}

iclose(ifp);
}


